	/********************************************************/
	/* Effacements : Routines de sauvegarde et d'effacement */
	/********************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

/* Routines externes : */

/* Routines Locales */

/********************************************/
void WinEDA_PcbFrame::UnDeleteItem(wxDC * DC)
/********************************************/
/* Restitution d'un element (MODULE ou TRACK ) Efface
*/
{
EDA_BaseStruct * PtStruct, *PtNext;
TRACK * pt_track;
int net_code;

	if( ! g_UnDeleteStackPtr ) return;

	g_UnDeleteStackPtr --;
	PtStruct = g_UnDeleteStack[g_UnDeleteStackPtr];
	if ( PtStruct == NULL ) return;	// Ne devrait pas se produire

	switch(PtStruct->m_StructType)
		{
		case TYPEVIA:
		case TYPETRACK:
			for( ; PtStruct != NULL; PtStruct = PtNext)
				{
				PtNext = PtStruct->Pnext;
				PtStruct->SetState(DELETED,OFF);	/* Effacement du bit DELETED */
				((TRACK*)PtStruct)->Draw(DrawPanel, DC, GR_OR);
				}

			PtStruct = g_UnDeleteStack[g_UnDeleteStackPtr];
			net_code = ((TRACK*)PtStruct)->m_NetCode;
			pt_track = ( (TRACK*) PtStruct)->GetBestInsertPoint( m_Pcb );
			((TRACK*)PtStruct)->Insert(m_Pcb, pt_track);
			g_UnDeleteStack[g_UnDeleteStackPtr] = NULL;

			test_1_net_connexion(DC, net_code );
			Affiche_Infos_Status_Pcb(this);
			break;

		case TYPEMODULE:
			/* Erase general rastnest if needed */
			if(g_Show_Ratsnest) DrawGeneralRatsnest(DC);
			/* Reinsertion du module dans la liste chainee des modules,
			en debut de chaine */
			PtStruct->Pback = m_Pcb;
			PtNext = m_Pcb->m_Modules;
			PtStruct->Pnext = PtNext;
			if( PtNext) PtNext->Pback = PtStruct;
			m_Pcb->m_Modules = (MODULE*)PtStruct;
			g_UnDeleteStack[g_UnDeleteStackPtr] = NULL;

			((MODULE *) PtStruct)->Draw(DrawPanel, DC, wxPoint(0,0), GR_OR);

			PtStruct->SetState(DELETED,OFF);	/* Creal DELETED flag */
			PtStruct->m_Flags = 0;
			m_Pcb->m_Status_Pcb = 0 ;
			build_liste_pads();
			ReCompile_Ratsnest_After_Changes( DC );
			break;

		default:
			DisplayError(this, wxT("WinEDA_PcbFrame::UnDeleteItem(): unexpected Struct type"));
			break;
		}
}


	/**************************************************************/
	/* void * SaveItemEfface(int type, void * PtItem, int nbitems) */
	/**************************************************************/

/* Sauvegarde d'un element aux fins de restitution par Undelete
	Supporte actuellement : Module et segments de piste
*/
EDA_BaseStruct * WinEDA_PcbFrame::SaveItemEfface(EDA_BaseStruct * PtItem, int nbitems)
{
EDA_BaseStruct * NextS, * PtStruct = PtItem;
int ii;

	if( (PtItem == NULL) || (nbitems == 0) ) return NULL;

	if (g_UnDeleteStackPtr >= UNDELETE_STACK_SIZE )
		{
		/* Delete last deleted item, and shift stack. */
		DeleteStructList(g_UnDeleteStack[0]);
		for (ii = 0; ii < (g_UnDeleteStackPtr-1); ii++)
			{
			g_UnDeleteStack[ii] = g_UnDeleteStack[ii + 1];
			}
		g_UnDeleteStackPtr--;;
		}

	switch ( PtStruct->m_StructType )
		{
		case TYPEVIA:
		case TYPETRACK:
			{
			EDA_BaseStruct * Back = NULL;
			g_UnDeleteStack[g_UnDeleteStackPtr++] = PtStruct;

			for ( ; nbitems > 0; nbitems--, PtStruct = NextS)
				{
				NextS = PtStruct->Pnext;
				((TRACK*)PtStruct)->UnLink();
				PtStruct->SetState(DELETED, ON);
				if( nbitems <= 1 ) NextS = NULL;	/* fin de chaine */
				PtStruct->Pnext = NextS;
				PtStruct->Pback = Back; Back = PtStruct;
				if(NextS == NULL) break;
				}
			}
			break;

		case TYPEMODULE:
			{
			MODULE * Module = (MODULE*)PtItem;
			Module->UnLink();
			Module->SetState(DELETED, ON);
			g_UnDeleteStack[g_UnDeleteStackPtr++] = Module;
			build_liste_pads();
			}
			break;

		default: break;
		}

	return(g_UnDeleteStack[g_UnDeleteStackPtr-1]);
}


