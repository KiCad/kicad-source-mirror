/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file editedge.cpp
 * @brief Edit segments and edges of PCB.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <gr_basic.h>
#include <pcbcommon.h>

#include <pcbnew.h>
#include <protos.h>
#include <macros.h>

#include <class_board.h>
#include <class_drawsegment.h>


static void Abort_EditEdge( EDA_DRAW_PANEL* aPanel, wxDC* DC );
static void DrawSegment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition, bool aErase );
static void Move_Segment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                          bool aErase );


static wxPoint s_InitialPosition;  // Initial cursor position.
static wxPoint s_LastPosition;     // Current cursor position.


// Start move of a graphic element type DRAWSEGMENT
void PCB_EDIT_FRAME::Start_Move_DrawItem( DRAWSEGMENT* drawitem, wxDC* DC )
{
    if( drawitem == NULL )
        return;

    drawitem->Draw( m_canvas, DC, GR_XOR );
    drawitem->SetFlags( IS_MOVED );
    s_InitialPosition = s_LastPosition = GetCrossHairPosition();
    SetMsgPanel( drawitem );
    m_canvas->SetMouseCapture( Move_Segment, Abort_EditEdge );
    SetCurItem( drawitem );
    m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
}


/*
 * Place graphic element of type DRAWSEGMENT.
 */
void PCB_EDIT_FRAME::Place_DrawItem( DRAWSEGMENT* drawitem, wxDC* DC )
{
    if( drawitem == NULL )
        return;

    drawitem->ClearFlags();
    SaveCopyInUndoList(drawitem, UR_MOVED, s_LastPosition - s_InitialPosition);
    drawitem->Draw( m_canvas, DC, GR_OR );
    m_canvas->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );
    OnModify();
}

/*
 * Redraw segment during cursor movement.
 */
static void Move_Segment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                          bool aErase )
{
    DRAWSEGMENT* segment = (DRAWSEGMENT*) aPanel->GetScreen()->GetCurItem();

    if( segment == NULL )
        return;

    if( aErase )
        segment->Draw( aPanel, aDC, GR_XOR );

    wxPoint delta;
    delta = aPanel->GetParent()->GetCrossHairPosition() - s_LastPosition;

    segment->SetStart( segment->GetStart() + delta );
    segment->SetEnd(   segment->GetEnd()   + delta );

    s_LastPosition = aPanel->GetParent()->GetCrossHairPosition();

    segment->Draw( aPanel, aDC, GR_XOR );
}


void PCB_EDIT_FRAME::Delete_Segment_Edge( DRAWSEGMENT* Segment, wxDC* DC )
{
    EDA_ITEM* PtStruct;
    int       track_fill_copy = DisplayOpt.DisplayDrawItems;

    if( Segment == NULL )
        return;

    if( Segment->IsNew() )  // Trace in progress.
    {
        // Delete current segment.
        DisplayOpt.DisplayDrawItems = SKETCH;
        Segment->Draw( m_canvas, DC, GR_XOR );
        PtStruct = Segment->Back();
        Segment ->DeleteStructure();

        if( PtStruct && (PtStruct->Type() == PCB_LINE_T ) )
            Segment = (DRAWSEGMENT*) PtStruct;

        DisplayOpt.DisplayDrawItems = track_fill_copy;
        SetCurItem( NULL );
    }
    else if( Segment->GetFlags() == 0 )
    {
        Segment->Draw( m_canvas, DC, GR_XOR );
        Segment->ClearFlags();
        SaveCopyInUndoList(Segment, UR_DELETED);
        Segment->UnLink();
        SetCurItem( NULL );
        OnModify();
    }
}


void PCB_EDIT_FRAME::Delete_Drawings_All_Layer( LAYER_NUM aLayer )
{
    if( aLayer <= LAST_COPPER_LAYER )
    {
        DisplayError( this, _( "Copper layer global delete not allowed!" ) );
        return;
    }

    wxString msg = wxString::Format(
        _( "Delete everything on layer %s?" ),
        GetChars( GetBoard()->GetLayerName( aLayer ) ) );

    if( !IsOK( this, msg ) )
        return;

    PICKED_ITEMS_LIST   pickList;
    ITEM_PICKER         picker( NULL, UR_DELETED );
    BOARD_ITEM*         PtNext;

    for( BOARD_ITEM* item = GetBoard()->m_Drawings;  item;  item = PtNext )
    {
        PtNext = item->Next();

        switch( item->Type() )
        {
        case PCB_LINE_T:
        case PCB_TEXT_T:
        case PCB_DIMENSION_T:
        case PCB_TARGET_T:
            if( item->GetLayer() == aLayer )
            {
                item->UnLink();
                picker.SetItem( item );
                pickList.PushItem( picker );
            }

            break;

        default:
        {
            wxString msg;
            msg.Printf( wxT("Delete_Drawings_All_Layer() error: unknown type %d"),
                        item->Type() );
            wxMessageBox( msg );
            break;
        }
        }
    }

    if( pickList.GetCount() )
    {
        OnModify();
        SaveCopyInUndoList(pickList, UR_DELETED);
    }
}


