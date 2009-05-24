/*******************************/
/* Edition des pistes			*/
/* Routines de trace de pistes */
/*******************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "protos.h"


/* Routines Locales */

/* variables locales */

/***********************************************/
void WinEDA_PcbFrame::DisplayTrackSettings()
/***********************************************/

/* Display the current track width and via diameter
 */
{
    wxString msg;
    wxString buftrc, bufvia;

    valeur_param( g_DesignSettings.m_CurrentTrackWidth, buftrc );
    valeur_param( g_DesignSettings.m_CurrentViaSize, bufvia );
    msg.Printf( _( "Track Width: %s   Vias Size : %s" ),
               buftrc.GetData(), bufvia.GetData() );
    Affiche_Message( msg );
    m_SelTrackWidthBox_Changed = TRUE;
    m_SelViaSizeBox_Changed    = TRUE;
}


/***********************************************/
void WinEDA_PcbFrame::Ratsnest_On_Off( wxDC* DC )
/***********************************************/

/* Affiche ou efface le chevelu selon l'etat du bouton d'appel */
{
    int      ii;
    RATSNEST_ITEM* pt_chevelu;

    if( (GetBoard()->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK) == 0 )
    {
        if( g_Show_Ratsnest )
            Compile_Ratsnest( DC, TRUE );
        return;
    }

    DrawGeneralRatsnest( DC, 0 ); /* effacement eventuel du chevelu affiche */

    pt_chevelu = GetBoard()->m_Ratsnest;
    if( pt_chevelu == NULL )
        return;

    if( g_Show_Ratsnest )
    {
        for( ii = GetBoard()->GetNumRatsnests(); ii > 0; pt_chevelu++, ii-- )
        {
            pt_chevelu->m_Status |= CH_VISIBLE;
        }

        DrawGeneralRatsnest( DC, 0 );
    }
    else
    {
        for( ii = GetBoard()->GetNumRatsnests(); ii > 0; pt_chevelu++, ii-- )
        {
            pt_chevelu->m_Status &= ~CH_VISIBLE;
        }
    }
}


/*************************************************************************/
void WinEDA_PcbFrame::ExChange_Track_Layer( TRACK* pt_segm, wxDC* DC )
/*************************************************************************/

