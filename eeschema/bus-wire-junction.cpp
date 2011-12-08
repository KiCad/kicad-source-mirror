/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
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

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "wxEeschemaStruct.h"

#include "lib_draw_item.h"
#include "lib_pin.h"
#include "general.h"
#include "protos.h"
#include "sch_bus_entry.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_polyline.h"
#include "sch_text.h"
#include "sch_component.h"
#include "sch_sheet.h"


static void AbortCreateNewLine( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void ComputeBreakPoint( SCH_LINE* segment, const wxPoint& new_pos );

SCH_ITEM* s_OldWiresList;
wxPoint   s_ConnexionStartPoint;


/**
 * Mouse capture callback for drawing line segments.
 */
static void DrawSegment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                         bool aErase )
{
    SCH_LINE* CurrentLine = (SCH_LINE*) aPanel->GetScreen()->GetCurItem();
    SCH_LINE* segment;
    int color;

    if( CurrentLine == NULL )
        return;

    color = ReturnLayerColor( CurrentLine->GetLayer() ) ^ HIGHLIGHT_FLAG;

    if( aErase )
    {
        segment = CurrentLine;

        while( segment )
        {
            if( !segment->IsNull() )  // Redraw if segment length != 0
                segment->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, color );

            segment = segment->Next();
        }
    }

    wxPoint endpos = aPanel->GetScreen()->GetCrossHairPosition();

    if( g_HVLines ) /* Coerce the line to vertical or horizontal one: */
        ComputeBreakPoint( CurrentLine, endpos );
    else
        CurrentLine->SetEndPoint( endpos );

    segment = CurrentLine;

    while( segment )
    {
        if( !segment->IsNull() )  // Redraw if segment length != 0
            segment->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, color );

        segment = segment->Next();
    }
}


void SCH_EDIT_FRAME::BeginSegment( wxDC* DC, int type )
{
    SCH_LINE* oldsegment, * newsegment, * nextsegment;
    wxPoint   cursorpos = GetScreen()->GetCrossHairPosition();

    if( GetScreen()->GetCurItem() && (GetScreen()->GetCurItem()->GetFlags() == 0) )
        GetScreen()->SetCurItem( NULL );

    if( GetScreen()->GetCurItem() )
    {
        switch( GetScreen()->GetCurItem()->Type() )
        {
        case SCH_LINE_T:
        case SCH_POLYLINE_T:
            break;

        default:
            return;
        }
    }

    oldsegment = newsegment = (SCH_LINE*) GetScreen()->GetCurItem();

    if( !newsegment )  /* first point : Create first wire or bus */
    {
        s_ConnexionStartPoint = cursorpos;
        s_OldWiresList = GetScreen()->ExtractWires( true );
        GetScreen()->SchematicCleanUp( DrawPanel );

        switch( type )
        {
        default:
            newsegment = new SCH_LINE( cursorpos, LAYER_NOTES );
            break;

        case LAYER_WIRE:
            newsegment = new SCH_LINE( cursorpos, LAYER_WIRE );

            /* A junction will be created later, when we'll know the
             * segment end position, and if the junction is really needed */
            break;

        case LAYER_BUS:
            newsegment = new SCH_LINE( cursorpos, LAYER_BUS );
            break;
        }

        newsegment->SetFlags( IS_NEW );

        if( g_HVLines ) // We need 2 segments to go from a given start pin to an end point
        {
            nextsegment = new SCH_LINE( *newsegment );
            nextsegment->SetFlags( IS_NEW );
            newsegment->SetNext( nextsegment );
            nextsegment->SetBack( newsegment );
        }

        GetScreen()->SetCurItem( newsegment );
        DrawPanel->SetMouseCapture( DrawSegment, AbortCreateNewLine );
        m_itemToRepeat = NULL;
    }
    else    // A segment is in progress: terminates the current segment and add a new segment.
    {
        nextsegment = oldsegment->Next();

        if( !g_HVLines )
        {
            // if only one segment is needed and it has length = 0, do not create a new one.
            if( oldsegment->IsNull() )
                return;
        }
        else
        {
            /* if we want 2 segment and the last two have len = 0, do not
             * create a new one */
            if( oldsegment->IsNull() && nextsegment && nextsegment->IsNull() )
                return;
        }

        DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );

        /* Creates the new segment, or terminates the command
         * if the end point is on a pin, junction or an other wire or bus */
        if( GetScreen()->IsTerminalPoint( cursorpos, oldsegment->GetLayer() ) )
        {
            EndSegment( DC );
            return;
        }

        oldsegment->SetNext( GetScreen()->GetDrawItems() );
        GetScreen()->SetDrawItems( oldsegment );
        DrawPanel->CrossHairOff( DC );     // Erase schematic cursor
        oldsegment->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        DrawPanel->CrossHairOn( DC );      // Display schematic cursor

        /* Create a new segment, and chain it after the current new segment */
        if( nextsegment )
        {
            newsegment = new SCH_LINE( *nextsegment );
            nextsegment->SetStartPoint( newsegment->GetEndPoint() );
            nextsegment->SetNext( NULL );
            nextsegment->SetBack( newsegment );
            newsegment->SetNext( nextsegment );
            newsegment->SetBack( NULL );
        }
        else
        {
            newsegment = new SCH_LINE( *oldsegment );
            newsegment->SetStartPoint( oldsegment->GetEndPoint() );
        }

        newsegment->SetEndPoint( cursorpos );
        oldsegment->ClearFlags( IS_NEW );
        oldsegment->SetFlags( SELECTED );
        newsegment->SetFlags( IS_NEW );
        GetScreen()->SetCurItem( newsegment );
        DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );

        /* This is the first segment: Now we know the start segment position.
         * Create a junction if needed. Note: a junction can be needed later,
         * if the new segment is merged (after a cleanup) with an older one
         * (tested when the connection will be finished)*/
        if( oldsegment->GetStartPoint() == s_ConnexionStartPoint )
        {
            if( GetScreen()->IsJunctionNeeded( s_ConnexionStartPoint ) )
                AddJunction( DC, s_ConnexionStartPoint );
        }
    }
}