static void Abort_EditEdge( EDA_DRAW_PANEL* aPanel, wxDC* DC )
{
    DRAWSEGMENT* Segment = (DRAWSEGMENT*) aPanel->GetScreen()->GetCurItem();

    if( Segment == NULL )
    {
        aPanel->SetMouseCapture( NULL, NULL );
        return;
    }

    if( Segment->IsNew() )
    {
        aPanel->CallMouseCapture( DC, wxDefaultPosition, false );
        Segment ->DeleteStructure();
        Segment = NULL;
    }
    else
    {
        wxPoint pos = aPanel->GetParent()->GetCrossHairPosition();
        aPanel->GetParent()->SetCrossHairPosition( s_InitialPosition );
        aPanel->CallMouseCapture( DC, wxDefaultPosition, true );
        aPanel->GetParent()->SetCrossHairPosition( pos );
        Segment->ClearFlags();
        Segment->Draw( aPanel, DC, GR_OR );
    }

#ifdef USE_WX_OVERLAY
    aPanel->Refresh();
#endif

    aPanel->SetMouseCapture( NULL, NULL );
    ( (PCB_EDIT_FRAME*) aPanel->GetParent() )->SetCurItem( NULL );
}


/* Initialize the drawing of a segment of type other than trace.
 */
DRAWSEGMENT* PCB_EDIT_FRAME::Begin_DrawSegment( DRAWSEGMENT* Segment, STROKE_T shape, wxDC* DC )
{
    int          s_large;
    DRAWSEGMENT* DrawItem;

    s_large = GetDesignSettings().m_DrawSegmentWidth;

    if( GetActiveLayer() == EDGE_N )
    {
        s_large = GetDesignSettings().m_EdgeSegmentWidth;
    }

    if( Segment == NULL )        // Create new trace.
    {
        SetCurItem( Segment = new DRAWSEGMENT( GetBoard() ) );
        Segment->SetFlags( IS_NEW );
        Segment->SetLayer( GetActiveLayer() );
        Segment->SetWidth( s_large );
        Segment->SetShape( shape );
        Segment->SetAngle( 900 );
        Segment->SetStart( GetCrossHairPosition() );
        Segment->SetEnd( GetCrossHairPosition() );
        m_canvas->SetMouseCapture( DrawSegment, Abort_EditEdge );
    }
    else    /* The ending point ccordinate Segment->m_End was updated by he function
             * DrawSegment() called on a move mouse event
             * during the segment creation
             */
    {
        if( Segment->GetStart() != Segment->GetEnd() )
        {
            if( Segment->GetShape() == S_SEGMENT )
            {
                SaveCopyInUndoList( Segment, UR_NEW );
                GetBoard()->Add( Segment );

                OnModify();
                Segment->ClearFlags();

                Segment->Draw( m_canvas, DC, GR_OR );

                DrawItem = Segment;

                SetCurItem( Segment = new DRAWSEGMENT( GetBoard() ) );

                Segment->SetFlags( IS_NEW );
                Segment->SetLayer( DrawItem->GetLayer() );
                Segment->SetWidth( s_large );
                Segment->SetShape( DrawItem->GetShape() );
                Segment->SetType( DrawItem->GetType() );
                Segment->SetAngle( DrawItem->GetAngle() );
                Segment->SetStart( DrawItem->GetEnd() );
                Segment->SetEnd( DrawItem->GetEnd() );
                DrawSegment( m_canvas, DC, wxDefaultPosition, false );
            }
            else
            {
                End_Edge( Segment, DC );
                Segment = NULL;
            }
        }
    }

    return Segment;
}


void PCB_EDIT_FRAME::End_Edge( DRAWSEGMENT* Segment, wxDC* DC )
{
    if( Segment == NULL )
        return;

    Segment->Draw( m_canvas, DC, GR_OR );

    // Delete if segment length is zero.
    if( Segment->GetStart() == Segment->GetEnd() )
    {
        Segment->DeleteStructure();
    }
    else
    {
        Segment->ClearFlags();
        GetBoard()->Add( Segment );
        OnModify();
        SaveCopyInUndoList( Segment, UR_NEW );
    }

    m_canvas->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );
}


/* Redraw segment during cursor movement
 */
static void DrawSegment( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition, bool aErase )
{
    DRAWSEGMENT* Segment = (DRAWSEGMENT*) aPanel->GetScreen()->GetCurItem();
    int          t_fill = DisplayOpt.DisplayDrawItems;

    if( Segment == NULL )
        return;

    DisplayOpt.DisplayDrawItems = SKETCH;

    if( aErase )
        Segment->Draw( aPanel, aDC, GR_XOR );

    if( g_Segments_45_Only && Segment->GetShape() == S_SEGMENT )
    {
        wxPoint pt;

        CalculateSegmentEndPoint( aPanel->GetParent()->GetCrossHairPosition(),
                                  Segment->GetStart().x, Segment->GetStart().y,
                                  &pt.x, &pt.y );
        Segment->SetEnd( pt );
    }
    else    // here the angle is arbitrary
    {
        Segment->SetEnd( aPanel->GetParent()->GetCrossHairPosition() );
    }

    Segment->Draw( aPanel, aDC, GR_XOR );
    DisplayOpt.DisplayDrawItems = t_fill;
}
