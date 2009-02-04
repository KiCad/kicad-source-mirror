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
    int      id = event.GetId();
    wxPoint  pos;
    wxString FullFileName;

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
        wxString mask( wxT( "*" ) ); mask += g_Prj_Config_Filename_ext;
        FullFileName = g_RootSheet->m_AssociatedScreen->m_FileName;
        ChangeFileNameExt( FullFileName, g_Prj_Config_Filename_ext );

        FullFileName = EDA_FileSelector( _( "Read config file" ),
                                         wxGetCwd(),                /* Chemin par defaut */
                                         FullFileName,              /* nom fichier par defaut */
                                         g_Prj_Config_Filename_ext, /* extension par defaut */
                                         mask,                      /* Masque d'affichage */
                                         this,
                                         wxFD_OPEN,
                                         TRUE /* ne change pas de repertoire courant */
                                         );
        if( FullFileName.IsEmpty() )
            break;
        if( !wxFileExists( FullFileName ) )
        {
            wxString msg = _( "File " ) + FullFileName + _( "not found" );;
            DisplayError( this, msg ); break;
        }
        Read_Config( FullFileName, TRUE );
    }
    break;

    case ID_PREFERENCES_CREATE_CONFIG_HOTKEYS:
        FullFileName  = ReturnHotkeyConfigFilePath( g_ConfigFileLocationChoice );
        FullFileName += HOTKEY_FILENAME;
        FullFileName += DEFAULT_HOTKEY_FILENAME_EXT;
        WriteHotkeyConfigFile( FullFileName, s_Eeschema_Hokeys_Descr, true );
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
        DisplayError( this,
                     wxT(
                         "WinEDA_SchematicFrame::Process_Config internal error" ) );
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
    wxString                FullFileName;
    bool                    IsRead = TRUE;
    wxArrayString           liblist_tmp = g_LibName_List;
    WinEDA_SchematicFrame*  frame;

    frame = (WinEDA_SchematicFrame*)wxGetApp().GetTopWindow();

    if( CfgFileName.IsEmpty() )
        FullFileName = g_RootSheet->m_AssociatedScreen->m_FileName;
    else
        FullFileName = CfgFileName;
    g_LibName_List.Clear();

    if( !wxGetApp().ReadProjectConfig( FullFileName,
                                       GROUP, ParamCfgList,
                                       ForceRereadConfig ? FALSE : TRUE ) ) // Config non lue
    {
        g_LibName_List = liblist_tmp;
        IsRead = FALSE;
    }

    /* Traitement des variables particulieres: */
    SetRealLibraryPath( wxT( "library" ) );

    // If the list is void, load the libraries "power.lib" and "device.lib"
    if( g_LibName_List.GetCount() == 0 )
    {
        g_LibName_List.Add( wxT( "power" ) );
        g_LibName_List.Add( wxT( "device" ) );
    }

    if( frame )
    {
        frame->SetDrawBgColor( g_DrawBgColor );
        frame->m_Draw_Grid = g_ShowGrid;
    }

    LoadLibraries( frame );

    return IsRead;
}


/****************************************************************/
void WinEDA_SchematicFrame::Save_Config( wxWindow* displayframe )
/***************************************************************/
{
    wxString path;
    wxString FullFileName;
    wxString mask( wxT( "*" ) );

    mask += g_Prj_Config_Filename_ext;
    FullFileName = g_RootSheet->m_AssociatedScreen->m_FileName.AfterLast( '/' ) /*ConfigFileName*/;
    ChangeFileNameExt( FullFileName, g_Prj_Config_Filename_ext );

    path = wxGetCwd();
    FullFileName = EDA_FileSelector( _( "Save preferences" ),
                                     path,                      /* Chemin par defaut */
                                     FullFileName,              /* nom fichier par defaut */
                                     g_Prj_Config_Filename_ext, /* extension par defaut */
                                     mask,                      /* Masque d'affichage */
                                     displayframe,
                                     wxFD_SAVE,
                                     TRUE
                                     );
    if( FullFileName.IsEmpty() )
        return;

    /* ecriture de la configuration */
    wxGetApp().WriteProjectConfig( FullFileName, GROUP, ParamCfgList );
}
