/***********************************/
/** pcbcfg() : configuration      **/
/***********************************/

/* lit ou met a jour la configuration de PCBNEW */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbplot.h"
#include "pcbcfg.h"
#include "worksheet.h"
#include "pcbnew_id.h"
#include "hotkeys.h"
#include "protos.h"
//#include "dialog_copper_layers_setup.h"

/* Routines Locales */

/* Variables locales */

#define HOTKEY_FILENAME wxT( "pcbnew" )

/***********************************************************/
void WinEDA_PcbFrame::Process_Config( wxCommandEvent& event )
/***********************************************************/
{
    int        id = event.GetId();
    wxPoint    pos;

    wxClientDC dc( DrawPanel );

    wxFileName fn;

    DrawPanel->PrepareGraphicContext( &dc );

    pos    = GetPosition();
    pos.x += 20;
    pos.y += 20;

    switch( id )
    {
    case ID_COLORS_SETUP:
        DisplayColorSetupFrame( this, pos );
        break;

/*
    case ID_PCB_COPPER_LAYERS_SETUP:
    {
        DIALOG_COPPER_LAYERS_SETUP dialog( this );
        dialog.ShowModal();
    }
        break;
*/

    case ID_PCB_LAYERS_SETUP:
        DisplayDialogLayerSetup( this );
        break;

    case ID_CONFIG_REQ:             // Creation de la fenetre de configuration
        InstallConfigFrame( pos );
        break;

    case ID_PCB_TRACK_SIZE_SETUP:
    case ID_OPTIONS_SETUP:
    case ID_PCB_DRAWINGS_WIDTHS_SETUP:
        InstallPcbOptionsFrame( pos, &dc, id );
        break;

    case ID_PCB_PAD_SETUP:
        InstallPadOptionsFrame( NULL, NULL, pos );
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
    case ID_PREFERENCES_CREATE_CONFIG_HOTKEYS:
        fn.SetPath( ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice ) );
        fn.SetName( HOTKEY_FILENAME );
        fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );
        WriteHotkeyConfigFile( fn.GetFullPath(), s_Pcbnew_Editor_Hokeys_Descr,
                               true );
        break;

    case ID_PREFERENCES_READ_CONFIG_HOTKEYS:
        Read_Hotkey_Config( this, true );
        break;

    case ID_PREFERENCES_EDIT_CONFIG_HOTKEYS:
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
    FullFileName += wxT(".");
    FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
    return frame->ReadHotkeyConfigFile( FullFileName,
                                        s_Pcbnew_Editor_Hokeys_Descr,
                                        verbose );
}


/**************************************************************************/
bool Read_Config( const wxString& projectFileName )
/*************************************************************************/

/* lit la configuration, si elle n'a pas deja ete lue
 * 1 - lit <nom fichier brd>.pro
 * 2 - si non trouve lit <chemin de *.exe>/kicad.pro
 * 3 - si non trouve: init des variables aux valeurs par defaut
 *
 * Retourne TRUE si lu, FALSE si config non lue ou non modifiée
 */
{
    wxFileName fn = projectFileName;
    int      ii;

    if( fn.GetExt() != ProjectFileExtension )
        fn.SetExt( ProjectFileExtension );

    wxGetApp().RemoveLibraryPath( g_UserLibDirBuffer );

    /* Init des valeurs par defaut */
    g_LibName_List.Clear();

    wxGetApp().ReadProjectConfig( fn.GetFullPath(),
                                  GROUP, ParamCfgList, FALSE );

    /* User library path takes precedent over default library search paths. */
    wxGetApp().InsertLibraryPath( g_UserLibDirBuffer, 1 );

    /* Reset the ITEM*NOT*SHOW flag when loading a new config
     *  Because it could creates SERIOUS mistakes for the user,
     * if some items are not visible after loading a board...
     */
    for( ii = 0; ii < LAYER_COUNT; ii++ )
        g_DesignSettings.SetLayerVisibility( ii, true );

    DisplayOpt.Show_Modules_Cmp = true;
    DisplayOpt.Show_Modules_Cu = true;

    // These parameters could be left in their previous state, or resetted
    // Comment or uncomment to keep or reset this option after loading a board

    g_DesignSettings.SetElementVisibility( MODULE_TEXT_NOV_VISIBLE, true );
    g_DesignSettings.SetElementVisibility( MODULE_TEXT_CMP_VISIBLE, true );
    g_DesignSettings.SetElementVisibility( MODULE_TEXT_CU_VISIBLE, true );
    g_DesignSettings.SetElementVisibility( PAD_CMP_VISIBLE, true );
    g_DesignSettings.SetElementVisibility( PAD_CU_VISIBLE, true );
    g_DesignSettings.SetElementVisibility( VIA_THROUGH_VISIBLE, true );
    g_DesignSettings.SetElementVisibility( VIA_BLIND_BURIED_VISIBLE, true );
    g_DesignSettings.SetElementVisibility( VIA_MICROVIA_VISIBLE, true );
    g_DesignSettings.SetElementVisibility( ANCHOR_VISIBLE, true );

    DisplayOpt.DisplayPadNoConn = true;
    return TRUE;
}


/**********************************************************/
void WinEDA_PcbFrame::Update_config( wxWindow* displayframe )
/***********************************************************/
/* enregistrement de la config */
{
    wxFileName fn;

    fn = GetScreen()->m_FileName;
    fn.SetExt( ProjectFileExtension );

    wxFileDialog dlg( this, _( "Save Project File" ), fn.GetPath(),
                      fn.GetFullName(), ProjectFileWildcard,
                      wxFD_SAVE | wxFD_CHANGE_DIR );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    /* ecriture de la configuration */
    wxGetApp().WriteProjectConfig( fn.GetFullPath(), wxT( "/pcbnew" ),
                                   ParamCfgList );
}
