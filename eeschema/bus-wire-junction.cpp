/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
#include <wxEeschemaStruct.h>

#include <lib_draw_item.h>
#include <lib_pin.h>
#include <general.h>
#include <protos.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_polyline.h>
#include <sch_text.h>
#include <sch_component.h>
#include <sch_sheet.h>


static void AbortCreateNewLine( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void ComputeBreakPoint( SCH_LINE* segment, const wxPoint& new_pos );

static DLIST< SCH_ITEM > s_wires;
static DLIST< SCH_ITEM > s_oldWires;

static wxPoint s_startPoint;


/**
 * Mouse capture callback for drawing line segments.
 */
static void DrawSegment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                         bool aErase )
{
    SCH_LINE* segment;
    int color;

    if( s_wires.GetCount() == 0 )
        return;

    segment = (SCH_LINE*) s_wires.begin();
    color = ReturnLayerColor( segment->GetLayer() ) ^ HIGHLIGHT_FLAG;

    if( aErase )
    {
        while( segment )
        {
            if( !segment->IsNull() )  // Redraw if segment length != 0
                segment->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, color );

            segment = segment->Next();
        }
    }

    wxPoint endpos = aPanel->GetScreen()->GetCrossHairPosition();

    if( g_HVLines ) /* Coerce the line to vertical or horizontal one: */
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
    wxPoint   cursorpos = GetScreen()->GetCrossHairPosition();

    segment = (SCH_LINE*) GetScreen()->GetCurItem();

    if( !segment )  /* first point : Create first wire or bus */
    {
        s_startPoint = cursorpos;
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
        if( g_HVLines )
        {
            nextSegment = new SCH_LINE( *segment );
            nextSegment->SetFlags( IS_NEW );
            s_wires.PushBack( nextSegment );
            GetScreen()->SetCurItem( nextSegment );
        }

        m_canvas->SetMouseCapture( DrawSegment, AbortCreateNewLine );
        m_itemToRepeat = NULL;
    }
    else    // A segment is in progress: terminates the current segment and add a new segment.
    {
        SCH_LINE* prevSegment = (SCH_LINE*) segment->Back();

        wxLogDebug( wxT( "Adding new segment after " ) + segment->GetSelectMenuText() );

        if( !g_HVLines )
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

        if( segment->IsNull() )
        {
            wxLogDebug( wxT( "Removing null segment: " ) + segment->GetSelectMenuText() );

            SCH_ITEM* previousSegment = item->Back();

            delete s_wires.Remove( item );

            if( previousSegment == NULL )
                item = s_wires.begin();
            else
                item = previousSegment;
        }

        item = item->Next();
    }

    if( s_wires.GetCount() == 0 )
        return;

    // Get the last non-null wire.
    m_itemToRepeat = segment = (SCH_LINE*) s_wires.GetLast();
    screen->SetCurItem( NULL );
    m_canvas->EndMouseCapture( -1, -1, wxEmptyString, false );

    DLIST< SCH_ITEM > tmp;

    for( item = s_wires.begin();  item != NULL;  item = item->Next() )
        tmp.PushBack( (SCH_ITEM*) item->Clone() );

    // Temporarily add the new segments to the schematic item list to test if any
    // junctions are required.
    screen->Append( tmp );

    // Correct and remove segments that need merged.
    screen->SchematicCleanUp( NULL, DC );

    // A junction may be needed to connect the last segment.  If the last segment was
    // removed by a cleanup, a junction may be needed to connect the segment's end point
    // which is also the same as the previous segment's start point.
    if( screen->IsJunctionNeeded( segment->GetEndPoint() ) )
        s_wires.Append( AddJunction( DC, segment->GetEndPoint() ) );
    else if( screen->IsJunctionNeeded( segment->GetStartPoint() ) )
        s_wires.Append( AddJunction( DC, segment->GetStartPoint() ) );

    // Automatically place a junction on the start point if necessary because the cleanup
    // can suppress intermediate points by merging wire segments.
    if( screen->IsJunctionNeeded( s_startPoint ) )
        s_wires.Append( AddJunction( DC, s_startPoint ) );

    // Make a copy of the original wires, buses, and junctions.
    for( item = s_oldWires.begin();  item != NULL;  item = item->Next() )
        tmp.PushBack( (SCH_ITEM*) item->Clone() );

    // Restore the old wires.
    if( tmp.GetCount() != 0 )
        screen->ReplaceWires( tmp );

    // Now add the new wires and any required junctions to the schematic item list.
    screen->Append( s_wires );

    screen->SchematicCleanUp( NULL, DC );
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
        if( ABS( midPoint.x - aSegment->GetStartPoint().x ) <
            ABS( midPoint.y - aSegment->GetStartPoint().y ) )
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

    m_itemToRepeat = NULL;

    if( ( screen->GetCurItem() == NULL ) || !screen->GetCurItem()->IsNew() )
        return;

    /* Cancel trace in progress */
    if( screen->GetCurItem()->Type() == SCH_POLYLINE_T )
    {
        SCH_POLYLINE* polyLine = (SCH_POLYLINE*) screen->GetCurItem();
        wxPoint       endpos;

        endpos = screen->GetCrossHairPosition();

        int idx = polyLine->GetCornerCount() - 1;
        wxPoint pt = (*polyLine)[idx];

        if( g_HVLines )
        {
            /* Coerce the line to vertical or horizontal one: */
            if( ABS( endpos.x - pt.x ) < ABS( endpos.y - pt.y ) )
                endpos.x = pt.x;
            else
                endpos.y = pt.y;
        }

        polyLine->SetPoint( idx, endpos );
        polyLine->Draw( m_canvas, DC, wxPoint( 0, 0 ), g_XorMode );
    }
    else
    {
        DrawSegment( m_canvas, DC, wxDefaultPosition, false );
    }

    screen->Remove( screen->GetCurItem() );
    m_canvas->SetMouseCaptureCallback( NULL );
    screen->SetCurItem( NULL );
}


