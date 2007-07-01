	/******************************************************/
	/* Files.cp: Lecture / Sauvegarde des fichiers gerber */
	/******************************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"
#include "id.h"


/* Routines locales */
static void LoadDCodeFile(WinEDA_GerberFrame * frame, const wxString & FullFileName, wxDC * DC);


/********************************************************/
void WinEDA_GerberFrame::Files_io(wxCommandEvent& event)
/********************************************************/
/* Gestion generale  des commandes de lecture de fichiers
*/
{
int id = event.GetId();
wxClientDC dc(DrawPanel);

	DrawPanel->CursorOff(&dc);

	switch (id)
		{
		case ID_MENU_LOAD_FILE:
		case ID_LOAD_FILE:
			if ( Clear_Pcb(&dc, TRUE) )
				{
				LoadOneGerberFile(wxEmptyString, &dc, 0);
				}
			break;

		case ID_MENU_INC_LAYER_AND_APPEND_FILE:
		case ID_INC_LAYER_AND_APPEND_FILE:
			{
			int layer = GetScreen()->m_Active_Layer;
			GetScreen()->m_Active_Layer++;
			if( ! LoadOneGerberFile(wxEmptyString, &dc, 0) )
				GetScreen()->m_Active_Layer = layer;
			SetToolbars();
			}
			break;

		case ID_MENU_APPEND_FILE:
		case ID_APPEND_FILE:
			LoadOneGerberFile(wxEmptyString, &dc, 0);
			break;

		case ID_MENU_NEW_BOARD:
		case ID_NEW_BOARD:
			Clear_Pcb(&dc, TRUE);
			Zoom_Automatique(FALSE);
			GetScreen()->SetRefreshReq();
			break;

		case ID_LOAD_FILE_1:
		case ID_LOAD_FILE_2:
		case ID_LOAD_FILE_3:
		case ID_LOAD_FILE_4:
		case ID_LOAD_FILE_5:
		case ID_LOAD_FILE_6:
		case ID_LOAD_FILE_7:
		case ID_LOAD_FILE_8:
		case ID_LOAD_FILE_9:
		case ID_LOAD_FILE_10:
			if ( Clear_Pcb(&dc, TRUE) )
				{
				LoadOneGerberFile(
					GetLastProject(id - ID_LOAD_FILE_1).GetData(),
					&dc, FALSE);
				}
			break;

		case ID_GERBVIEW_LOAD_DRILL_FILE:
			DisplayError(this, _("Not yet available..."));
			break;

		case ID_GERBVIEW_LOAD_DCODE_FILE:
			LoadDCodeFile(this, wxEmptyString, &dc);
			break;

		case ID_SAVE_BOARD:
		case ID_MENU_SAVE_BOARD:
			SaveGerberFile(GetScreen()->m_FileName, &dc);
			break;

		case ID_MENU_SAVE_BOARD_AS:
			SaveGerberFile(wxEmptyString, &dc);
			break;

		default:
			DisplayError(this, wxT("File_io Internal Error") );
			break;
		}

	DrawPanel->MouseToCursorSchema();
	DrawPanel->CursorOn(&dc);
}



/*******************************************************************************************/
int WinEDA_GerberFrame::LoadOneGerberFile(const wxString & FullFileName,
		wxDC * DC, int mode)
/*******************************************************************************************/

/*
 Lecture d'un fichier PCB, le nom etant dans PcbNameBuffer.s
 retourne:
	0 si fichier non lu ( annulation de commande ... )
	1 si OK
*/
{
wxString filename = FullFileName;
wxString path = wxPathOnly(FullFileName);

	ActiveScreen = GetScreen();
	if( filename == wxEmptyString)
	{
		wxString mask = wxT("*") + g_PhotoFilenameExt;
		mask += wxT(";*.gbr;*.lgr;*.ger");
		filename = EDA_FileSelector(_("Gerber files:"),
					path,					/* Chemin par defaut */
					wxEmptyString,					 	/* nom fichier par defaut */
					g_PhotoFilenameExt,			/* extension par defaut */
					mask,					/* Masque d'affichage */
					this,
					0,
					FALSE
					);
		if ( filename == wxEmptyString ) return FALSE;
	}

	GetScreen()->m_FileName = filename;
	wxSetWorkingDirectory(path);
	ChangeFileNameExt(filename,g_PenFilenameExt);

	if ( Read_GERBER_File(DC, GetScreen()->m_FileName, filename) )
		SetLastProject(GetScreen()->m_FileName);

	Zoom_Automatique(FALSE);
	GetScreen()->SetRefreshReq();
	g_SaveTime = time(NULL);

	return(1);
}

/**********************************************************************************************/
static void LoadDCodeFile(WinEDA_GerberFrame * frame, const wxString & FullFileName, wxDC * DC)
/**********************************************************************************************/

/*
 Lecture d'un fichier PCB, le nom etant dans PcbNameBuffer.s
 retourne:
	0 si fichier non lu ( annulation de commande ... )
	1 si OK
*/
{
wxString filename = FullFileName;

	ActiveScreen = frame->GetScreen();

	if( filename == wxEmptyString)
		{
		wxString penfilesmask( wxT("*") );
		penfilesmask += g_PenFilenameExt;
		filename = frame->GetScreen()->m_FileName;
		ChangeFileNameExt(filename,g_PenFilenameExt);
		filename = EDA_FileSelector(_("D CODES files:"),
					wxEmptyString,						/* Chemin par defaut */
					filename, 				/* nom fichier par defaut */
					g_PenFilenameExt,			/* extension par defaut */
					penfilesmask,			/* Masque d'affichage */
					frame,
					0,
					TRUE
					);
		if ( filename == wxEmptyString ) return;
		}

	frame->Read_D_Code_File(filename);
	frame->CopyDCodesSizeToItems();
	frame->GetScreen()->SetRefreshReq();
}



/*******************************************************************************/
bool WinEDA_GerberFrame::SaveGerberFile(const wxString & FullFileName, wxDC * DC)
/*******************************************************************************/
/* Sauvegarde du fichier PCB en format ASCII
*/
{
wxString filename = FullFileName;

	if( filename == wxEmptyString )
		{
		wxString mask( wxT("*"));
		mask += g_PhotoFilenameExt;
		filename = EDA_FileSelector(_("Gerber files:"),
					wxEmptyString,						/* Chemin par defaut */
					GetScreen()->m_FileName,	 	/* nom fichier par defaut */
					g_PhotoFilenameExt,			/* extension par defaut */
					mask,					/* Masque d'affichage */
					this,
					wxFD_SAVE,
					FALSE
					);
			if ( filename.IsEmpty() ) return FALSE;
		}

	GetScreen()->m_FileName = filename;

// TODO

	return TRUE;
}



