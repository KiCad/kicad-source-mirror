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

namespace KIGFX
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
    VIEW_CONTROLS( VIEW* aView ) : m_view( aView ),
        m_forceCursorPosition( false ), m_cursorCaptured( false ), m_snappingEnabled( false ),
        m_grabMouse( false ), m_autoPanEnabled( false ), m_autoPanMargin( 0.1 ),
        m_autoPanSpeed( 0.15 ), m_enableZoomNoCenter( false )
    {
    }

    virtual ~VIEW_CONTROLS()
    {}

    /**
     * Function SetSnapping()
     * Enables/disables snapping cursor to grid.
     *
     * @param aEnabled says whether the opion should be enabled or disabled.
     */
    virtual void SetSnapping( bool aEnabled )
    {
        m_snappingEnabled = aEnabled;
    }

    /**
     * Function SetGrabMouse
     * Turns on/off mouse grabbing. When the mouse is grabbed, it cannot go outside the VIEW.
     * @param aEnabled tells if mouse should be grabbed or not.
     */
    virtual void SetGrabMouse( bool aEnabled )
    {
        m_grabMouse = aEnabled;
    }

    /**
     * Function SetAutoPan
     * Turns on/off auto panning (this feature is used when there is a tool active (eg. drawing a
     * track) and user moves mouse to the VIEW edge - then the view can be translated or not).
     * @param aEnabled tells if the autopanning should be active.
     */
    virtual void SetAutoPan( bool aEnabled )
    {
        m_autoPanEnabled = aEnabled;
    }

    /**
     * Function SetAutoPanSpeed()
     * Sets speed of autopanning.
     * @param aSpeed is a new speed for autopanning.
     */
    virtual void SetAutoPanSpeed( float aSpeed )
    {
        m_autoPanSpeed = aSpeed;
    }

    /**
     * Function SetAutoPanMArgin()
     * Sets margin for autopanning (ie. the area when autopanning becomes active).
     * @param aMargin is a new margin for autopanning.
     */
    virtual void SetAutoPanMargin( float aMargin )
    {
        m_autoPanMargin = aMargin;
    };

    /**
     * Function GetMousePosition()
     * Returns the current mouse pointer position in screen coordinates. Note, that it may be
     * different from the cursor position if snapping is enabled (@see GetCursorPosition()).
     *
     * @return The current mouse pointer position in screen coordinates.
     */
    virtual VECTOR2D GetMousePosition() const = 0;

    /**
     * Function GetCursorPosition()
     * Returns the current cursor position in world coordinates. Note, that it may be
     * different from the mouse pointer position if snapping is enabled or cursor position
     * is forced to specific point.
     *
     * @return The current cursor position in world coordinates.
     */
    virtual VECTOR2D GetCursorPosition() const = 0;

    /**
     * Function ForceCursorPosition()
     * Places the cursor immediately at a given point. Mouse movement is ignored.
     * @param aEnabled enable forced cursor position
     * @param aPosition the position
     */
    virtual void ForceCursorPosition( bool aEnabled, const VECTOR2D& aPosition = VECTOR2D( 0, 0 ) )
    {
        m_forcedPosition = aPosition;
        m_forceCursorPosition = aEnabled;
    }

    /**
     * Function ShowCursor()
     * Enables or disables display of cursor.
     * @param aEnabled decides if the cursor should be shown.
     */
    virtual void ShowCursor( bool aEnabled );

    /**
     * Function CaptureCursor()
     * Forces the cursor to stay within the drawing panel area.
     * @param aEnabled determines if the cursor should be captured.
     */
    virtual void CaptureCursor( bool aEnabled )
    {
        m_cursorCaptured = aEnabled;
    }

    inline bool IsCursorPositionForced() const
    {
        return m_forceCursorPosition;
    }

    /**
     * Function SetEnableZoomNoCenter()
     * Enables or Disables warping the cursor to the center of the drawing i
     * panel area when zooming.
     * @param aEnabled is true if the cursor should not be centered and false if
     * the cursor should be centered.
     */
    virtual void SetEnableZoomNoCenter( bool aEnable )
    {
        m_enableZoomNoCenter = aEnable;
    }

    virtual bool GetEnableZoomNoCenter() const
    {
        return m_enableZoomNoCenter;
    }

protected:
    /// Pointer to controlled VIEW.
    VIEW*       m_view;

    /// Current cursor position
    VECTOR2D    m_cursorPosition;

    /// Forced cursor position
    VECTOR2D    m_forcedPosition;

    /// Is the forced cursor position enabled
    bool        m_forceCursorPosition;

    /// Should the cursor be locked within the parent window area
    bool        m_cursorCaptured;

    /// Should the cursor snap to grid or move freely
    bool        m_snappingEnabled;

    /// Flag for grabbing the mouse cursor
    bool        m_grabMouse;

    /// Flag for turning on autopanning
    bool        m_autoPanEnabled;

    /// Distance from cursor to VIEW edge when panning is active
    float       m_autoPanMargin;

    /// How fast is panning when in auto mode
    float       m_autoPanSpeed;

    /// If the cursor should be warped to the center of the view area when zooming
    bool        m_enableZoomNoCenter;
};
} // namespace KIGFX

#endif
