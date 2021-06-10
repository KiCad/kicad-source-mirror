/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <gal/dpi_scaling.h>

#include <core/optional.h>

#include <env_vars.h>
#include <settings/common_settings.h>
#include <kiplatform/ui.h>

#include <wx/log.h>


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
static OPT<double> getKiCadConfiguredScale( const COMMON_SETTINGS& aConfig )
{
    OPT<double> scale;
    double      canvas_scale = aConfig.m_Appearance.canvas_scale;

    if( canvas_scale > 0.0 )
    {
        scale = canvas_scale;
    }

    if( scale )
    {
        wxLogTrace( traceHiDpi, "Scale factor (configured): %f", *scale );
    }

    return scale;
}


/**
 * Get the toolkit scale factor from a user-set environment variable
 * (for example GDK_SCALE on GTK).
 *
 * @return the scale factor, if set
 */
static OPT<double> getEnvironmentScale()
{
    const wxPortId port_id = wxPlatformInfo::Get().GetPortId();
    OPT<double>    scale;

    if( port_id == wxPORT_GTK )
    {
        // Under GTK, the user can use GDK_SCALE to force the scaling
        scale = GetEnvVar<double>( "GDK_SCALE" );
    }

    if( scale )
    {
        wxLogTrace( traceHiDpi, "Scale factor (environment): %f", *scale );
    }

    return scale;
}


DPI_SCALING::DPI_SCALING( COMMON_SETTINGS* aConfig, const wxWindow* aWindow )
        : m_config( aConfig ), m_window( aWindow )
{
}


double DPI_SCALING::GetScaleFactor() const
{
    OPT<double> val;

    if( m_config )
    {
        val = getKiCadConfiguredScale( *m_config );
    }

    if( !val )
    {
        val = getEnvironmentScale();
    }

    if( !val && m_window )
    {
        // Use the native WX reporting.
        // On Linux, this will not work until WX 3.2 and GTK >= 3.10
        // Otherwise it returns 1.0
        val = KIPLATFORM::UI::GetSystemScaleFactor( m_window );
        wxLogTrace( traceHiDpi, "Scale factor (WX): %f", *val );
    }

    if( !val )
    {
        // Nothing else we can do, give it a default value
        val = GetDefaultScaleFactor();
        wxLogTrace( traceHiDpi, "Scale factor (default): %f", *val );
    }

    return *val;
}


bool DPI_SCALING::GetCanvasIsAutoScaled() const
{
    if( m_config == nullptr )
    {
        // No configuration given, so has to be automatic scaling
        return true;
    }

    const bool automatic = getKiCadConfiguredScale( *m_config ) == boost::none;
    wxLogTrace( traceHiDpi, "Scale is automatic: %d", automatic );
    return automatic;
}


void DPI_SCALING::SetDpiConfig( bool aAuto, double aValue )
{
    wxCHECK_RET( m_config != nullptr, "Setting DPI config without a config store." );

    const double value = aAuto ? 0.0 : aValue;

    m_config->m_Appearance.canvas_scale = value;
}


double DPI_SCALING::GetMaxScaleFactor()
{
    // displays with higher than 4.0 DPI are not really going to be useful
    // for KiCad (even an 8k display would be effectively only ~1080p at 4x)
    return 6.0;
}


double DPI_SCALING::GetMinScaleFactor()
{
    // scales under 1.0 don't make sense from a HiDPI perspective
    return 1.0;
}

double DPI_SCALING::GetDefaultScaleFactor()
{
    // no scaling => 1.0
    return 1.0;
}
