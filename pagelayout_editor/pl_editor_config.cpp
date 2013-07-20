/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pl_editor_config.cpp
 * @brief page layout editor configuration.
*/

#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>
#include <gestfich.h>
#include <param_config.h>

#include <pl_editor_frame.h>
#include <hotkeys.h>
#include <dialog_hotkeys_editor.h>
#include <pl_editor_id.h>


#define GROUP wxT("/pl_editor")

#define INSETUP true


void PL_EDITOR_FRAME::Process_Config( wxCommandEvent& event )
{
    int      id = event.GetId();

    switch( id )
    {
    case wxID_PREFERENCES:
        break;

    // Standard basic hotkey IDs
    case ID_PREFERENCES_HOTKEY_SHOW_EDITOR:
        InstallHotkeyFrame( this, s_PlEditor_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_EXPORT_CONFIG:
        ExportHotkeyConfigToFile( s_PlEditor_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_IMPORT_CONFIG:
        ImportHotkeyConfigFromFile( s_PlEditor_Hokeys_Descr );
        break;

    case ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST:
        DisplayHotkeyList( this, s_PlEditor_Hokeys_Descr );
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
