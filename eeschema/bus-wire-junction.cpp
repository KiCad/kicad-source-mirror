/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file bus-wire-junction.cpp
 * @brief Code for editing buses, wires, and junctions.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>

#include <lib_draw_item.h>
#include <lib_pin.h>
#include <general.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_text.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_view.h>
#include <view/view_group.h>



static void AbortCreateNewLine( EDA_DRAW_PANEL* aPanel, wxDC* aDC );
static void ComputeBreakPoint( SCH_SCREEN* aScreen, SCH_LINE* aSegment, wxPoint& new_pos );

static DLIST< SCH_ITEM > s_wires;       // when creating a new set of wires,
                                        // stores here the new wires.


/**
 * In a contiguous list of wires, remove wires that backtrack over the previous
 * wire. Example:
 *
 * Wire is added:
 * ---------------------------------------->
 *
 * A second wire backtracks over it:
 * -------------------<====================>
 *
 * RemoveBacktracks is called:
 * ------------------->
 */
static void RemoveBacktracks( DLIST<SCH_ITEM>& aWires )
{
    EDA_ITEM* first = aWires.GetFirst();
    std::vector<SCH_LINE*> last_lines;

    for( EDA_ITEM* p = first; p; )
    {
        SCH_LINE *line = static_cast<SCH_LINE*>( p );
        p = line->Next();

        if( line->IsNull() )
        {
            delete s_wires.Remove( line );
            continue;
        }

        if( !last_lines.empty() )
        {
            SCH_LINE* last_line = last_lines[last_lines.size() - 1];
            bool contiguous = ( last_line->GetEndPoint() == line->GetStartPoint() );
            bool backtracks = IsPointOnSegment( last_line->GetStartPoint(),
                    last_line->GetEndPoint(), line->GetEndPoint() );
            bool total_backtrack = ( last_line->GetStartPoint() == line->GetEndPoint() );

            if( contiguous && backtracks )
            {
                if( total_backtrack )
                {
                    delete s_wires.Remove( last_line );
                    delete s_wires.Remove( line );
                    last_lines.pop_back();
                }
                else
                {
                    last_line->SetEndPoint( line->GetEndPoint() );
                    delete s_wires.Remove( line );
                }
            }
            else
            {
                last_lines.push_back( line );
            }
        }
        else
        {
            last_lines.push_back( line );
        }
    }
}


/**
 * Mouse capture callback for drawing line segments.
 */
static void DrawSegment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition, bool aErase )
{
    if( s_wires.GetCount() == 0 )
        return;

    SCH_EDIT_FRAME* frame = (SCH_EDIT_FRAME*) aPanel->GetParent();

    wxPoint endpos = frame->GetCrossHairPosition();

    if( frame->GetForceHVLines() ) /* Coerce the line to vertical or horizontal one: */
        ComputeBreakPoint( frame->GetScreen(), (SCH_LINE*) s_wires.GetLast()->Back(), endpos );
    else
        ( (SCH_LINE*) s_wires.GetLast() )->SetEndPoint( endpos );

    auto view = static_cast<SCH_DRAW_PANEL*>( aPanel )->GetView();

    view->ClearPreview();

    for( SCH_LINE* segment = (SCH_LINE*) s_wires.begin(); segment; segment = segment->Next() )
    {
        if( !segment->IsNull() )  // Add to preview if segment length != 0
            view->AddToPreview( segment->Clone() );
    }
}


