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

#include <core/kicad_algo.h>
#include <eeschema_id.h>
#include <general.h>
#include <lib_item.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_view.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <trigo.h>


std::vector<wxPoint> SCH_EDIT_FRAME::GetSchematicConnections()
{
    std::vector<wxPoint> retval;

    for( SCH_ITEM* item : GetScreen()->Items() )
    {
        // Avoid items that are changing
        if( !( item->GetEditFlags() & ( IS_DRAGGING | IS_MOVING | IS_DELETED ) ) )
        {
            std::vector<wxPoint> pts = item->GetConnectionPoints();
            retval.insert( retval.end(), pts.begin(), pts.end() );
        }
    }

    // We always have some overlapping connection points.  Drop duplicates here
    std::sort( retval.begin(), retval.end(),
            []( const wxPoint& a, const wxPoint& b ) -> bool
            { return a.x < b.x || (a.x == b.x && a.y < b.y); } );
    retval.erase(
            std::unique( retval.begin(), retval.end() ), retval.end() );

    return retval;
}


void SCH_EDIT_FRAME::TestDanglingEnds()
{
    std::function<void( SCH_ITEM* )> changeHandler =
            [&]( SCH_ITEM* aChangedItem ) -> void
            {
                GetCanvas()->GetView()->Update( aChangedItem, KIGFX::REPAINT );
            };

    GetScreen()->TestDanglingEnds( nullptr, &changeHandler );
}


bool SCH_EDIT_FRAME::TrimWire( const wxPoint& aStart, const wxPoint& aEnd )
{
    SCH_SCREEN* screen = GetScreen();
    bool        retval = false;

    std::vector<SCH_LINE*> wires;
    EDA_RECT    bb( aStart, wxSize( 1, 1 ) );

    bb.Merge( aEnd );

    if( aStart == aEnd )
        return retval;

    // We cannot modify the RTree while iterating, so push the possible
    // wires into a separate structure.
    for( EDA_ITEM* item : screen->Items().Overlapping( bb ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( item->Type() == SCH_LINE_T && line->GetLayer() == LAYER_WIRE )
            wires.push_back( line );
    }

    for( SCH_LINE* line : wires )
    {
        // Don't remove wires that are already deleted or are currently being dragged
        if( line->GetEditFlags() & ( STRUCT_DELETED | IS_DRAGGING | IS_MOVING | SKIP_STRUCT ) )
            continue;

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
        BreakSegment( line, aStart, &return_line );

        if( IsPointOnSegment( return_line->GetStartPoint(), return_line->GetEndPoint(), aEnd ) )
            line = return_line;

        // Step 2: break the remaining segment.  return_line remains line if not broken.
        // Ensure that *line _also_ contains aStart.  This is our overlapping segment
        BreakSegment( line, aEnd, &return_line );

        if( IsPointOnSegment( return_line->GetStartPoint(), return_line->GetEndPoint(), aStart ) )
            line = return_line;

        SaveCopyInUndoList( screen, line, UNDO_REDO::DELETED, true );
        RemoveFromScreen( line, screen );

        retval = true;
    }

    return retval;
}


bool SCH_EDIT_FRAME::SchematicCleanUp( SCH_SCREEN* aScreen )
{
    PICKED_ITEMS_LIST            itemList;
    EE_SELECTION_TOOL*           selectionTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    std::vector<SCH_ITEM*>       deletedItems;
    std::vector<SCH_LINE*>       lines;
    std::vector<SCH_JUNCTION*>   junctions;
    std::vector<SCH_NO_CONNECT*> ncs;
    bool                         changed = true;

    if( aScreen == nullptr )
        aScreen = GetScreen();

    auto remove_item = [&]( SCH_ITEM* aItem ) -> void
                       {
                           changed = true;
                           aItem->SetFlags( STRUCT_DELETED );
                           itemList.PushItem( ITEM_PICKER( aScreen, aItem, UNDO_REDO::DELETED ) );
                           deletedItems.push_back( aItem );
                       };

    BreakSegmentsOnJunctions( aScreen );

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_JUNCTION_T ) )
    {
        if( !aScreen->IsJunctionNeeded( item->GetPosition() ) )
            remove_item( item );
        else
            junctions.push_back( static_cast<SCH_JUNCTION*>( item ) );
    }

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_NO_CONNECT_T ) )
    {
        ncs.push_back( static_cast<SCH_NO_CONNECT*>( item ) );
    }

    alg::for_all_pairs( junctions.begin(), junctions.end(),
            [&]( SCH_JUNCTION* aFirst, SCH_JUNCTION* aSecond )
            {
                if( ( aFirst->GetEditFlags() & STRUCT_DELETED )
                        || ( aSecond->GetEditFlags() & STRUCT_DELETED ) )
                {
                    return;
                }

                if( aFirst->GetPosition() == aSecond->GetPosition() )
                    remove_item( aSecond );
            } );

    alg::for_all_pairs( ncs.begin(), ncs.end(),
            [&]( SCH_NO_CONNECT* aFirst, SCH_NO_CONNECT* aSecond )
            {
                if( ( aFirst->GetEditFlags() & STRUCT_DELETED )
                        || ( aSecond->GetEditFlags() & STRUCT_DELETED ) )
                {
                    return;
                }

                if( aFirst->GetPosition() == aSecond->GetPosition() )
                    remove_item( aSecond );
            } );


    while( changed )
    {
        changed = false;
        lines.clear();

        for( SCH_ITEM* item : aScreen->Items().OfType( SCH_LINE_T ) )
        {
            if( item->GetLayer() == LAYER_WIRE || item->GetLayer() == LAYER_BUS )
                lines.push_back( static_cast<SCH_LINE*>( item ) );
        }

        for( auto it1 = lines.begin(); it1 != lines.end(); ++it1 )
        {
            SCH_LINE* firstLine = *it1;

            if( firstLine->GetEditFlags() & STRUCT_DELETED )
                continue;

            if( firstLine->IsNull() )
            {
                remove_item( firstLine );
                continue;
            }

            auto it2 = it1;

            for( ++it2; it2 != lines.end(); ++it2 )
            {
                SCH_LINE* secondLine = *it2;

                if( secondLine->GetFlags() & STRUCT_DELETED )
                    continue;

                if( !secondLine->IsParallel( firstLine )
                        || !secondLine->IsStrokeEquivalent( firstLine )
                        || secondLine->GetLayer() != firstLine->GetLayer() )
                {
                    continue;
                }

                // Remove identical lines
                if( firstLine->IsEndPoint( secondLine->GetStartPoint() )
                        && firstLine->IsEndPoint( secondLine->GetEndPoint() ) )
                {
                    remove_item( secondLine );
                    continue;
                }

                // See if we can merge an overlap (or two colinear touching segments with
                // no junction where they meet).
                SCH_LINE* mergedLine = secondLine->MergeOverlap( aScreen, firstLine, true );

                if( mergedLine != nullptr )
                {
                    remove_item( firstLine );
                    remove_item( secondLine );
                    itemList.PushItem( ITEM_PICKER( aScreen, mergedLine, UNDO_REDO::NEWITEM ) );

                    AddToScreen( mergedLine, aScreen );

                    if( firstLine->IsSelected() )
                        selectionTool->AddItemToSel( mergedLine, true /*quiet mode*/ );

                    break;
                }
            }
        }
    }

    for( SCH_ITEM* item : deletedItems )
    {
        if( item->IsSelected() )
            selectionTool->RemoveItemFromSel( item, true /*quiet mode*/ );

        RemoveFromScreen( item, aScreen );
    }

    if( itemList.GetCount() )
        SaveCopyInUndoList( itemList, UNDO_REDO::DELETED, true );

    return itemList.GetCount() > 0;
}