void SCH_EDIT_FRAME::EndSegment( wxDC* DC )
{
    SCH_LINE* firstsegment = (SCH_LINE*) GetScreen()->GetCurItem();
    SCH_LINE* lastsegment = firstsegment;
    SCH_LINE* segment;

    if( firstsegment == NULL )
        return;

    if( !firstsegment->IsNew() )
        return;

    /* Delete Null segments and Put line it in Drawlist */
    lastsegment = firstsegment;

    while( lastsegment )
    {
        SCH_LINE* nextsegment = lastsegment->Next();

        if( lastsegment->IsNull() )
        {
            SCH_LINE* previous_segment = lastsegment->Back();

            if( firstsegment == lastsegment )
                firstsegment = nextsegment;

            if( nextsegment )
                nextsegment->SetBack( NULL );

            if( previous_segment )
                previous_segment->SetNext( nextsegment );

            delete lastsegment;
        }

        lastsegment = nextsegment;
    }

    /* put the segment list to the main linked list */
    segment = lastsegment = firstsegment;

    while( segment )
    {
        lastsegment = segment;
        segment     = segment->Next();
        lastsegment->SetNext( GetScreen()->GetDrawItems() );
        GetScreen()->SetDrawItems( lastsegment );
    }

    DrawPanel->EndMouseCapture( -1, -1, wxEmptyString, false );
    GetScreen()->SetCurItem( NULL );

    wxPoint end_point, alt_end_point;

    /* A junction can be needed to connect the last segment
     *  usually to m_end coordinate.
     *  But if the last segment is removed by a cleanup, because of redundancy,
     * a junction can be needed to connect the previous segment m_end
     * coordinate with is also the lastsegment->m_start coordinate */
    if( lastsegment )
    {
        end_point     = lastsegment->GetEndPoint();
        alt_end_point = lastsegment->GetStartPoint();
    }

    GetScreen()->SchematicCleanUp( DrawPanel );

    /* clear flags and find last segment entered, for repeat function */
    segment = (SCH_LINE*) GetScreen()->GetDrawItems();

    while( segment )
    {
        if( segment->GetFlags() )
        {
            if( !m_itemToRepeat )
                m_itemToRepeat = segment;
        }

        segment->ClearFlags();
        segment = segment->Next();
    }

    // Automatic place of a junction on the end point, if needed
    if( lastsegment )
    {
        if( GetScreen()->IsJunctionNeeded( end_point ) )
            AddJunction( DC, end_point );
        else if( GetScreen()->IsJunctionNeeded( alt_end_point ) )
            AddJunction( DC, alt_end_point );
    }

    /* Automatic place of a junction on the start point if necessary because
     * the cleanup can suppress intermediate points by merging wire segments */
    if( GetScreen()->IsJunctionNeeded( s_ConnexionStartPoint ) )
        AddJunction( DC, s_ConnexionStartPoint );

    GetScreen()->TestDanglingEnds( DrawPanel, DC );

    /* Redraw wires and junctions which can be changed by TestDanglingEnds() */
    DrawPanel->CrossHairOff( DC );   // Erase schematic cursor
    EDA_ITEM* item = GetScreen()->GetDrawItems();

    while( item )
    {
        switch( item->Type() )
        {
        case SCH_JUNCTION_T:
        case SCH_LINE_T:
            DrawPanel->RefreshDrawingRect( item->GetBoundingBox() );
            break;

        default:
            break;
        }

        item = item->Next();
    }

    DrawPanel->CrossHairOn( DC );    // Display schematic cursor

    SaveCopyInUndoList( s_OldWiresList, UR_WIRE_IMAGE );
    s_OldWiresList = NULL;

    OnModify( );
}


