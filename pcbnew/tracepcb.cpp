		/*****************************************/
		/* Routines generales d'affichage du PCB */
		/*****************************************/

	/* fichier tracepcb.cpp */
/*
 Routines d'affichage grille, Boite de coordonnees, Curseurs, marqueurs ...
*/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "pcbplot.h"

#include "protos.h"

/* Routines Locales : */

/* Variables Locales */


/**********************************************************************/
void WinEDA_ModuleEditFrame::RedrawActiveWindow(wxDC * DC, bool EraseBg)
/**********************************************************************/

/* Trace le PCB, et les elements complementaires ( axes, grille .. )
 pour l'ecran actif et ses sous ecran
*/

{
MODULE * Module;
PCB_SCREEN * screen = GetScreen();

	if ( ! m_Pcb || ! screen ) return;

	ActiveScreen = screen;
	GRSetDrawMode(DC, GR_COPY);

	if ( EraseBg ) DrawPanel->EraseScreen(DC);

	DrawPanel->DrawBackGround(DC);
	TraceWorkSheet(DC, screen, 0);

	Module = (MODULE*) m_Pcb->m_Modules;
	for ( ; Module != NULL; Module = (MODULE *) Module->Pnext )
		{
		Module->Draw(DrawPanel, DC, wxPoint(0,0), GR_OR);
		}


	Affiche_Status_Box();

	if( DrawPanel->ManageCurseur )
		DrawPanel->ManageCurseur(DrawPanel, DC, FALSE);

	/* Reaffichage du curseur */
	DrawPanel->Trace_Curseur(DC);

	screen->ClrRefreshReq();
}

/****************************************************************/
void WinEDA_PcbFrame::RedrawActiveWindow(wxDC * DC, bool EraseBg)
/****************************************************************/
/* Trace le PCB, et les elements complementaires ( axes, grille .. )
 pour l'ecran actif et ses sous ecran
*/
{
PCB_SCREEN * Screen = GetScreen();

	if ( ! m_Pcb || ! Screen ) return;

	ActiveScreen = GetScreen();
	GRSetDrawMode(DC, GR_COPY);

	if ( EraseBg ) DrawPanel->EraseScreen(DC);

	DrawPanel->DrawBackGround(DC);

	Trace_Pcb(DC, GR_OR);
	TraceWorkSheet(DC, GetScreen(), 0);
	Affiche_Status_Box();

	/* Reaffichage des curseurs */
	for( Screen = GetScreen(); Screen != NULL; Screen = Screen->Next() )
	{
		if( DrawPanel->ManageCurseur )
			DrawPanel->ManageCurseur(DrawPanel, DC, FALSE);
		DrawPanel->Trace_Curseur(DC);
	}
}

/****************************************************/
void WinEDA_PcbFrame::Trace_Pcb(wxDC * DC, int mode)
/****************************************************/
/* Trace l'ensemble des elements du PCB sur l'ecran actif*/
{
MARQUEUR * Marqueur;
MODULE * Module;
EDA_BaseStruct * PtStruct;

	if ( ! m_Pcb ) return;

	Module = (MODULE*) m_Pcb->m_Modules;
	for ( ; Module != NULL; Module = (MODULE *) Module->Pnext )
		{
		bool display = TRUE, MaskLay = ALL_CU_LAYERS;
		if( Module->m_Flags & IS_MOVED ) continue ;

		if( ! DisplayOpt.Show_Modules_Cmp )
			{
			if(Module->m_Layer == CMP_N) display = FALSE;
			MaskLay &= ~CMP_LAYER;
			}
		if( ! DisplayOpt.Show_Modules_Cu )
			{
			if(Module->m_Layer == CUIVRE_N) display = FALSE;
			MaskLay &= ~CUIVRE_LAYER;
			}

		if ( display ) Module->Draw(DrawPanel, DC, wxPoint(0,0), mode);
		else  Trace_Pads_Only(DrawPanel, DC, Module, 0, 0, MaskLay, mode);
		}

	/* Trace des elements particuliers de Drawings Pcb */

	PtStruct = m_Pcb->m_Drawings;
	for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
		{
		if ( PtStruct->m_Flags & IS_MOVED ) continue;

		switch(PtStruct->m_StructType)
			{
			case TYPECOTATION:
				((COTATION*) PtStruct)->Draw(DrawPanel, DC, wxPoint(0,0), mode);
				break;

			case TYPETEXTE:
				((TEXTE_PCB*) PtStruct)->Draw(DrawPanel, DC, wxPoint(0,0), mode );
				break;

			case TYPEMIRE:
				((MIREPCB*) PtStruct)->Draw(DrawPanel, DC, wxPoint(0,0), mode);
				break;

			case TYPEMARQUEUR:	 /* Trace des marqueurs */
				Marqueur = ( MARQUEUR*) PtStruct;
				Marqueur->Draw(DrawPanel, DC, mode);
				break;

			default: break;
			}
		}

	Trace_Pistes(DrawPanel, m_Pcb, DC, mode);
	if ( g_HightLigt_Status ) DrawHightLight(DC, g_HightLigth_NetCode) ;

	EDGE_ZONE * segment = m_Pcb->m_CurrentLimitZone;
	for( ; segment != NULL; segment = (EDGE_ZONE *) segment->Pback)
		{
		if ( segment->m_Flags & IS_MOVED ) continue;
		Trace_DrawSegmentPcb(DrawPanel, DC, segment, mode);
		}

	Trace_PcbEdges(DC, mode);
	DrawGeneralRatsnest(DC);

	m_CurrentScreen->ClrRefreshReq();
}


/**************************************************************/
void WinEDA_PcbFrame::Trace_PcbEdges(wxDC * DC, int mode_color)
/**************************************************************/
/* impression des contours ( edge pcb) : et draw */
{
EDA_BaseStruct * PtStruct;

	if ( ! m_Pcb ) return;
	for ( PtStruct = m_Pcb->m_Drawings; PtStruct != NULL; PtStruct = PtStruct->Pnext)
		{
		if ( PtStruct->m_Flags & IS_MOVED ) continue;
		if( PtStruct->m_StructType != TYPEDRAWSEGMENT ) continue;
		Trace_DrawSegmentPcb(DrawPanel, DC, (DRAWSEGMENT *) PtStruct,mode_color);
		}
}



