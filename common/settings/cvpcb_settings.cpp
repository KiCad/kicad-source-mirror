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

#include <settings/cvpcb_settings.h>
#include <settings/parameters.h>
#include <wx/config.h>
#include <geometry/geometry_utils.h>


///! Update the schema version whenever a migration is required
const int cvpcbSchemaVersion = 0;

CVPCB_SETTINGS::CVPCB_SETTINGS() :
        PCB_VIEWERS_SETTINGS_BASE( "cvpcb", cvpcbSchemaVersion ),
        m_FilterFlags( 0 ),
        m_LibrariesWidth( 0 ),
        m_FootprintsWidth( 0 )
{
    // We always snap and don't let the user configure it
    m_FootprintViewerMagneticSettings.pads     = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
    m_FootprintViewerMagneticSettings.tracks   = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
    m_FootprintViewerMagneticSettings.graphics = true;

    // Init settings:
    m_params.emplace_back( new PARAM<int>( "filter_footprint", &m_FilterFlags, 0 ) );
    m_params.emplace_back( new PARAM<wxString>( "filter_footprint_text", &m_FilterString, "" ) );

    m_params.emplace_back( new PARAM<int>( "libraries_pane_width", &m_LibrariesWidth, 0 ) );
    m_params.emplace_back( new PARAM<int>( "footprints_pane_width", &m_FootprintsWidth, 0 ) );

    addParamsForWindow( &m_FootprintViewer, "footprint_viewer" );

    m_params.emplace_back( new PARAM<double>( "footprint_viewer.zoom",
            &m_FootprintViewerZoom, 1.0 ) );
    m_params.emplace_back( new PARAM<bool>( "footprint_viewer.autozoom",
            &m_FootprintViewerAutoZoomOnSelect, true ) );

    m_params.emplace_back( new PARAM<int>( "footprint_viewer.angle_snap_mode",
            reinterpret_cast<int*>( &m_ViewersDisplay.m_AngleSnapMode ),
            static_cast<int>( LEADER_MODE::DEG45 ) ) );

    m_params.emplace_back( new PARAM<bool>( "footprint_viewer.show_pad_fill",
            &m_ViewersDisplay.m_DisplayPadFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "footprint_viewer.show_pad_number",
            &m_ViewersDisplay.m_DisplayPadNumbers, true ) );

    m_params.emplace_back( new PARAM<bool>( "footprint_viewer.show_text_fill",
            &m_ViewersDisplay.m_DisplayTextFill, true ) );

    m_params.emplace_back( new PARAM<bool>( "footprint_viewer.show_graphic_fill",
            &m_ViewersDisplay.m_DisplayGraphicsFill, true ) );
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

    ret &= fromLegacy<bool>( aCfg, "FootprintViewerFrameAutoZoom",   "footprint_viewer.auto_zoom" );
    ret &= fromLegacy<double>( aCfg, "FootprintViewerFrameZoom",     "footprint_viewer.zoom" );

    return ret;
}
