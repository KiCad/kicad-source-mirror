/*****************************************************************/
/* fonctions membres de la classe EQUIPOT et fonctions associées */
/*****************************************************************/

#include "fctsys.h"
#include "wxstruct.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "protos.h"


	/*********************************************************/
	/* classe EQUIPOT: gestion des listes d'equipotentielles */
	/*********************************************************/

/* Constructeur de la classe EQUIPOT */
EQUIPOT::EQUIPOT(EDA_BaseStruct * StructFather):
			EDA_BaseStruct( StructFather, PCB_EQUIPOT_STRUCT_TYPE)
{
	m_NetCode = 0;
	m_NbNodes = m_NbLink = m_NbNoconn = 0;
	m_Masque_Layer = 0;
	m_Masque_Plan = 0;
	m_ForceWidth = 0;
	m_PadzoneStart = NULL;// pointeur sur debut de liste pads du net
	m_PadzoneEnd = NULL;	// pointeur sur fin de liste pads du net
	m_RatsnestStart = NULL;	// pointeur sur debut de liste ratsnests du net
	m_RatsnestEnd = NULL;	// pointeur sur fin de liste ratsnests du net

}

	/* destructeut */

EQUIPOT::~EQUIPOT(void)
{
}

void EQUIPOT::UnLink( void )
{
	/* Modification du chainage arriere */
	if( Pback )
		{
		if( Pback->m_StructType != TYPEPCB)
			{
			Pback->Pnext = Pnext;
			}

		else /* Le chainage arriere pointe sur la structure "Pere" */
			{
			((BOARD*)Pback)->m_Equipots = (EQUIPOT*)Pnext;
			}
		}

	/* Modification du chainage avant */
	if( Pnext) Pnext->Pback = Pback;

	Pnext = Pback = NULL;
}


/*************************************************/
EQUIPOT * GetEquipot(BOARD * pcb, int netcode)
/**************************************************/
/*
	retourne un pointeur sur la structure EQUIPOT de numero netcode
*/
{
EQUIPOT * Equipot ;

	if( netcode <= 0 ) return NULL;
   
	Equipot = (EQUIPOT*)pcb->m_Equipots;
	while ( Equipot )
		{
		if(Equipot->m_NetCode == netcode ) break;
		Equipot = (EQUIPOT*) Equipot->Pnext;
		}

	return(Equipot);
}


/*********************************************************/
int EQUIPOT:: ReadEquipotDescr(FILE * File, int * LineNum)
/*********************************************************/
/* Routine de lecture de 1 descr Equipotentielle.
	retourne 0 si OK
   			1 si lecture incomplete
*/
{
char Line[1024], Ltmp[1024];
int tmp;

	while( GetLine(File, Line, LineNum ) )
		{
		if( strnicmp(Line,"$End",4) == 0 )return 0;

		if( strncmp(Line,"Na", 2) == 0 )	/* Texte */
			{
			sscanf(Line+2," %d", &tmp);
			m_NetCode = tmp;

			ReadDelimitedText(Ltmp, Line + 2, sizeof(Ltmp) );
			m_Netname = CONV_FROM_UTF8(Ltmp);
			continue;
			}

		if( strncmp(Line,"Lw", 2) == 0 )	/* Texte */
			{
			sscanf(Line+2," %d", &tmp);
			m_ForceWidth = tmp;
			continue;
			}
		}
	return 1;
}


/********************************************/
int EQUIPOT:: WriteEquipotDescr(FILE * File)
/********************************************/
{
	if( GetState(DELETED) ) return(0);

	fprintf( File,"$EQUIPOT\n");
	fprintf( File,"Na %d \"%.16s\"\n", m_NetCode, CONV_TO_UTF8(m_Netname) );
	fprintf( File,"St %s\n","~");
	if( m_ForceWidth) fprintf( File,"Lw %d\n",m_ForceWidth );
	fprintf( File,"$EndEQUIPOT\n");
	return(1);
}


