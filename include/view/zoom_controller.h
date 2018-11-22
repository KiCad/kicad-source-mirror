/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file zoom_control.h
 * @brief ZOOM_CONTROLLER class definition.
 */

#ifndef __ZOOM_CONTROLLER_H
#define __ZOOM_CONTROLLER_H


namespace KIGFX
{

/**
 * Class that handles the response of the zoom scale to external inputs
 */
class ZOOM_CONTROLLER
{
public:
    virtual ~ZOOM_CONTROLLER() = default;

    /**
     * Gets the scale factor produced by a given mousewheel rotation
     * @param  aRotation rotation of the mouse wheel (this comes from
     *                    wxMouseEvent::GetWheelRotation())
     * @return            the scale factor to scroll by
     */
    virtual double GetScaleForRotation( int aRotation ) = 0;
};


/**
 * Class that zooms faster if scroll events happen very close together.
 */
class ACCELERATING_ZOOM_CONTROLLER : public ZOOM_CONTROLLER
{
public:
    /**
     * @param aAccTimeout the timeout - if a scoll happens within this timeframe,
     *                    the zoom will be faster
     */
    ACCELERATING_ZOOM_CONTROLLER( unsigned aAccTimeout );

    double GetScaleForRotation( int aRotation ) override;

private:
    /**
     * @return the timestamp of an event at the current time. Monotonic.
     */
    double getTimeStamp() const;

    /// The timestamp of the last event
    double   m_lastTimeStamp;
    /// The timeout value
    unsigned m_accTimeout;
};


/**
 * A ZOOM_CONTROLLER that zooms by a fixed factor based only on the magnitude
 * of the scroll wheel rotation.
 */
class CONSTANT_ZOOM_CONTROLLER : public ZOOM_CONTROLLER
{
public:
    /**
     * @param aScale a scaling parameter that adjusts the magnitude of the
     * scroll. This factor might be dependent on the platform for comfort.
     */
    CONSTANT_ZOOM_CONTROLLER( double aScale );

    double GetScaleForRotation( int aRotation ) override;

    /// A suitable (magic) scale factor for GTK3 systems
    static constexpr double GTK3_SCALE = 0.001;

    /// A suitable (magic) scale factor for Mac systems
    static constexpr double MAC_SCALE = 0.01;

private:
    /// The scale factor set by the constructor.
    double m_scale;
};

} // namespace KIGFX

#endif