void SCH_EDIT_FRAME::BeginSegment( int type )
{
    SCH_LINE* segment;
    SCH_LINE* nextSegment;
    wxPoint   cursorpos = GetCrossHairPosition();

    // We should know if a segment is currently in progress
    segment = (SCH_LINE*) GetScreen()->GetCurItem();
    if( segment )   // a current item exists, but not necessary a currently edited item
    {
        if( !segment->GetFlags() || ( segment->Type() != SCH_LINE_T ) )
        {
            if( segment->GetFlags() )
                wxLogDebug( wxT( "BeginSegment: item->GetFlags()== %X" ), segment->GetFlags() );

            // no wire, bus or graphic line in progress
            segment = NULL;
        }
    }

    if( !segment )      // first point : Create the first wire or bus segment
    {
        switch( type )
        {
        default:
            segment = new SCH_LINE( cursorpos, LAYER_NOTES );
            break;

        case LAYER_WIRE:
            segment = new SCH_LINE( cursorpos, LAYER_WIRE );

            /* A junction will be created later, when we'll know the
             * segment end position, and if the junction is really needed */
            break;

        case LAYER_BUS:
            segment = new SCH_LINE( cursorpos, LAYER_BUS );
            break;
        }

        segment->SetFlags( IS_NEW );
        s_wires.PushBack( segment );
        GetScreen()->SetCurItem( segment );

        // We need 2 segments to go from a given start pin to an end point when the horizontal
        // and vertical lines only switch is on.
        if( GetForceHVLines() )
        {
            nextSegment = new SCH_LINE( *segment );
            nextSegment->SetFlags( IS_NEW );
            s_wires.PushBack( nextSegment );
            GetScreen()->SetCurItem( nextSegment );
        }

        m_canvas->SetMouseCapture( DrawSegment, AbortCreateNewLine );
        SetRepeatItem( NULL );
    }
    else    // A segment is in progress: terminates the current segment and add a new segment.
    {
        SCH_LINE* prevSegment = segment->Back();

        // Be aware prevSegment can be null when the horizontal and vertical lines only switch
        // is off when we create the first segment.

        if( !GetForceHVLines() )
        {
            // If only one segment is needed and it has a zero length, do not create a new one.
            if( segment->IsNull() )
                return;
        }
        else
        {
            wxCHECK_RET( prevSegment != NULL, wxT( "Failed to create second line segment." ) );

            // If two segments are required and they both have zero length, do not
            // create a new one.
            if( prevSegment && prevSegment->IsNull() && segment->IsNull() )
                return;
        }

        m_canvas->CallMouseCapture( nullptr, wxDefaultPosition, false );

        // Terminate the command if the end point is on a pin, junction, or another wire or bus.
        if( GetScreen()->IsTerminalPoint( cursorpos, segment->GetLayer() ) )
        {
            EndSegment();
            return;
        }

        // Create a new segment, and chain it after the current new segment.
        nextSegment = new SCH_LINE( *segment );
        nextSegment->SetStartPoint( cursorpos );
        s_wires.PushBack( nextSegment );

        segment->SetEndPoint( cursorpos );
        segment->ClearFlags( IS_NEW );
        segment->SetFlags( SELECTED );
        nextSegment->SetFlags( IS_NEW );
        GetScreen()->SetCurItem( nextSegment );
        m_canvas->CallMouseCapture( nullptr, wxDefaultPosition, false );
    }
}


void SCH_EDIT_FRAME::GetSchematicConnections( std::vector< wxPoint >& aConnections )
{
    for( SCH_ITEM* item = GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        // Avoid items that are changing
        if( !( item->GetFlags() & ( IS_DRAGGED | IS_MOVED | IS_DELETED ) ) )
            item->GetConnectionPoints( aConnections );
    }

    // We always have some overlapping connection points.  Drop duplicates here
    std::sort( aConnections.begin(), aConnections.end(),
            []( const wxPoint& a, const wxPoint& b ) -> bool
            { return a.x < b.x || (a.x == b.x && a.y < b.y); } );
    aConnections.erase( unique( aConnections.begin(), aConnections.end() ), aConnections.end() );
}


bool SCH_EDIT_FRAME::TestDanglingEnds()
{
    std::vector<DANGLING_END_ITEM> endPoints;
    bool hasStateChanged = false;

    for( SCH_ITEM* item = GetScreen()->GetDrawList().begin(); item; item = item->Next() )
        item->GetEndPoints( endPoints );

    for( SCH_ITEM* item = GetScreen()->GetDrawList().begin(); item; item = item->Next() )
    {
        if( item->UpdateDanglingState( endPoints ) )
        {
            GetCanvas()->GetView()->Update( item, KIGFX::REPAINT );
            hasStateChanged = true;
        }
    }

    return hasStateChanged;
}


