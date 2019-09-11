/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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
 * @file drag_round_corner.cpp  maui RF all file
 * @brief Routines used to create round trace corners
 */


#include <fctsys.h>
#include <common.h>
#include <trigo.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <pcb_base_frame.h>
#include <macros.h>

#include <drag.h>
#include <pcbnew.h>

#include <pcb_edit_frame.h>  // maui RF
#include <pcbnew_id.h>  // maui RF

#include <class_module.h>
#include <class_board.h>
#include <connectivity/connectivity_data.h>


#define N_SEGMENTS 32

static void Show_MoveNode( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                           bool aErase );
static void Abort_CreateRoundTrack( EDA_DRAW_PANEL* Panel, wxDC* DC );

static PICKED_ITEMS_LIST s_ItemsListPicker;

static wxPoint s_PosInit;
static TRACK *s_FirstSegment, *s_SecondSegment;
static std::vector<TRACK*> s_RoundSegmentList;

static wxPoint s_Line2Vec, s_TransformMove;
static double s_TransformRotate, s_Line2Angle;


/** Abort function for creating a round track corner
 */
static void Abort_CreateRoundTrack( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) aPanel->GetParent();
    BOARD * pcb = frame->GetBoard();

    pcb->HighLightOFF();
    pcb->PopHighLight();

    frame->SetCurItem( NULL );
    aPanel->SetMouseCapture( NULL, NULL );

    //// Undo move and redraw trace segments.
    //for( unsigned jj=0 ; jj < s_RoundSegmentList.size(); jj++ )
    //{
    //    TRACK* track = s_RoundSegmentList[jj].m_Track;
    //    s_RoundSegmentList[jj].RestoreInitialValues();
    //    track->SetState( IN_EDIT, false );
    //    track->ClearFlags();
    //}

    // Clear the undo picker list:
    s_ItemsListPicker.ClearListAndDeleteItems();
    EraseDragList();
    aPanel->Refresh();
}

static wxPoint TransformToAngleCoords(wxPoint in_pos){
    wxPoint ret_val = in_pos;
    ret_val = ret_val - s_TransformMove;
    RotatePoint(&ret_val, s_TransformRotate);
    return ret_val;
}

static wxPoint TransformFromAngleCoords(wxPoint in_pos){
    wxPoint ret_val = in_pos;
    RotatePoint(&ret_val, -s_TransformRotate);
    ret_val = ret_val + s_TransformMove;
    return ret_val;
}

static wxPoint GetCircleCoords(wxPoint in_pos){
    wxPoint cursor_transformed = TransformToAngleCoords(in_pos);
    double distance_line1 = cursor_transformed.x;
    double distance_line2 = (double)(cursor_transformed.x) * s_Line2Vec.x + (double)(cursor_transformed.y) * s_Line2Vec.y;
    distance_line2 /= EuclideanNorm(s_Line2Vec);
    double chosen_distance = (distance_line1 < distance_line2) ? distance_line2 : distance_line1;
    if(chosen_distance < 0.0) chosen_distance = 0.0;

    wxPoint ret_circle_coords;
    ret_circle_coords.x = chosen_distance;
    ret_circle_coords.y = ret_circle_coords.x*tan(DECIDEG2RAD(s_Line2Angle/2));

    return ret_circle_coords;
}

static wxPoint CircleCoord(wxPoint in_pos, double in_decideg){
    wxPoint ret_pos;
    ret_pos.x = in_pos.x + cosdecideg( abs(in_pos.y), in_decideg );
    ret_pos.y = in_pos.y + sindecideg( abs(in_pos.y), in_decideg );
    return ret_pos;
}

