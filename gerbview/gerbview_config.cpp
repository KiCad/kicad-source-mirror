/************************************************/
/** gerbview_config.cpp : Gerbview configuration*/
/************************************************/

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
#include "class_board_design_settings.h"

#include "gerbview_config.h"
#include "protos.h"


#define HOTKEY_FILENAME wxT( "gerbview" )


void WinEDA_GerberFrame::Process_Config( wxCommandEvent& event )
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

    case ID_CONFIG_REQ:
    {
        InstallConfigFrame( pos );
        break;
    }

    case ID_CONFIG_SAVE:
        Update_config();
        break;

   /* Hotkey IDs */
    case ID_PREFERENCES_HOTKEY_CREATE_CONFIG:
        FullFileName  = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );
        FullFileName += HOTKEY_FILENAME;
        FullFileName +=  wxT(".");
        FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
        WriteHotkeyConfigFile( FullFileName, s_Gerbview_Hokeys_Descr, true );
        break;

    case ID_PREFERENCES_HOTKEY_READ_CONFIG:
        Read_Hotkey_Config( this, true );
        break;

    case ID_PREFERENCES_HOTKEY_EDIT_CONFIG:
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

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        DisplayHotkeyList( this, s_Gerbview_Hokeys_Descr );
        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_GerberFrame::Process_Config internal error" ) );
    }
}


/* Read configuration, if it has not already read.
 * 1 - bed gerbview.cnf
 * 2 - if no bed is path> gerbview.exe> / gerbview.cnf
 * 3 - If not found: init variables to default values
 */
bool Read_Config()
{
    wxGetApp().ReadProjectConfig( wxT( "gerbview.cnf" ), GROUP, ParamCfgList,
                                  FALSE );

    if( g_PhotoFilenameExt.IsEmpty() )
        g_PhotoFilenameExt = wxT( "pho" );
    if( g_DrillFilenameExt.IsEmpty() )
        g_DrillFilenameExt = wxT( "drl" );
    if( g_PenFilenameExt.IsEmpty() )
        g_PenFilenameExt = wxT( "pen" );

    return TRUE;
}


void WinEDA_GerberFrame::Update_config()
{
    wxFileName fn = wxFileName( wxEmptyString, wxT( "gerbview" ),
                                GerbviewProjectFileExt );

    wxFileDialog dlg( this, _( "Save GerbView Project File" ), wxEmptyString,
                      fn.GetFullName(), GerbviewProjectFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxGetApp().WriteProjectConfig( dlg.GetPath(), GROUP, ParamCfgList );
}


/*
 * Read the hotkey files config for pcbnew and module_edit
 */
bool Read_Hotkey_Config( WinEDA_DrawFrame* frame, bool verbose )
{
    wxString FullFileName =
        ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );

    FullFileName += HOTKEY_FILENAME;
    FullFileName +=  wxT(".");
    FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
    return frame->ReadHotkeyConfigFile( FullFileName,
                                        s_Gerbview_Hokeys_Descr,
                                        verbose );
}