void SCH_EDIT_FRAME::EndSegment()
{
    SCH_SCREEN* screen = GetScreen();
    SCH_LINE* segment = (SCH_LINE*) screen->GetCurItem();
    PICKED_ITEMS_LIST itemList;

    if( segment == NULL || segment->Type() != SCH_LINE_T || !segment->IsNew() )
        return;

    // Remove zero length segments and segments backtracking over other segments.
    RemoveBacktracks( s_wires );

    if( s_wires.GetCount() == 0 )
    {
        screen->ClearDrawingState();
        screen->SetCurItem( NULL );
        return;
    }

    // Collect the possible connection points for the new lines
    std::vector< wxPoint > connections;
    std::vector< wxPoint > new_ends;
    GetSchematicConnections( connections );

    // Check each new segment for possible junctions and add/split if needed
    for( SCH_ITEM* wire = s_wires.GetFirst(); wire; wire=wire->Next() )
    {
        SCH_LINE* test_line = (SCH_LINE*) wire;
        if( wire->GetFlags() & SKIP_STRUCT )
            continue;

        wire->GetConnectionPoints( new_ends );

        for( auto i : connections )
        {
            if( IsPointOnSegment( test_line->GetStartPoint(), test_line->GetEndPoint(), i ) )
            {
                new_ends.push_back( i );
            }
        }
        itemList.PushItem( ITEM_PICKER( wire, UR_NEW ) );
    }

    // Get the last non-null wire (this is the last created segment).
    SetRepeatItem( segment = (SCH_LINE*) s_wires.GetLast() );

    // Add the new wires
    AddToScreen( s_wires );

    auto view = GetCanvas()->GetView();
    view->ClearPreview();
    view->ShowPreview( false );
    view->ClearHiddenFlags();

    SaveCopyInUndoList(itemList, UR_NEW);

    // Correct and remove segments that need to be merged.
    SchematicCleanUp( true );

    for( auto item = GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        std::vector< wxPoint > pts;
        item->GetConnectionPoints( pts );

        if( pts.size() > 2 )
            continue;

        for( auto i = pts.begin(); i != pts.end(); i++ )
            for( auto j = i + 1; j != pts.end(); j++ )
                    TrimWire( *i, *j, true );
    }

    for( auto i : new_ends )
    {
        if( screen->IsJunctionNeeded( i, true ) )
            AddJunction( i, true );
    }

    TestDanglingEnds();
    screen->ClearDrawingState();
    screen->SetCurItem( NULL );
    m_canvas->EndMouseCapture( -1, -1, wxEmptyString, false );
    m_canvas->Refresh();
    OnModify();
}


// A helper function to find any sheet pins at the specified position.
static const SCH_SHEET_PIN* getSheetPin( SCH_SCREEN* aScreen, const wxPoint& aPosition )
{
    for( SCH_ITEM* item = aScreen->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;

            for( const SCH_SHEET_PIN& pin : sheet->GetPins() )
            {
                if( pin.GetPosition() == aPosition )
                    return &pin;
            }
        }
    }

    return nullptr;
}


/**
 * Function ComputeBreakPoint
 * computes the middle coordinate for 2 segments from the start point to \a aPosition
 * with the segments kept in the horizontal or vertical axis only.
 *
 * @param aSegment A pointer to a #SCH_LINE object containing the first line break point
 *                 to compute.
 * @param aPosition A reference to a wxPoint object containing the coordinates of the
 *                  position used to calculate the line break point.
 */
