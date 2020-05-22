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

#include <cvpcb_settings.h>
#include <settings/parameters.h>
#include <wx/config.h>


///! Update the schema version whenever a migration is required
const int cvpcbSchemaVersion = 0;

CVPCB_SETTINGS::CVPCB_SETTINGS() :
        APP_SETTINGS_BASE( "cvpcb", cvpcbSchemaVersion )
{
    // Make Coverity happy:
    m_FilterFootprint = 0;

    m_FootprintViewerMagneticSettings.pads     = MAGNETIC_OPTIONS::NO_EFFECT;
    m_FootprintViewerMagneticSettings.tracks   = MAGNETIC_OPTIONS::NO_EFFECT;
    m_FootprintViewerMagneticSettings.graphics = false;

    // Init settings:
    m_params.emplace_back( new PARAM<int>( "filter_footprint", &m_FilterFootprint, 0 ) );

    addParamsForWindow( &m_FootprintViewer, "footprint_viewer" );

    m_params.emplace_back(
            new PARAM<bool>( "footprint_viewer.pad_fill",
                    &m_FootprintViewerDisplayOptions.m_DisplayPadFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "footprint_viewer.pad_numbers",
            &m_FootprintViewerDisplayOptions.m_DisplayPadNum, true ) );

    m_params.emplace_back( new PARAM<bool>( "footprint_viewer.footprint_text_fill",
            &m_FootprintViewerDisplayOptions.m_DisplayModTextFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "footprint_viewer.graphic_items_fill",
            &m_FootprintViewerDisplayOptions.m_DisplayDrawItemsFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "footprint_viewer.magnetic_graphics",
            &m_FootprintViewerMagneticSettings.graphics, false ) );

    m_params.emplace_back( new PARAM<int>( "footprint_viewer.magnetic_pads",
            reinterpret_cast<int*>( &m_FootprintViewerMagneticSettings.pads ),
            static_cast<int>( MAGNETIC_OPTIONS::NO_EFFECT ) ) );
}


bool CVPCB_SETTINGS::MigrateFromLegacy( wxConfigBase* aCfg )
{
    bool ret = APP_SETTINGS_BASE::MigrateFromLegacy( aCfg );

    ret &= fromLegacy<int>( aCfg, "FilterFootprint", "filter_footprint" );

    ret &= migrateWindowConfig( aCfg, "FootprintViewerFrame", "footprint_viewer" );

    ret &= fromLegacy<bool>( aCfg, "FootprintViewerFrameDiPadFi", "footprint_viewer.pad_fill" );
    ret &= fromLegacy<bool>( aCfg, "FootprintViewerFrameDiPadNu", "footprint_viewer.pad_numbers" );
    ret &= fromLegacy<bool>(
            aCfg, "FootprintViewerFrameDiModTx", "footprint_viewer.footprint_text_fill" );

    return ret;
}
