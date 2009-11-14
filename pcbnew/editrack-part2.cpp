/*******************************/
/* Edit tracks          */
/*******************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"

#include "protos.h"


/* Displays or hides the ratsnest. */
void WinEDA_PcbFrame::Ratsnest_On_Off( wxDC* DC )
{
    unsigned      ii;

    if( ( GetBoard()->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK ) == 0 )
    {
        if( g_Show_Ratsnest )
            Compile_Ratsnest( DC, TRUE );
        return;
    }

    DrawGeneralRatsnest( DC, 0 );

    if( GetBoard()->GetRatsnestsCount() == 0 )
        return;

    if( g_Show_Ratsnest )
    {
        for( ii = 0; ii < GetBoard()->GetRatsnestsCount(); ii++ )
        {
            GetBoard()->m_FullRatsnest[ii].m_Status |= CH_VISIBLE;
        }
    }
    else
    {
        for( ii = 0; ii < GetBoard()->GetRatsnestsCount(); ii++ )
        {
            GetBoard()->m_FullRatsnest[ii].m_Status &= ~CH_VISIBLE;
        }
    }
}


/*
 * Exchange layer the track pointed to by the mouse:
 * The track must be on one layer of work,
 * It is put on another layer of work, if possible
 * (Or DRC = Off).
 */
void WinEDA_PcbFrame::ExChange_Track_Layer( TRACK* pt_segm, wxDC* DC )
{
    int    ii;
    TRACK* pt_track;
    int    l1, l2, nb_segm;

    if( ( pt_segm == NULL ) || ( pt_segm->Type() == TYPE_ZONE ) )
    {
        return;
    }

    l1 = Route_Layer_TOP; l2 = Route_Layer_BOTTOM;

    pt_track = Marque_Une_Piste( GetBoard(), pt_segm, &nb_segm, NULL, true );
    Trace_Une_Piste( DrawPanel, DC, pt_track, nb_segm, GR_XOR );

    /* Clear the BUSY flag and backup member. Param layer original. */
    ii = nb_segm; pt_segm = pt_track;
    for( ; ii > 0; ii--, pt_segm = (TRACK*) pt_segm->Next() )
    {
        pt_segm->SetState( BUSY, OFF );
        pt_segm->m_Param = pt_segm->GetLayer();    /* For backup. */
    }

    ii = 0; pt_segm = pt_track;
    for( ; ii < nb_segm; ii++, pt_segm = (TRACK*) pt_segm->Next() )
    {
        if( pt_segm->Type() == TYPE_VIA )
            continue;

        /* Invert layers. */
        if( pt_segm->GetLayer() == l1 )
            pt_segm->SetLayer( l2 );
        else if( pt_segm->GetLayer() == l2 )
            pt_segm->SetLayer( l1 );

        if( Drc_On && BAD_DRC==m_drc->Drc( pt_segm, GetBoard()->m_Track ) )
        {
            /* Discard changes. */
            ii = 0;
            pt_segm = pt_track;

            for( ; ii < nb_segm; ii++, pt_segm = pt_segm->Next() )
            {
                pt_segm->SetLayer( pt_segm->m_Param );
            }

            Trace_Une_Piste( DrawPanel, DC, pt_track, nb_segm, GR_OR );
            DisplayError( this, _( "Drc error, canceled" ), 10 );
            return;
        }
    }

    Trace_Une_Piste( DrawPanel, DC, pt_track, nb_segm, GR_OR | GR_SURBRILL );
    /* Control of segment end point, is it on a pad? */
    ii = 0; pt_segm = pt_track;
    for( ; ii < nb_segm; pt_segm = pt_segm->Next(), ii++ )
    {
        pt_segm->start = Locate_Pad_Connecte( GetBoard(), pt_segm, START );
        pt_segm->end   = Locate_Pad_Connecte( GetBoard(), pt_segm, END );
    }

    test_1_net_connexion( DC, pt_track->GetNet() );
    pt_track->DisplayInfo( this );
    GetScreen()->SetModify();
}