static void ComputeBreakPoint( SCH_SCREEN* aScreen, SCH_LINE* aSegment, wxPoint& aPosition )
{
    wxCHECK_RET( aSegment != nullptr, wxT( "Cannot compute break point of NULL line segment." ) );

    SCH_LINE* nextSegment = aSegment->Next();

    wxPoint midPoint;
    int iDx = aSegment->GetEndPoint().x - aSegment->GetStartPoint().x;
    int iDy = aSegment->GetEndPoint().y - aSegment->GetStartPoint().y;

    const SCH_SHEET_PIN* connectedPin = getSheetPin( aScreen, aSegment->GetStartPoint() );
    auto force = connectedPin ? connectedPin->GetEdge() : SCH_SHEET_PIN::SHEET_UNDEFINED_SIDE;

    if( force == SCH_SHEET_PIN::SHEET_LEFT_SIDE || force == SCH_SHEET_PIN::SHEET_RIGHT_SIDE )
    {
        if( aPosition.x == connectedPin->GetPosition().x )  // push outside sheet boundary
        {
            int direction = ( force == SCH_SHEET_PIN::SHEET_LEFT_SIDE ) ? -1 : 1;
            aPosition.x += aScreen->GetGridSize().x * direction;
        }

        midPoint.x = aPosition.x;
        midPoint.y = aSegment->GetStartPoint().y;     // force horizontal
    }
    else if( iDy != 0 )    // keep the first segment orientation (vertical)
    {
        midPoint.x = aSegment->GetStartPoint().x;
        midPoint.y = aPosition.y;
    }
    else if( iDx != 0 )    // keep the first segment orientation (horizontal)
    {
        midPoint.x = aPosition.x;
        midPoint.y = aSegment->GetStartPoint().y;
    }
    else
    {
        if( std::abs( aPosition.x - aSegment->GetStartPoint().x ) <
            std::abs( aPosition.y - aSegment->GetStartPoint().y ) )
        {
            midPoint.x = aSegment->GetStartPoint().x;
            midPoint.y = aPosition.y;
        }
        else
        {
            midPoint.x = aPosition.x;
            midPoint.y = aSegment->GetStartPoint().y;
        }
    }

    aSegment->SetEndPoint( midPoint );
    nextSegment->SetStartPoint( midPoint );
    nextSegment->SetEndPoint( aPosition );
}


void SCH_EDIT_FRAME::DeleteCurrentSegment()
{
    SCH_SCREEN* screen = GetScreen();

    SetRepeatItem( NULL );

    if( ( screen->GetCurItem() == NULL ) || !screen->GetCurItem()->IsNew() )
        return;

    RemoveFromScreen( screen->GetCurItem() );
    m_canvas->SetMouseCaptureCallback( NULL );
    screen->SetCurItem( NULL );
}


void SCH_EDIT_FRAME::SaveWireImage()
{
    PICKED_ITEMS_LIST oldItems;
    oldItems.m_Status = UR_WIRE_IMAGE;

    SCH_ITEM* item;
    SCH_ITEM* next_item;

    for( item = GetScreen()->GetDrawItems(); item; item = next_item )
    {
        next_item = item->Next();

        if( item->Type() == SCH_JUNCTION_T || item->Type() == SCH_LINE_T )
        {
            GetScreen()->Remove( item );
            GetCanvas()->GetView()->Remove( item );

            oldItems.PushItem( ITEM_PICKER( item, UR_WIRE_IMAGE ) );

            SCH_ITEM* item_copy = static_cast<SCH_ITEM*>( item->Clone() );

            GetScreen()->GetDrawList().Insert( item_copy, next_item );
            GetCanvas()->GetView()->Add( item_copy );
        }
    }

    if( oldItems.GetCount() != 0 )
        SaveCopyInUndoList( oldItems, UR_WIRE_IMAGE );
}


bool SCH_EDIT_FRAME::TrimWire( const wxPoint& aStart, const wxPoint& aEnd, bool aAppend )
{
    SCH_LINE* line;
    SCH_ITEM* next_item = NULL;
    bool retval = false;

    if( aStart == aEnd )
        return retval;

    for( SCH_ITEM* item = GetScreen()->GetDrawItems(); item; item = next_item )
    {
        next_item = item->Next();

        // Don't remove wires that are already deleted or are currently being dragged
        if( item->GetFlags() & ( STRUCT_DELETED | IS_DRAGGED | IS_MOVED | SKIP_STRUCT ) )
            continue;

        if( item->Type() != SCH_LINE_T || item->GetLayer() != LAYER_WIRE )
            continue;

        line = (SCH_LINE*) item;
        if( !IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), aStart ) ||
                !IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), aEnd ) )
        {
            continue;
        }

        // Don't remove entire wires
        if( ( line->GetStartPoint() == aStart && line->GetEndPoint() == aEnd )
            || ( line->GetStartPoint() == aEnd && line->GetEndPoint() == aStart ) )
        {
            continue;
        }

        // Step 1: break the segment on one end.  return_line remains line if not broken.
        // Ensure that *line points to the segment containing aEnd
        SCH_LINE* return_line = line;
        aAppend |= BreakSegment( line, aStart, aAppend, &return_line );
        if( IsPointOnSegment( return_line->GetStartPoint(), return_line->GetEndPoint(), aEnd ) )
            line = return_line;

        // Step 2: break the remaining segment.  return_line remains line if not broken.
        // Ensure that *line _also_ contains aStart.  This is our overlapping segment
        aAppend |= BreakSegment( line, aEnd, aAppend, &return_line );
        if( IsPointOnSegment( return_line->GetStartPoint(), return_line->GetEndPoint(), aStart ) )
            line = return_line;

        SaveCopyInUndoList( (SCH_ITEM*)line, UR_DELETED, aAppend );
        RemoveFromScreen( (SCH_ITEM*)line );
        aAppend = true;
        retval = true;
    }

    return retval;
}


