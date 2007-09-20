/************************************************/
/** gerbview_config.cpp : Gerbview configuration*/
/************************************************/

/* Functions to handle Gerbview configuration */

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "id.h"
#include "hotkeys_basic.h"
#include "hotkeys.h"
#include "gerbview_config.h"

#include "protos.h"

/* Routines Locales */

/* Variables locales */

#define HOTKEY_FILENAME wxT( "gerbview" )


/*************************************************************/
void WinEDA_GerberFrame::Process_Config( wxCommandEvent& event )
/*************************************************************/
{
    int      id = event.GetId();
    wxPoint  pos;
    wxString FullFileName;

    pos    = GetPosition();
    pos.x += 20; pos.y += 20;

    switch( id )
    {
    case ID_COLORS_SETUP:
        DisplayColorSetupFrame( this, pos );
        break;

    case ID_CONFIG_REQ:             // Creation de la fenetre de configuration
    {
        InstallConfigFrame( pos );
        break;
    }

    case ID_PCB_TRACK_SIZE_SETUP:
    case ID_PCB_LOOK_SETUP:
    case ID_OPTIONS_SETUP:
        InstallPcbOptionsFrame( pos, id );
        break;

    case ID_CONFIG_SAVE:
        Update_config();
        break;

    case ID_PREFERENCES_CREATE_CONFIG_HOTKEYS:
        FullFileName  = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );
        FullFileName += HOTKEY_FILENAME;
        FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
        WriteHotkeyConfigFile( FullFileName, s_Gerbview_Hokeys_Descr, true );
        break;

    case ID_PREFERENCES_READ_CONFIG_HOTKEYS:
        Read_Hotkey_Config( this, true );
        break;

    case ID_PREFERENCES_HOTKEY_PATH_IS_HOME:
    case ID_PREFERENCES_HOTKEY_PATH_IS_KICAD:
 		HandleHotheyConfigMenuSelection( this, id );
        break;

    default:
        DisplayError( this, wxT( "WinEDA_GerberFrame::Process_Config internal error" ) );
    }
}


/*****************************************************/
bool Read_Config()
/*****************************************************/

/* lit la configuration, si elle n'a pas deja etee lue
  * 1 - lit gerbview.cnf
  * 2 - si non trouve lit <chemin de gerbview.exe>/gerbview.cnf
  * 3 - si non trouve: init des variables aux valeurs par defaut
 *
  * Retourne un pointeur su le message d'erreur a afficher
 */
{
    g_Prj_Config_Filename_ext = wxT( ".cnf" );
    EDA_Appl->ReadProjectConfig( wxT( "gerbview" ), GROUP, ParamCfgList, FALSE );

    /* Inits autres variables */
    if( ScreenPcb )
        ScreenPcb->SetGrid( TmpGrid );
    if( g_PhotoFilenameExt.IsEmpty() )
        g_PhotoFilenameExt = wxT( ".pho" );
    if( g_DrillFilenameExt.IsEmpty() )
        g_DrillFilenameExt = wxT( ".drl" );
    if( g_PenFilenameExt.IsEmpty() )
        g_PenFilenameExt = wxT( ".pen" );

    return TRUE;
}


/******************************************/
void WinEDA_GerberFrame::Update_config()
/******************************************/

/*
  * creation du fichier de config
 */
{
    wxString FullFileName;
    wxString mask( wxT( "*" ) ),

    g_Prj_Config_Filename_ext = wxT( ".cnf"; )
                                mask += g_Prj_Config_Filename_ext;

    FullFileName = wxT( "gerbview" );
    ChangeFileNameExt( FullFileName, g_Prj_Config_Filename_ext );

    FullFileName = EDA_FileSelector( _( "Save config file" ),
                                     wxEmptyString,             /* Chemin par defaut */
                                     FullFileName,              /* nom fichier par defaut */
                                     g_Prj_Config_Filename_ext, /* extension par defaut */
                                     mask,                      /* Masque d'affichage */
                                     this,
                                     wxFD_SAVE,
                                     TRUE
                                     );
    if( FullFileName.IsEmpty() )
        return;

    /* ecriture de la configuration */
    EDA_Appl->WriteProjectConfig( FullFileName, GROUP, ParamCfgList );
}


/***************************************************************/
bool Read_Hotkey_Config( WinEDA_DrawFrame* frame, bool verbose )
/***************************************************************/

/*
 * Read the hotkey files config for pcbnew and module_edit
 */
{
    wxString FullFileName = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );

    FullFileName += HOTKEY_FILENAME;
    FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
    return frame->ReadHotkeyConfigFile( FullFileName, s_Gerbview_Hokeys_Descr, verbose );
}