bool WinEDA_PcbFrame::Other_Layer_Route( TRACK* aTrack, wxDC* DC )
{
    unsigned    itmp;

    if( aTrack == NULL )
    {
        if( ((PCB_SCREEN*)GetScreen())->m_Active_Layer !=
            ((PCB_SCREEN*)GetScreen())->m_Route_Layer_TOP )
            ((PCB_SCREEN*)GetScreen())->m_Active_Layer =
                ((PCB_SCREEN*)GetScreen())->m_Route_Layer_TOP;
        else
            ((PCB_SCREEN*)GetScreen())->m_Active_Layer =
                ((PCB_SCREEN*)GetScreen())->m_Route_Layer_BOTTOM;

        UpdateStatusBar();
        SetToolbars();
        return true;
    }

    /* Avoid more than one via on the current location: */
    if( Locate_Via( GetBoard(), g_CurrentTrackSegment->m_End,
                    g_CurrentTrackSegment->GetLayer() ) )
        return false;

    for( TRACK* segm = g_FirstTrackSegment;  segm;  segm = segm->Next() )
    {
        if( segm->Type()==TYPE_VIA
            && g_CurrentTrackSegment->m_End==segm->m_Start )
            return false;
    }

    /* Is the current segment Ok (no DRC error) ? */
    if( Drc_On )
    {
        if( BAD_DRC==m_drc->Drc( g_CurrentTrackSegment, GetBoard()->m_Track ) )
            /* DRC error, the change layer is not made */
            return false;

        // Handle 2 segments.
        if( g_TwoSegmentTrackBuild && g_CurrentTrackSegment->Back() )
        {
            if( BAD_DRC == m_drc->Drc( g_CurrentTrackSegment->Back(),
                                       GetBoard()->m_Track ) )
                return false;
        }
    }

    /* Save current state before placing a via.
     * If the via cannot be placed this current state will be reused
     */
    itmp = g_CurrentTrackList.GetCount();
    Begin_Route( g_CurrentTrackSegment, DC );

    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

    /* create the via */
    SEGVIA* via    = new SEGVIA( GetBoard() );
    via->m_Flags   = IS_NEW;
    via->m_Shape   = g_DesignSettings.m_CurrentViaType;
    via->m_Width   = GetBoard()->GetCurrentViaSize();
    via->SetNet( g_HightLigth_NetCode );
    via->m_Start   = via->m_End = g_CurrentTrackSegment->m_End;
    int old_layer = ((PCB_SCREEN*)GetScreen())->m_Active_Layer;

    // swap the layers.
    if( ((PCB_SCREEN*)GetScreen())->m_Active_Layer !=
        ((PCB_SCREEN*)GetScreen())->m_Route_Layer_TOP )
        ((PCB_SCREEN*)GetScreen())->m_Active_Layer =
            ((PCB_SCREEN*)GetScreen())->m_Route_Layer_TOP;
    else
        ((PCB_SCREEN*)GetScreen())->m_Active_Layer =
            ((PCB_SCREEN*)GetScreen())->m_Route_Layer_BOTTOM;

    /* Adjust the via layer pair */
    switch ( via->Shape() )
    {
        case VIA_BLIND_BURIED:
            via->SetLayerPair( old_layer,
                               ((PCB_SCREEN*)GetScreen())->m_Active_Layer );
            via->SetDrillValue( GetBoard()->GetCurrentViaDrill() );
            break;

        case VIA_MICROVIA:  // from external to the near neighbor inner layer
            if ( old_layer == COPPER_LAYER_N )
                ((PCB_SCREEN*)GetScreen())->m_Active_Layer = LAYER_N_2;
            else if ( old_layer == LAYER_CMP_N )
                ((PCB_SCREEN*)GetScreen())->m_Active_Layer =
                    GetBoard()->m_BoardSettings->GetCopperLayerCount() - 2;
            else if ( old_layer == LAYER_N_2 )
                ((PCB_SCREEN*)GetScreen())->m_Active_Layer = COPPER_LAYER_N;
            else if ( old_layer == GetBoard()->m_BoardSettings->GetCopperLayerCount() - 2 )
                ((PCB_SCREEN*)GetScreen())->m_Active_Layer = LAYER_CMP_N;
            // else error
            via->SetLayerPair( old_layer,
                               ((PCB_SCREEN*)GetScreen())->m_Active_Layer );
            {
                NETINFO_ITEM* net = GetBoard()->FindNet( via->GetNet() );
                via->m_Width      = net->GetMicroViaSize();
            }
            break;

        default:
            // Usual via is from copper to component; layer pair is 0 and 0x0F.
            via->SetDrillValue( GetBoard()->GetCurrentViaDrill() );
            via->SetLayerPair( COPPER_LAYER_N, LAYER_CMP_N );
            break;
    }

    if( Drc_On && BAD_DRC==m_drc->Drc( via, GetBoard()->m_Track ) )
    {
        /* DRC fault: the Via cannot be placed here ... */
        delete via;

        ((PCB_SCREEN*)GetScreen())->m_Active_Layer = old_layer;

        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

        // delete the track(s) added in Begin_Route()
        while( g_CurrentTrackList.GetCount() > itmp )
        {
            Delete_Segment( DC, g_CurrentTrackSegment );
        }

        // use the form of SetCurItem() which does not write to the msg panel,
        // SCREEN::SetCurItem(), so the DRC error remains on screen.
        // WinEDA_PcbFrame::SetCurItem() calls DisplayInfo().
        GetScreen()->SetCurItem( g_CurrentTrackSegment );

        return false;
    }

    TRACK*  lastNonVia = g_CurrentTrackSegment;

    /* A new via was created. It was Ok.
     */
    g_CurrentTrackList.PushBack( via );

    /* The via is now in linked list and we need a new track segment
     * after the via, starting at via location.
     * it will become the new current segment (from via to the mouse cursor)
     */

    TRACK* track = lastNonVia->Copy();

    /* the above creates a new segment from the last entered segment, with the
     * current width, flags, netcode, etc... values.
     * layer, start and end point are not correct,
     * and will be modified next
     */

    // set the layer to the new value
    track->SetLayer( ((PCB_SCREEN*)GetScreen())->m_Active_Layer );

    /* the start point is the via position and the end point is the cursor
     * which also is on the via (will change when moving mouse)
     */
    track->m_Start = track->m_End = via->m_Start;

    g_CurrentTrackList.PushBack( track );

    if( g_TwoSegmentTrackBuild )
    {
        // Create a second segment (we must have 2 track segments to adjust)
        g_CurrentTrackList.PushBack( g_CurrentTrackSegment->Copy() );
    }

    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
    via->DisplayInfo( this );

    UpdateStatusBar();

    SetToolbars();

    return true;
}