bool SCH_EDIT_FRAME::SchematicCleanUp( bool aAppend )
{
    SCH_ITEM*           item = NULL;
    SCH_ITEM*           secondItem = NULL;
    PICKED_ITEMS_LIST   itemList;
    SCH_SCREEN*         screen = GetScreen();

    auto remove_item = [ &itemList ]( SCH_ITEM* aItem ) -> void
    {
        aItem->SetFlags( STRUCT_DELETED );
        itemList.PushItem( ITEM_PICKER( aItem, UR_DELETED ) );
    };

    BreakSegmentsOnJunctions( true );

    for( item = screen->GetDrawItems(); item; item = item->Next() )
    {
        if( ( item->Type() != SCH_LINE_T )
            && ( item->Type() != SCH_JUNCTION_T )
            && ( item->Type() != SCH_NO_CONNECT_T ) )
            continue;

        if( item->GetFlags() & STRUCT_DELETED )
            continue;

        // Remove unneeded junctions
        if( ( item->Type() == SCH_JUNCTION_T )
            && ( !screen->IsJunctionNeeded( item->GetPosition() ) ) )
        {
            remove_item( item );
            continue;
        }

        // Remove zero-length lines
        if( item->Type() == SCH_LINE_T
            && ( (SCH_LINE*) item )->IsNull() )
        {
            remove_item( item );
            continue;
        }

        for( secondItem = item->Next(); secondItem; secondItem = secondItem->Next() )
        {
            if( item->Type() != secondItem->Type() || ( secondItem->GetFlags() & STRUCT_DELETED ) )
                continue;

            // Merge overlapping lines
            if( item->Type() == SCH_LINE_T )
            {
                SCH_LINE*   firstLine   = (SCH_LINE*) item;
                SCH_LINE*   secondLine  = (SCH_LINE*) secondItem;
                SCH_LINE*   line = NULL;
                bool needed = false;

                if( !secondLine->IsParallel( firstLine )
                  || secondLine->GetLineStyle() != firstLine->GetLineStyle()
                  || secondLine->GetLineColor() != firstLine->GetLineColor()
                  || secondLine->GetLineSize() != firstLine->GetLineSize() )
                    continue;

                // Remove identical lines
                if( firstLine->IsEndPoint( secondLine->GetStartPoint() )
                    && firstLine->IsEndPoint( secondLine->GetEndPoint() ) )
                {
                    remove_item( secondItem );
                    continue;
                }

                // If the end points overlap, check if we still need the junction
                if( secondLine->IsEndPoint( firstLine->GetStartPoint() ) )
                    needed = screen->IsJunctionNeeded( firstLine->GetStartPoint() );
                else if( secondLine->IsEndPoint( firstLine->GetEndPoint() ) )
                    needed = screen->IsJunctionNeeded( firstLine->GetEndPoint() );

                if( !needed && ( line = (SCH_LINE*) secondLine->MergeOverlap( firstLine ) ) )
                {
                    remove_item( item );
                    remove_item( secondItem );
                    itemList.PushItem( ITEM_PICKER( line, UR_NEW ) );
                    AddToScreen( line );
                    break;
                }
            }
            // Remove duplicate junctions and no-connects
            else if( secondItem->GetPosition() == item->GetPosition() )
                remove_item( secondItem );
        }
    }

    for( item = screen->GetDrawItems(); item; item = secondItem )
    {
        secondItem = item->Next();

        if( item->GetFlags() & STRUCT_DELETED )
            RemoveFromScreen( item );
    }

    SaveCopyInUndoList( itemList, UR_CHANGED, aAppend );

    return itemList.GetCount() > 0;
}


