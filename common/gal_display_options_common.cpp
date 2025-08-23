/*
* This program source code file is part of KICAD, a free EDA CAD application.
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

#include <gal_display_options_common.h>
#include <settings/app_settings.h>
#include <settings/common_settings.h>

#include <wx/log.h>

#include <config_map.h>
#include <dpi_scaling_common.h>

using namespace KIGFX;

/**
 * Flag to enable GAL_DISPLAY_OPTIONS logging
 *
 * Use "KICAD_GAL_DISPLAY_OPTIONS" to enable.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* traceGalDispOpts = wxT( "KICAD_GAL_DISPLAY_OPTIONS" );


static const UTIL::CFG_MAP<KIGFX::GRID_STYLE> gridStyleConfigVals = {
    { KIGFX::GRID_STYLE::DOTS, 0 },
    { KIGFX::GRID_STYLE::LINES, 1 },
    { KIGFX::GRID_STYLE::SMALL_CROSS, 2 },
};

static const UTIL::CFG_MAP<KIGFX::GRID_SNAPPING> gridSnapConfigVals = {
    { KIGFX::GRID_SNAPPING::ALWAYS, 0 },
    { KIGFX::GRID_SNAPPING::WITH_GRID, 1 },
    { KIGFX::GRID_SNAPPING::NEVER, 2 }
};

GAL_DISPLAY_OPTIONS_IMPL::GAL_DISPLAY_OPTIONS_IMPL() :
    GAL_DISPLAY_OPTIONS(),
    m_dpi( { nullptr, nullptr } )
{
}


void GAL_DISPLAY_OPTIONS_IMPL::ReadWindowSettings( WINDOW_SETTINGS& aCfg )
{
    wxLogTrace( traceGalDispOpts, wxS( "Reading app-specific options" ) );

    m_gridStyle = UTIL::GetValFromConfig( gridStyleConfigVals, aCfg.grid.style );
    m_gridSnapping = UTIL::GetValFromConfig( gridSnapConfigVals, aCfg.grid.snap );
    m_gridLineWidth = aCfg.grid.line_width;
    m_gridMinSpacing = aCfg.grid.min_spacing;
    m_axesEnabled = aCfg.grid.axes_enabled;

    m_crossHairMode = aCfg.cursor.cross_hair_mode;
    m_forceDisplayCursor = aCfg.cursor.always_show_cursor;

    NotifyChanged();
}


void GAL_DISPLAY_OPTIONS_IMPL::ReadCommonConfig( COMMON_SETTINGS& aSettings, wxWindow* aWindow )
{
    wxLogTrace( traceGalDispOpts, wxS( "Reading common config" ) );

    antialiasing_mode =
            static_cast<KIGFX::GAL_ANTIALIASING_MODE>( aSettings.m_Graphics.aa_mode );

    m_dpi = DPI_SCALING_COMMON( &aSettings, aWindow );
    UpdateScaleFactor();

    NotifyChanged();
}


void GAL_DISPLAY_OPTIONS_IMPL::ReadConfig( COMMON_SETTINGS& aCommonConfig,
                                           WINDOW_SETTINGS& aWindowConfig, wxWindow* aWindow )
{
    wxLogTrace( traceGalDispOpts, wxS( "Reading common and app config" ) );

    ReadWindowSettings( aWindowConfig );

    ReadCommonConfig( aCommonConfig, aWindow );
}


void GAL_DISPLAY_OPTIONS_IMPL::WriteConfig( WINDOW_SETTINGS& aCfg )
{
    wxLogTrace( traceGalDispOpts, wxS( "Writing window settings" ) );

    aCfg.grid.style = UTIL::GetConfigForVal( gridStyleConfigVals, m_gridStyle );
    aCfg.grid.snap = UTIL::GetConfigForVal( gridSnapConfigVals, m_gridSnapping );
    aCfg.grid.line_width = m_gridLineWidth;
    aCfg.grid.min_spacing = m_gridMinSpacing;
    aCfg.grid.axes_enabled = m_axesEnabled;
    aCfg.cursor.cross_hair_mode = m_crossHairMode;
    aCfg.cursor.always_show_cursor = m_forceDisplayCursor;
}


void GAL_DISPLAY_OPTIONS_IMPL::UpdateScaleFactor()
{
    if( m_scaleFactor != m_dpi.GetScaleFactor() )
    {
        m_scaleFactor = m_dpi.GetScaleFactor();
        NotifyChanged();
    }
}
