		/****************************/
		/* affichage des empreintes */
		/****************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

#define Pad_fill (Pad_Fill_Item.State == RUN)

static void Pad_Surbrillance(WinEDA_DrawPanel * panel, wxDC * DC, MODULE * Module, int NetCode);

/* variables locales : */
static int draw_mode ;

/**************************************************/
int WinEDA_PcbFrame::Select_High_Light(wxDC * DC)
/**************************************************/
/* Localise track ou pad et met en surbrillance le net correspondant
Retourne le netcode, ou -1 si pas de net localisé */
{
TRACK * pt_piste;
D_PAD* pt_pad ;
int masquelayer = g_TabOneLayerMask[GetScreen()->m_Active_Layer];
int code = -1;

	if ( g_HightLigt_Status ) Hight_Light(DC);
	pt_piste = Locate_Pistes(m_Pcb->m_Track, masquelayer, CURSEUR_OFF_GRILLE);
	if ( pt_piste)
	{
		code = g_HightLigth_NetCode = pt_piste->m_NetCode;
		Hight_Light(DC);
	}
	else
	{
		pt_pad = Locate_Any_Pad(m_Pcb, CURSEUR_OFF_GRILLE);
		if( pt_pad != NULL )
		{
			code = g_HightLigth_NetCode = pt_pad->m_NetCode ;
			Hight_Light(DC) ;
		}
	}

	return code;
}


/*******************************************/
void WinEDA_PcbFrame::Hight_Light(wxDC * DC)
/*******************************************/
/*
 fonction d'appel de Surbrillance a partir du menu
 Met ou supprime la surbrillance d'un net pointe par la souris
*/
{
	g_HightLigt_Status = !g_HightLigt_Status;
	DrawHightLight( DC, g_HightLigth_NetCode) ;
}



/****************************************************************/
void WinEDA_PcbFrame::DrawHightLight(wxDC * DC, int NetCode)
/****************************************************************/
/* Met ou supprime la surbrillance d'un net de nom NetName
*/
{
TRACK * pts ;
MODULE * Module;
PCB_SCREEN * OldScreen = (PCB_SCREEN *) ActiveScreen;

	if(g_HightLigt_Status ) draw_mode = GR_SURBRILL | GR_OR;
	else draw_mode = GR_AND | GR_SURBRILL;

	Module = m_Pcb->m_Modules;

	/* Surbrillance des Pastilles : */

	for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
	{
		Pad_Surbrillance(DrawPanel, DC, Module, NetCode) ;
	}

	/* Surbrillance des pistes : */
	for ( pts = m_Pcb->m_Track; pts != NULL; pts = (TRACK*) pts->Pnext)
	{
		/* est ce que la piste fait partie du net ? : */
		if( pts->m_NetCode == NetCode )
		{
			pts->Draw(DrawPanel, DC, draw_mode);
		}
	}
}


/*******************************************************/
static void Pad_Surbrillance(WinEDA_DrawPanel * panel,
				wxDC * DC, MODULE * Module, int NetCode)
/*******************************************************/
/* Mise en Surbrillance des Pads */
{
D_PAD * pt_pad ;

	/* trace des pastilles */
	for(pt_pad = Module->m_Pads; pt_pad != NULL; pt_pad = (D_PAD*)pt_pad->Pnext)
		{
		if ( pt_pad->m_NetCode == NetCode )
			{
			pt_pad->Draw(panel, DC, wxPoint(0,0),draw_mode);
			}
		}
}