SCH_JUNCTION* SCH_EDIT_FRAME::AddJunction( wxDC* aDC, const wxPoint& aPosition,
                                           bool aPutInUndoList )
{
    SCH_JUNCTION* junction = new SCH_JUNCTION( aPosition );

    m_itemToRepeat = junction;

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
    SCH_NO_CONNECT* NewNoConnect;

    NewNoConnect   = new SCH_NO_CONNECT( aPosition );
    m_itemToRepeat = NewNoConnect;

    m_canvas->CrossHairOff( aDC );     // Erase schematic cursor
    NewNoConnect->Draw( m_canvas, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    m_canvas->CrossHairOn( aDC );      // Display schematic cursor

    GetScreen()->Append( NewNoConnect );
    OnModify();
    SaveCopyInUndoList( NewNoConnect, UR_NEW );
    return NewNoConnect;
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
    if( m_itemToRepeat == NULL )
        return;

    m_itemToRepeat = m_itemToRepeat->Clone();

    if( m_itemToRepeat->Type() == SCH_COMPONENT_T ) // If repeat component then put in move mode
    {
        wxPoint pos = GetScreen()->GetCrossHairPosition() -
                      ( (SCH_COMPONENT*) m_itemToRepeat )->GetPosition();
        m_itemToRepeat->SetFlags( IS_NEW );
        ( (SCH_COMPONENT*) m_itemToRepeat )->SetTimeStamp( GetNewTimeStamp() );
        m_itemToRepeat->Move( pos );
        m_itemToRepeat->Draw( m_canvas, DC, wxPoint( 0, 0 ), g_XorMode );
        MoveItem( m_itemToRepeat, DC );
        return;
    }

    m_itemToRepeat->Move( wxPoint( g_RepeatStep.GetWidth(), g_RepeatStep.GetHeight() ) );

    if( m_itemToRepeat->CanIncrementLabel() )
        ( (SCH_TEXT*) m_itemToRepeat )->IncrementLabel();

    if( m_itemToRepeat )
    {
        GetScreen()->Append( m_itemToRepeat );
        GetScreen()->TestDanglingEnds();
        m_itemToRepeat->Draw( m_canvas, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        SaveCopyInUndoList( m_itemToRepeat, UR_NEW );
        m_itemToRepeat->ClearFlags();
    }
}
