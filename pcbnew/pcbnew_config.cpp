/****************************************/
/** pcbnew_config.cpp : configuration  **/
/****************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"
#include "pcbplot.h"
#include "pcbnew_config.h"
#include "worksheet.h"
#include "pcbnew_id.h"
#include "hotkeys.h"
#include "protos.h"

#include "dialog_mask_clearance.h"
#include "dialog_general_options.h"


#define HOTKEY_FILENAME wxT( "pcbnew" )


void WinEDA_PcbFrame::Process_Config( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxPoint    pos;

    wxFileName fn;

    pos    = GetPosition();
    pos.x += 20;
    pos.y += 20;

    switch( id )
    {
    case ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG:
        if( m_OptionsToolBar )
        {   //This command is same as the Options Vertical Toolbar
            // tool Show/hide layers manager
            bool state = m_OptionsToolBar->GetToolState(ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR);
            m_OptionsToolBar->ToggleTool(ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR, !state);
            wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED,
                                  ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR );
            wxPostEvent( this, event );
        }
        break;

    case ID_PCB_LAYERS_SETUP:
        DisplayDialogLayerSetup( this );
        break;

    case ID_CONFIG_REQ:
        InstallConfigFrame( pos );
        break;

    case ID_PCB_MASK_CLEARANCE:
        {
            DIALOG_PADS_MASK_CLEARANCE dlg( this );
            dlg.ShowModal();
        }
        break;

    case ID_OPTIONS_SETUP:
        {
            Dialog_GeneralOptions dlg( this );
            dlg.ShowModal();
        }
        break;

    case ID_PCB_DRAWINGS_WIDTHS_SETUP:
        InstallPcbOptionsFrame( id );
        break;

    case ID_PCB_PAD_SETUP:
        InstallPadOptionsFrame( NULL );
        break;

    case ID_CONFIG_SAVE:
        Update_config( this );
        break;

    case ID_CONFIG_READ:
    {
        fn = GetScreen()->m_FileName;
        fn.SetExt( ProjectFileExtension );

        wxFileDialog dlg( this, _( "Read Project File" ), fn.GetPath(),
                          fn.GetFullName(), ProjectFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            break;

        if( !wxFileExists( dlg.GetPath() ) )
        {
            wxString msg;
            msg.Printf( _( "File %s not found" ), GetChars( dlg.GetPath() ) );
            DisplayError( this, msg );
            break;
        }

        Read_Config( dlg.GetPath() );
        break;
    }
    case ID_PREFERENCES_HOTKEY_CREATE_CONFIG:
        fn.SetPath( ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice ) );
        fn.SetName( HOTKEY_FILENAME );
        fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );
        WriteHotkeyConfigFile( fn.GetFullPath(), s_Pcbnew_Editor_Hokeys_Descr,
                               true );
        break;

    case ID_PREFERENCES_HOTKEY_READ_CONFIG:
        Read_Hotkey_Config( this, true );
        break;

    case ID_PREFERENCES_HOTKEY_EDIT_CONFIG:
    {
        fn.SetPath( ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice ) );
        fn.SetName( HOTKEY_FILENAME );
        fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );

        wxString editorname = wxGetApp().GetEditorName();
        if( !editorname.IsEmpty() )
            ExecuteFile( this, editorname, QuoteFullPath( fn ) );
        break;
    }

    case ID_PREFERENCES_HOTKEY_PATH_IS_HOME:
    case ID_PREFERENCES_HOTKEY_PATH_IS_KICAD:
        HandleHotkeyConfigMenuSelection( this, id );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        DisplayHotkeyList( this, s_Board_Editor_Hokeys_Descr );
        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_PcbFrame::Process_Config internal error" ) );
    }
}


/*
 * Read the hotkey files config for pcbnew and module_edit
 */
bool Read_Hotkey_Config( WinEDA_DrawFrame* frame, bool verbose )
{
    wxString FullFileName = ReturnHotkeyConfigFilePath(
        g_ConfigFileLocationChoice );

    FullFileName += HOTKEY_FILENAME;
    FullFileName += wxT(".");
    FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
    return frame->ReadHotkeyConfigFile( FullFileName,
                                        s_Pcbnew_Editor_Hokeys_Descr,
                                        verbose );
}


/** Function Read_Config
 * Read the project configuration file
 * @param projectFileName = the config filename
 *  if not found use kicad.pro
 *  if not found : initialize default values
 * @return true if the current config is modified, false if no change
 */
bool WinEDA_PcbFrame::Read_Config( const wxString& projectFileName )
{
    wxFileName fn = projectFileName;

    if( fn.GetExt() != ProjectFileExtension )
        fn.SetExt( ProjectFileExtension );

    wxGetApp().RemoveLibraryPath( g_UserLibDirBuffer );

    /* Initialize default values. */
    g_LibName_List.Clear();

    wxGetApp().ReadProjectConfig( fn.GetFullPath(),
                                  GROUP, ParamCfgList, FALSE );

    /* User library path takes precedent over default library search paths. */
    wxGetApp().InsertLibraryPath( g_UserLibDirBuffer, 1 );

    /* Reset the items visibility flag when loading a new config
     *  Because it could creates SERIOUS mistakes for the user,
     * if some items are not visible after loading a board...
     */
    g_DesignSettings.SetVisibleAlls( );

    DisplayOpt.DisplayPadNoConn = true;
    return TRUE;
}


void WinEDA_PcbFrame::Update_config( wxWindow* displayframe )
{
    wxFileName fn;

    fn = GetScreen()->m_FileName;
    fn.SetExt( ProjectFileExtension );

    wxFileDialog dlg( this, _( "Save Project File" ), fn.GetPath(),
                      fn.GetFullName(), ProjectFileWildcard,
                      wxFD_SAVE | wxFD_CHANGE_DIR );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxGetApp().WriteProjectConfig( fn.GetFullPath(), wxT( "/pcbnew" ),
                                   ParamCfgList );
}