/* Displays:
 * The status of the net on top of the screen segment advanced by mouse.
 * PCB status or bottom of screen if no segment peak.
 */
void WinEDA_PcbFrame::Affiche_Status_Net( wxDC* DC )
{
    TRACK* pt_segm;
    int    masquelayer =
        g_TabOneLayerMask[((PCB_SCREEN*)GetScreen())->m_Active_Layer];

    pt_segm = Locate_Pistes( GetBoard()->m_Track, masquelayer,
                             CURSEUR_OFF_GRILLE );
    if( pt_segm == NULL )
        GetBoard()->DisplayInfo( this );
    else
        test_1_net_connexion( DC, pt_segm->GetNet() );
}


/* Draw ratsnest.
 *
 * The net edge pad with mouse or module locates the mouse.
 * Delete if the ratsnest if no module or pad is selected.
 */
void WinEDA_PcbFrame::Show_1_Ratsnest( EDA_BaseStruct* item, wxDC* DC )
{
    D_PAD*   pt_pad = NULL;
    MODULE*  Module = NULL;

    if( g_Show_Ratsnest )
        return;

    if( ( GetBoard()->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK ) == 0 )
        Compile_Ratsnest( DC, TRUE );

    if( item )
    {
        if( item->Type() == TYPE_PAD )
        {
            pt_pad = (D_PAD*) item;
            Module = (MODULE*) pt_pad->GetParent();
        }

        if( pt_pad ) /* Displaying the ratsnest of the corresponding net. */
        {
            pt_pad->DisplayInfo( this );
            for( unsigned ii = 0; ii < GetBoard()->GetRatsnestsCount(); ii++ )
            {
                RATSNEST_ITEM* net = &GetBoard()->m_FullRatsnest[ii];
                if( net->GetNet() == pt_pad->GetNet() )
                {
                    if( ( net->m_Status & CH_VISIBLE ) != 0 )
                        continue;
                    net->m_Status |= CH_VISIBLE;
                    if( ( net->m_Status & CH_ACTIF ) == 0 )
                        continue;

                    net->Draw( DrawPanel, DC, GR_XOR, wxPoint( 0, 0 ) );
                }
            }
        }
        else
        {
            if( item->Type() == TYPE_TEXTE_MODULE )
            {
                if( item->GetParent()
                    && ( item->GetParent()->Type() == TYPE_MODULE ) )
                    Module = (MODULE*) item->GetParent();
            }
            else if( item->Type() == TYPE_MODULE )
            {
                Module = (MODULE*) item;
            }

            if( Module )
            {
                Module->DisplayInfo( this );
                pt_pad = Module->m_Pads;
                for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Next() )
                {
                    for( unsigned ii = 0; ii < GetBoard()->GetRatsnestsCount();
                         ii++ )
                    {
                        RATSNEST_ITEM* net = &GetBoard()->m_FullRatsnest[ii];
                        if( ( net->m_PadStart == pt_pad )
                            || ( net->m_PadEnd == pt_pad ) )
                        {
                            if( net->m_Status & CH_VISIBLE )
                                continue;

                            net->m_Status |= CH_VISIBLE;
                            if( (net->m_Status & CH_ACTIF) == 0 )
                                continue;

                            net->Draw( DrawPanel, DC, GR_XOR, wxPoint( 0, 0 ) );
                        }
                    }
                }

                pt_pad = NULL;
            }
        }
    }

    /* Erase if no pad or module has been selected. */
    if( ( pt_pad == NULL ) && ( Module == NULL ) )
    {
        DrawGeneralRatsnest( DC );

        for( unsigned ii = 0; ii < GetBoard()->GetRatsnestsCount(); ii++ )
            GetBoard()->m_FullRatsnest[ii].m_Status &= ~CH_VISIBLE;
    }
}


/* High light the unconnected pads
 */
void WinEDA_PcbFrame::Affiche_PadsNoConnect( wxDC* DC )
{
    for( unsigned ii = 0; ii < GetBoard()->GetRatsnestsCount(); ii++ )
    {
        RATSNEST_ITEM* net = &GetBoard()->m_FullRatsnest[ii];
        if( (net->m_Status & CH_ACTIF) == 0 )
            continue;

        net->m_PadStart->Draw( DrawPanel, DC, GR_OR | GR_SURBRILL );
        net->m_PadEnd->Draw( DrawPanel, DC, GR_OR | GR_SURBRILL );
    }
}
