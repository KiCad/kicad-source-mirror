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
 *
 */

/**
 * @file zoom_controller.h
 * @brief ZOOM_CONTROLLER class definition.
 */

#ifndef __ZOOM_CONTROLLER_H
#define __ZOOM_CONTROLLER_H

#include <gal/gal.h>
#include <chrono>
#include <memory>

namespace KIGFX
{

/**
 * Handle the response of the zoom scale to external inputs.
 */
class GAL_API ZOOM_CONTROLLER
{
public:
    virtual ~ZOOM_CONTROLLER() = default;

    /**
     * Get the scale factor produced by a given mousewheel rotation.
     *
     * @param aRotation rotation of the mouse wheel (this comes from
     *                  wxMouseEvent::GetWheelRotation()).
     * @return the scale factor to scroll by.
     */
    virtual double GetScaleForRotation( int aRotation ) = 0;
};


/**
 * Class that zooms faster if scroll events happen very close together.
 */
class GAL_API ACCELERATING_ZOOM_CONTROLLER : public ZOOM_CONTROLLER
{
public:
    /// The type of the acceleration timeout.
    using TIMEOUT = std::chrono::milliseconds;

    /// The clock used for the timestamp (guaranteed to be monotonic).
    using CLOCK = std::chrono::steady_clock;

    /// The type of the time stamps.
    using TIME_PT = std::chrono::time_point<CLOCK>;

    /// The default timeout, after which a another scroll will not be accelerated.
    static constexpr TIMEOUT DEFAULT_TIMEOUT = std::chrono::milliseconds( 500 );

    /// The default minimum step factor for accelerating controller.
    static constexpr double DEFAULT_ACCELERATION_SCALE = 5.0;

    /*
     * A class interface that provides timestamps for events.
     */
    class TIMESTAMP_PROVIDER
    {
    public:
        virtual ~TIMESTAMP_PROVIDER() = default;

        /**
         * @return the timestamp at the current time.
         */
        virtual TIME_PT GetTimestamp() = 0;
    };

    /**
     * @param aAccTimeout the timeout - if a scroll happens within this timeframe,
     *                    the zoom will be faster.
     * @param aTimestampProv a provider for timestamps. If null, a default will be provided,
     *                       which is the main steady_clock (this is probably what you want
     *                       for real usage).
     */
    ACCELERATING_ZOOM_CONTROLLER( double aScale = DEFAULT_ACCELERATION_SCALE,
                                  const TIMEOUT& aAccTimeout = DEFAULT_TIMEOUT,
                                  TIMESTAMP_PROVIDER* aTimestampProv = nullptr );

    double GetScaleForRotation( int aRotation ) override;

    TIMEOUT GetTimeout() const
    {
        return m_accTimeout;
    }

    void SetTimeout( const TIMEOUT& aNewTimeout )
    {
        m_accTimeout = aNewTimeout;
    }

private:
    /// The timestamp provider to use (might be provided externally).
    TIMESTAMP_PROVIDER* m_timestampProv;

    /// Any provider owned by this class (the default one, if used).
    std::unique_ptr<TIMESTAMP_PROVIDER> m_ownTimestampProv;

    /// The timestamp of the previous event.
    TIME_PT m_prevTimestamp;

    /// The timeout value.
    TIMEOUT m_accTimeout;

    /// Previous rotation was positive.
    bool m_prevRotationPositive = false;

    /// A multiplier for the minimum zoom step size
    double m_scale;
};


/**
 * A CONSTANT_ZOOM_CONTROLLER that zooms by a fixed factor based only on the magnitude of the scroll
 * wheel rotation.
 */
class GAL_API CONSTANT_ZOOM_CONTROLLER : public ZOOM_CONTROLLER
{
public:
    /**
     * @param aScale a scaling parameter that adjusts the magnitude of the scroll. This factor
     *               might be dependent on the platform for comfort.
     */
    CONSTANT_ZOOM_CONTROLLER( double aScale );

    double GetScaleForRotation( int aRotation ) override;

    /// A suitable (magic) scale factor for GTK3 systems.
    static constexpr double GTK3_SCALE = 0.002;

    /// A suitable (magic) scale factor for Mac systems.
    static constexpr double MAC_SCALE = 0.01;

    /// A suitable (magic) scale factor for Windows systems.
    static constexpr double MSW_SCALE = 0.005;

    /// Multiplier for manual scale ssetting.
    static constexpr double MANUAL_SCALE_FACTOR = 0.001;

private:
    /// The scale factor set by the constructor.
    double m_scale;
};

} // namespace KIGFX

#endif
