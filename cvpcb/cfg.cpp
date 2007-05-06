	/***************************************/
	/** cfg.cpp : configuration de CVPCB  **/
	/***************************************/

/* lit ou met a jour la configuration de CVPCB */

#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "cvpcb.h"
#include "protos.h"

#include "cfg.h"

/* Routines Locales */
/**/


/**************************************************/
void Read_Config( const wxString & FileName )
/**************************************************/
/* lit la configuration
	1 - lit cvpcb.cnf
	2 - si non trouve lit <chemin de cvpcb.exe>/cvpcb.cnf
	3 - si non trouve: init des variables aux valeurs par defaut

Remarque:
	le chemin de l'executable cvpcb.exe doit etre dans BinDir
*/
{
wxString FullFileName = FileName ;

	/* Init des valeurs par defaut */
	g_LibName_List.Clear();
	g_ListName_Equ.Clear();

	EDA_Appl->ReadProjectConfig(FullFileName,
		GROUP, ParamCfgList, FALSE);

	if ( PkgInExtBuffer.IsEmpty() ) PkgInExtBuffer = wxT(".pkg");
	if ( NetInExtBuffer.IsEmpty() ) NetInExtBuffer = wxT(".net"),

	/* Inits autres variables */
	SetRealLibraryPath( wxT("modules"));
}


/************************************************************/
void WinEDA_CvpcbFrame::Update_Config(wxCommandEvent& event)
/************************************************************/
/* fonction relai d'appel a Save_Config,
	la vraie fonction de sauvegarde de la config
*/
{
	Save_Config(this);
}

/************************************/
void Save_Config(wxWindow * parent)
/************************************/
/* enregistrement de la config */
{
wxString path, FullFileName;
wxString mask( wxT("*"));
	
	FullFileName = FFileName;
	ChangeFileNameExt( FullFileName, g_Prj_Config_Filename_ext );

	path = wxGetCwd();
	FullFileName = EDA_FileSelector(_("Save config file"),
					path,				/* Chemin par defaut */
					FullFileName,		/* nom fichier par defaut */
					g_Prj_Config_Filename_ext,	/* extension par defaut */
					mask,				/* Masque d'affichage */
					parent,
					wxFD_SAVE,
					TRUE
					);
	if ( FullFileName.IsEmpty()) return;

	/* ecriture de la configuration */
	EDA_Appl->WriteProjectConfig(FullFileName, GROUP, ParamCfgList);
}

