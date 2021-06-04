/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <settings/common_settings.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>
#include "symbol_editor_settings.h"
#include <default_values.h>


///! Update the schema version whenever a migration is required
const int libeditSchemaVersion = 0;


SYMBOL_EDITOR_SETTINGS::SYMBOL_EDITOR_SETTINGS() :
        APP_SETTINGS_BASE( "symbol_editor", libeditSchemaVersion ),
        m_Defaults(),
        m_Repeat(),
        m_ShowPinElectricalType( true ),
        m_LibWidth(),
        m_EditComponentVisibleColumns()
{
    // Make Coverity happy
    m_UseEeschemaColorSettings = true;;

    // Init settings:
    SetLegacyFilename( "eeschema" );

    m_params.emplace_back( new PARAM<int>( "defaults.line_width",
                           &m_Defaults.line_width, DEFAULT_LINE_THICKNESS ) );

    m_params.emplace_back( new PARAM<int>( "defaults.text_size",
                           &m_Defaults.text_size, DEFAULT_TEXT_SIZE ) );

    m_params.emplace_back( new PARAM<int>( "defaults.pin_length",
                                           &m_Defaults.pin_length, DEFAULT_PIN_LENGTH ) );

    m_params.emplace_back( new PARAM<int>( "defaults.pin_name_size",
                           &m_Defaults.pin_name_size, DEFAULT_PINNAME_SIZE ) );

    m_params.emplace_back( new PARAM<int>( "defaults.pin_num_size",
                           &m_Defaults.pin_num_size, DEFAULT_PINNUM_SIZE ) );

    m_params.emplace_back( new PARAM<int>( "repeat.label_delta", &m_Repeat.label_delta, 1 ) );

    m_params.emplace_back( new PARAM<int>( "repeat.pin_step", &m_Repeat.pin_step, 100 ) );

    m_params.emplace_back( new PARAM<int>( "repeat.x_step", &m_Repeat.x_step, 0 ) );

    m_params.emplace_back( new PARAM<int>( "repeat.y_step", &m_Repeat.y_step, 100 ) );

    m_params.emplace_back( new PARAM<bool>( "show_pin_electrical_type",
            &m_ShowPinElectricalType, true ) );

    m_params.emplace_back( new PARAM<int>( "lib_table_width", &m_LibWidth, 250 ) );

    m_params.emplace_back( new PARAM<wxString>( "edit_component_visible_columns",
            &m_EditComponentVisibleColumns, "0 1 2 3 4 5 6 7" ) );

    m_params.emplace_back( new PARAM<wxString>( "pin_table_visible_columns",
            &m_PinTableVisibleColumns, "0 1 2 3 4 8 9" ) );

    m_params.emplace_back(
            new PARAM<bool>( "use_eeschema_color_settings", &m_UseEeschemaColorSettings, true ) );
}


bool SYMBOL_EDITOR_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    // Now modify the loaded grid selection, because in earlier versions the grids index was shared
    // between all applications and started at 1000 mils.  There is a 4-position offset between
    // this index and the possible eeschema grids list that we have to subtract.
    std::string gridSizePtr = "window.grid.last_size";

    if( OPT<int> currentSize = Get<int>( gridSizePtr ) )
    {
        Set( gridSizePtr, *currentSize - 4 );
    }
    else
    {
        // Otherwise, default grid size should be 50 mils; index 1
        Set( gridSizePtr,  1 );
    }

    ret &= fromLegacy<int>( aCfg, "DefaultWireWidth",      "defaults.line_width" );
    ret &= fromLegacy<int>( aCfg, "DefaultPinLength",      "defaults.pin_length" );
    ret &= fromLegacy<int>( aCfg, "LibeditPinNameSize",    "defaults.pin_name_size" );
    ret &= fromLegacy<int>( aCfg, "LibeditPinNumSize",     "defaults.pin_num_size" );

    ret &= fromLegacy<int>( aCfg, "LibeditRepeatLabelInc", "repeat.label_delta" );
    ret &= fromLegacy<int>( aCfg, "LibeditPinRepeatStep",  "repeat.pin_step" );
    ret &= fromLegacy<int>( aCfg, "LibeditRepeatStepX",    "repeat.x_step" );
    ret &= fromLegacy<int>( aCfg, "LibeditRepeatStepY",    "repeat.y_step" );

    ret &= fromLegacy<int>(  aCfg, "LibeditLibWidth",              "lib_table_width" );
    ret &= fromLegacy<bool>( aCfg, "LibeditShowPinElectricalType", "show_pin_electrical_type" );

    ret &= fromLegacyString( aCfg, "LibEditFieldsShownColumns", "edit_component_visible_columns" );

    ret &= fromLegacyString( aCfg, "PinTableShownColumns", "pin_table_visible_columns" );

    return ret;
}