static void RearrangeRoundSegments(wxPoint in_circle_center){
    double start_angle, end_angle, angle_step;

    // Whether or not the circle is above or below the transformed X axis affects what angle we start and end at
    if(in_circle_center.y < 0.0){
        start_angle = 900.0;
        end_angle = 2700.0 + ArcTangente(s_Line2Vec.y, s_Line2Vec.x);
    } else {
        start_angle = -900.0;
        end_angle = -900.0 - (1800.0 - ArcTangente(s_Line2Vec.y, s_Line2Vec.x));
    }
    angle_step = ( end_angle - start_angle ) / N_SEGMENTS;

    s_FirstSegment->SetEnd(TransformFromAngleCoords(CircleCoord(in_circle_center, start_angle)));
    s_SecondSegment->SetStart(TransformFromAngleCoords(CircleCoord(in_circle_center, end_angle)));

    double cur_angle = start_angle;
    for( unsigned ii = 0; ii < s_RoundSegmentList.size(); ii++){
        s_RoundSegmentList[ii]->SetStart(TransformFromAngleCoords(CircleCoord(in_circle_center, cur_angle)));
        s_RoundSegmentList[ii]->SetEnd(TransformFromAngleCoords(CircleCoord(in_circle_center, cur_angle+angle_step)));

        cur_angle += angle_step;
    }
}


// Redraw the moved node according to the mouse cursor position
static void Show_MoveNode( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                           bool aErase )
{
    // DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*) aPanel->GetDisplayOptions();
    PCB_DISPLAY_OPTIONS* displ_opts = (PCB_DISPLAY_OPTIONS*) aPanel->GetDisplayOptions();
    int          tmp = displ_opts->m_DisplayPcbTrackFill;
    GR_DRAWMODE  draw_mode = GR_XOR | GR_HIGHLIGHT;
    displ_opts->m_DisplayPcbTrackFill = false;

#ifndef USE_WX_OVERLAY
    aErase = true;
#else
    aErase = false;
#endif

    // set the new track coordinates
    wxPoint Pos = aPanel->GetParent()->GetCrossHairPosition();

    wxPoint circle_pos = GetCircleCoords(Pos);
    RearrangeRoundSegments(circle_pos);

    //Redraw all the current circle segments
    for( unsigned ii = 0; ii < s_RoundSegmentList.size(); ii++ )
        s_RoundSegmentList[ii]->Draw( aPanel, aDC, draw_mode );

    displ_opts->m_DisplayPcbTrackFill = tmp;


}

void PCB_EDIT_FRAME::Start_DragRoundCorner( TRACK* aTrack, wxDC* aDC )
{
    if( !aTrack )
        return;

    s_RoundSegmentList.clear();

    // Change highlighted net: the new one will be highlighted
    GetBoard()->PushHighLight();

    if( GetBoard()->IsHighLightNetON() )
        HighLight( aDC );

    STATUS_FLAGS diag = aTrack->IsPointOnEnds( GetCrossHairPosition(), -1 );

    TRACK *next_segment = (diag & STARTPOINT) ? aTrack->GetTrack( GetBoard()->m_Track, NULL, ENDPOINT_START, true, false ) :
                                                aTrack->GetTrack( GetBoard()->m_Track, NULL, ENDPOINT_END, true, false );

    //Swap the connecting segment's start and end point in case two ends (or starts) are connected together
    STATUS_FLAGS next_segment_diag = next_segment->IsPointOnEnds( GetCrossHairPosition(), -1 );
    if( next_segment_diag == diag ) {
        wxPoint new_end = next_segment->GetStart();
        next_segment->SetStart( next_segment->GetEnd() );
        next_segment->SetEnd( new_end );
    }

    //TODO: Put some error checking in here...
    //Re-order the two converging tracks if we're selecting the startpoint
    s_FirstSegment = (diag & STARTPOINT) ? next_segment : aTrack;
    s_SecondSegment = (diag & STARTPOINT) ? aTrack : next_segment;
    s_PosInit = s_FirstSegment->GetEnd();
    s_TransformMove.x = s_PosInit.x;
    s_TransformMove.y = s_PosInit.y;
    s_TransformRotate = ArcTangente( (s_FirstSegment->GetStart().y - s_PosInit.y),
                                     (s_FirstSegment->GetStart().x - s_PosInit.x) );
    s_Line2Vec = TransformToAngleCoords( s_SecondSegment->GetEnd() );
    s_Line2Angle = ArcTangente( s_Line2Vec.y, s_Line2Vec.x );

    // Prepare the Undo command
    ITEM_PICKER picker( s_SecondSegment, UR_CHANGED );
    picker.SetLink( s_SecondSegment->Clone() );
    s_ItemsListPicker.PushItem( picker );
    
    s_FirstSegment->SetStatus( 0 );
    s_FirstSegment->ClearFlags();
    s_SecondSegment->SetStatus( 0 );
    s_SecondSegment->ClearFlags();

    //Start by breaking first segment into N_SEGMENTS different segments
    wxPoint start_point = s_FirstSegment->GetStart();
    wxPoint end_point = s_FirstSegment->GetEnd();
    for(int ii=0; ii < N_SEGMENTS; ii++){
        double fraction_down_line = (double)(N_SEGMENTS-ii)/(N_SEGMENTS+1);
        wxPoint pos = (start_point*(1.0-fraction_down_line)) + (end_point*fraction_down_line);
        TRACK *new_track = GetBoard()->CreateLockPoint(pos, s_FirstSegment, &s_ItemsListPicker);
        s_RoundSegmentList.push_back(new_track);
    }
    
    // SaveCopyInUndoList( s_ItemsListPicker, UR_UNSPECIFIED ); // maui RF remove temporary Undo Option TODO fix undo
    // evaluate void PCB_EDIT_FRAME::OnActionPlugin( wxCommandEvent& aEvent ) to save the action maui RF
    
    aTrack->SetFlags( IS_DRAGGED );

    m_canvas->SetMouseCapture( Show_MoveNode, Abort_CreateRoundTrack );

    GetBoard()->SetHighLightNet( aTrack->GetNetCode() );
    GetBoard()->HighLightON();

    GetBoard()->DrawHighLight( m_canvas, aDC, GetBoard()->GetHighLightNetCode() );
    m_canvas->CallMouseCapture( aDC, wxDefaultPosition, true );

    UndrawAndMarkSegmentsToDrag( m_canvas, aDC );
}

