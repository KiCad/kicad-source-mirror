	/************************************************/
	/* Module editor: Dialog box for editing module	*/
	/*  properties and carateristics				*/
	/************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "trigo.h"
#include "bitmaps.h"
#include "3d_struct.h"
#include "3d_viewer.h"

#include "protos.h"

/* Variables locales: */
bool GoToEditor = FALSE;
	/**************************************/
	/* class WinEDA_ModulePropertiesFrame */
	/**************************************/

#include "dialog_edit_module.cpp"

/*******************************************************************/
void WinEDA_BasePcbFrame::InstallModuleOptionsFrame(MODULE * Module,
					wxDC * DC, const wxPoint & pos)
/*******************************************************************/
/* Fonction relai d'installation de la frame d'édition des proprietes
du module*/
{
	WinEDA_ModulePropertiesFrame * frame = new WinEDA_ModulePropertiesFrame(this,
					Module, DC, pos);
	frame->ShowModal(); frame->Destroy();

	if ( GoToEditor && GetScreen()->m_CurrentItem )
	{
		if (m_Parent->m_ModuleEditFrame == NULL )
		{
			m_Parent->m_ModuleEditFrame = new WinEDA_ModuleEditFrame(this,
						m_Parent,_("Module Editor"),
						wxPoint(-1, -1), wxSize(600,400) );
		}

		m_Parent->m_ModuleEditFrame->Load_Module_Module_From_BOARD(
			(MODULE*)GetScreen()->m_CurrentItem);
		GetScreen()->m_CurrentItem = NULL;

		GoToEditor = FALSE;
		m_Parent->m_ModuleEditFrame->Show(TRUE);
		m_Parent->m_ModuleEditFrame->Iconize(FALSE);
	}
}




/*******************************************************************/
void WinEDA_ModuleEditFrame::Place_Ancre(MODULE* pt_mod , wxDC * DC)
/*******************************************************************/
/*
 Repositionne l'ancre sous le curseur souris
	Le module doit etre d'abort selectionne
*/
{
int deltaX, deltaY;
EDA_BaseStruct * PtStruct;
D_PAD * pt_pad;

	if(pt_mod == NULL) 	return ;

	pt_mod->DrawAncre(DrawPanel, DC, wxPoint(0,0), DIM_ANCRE_MODULE, GR_XOR);

	deltaX = pt_mod->m_Pos.x - GetScreen()->m_Curseur.x;
	deltaY = pt_mod->m_Pos.y - GetScreen()->m_Curseur.y;

	pt_mod->m_Pos = GetScreen()->m_Curseur;

	/* Mise a jour des coord relatives des elements:
	les coordonnees relatives sont relatives a l'ancre, pour orient 0.
	il faut donc recalculer deltaX et deltaY en orientation 0 */
	RotatePoint(&deltaX, &deltaY, - pt_mod->m_Orient);

	/* Mise a jour des coord relatives des pads */
	pt_pad = (D_PAD*)pt_mod->m_Pads;
	for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext)
		{
		pt_pad->m_Pos0.x += deltaX; pt_pad->m_Pos0.y += deltaY;
		}

	/* Mise a jour des coord relatives contours .. */
	PtStruct = pt_mod->m_Drawings;
	for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext)
		{
		switch( PtStruct->m_StructType)
			{
			case TYPEEDGEMODULE:
				#undef STRUCT
				#define STRUCT ((EDGE_MODULE*) PtStruct)
				STRUCT->m_Start0.x += deltaX; STRUCT->m_Start0.y += deltaY;
				STRUCT->m_End0.x += deltaX; STRUCT->m_End0.y += deltaY;
				break;

			case TYPETEXTEMODULE:
				#undef STRUCT
				#define STRUCT ((TEXTE_MODULE*) PtStruct)
				STRUCT->m_Pos0.x += deltaX; STRUCT->m_Pos0.y += deltaY;
				break;

			default:
				break;
			}
		}
	pt_mod->Set_Rectangle_Encadrement();
	pt_mod->DrawAncre(DrawPanel, DC, wxPoint(0,0), DIM_ANCRE_MODULE, GR_OR);
}

/**********************************************************************/
void WinEDA_ModuleEditFrame::RemoveStruct(EDA_BaseStruct * Item, wxDC * DC)
/**********************************************************************/
{
	if ( Item == NULL ) return;

	switch( Item->m_StructType )
		{
		case TYPEPAD:
			DeletePad( (D_PAD*) Item, DC);
			break;

		case TYPETEXTEMODULE:
			{
			TEXTE_MODULE * text = (TEXTE_MODULE *) Item;
			if ( text->m_Type == TEXT_is_REFERENCE )
				{
				DisplayError(this, _("Text is REFERENCE!") );
				break;
				}
			if ( text->m_Type == TEXT_is_VALUE )
				{
				DisplayError(this, _("Text is VALUE!") );
				break;
				}
			DeleteTextModule(text, DC);
			}
			break;

		case TYPEEDGEMODULE:
			Delete_Edge_Module((EDGE_MODULE *) Item, DC);
			break;

		case TYPEMODULE:
			break;

		default:
			{
			wxString Line;
			Line.Printf( wxT(" Remove: StructType %d Inattendu"),
										Item->m_StructType);
			DisplayError(this, Line);
			}
			break;
		}
}

