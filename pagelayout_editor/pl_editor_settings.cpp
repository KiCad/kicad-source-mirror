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

#include <settings/parameters.h>
#include <wx/config.h>

#include "pl_editor_settings.h"

///! Update the schema version whenever a migration is required
const int plEditorSchemaVersion = 0;


PL_EDITOR_SETTINGS::PL_EDITOR_SETTINGS() :
        APP_SETTINGS_BASE( "pl_editor", plEditorSchemaVersion )
{
    // Make Coverity happy:
    m_CornerOrigin = 0;
    m_PropertiesFrameWidth = 150;
    m_LastCustomWidth = 17000;
    m_LastCustomHeight = 11000;
    m_LastWasPortrait = false;
    m_BlackBackground = false;

    // Build settings:
    m_params.emplace_back(
            new PARAM<int>( "properties_frame_width", &m_PropertiesFrameWidth, 150 ) );

    m_params.emplace_back( new PARAM<int>( "corner_origin", &m_CornerOrigin, 0 ) );

    m_params.emplace_back( new PARAM<bool>( "black_background", &m_BlackBackground, false ) );

    m_params.emplace_back( new PARAM<wxString>( "last_paper_size", &m_LastPaperSize, "A3" ) );

    m_params.emplace_back( new PARAM<int>( "last_custom_width", &m_LastCustomWidth, 17000 ) );

    m_params.emplace_back( new PARAM<int>( "last_custom_height", &m_LastCustomHeight, 11000 ) );

    m_params.emplace_back( new PARAM<bool>( "last_was_portrait", &m_LastWasPortrait, false ) );
}


bool PL_EDITOR_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    ret &= fromLegacy<int>(  aCfg, "PropertiesFrameWidth",  "properties_frame_width" );
    ret &= fromLegacy<int>(  aCfg, "CornerOriginChoice",    "corner_origin" );
    ret &= fromLegacy<bool>( aCfg, "BlackBgColor",          "black_background" );
    ret &= fromLegacy<int>(  aCfg, "LastUsedPaperSize",     "last_paper_size" );
    ret &= fromLegacy<int>(  aCfg, "LastUsedCustomWidth",   "last_custom_width" );
    ret &= fromLegacy<int>(  aCfg, "LastUsedCustomHeight",  "last_custom_height" );
    ret &= fromLegacy<bool>( aCfg, "LastUsedWasPortrait",   "last_was_portrait" );

    return ret;
}
