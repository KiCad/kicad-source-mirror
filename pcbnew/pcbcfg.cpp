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
#include "hotkeys.h"

#include "protos.h"

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

    wxString   FullFileName;

    DrawPanel->PrepareGraphicContext( &dc );

    pos    = GetPosition();
    pos.x += 20;
    pos.y += 20;

    switch( id )
    {
    case ID_COLORS_SETUP:
        DisplayColorSetupFrame( this, pos );
        break;

    case ID_CONFIG_REQ:             // Creation de la fenetre de configuration
        InstallConfigFrame( pos );
        break;

    case ID_PCB_TRACK_SIZE_SETUP:
    case ID_PCB_LOOK_SETUP:
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
        FullFileName = GetScreen()->m_FileName.AfterLast( '/' );
        ChangeFileNameExt( FullFileName, g_Prj_Config_Filename_ext );
        FullFileName = EDA_FileSelector( _( "Read config file" ),
                                         wxPathOnly( GetScreen()->m_FileName ), /* Chemin par defaut */
                                         FullFileName,                          /* nom fichier par defaut */
                                         g_Prj_Config_Filename_ext,             /* extension par defaut */
                                         FullFileName,                          /* Masque d'affichage */
                                         this,
                                         wxFD_OPEN,
                                         TRUE /* ne change pas de repertoire courant */
                                         );
        if( FullFileName.IsEmpty() )
            break;
        if( !wxFileExists( FullFileName ) )
        {
            wxString msg;
            msg.Printf( _( "File %s not found" ), FullFileName.GetData() );
            DisplayError( this, msg ); break;
        }
        Read_Config( FullFileName );
        break;

    case ID_PREFERENCES_CREATE_CONFIG_HOTKEYS:
        FullFileName  = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );
        FullFileName += HOTKEY_FILENAME;
        FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
        WriteHotkeyConfigFile( FullFileName,
                               s_Pcbnew_Editor_Hokeys_Descr,
                               true );
        break;

    case ID_PREFERENCES_READ_CONFIG_HOTKEYS:
        Read_Hotkey_Config( this, true );
        break;

    case ID_PREFERENCES_EDIT_CONFIG_HOTKEYS:
    {
        FullFileName  = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );
        FullFileName += HOTKEY_FILENAME;
        FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
        AddDelimiterString( FullFileName );
        wxString editorname = GetEditorName();
        if( !editorname.IsEmpty() )
            ExecuteFile( this, editorname, FullFileName );
        break;
    }

    case ID_PREFERENCES_HOTKEY_PATH_IS_HOME:
    case ID_PREFERENCES_HOTKEY_PATH_IS_KICAD:
        HandleHotkeyConfigMenuSelection( this, id );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:           // Display Current hotkey list for eeschema
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
    FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
    return frame->ReadHotkeyConfigFile( FullFileName,
                                        s_Pcbnew_Editor_Hokeys_Descr,
                                        verbose );
}


/**************************************************************************/
bool Read_Config( const wxString& project_name )
/*************************************************************************/

/* lit la configuration, si elle n'a pas deja ete lue
 * 1 - lit <nom fichier brd>.pro
 * 2 - si non trouve lit <chemin de *.exe>/kicad.pro
 * 3 - si non trouve: init des variables aux valeurs par defaut
 *
 * Retourne TRUE si lu, FALSE si config non lue ou non modifiée
 */
{
    wxString FullFileName;
    int      ii;

    g_Prj_Config_Filename_ext = wxT( ".pro" );
    FullFileName = project_name;
    ChangeFileNameExt( FullFileName, g_Prj_Config_Filename_ext );

    /* Init des valeurs par defaut */
    g_LibName_List.Clear();

    wxGetApp().ReadProjectConfig( FullFileName,
                                  GROUP, ParamCfgList, FALSE );

    /* Traitement des variables particulieres: */

    SetRealLibraryPath( wxT( "modules" ) );

    if( ScreenPcb )
    {
        ScreenPcb->AddGrid( g_UserGrid, g_UserGrid_Unit, ID_POPUP_GRID_USER );
    }

    g_DesignSettings.m_TrackWidthHistory[0] =
        g_DesignSettings.m_CurrentTrackWidth;
    g_DesignSettings.m_ViaSizeHistory[0] =
        g_DesignSettings.m_CurrentViaSize;
    for( ii = 1; ii < HISTORY_NUMBER; ii++ )
    {
        g_DesignSettings.m_TrackWidthHistory[ii] = 0;
        g_DesignSettings.m_ViaSizeHistory[ii]    = 0;
    }

    return TRUE;
}


/**********************************************************/
void WinEDA_PcbFrame::Update_config( wxWindow* displayframe )
/***********************************************************/
/* enregistrement de la config */
{
    wxString FullFileName;
    wxString mask;

    mask = wxT( "*" ) + g_Prj_Config_Filename_ext;
    FullFileName = GetScreen()->m_FileName.AfterLast( '/' );
    ChangeFileNameExt( FullFileName, g_Prj_Config_Filename_ext );

    FullFileName = EDA_FileSelector( _( "Save preferences" ),
                                     wxPathOnly( GetScreen()->m_FileName ), /* Chemin par defaut */
                                     FullFileName,                          /* nom fichier par defaut */
                                     g_Prj_Config_Filename_ext,             /* extension par defaut */
                                     mask,                                  /* Masque d'affichage */
                                     displayframe,
                                     wxFD_SAVE,
                                     TRUE
                                     );
    if( FullFileName.IsEmpty() )
        return;

    /* ecriture de la configuration */
    wxGetApp().WriteProjectConfig( FullFileName, wxT( "/pcbnew" ),
                                   ParamCfgList );
}
