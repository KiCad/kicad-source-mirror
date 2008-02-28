	/*******************************************************/
	/* delsheet.cpp  Routine d'effacement d'une hierarchie */
	/*******************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"
#include "schframe.h"


/**************************************************************************/
void DeleteSubHierarchy(DrawSheetStruct * FirstSheet, bool confirm_deletion)
/**************************************************************************/

/*  Free (delete) all schematic data (include the sub hierarchy sheets )
	for the hierarchical sheet FirstSheet
	FirstSheet is not deleted.
*/
{
EDA_BaseStruct *DrawStruct;
EDA_BaseStruct *EEDrawList;
WinEDA_SchematicFrame * frame = g_EDA_Appl->m_SchematicFrame;
wxString msg;

	if( FirstSheet == NULL ) return;

	if( FirstSheet->Type() != DRAW_SHEET_STRUCT_TYPE)
		{
		DisplayError(NULL,
				wxT("DeleteSubHierarchy error(): NOT a Sheet"));
		return;
		}

	/* effacement du sous schema correspondant */
	if( FirstSheet->m_AssociatedScreen->IsModify() && confirm_deletion )
	{
		msg.Printf( _("Sheet %s (file %s) modified. Save it?"),
					FirstSheet->m_SheetName.GetData(),
					FirstSheet->GetFileName().GetData());
		if( IsOK(NULL, msg) )
		{
			frame->SaveEEFile(FirstSheet->m_AssociatedScreen, FILE_SAVE_AS);
		}
	}

	/* free the sub hierarchy */
	if(FirstSheet->m_AssociatedScreen){
		EEDrawList = FirstSheet->m_AssociatedScreen->EEDrawList;
		while (EEDrawList != NULL)
		{
			DrawStruct = EEDrawList;
			EEDrawList = EEDrawList->Pnext;
			if( DrawStruct->Type() == DRAW_SHEET_STRUCT_TYPE)
			{
				DeleteSubHierarchy((DrawSheetStruct *) DrawStruct, confirm_deletion);
			}
		}
		/* Effacement des elements de la feuille courante */
		FirstSheet->m_AssociatedScreen->FreeDrawList();
	}
}

/*********************************************************************/
//void ClearDrawList(EDA_BaseStruct *DrawList, bool confirm_deletion)
/********************************************************************/
/* free the draw list DrawList and the subhierarchies */
//this is redundant -- use FreeDrawList, a member of SCH_SCREEN
/*
{
EDA_BaseStruct *DrawStruct;

	while (DrawList != NULL)
	{
		DrawStruct = DrawList;
		DrawList = DrawList->Pnext;

		if( DrawStruct->Type() == DRAW_SHEET_STRUCT_TYPE)
		{
			DeleteSubHierarchy((DrawSheetStruct*) DrawStruct, confirm_deletion);
		}

		delete DrawStruct;
	}
}
*/
/********************************************************************/
bool ClearProjectDrawList(SCH_SCREEN * screen, bool confirm_deletion)
/********************************************************************/
/* free the draw list screen->EEDrawList and the subhierarchies
	clear the screen datas (filenames ..)
*/
{
	if ( screen == NULL ) return(TRUE);

	screen->FreeDrawList();

	/* Clear the  screen datas */
	screen->m_ScreenNumber = screen->m_NumberOfScreen = 1;
	screen->m_Title.Empty();
	screen->m_Revision.Empty();
	screen->m_Company.Empty();
	screen->m_Commentaire1.Empty();
	screen->m_Commentaire2.Empty();
	screen->m_Commentaire3.Empty();
	screen->m_Commentaire4.Empty();
	screen->m_Date = GenDate();

	return TRUE;
}

