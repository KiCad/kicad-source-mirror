	/*********************************************************/
	/* Routines relatives a la gestions des pistes en "DRAG" */
	/*********************************************************/

		/* Fichier dragsegm.cpp */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "trigo.h"

#include "protos.h"

#include "drag.h"

/* fonctions locales */

DRAG_SEGM::DRAG_SEGM(TRACK * segm)
{
	m_Segm = segm;
	m_StartInitialValue = m_Segm->m_Start;
	m_EndInitialValue = m_Segm->m_End;

	m_Pad_Start = m_Pad_End = NULL;
	m_Flag = 0;
}


DRAG_SEGM::~DRAG_SEGM()
{
}


void DRAG_SEGM::SetInitialValues(void)
{
	m_Segm->m_Start = m_StartInitialValue;
	m_Segm->m_End = m_EndInitialValue;
}

/*******************************************************************/
void Dessine_Segments_Dragges(WinEDA_DrawPanel * panel, wxDC * DC)
/*******************************************************************/
/* trace les segments dragges en mode EDIT  */
{
D_PAD* pt_pad;
TRACK * Track;
DRAG_SEGM * pt_drag;

	if(g_DragSegmentList == NULL ) return;

	pt_drag = g_DragSegmentList;
	for( ; pt_drag; pt_drag = pt_drag->Pnext)
		{
		int px, py;

		Track = pt_drag->m_Segm;

		Track->Draw(panel, DC, GR_XOR); /* effacement */

		pt_pad = pt_drag->m_Pad_Start;
		if( pt_pad)
			{
			px = pt_pad->m_Pos.x - g_Offset_Module.x;
			py = pt_pad->m_Pos.y - g_Offset_Module.y;
			Track->m_Start.x = px; Track->m_Start.y = py;
			}
		pt_pad = pt_drag->m_Pad_End;
		if( pt_pad)
			{
			px = pt_pad->m_Pos.x - g_Offset_Module.x;
			py = pt_pad->m_Pos.y - g_Offset_Module.y;
			Track->m_End.x = px; Track->m_End.y = py;
			}
		Track->Draw(panel, DC, GR_XOR);
		}
}


/*************************************************************************/
void Build_Drag_Liste(WinEDA_DrawPanel * panel, wxDC * DC, MODULE * Module)
/*************************************************************************/
/* Construit la liste des segments connectes aus pads du module Module
	pour le drag de ces segments
	la liste est mise a l'adresse pointee par pt_drag.
	la variable globale nb_drag_segment est incrementee du nombre de segments
	Met l'attribut EDIT sur les segments selectionnes
	et les affiche en mode EDIT (sketch)
*/
{
D_PAD * pt_pad;

	pt_pad = Module->m_Pads;
	for( ; pt_pad != NULL ; pt_pad = (D_PAD *) pt_pad->Pnext)
		{
		Build_1_Pad_SegmentsToDrag(panel, DC, pt_pad);
		}
	return;
}


/**********************************************************************************/
void Build_1_Pad_SegmentsToDrag(WinEDA_DrawPanel * panel, wxDC * DC, D_PAD * PtPad)
/**********************************************************************************/
/* Routine construisant la liste les segments de piste connectes au pad PtPad
	Les net_codes sont supposes a jour.
*/
{
TRACK * Track;
DRAG_SEGM * pt_drag;
int net_code = PtPad->m_NetCode;
int pX, pY, MasqueLayer;
BOARD * pcb = ((WinEDA_BasePcbFrame*)(panel->m_Parent))->m_Pcb;

	Track = pcb->m_Track->GetStartNetCode(net_code);

	pX = PtPad->m_Pos.x; pY = PtPad->m_Pos.y;
	MasqueLayer = PtPad->m_Masque_Layer;
	for( ; Track != NULL; Track = (TRACK*)Track->Pnext )
	{
		if( Track->m_NetCode != net_code ) break;	/* hors zone */
		if( (MasqueLayer & Track->ReturnMaskLayer()) == 0 ) continue; /* couches differentes */
		if( (pX == Track->m_Start.x) && (pY == Track->m_Start.y) )
		{
			pt_drag  = new DRAG_SEGM(Track);
			pt_drag->Pnext = g_DragSegmentList;
			g_DragSegmentList = pt_drag;
			pt_drag->m_Pad_Start = PtPad;
			Track->Draw(panel, DC, GR_XOR);
			Track->SetState(EDIT,ON);
			Track->Draw(panel, DC, GR_XOR);
		}
		if( (pX == Track->m_End.x) && (pY == Track->m_End.y) )
		{
			pt_drag  = new DRAG_SEGM(Track);
			pt_drag->Pnext = g_DragSegmentList;
			g_DragSegmentList = pt_drag;
			pt_drag->m_Pad_End = PtPad;
			Track->Draw(panel, DC, GR_XOR);
			Track->SetState(EDIT,ON);
			Track->Draw(panel, DC, GR_XOR);
		}
	}
}


/**********************************************************************************/
void Collect_TrackSegmentsToDrag(WinEDA_DrawPanel * panel, wxDC * DC,
	wxPoint & point, int MasqueLayer, int net_code)
/**********************************************************************************/
/* Routine construisant la liste les segments de piste connectes a la via Via
	Les net_codes sont supposes a jour.
*/
{
TRACK * Track;
DRAG_SEGM * pt_drag;
int pX, pY;
BOARD * pcb = ((WinEDA_BasePcbFrame*)(panel->m_Parent))->m_Pcb;

	Track = pcb->m_Track->GetStartNetCode(net_code);

	pX = point.x; pY = point.y;
	for( ; Track != NULL; Track = (TRACK*)Track->Pnext )
	{
		if( Track->m_NetCode != net_code ) break;	/* hors zone */
		if( (MasqueLayer & Track->ReturnMaskLayer()) == 0 ) continue; /* couches differentes */
		if ( Track->m_Flags & IS_DRAGGED) continue;		// already in list
		if( (pX == Track->m_Start.x) && (pY == Track->m_Start.y) )
		{
			pt_drag  = new DRAG_SEGM(Track);
			pt_drag->Pnext = g_DragSegmentList;
			g_DragSegmentList = pt_drag;
			pt_drag->m_Flag |= 1;
			Track->Draw(panel, DC, GR_XOR);
			Track->SetState(EDIT,ON);
			Track->m_Flags |= STARTPOINT;
			Track->Draw(panel, DC, GR_XOR);
		}
		if( (pX == Track->m_End.x) && (pY == Track->m_End.y) )
		{
			pt_drag  = new DRAG_SEGM(Track);
			pt_drag->Pnext = g_DragSegmentList;
			g_DragSegmentList = pt_drag;
			pt_drag->m_Flag |= 2;
			Track->m_Flags |= ENDPOINT;
			Track->Draw(panel, DC, GR_XOR);
			Track->SetState(EDIT,ON);
			Track->Draw(panel, DC, GR_XOR);
		}
	}
}


/*****************************/
void EraseDragListe(void)
/*****************************/
/* Routine de liberation memoire de la liste des structures DRAG_SEGM
	remet a zero le pointeur global g_DragSegmentList
*/
{
DRAG_SEGM * pt_drag, * NextStruct;

	if( g_DragSegmentList == NULL ) return;

	pt_drag = g_DragSegmentList;
	for( ; pt_drag != NULL; pt_drag =  NextStruct)
		{
		NextStruct = pt_drag->Pnext; delete pt_drag;
		}
	g_DragSegmentList = NULL;
}
