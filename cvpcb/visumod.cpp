	/************************************************/
	/* Routines de visualisation du module courant  */
	/************************************************/


#include "fctsys.h"
#include "common.h"
#include "cvpcb.h"
#include "macros.h"
#include "pcbnew.h"

#include "protos.h"

/* defines locaux */

/* Variables locales */


/*******************************************************************/
void WinEDA_DisplayFrame::RedrawActiveWindow(wxDC * DC, bool EraseBg)
/*******************************************************************/
/* Affiche le module courant */
{
	if (! m_Pcb ) return;
		
MODULE * Module= m_Pcb->m_Modules;

	ActiveScreen = (PCB_SCREEN *) GetScreen();

	if ( EraseBg ) DrawPanel->EraseScreen(DC);

	DrawPanel->DrawBackGround(DC);

	if( Module )
		{
		Module->Draw(DrawPanel, DC, wxPoint(0,0), GR_COPY);
		Module->Display_Infos(this);
		}

	Affiche_Status_Box();
	DrawPanel->Trace_Curseur(DC);
}



/***********************************************/
void DeleteStructure( void * GenericStructure )
/***********************************************/
/* Supprime de la liste chainee la stucture pointee par GenericStructure
	et libere la memoire correspondante
*/
{
EDA_BaseStruct * PtStruct, *PtNext, *PtBack;
int IsDeleted;
int typestruct;
wxString msg;

	PtStruct = (EDA_BaseStruct *) GenericStructure;
	if( PtStruct == NULL) return ;

	typestruct = (int)PtStruct->m_StructType;
	IsDeleted = PtStruct->GetState(DELETED);

	PtNext = PtStruct->Pnext;
	PtBack = PtStruct->Pback;

	switch( typestruct )
		{
		case TYPE_NOT_INIT:
			DisplayError(NULL, wxT("DeleteStruct: Type Structure Non Initialise"));
			break;

		 case PCB_EQUIPOT_STRUCT_TYPE:
			#undef Struct
			#define Struct ((EQUIPOT*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;

		 case TYPEMODULE:
			#undef Struct
			#define Struct ((MODULE*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;


		 case TYPEPAD:
			#undef Struct
			#define Struct ((D_PAD*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;

		case TYPECOTATION:
			#undef Struct
			#define Struct ((COTATION*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;

		case TYPEMIRE:
			#undef Struct
			#define Struct ((MIREPCB*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;

		case TYPEDRAWSEGMENT:
			#undef Struct
			#define Struct ((DRAWSEGMENT*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;

		 case TYPETEXTE:
			#undef Struct
			#define Struct ((TEXTE_PCB*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;


		 case TYPETEXTEMODULE:
			#undef Struct
			#define Struct ((TEXTE_MODULE*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;

		 case TYPEEDGEMODULE:
			#undef Struct
			#define Struct ((EDGE_MODULE*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;

		case TYPETRACK:
			#undef Struct
			#define Struct ((TRACK*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;

		case TYPEVIA:
			#undef Struct
			#define Struct ((SEGVIA*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;

		case TYPEZONE:
			#undef Struct
			#define Struct ((SEGZONE*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;

		 case TYPEMARQUEUR:
			#undef Struct
			#define Struct ((MARQUEUR*)PtStruct)
			Struct->UnLink();
			delete Struct;
			break;

		case TYPEPCB:

		default:
			msg.Printf( wxT(" DeleteStructure: Type %d Inattendu"),
										PtStruct->m_StructType);
			DisplayError(NULL, msg);
			break;
		}
}


