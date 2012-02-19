/*********************************/
/* functions used to drag tracks */
/*********************************/

/* Fichier dragsegm.cpp */

#include <fctsys.h>
#include <common.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <wxBasePcbFrame.h>

#include <drag.h>
#include <pcbnew.h>

#include <class_module.h>
#include <class_board.h>


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
    if( g_DragSegmentList.size() == 0 )
        return;

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        wxPoint pos;

        TRACK* track = g_DragSegmentList[ii].m_Segm;

#ifndef USE_WX_OVERLAY
        track->Draw( panel, DC, GR_XOR );   // erase from screen at old position
#endif

        D_PAD* pad = g_DragSegmentList[ii].m_Pad_Start;

        if( pad )
        {
            pos = pad->GetPosition() - g_Offset_Module;
            track->m_Start = pos;
        }

        pad = g_DragSegmentList[ii].m_Pad_End;

        if( pad )
        {
            pos = pad->GetPosition() - g_Offset_Module;
            track->m_End = pos;
        }

        track->Draw( panel, DC, GR_XOR );
    }
}


/** Build the list of track segments connected to pads of a given module
 *  by populate the std::vector<DRAG_SEGM> g_DragSegmentList
 *  For each selected track segment set the EDIT flag
 *  and redraw them in EDIT mode (sketch mode)
 */
void Build_Drag_Liste( EDA_DRAW_PANEL* panel, wxDC* DC, MODULE* aModule )
{
    for( D_PAD* pad = aModule->m_Pads;  pad;  pad = pad->Next() )
    {
        Build_1_Pad_SegmentsToDrag( panel, DC, pad );
    }

    return;
}


/** Build the list of track segments connected to a given pad
 *  by populate the std::vector<DRAG_SEGM> g_DragSegmentList
 *  For each selected track segment set the EDIT flag
 *  and redraw them in EDIT mode (sketch mode)
 *  Net codes must be OK.
 */
void Build_1_Pad_SegmentsToDrag( EDA_DRAW_PANEL* panel, wxDC* DC, D_PAD* aPad )
{
    BOARD*  pcb = ( (PCB_BASE_FRAME*)( panel->GetParent() ) )->GetBoard();

    int     net_code = aPad->GetNet();

    TRACK*  track = pcb->m_Track->GetStartNetCode( net_code );

    wxPoint pos = aPad->GetPosition();

    int     layerMask = aPad->GetLayerMask();

    for( ; track; track = track->Next() )
    {
        if( track->GetNet() != net_code )
            break;

        if( ( layerMask & track->ReturnMaskLayer() ) == 0 )
            continue;

        if( pos == track->m_Start )
        {
            AddSegmentToDragList( panel, DC, STARTPOINT, track );
            g_DragSegmentList.back().m_Pad_Start = aPad;
        }

        if( pos == track->m_End )
        {
            AddSegmentToDragList( panel, DC, ENDPOINT, track );
            g_DragSegmentList.back().m_Pad_End = aPad;
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
        Track->SetFlags( STARTPOINT );

    if( (flag & ENDPOINT) )
        Track->SetFlags( ENDPOINT );

    Track->Draw( panel, DC, GR_XOR );
    g_DragSegmentList.push_back( wrapper );
}


/* Build the list of tracks connected to the ref point
 * Net codes must be up to date, because only tracks having the right net code are tested.
 * @param aRefPos = reference point of connection
 */
void Collect_TrackSegmentsToDrag( EDA_DRAW_PANEL* panel, wxDC* DC,
                                  wxPoint& aRefPos, int LayerMask, int net_code )
{
    BOARD* pcb = ( (PCB_BASE_FRAME*)( panel->GetParent() ) )->GetBoard();

    TRACK* track = pcb->m_Track->GetStartNetCode( net_code );

    for( ; track; track = track->Next() )
    {
        if( track->GetNet() != net_code )   // not the same netcodenet code: all candidates tested
            break;

        if( ( LayerMask & track->ReturnMaskLayer() ) == 0 )
            continue;                       // Cannot be connected, not on the same layer

        if( track->IsDragging() )
            continue;                       // already put in list

        int flag = 0;

        if( (track->m_Start == aRefPos) && ((track->GetFlags() & STARTPOINT) == 0) )
            flag |= STARTPOINT;

        if( track->m_End == aRefPos && ((track->GetFlags() & ENDPOINT) == 0)  )
            flag |= ENDPOINT;

        // Note: vias will be flagged with both STARTPOINT and ENDPOINT
        // and must not be entered twice.
        if( flag )
        {
            AddSegmentToDragList( panel, DC, flag, track );

            // If a connected via is found at location aRefPos,
            // collect also tracks connected by this via.
            if( track->Type() == PCB_VIA_T )
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
        g_DragSegmentList[ii].m_Segm->ClearFlags();

    g_DragSegmentList.clear();
}
