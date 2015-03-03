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
#include <config_params.h>
#include <colors_selection.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <hotkeys.h>
#include <dialog_hotkeys_editor.h>


#define GROUP wxT("/gerbview")


void GERBVIEW_FRAME::Process_Config( wxCommandEvent& event )
{
    int      id = event.GetId();
    wxString FullFileName;

    switch( id )
    {
    // Hotkey IDs
    case ID_PREFERENCES_HOTKEY_EXPORT_CONFIG:
        ExportHotkeyConfigToFile( s_Gerbview_Hokeys_Descr, wxT( "gerbview" ) );
        break;

    case ID_PREFERENCES_HOTKEY_IMPORT_CONFIG:
        ImportHotkeyConfigFromFile( s_Gerbview_Hokeys_Descr, wxT( "gerbview" ) );
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
                                                        wxT( "DCodeColorEx" ),
                                                        &g_ColorsSettings.m_ItemsColors[
                                                            DCODES_VISIBLE],
                                                        WHITE ) );
    m_configSettings.push_back( new PARAM_CFG_SETCOLOR( true,
                                                        wxT( "NegativeObjectsColorEx" ),
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
        wxT("ColorLayer0Ex"),  wxT("ColorLayer1Ex"),  wxT("ColorLayer2Ex"),  wxT("ColorLayer3Ex"),
        wxT("ColorLayer4Ex"),  wxT("ColorLayer5Ex"),  wxT("ColorLayer6Ex"),  wxT("ColorLayer7Ex"),
        wxT("ColorLayer8Ex"),  wxT("ColorLayer9Ex"),  wxT("ColorLayer10Ex"), wxT("ColorLayer11Ex"),
        wxT("ColorLayer12Ex"), wxT("ColorLayer13Ex"), wxT("ColorLayer14Ex"), wxT("ColorLayer15Ex"),
        wxT("ColorLayer16Ex"), wxT("ColorLayer17Ex"), wxT("ColorLayer18Ex"), wxT("ColorLayer19Ex"),
        wxT("ColorLayer20Ex"), wxT("ColorLayer21Ex"), wxT("ColorLayer22Ex"), wxT("ColorLayer23Ex"),
        wxT("ColorLayer24Ex"), wxT("ColorLayer25Ex"), wxT("ColorLayer26Ex"), wxT("ColorLayer27Ex"),
        wxT("ColorLayer28Ex"), wxT("ColorLayer29Ex"), wxT("ColorLayer30Ex"), wxT("ColorLayer31Ex"),
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
