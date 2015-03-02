/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
#include <class_drawpanel.h>
#include <confirm.h>
#include <schframe.h>

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


static void AbortCreateNewLine( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void ComputeBreakPoint( SCH_LINE* segment, const wxPoint& new_pos );

static DLIST< SCH_ITEM > s_wires;       // when creating a new set of wires,
                                        // stores here the new wires.
static DLIST< SCH_ITEM > s_oldWires;    // when creating a new set of wires,
                                        // stores here the old wires (for undo command)


/**
 * Mouse capture callback for drawing line segments.
 */
static void DrawSegment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                         bool aErase )
{
    SCH_LINE* segment;

    if( s_wires.GetCount() == 0 )
        return;

    segment = (SCH_LINE*) s_wires.begin();
    EDA_COLOR_T color = GetLayerColor( segment->GetLayer() );
    ColorChangeHighlightFlag( &color, !(color & HIGHLIGHT_FLAG) );

    if( aErase )
    {
        while( segment )
        {
            if( !segment->IsNull() )  // Redraw if segment length != 0
                segment->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, color );

            segment = segment->Next();
        }
    }

    SCH_EDIT_FRAME* frame = (SCH_EDIT_FRAME*) aPanel->GetParent();

    wxPoint endpos = frame->GetCrossHairPosition();

    if( frame->GetForceHVLines() ) /* Coerce the line to vertical or horizontal one: */
        ComputeBreakPoint( (SCH_LINE*) s_wires.GetLast()->Back(), endpos );
    else
        ( (SCH_LINE*) s_wires.GetLast() )->SetEndPoint( endpos );

    segment = (SCH_LINE*) s_wires.begin();

    while( segment )
    {
        if( !segment->IsNull() )  // Redraw if segment length != 0
            segment->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, color );

        segment = segment->Next();
    }
}


void SCH_EDIT_FRAME::BeginSegment( wxDC* DC, int type )
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
            {
                wxLogDebug( wxT( "BeginSegment: item->GetFlags()== %X" ),
                    segment->GetFlags() );
            }
            // no wire, bus or graphic line in progress
            segment = NULL;
        }
    }

    if( !segment )  /* first point : Create first wire or bus */
    {
        GetScreen()->ExtractWires( s_oldWires, true );
        GetScreen()->SchematicCleanUp( m_canvas );

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
        SCH_LINE* prevSegment = (SCH_LINE*) segment->Back();

        // Be aware prevSegment can be null when the horizontal and vertical lines only switch is off
        // when we create the first segment.

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

        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

        // Terminate the command if the end point is on a pin, junction, or another wire or bus.
        if( GetScreen()->IsTerminalPoint( cursorpos, segment->GetLayer() ) )
        {
            EndSegment( DC );
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
        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
    }
}


void SCH_EDIT_FRAME::EndSegment( wxDC* DC )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_LINE* segment = (SCH_LINE*) screen->GetCurItem();

    if( segment == NULL || segment->Type() != SCH_LINE_T || !segment->IsNew() )
        return;

    // Delete zero length segments and clear item flags.
    SCH_ITEM* item = s_wires.begin();

    while( item )
    {
        item->ClearFlags();

        wxCHECK_RET( item->Type() == SCH_LINE_T, wxT( "Unexpected object type in wire list." ) );

        segment = (SCH_LINE*) item;
        item = item->Next();

        if( segment->IsNull() )
            delete s_wires.Remove( segment );
    }

    if( s_wires.GetCount() == 0 )
        return;

    // Get the last non-null wire (this is the last created segment).
    SetRepeatItem( segment = (SCH_LINE*) s_wires.GetLast() );

    screen->SetCurItem( NULL );
    m_canvas->EndMouseCapture( -1, -1, wxEmptyString, false );

    // store the terminal point of this last segment: a junction could be needed
    // (the last wire could be merged/deleted/modified, and lost)
    wxPoint endpoint = segment->GetEndPoint();

    // store the starting point of this first segment: a junction could be needed
    SCH_LINE* firstsegment = (SCH_LINE*) s_wires.GetFirst();
    wxPoint startPoint = firstsegment->GetStartPoint();

    screen->Append( s_wires );

    // Correct and remove segments that need to be merged.
    screen->SchematicCleanUp( NULL, DC );

    // A junction could be needed to connect the end point of the last created segment.
    if( screen->IsJunctionNeeded( endpoint ) )
        screen->Append( AddJunction( DC, endpoint ) );

    // A junction could be needed to connect the start point of the set of new created wires
    if( screen->IsJunctionNeeded( startPoint ) )
        screen->Append( AddJunction( DC, startPoint ) );

    m_canvas->Refresh();

    // Put the snap shot of the previous wire, buses, and junctions in the undo/redo list.
    PICKED_ITEMS_LIST oldItems;

    oldItems.m_Status = UR_WIRE_IMAGE;

    while( s_oldWires.GetCount() != 0 )
    {
        ITEM_PICKER picker = ITEM_PICKER( s_oldWires.PopFront(), UR_WIRE_IMAGE );
        oldItems.PushItem( picker );
    }

    SaveCopyInUndoList( oldItems, UR_WIRE_IMAGE );

    OnModify();
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
static void ComputeBreakPoint( SCH_LINE* aSegment, const wxPoint& aPosition )
{
    wxCHECK_RET( aSegment != NULL, wxT( "Cannot compute break point of NULL line segment." ) );

    SCH_LINE* nextSegment = aSegment->Next();
    wxPoint midPoint = aPosition;

    wxCHECK_RET( nextSegment != NULL,
                 wxT( "Cannot compute break point of NULL second line segment." ) );

#if 0
    if( ABS( midPoint.x - aSegment->GetStartPoint().x ) <
        ABS( midPoint.y - aSegment->GetStartPoint().y ) )
        midPoint.x = aSegment->GetStartPoint().x;
    else
        midPoint.y = aSegment->GetStartPoint().y;
#else
    int iDx = aSegment->GetEndPoint().x - aSegment->GetStartPoint().x;
    int iDy = aSegment->GetEndPoint().y - aSegment->GetStartPoint().y;

    if( iDy != 0 )         // keep the first segment orientation (currently horizontal)
    {
        midPoint.x = aSegment->GetStartPoint().x;
    }
    else if( iDx != 0 )    // keep the first segment orientation (currently vertical)
    {
        midPoint.y = aSegment->GetStartPoint().y;
    }
    else
    {
        if( std::abs( midPoint.x - aSegment->GetStartPoint().x ) <
            std::abs( midPoint.y - aSegment->GetStartPoint().y ) )
            midPoint.x = aSegment->GetStartPoint().x;
        else
            midPoint.y = aSegment->GetStartPoint().y;
    }
#endif

    aSegment->SetEndPoint( midPoint );
    nextSegment->SetStartPoint( midPoint );
    nextSegment->SetEndPoint( aPosition );
}


