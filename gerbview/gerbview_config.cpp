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

void GERBVIEW_FRAME::Process_Config( wxCommandEvent& event )
{
    int      id = event.GetId();
    wxPoint  pos;
    wxString FullFileName;

    pos    = GetPosition();
    pos.x += 20;
    pos.y += 20;

    switch( id )
    {
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
                     wxT( "GERBVIEW_FRAME::Process_Config internal error" ) );
    }
}


/*
 * Return the Gerbview applications settings list.
 * (list of parameters that must be saved in Gerbview parameters)
 *
 * This replaces the old statically define list that had the project
 * file settings and the application settings mixed together.  This
 * was confusing and caused some settings to get saved and loaded
 * incorrectly.  Currently, only the settings that are needed at start
 * up by the main window are defined here.  There are other locally used
 * settings scattered thoughout the EESchema source code.  If you need
 * to define a configuration setting that need to be loaded at run time,
 * this is the place to define it.
 *
 * TODO: Define the configuration variables as member variables instead of
 *       global variables or move them to the object class where they are
 *       used.
 */

PARAM_CFG_ARRAY& GERBVIEW_FRAME::GetConfigurationSettings( void )
{
    if( !m_configSettings.empty() )
        return m_configSettings;

    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "Units" ),
                                                    (int*) &g_UserUnit, 0, 0, 1 ) );

    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "DrawModeOption" ),
                                                    &m_displayMode, 2, 0, 2 ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true,
                                                        wxT( "DCodeColor" ),
                                                        &g_ColorsSettings.m_ItemsColors[
                                                            DCODES_VISIBLE],
                                                        WHITE ) );

    m_configSettings.push_back( new PARAM_CFG_BOOL( true,
                                                    wxT( "DisplayPolarCoordinates" ),
                                                    &DisplayOpt.DisplayPolarCood,
                                                    false ) );

    // Color select parameters:
    static const int color_default[32] = // Default values for color layers 0 to 31
    {
        GREEN,     BLUE,         LIGHTGRAY, MAGENTA,
        RED,       DARKGREEN,    BROWN,     MAGENTA,
        LIGHTGRAY, BLUE,         GREEN,     CYAN,
        LIGHTRED,  LIGHTMAGENTA, YELLOW,    RED,
        BLUE,      BROWN,        LIGHTCYAN, RED,
        MAGENTA,   CYAN,         BROWN,     MAGENTA,
        LIGHTGRAY, BLUE,         GREEN,     DARKCYAN,
        YELLOW,    LIGHTMAGENTA, YELLOW,    LIGHTGRAY
    };

    // List of keywords used as identifiers in config
    // they *must* be static const and not temporary created,
    // because the parameter list that use these keywords does not store them,
    // just points on them
    static const wxChar * keys[32] =
    {
        wxT("ColorLayer0"), wxT("ColorLayer1"), wxT("ColorLayer2"), wxT("ColorLayer3"),
        wxT("ColorLayer4"), wxT("ColorLayer5"), wxT("ColorLayer6"), wxT("ColorLayer7"),
        wxT("ColorLayer8"), wxT("ColorLayer9"), wxT("ColorLayer10"), wxT("ColorLayer11"),
        wxT("ColorLayer12"), wxT("ColorLaye13"), wxT("ColorLayer14"), wxT("ColorLayer15")
    };
    for( unsigned ii = 0; ii < 32; ii++ )
    {
        int * prm = &g_ColorsSettings.m_LayersColors[1];
        PARAM_CFG_SETCOLOR * prm_entry =
            new PARAM_CFG_SETCOLOR( true, keys[ii], prm, color_default[1] );
        m_configSettings.push_back( prm_entry );
    }

    return m_configSettings;
}