bool SCH_EDIT_FRAME::BreakSegment( SCH_LINE* aSegment, const wxPoint& aPoint, bool aAppend,
        SCH_LINE** aNewSegment )
{
    if( !IsPointOnSegment( aSegment->GetStartPoint(), aSegment->GetEndPoint(), aPoint )
            || aSegment->IsEndPoint( aPoint ) )
        return false;

    SaveCopyInUndoList( aSegment, UR_CHANGED, aAppend );
    SCH_LINE* newSegment = new SCH_LINE( *aSegment );
    SaveCopyInUndoList( newSegment, UR_NEW, true );

    newSegment->SetStartPoint( aPoint );
    AddToScreen( newSegment );

    RefreshItem( aSegment );
    aSegment->SetEndPoint( aPoint );

    if( aNewSegment )
        *aNewSegment = newSegment;

    return true;
}


bool SCH_EDIT_FRAME::BreakSegments( const wxPoint& aPoint, bool aAppend )
{
    bool brokenSegments = false;

    for( SCH_ITEM* segment = GetScreen()->GetDrawItems(); segment; segment = segment->Next() )
    {
        if( ( segment->Type() != SCH_LINE_T ) || ( segment->GetLayer() == LAYER_NOTES ) )
            continue;

        brokenSegments |= BreakSegment( (SCH_LINE*) segment, aPoint, aAppend || brokenSegments );
    }

    return brokenSegments;
}


bool SCH_EDIT_FRAME::BreakSegmentsOnJunctions( bool aAppend )
{
    bool brokenSegments = false;

    for( SCH_ITEM* item = GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() == SCH_JUNCTION_T )
        {
            SCH_JUNCTION* junction = ( SCH_JUNCTION* ) item;

            if( BreakSegments( junction->GetPosition(), brokenSegments || aAppend ) )
                brokenSegments = true;
        }
        else
        {
            SCH_BUS_ENTRY_BASE* busEntry = dynamic_cast<SCH_BUS_ENTRY_BASE*>( item );
            if( busEntry )
            {
                if( BreakSegments( busEntry->GetPosition(), brokenSegments || aAppend )
                 || BreakSegments( busEntry->m_End(), brokenSegments || aAppend ) )
                    brokenSegments = true;
            }
        }
    }

    return brokenSegments;
}


void SCH_EDIT_FRAME::DeleteJunction( SCH_ITEM* aJunction, bool aAppend )
{
    SCH_SCREEN* screen = GetScreen();
    PICKED_ITEMS_LIST itemList;

    auto remove_item = [ & ]( SCH_ITEM* aItem ) -> void
    {
        aItem->SetFlags( STRUCT_DELETED );
        itemList.PushItem( ITEM_PICKER( aItem, UR_DELETED ) );
    };

    remove_item( aJunction );

    for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
    {
        SCH_LINE* firstLine = dynamic_cast<SCH_LINE*>( item );

        if( !firstLine || !firstLine->IsEndPoint( aJunction->GetPosition() )
                  || ( firstLine->GetFlags() & STRUCT_DELETED ) )
            continue;

        for( SCH_ITEM* secondItem = item->Next(); secondItem; secondItem = secondItem->Next() )
        {
            SCH_LINE* secondLine = dynamic_cast<SCH_LINE*>( secondItem );

            if( !secondLine || !secondLine->IsEndPoint( aJunction->GetPosition() )
                    || ( secondItem->GetFlags() & STRUCT_DELETED )
                    || !secondLine->IsParallel( firstLine ) )
                continue;


            // Remove identical lines
            if( firstLine->IsEndPoint( secondLine->GetStartPoint() )
                && firstLine->IsEndPoint( secondLine->GetEndPoint() ) )
            {
                remove_item( secondItem );
                continue;
            }

            // Try to merge the remaining lines
            if( SCH_LINE* line = (SCH_LINE*) secondLine->MergeOverlap( firstLine ) )
            {
                remove_item( item );
                remove_item( secondItem );
                itemList.PushItem( ITEM_PICKER( line, UR_NEW ) );
                AddToScreen( line );
                break;
            }
        }
    }

    SaveCopyInUndoList( itemList, UR_DELETED, aAppend );

    SCH_ITEM* nextitem;
    for( SCH_ITEM* item = screen->GetDrawItems(); item; item = nextitem )
    {
        nextitem = item->Next();

        if( item->GetFlags() & STRUCT_DELETED )
            RemoveFromScreen( item );
    }
}


