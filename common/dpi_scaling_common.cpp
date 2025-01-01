/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <dpi_scaling_common.h>

#include <optional>

#include <env_vars.h>
#include <settings/common_settings.h>
#include <kiplatform/ui.h>

#include <wx/log.h>
#include <wx/window.h>


/**
 * Flag to enable trace for HiDPI scaling factors
 *
 * Use "KICAD_TRACE_HIGH_DPI" to enable.
 *
 * @ingroup trace_env_vars
 */
const wxChar* const traceHiDpi = wxT( "KICAD_TRACE_HIGH_DPI" );


/**
 * Get a user-configured scale factor from KiCad config file
 *
 * @return the scale factor, if set
 */
static std::optional<double> getKiCadConfiguredScale( const COMMON_SETTINGS& aConfig )
{
    std::optional<double> scale;
    double                canvas_scale = aConfig.m_Appearance.canvas_scale;

    if( canvas_scale > 0.0 )
    {
        scale = canvas_scale;
    }

    return scale;
}


/**
 * Get the toolkit scale factor from a user-set environment variable
 * (for example GDK_SCALE on GTK).
 *
 * @return the scale factor, if set
 */
static std::optional<double> getEnvironmentScale()
{
    const wxPortId        port_id = wxPlatformInfo::Get().GetPortId();
    std::optional<double> scale;

    if( port_id == wxPORT_GTK )
    {
        // Under GTK, the user can use GDK_SCALE to force the scaling
        scale = ENV_VAR::GetEnvVar<double>( wxS( "GDK_SCALE" ) );
    }

    return scale;
}


DPI_SCALING_COMMON::DPI_SCALING_COMMON( COMMON_SETTINGS* aConfig, const wxWindow* aWindow ) :
        m_config( aConfig ),
        m_window( aWindow )
{
}


double DPI_SCALING_COMMON::GetScaleFactor() const
{
    std::optional<double> val;
    wxString              src;

    if( m_config )
    {
        val = getKiCadConfiguredScale( *m_config );
        src = wxS( "config" );
    }

    if( !val )
    {
        val = getEnvironmentScale();
        src = wxS( "env" );
    }

    if( !val && m_window )
    {
        // Use the native WX reporting.
        // On Linux, this will not work until WX 3.2 and GTK >= 3.10
        // Otherwise it returns 1.0
        val = KIPLATFORM::UI::GetPixelScaleFactor( m_window );
        src = wxS( "platform" );
    }

    if( !val )
    {
        // Nothing else we can do, give it a default value
        val = GetDefaultScaleFactor();
        src = wxS( "default" );
    }

    wxLogTrace( traceHiDpi, wxS( "Scale factor (%s): %f" ), src, *val );

    return *val;
}


double DPI_SCALING_COMMON::GetContentScaleFactor() const
{
    std::optional<double> val;
    wxString              src;

    if( m_config )
    {
        val = getKiCadConfiguredScale( *m_config );
        src = wxS( "config" );
    }

    if( !val )
    {
        val = getEnvironmentScale();
        src = wxS( "env" );
    }

    if( !val && m_window )
    {
        // Use the native WX reporting.
        // On Linux, this will not work until WX 3.2 and GTK >= 3.10
        // Otherwise it returns 1.0
        val = KIPLATFORM::UI::GetContentScaleFactor( m_window );
        src = wxS( "platform" );
    }

    if( !val )
    {
        // Nothing else we can do, give it a default value
        val = GetDefaultScaleFactor();
        src = wxS( "default" );
    }

    wxLogTrace( traceHiDpi, wxS( "Content scale factor (%s): %f" ), src, *val );

    return *val;
}


bool DPI_SCALING_COMMON::GetCanvasIsAutoScaled() const
{
    if( m_config == nullptr )
    {
        // No configuration given, so has to be automatic scaling
        return true;
    }

    const bool automatic = getKiCadConfiguredScale( *m_config ) == std::nullopt;
    wxLogTrace( traceHiDpi, wxS( "Scale is automatic: %d" ), automatic );
    return automatic;
}


void DPI_SCALING_COMMON::SetDpiConfig( bool aAuto, double aValue )
{
    wxCHECK_RET( m_config != nullptr, wxS( "Setting DPI config without a config store." ) );

    const double value = aAuto ? 0.0 : aValue;

    m_config->m_Appearance.canvas_scale = value;
}
