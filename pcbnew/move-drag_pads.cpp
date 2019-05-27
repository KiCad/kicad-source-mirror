

/**
 * @file move-drag_pads.cpp
 * @brief Edit footprint pads.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <class_drawpanel.h>
#include <trigo.h>
#include <pcb_base_frame.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <drag.h>


static wxPoint Pad_OldPos;


// Routine to place a moved pad.
void PCB_BASE_FRAME::PlacePad( D_PAD* aPad, wxDC* DC )
{
    int     dX, dY;
    TRACK*  track;

    if( aPad == NULL )
        return;

    MODULE* module = aPad->GetParent();

    ITEM_PICKER       picker( NULL, UR_CHANGED );
    PICKED_ITEMS_LIST pickList;

    // Save dragged track segments in undo list
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        track = g_DragSegmentList[ii].m_Track;

        // Set the old state
        if( g_DragSegmentList[ii].m_Pad_Start )
            track->SetStart( Pad_OldPos );

        if( g_DragSegmentList[ii].m_Pad_End )
            track->SetEnd( Pad_OldPos );

        picker.SetItem( track );
        pickList.PushItem( picker );
    }

    // Save old module and old items values
    aPad->ClearFlags();
    wxPoint pad_curr_position = aPad->GetPosition();

    aPad->SetPosition( Pad_OldPos );

    if( g_DragSegmentList.size() == 0 )
        SaveCopyInUndoList( module, UR_CHANGED );
    else
    {
        picker.SetItem( module );
        pickList.PushItem( picker );
        SaveCopyInUndoList( pickList, UR_CHANGED );
    }

    aPad->SetPosition( pad_curr_position );
    aPad->Draw( m_canvas, DC, GR_XOR );

    // Redraw dragged track segments
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        track = g_DragSegmentList[ii].m_Track;

        // Set the new state
        if( g_DragSegmentList[ii].m_Pad_Start )
            track->SetStart( aPad->GetPosition() );

        if( g_DragSegmentList[ii].m_Pad_End )
            track->SetEnd( aPad->GetPosition() );

        if( DC )
            track->Draw( m_canvas, DC, GR_XOR );

        track->SetState( IN_EDIT, false );
        track->ClearFlags();

        if( DC )
            track->Draw( m_canvas, DC, GR_OR );
    }

    // Compute local coordinates (i.e refer to module position and for module orient = 0)
    dX = aPad->GetPosition().x - Pad_OldPos.x;
    dY = aPad->GetPosition().y - Pad_OldPos.y;

    RotatePoint( &dX, &dY, -module->GetOrientation() );

    aPad->SetX0( dX + aPad->GetPos0().x );
    aPad->SetY0( dY + aPad->GetPos0().y );

    if( DC )
        aPad->Draw( m_canvas, DC, GR_OR );

    module->CalculateBoundingBox();
    module->SetLastEditTime();

    EraseDragList();

    OnModify();
    m_canvas->SetMouseCapture( NULL, NULL );
}
