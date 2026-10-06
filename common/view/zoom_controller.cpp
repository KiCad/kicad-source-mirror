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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <view/zoom_controller.h>

#include <trace_helpers.h>

#include <wx/log.h>

#include <algorithm>
#include <cmath>

using namespace KIGFX;


// Notches carried by one wheel event, where aNotch is the rotation of a single notch
// A zero aNotch means the rotation is a scroll speed, so the event counts as one notch
static double notchCount( int aRotation, int aNotch )
{
    if( aNotch <= 0 || std::abs( aRotation ) < aNotch )
        return 1.0;

    return std::abs( aRotation ) / double( aNotch );
}


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
        double aScale, const TIMEOUT& aAccTimeout, TIMESTAMP_PROVIDER* aTimestampProv, int aNotch ) :
        m_accTimeout( aAccTimeout ),
        m_scale( aScale ),
        m_notch( aNotch )
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

    if( notchCount( aRotation, m_notch ) > 1.0 )
        zoomScale = std::pow( zoomScale, notchCount( aRotation, m_notch ) );

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


CONSTANT_ZOOM_CONTROLLER::CONSTANT_ZOOM_CONTROLLER( double aScale, int aNotch ) :
        m_scale( aScale ),
        m_notch( aNotch )
{
}


double CONSTANT_ZOOM_CONTROLLER::GetScaleForRotation( int aRotation )
{
    wxLogTrace( traceZoomScroll, wxString::Format( "Rot %d", aRotation ) );

    const int rotation = aRotation;

    aRotation = ( aRotation > 0 ) ? std::min( aRotation, 100 ) : std::max( aRotation, -100 );

    double dscale = aRotation * m_scale;

    double zoom_scale = ( aRotation > 0 ) ? ( 1 + dscale ) : 1 / ( 1 - dscale );

    // A notch saturates the clamp, so each further notch of a merged event zooms by that factor again
    if( notchCount( rotation, m_notch ) > 1.0 )
        zoom_scale = std::pow( zoom_scale, notchCount( rotation, m_notch ) );

    wxLogTrace( traceZoomScroll, wxString::Format( "  Zoom factor: %f", zoom_scale ) );

    return zoom_scale;
}
