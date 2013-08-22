/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * @file view_controls.h
 * @brief VIEW_CONTROLS class definition.
 */

#ifndef __VIEW_CONTROLS_H
#define __VIEW_CONTROLS_H

#include <math/box2.h>

namespace KiGfx
{
class VIEW;

/**
 * Class VIEW_CONTROLS
 * is an interface for classes handling user events controlling the view behaviour
 * (such as zooming, panning, mouse grab, etc.)
 */
class VIEW_CONTROLS
{
public:
    VIEW_CONTROLS( VIEW* aView ) : m_view( aView ) {};
    virtual ~VIEW_CONTROLS() {};

    /**
     * Function Activate
     * Determines if all view related events (mouse wheel, right click panning, etc.), should be
     * handled or not. If not - they can be processed by the legacy view.
     * @param aEnabled tells if events should be handled.
     */
    virtual void Activate( bool aEnabled ) {};

    /**
     * Function SetGrabMouse
     * Turns on/off mouse grabbing. When the mouse is grabbed, it cannot go outside the VIEW.
     * @param aEnabled tells if mouse should be grabbed or not.
     */
    virtual void SetGrabMouse( bool aEnabled ) {};

    /**
     * Function SetAutoPan
     * Turns on/off auto panning (this feature is used when there is a tool active (eg. drawing a
     * track) and user moves mouse to the VIEW edge - then the view can be translated or not).
     * @param aEnabled tells if the autopanning should be active.
     */
    virtual void SetAutoPan( bool aEnabled ) {}

    /**
     * Function SetPanSpeed
     * Sets speed of panning.
     * @param aSpeed is a new speed for panning.
     */
    virtual void SetPanSpeed( float aSpeed ) {};

    /**
     * Function SetZoomSpeed
     * Determines how much zoom factor should be affected on one zoom event (eg. mouse wheel).
     * @param aSpeed is a new zooming speed.
     */
    virtual void SetZoomSpeed( float aSpeed ) {};

    /**
     * Function AnimatedZoom
     * // TODO
     */
    virtual void AnimatedZoom( const BOX2I& aExtents ) {};

    virtual void WarpCursor (const VECTOR2D& aPosition ) {};

    virtual void ShowCursor (bool aEnabled ) {};


protected:
    /// Pointer to controlled VIEW.
    VIEW* m_view;
};
} // namespace KiGfx

#endif
