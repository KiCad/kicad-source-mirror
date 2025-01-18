/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmap2cmp_settings.h>
#include <settings/parameters.h>
#include <wx/config.h>


///! Update the schema version whenever a migration is required
const int bitmap2cmpSchemaVersion = 1;


BITMAP2CMP_SETTINGS::BITMAP2CMP_SETTINGS() :
        APP_SETTINGS_BASE( "bitmap2component", bitmap2cmpSchemaVersion ),
        m_BitmapFileName(),
        m_ConvertedFileName(),
        m_Units(),
        m_Threshold(),
        m_Negative(),
        m_LastFormat(),
        m_LastLayer()
{
    m_params.emplace_back( new PARAM<wxString>( "bitmap_file_name", &m_BitmapFileName, "" ) );
    m_params.emplace_back( new PARAM<wxString>( "converted_file_name", &m_ConvertedFileName, "" ) );
    m_params.emplace_back( new PARAM<int>( "units", &m_Units, 0 ) );
    m_params.emplace_back( new PARAM<int>( "threshold", &m_Threshold, 50 ) );
    m_params.emplace_back( new PARAM<bool>( "negative", &m_Negative, false ) );
    m_params.emplace_back( new PARAM<int>( "last_format", &m_LastFormat, 0 ) );
    m_params.emplace_back( new PARAM<int>( "last_mod_layer", &m_LastLayer, 0 ) );

    registerMigration( 0, 1,
            [&]() -> bool
            {
                // Version 1 introduced a new layer (F.Cu), and changed the ordering to
                // be consistent with PCBNew.
                switch( Get<int>( "last_mod_layer" ).value_or( 0 ) )
                {
                default:
                case 0: Set( "last_mod_layer", 1 ); break;
                case 1: Set( "last_mod_layer", 2 ); break;
                case 2: Set( "last_mod_layer", 7 ); break;
                case 3: Set( "last_mod_layer", 3 ); break;
                case 4: Set( "last_mod_layer", 4 ); break;
                case 5: Set( "last_mod_layer", 5 ); break;
                case 6: Set( "last_mod_layer", 6 ); break;
                }
                return true;
            } );
}


bool BITMAP2CMP_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    ret &= fromLegacyString( aCfg, "Last_input",        "bitmap_file_name" );
    ret &= fromLegacyString( aCfg, "Last_output",       "converted_file_name" );
    ret &= fromLegacy<int>(  aCfg, "Last_format",       "last_format" );
    ret &= fromLegacy<int>(  aCfg, "Last_modlayer",     "last_mod_layer" );
    ret &= fromLegacy<int>(  aCfg, "Threshold",         "threshold" );
    ret &= fromLegacy<bool>( aCfg, "Negative_choice",   "negative" );
    ret &= fromLegacy<int>(  aCfg, "Unit_selection",    "units" );

    return ret;
}
