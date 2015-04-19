/**
 * @file pl_editor_config.cpp
 * @brief page layout editor configuration.
*/

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>
#include <gestfich.h>
#include <config_params.h>

#include <pl_editor_frame.h>
#include <hotkeys.h>
#include <dialog_hotkeys_editor.h>
#include <pl_editor_id.h>

#define GROUP wxT("/pl_editor")


void PL_EDITOR_FRAME::Process_Config( wxCommandEvent& event )
{
    int      id = event.GetId();

    switch( id )
    {
    case ID_MENU_SWITCH_BGCOLOR:
        if( GetDrawBgColor() == WHITE )
            SetDrawBgColor( BLACK );
        else
            SetDrawBgColor( WHITE );

        GetMenuBar()->SetLabel( ID_MENU_SWITCH_BGCOLOR,
                                GetDrawBgColor() == WHITE ?
                                _( "&BackGround Black" ) :
                                _( "&BackGround White" ) );
        m_canvas->Refresh();
        break;

    case ID_MENU_GRID_ONOFF:
        SetGridVisibility( ! IsGridVisible() );
        GetMenuBar()->SetLabel( ID_MENU_GRID_ONOFF,
                                IsGridVisible() ? _( "Hide &Grid" ) :
                                _( "Show &Grid" ) );
        m_canvas->Refresh();
        break;

    // Standard basic hotkey IDs
    case ID_PREFERENCES_HOTKEY_SHOW_EDITOR:
        InstallHotkeyFrame( this, PlEditorHokeysDescr );
        break;

    case ID_PREFERENCES_HOTKEY_EXPORT_CONFIG:
        ExportHotkeyConfigToFile( PlEditorHokeysDescr, wxT( "pl_editor" ) );
        break;

    case ID_PREFERENCES_HOTKEY_IMPORT_CONFIG:
        ImportHotkeyConfigFromFile( PlEditorHokeysDescr, wxT( "pl_editor" ) );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        DisplayHotkeyList( this, PlEditorHokeysDescr );
        break;

    default:
        wxMessageBox( wxT( "PL_EDITOR_FRAME::Process_Config error" ) );
        break;
    }
}


PARAM_CFG_ARRAY& PL_EDITOR_FRAME::GetConfigurationSettings()
{
    if( !m_configSettings.empty() )
        return m_configSettings;

    m_configSettings.push_back( new PARAM_CFG_INT( true, wxT( "Units" ),
                                                   (int*) &g_UserUnit, 0, 0, 1 ) );

    return m_configSettings;
}
