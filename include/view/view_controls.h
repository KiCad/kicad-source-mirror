/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef __VIEW_CONTROLS_H
#define __VIEW_CONTROLS_H

#include <math/box2.h>
#include <settings/common_settings.h>
#include <gal/gal.h>

namespace KIGFX
{
class VIEW;


/// Structure to keep VIEW_CONTROLS settings for easy store/restore operations.
struct GAL_API VC_SETTINGS
{
    VC_SETTINGS()
    {
        Reset();
    }

    /// Restore the default settings.
    void Reset();

    /// Flag determining the cursor visibility.
    bool m_showCursor;

    /// Forced cursor position (world coordinates).
    VECTOR2D m_forcedPosition;

    /// Is the forced cursor position enabled.
    bool m_forceCursorPosition;

    /// Should the cursor be locked within the parent window area.
    bool m_cursorCaptured;

    /// Should the cursor snap to grid or move freely.
    bool m_snappingEnabled;

    /// Flag for grabbing the mouse cursor.
    bool m_grabMouse;

    /// Flag for automatic focus switching between Schematic and PCB editors.
    bool m_focusFollowSchPcb;

    /// Flag for turning on autopanning.
    bool m_autoPanEnabled;

    /// Flag for turning on autopanning.
    bool m_autoPanSettingEnabled;

    /// Distance from cursor to VIEW edge when panning is active.
    float m_autoPanMargin;

    /// How fast is panning when in auto mode.
    float m_autoPanSpeed;

    /// How fast does panning accelerate when approaching the window boundary.
    float m_autoPanAcceleration;

    /// If the cursor is allowed to be warped.
    bool m_warpCursor;

    /// Enable horizontal panning with the horizontal scroll/trackpad input.
    bool m_horizontalPan;

    /// Enable the accelerating zoom controller.
    bool m_zoomAcceleration;

    /// Zoom speed for the non-accelerating zoom controller.
    int m_zoomSpeed;

    /// When true, ignore zoom_speed and pick a platform-specific default.
    bool m_zoomSpeedAuto;

    /// What modifier key to enable zoom with the (vertical) scroll wheel.
    int m_scrollModifierZoom;

    /// What modifier key to enable horizontal pan with the (vertical) scroll wheel.
    int m_scrollModifierPanH;

    /// What modifier key to enable vertical with the (vertical) scroll wheel.
    int m_scrollModifierPanV;

    /// What modifier key pans the view when the mouse moves with it held.
    int m_motionPanModifier;

    MOUSE_DRAG_ACTION m_dragLeft;
    MOUSE_DRAG_ACTION m_dragMiddle;
    MOUSE_DRAG_ACTION m_dragRight;

    /// Is last cursor motion event coming from keyboard arrow cursor motion action.
    bool m_lastKeyboardCursorPositionValid;

    /// ACTIONS::CURSOR_UP, ACTIONS::CURSOR_DOWN, etc.
    long m_lastKeyboardCursorCommand;

    /// Position of the above event.
    VECTOR2D m_lastKeyboardCursorPosition;


    /// Whether to invert the scroll wheel movement for zoom.
    bool m_scrollReverseZoom;

    /// Whether to invert the scroll wheel movement for horizontal pan.
    bool m_scrollReversePanH;
};


/**
 * An interface for classes handling user events controlling the view behavior such as
 * zooming, panning, mouse grab, etc.
 */
class GAL_API VIEW_CONTROLS
{
public:
    VIEW_CONTROLS( VIEW* aView ) :
            m_view( aView ),
            m_cursorWarped( false )
    {
    }

    virtual ~VIEW_CONTROLS()
    {
    }

    /**
     * Turn on/off mouse grabbing.
     * When the mouse is grabbed, it cannot go outside the VIEW.
     *
     * @param aEnabled tells if mouse should be grabbed or not.
     */
    virtual void SetGrabMouse( bool aEnabled )
    {
        m_settings.m_grabMouse = aEnabled;
    }

    /**
     * Turn on/off auto panning (this feature is used when there is a tool active (eg. drawing a
     * track) and user moves mouse to the VIEW edge - then the view can be translated or not).
     *
     * @param aEnabled tells if the autopanning should be active.
     */
    virtual void SetAutoPan( bool aEnabled )
    {
        m_settings.m_autoPanEnabled = aEnabled;
    }

    /**
     * Turn on/off auto panning (user setting to disable it entirely).
     *
     * @param aEnabled tells if the autopanning should be enabled.
     */
    virtual void EnableAutoPan( bool aEnabled )
    {
        m_settings.m_autoPanSettingEnabled = aEnabled;
    }

    /**
     * Set the speed of autopanning.
     *
     * @param aSpeed is a new speed for autopanning.
     */
    virtual void SetAutoPanSpeed( float aSpeed )
    {
        m_settings.m_autoPanSpeed = aSpeed;
    }

    /**
     * Set the speed of autopanning.
     *
     * @param aSpeed is a new speed for autopanning.
     */
    virtual void SetAutoPanAcceleration( float aAcceleration )
    {
        m_settings.m_autoPanAcceleration = aAcceleration;
    }

    /**
     * Set the margin for autopanning (ie. the area when autopanning becomes active).
     *
     * @param aMargin is a new margin for autopanning.
     */
    virtual void SetAutoPanMargin( float aMargin )
    {
        m_settings.m_autoPanMargin = aMargin;
    }

    virtual void PinCursorInsideNonAutoscrollArea( bool aWarpMouseCursor ) = 0;

