/*********************************/
/* functions used to drag tracks */
/*********************************/

/* Fichier dragsegm.cpp */

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "pcbnew.h"

#include "drag.h"

/* a list of DRAG_SEGM items used to move or drag tracks */
std::vector<DRAG_SEGM> g_DragSegmentList;

/* helper class to handle a list of track segments to drag or move
 */
DRAG_SEGM::DRAG_SEGM( TRACK* segm )
{
    m_Segm = segm;
    m_StartInitialValue = m_Segm->m_Start;
    m_EndInitialValue   = m_Segm->m_End;
    m_Pad_Start = m_Pad_End = NULL;
    m_Flag = 0;
}


/* Redraw the list of segments starting in g_DragSegmentList, while moving a footprint */
void DrawSegmentWhileMovingFootprint( EDA_DRAW_PANEL* panel, wxDC* DC )
{
    D_PAD* pt_pad;
    TRACK* Track;

    if( g_DragSegmentList.size() == 0 )
        return;

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        wxPoint pos;
        Track = g_DragSegmentList[ii].m_Segm;
#ifndef USE_WX_OVERLAY
        Track->Draw( panel, DC, GR_XOR );   // erase from screen at old position
#endif
        pt_pad = g_DragSegmentList[ii].m_Pad_Start;

        if( pt_pad )
        {
            pos = pt_pad->m_Pos - g_Offset_Module;
            Track->m_Start = pos;
        }

        pt_pad = g_DragSegmentList[ii].m_Pad_End;

        if( pt_pad )
        {
            pos = pt_pad->m_Pos - g_Offset_Module;
            Track->m_End = pos;
        }

        Track->Draw( panel, DC, GR_XOR );
    }
}


/** Build the list of track segments connected to pads of a given module
 *  by populate the std::vector<DRAG_SEGM> g_DragSegmentList
 *  For each selected track segment set the EDIT flag
 *  and redraw them in EDIT mode (sketch mode)
 */
void Build_Drag_Liste( EDA_DRAW_PANEL* panel, wxDC* DC, MODULE* Module )
{
    D_PAD* pt_pad;

    pt_pad = Module->m_Pads;

    for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Next() )
    {
        Build_1_Pad_SegmentsToDrag( panel, DC, pt_pad );
    }

    return;
}


/** Build the list of track segments connected to a given pad
 *  by populate the std::vector<DRAG_SEGM> g_DragSegmentList
 *  For each selected track segment set the EDIT flag
 *  and redraw them in EDIT mode (sketch mode)
 *  Net codes must be OK.
 */
void Build_1_Pad_SegmentsToDrag( EDA_DRAW_PANEL* panel, wxDC* DC, D_PAD* PtPad )
{
    TRACK*  Track;
    int     net_code = PtPad->GetNet();
    int     LayerMask;
    wxPoint pos;
    BOARD*  pcb = ( (PCB_BASE_FRAME*)( panel->GetParent() ) )->GetBoard();

    Track = pcb->m_Track->GetStartNetCode( net_code );

    pos = PtPad->m_Pos;
    LayerMask = PtPad->m_layerMask;

    for( ; Track; Track = Track->Next() )
    {
        if( Track->GetNet() != net_code )
            break;

        if( ( LayerMask & Track->ReturnMaskLayer() ) == 0 )
            continue;

        if( pos == Track->m_Start )
        {
            AddSegmentToDragList( panel, DC, STARTPOINT, Track );
            g_DragSegmentList.back().m_Pad_Start = PtPad;
        }

        if( pos == Track->m_End )
        {
            AddSegmentToDragList( panel, DC, ENDPOINT, Track );
            g_DragSegmentList.back().m_Pad_End = PtPad;
        }
    }
}


/* Add the segment"Track" to the drag list, and erase it from screen
 *  flag = STARTPOINT (if the point to drag is the start point of Track) or ENDPOINT
 */
void AddSegmentToDragList( EDA_DRAW_PANEL* panel, wxDC* DC, int flag, TRACK* Track )
{
    DRAG_SEGM wrapper( Track );

    if( (flag & STARTPOINT) )
        wrapper.m_Flag |= 1;

    if( (flag & ENDPOINT) )
        wrapper.m_Flag |= 2;

    Track->Draw( panel, DC, GR_XOR );
    Track->SetState( IN_EDIT, ON );

    if( (flag & STARTPOINT) )
        Track->m_Flags |= STARTPOINT;

    if( (flag & ENDPOINT) )
        Track->m_Flags |= ENDPOINT;

    Track->Draw( panel, DC, GR_XOR );
    g_DragSegmentList.push_back( wrapper );
}


/* Build the list of tracks connected to the ref point
 *  Net codes must be OK.
 * @param aRefPos = reference point of connection
 */
void Collect_TrackSegmentsToDrag( EDA_DRAW_PANEL* panel, wxDC* DC,
                                  wxPoint& aRefPos, int LayerMask, int net_code )
{
    BOARD* pcb = ( (PCB_BASE_FRAME*)( panel->GetParent() ) )->GetBoard();

    TRACK* track = pcb->m_Track->GetStartNetCode( net_code );

    for( ; track; track = track->Next() )
    {
        if( track->GetNet() != net_code )   // Bad net, not connected
            break;

        if( ( LayerMask & track->ReturnMaskLayer() ) == 0 )
            continue;                       // Cannot be connected, not on the same layer

        if( track->m_Flags & IS_DRAGGED )
            continue;                       // already put in list

        int flag = 0;

        if( (track->m_Start == aRefPos) && ((track->m_Flags & STARTPOINT) == 0) )
            flag |= STARTPOINT;

        if( track->m_End == aRefPos && ((track->m_Flags & ENDPOINT) == 0)  )
            flag |= ENDPOINT;

        // Note: vias will be flagged with both STARTPOINT and ENDPOINT
        // and must not be entered twice.
        if( flag )
        {
            AddSegmentToDragList( panel, DC, flag, track );

            // If a connected via is found at location aRefPos,
            // collect also tracks connected by this via.
            if( track->Type() == TYPE_VIA )
                Collect_TrackSegmentsToDrag( panel, DC, aRefPos, track->ReturnMaskLayer(),
                                             net_code );
        }
    }
}


/**
 * Function EraseDragList
 * clear the .m_Flags of all track segments found in g_DragSegmentList
 * and clear the list.
 * the memory is not freed and will be reused when creating a new list
 */
void EraseDragList()
{
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
        g_DragSegmentList[ii].m_Segm->m_Flags = 0;

    g_DragSegmentList.clear();
}
