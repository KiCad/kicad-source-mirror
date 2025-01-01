/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013-2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <view/zoom_controller.h>

#include <trace_helpers.h>

#include <wx/log.h>

#include <algorithm>

using namespace KIGFX;


/**
 * A very simple timestamper that uses the #KIGFX::ACCELERATING_ZOOM_CONTROLLER::CLOCK
 * to provide a timestamp. Since that's a steady_clock, it's monotonic.
 */
class SIMPLE_TIMESTAMPER : public ACCELERATING_ZOOM_CONTROLLER::TIMESTAMP_PROVIDER
{
public:
    ACCELERATING_ZOOM_CONTROLLER::TIME_PT GetTimestamp() override
    {
        return ACCELERATING_ZOOM_CONTROLLER::CLOCK::now();
    }
};


ACCELERATING_ZOOM_CONTROLLER::ACCELERATING_ZOOM_CONTROLLER(
        double aScale, const TIMEOUT& aAccTimeout, TIMESTAMP_PROVIDER* aTimestampProv ) :
        m_accTimeout( aAccTimeout ),
        m_scale( aScale )
{
    if( aTimestampProv )
    {
        m_timestampProv = aTimestampProv;
    }
    else
    {
        m_ownTimestampProv = std::make_unique<SIMPLE_TIMESTAMPER>();
        m_timestampProv = m_ownTimestampProv.get();
    }

    m_prevTimestamp = m_timestampProv->GetTimestamp();
}


double ACCELERATING_ZOOM_CONTROLLER::GetScaleForRotation( int aRotation )
{
    // The minimal step value when changing the current zoom level
    const double minStep = 1.05;

    const auto timestamp = m_timestampProv->GetTimestamp();
    auto       timeDiff = std::chrono::duration_cast<TIMEOUT>( timestamp - m_prevTimestamp );

    m_prevTimestamp = timestamp;

    wxLogTrace( traceZoomScroll,
            wxString::Format( "Rot %d, time diff: %ldms", aRotation, (long)timeDiff.count() ) );

    double zoomScale;

    // Set scaling speed depending on scroll wheel event interval
    if( timeDiff < m_accTimeout
        && ( (aRotation > 0) == m_prevRotationPositive ) )
    {
        zoomScale = ( 2.05 * m_scale / 5.0 ) - timeDiff / m_accTimeout;

        // be sure zoomScale value is significant
        zoomScale = std::max( zoomScale, minStep );

        if( aRotation < 0 )
            zoomScale = 1.0 / zoomScale;
    }
    else
    {
        zoomScale = ( aRotation > 0 ) ? minStep : 1 / minStep;
    }
    m_prevRotationPositive = aRotation > 0;

    wxLogTrace( traceZoomScroll, wxString::Format( "  Zoom factor: %f", zoomScale ) );

    return zoomScale;
}

#ifdef __MINGW32__
// For some reason, this is needed to avoid a link issue
// (undefined reference to ACCELERATING_ZOOM_CONTROLLER::DEFAULT_TIMEOUT
// and GAL_API CONSTANT_ZOOM_CONTROLLER::MSW_SCALE)
constexpr ACCELERATING_ZOOM_CONTROLLER::TIMEOUT ACCELERATING_ZOOM_CONTROLLER::DEFAULT_TIMEOUT;
constexpr double GAL_API CONSTANT_ZOOM_CONTROLLER::MSW_SCALE;
#endif


CONSTANT_ZOOM_CONTROLLER::CONSTANT_ZOOM_CONTROLLER( double aScale ) : m_scale( aScale )
{
}


double CONSTANT_ZOOM_CONTROLLER::GetScaleForRotation( int aRotation )
{
    wxLogTrace( traceZoomScroll, wxString::Format( "Rot %d", aRotation ) );

    aRotation = ( aRotation > 0 ) ? std::min( aRotation, 100 ) : std::max( aRotation, -100 );

    double dscale = aRotation * m_scale;

    double zoom_scale = ( aRotation > 0 ) ? ( 1 + dscale ) : 1 / ( 1 - dscale );

    wxLogTrace( traceZoomScroll, wxString::Format( "  Zoom factor: %f", zoom_scale ) );

    return zoom_scale;
}
