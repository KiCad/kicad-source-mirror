	/***************************************************************/
	/* Edition des pistes: Routines de modification de dimensions: */
	/* Modif de largeurs de segment, piste, net , zone et diam Via */
	/***************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "protos.h"

/* Routines Locales */

/*********************************************************************/
int WinEDA_PcbFrame::Edit_TrackSegm_Width(wxDC * DC, TRACK * pt_segm)
/*********************************************************************/
/* Routine de modification de la largeur d'un segment ( ou via) de piste
   Routine de base utilisee par les autres routines
*/
{
int errdrc = OK_DRC;
int old_w, consigne ;

	pt_segm->Draw(DrawPanel, DC, GR_XOR) ; // effacement a l'ecran

	/* Test DRC et mise a la largeur */
	old_w = pt_segm->m_Width;
	consigne = pt_segm->m_Width = g_DesignSettings.m_CurrentTrackWidth;
	if( pt_segm->m_StructType == TYPEVIA )
		{
		consigne = pt_segm->m_Width = g_DesignSettings.m_CurrentViaSize;
		}

	if ( old_w < consigne) /* DRC utile puisque augm de dimension */
		{
		if(Drc_On) errdrc = Drc(this, DC, pt_segm, m_Pcb->m_Track,1);
		if(errdrc == BAD_DRC) pt_segm->m_Width = old_w;
		else GetScreen()->SetModify();
		}

	else  GetScreen()->SetModify();	/* Correction systematiquement faite si reduction */

	pt_segm->Draw(DrawPanel, DC, GR_OR) ;
	return(errdrc);
}

/*****************************************************************/
void WinEDA_PcbFrame::Edit_Track_Width(wxDC * DC,TRACK * pt_segm)
/*****************************************************************/
{
int ii;
TRACK * pt_track;
int errdrc;
int nb_segm, nb_segm_modifies = 0, nb_segm_non_modifies = 0;

	if( pt_segm == NULL) return;

	pt_track = Marque_Une_Piste(this, DC, pt_segm, &nb_segm, 0);
	for(ii = 0; ii < nb_segm; ii++, pt_track = (TRACK*) pt_track->Pnext)
		{
		pt_track->SetState(BUSY,OFF);
		errdrc = Edit_TrackSegm_Width(DC, pt_track);
		if(errdrc == BAD_DRC) nb_segm_non_modifies++;
		else nb_segm_modifies++;
		}
}


/***********************************************************/
void WinEDA_PcbFrame::Edit_Net_Width(wxDC * DC, int Netcode)
/***********************************************************/
{
TRACK *pt_segm;
int errdrc;
int nb_segm_modifies = 0;
int nb_segm_non_modifies = 0;

	if (Netcode <= 0 ) return;

	if( ! IsOK(this, "Change largeur NET ?") ) return;

	/* balayage des segments */
	for( pt_segm = m_Pcb->m_Track; pt_segm != NULL; pt_segm = (TRACK*) pt_segm->Pnext )
		{
		if ( Netcode != pt_segm->m_NetCode ) /* mauvaise piste */
			continue ;
		/* piste d'un net trouvee */
		errdrc = Edit_TrackSegm_Width(DC, pt_segm);
		if(errdrc == BAD_DRC) nb_segm_non_modifies++;
		else nb_segm_modifies++;
		}
}




/*************************************************************************/
bool WinEDA_PcbFrame::Resize_Pistes_Vias(wxDC * DC, bool Track, bool Via)
/*************************************************************************/

/* remet a jour la largeur des pistes et/ou le diametre des vias
	Si piste == 0 , pas de cht sur les pistes
	Si via == 0 , pas de cht sur les vias
*/
{
TRACK * pt_segm ;
int errdrc;
int nb_segm_modifies = 0;
int nb_segm_non_modifies = 0;

	if ( Track && Via)
		{
		if( ! IsOK(this, _("Edit All Tracks and Vias Sizes")) ) return FALSE;
		}

	else if ( Via )
		{
		if( ! IsOK(this, _("Edit All Via Sizes")) ) return FALSE;
		}

	else if( Track )
		{
		if( ! IsOK(this, _("Edit All Track Sizes")) ) return FALSE;
		}

	pt_segm = m_Pcb->m_Track ;
	for ( ; pt_segm != NULL; pt_segm = (TRACK*) pt_segm->Pnext )
		{
		if( pt_segm->m_StructType == TYPEVIA ) /* mise a jour du diametre de la via */
			{
			if ( Via )
				{
				errdrc = Edit_TrackSegm_Width(DC, pt_segm);
				if(errdrc == BAD_DRC) nb_segm_non_modifies++;
				else nb_segm_modifies++;
				}
			}
		else	/* mise a jour de la largeur du segment */
			{
			if ( Track )
				{
				errdrc = Edit_TrackSegm_Width(DC, pt_segm);
				if(errdrc == BAD_DRC) nb_segm_non_modifies++;
				else nb_segm_modifies++;
				}
			}
		}

	if ( nb_segm_modifies  ) return TRUE;
	return FALSE;
}