/*
 *  change de couche la piste pointee par la souris :
 *  la piste doit etre sur une des couches de travail,
 *  elle est mise sur l'autre couche de travail, si cela est possible
 *  (ou si DRC = Off ).
 */
{
    int    ii;
    TRACK* pt_track;
    int    l1, l2, nb_segm;

    if( (pt_segm == NULL ) || ( pt_segm->Type() == TYPE_ZONE ) )
    {
        return;
    }

    l1 = Route_Layer_TOP; l2 = Route_Layer_BOTTOM;

    pt_track = Marque_Une_Piste( this, DC, pt_segm, &nb_segm, GR_XOR );

    /* effacement du flag BUSY et sauvegarde en membre .param de la couche
     *  initiale */
    ii = nb_segm; pt_segm = pt_track;
    for( ; ii > 0; ii--, pt_segm = (TRACK*) pt_segm->Next() )
    {
        pt_segm->SetState( BUSY, OFF );
        pt_segm->m_Param = pt_segm->GetLayer();    /* pour sauvegarde */
    }

    ii = 0; pt_segm = pt_track;
    for( ; ii < nb_segm; ii++, pt_segm = (TRACK*) pt_segm->Next() )
    {
        if( pt_segm->Type() == TYPE_VIA )
            continue;

        /* inversion des couches */
        if( pt_segm->GetLayer() == l1 )
            pt_segm->SetLayer( l2 );
        else if( pt_segm->GetLayer() == l2 )
            pt_segm->SetLayer( l1 );

        if( Drc_On && BAD_DRC==m_drc->Drc( pt_segm, GetBoard()->m_Track ) )
        {
            /* Annulation du changement */
            ii = 0; pt_segm = pt_track;
            for( ; ii < nb_segm; ii++, pt_segm = pt_segm->Next() )
            {
                pt_segm->SetLayer( pt_segm->m_Param );
            }

            Trace_Une_Piste( DrawPanel, DC, pt_track, nb_segm, GR_OR );
            DisplayError( this, _( "Drc error, cancelled" ), 10 );
            return;
        }
    }

    Trace_Une_Piste( DrawPanel, DC, pt_track, nb_segm, GR_OR | GR_SURBRILL );
    /* controle des extremites de segments: sont-ils sur un pad */
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



/****************************************************************/
bool WinEDA_PcbFrame::Other_Layer_Route( TRACK* aTrack, wxDC* DC )
/****************************************************************/
{
    unsigned    itmp;

    if( aTrack == NULL )
    {
        if( ((PCB_SCREEN*)GetScreen())->m_Active_Layer != ((PCB_SCREEN*)GetScreen())->m_Route_Layer_TOP )
            ((PCB_SCREEN*)GetScreen())->m_Active_Layer = ((PCB_SCREEN*)GetScreen())->m_Route_Layer_TOP;
        else
            ((PCB_SCREEN*)GetScreen())->m_Active_Layer = ((PCB_SCREEN*)GetScreen())->m_Route_Layer_BOTTOM;

        UpdateStatusBar();
        SetToolbars();
        return true;
    }

    /* Avoid more than one via on the current location: */
    if( Locate_Via( GetBoard(), g_CurrentTrackSegment->m_End, g_CurrentTrackSegment->GetLayer() ) )
        return false;

    for( TRACK* segm = g_FirstTrackSegment;  segm;  segm = segm->Next() )
    {
        if( segm->Type()==TYPE_VIA  &&  g_CurrentTrackSegment->m_End==segm->m_Start )
            return false;
    }

    /* Is the current segment Ok (no DRC error) ? */
    if( Drc_On )
    {
        if( BAD_DRC==m_drc->Drc( g_CurrentTrackSegment, GetBoard()->m_Track ) )
            /* DRC error, the change layer is not made */
            return false;

        if( g_TwoSegmentTrackBuild && g_CurrentTrackSegment->Back() )    // We must handle 2 segments
        {
            if( BAD_DRC == m_drc->Drc( g_CurrentTrackSegment->Back(), GetBoard()->m_Track ) )
                return false;
        }
    }

    /* Save current state before placing a via.
     * If the via canot be placed this current state will be reused
     */
    itmp = g_CurrentTrackList.GetCount();
    Begin_Route( g_CurrentTrackSegment, DC );

    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

    /* create the via */
    SEGVIA* via    = new SEGVIA( GetBoard() );
    via->m_Flags   = IS_NEW;
    via->m_Shape   = g_DesignSettings.m_CurrentViaType;
    via->m_Width   = g_DesignSettings.m_CurrentViaSize;
    via->SetNet( g_HightLigth_NetCode );
    via->m_Start   = via->m_End = g_CurrentTrackSegment->m_End;
    int old_layer = ((PCB_SCREEN*)GetScreen())->m_Active_Layer;

    //swap the layers.
    if( ((PCB_SCREEN*)GetScreen())->m_Active_Layer != ((PCB_SCREEN*)GetScreen())->m_Route_Layer_TOP )
        ((PCB_SCREEN*)GetScreen())->m_Active_Layer = ((PCB_SCREEN*)GetScreen())->m_Route_Layer_TOP;
    else
        ((PCB_SCREEN*)GetScreen())->m_Active_Layer = ((PCB_SCREEN*)GetScreen())->m_Route_Layer_BOTTOM;

    /* Adjust the via layer pair */
    switch ( via->Shape() )
    {
        case VIA_BLIND_BURIED:
            via->SetLayerPair( old_layer, ((PCB_SCREEN*)GetScreen())->m_Active_Layer );
            break;

        case VIA_MICROVIA:	// from external to the near neghbour inner layer
            if ( old_layer == COPPER_LAYER_N )
                ((PCB_SCREEN*)GetScreen())->m_Active_Layer = LAYER_N_2;
            else if ( old_layer == LAYER_CMP_N )
                ((PCB_SCREEN*)GetScreen())->m_Active_Layer = GetBoard()->m_BoardSettings->m_CopperLayerCount - 2;
            else if ( old_layer == LAYER_N_2 )
                ((PCB_SCREEN*)GetScreen())->m_Active_Layer = COPPER_LAYER_N;
            else if ( old_layer == GetBoard()->m_BoardSettings->m_CopperLayerCount - 2 )
                ((PCB_SCREEN*)GetScreen())->m_Active_Layer = LAYER_CMP_N;
            // else error
            via->SetLayerPair( old_layer, ((PCB_SCREEN*)GetScreen())->m_Active_Layer );
            via->m_Width   = g_DesignSettings.m_CurrentMicroViaSize;
            break;

        default:
            // Usual via is from copper to component; layer pair is 0 and 0x0F.
            via->SetLayerPair( COPPER_LAYER_N, LAYER_CMP_N );
            break;
    }

    if( Drc_On &&  BAD_DRC==m_drc->Drc( via, GetBoard()->m_Track ) )
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

    track->SetLayer( ((PCB_SCREEN*)GetScreen())->m_Active_Layer ); // set the layer to the new value

    /* the start point is the via position,
     * and the end point is the cursor which also is on the via (will change when moving mouse)
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


/*************************************************/
void WinEDA_PcbFrame::Affiche_Status_Net( wxDC* DC )
/*************************************************/

/* Affiche:
 *  le status du net en haut d'ecran du segment pointe par la souris
 *  ou le status PCB en bas d'ecran si pas de segment pointe
 */
{
    TRACK* pt_segm;
    int    masquelayer = g_TabOneLayerMask[((PCB_SCREEN*)GetScreen())->m_Active_Layer];

    pt_segm = Locate_Pistes( GetBoard()->m_Track, masquelayer, CURSEUR_OFF_GRILLE );
    if( pt_segm == NULL )
        GetBoard()->DisplayInfo( this );
    else
        test_1_net_connexion( DC, pt_segm->GetNet() );
}


/**********************************************************************/
void WinEDA_PcbFrame::Show_1_Ratsnest( EDA_BaseStruct* item, wxDC* DC )
/**********************************************************************/

/* Affiche le ratsnest relatif
 *  au net du pad pointe par la souris
 *  ou au module localise par la souris
 *  Efface le chevelu affiche si aucun module ou pad n'est selectionne
 */
{
    int      ii;
    RATSNEST_ITEM* pt_chevelu;
    D_PAD*   pt_pad = NULL;
    MODULE*  Module = NULL;

    if( g_Show_Ratsnest )
        return; // Deja Affich�

    if( (GetBoard()->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK) == 0 )
    {
        Compile_Ratsnest( DC, TRUE );
    }

    if( item )
    {
        if( item->Type() == TYPE_PAD )
        {
            pt_pad = (D_PAD*) item;
            Module = (MODULE*) pt_pad->GetParent();
        }

        if( pt_pad ) /* Affichage du chevelu du net correspondant */
        {
            pt_pad->DisplayInfo( this );
            pt_chevelu = (RATSNEST_ITEM*) GetBoard()->m_Ratsnest;
            for( ii = GetBoard()->GetNumRatsnests(); ii > 0; pt_chevelu++, ii-- )
            {
                if( pt_chevelu->GetNet() == pt_pad->GetNet() )
                {
                    if( (pt_chevelu->m_Status & CH_VISIBLE) != 0 )
                        continue;
                    pt_chevelu->m_Status |= CH_VISIBLE;
                    if( (pt_chevelu->m_Status & CH_ACTIF) == 0 )
                        continue;

                    GRSetDrawMode( DC, GR_XOR );
                    GRLine( &DrawPanel->m_ClipBox, DC, pt_chevelu->m_PadStart->m_Pos.x,
                            pt_chevelu->m_PadStart->m_Pos.y,
                            pt_chevelu->m_PadEnd->m_Pos.x,
                            pt_chevelu->m_PadEnd->m_Pos.y,
                            0,
                            g_DesignSettings.m_RatsnestColor );
                }
            }
        }
        else
        {
            if( item->Type() == TYPE_TEXTE_MODULE )
            {
                if( item->GetParent() && (item->GetParent()->Type()  == TYPE_MODULE) )
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
                    pt_chevelu = (RATSNEST_ITEM*) GetBoard()->m_Ratsnest;
                    for( ii = GetBoard()->GetNumRatsnests(); ii > 0; pt_chevelu++, ii-- )
                    {
                        if( (pt_chevelu->m_PadStart == pt_pad)
                           || (pt_chevelu->m_PadEnd == pt_pad) )
                        {
                            if( pt_chevelu->m_Status & CH_VISIBLE )
                                continue;

                            pt_chevelu->m_Status |= CH_VISIBLE;
                            if( (pt_chevelu->m_Status & CH_ACTIF) == 0 )
                                continue;

                            GRSetDrawMode( DC, GR_XOR );
                            GRLine( &DrawPanel->m_ClipBox, DC, pt_chevelu->m_PadStart->m_Pos.x,
                                    pt_chevelu->m_PadStart->m_Pos.y,
                                    pt_chevelu->m_PadEnd->m_Pos.x,
                                    pt_chevelu->m_PadEnd->m_Pos.y,
                                    0,
                                    g_DesignSettings.m_RatsnestColor );
                        }
                    }
                }

                pt_pad = NULL;
            }
        }
    }

    /* Effacement complet des selections
     *  si aucun pad ou module n'a ete localise */
    if( (pt_pad == NULL) && (Module == NULL) )
    {
        DrawGeneralRatsnest( DC );
        pt_chevelu = (RATSNEST_ITEM*) GetBoard()->m_Ratsnest;

        for( ii = GetBoard()->GetNumRatsnests(); (ii > 0) && pt_chevelu; pt_chevelu++, ii-- )
            pt_chevelu->m_Status &= ~CH_VISIBLE;
    }
}


/*****************************************************/
void WinEDA_PcbFrame::Affiche_PadsNoConnect( wxDC* DC )
/*****************************************************/

/* Met en surbrillance les pads non encore connectes ( correspondants aux
 *  chevelus actifs
 */
{
    int      ii;
    RATSNEST_ITEM* pt_chevelu;
    D_PAD*   pt_pad;

    pt_chevelu = (RATSNEST_ITEM*) GetBoard()->m_Ratsnest;
    for( ii = GetBoard()->GetNumRatsnests(); ii > 0; pt_chevelu++, ii-- )
    {
        if( (pt_chevelu->m_Status & CH_ACTIF) == 0 )
            continue;

        pt_pad = pt_chevelu->m_PadStart;

        if( pt_pad )
            pt_pad->Draw( DrawPanel, DC, GR_OR | GR_SURBRILL );

        pt_pad = pt_chevelu->m_PadEnd;
        if( pt_pad )
            pt_pad->Draw( DrawPanel, DC, GR_OR | GR_SURBRILL );
    }
}
