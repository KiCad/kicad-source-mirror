/************************************************/
/** gerbview_config.cpp : Gerbview configuration*/
/************************************************/

/* Functions to handle Gerbview configuration */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "pcbcommon.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "hotkeys.h"

#include "gerbview_config.h"
#include "protos.h"


#define HOTKEY_FILENAME wxT( "gerbview" )


const wxString GerbviewProjectFileExt( wxT( "cnf" ) );
const wxString GerbviewProjectFileWildcard( _( "GerbView project files (.cnf)|*.cnf" ) );


/*************************************************************/
void WinEDA_GerberFrame::Process_Config( wxCommandEvent& event )
/*************************************************************/
{
    int      id = event.GetId();
    wxPoint  pos;
    wxString FullFileName;

    pos    = GetPosition();
    pos.x += 20;
    pos.y += 20;

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
    case ID_PCB_DISPLAY_OPTIONS_SETUP:
    case ID_OPTIONS_SETUP:
        InstallPcbOptionsFrame( pos, id );
        break;

    case ID_CONFIG_SAVE:
        Update_config();
        break;

    case ID_PREFERENCES_CREATE_CONFIG_HOTKEYS:
        FullFileName  = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );
        FullFileName += HOTKEY_FILENAME;
        FullFileName +=  wxT(".");
        FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
        WriteHotkeyConfigFile( FullFileName, s_Gerbview_Hokeys_Descr, true );
        break;

    case ID_PREFERENCES_READ_CONFIG_HOTKEYS:
        Read_Hotkey_Config( this, true );
        break;

    case ID_PREFERENCES_EDIT_CONFIG_HOTKEYS:
    {
        FullFileName  = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );
        FullFileName += HOTKEY_FILENAME;
        FullFileName +=  wxT(".");
        FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
        AddDelimiterString( FullFileName );
        wxString editorname = wxGetApp().GetEditorName();
        if( !editorname.IsEmpty() )
            ExecuteFile( this, editorname, FullFileName );
    }
    break;

    case ID_PREFERENCES_HOTKEY_PATH_IS_HOME:
    case ID_PREFERENCES_HOTKEY_PATH_IS_KICAD:
        HandleHotkeyConfigMenuSelection( this, id );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:           // Display Current hotkey list for gerbview
        DisplayHotkeyList( this, s_Gerbview_Hokeys_Descr );
        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_GerberFrame::Process_Config internal error" ) );
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
    wxGetApp().ReadProjectConfig( wxT( "gerbview.cnf" ), GROUP, ParamCfgList,
                                  FALSE );

    /* Inits autres variables */
    if( g_PhotoFilenameExt.IsEmpty() )
        g_PhotoFilenameExt = wxT( "pho" );
    if( g_DrillFilenameExt.IsEmpty() )
        g_DrillFilenameExt = wxT( "drl" );
    if( g_PenFilenameExt.IsEmpty() )
        g_PenFilenameExt = wxT( "pen" );

    return TRUE;
}


/******************************************/
void WinEDA_GerberFrame::Update_config()
/******************************************/

/*
 * creation du fichier de config
 */
{
    wxFileName fn = wxFileName( wxEmptyString, wxT( "gerbview" ),
                                GerbviewProjectFileExt );

    wxFileDialog dlg( this, _( "Save GerbView Project File" ), wxEmptyString,
                      fn.GetFullName(), GerbviewProjectFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    /* ecriture de la configuration */
    wxGetApp().WriteProjectConfig( dlg.GetPath(), GROUP, ParamCfgList );
}


/***************************************************************/
bool Read_Hotkey_Config( WinEDA_DrawFrame* frame, bool verbose )
/***************************************************************/

/*
 * Read the hotkey files config for pcbnew and module_edit
 */
{
    wxString FullFileName = ReturnHotkeyConfigFilePath(
        g_ConfigFileLocationChoice );

    FullFileName += HOTKEY_FILENAME;
    FullFileName +=  wxT(".");
    FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
    return frame->ReadHotkeyConfigFile( FullFileName,
                                        s_Gerbview_Hokeys_Descr,
                                        verbose );
}
