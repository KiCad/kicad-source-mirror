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

#include <gal/gal_display_options.h>
#include <settings/app_settings.h>
#include <settings/common_settings.h>

#include <wx/log.h>

#include <config_map.h>
#include <dpi_scaling.h>

using namespace KIGFX;


/**
 * Flag to enable GAL_DISPLAY_OPTIONS logging
 *
 * Use "KICAD_GAL_DISPLAY_OPTIONS" to enable.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* traceGalDispOpts = wxT( "KICAD_GAL_DISPLAY_OPTIONS" );


GAL_DISPLAY_OPTIONS::GAL_DISPLAY_OPTIONS()
    : antialiasing_mode( GAL_ANTIALIASING_MODE::AA_NONE ),
      m_gridStyle( GRID_STYLE::DOTS ),
      m_gridSnapping( GRID_SNAPPING::ALWAYS ),
      m_gridLineWidth( 1.0 ),
      m_gridMinSpacing( 10.0 ),
      m_axesEnabled( false ),
      m_crossHairMode( CROSS_HAIR_MODE::SMALL_CROSS ),
      m_forceDisplayCursor( false ),
      m_scaleFactor( DPI_SCALING::GetDefaultScaleFactor() )
{
}


void GAL_DISPLAY_OPTIONS::NotifyChanged()
{
    wxLogTrace( traceGalDispOpts, wxS( "Change notification" ) );

    Notify( &GAL_DISPLAY_OPTIONS_OBSERVER::OnGalDisplayOptionsChanged, *this );
}