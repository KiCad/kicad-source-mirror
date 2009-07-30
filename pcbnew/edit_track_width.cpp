/***************************************************************/
/* Edition des pistes: Routines de modification de dimensions: */
/* Modif de largeurs de segment, piste, net , zone et diam Via */
/***************************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "protos.h"

/* Routines Locales */

/*********************************************************************/
int WinEDA_PcbFrame::Edit_TrackSegm_Width( wxDC* DC, TRACK* pt_segm )
/*********************************************************************/

/* Routine to modify one track segment width or one via diameter.
 *  Basic routine used by other routines when editing tracks or vias
 */
{
    int errdrc = OK_DRC;
    int old_w, consigne;

    DrawPanel->CursorOff( DC );                     // Erase cursor shape
    pt_segm->Draw( DrawPanel, DC, GR_XOR );         // Erase old track shape

    /* Test DRC and width change */
    old_w    = pt_segm->m_Width;
    consigne = pt_segm->m_Width = g_DesignSettings.m_CurrentTrackWidth;
    if( pt_segm->Type() == TYPE_VIA )
    {
        consigne = pt_segm->m_Width = g_DesignSettings.m_CurrentViaSize;
        if ( pt_segm->m_Shape == VIA_MICROVIA )
            consigne = pt_segm->m_Width = g_DesignSettings.m_CurrentMicroViaSize;
    }

    if( old_w < consigne ) /* DRC utile puisque augm de dimension */
    {
        if( Drc_On )
            errdrc = m_drc->Drc( pt_segm, GetBoard()->m_Track );
        if( errdrc == BAD_DRC )
            pt_segm->m_Width = old_w;
        else
            GetScreen()->SetModify();
    }
    else
        GetScreen()->SetModify();                   /* Correction systematiquement faite si reduction */

    pt_segm->Draw( DrawPanel, DC, GR_OR );          // Display new track shape
    DrawPanel->CursorOn( DC );                      // Display cursor shape
    return errdrc;
}


/*****************************************************************/
void WinEDA_PcbFrame::Edit_Track_Width( wxDC* DC, TRACK* pt_segm )
/*****************************************************************/
{
    int    ii;
    TRACK* pt_track;
    int    errdrc;
    int    nb_segm, nb_segm_modifies = 0, nb_segm_non_modifies = 0;

    if( pt_segm == NULL )
        return;

    pt_track = Marque_Une_Piste( this, DC, pt_segm, &nb_segm, 0 );
    for( ii = 0; ii < nb_segm; ii++, pt_track = pt_track->Next() )
    {
        pt_track->SetState( BUSY, OFF );
        errdrc = Edit_TrackSegm_Width( DC, pt_track );
        if( errdrc == BAD_DRC )
            nb_segm_non_modifies++;
        else
            nb_segm_modifies++;
    }
}


/***********************************************************/
void WinEDA_PcbFrame::Edit_Net_Width( wxDC* DC, int Netcode )
/***********************************************************/
{
    TRACK* pt_segm;
    int    errdrc;
    int    nb_segm_modifies     = 0;
    int    nb_segm_non_modifies = 0;

    if( Netcode <= 0 )
        return;

    if( !IsOK( this, _( "Change track width (entire NET) ?" ) ) )
        return;

    /* balayage des segments */
    for( pt_segm = GetBoard()->m_Track; pt_segm != NULL; pt_segm = pt_segm->Next() )
    {
        if( Netcode != pt_segm->GetNet() ) /* mauvaise piste */
            continue;
        /* piste d'un net trouvee */
        errdrc = Edit_TrackSegm_Width( DC, pt_segm );
        if( errdrc == BAD_DRC )
            nb_segm_non_modifies++;
        else
            nb_segm_modifies++;
    }
}


/*************************************************************************/
bool WinEDA_PcbFrame::Resize_Pistes_Vias( wxDC* DC, bool Track, bool Via )
/*************************************************************************/

/* remet a jour la largeur des pistes et/ou le diametre des vias
 *  Si piste == 0 , pas de cht sur les pistes
 *  Si via == 0 , pas de cht sur les vias
 */
{
    TRACK* pt_segm;
    int    errdrc;
    int    nb_segm_modifies     = 0;
    int    nb_segm_non_modifies = 0;

    if( Track && Via )
    {
        if( !IsOK( this, _( "Edit All Tracks and Vias Sizes" ) ) )
            return FALSE;
    }
    else if( Via )
    {
        if( !IsOK( this, _( "Edit All Via Sizes" ) ) )
            return FALSE;
    }
    else if( Track )
    {
        if( !IsOK( this, _( "Edit All Track Sizes" ) ) )
            return FALSE;
    }

    pt_segm = GetBoard()->m_Track;
    for( ; pt_segm != NULL; pt_segm = pt_segm->Next() )
    {
        if( pt_segm->Type() == TYPE_VIA ) /* mise a jour du diametre de la via */
        {
            if( Via )
            {
                errdrc = Edit_TrackSegm_Width( DC, pt_segm );
                if( errdrc == BAD_DRC )
                    nb_segm_non_modifies++;
                else
                    nb_segm_modifies++;
            }
        }
        else    /* mise a jour de la largeur du segment */
        {
            if( Track )
            {
                errdrc = Edit_TrackSegm_Width( DC, pt_segm );
                if( errdrc == BAD_DRC )
                    nb_segm_non_modifies++;
                else
                    nb_segm_modifies++;
            }
        }
    }

    if( nb_segm_modifies  )
        return TRUE;
    return FALSE;
}
