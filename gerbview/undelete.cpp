	/********************************************************/
	/* Effacements : Routines de sauvegarde et d'effacement */
	/********************************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"

#include "protos.h"

/* Routines externes : */

/* Routines Locales */

/***********************************************/
void WinEDA_GerberFrame::UnDeleteItem(wxDC * DC)
/***********************************************/
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
				Trace_Segment(DrawPanel, DC, (TRACK*) PtStruct, GR_OR);
				}

			PtStruct = g_UnDeleteStack[g_UnDeleteStackPtr];
			net_code = ((TRACK*)PtStruct)->m_NetCode;
			pt_track = ( (TRACK*) PtStruct)->GetBestInsertPoint(m_Pcb);
			((TRACK*)PtStruct)->Insert(m_Pcb, pt_track);

			g_UnDeleteStack[g_UnDeleteStackPtr] = NULL;
			break;

		default:
			DisplayError(this, wxT("Undelete struct: type Struct inattendu"));
			break;
		}
}


/********************************************************************/
EDA_BaseStruct * SaveItemEfface(EDA_BaseStruct * PtItem, int nbitems)
/********************************************************************/
/* Sauvegarde d'un element aux fins de restitution par Undelete
	Supporte actuellement : Module et segments de piste
*/
{
EDA_BaseStruct * NextS, * PtStruct = PtItem;
int ii;
	if( (PtItem == NULL) || (nbitems == 0) ) return NULL;

	if (g_UnDeleteStackPtr >= UNDELETE_STACK_SIZE )
		{
		/* Delete last deleted item, and shift stack. */
		DeleteStructure(g_UnDeleteStack[0]);
		for (ii = 0; ii < (g_UnDeleteStackPtr-1); ii++)
			{
			g_UnDeleteStack[ii] = g_UnDeleteStack[ii + 1];
			}
		g_UnDeleteStackPtr--;;
		}
	g_UnDeleteStack[g_UnDeleteStackPtr++] = PtItem;

	switch ( PtStruct->m_StructType )
		{
		case TYPEVIA:
		case TYPETRACK:
			{
			EDA_BaseStruct * Back = NULL;
			g_UnDeleteStack[g_UnDeleteStackPtr-1] = PtStruct;

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

		default: break;
		}

	return(g_UnDeleteStack[g_UnDeleteStackPtr-1]);
}


