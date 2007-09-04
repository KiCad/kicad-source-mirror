/*******************************/
/* Edition des pistes			*/
/* Routines de trace de pistes */
/*******************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

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
    CHEVELU* pt_chevelu;

    if( (m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 )
    {
        if( g_Show_Ratsnest )
            Compile_Ratsnest( DC, TRUE );
        return;
    }

    DrawGeneralRatsnest( DC, 0 ); /* effacement eventuel du chevelu affiche */

    pt_chevelu = m_Pcb->m_Ratsnest;
    if( pt_chevelu == NULL )
        return;

    if( g_Show_Ratsnest )
    {
        for( ii = m_Pcb->GetNumRatsnests(); ii > 0; pt_chevelu++, ii-- )
        {
            pt_chevelu->status |= CH_VISIBLE;
        }

        DrawGeneralRatsnest( DC, 0 );
    }
    else
    {
        for( ii = m_Pcb->GetNumRatsnests(); ii > 0; pt_chevelu++, ii-- )
        {
            pt_chevelu->status &= ~CH_VISIBLE;
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

    if( (pt_segm == NULL ) || ( pt_segm->Type() == TYPEZONE ) )
    {
        return;
    }

    l1 = Route_Layer_TOP; l2 = Route_Layer_BOTTOM;

    pt_track = Marque_Une_Piste( this, DC, pt_segm, &nb_segm, GR_XOR );

    /* effacement du flag BUSY et sauvegarde en membre .param de la couche
     *  initiale */
    ii = nb_segm; pt_segm = pt_track;
    for( ; ii > 0; ii--, pt_segm = (TRACK*) pt_segm->Pnext )
    {
        pt_segm->SetState( BUSY, OFF );
        pt_segm->m_Param = pt_segm->GetLayer();    /* pour sauvegarde */
    }

    ii = 0; pt_segm = pt_track;
    for( ; ii < nb_segm; ii++, pt_segm = (TRACK*) pt_segm->Pnext )
    {
        if( pt_segm->Type() == TYPEVIA )
            continue;

        /* inversion des couches */
        if( pt_segm->GetLayer() == l1 )
            pt_segm->SetLayer( l2 );
        else if( pt_segm->GetLayer() == l2 )
            pt_segm->SetLayer( l1 );

        if( (Drc_On) && ( Drc( this, DC, pt_segm, m_Pcb->m_Track, 1 ) == BAD_DRC ) )
        {       /* Annulation du changement */
            ii = 0; pt_segm = pt_track;
            for( ; ii < nb_segm; ii++, pt_segm = (TRACK*) pt_segm->Pnext )
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
    for( ; ii < nb_segm; pt_segm = (TRACK*) pt_segm->Pnext, ii++ )
    {
        pt_segm->start = Locate_Pad_Connecte( m_Pcb, pt_segm, START );
        pt_segm->end   = Locate_Pad_Connecte( m_Pcb, pt_segm, END );
    }

    test_1_net_connexion( DC, pt_track->m_NetCode );
    pt_track->Display_Infos( this );
    GetScreen()->SetModify();
}


/****************************************************************/
void WinEDA_PcbFrame::Other_Layer_Route( TRACK* track, wxDC* DC )
/****************************************************************/

/*
 *  if no track in progress :
 *		swap the active layer between m_Route_Layer_TOP and m_Route_Layer_BOTTOM
 *  if a track is in progress :
 *		put (if possible, i.e. if no DRC problem) a Via on the end of the current
 *		track, swap the current active layer and start a new trac segment on the new layer
 */
{
    TRACK*  pt_segm;
    SEGVIA* Via;
    int     ii;
    int     itmp;

    if( track == NULL )
    {
        if( GetScreen()->m_Active_Layer != GetScreen()->m_Route_Layer_TOP )
            GetScreen()->m_Active_Layer = GetScreen()->m_Route_Layer_TOP;
        else
            GetScreen()->m_Active_Layer = GetScreen()->m_Route_Layer_BOTTOM;
        Affiche_Status_Box();
        SetToolbars();
        return;
    }

    /* Avoid more than one via on the current location: */
    if( Locate_Via( m_Pcb, g_CurrentTrackSegment->m_End, g_CurrentTrackSegment->GetLayer() ) )
        return;
    pt_segm = g_FirstTrackSegment;
    for( ii = 0; ii < g_TrackSegmentCount - 1; ii++, pt_segm = (TRACK*) pt_segm->Pnext )
    {
        if( (pt_segm->Type() == TYPEVIA)
           && (g_CurrentTrackSegment->m_End == pt_segm->m_Start) )
            return;
    }

    /* Is the current segment Ok (no DRC error) */
    if( Drc_On )
        if( Drc( this, DC, g_CurrentTrackSegment, m_Pcb->m_Track, 1 ) == BAD_DRC )
			/* DRC error, the change layer is not made */
            return;

    /* Saving current state before placing a via.
		If the via canot be placed this current state will be reused */
    itmp = g_TrackSegmentCount;
    Begin_Route( g_CurrentTrackSegment, DC );

    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

	/* create the via */
    Via = new SEGVIA( m_Pcb );
    Via->m_Flags   = IS_NEW;
    Via->m_Width   = g_DesignSettings.m_CurrentViaSize;
    Via->m_Shape   = g_DesignSettings.m_CurrentViaType;
    Via->m_NetCode = g_HightLigth_NetCode;
    Via->m_Start   = Via->m_End = g_CurrentTrackSegment->m_End;

    int old_layer = GetScreen()->m_Active_Layer );

    //swap the layers.
    if( GetScreen()->m_Active_Layer != GetScreen()->m_Route_Layer_TOP )
        GetScreen()->m_Active_Layer = GetScreen()->m_Route_Layer_TOP;
    else
        GetScreen()->m_Active_Layer = GetScreen()->m_Route_Layer_BOTTOM;

	/* Adjust the via layer pair */
    if( (Via->m_Shape & 15) == VIA_ENTERREE )
    {
        Via->SetLayer( old_layer | GetScreen()->m_Active_Layer << 4 );
    }
    else if( (Via->m_Shape & 15) == VIA_BORGNE )    //blind via
    {                                               // A revoir! ( la via devrait deboucher sur 1 cote )
        Via->SetLayer( old_layer | GetScreen()->m_Active_Layer << 4 );
    }
    else
        Via->SetLayer( 0x0F );	// Usual via is from copper to component; layer pair is 0 and 0x0F

    if( Drc_On &&( Drc( this, DC, Via, m_Pcb->m_Track, 1 ) == BAD_DRC ) )
    { 
        /* DRC fault: the Via cannot be placed here ... */
        delete Via;
        GetScreen()->m_Active_Layer = old_layer;
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        return;
    }

    /* A new via was created. It was Ok.
	Put it in linked list, after the g_CurrentTrackSegment */
    Via->Pback = g_CurrentTrackSegment;
    g_CurrentTrackSegment->Pnext = Via;
    g_TrackSegmentCount++;

	/* The g_CurrentTrackSegment is now in linked list and we need a new track segment 
	after the via, starting at via location.
	it will become the new curren segment (from via to the mouse cursor)
	*/
	g_CurrentTrackSegment = g_CurrentTrackSegment->Copy();	/* create a new segment
	from the last entered segment, with the current width, flags, netcode, etc... values
	layer, start and end point are not correct, and will be modified next */
    
    g_CurrentTrackSegment->SetLayer( GetScreen()->m_Active_Layer );	// set the layer to the new value
    
	/* the start point is the via position,
	and the end point is the cursor which also is on the via (will change when moving mouse)
	*/
    g_CurrentTrackSegment->m_Start = g_CurrentTrackSegment->m_End = Via->m_Start;
    
    g_TrackSegmentCount++;
    
    g_CurrentTrackSegment->Pback = Via;
    
    Via->Pnext = g_CurrentTrackSegment;
    
    if( g_TwoSegmentTrackBuild )
    {   
        // Create a second segment (we must have 2 track segments to adjust)
        TRACK* track = g_CurrentTrackSegment;
        
        g_CurrentTrackSegment = track->Copy();
        
        g_TrackSegmentCount++;
        g_CurrentTrackSegment->Pback = track;
        track->Pnext = g_CurrentTrackSegment;
    }

    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
    Via->Display_Infos( this );

    Affiche_Status_Box();
    SetToolbars();
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
    int    masquelayer = g_TabOneLayerMask[GetScreen()->m_Active_Layer];

    pt_segm = Locate_Pistes( m_Pcb->m_Track, masquelayer, CURSEUR_OFF_GRILLE );
    if( pt_segm == NULL )
        m_Pcb->Display_Infos( this );
    else
        test_1_net_connexion( DC, pt_segm->m_NetCode );
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
    CHEVELU* pt_chevelu;
    D_PAD*   pt_pad = NULL;
    MODULE*  Module = NULL;

    if( g_Show_Ratsnest )
        return;                     // Deja Affich�

    if( (m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 )
    {
        Compile_Ratsnest( DC, TRUE );
    }

    if( item )
    {
        if( item->Type() == TYPEPAD )
        {
            pt_pad = (D_PAD*) item;
            Module = (MODULE*) pt_pad->m_Parent;
        }
    
        if( pt_pad ) /* Affichage du chevelu du net correspondant */
        {
            pt_pad->Display_Infos( this );
            pt_chevelu = (CHEVELU*) m_Pcb->m_Ratsnest;
            for( ii = m_Pcb->GetNumRatsnests(); ii > 0; pt_chevelu++, ii-- )
            {
                if( pt_chevelu->m_NetCode == pt_pad->m_NetCode )
                {
                    if( (pt_chevelu->status & CH_VISIBLE) != 0 )
                        continue;
                    pt_chevelu->status |= CH_VISIBLE;
                    if( (pt_chevelu->status & CH_ACTIF) == 0 )
                        continue;
    
                    GRSetDrawMode( DC, GR_XOR );
                    GRLine( &DrawPanel->m_ClipBox, DC, pt_chevelu->pad_start->m_Pos.x,
                            pt_chevelu->pad_start->m_Pos.y,
                            pt_chevelu->pad_end->m_Pos.x,
                            pt_chevelu->pad_end->m_Pos.y,
                            0,
                            g_DesignSettings.m_RatsnestColor );
                }
            }
        }
        else
        {
            if( item->Type() == TYPETEXTEMODULE )
            {
                if( item->m_Parent && (item->m_Parent->Type()  == TYPEMODULE) )
                    Module = (MODULE*) item->m_Parent;
            }
            
            else if( item->Type() == TYPEMODULE )
            {
                Module = (MODULE*) item;
            }
    
            if( Module )
            {
                Module->Display_Infos( this );
                pt_pad = Module->m_Pads;
                for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
                {
                    pt_chevelu = (CHEVELU*) m_Pcb->m_Ratsnest;
                    for( ii = m_Pcb->GetNumRatsnests(); ii > 0; pt_chevelu++, ii-- )
                    {
                        if( (pt_chevelu->pad_start == pt_pad)
                           || (pt_chevelu->pad_end == pt_pad) )
                        {
                            if( pt_chevelu->status & CH_VISIBLE )
                                continue;
    
                            pt_chevelu->status |= CH_VISIBLE;
                            if( (pt_chevelu->status & CH_ACTIF) == 0 )
                                continue;
    
                            GRSetDrawMode( DC, GR_XOR );
                            GRLine( &DrawPanel->m_ClipBox, DC, pt_chevelu->pad_start->m_Pos.x,
                                    pt_chevelu->pad_start->m_Pos.y,
                                    pt_chevelu->pad_end->m_Pos.x,
                                    pt_chevelu->pad_end->m_Pos.y,
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
        MsgPanel->EraseMsgBox();
        DrawGeneralRatsnest( DC );
        pt_chevelu = (CHEVELU*) m_Pcb->m_Ratsnest;
        for( ii = m_Pcb->GetNumRatsnests(); (ii > 0) && pt_chevelu; pt_chevelu++, ii-- )
            pt_chevelu->status &= ~CH_VISIBLE;
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
    CHEVELU* pt_chevelu;
    D_PAD*   pt_pad;

    pt_chevelu = (CHEVELU*) m_Pcb->m_Ratsnest;
    for( ii = m_Pcb->GetNumRatsnests(); ii > 0; pt_chevelu++, ii-- )
    {
        if( (pt_chevelu->status & CH_ACTIF) == 0 )
            continue;

        pt_pad = pt_chevelu->pad_start;

        if( pt_pad )
            pt_pad->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR | GR_SURBRILL );

        pt_pad = pt_chevelu->pad_end;
        if( pt_pad )
            pt_pad->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR | GR_SURBRILL );
    }
}
