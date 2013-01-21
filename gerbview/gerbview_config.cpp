/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file gerbview_config.cpp
 * @brief GerbView configuration.
*/

#include <fctsys.h>
#include <macros.h>
#include <id.h>
#include <common.h>
#include <class_drawpanel.h>
#include <gestfich.h>
#include <param_config.h>
#include <colors_selection.h>

#include <gerbview.h>
#include <hotkeys.h>
#include <dialog_hotkeys_editor.h>


#define GROUP wxT("/gerbview")

#define INSETUP true


void GERBVIEW_FRAME::Process_Config( wxCommandEvent& event )
{
    int      id = event.GetId();
    wxString FullFileName;

    switch( id )
    {
    // Hotkey IDs
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

        // Display current hotkey list for GerbView.
        DisplayHotkeyList( this, s_Gerbview_Hokeys_Descr );
        break;

    default:
        wxMessageBox( wxT( "GERBVIEW_FRAME::Process_Config error" ) );
        break;
    }
}


PARAM_CFG_ARRAY& GERBVIEW_FRAME::GetConfigurationSettings()
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
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true,
                                                        wxT( "NegativeObjectsColor" ),
                                                        &g_ColorsSettings.m_ItemsColors[
                                                            NEGATIVE_OBJECTS_VISIBLE],
                                                        DARKGRAY ) );
    m_configSettings.push_back( new PARAM_CFG_BOOL( true,
                                                    wxT( "DisplayPolarCoordinates" ),
                                                    &m_DisplayOptions.m_DisplayPolarCood,
                                                    false ) );

    // Default colors for layers 0 to 31
    static const EDA_COLOR_T color_default[] = {
        GREEN,     BLUE,         LIGHTGRAY, MAGENTA,
        RED,       DARKGREEN,    BROWN,     MAGENTA,
        LIGHTGRAY, BLUE,         GREEN,     CYAN,
        LIGHTRED,  LIGHTMAGENTA, YELLOW,    RED,
        BLUE,      BROWN,        LIGHTCYAN, RED,
        MAGENTA,   CYAN,         BROWN,     MAGENTA,
        LIGHTGRAY, BLUE,         GREEN,     DARKCYAN,
        YELLOW,    LIGHTMAGENTA, YELLOW,    LIGHTGRAY,
    };

    // List of keywords used as identifiers in config.
    // They *must* be static const and not temporarily created,
    // because the parameter list that use these keywords does not store them,
    // just points to them.
    static const wxChar* keys[] = {
        wxT("ColorLayer_0"),     wxT("ColorLayer_1"),     wxT("ColorLayer_2"),     wxT("ColorLayer_3"),
        wxT("ColorLayer_4"),     wxT("ColorLayer_5"),     wxT("ColorLayer_6"),     wxT("ColorLayer_7"),
        wxT("ColorLayer_8"),     wxT("ColorLayer_9"),     wxT("ColorLayer_10"),    wxT("ColorLayer_11"),
        wxT("ColorLayer_12"),    wxT("ColorLayer_13"),    wxT("ColorLayer_14"),    wxT("ColorLayer_15"),

        wxT("ColorLayer_16"),    wxT("ColorLayer_17"),    wxT("ColorLayer_18"),    wxT("ColorLayer_19"),
        wxT("ColorLayer_20"),    wxT("ColorLayer_21"),    wxT("ColorLayer_22"),    wxT("ColorLayer_23"),
        wxT("ColorLayer_24"),    wxT("ColorLayer_25"),    wxT("ColorLayer_26"),    wxT("ColorLayer_27"),
        wxT("ColorLayer_28"),    wxT("ColorLayer_29"),    wxT("ColorLayer_30"),    wxT("ColorLayer_31"),
    };

    wxASSERT( DIM(keys) == DIM(color_default) );
    wxASSERT( DIM(keys) <= DIM(g_ColorsSettings.m_LayersColors) && DIM(keys) <= DIM(color_default) );

    for( unsigned i = 0; i < DIM(keys);  ++i )
    {
        EDA_COLOR_T* prm = &g_ColorsSettings.m_LayersColors[i];

        PARAM_CFG_SETCOLOR* prm_entry =
            new PARAM_CFG_SETCOLOR( true, keys[i], prm, color_default[i] );

        m_configSettings.push_back( prm_entry );
    }

    return m_configSettings;
}
