	/****************************/
	/*	EESCHEMA - files-io.cpp	*/
	/****************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"
#include "id.h"

/* Fonctions locales */


/****************************************************************/
void WinEDA_SchematicFrame::Save_File(wxCommandEvent& event)
/****************************************************************/
/* Commands to save shepatic project or the current page.
*/
{
int id = event.GetId();

	switch (id)
	{
		case ID_SAVE_PROJECT: /* Update Schematic File */
			SaveProject(this);
			break;

		case ID_SAVE_ONE_SHEET: /* Update Schematic File */
			SaveEEFile(NULL, FILE_SAVE_AS);
			break;

		case ID_SAVE_ONE_SHEET_AS: /* Save EED (new name) */
			SaveEEFile(NULL, FILE_SAVE_NEW);
			break;

		default: DisplayError(this, wxT("WinEDA_SchematicFrame::Save_File Internal Error"));
			break;
	}
}


/******************************************************************************************/
bool WinEDA_SchematicFrame::LoadOneSheet(SCH_SCREEN * screen, const wxString & filename)
/******************************************************************************************/
{
wxString FullFileName = filename;
	
	if( screen->EEDrawList != NULL )
	{
		if( !IsOK(this, _("Clear SubHierarchy ?") ) ) return FALSE;
	}

	if( FullFileName.IsEmpty() )
	{
		wxString mask;
		mask = wxT("*") + g_SchExtBuffer;
		FullFileName = EDA_FileSelector( _("Schematic files:"),
					wxEmptyString,		  	/* default path */
					screen->m_FileName,		/* default filename */
					g_SchExtBuffer,		  	/* extension par defaut */
					mask,					/* Masque d'affichage */
					this,
					wxFD_OPEN,
					FALSE
					);
		if ( FullFileName.IsEmpty() ) return FALSE;
	}

	ClearProjectDrawList(screen, TRUE);

	screen->m_FileName = FullFileName;
	LoadOneEEFile(screen, FullFileName);
	screen->SetModify();

	if ( GetScreen() == screen ) Refresh(TRUE);
	return TRUE;
}

/****************************************************/
void SaveProject(WinEDA_SchematicFrame * frame)
/****************************************************/
/* Sauvegarde toutes les feuilles du projet
	et crée une librairie archive des composants, de nom <root_name>.chche.lib
*/
{
SCH_SCREEN * screen_tmp;
wxString LibArchiveFileName;
	
	if ( frame == NULL) return;

	screen_tmp = frame->GetScreen();

	EDA_ScreenList ScreenList(NULL);
	for ( ActiveScreen = ScreenList.GetFirst(); ActiveScreen != NULL; ActiveScreen = ScreenList.GetNext() )
	{
		frame->m_CurrentScreen = ActiveScreen;
		frame->SaveEEFile( NULL, FILE_SAVE_AS);
	}

	frame->m_CurrentScreen = ActiveScreen = screen_tmp;

	/* Creation du fichier d'archivage composants en repertoire courant */
	LibArchiveFileName = MakeFileName(wxEmptyString,ScreenSch->m_FileName,wxEmptyString);
	ChangeFileNameExt(LibArchiveFileName, wxEmptyString);
	/* mise a jour extension  */
	LibArchiveFileName += wxT(".cache") + g_LibExtBuffer;
	LibArchive(frame, LibArchiveFileName);

}

/************************/
int CountCmpNumber(void)
/************************/
/* Routine retournant le nombre de composants dans le schema,
powers non comprises */
{
BASE_SCREEN * Window;
EDA_BaseStruct *Phead;
int Nb = 0;

	Window = ScreenSch ;
	while( Window )
	{
		for( Phead=Window->EEDrawList; Phead != NULL; Phead=Phead->Pnext)
		{
			if (Phead->m_StructType == DRAW_LIB_ITEM_STRUCT_TYPE)
			{
				DrawPartStruct * Cmp = (DrawPartStruct *) Phead;
				if ( Cmp->m_Field[VALUE].m_Text.GetChar(0) != '#' ) Nb++;
			}
		}
		Window = (BASE_SCREEN*)Window->Pnext;
	}

	return(Nb);
}