void SCH_EDIT_FRAME::DeleteCurrentSegment( wxDC* DC )
{
    SCH_SCREEN* screen = GetScreen();

    SetRepeatItem( NULL );

    if( ( screen->GetCurItem() == NULL ) || !screen->GetCurItem()->IsNew() )
        return;

    DrawSegment( m_canvas, DC, wxDefaultPosition, false );

    screen->Remove( screen->GetCurItem() );
    m_canvas->SetMouseCaptureCallback( NULL );
    screen->SetCurItem( NULL );
}


SCH_JUNCTION* SCH_EDIT_FRAME::AddJunction( wxDC* aDC, const wxPoint& aPosition,
                                           bool aPutInUndoList )
{
    SCH_JUNCTION* junction = new SCH_JUNCTION( aPosition );

    SetRepeatItem( junction );

    m_canvas->CrossHairOff( aDC );     // Erase schematic cursor
    junction->Draw( m_canvas, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    m_canvas->CrossHairOn( aDC );      // Display schematic cursor

    if( aPutInUndoList )
    {
        GetScreen()->Append( junction );
        SaveCopyInUndoList( junction, UR_NEW );
        OnModify();
    }

    return junction;
}


SCH_NO_CONNECT* SCH_EDIT_FRAME::AddNoConnect( wxDC* aDC, const wxPoint& aPosition )
{
    SCH_NO_CONNECT* no_connect = new SCH_NO_CONNECT( aPosition );

    SetRepeatItem( no_connect );

    m_canvas->CrossHairOff( aDC );     // Erase schematic cursor
    no_connect->Draw( m_canvas, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    m_canvas->CrossHairOn( aDC );      // Display schematic cursor

    GetScreen()->Append( no_connect );
    OnModify();
    SaveCopyInUndoList( no_connect, UR_NEW );
    return no_connect;
}


/* Abort function for wire, bus or line creation
 */
static void AbortCreateNewLine( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) Panel->GetScreen();

    if( screen->GetCurItem() )
    {
        s_wires.DeleteAll();
        s_oldWires.DeleteAll();
        screen->SetCurItem( NULL );
        Panel->Refresh();
    }
    else
    {
        SCH_EDIT_FRAME* parent = ( SCH_EDIT_FRAME* ) Panel->GetParent();
        parent->SetRepeatItem( NULL );
    }

    // Clear flags used in edit functions.
    screen->ClearDrawingState();
}


void SCH_EDIT_FRAME::RepeatDrawItem( wxDC* DC )
{
    SCH_ITEM*   repeater = GetRepeatItem();

    if( !repeater )
        return;

    //D( repeater>Show( 0, std::cout ); )

    // clone the repeater, move it, insert into display list, then save a copy
    // via SetRepeatItem();

    SCH_ITEM* my_clone = (SCH_ITEM*) repeater->Clone();

    // If cloning a component then put into 'move' mode.
    if( my_clone->Type() == SCH_COMPONENT_T )
    {
        wxPoint pos = GetCrossHairPosition() -
                      ( (SCH_COMPONENT*) my_clone )->GetPosition();

        my_clone->SetFlags( IS_NEW );
        ( (SCH_COMPONENT*) my_clone )->SetTimeStamp( GetNewTimeStamp() );
        my_clone->Move( pos );
        my_clone->Draw( m_canvas, DC, wxPoint( 0, 0 ), g_XorMode );
        PrepareMoveItem( my_clone, DC );
    }
    else
    {
        my_clone->Move( wxPoint( g_RepeatStep.GetWidth(), g_RepeatStep.GetHeight() ) );

        if( my_clone->CanIncrementLabel() )
            ( (SCH_TEXT*) my_clone )->IncrementLabel();

        GetScreen()->Append( my_clone );
        GetScreen()->TestDanglingEnds();
        my_clone->Draw( m_canvas, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        SaveCopyInUndoList( my_clone, UR_NEW );
        my_clone->ClearFlags();
    }

    // clone my_clone, now that it has been moved, thus saving new position.
    SetRepeatItem( my_clone );
}