/**
 * Function ComputeBreakPoint
 * computes the middle coordinate for 2 segments from the start point to \a aPosition
 * with the segments kept in the horizontal or vertical axis only.
 */
static void ComputeBreakPoint( SCH_LINE* aSegment, const wxPoint& aPosition )
{
    SCH_LINE* nextsegment     = aSegment->Next();
    wxPoint   middle_position = aPosition;

    if( nextsegment == NULL )
        return;
#if 0
    if( ABS( middle_position.x - aSegment->GetStartPoint().x ) <
        ABS( middle_position.y - aSegment->GetStartPoint().y ) )
        middle_position.x = aSegment->GetStartPoint().x;
    else
        middle_position.y = aSegment->GetStartPoint().y;
#else
    int iDx = aSegment->GetEndPoint().x - aSegment->GetStartPoint().x;
    int iDy = aSegment->GetEndPoint().y - aSegment->GetStartPoint().y;

    if( iDy != 0 )         // keep the first segment orientation (currently horizontal)
    {
        middle_position.x = aSegment->GetStartPoint().x;
    }
    else if( iDx != 0 )    // keep the first segment orientation (currently vertical)
    {
        middle_position.y = aSegment->GetStartPoint().y;
    }
    else
    {
        if( ABS( middle_position.x - aSegment->GetStartPoint().x ) <
            ABS( middle_position.y - aSegment->GetStartPoint().y ) )
            middle_position.x = aSegment->GetStartPoint().x;
        else
            middle_position.y = aSegment->GetStartPoint().y;
    }
#endif

    aSegment->SetEndPoint( middle_position );
    nextsegment->SetStartPoint( middle_position );
    nextsegment->SetEndPoint( aPosition );
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
        polyLine->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
    }
    else
    {
        DrawSegment( DrawPanel, DC, wxDefaultPosition, false );
    }

    screen->RemoveFromDrawList( screen->GetCurItem() );
    DrawPanel->m_mouseCaptureCallback = NULL;
    screen->SetCurItem( NULL );
}


SCH_JUNCTION* SCH_EDIT_FRAME::AddJunction( wxDC* aDC, const wxPoint& aPosition,
                                           bool aPutInUndoList )
{
    SCH_JUNCTION* junction = new SCH_JUNCTION( aPosition );

    m_itemToRepeat = junction;

    DrawPanel->CrossHairOff( aDC );     // Erase schematic cursor
    junction->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->CrossHairOn( aDC );      // Display schematic cursor

    junction->SetNext( GetScreen()->GetDrawItems() );
    GetScreen()->SetDrawItems( junction );
    OnModify();

    if( aPutInUndoList )
        SaveCopyInUndoList( junction, UR_NEW );

    return junction;
}


SCH_NO_CONNECT* SCH_EDIT_FRAME::AddNoConnect( wxDC* aDC, const wxPoint& aPosition )
{
    SCH_NO_CONNECT* NewNoConnect;

    NewNoConnect   = new SCH_NO_CONNECT( aPosition );
    m_itemToRepeat = NewNoConnect;

    DrawPanel->CrossHairOff( aDC );     // Erase schematic cursor
    NewNoConnect->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->CrossHairOn( aDC );      // Display schematic cursor

    NewNoConnect->SetNext( GetScreen()->GetDrawItems() );
    GetScreen()->SetDrawItems( NewNoConnect );
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
        screen->RemoveFromDrawList( (SCH_ITEM*) screen->GetCurItem() );
        screen->SetCurItem( NULL );
        screen->ReplaceWires( s_OldWiresList );
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
        ( (SCH_COMPONENT*) m_itemToRepeat )->m_TimeStamp = GetNewTimeStamp();
        m_itemToRepeat->Move( pos );
        m_itemToRepeat->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
        MoveItem( m_itemToRepeat, DC );
        return;
    }

    m_itemToRepeat->Move( wxPoint( g_RepeatStep.GetWidth(), g_RepeatStep.GetHeight() ) );

    if( m_itemToRepeat->CanIncrementLabel() )
        ( (SCH_TEXT*) m_itemToRepeat )->IncrementLabel();

    if( m_itemToRepeat )
    {
        m_itemToRepeat->SetNext( GetScreen()->GetDrawItems() );
        GetScreen()->SetDrawItems( m_itemToRepeat );
        GetScreen()->TestDanglingEnds();
        m_itemToRepeat->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        SaveCopyInUndoList( m_itemToRepeat, UR_NEW );
        m_itemToRepeat->ClearFlags();
    }
}