SCH_JUNCTION* SCH_EDIT_FRAME::AddJunction( const wxPoint& aPosition, bool aAppend )
{
    SCH_JUNCTION* junction = new SCH_JUNCTION( aPosition );
    bool broken_segments = false;

    AddToScreen( junction );
    broken_segments = BreakSegments( aPosition, aAppend );
    TestDanglingEnds();
    OnModify();
    SaveCopyInUndoList( junction, UR_NEW, broken_segments || aAppend );

    auto view = GetCanvas()->GetView();
    view->ClearPreview();
    view->ShowPreview( false );
    view->ClearHiddenFlags();

    return junction;
}


SCH_NO_CONNECT* SCH_EDIT_FRAME::AddNoConnect( const wxPoint& aPosition )
{
    SCH_NO_CONNECT* no_connect = new SCH_NO_CONNECT( aPosition );

    SetRepeatItem( no_connect );
    AddToScreen( no_connect );
    SchematicCleanUp();
    TestDanglingEnds();
    OnModify();

    auto view = GetCanvas()->GetView();
    view->ClearPreview();
    view->ShowPreview( false );
    view->ClearHiddenFlags();

    SaveCopyInUndoList( no_connect, UR_NEW );
    return no_connect;
}


/* Abort function for wire, bus or line creation
 */
static void AbortCreateNewLine( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_EDIT_FRAME* parent = ( SCH_EDIT_FRAME* ) aPanel->GetParent();

    if( screen->GetCurItem() )
    {
        s_wires.DeleteAll();    // Free the list, for a future usage
        screen->SetCurItem( NULL );
    }
    else
    {
        parent->SetRepeatItem( NULL );
    }

    auto view = static_cast<SCH_DRAW_PANEL*>(aPanel)->GetView();
    view->ClearPreview();
    view->ShowPreview( false );
    view->ClearHiddenFlags();

    // Clear flags used in edit functions.
    screen->ClearDrawingState();
}


void SCH_EDIT_FRAME::RepeatDrawItem()
{
    SCH_ITEM*   repeater = GetRepeatItem();

    if( !repeater )
        return;

    // clone the repeater, move it, insert into display list, then save a copy
    // via SetRepeatItem();

    SCH_ITEM* my_clone = (SCH_ITEM*) repeater->Clone();

    // If cloning a component then put into 'move' mode.
    if( my_clone->Type() == SCH_COMPONENT_T )
    {
        wxPoint pos = GetCrossHairPosition() - ( (SCH_COMPONENT*) my_clone )->GetPosition();

        my_clone->SetFlags( IS_NEW );
        ( (SCH_COMPONENT*) my_clone )->SetTimeStamp( GetNewTimeStamp() );
        my_clone->Move( pos );
        PrepareMoveItem( my_clone );
    }
    else
    {
        my_clone->Move( GetRepeatStep() );

        if( my_clone->CanIncrementLabel() )
            ( (SCH_TEXT*) my_clone )->IncrementLabel( GetRepeatDeltaLabel() );

        AddToScreen( my_clone );
        SaveCopyInUndoList( my_clone, UR_NEW );

        if( my_clone->IsConnectable() )
        {
            PICKED_ITEMS_LIST list;

            list.PushItem( ITEM_PICKER( my_clone ) );
            CheckListConnections( list, true );
            SchematicCleanUp( true );
            TestDanglingEnds();
        }

        my_clone->ClearFlags();
    }

    // clone my_clone, now that it has been moved, thus saving new position.
    SetRepeatItem( my_clone );
}


