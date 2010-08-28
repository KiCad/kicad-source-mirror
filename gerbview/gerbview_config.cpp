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
#include "dialog_hotkeys_editor.h"

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
    case ID_CONFIG_REQ:
    {
        InstallConfigFrame( pos );
        break;
    }

    case ID_CONFIG_SAVE:
        Update_config();
        break;

   /* Hotkey IDs */
    case ID_PREFERENCES_HOTKEY_EXPORT_CONFIG:
        ExportHotkeyConfigToFile( s_Gerbview_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_IMPORT_CONFIG:
        ImportHotkeyConfigFromFile( s_Gerbview_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_EDITOR:
        InstallHotkeyFrame( this, s_Gerbview_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        // Display current hotkey list for eeschema.
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

