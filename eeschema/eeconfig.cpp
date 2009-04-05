/******************************************************/
/** eeconfig.cpp : routines et menus de configuration */
/*******************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "id.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"
#include "eeconfig.h"
#include "worksheet.h"
#include "hotkeys.h"


/* Variables locales */


#define HOTKEY_FILENAME wxT( "eeschema" )

/*********************************************************************/
void WinEDA_SchematicFrame::Process_Config( wxCommandEvent& event )
/*********************************************************************/
{
    int        id = event.GetId();
    wxPoint    pos;
    wxFileName fn;

    wxGetMousePosition( &pos.x, &pos.y );

    pos.y += 5;

    switch( id )
    {
    case ID_COLORS_SETUP:
        DisplayColorSetupFrame( this, pos );
        break;

    case ID_CONFIG_REQ:             // Creation de la fenetre de configuration
        InstallConfigFrame( pos );
        break;

    case ID_OPTIONS_SETUP:
        DisplayOptionFrame( this, pos );
        DrawPanel->Refresh( TRUE );   // Redraw, because grid settings may have changed.
        break;

    case ID_CONFIG_SAVE:
        Save_Config( this );
        break;

    case ID_CONFIG_READ:
    {
        fn = g_RootSheet->m_AssociatedScreen->m_FileName;
        fn.SetExt( ProjectFileExtension );

        wxFileDialog dlg( this, _( "Read Project File" ), fn.GetPath(),
                          fn.GetFullName(), ProjectFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            break;

        Read_Config( fn.GetFullPath(), TRUE );
    }
    break;

    case ID_PREFERENCES_CREATE_CONFIG_HOTKEYS:
        fn = wxFileName( ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice ),
                         HOTKEY_FILENAME,
                         DEFAULT_HOTKEY_FILENAME_EXT );
        WriteHotkeyConfigFile( fn.GetFullPath(), s_Eeschema_Hokeys_Descr, true );
        break;

    case ID_PREFERENCES_READ_CONFIG_HOTKEYS:
        Read_Hotkey_Config( this, true );
        break;

    case ID_PREFERENCES_EDIT_CONFIG_HOTKEYS:
    {
        fn = wxFileName( ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice ),
                         HOTKEY_FILENAME, DEFAULT_HOTKEY_FILENAME_EXT );
        wxString editorname = wxGetApp().GetEditorName();
        if( !editorname.IsEmpty() )
            ExecuteFile( this, editorname, QuoteFullPath( fn ) );
    }
    break;

    case ID_PREFERENCES_HOTKEY_PATH_IS_HOME:
    case ID_PREFERENCES_HOTKEY_PATH_IS_KICAD:
        HandleHotkeyConfigMenuSelection( this, id );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:           // Display Current hotkey list for eeschema
        DisplayHotkeyList( this, s_Schematic_Hokeys_Descr );
        break;

    default:
        DisplayError( this, wxT( "WinEDA_SchematicFrame::Process_Config " \
                                 "internal error" ) );
    }
}


/***************************************************************/
bool Read_Hotkey_Config( WinEDA_DrawFrame* frame, bool verbose )
/***************************************************************/

/*
 * Read the hotkey files config for eeschema and libedit
 */
{
    wxString FullFileName = ReturnHotkeyConfigFilePath(
        g_ConfigFileLocationChoice );

    FullFileName += HOTKEY_FILENAME;
    FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
    frame->ReadHotkeyConfigFile( FullFileName,
                                 s_Eeschema_Hokeys_Descr,
                                 verbose );

    return TRUE;
}


/***********************************************************************/
bool Read_Config( const wxString& CfgFileName, bool ForceRereadConfig )
/***********************************************************************/

/* lit la configuration, si elle n'a pas deja ete lue
 * 1 - lit <nom fichier root>.pro
 * 2 - si non trouve lit <chemin des binaires>../template/kicad.pro
 * 3 - si non trouve: init des variables aux valeurs par defaut
 *
 * Retourne TRUE si lu, FALSE si config non lue
 */
{
    wxFileName              fn;
    bool                    IsRead = TRUE;
    wxArrayString           liblist_tmp = g_LibName_List;
    WinEDA_SchematicFrame*  frame;

    frame = (WinEDA_SchematicFrame*)wxGetApp().GetTopWindow();

    if( CfgFileName.IsEmpty() )
        fn = g_RootSheet->m_AssociatedScreen->m_FileName;
    else
        fn = CfgFileName;
    g_LibName_List.Clear();

    /* Change the schematic file extension (.sch) to the project file
     * extension (.pro). */
    fn.SetExt( ProjectFileExtension );

    if( wxGetApp().GetLibraryPathList().Index( g_UserLibDirBuffer ) != wxNOT_FOUND )
    {
        wxLogDebug( wxT( "Removing path <%s> to library path search list." ),
                    g_UserLibDirBuffer.c_str() );
        wxGetApp().GetLibraryPathList().Remove( g_UserLibDirBuffer );
    }

    if( !wxGetApp().ReadProjectConfig( fn.GetFullPath(), GROUP, ParamCfgList,
                                       ForceRereadConfig ? FALSE : TRUE ) )
    {
        g_LibName_List = liblist_tmp;
        IsRead = FALSE;
    }

    if( wxFileName::DirExists( g_UserLibDirBuffer )
        && wxGetApp().GetLibraryPathList().Index( g_UserLibDirBuffer ) == wxNOT_FOUND )
    {
        wxLogDebug( wxT( "Adding path <%s> to library path search list." ),
                    g_UserLibDirBuffer.c_str() );
        wxGetApp().GetLibraryPathList().Add( g_UserLibDirBuffer );
    }

    // If the list is void, load the libraries "power.lib" and "device.lib"
    if( g_LibName_List.GetCount() == 0 )
    {
        g_LibName_List.Add( wxT( "power" ) );
        g_LibName_List.Add( wxT( "device" ) );
    }

    if( frame )
    {
        frame->SetDrawBgColor( g_DrawBgColor );
    }

    LoadLibraries( frame );

    return IsRead;
}


/****************************************************************/
void WinEDA_SchematicFrame::Save_Config( wxWindow* displayframe )
/***************************************************************/
{
    wxFileName fn;

    fn = g_RootSheet->m_AssociatedScreen->m_FileName  /*ConfigFileName*/;
    fn.SetExt( ProjectFileExtension );

    wxFileDialog dlg( this, _( "Save Project Settings" ), wxGetCwd(),
                      fn.GetFullName(), ProjectFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    /* ecriture de la configuration */
    wxGetApp().WriteProjectConfig( dlg.GetPath(), GROUP, ParamCfgList );
}


/*
 * Load the EESchema configuration parameters.
 */
void WinEDA_SchematicFrame::LoadSettings()
{
    WinEDA_DrawFrame::LoadSettings();
}


/*
 * Save the EESchema configuration parameters.
 */
void WinEDA_SchematicFrame::SaveSettings()
{
    WinEDA_DrawFrame::SaveSettings();
}
