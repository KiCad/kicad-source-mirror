	/***********************************/
	/** pcbcfg() : configuration	  **/
	/***********************************/

/* lit ou met a jour la configuration de PCBNEW */

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "pcbcfg.h"
#include "worksheet.h"
#include "id.h"
#include "hotkeys_basic.h"
#include "hotkeys.h"

#include "protos.h"

/* Routines Locales */

/* Variables locales */


/***********************************************************/
void WinEDA_PcbFrame::Process_Config(wxCommandEvent& event)
/***********************************************************/
{
int id = event.GetId();
wxPoint pos;
wxClientDC dc(DrawPanel);
wxString FullFileName;

	DrawPanel->PrepareGraphicContext(&dc);

	pos = GetPosition();
	pos.x += 20; pos.y += 20;

	switch( id )
		{
		case ID_COLORS_SETUP :
			DisplayColorSetupFrame(this, pos);
			break;

		case ID_CONFIG_REQ :		// Creation de la fenetre de configuration
			InstallConfigFrame(pos);
			break;

		case ID_PCB_TRACK_SIZE_SETUP:
		case ID_PCB_LOOK_SETUP:
		case ID_OPTIONS_SETUP:
		case ID_PCB_DRAWINGS_WIDTHS_SETUP:
			InstallPcbOptionsFrame(pos, &dc, id);
			break;

		case ID_PCB_PAD_SETUP:
			InstallPadOptionsFrame( NULL, NULL, pos);
			break;

		case ID_CONFIG_SAVE:
			Update_config(this);
			break;

		case ID_CONFIG_READ:
			FullFileName = GetScreen()->m_FileName.AfterLast('/');
			ChangeFileNameExt(FullFileName, g_Prj_Config_Filename_ext);
			FullFileName = EDA_FileSelector(_("Read config file"),
					wxPathOnly(GetScreen()->m_FileName),/* Chemin par defaut */
					FullFileName,			/* nom fichier par defaut */
					g_Prj_Config_Filename_ext,	/* extension par defaut */
					FullFileName,			/* Masque d'affichage */
					this,
					wxFD_OPEN,
					TRUE				/* ne change pas de repertoire courant */
					);
			if ( FullFileName.IsEmpty()) break;
			if ( ! wxFileExists(FullFileName) )
				{
				wxString msg;
				msg.Printf(_("File %s not found"), FullFileName.GetData());
				DisplayError(this, msg); break;
				}
			Read_Config(FullFileName );
			break;

		case ID_PREFERENCES_CREATE_CONFIG_HOTKEYS:
			FullFileName = DEFAULT_HOTKEY_FILENAME_PATH;
			FullFileName += wxT("pcbnew");
			FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
			WriteHotkeyConfigFile(FullFileName, s_Pcbnew_Editor_Hokeys_Descr, true);
			break;

		case ID_PREFERENCES_READ_CONFIG_HOTKEYS:
			Read_Hotkey_Config( this, true);
			break;

		default:
			DisplayError(this, wxT("WinEDA_PcbFrame::Process_Config internal error"));
		}
}


/***************************************************************/
bool Read_Hotkey_Config( WinEDA_DrawFrame * frame, bool verbose )
/***************************************************************/
/*
 * Read the hotkey files config for pcbnew and module_edit
*/
{
	wxString FullFileName = DEFAULT_HOTKEY_FILENAME_PATH;
	FullFileName += wxT("pcbnew");
	FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
	return frame->ReadHotkeyConfigFile(FullFileName, s_Pcbnew_Editor_Hokeys_Descr, verbose);
}


/**************************************************************************/
bool Read_Config(const wxString & project_name)
/*************************************************************************/
/* lit la configuration, si elle n'a pas deja ete lue
	1 - lit <nom fichier brd>.pro
	2 - si non trouve lit <chemin de *.exe>/kicad.pro
	3 - si non trouve: init des variables aux valeurs par defaut

	Retourne TRUE si lu, FALSE si config non lue ou non modifiée
*/
{
wxString FullFileName;
int ii;

	g_Prj_Config_Filename_ext = wxT(".pro");
	FullFileName = project_name;
	ChangeFileNameExt(FullFileName, g_Prj_Config_Filename_ext);

	/* Init des valeurs par defaut */
	 g_LibName_List.Clear();

	EDA_Appl->ReadProjectConfig( FullFileName,
		GROUP, ParamCfgList, FALSE);

	/* Traitement des variables particulieres: */

	SetRealLibraryPath( wxT("modules") );

	if (ScreenPcb)
	{
		ScreenPcb->m_Diviseur_Grille = Pcbdiv_grille;
		ScreenPcb->m_UserGrid = g_UserGrid;
		ScreenPcb->m_UserGridUnit = g_UserGrid_Unit;
	}

	g_DesignSettings.m_TrackWidhtHistory[0] = g_DesignSettings.m_CurrentTrackWidth;
	g_DesignSettings.m_ViaSizeHistory[0] = g_DesignSettings.m_CurrentViaSize;
	for ( ii = 1; ii < HIST0RY_NUMBER; ii++)
	{
		g_DesignSettings.m_TrackWidhtHistory[ii] = 0;
		g_DesignSettings.m_ViaSizeHistory[ii] = 0;
	}

	return TRUE;
}

/**********************************************************/
void WinEDA_PcbFrame::Update_config(wxWindow * displayframe)
/***********************************************************/
/* enregistrement de la config */
{
wxString FullFileName;
wxString mask;

	mask = wxT("*") + g_Prj_Config_Filename_ext;
	FullFileName = GetScreen()->m_FileName.AfterLast('/');
	ChangeFileNameExt(FullFileName, g_Prj_Config_Filename_ext);

	FullFileName = EDA_FileSelector(_("Save config file"),
					wxPathOnly(GetScreen()->m_FileName),	/* Chemin par defaut */
					FullFileName,		/* nom fichier par defaut */
					g_Prj_Config_Filename_ext,	/* extension par defaut */
					mask,			/* Masque d'affichage */
					displayframe,
					wxFD_SAVE,
					TRUE
					);
	if ( FullFileName.IsEmpty() ) return;

	Pcbdiv_grille = GetScreen()->m_Diviseur_Grille;

	/* ecriture de la configuration */
	EDA_Appl->WriteProjectConfig(FullFileName, wxT("/pcbnew"), ParamCfgList);
}