    /**
     * Return the current mouse pointer position.
     *
     * @note The position may be different from the cursor position if snapping is
     *       enabled (@see GetCursorPosition()).
     *
     * @note The position is clamped if outside of coordinates representation limits.
     *
     * @param aWorldCoordinates if true, the result is given in world coordinates, otherwise
     *                          it is given in screen coordinates.
     * @return The current mouse pointer position in either world or screen coordinates.
     */
    virtual VECTOR2D GetMousePosition( bool aWorldCoordinates = true ) const = 0;

    /**
     * Return the current cursor position in world coordinates.
     *
     * @note The position  may be different from the mouse pointer position if snapping is
     *       enabled or cursor position is forced to a specific point.
     *
     * @note The position is clamped if outside of coordinates representation limits.
     *
     * @return The current cursor position in world coordinates.
     */
    VECTOR2D GetCursorPosition() const
    {
        return GetCursorPosition( m_settings.m_snappingEnabled );
    }

    /**
     * Return the current cursor position in world coordinates ignoring the cursorUp
     * position force mode.
     *
     * @return The current cursor position in world coordinates.
     */
    virtual VECTOR2D GetRawCursorPosition( bool aSnappingEnabled = true ) const = 0;

    /**
     * Return the current cursor position in world coordinates.
     *
     * @note The position may be different from the mouse pointer position if snapping is
     *       enabled or cursor position is forced to a specific point.
     *
     * @note The position is clamped if outside of coordinates representation limits.
     *
     * @param aEnableSnapping selects whether cursor position should be snapped to the grid.
     * @return The current cursor position in world coordinates.
     */
    virtual VECTOR2D GetCursorPosition( bool aEnableSnapping ) const = 0;

    /**
     * Place the cursor immediately at a given point. Mouse movement is ignored.
     *
     * @note The position is clamped if outside of coordinates representation limits.
     *
     * @param aEnabled enable forced cursor position
     * @param aPosition the position (world coordinates).
     */
    virtual void ForceCursorPosition( bool aEnabled, const VECTOR2D& aPosition = VECTOR2D( 0, 0 ) )
    {
        m_settings.m_forceCursorPosition = aEnabled;
        m_settings.m_forcedPosition = aPosition;
    }

    /**
     * Move cursor to the requested position expressed in world coordinates.
     *
     * The position is not forced and will be overridden with the next mouse motion event.
     * Mouse cursor follows the world cursor.
     *
     * @note The position is clamped if outside of coordinates representation limits.
     *
     * @param aPosition is the requested cursor position in the world coordinates.
     * @param aWarpView enables/disables view warp if the cursor is outside the current viewport.
     */
    virtual void SetCursorPosition( const VECTOR2D& aPosition, bool aWarpView = true,
                                    bool aTriggeredByArrows = false, long aArrowCommand = 0 ) = 0;


    /**
     * Move the graphic crosshair cursor to the requested position expressed in world coordinates.
     *
     * @note The position is clamped if outside of coordinates representation limits.
     *
     * @param aPosition is the requested cursor position in the world coordinates.
     * @param aWarpView enables/disables view warp if the cursor is outside the current viewport.
     */
    virtual void SetCrossHairCursorPosition( const VECTOR2D& aPosition, bool aWarpView = true ) = 0;

    /**
     * Enable or disables display of cursor.
     *
     * @param aEnabled decides if the cursor should be shown.
     */
    virtual void ShowCursor( bool aEnabled );

    /**
     * Return true when cursor is visible.
     *
     * @return True if cursor is visible.
     */
    bool IsCursorShown() const;

    /**
     * Force the cursor to stay within the drawing panel area.
     *
     * @param aEnabled determines if the cursor should be captured.
     */
    virtual void CaptureCursor( bool aEnabled )
    {
        m_settings.m_cursorCaptured = aEnabled;
    }

    /**
     * If enabled (@see SetEnableCursorWarping(), warps the cursor to the specified position,
     * expressed either in the screen coordinates or the world coordinates.
     *
     * @note The position is clamped if outside of coordinates representation limits.
     *
     * @param aPosition is the position where the cursor should be warped.
     * @param aWorldCoordinates if true treats aPosition as the world coordinates, otherwise it
     *                          uses it as the screen coordinates.
     * @param aWarpView determines if the view can be warped too (only matters if the position is
     *                  specified in the world coordinates and its not visible in the current
     *                  viewport).
     */
    virtual void WarpMouseCursor( const VECTOR2D& aPosition, bool aWorldCoordinates = false,
                                  bool aWarpView = false ) = 0;

    /**
     * Enable or disable warping the cursor.
     *
     * @param aEnable is true if the cursor is allowed to be warped.
     */
    void EnableCursorWarping( bool aEnable )
    {
        m_settings.m_warpCursor = aEnable;
    }

    /**
     * @return the current setting for cursor warping.
     */
    bool IsCursorWarpingEnabled() const
    {
        return m_settings.m_warpCursor;
    }

    /**
     * Set the viewport center to the current cursor position and warps the cursor to the
     * screen center.
     */
    virtual void CenterOnCursor() = 0;

    /**
     * Restore the default VIEW_CONTROLS settings.
     */
    virtual void Reset();

    /// Return the current VIEW_CONTROLS settings.
    const VC_SETTINGS& GetSettings() const
    {
        return m_settings;
    }

    /// Apply VIEW_CONTROLS settings from an object.
    void ApplySettings( const VC_SETTINGS& aSettings );

    /// Load new settings from program common settings.
    virtual void LoadSettings() {}

protected:
    /// Pointer to controlled VIEW.
    VIEW* m_view;

    /// Application warped the cursor, not the user (keyboard).
    bool m_cursorWarped;

    /// Current VIEW_CONTROLS settings.
    VC_SETTINGS m_settings;
};
} // namespace KIGFX

#endif
