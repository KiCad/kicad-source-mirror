/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "spice_settings.h"
#include <settings/parameters.h>


const int spiceSettingsSchemaVersion = 0;


SPICE_SETTINGS::SPICE_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "simulator", spiceSettingsSchemaVersion, aParent, aPath ),
        m_fixIncludePaths( true )
{
    m_params.emplace_back( new PARAM<wxString>( "workbook_filename", &m_workbookFilename, "" ) );
    m_params.emplace_back( new PARAM<bool>( "fix_include_paths", &m_fixIncludePaths, true ) );
}


bool SPICE_SETTINGS::operator==( const SPICE_SETTINGS&aRhs ) const
{
    return m_workbookFilename == aRhs.m_workbookFilename
            && m_fixIncludePaths == aRhs.m_fixIncludePaths;
}


NGSPICE_SETTINGS::NGSPICE_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        SPICE_SETTINGS( aParent, aPath ),
        m_compatibilityMode( NGSPICE_COMPATIBILITY_MODE::LT_PSPICE )
{
    m_params.emplace_back( new PARAM_ENUM<NGSPICE_COMPATIBILITY_MODE>( "model_mode",
            &m_compatibilityMode, NGSPICE_COMPATIBILITY_MODE::LT_PSPICE,
            NGSPICE_COMPATIBILITY_MODE::USER_CONFIG, NGSPICE_COMPATIBILITY_MODE::HSPICE ) );
}


bool NGSPICE_SETTINGS::operator==( const SPICE_SETTINGS& aRhs ) const
{
    const NGSPICE_SETTINGS* settings = dynamic_cast<const NGSPICE_SETTINGS*>( &aRhs );

    wxCHECK( settings, false );

    return SPICE_SETTINGS::operator==( aRhs )
            && m_compatibilityMode == settings->m_compatibilityMode;
}