bool SCH_EDIT_FRAME::BreakSegment( SCH_LINE* aSegment, const wxPoint& aPoint,
                                   SCH_LINE** aNewSegment, SCH_SCREEN* aScreen )
{
    if( aScreen == nullptr )
        aScreen = GetScreen();

    SCH_LINE* newSegment = new SCH_LINE( *aSegment );

    newSegment->SetStartPoint( aPoint );
    AddToScreen( newSegment, aScreen );

    SaveCopyInUndoList( aScreen, newSegment, UNDO_REDO::NEWITEM, true );
    SaveCopyInUndoList( aScreen, aSegment, UNDO_REDO::CHANGED, true );

    UpdateItem( aSegment, false, true );
    aSegment->SetEndPoint( aPoint );

    if( aNewSegment )
        *aNewSegment = newSegment;

    return true;
}


bool SCH_EDIT_FRAME::BreakSegments( const wxPoint& aPoint, SCH_SCREEN* aScreen )
{
    static const KICAD_T wiresAndBuses[] = { SCH_LINE_LOCATE_WIRE_T, SCH_LINE_LOCATE_BUS_T, EOT };

    if( aScreen == nullptr )
        aScreen = GetScreen();

    bool                   brokenSegments = false;
    std::vector<SCH_LINE*> wires;

    for( SCH_ITEM* item : aScreen->Items().Overlapping( SCH_LINE_T, aPoint ) )
    {
        if( item->IsType( wiresAndBuses ) )
        {
            SCH_LINE* wire = static_cast<SCH_LINE*>( item );

            if( IsPointOnSegment( wire->GetStartPoint(), wire->GetEndPoint(), aPoint )
              && !wire->IsEndPoint( aPoint ) )
            {
                wires.push_back( wire );
            }
        }
    }

    for( SCH_LINE* wire : wires )
        brokenSegments |= BreakSegment( wire, aPoint, nullptr, aScreen );

    return brokenSegments;
}


