/*
* This program source code file is part of KICAD, a free EDA CAD application.
*
* Copyright (C) 2016-2017 Kicad Developers, see change_log.txt for contributors.
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

#include <gal/gal_display_options.h>
#include <wx/config.h>

#include <config_map.h>

using namespace KIGFX;

/*
 * Config option strings
 */
static const wxString GalGLAntialiasingKeyword( "OpenGLAntialiasingMode" );
static const wxString GalGridStyleConfig( "GridStyle" );
static const wxString GalGridLineWidthConfig( "GridLineWidth" );
static const wxString GalGridMaxDensityConfig( "GridMaxDensity" );


static const UTIL::CFG_MAP<KIGFX::OPENGL_ANTIALIASING_MODE> aaModeConfigVals =
{
    { KIGFX::OPENGL_ANTIALIASING_MODE::NONE,                0 },
    { KIGFX::OPENGL_ANTIALIASING_MODE::SUBSAMPLE_HIGH,      1 },
    { KIGFX::OPENGL_ANTIALIASING_MODE::SUBSAMPLE_ULTRA,     2 },
    { KIGFX::OPENGL_ANTIALIASING_MODE::SUPERSAMPLING_X2,    3 },
    { KIGFX::OPENGL_ANTIALIASING_MODE::SUPERSAMPLING_X4,    4 },
};


static const UTIL::CFG_MAP<KIGFX::GRID_STYLE> gridStyleConfigVals =
{
    { KIGFX::GRID_STYLE::DOTS,       0 },
    { KIGFX::GRID_STYLE::LINES,      1 },
    { KIGFX::GRID_STYLE::SMALL_CROSS,2 },
};


GAL_DISPLAY_OPTIONS::GAL_DISPLAY_OPTIONS()
    : gl_antialiasing_mode( OPENGL_ANTIALIASING_MODE::NONE ),
      m_gridStyle( GRID_STYLE::DOTS )
{}


void GAL_DISPLAY_OPTIONS::ReadConfig( wxConfigBase* aCfg, wxString aBaseName )
{
    long readLong; // Temp value buffer

    aCfg->Read( aBaseName + GalGLAntialiasingKeyword, &readLong,
                static_cast<long>( KIGFX::OPENGL_ANTIALIASING_MODE::NONE ) );
    gl_antialiasing_mode = UTIL::GetValFromConfig( aaModeConfigVals, readLong );

    aCfg->Read( aBaseName + GalGridStyleConfig, &readLong,
                static_cast<long>( KIGFX::GRID_STYLE::DOTS ) );
    m_gridStyle = UTIL::GetValFromConfig( gridStyleConfigVals, readLong );

    aCfg->Read( aBaseName + GalGridLineWidthConfig,
                &m_gridLineWidth, 0.5 );

    aCfg->Read( aBaseName + GalGridMaxDensityConfig,
                &m_gridMinSpacing, 10 );

    NotifyChanged();
}


void GAL_DISPLAY_OPTIONS::WriteConfig( wxConfigBase* aCfg, wxString aBaseName )
{
    aCfg->Write( aBaseName + GalGLAntialiasingKeyword,
                 UTIL::GetConfigForVal( aaModeConfigVals, gl_antialiasing_mode ) );

    aCfg->Write( aBaseName + GalGridStyleConfig,
                 UTIL::GetConfigForVal( gridStyleConfigVals, m_gridStyle ) );

    aCfg->Write( aBaseName + GalGridLineWidthConfig,
                 m_gridLineWidth );

    aCfg->Write( aBaseName + GalGridMaxDensityConfig,
                 m_gridMinSpacing );
}


void GAL_DISPLAY_OPTIONS::NotifyChanged()
{
    Notify( &GAL_DISPLAY_OPTIONS_OBSERVER::OnGalDisplayOptionsChanged, *this );
}
