	/**********************************************/
	/* GERBVIEW : Routines d'initialisation globale */
	/******* Fichier INITPCB.C ********************/
	/**********************************************/


#include "fctsys.h"

#include "common.h"
#include "gerbview.h"

#include "protos.h"

/* Routines Locales */



/********************************************************/
bool WinEDA_GerberFrame::Clear_Pcb(wxDC * DC, bool query)
/********************************************************/
/* Realise les init des pointeurs et variables
	Si Item == NULL, il n'y aura pas de confirmation
*/
{
int layer;

	if( m_Pcb == NULL ) return FALSE;

	if ( query )
		{
		if (m_Pcb->m_Drawings || m_Pcb->m_Track || m_Pcb->m_Zone )
			{
			if( ! IsOK(this, _("Current Data will be lost ?")) ) return FALSE;
			}
		}

	DeleteStructList(m_Pcb->m_Drawings);
	m_Pcb->m_Drawings = NULL;

	DeleteStructList(m_Pcb->m_Track);
	m_Pcb->m_Track = NULL;
	m_Pcb->m_NbSegmTrack = 0;

	DeleteStructList(m_Pcb->m_Zone);
	m_Pcb->m_Zone = NULL;
	m_Pcb->m_NbSegmZone = 0;

	for ( ; g_UnDeleteStackPtr != 0; )
	{
		g_UnDeleteStackPtr--;
		DeleteStructList(g_UnDeleteStack[ g_UnDeleteStackPtr]);
	}

	/* init pointeurs  et variables */
	for ( layer = 0; layer < 32; layer++ )
		{
		if ( g_GERBER_Descr_List[layer] )
			g_GERBER_Descr_List[layer]->InitToolTable();
		}

	/* remise a 0 ou a une valeur initiale des variables de la structure */
	m_Pcb->m_BoundaryBox.SetOrigin(0,0);
	m_Pcb->m_BoundaryBox.SetSize(0,0);
	m_Pcb->m_Status_Pcb = 0;
	m_Pcb->m_NbLoclinks = 0;
	m_Pcb->m_NbLinks = 0;
	m_Pcb->m_NbPads = 0;
	m_Pcb->m_NbNets = 0;
	m_Pcb->m_NbNodes = 0;
	m_Pcb->m_NbNoconnect = 0;
	m_Pcb->m_NbSegmTrack = 0;
	m_Pcb->m_NbSegmZone = 0;

	/* Init parametres de gestion des ecrans PAD et PCB */
	m_CurrentScreen = ActiveScreen = ScreenPcb;
	GetScreen()->Init();

	return TRUE;
}

/*********************************************************/
void WinEDA_GerberFrame::Erase_Zones(wxDC * DC, bool query)
/*********************************************************/
{

	if( query && !IsOK(this, _("Delete zones ?") ) ) return ;

	if( m_Pcb->m_Zone )
		{
		DeleteStructList(m_Pcb->m_Zone);
		m_Pcb->m_Zone = NULL;
		m_Pcb->m_NbSegmZone = 0;
		}
	ScreenPcb->SetModify();
}


/*****************************************************/
void WinEDA_GerberFrame::Erase_Segments_Pcb(wxDC * DC,
					bool all_layers, bool query)
/*****************************************************/
{
EDA_BaseStruct * PtStruct, *PtNext;
int layer = GetScreen()->m_Active_Layer;

	if ( all_layers ) layer = -1;

	PtStruct = (EDA_BaseStruct *) m_Pcb->m_Drawings;
	for( ; PtStruct != NULL; PtStruct = PtNext)
		{
		PtNext = PtStruct->Pnext;
		switch( PtStruct->m_StructType )
			{
			case TYPEDRAWSEGMENT:
				if( (((DRAWSEGMENT*)PtStruct)->m_Layer == layer)
					|| layer < 0)
					 DeleteStructure(PtStruct);
				break;

			case TYPETEXTE:
				if( (((TEXTE_PCB*)PtStruct)->m_Layer == layer)
					|| layer < 0)
					DeleteStructure(PtStruct);
				break;

			case TYPECOTATION:
				if( (((COTATION*)PtStruct)->m_Layer == layer)
					|| layer < 0)
					DeleteStructure(PtStruct);
				break;

			case TYPEMIRE:
				if( (((MIREPCB*)PtStruct)->m_Layer == layer)
 					|| layer < 0)
					DeleteStructure(PtStruct);
				break;

			default:
				DisplayError(this, wxT("Type Draw inconnu/inattendu"));
				break;
			}
		}

	ScreenPcb->SetModify();
}


/****************************************************************/
void WinEDA_GerberFrame::Erase_Pistes(wxDC * DC, int masque_type,
			bool query)
/****************************************************************/
/* Efface les segments de piste, selon les autorisations affichees
masque_type = masque des options de selection:
SEGM_FIXE, SEGM_AR
	Si un des bits est a 1, il n'y a pas effacement du segment de meme bit a 1
*/
{
TRACK * pt_segm;
EDA_BaseStruct * PtNext;

	if( query && ! IsOK(this, _("Delete Tracks?")) ) return;

	/* Marquage des pistes a effacer */
	for( pt_segm = m_Pcb->m_Track; pt_segm != NULL; pt_segm = (TRACK*) PtNext)
		{
		PtNext = pt_segm->Pnext;
		if( pt_segm->GetState(SEGM_FIXE|SEGM_AR) & masque_type) continue;
		DeleteStructure(pt_segm);
		}

	ScreenPcb->SetModify();
}


/*****************************************************************/
void WinEDA_GerberFrame::Erase_Textes_Pcb(wxDC * DC, bool query)
/*****************************************************************/
{
EDA_BaseStruct * PtStruct, *PtNext;

	if( query && ! IsOK(this, _("Delete Pcb Texts") ) ) return;

	PtStruct = (EDA_BaseStruct*) m_Pcb->m_Drawings;
	for( ; PtStruct != NULL; PtStruct = PtNext)
		{
		PtNext = PtStruct->Pnext;
		if(PtStruct->m_StructType == TYPETEXTE ) DeleteStructure(PtStruct);
		}

	ScreenPcb->SetModify();
}

/*******************************************************************/
void WinEDA_GerberFrame::Erase_Current_Layer(wxDC * DC, bool query)
/*******************************************************************/
{
TRACK * pt_segm;
EDA_BaseStruct * PtNext;
int layer = GetScreen()->m_Active_Layer;
wxString msg;

	msg.Printf( _("Delete Layer %d"), layer+1);
	if( query && ! IsOK(this, msg) ) return;

	/* Marquage des pistes a effacer */
	for( pt_segm = m_Pcb->m_Track; pt_segm != NULL; pt_segm = (TRACK*) PtNext)
		{
		PtNext = pt_segm->Pnext;
		if( pt_segm->m_Layer != layer) continue;
		DeleteStructure(pt_segm);
		}

	ScreenPcb->SetModify();
	ScreenPcb->SetRefreshReq();
}