// Place a dragged (or moved) track segment or via
bool PCB_EDIT_FRAME::PlaceRoundTrackSegment( TRACK* Track, wxDC* DC )
{
    int        errdrc;

    if( Track == NULL )
        return false;
//
//    int current_net_code = Track->GetNetCode();
//
//    // DRC control:
//    if( g_Drc_On )
//    {
//        errdrc = m_drc->Drc( Track, GetBoard()->m_Track );
//
//        if( errdrc == BAD_DRC )
//            return false;
//
//        // Redraw the dragged segments
//        for( unsigned ii = 0; ii < s_RoundSegmentList.size(); ii++ )
//        {
//            errdrc = m_drc->Drc( s_RoundSegmentList[ii].m_Track, GetBoard()->m_Track );
//
//            if( errdrc == BAD_DRC )
//                return false;
//        }
//    }
//
//    // DRC Ok: place track segments
//    Track->ClearFlags();
//    Track->SetState( IN_EDIT, false );
//
//    // Draw dragged tracks
//    for( unsigned ii = 0; ii < s_RoundSegmentList.size(); ii++ )
//    {
//        Track = s_RoundSegmentList[ii].m_Track;
//        Track->SetState( IN_EDIT, false );
//        Track->ClearFlags();
//
//        /* Test the connections modified by the move
//         *  (only pad connection must be tested, track connection will be
//         * tested by TestNetConnection() ) */
//        LSET layerMask( Track->GetLayer() );
//
//        Track->start = GetBoard()->GetPadFast( Track->GetStart(), layerMask );
//
//        if( Track->start )
//            Track->SetState( BEGIN_ONPAD, true );
//        else
//            Track->SetState( BEGIN_ONPAD, false );
//
//        Track->end = GetBoard()->GetPadFast( Track->GetEnd(), layerMask );
//
//        if( Track->end )
//            Track->SetState( END_ONPAD, true );
//        else
//            Track->SetState( END_ONPAD, false );
//    }
//
//    EraseDragList();
//
//    SaveCopyInUndoList( s_ItemsListPicker, UR_UNSPECIFIED );
//    s_ItemsListPicker.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items
//
//    GetBoard()->PopHighLight();
//
//    OnModify();
//    m_canvas->SetMouseCapture( NULL, NULL );
//
//    if( current_net_code > 0 )
//        TestNetConnection( DC, current_net_code );
//
//    m_canvas->Refresh();

    return true;
}