bool SCH_EDIT_FRAME::BreakSegmentsOnJunctions( SCH_SCREEN* aScreen )
{
    if( aScreen == nullptr )
        aScreen = GetScreen();

    bool brokenSegments = false;

    std::set<wxPoint> point_set;

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_JUNCTION_T ) )
        point_set.insert( item->GetPosition() );

    for( SCH_ITEM* item : aScreen->Items().OfType( SCH_BUS_WIRE_ENTRY_T ) )
    {
        SCH_BUS_WIRE_ENTRY* entry = static_cast<SCH_BUS_WIRE_ENTRY*>( item );
        point_set.insert( entry->GetPosition() );
        point_set.insert( entry->GetEnd() );
    }


    for( const wxPoint& pt : point_set )
        brokenSegments |= BreakSegments( pt, aScreen );

    return brokenSegments;
}


void SCH_EDIT_FRAME::DeleteJunction( SCH_ITEM* aJunction, bool aAppend )
{
    SCH_SCREEN*        screen = GetScreen();
    PICKED_ITEMS_LIST  undoList;
    EE_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    KICAD_T            wiresAndBuses[] = { SCH_LINE_LOCATE_WIRE_T, SCH_LINE_LOCATE_BUS_T, EOT };

    auto remove_item = [ & ]( SCH_ITEM* aItem ) -> void
    {
        aItem->SetFlags( STRUCT_DELETED );
        undoList.PushItem( ITEM_PICKER( screen, aItem, UNDO_REDO::DELETED ) );
    };

    remove_item( aJunction );
    RemoveFromScreen( aJunction, screen );

    /// Note that std::list or similar is required here as we may insert values in the
    /// loop below.  This will invalidate iterators in a std::vector or std::deque
    std::list<SCH_LINE*> lines;

    for( SCH_ITEM* item : screen->Items().Overlapping( SCH_LINE_T, aJunction->GetPosition() ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( line->IsType( wiresAndBuses ) && line->IsEndPoint( aJunction->GetPosition() )
                && !( line->GetEditFlags() & STRUCT_DELETED ) )
            lines.push_back( line );
    }

    alg::for_all_pairs( lines.begin(), lines.end(),
            [&]( SCH_LINE* firstLine, SCH_LINE* secondLine )
            {
                if( ( firstLine->GetEditFlags() & STRUCT_DELETED )
                        || ( secondLine->GetEditFlags() & STRUCT_DELETED )
                        || !secondLine->IsParallel( firstLine ) )
                {
                    return;
                }

                // Remove identical lines
                if( firstLine->IsEndPoint( secondLine->GetStartPoint() )
                        && firstLine->IsEndPoint( secondLine->GetEndPoint() ) )
                {
                    remove_item( firstLine );
                    return;
                }

                // Try to merge the remaining lines
                if( SCH_LINE* line = secondLine->MergeOverlap( screen, firstLine, false ) )
                {
                    remove_item( firstLine );
                    remove_item( secondLine );
                    undoList.PushItem( ITEM_PICKER( screen, line, UNDO_REDO::NEWITEM ) );
                    AddToScreen( line, screen );

                    if( line->IsSelected() )
                        selectionTool->AddItemToSel( line, true /*quiet mode*/ );

                    lines.push_back( line );
                }
            } );

    SaveCopyInUndoList( undoList, UNDO_REDO::DELETED, aAppend );

    for( SCH_LINE* line : lines )
    {
        if( line->GetEditFlags() & STRUCT_DELETED )
        {
            if( line->IsSelected() )
                selectionTool->RemoveItemFromSel( line, true /*quiet mode*/ );

            RemoveFromScreen( line, screen );
        }
    }
}


SCH_JUNCTION* SCH_EDIT_FRAME::AddJunction( SCH_SCREEN* aScreen, const wxPoint& aPos,
                                           bool aUndoAppend, bool aFinal )
{
    SCH_JUNCTION* junction = new SCH_JUNCTION( aPos );

    AddToScreen( junction, aScreen );
    SaveCopyInUndoList( aScreen, junction, UNDO_REDO::NEWITEM, aUndoAppend );
    BreakSegments( aPos );

    if( aFinal )
    {
        m_toolManager->PostEvent( EVENTS::SelectedItemsModified );

        TestDanglingEnds();
        OnModify();

        KIGFX::SCH_VIEW* view = GetCanvas()->GetView();
        view->ClearPreview();
        view->ShowPreview( false );
        view->ClearHiddenFlags();
    }

    return junction;
}


