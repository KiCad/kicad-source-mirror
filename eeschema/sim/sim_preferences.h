/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __SIM_PREFERENCES__
#define __SIM_PREFERENCES__


/**
 * @file sim_preferences.h
 *
 * Contains preferences pertaining to the simulator.
 */

/**
 * Enumerates the possible mouse wheel actions that can be performed on simulator plots.
 */
enum class SIM_MOUSE_WHEEL_ACTION
{
    // Directly using mpWindow::MouseWheelAction would leak wxMathPlot via the eeschema_settings.h
    // header, so we duplicate it here in this miminal header.

    NONE,
    PAN_LEFT_RIGHT,
    PAN_RIGHT_LEFT,
    PAN_UP_DOWN,
    ZOOM,
    ZOOM_HORIZONTALLY,
    ZOOM_VERTICALLY,
    COUNT // Internal use only
};

/**
 * Contains the set of modified mouse wheel actions that can be performed on a simulator plot.
 */
struct SIM_MOUSE_WHEEL_ACTION_SET
{
    // Directly using mpWindow::MouseWheelActionSet would leak wxMathPlot via the
    // eeschema_settings.h header, so we duplicate it here in this miminal header.

    SIM_MOUSE_WHEEL_ACTION vertical_unmodified;
    SIM_MOUSE_WHEEL_ACTION vertical_with_ctrl;
    SIM_MOUSE_WHEEL_ACTION vertical_with_shift;
    SIM_MOUSE_WHEEL_ACTION vertical_with_alt;
    SIM_MOUSE_WHEEL_ACTION horizontal;

    static SIM_MOUSE_WHEEL_ACTION_SET GetMouseDefaults()
    {
        // Returns defaults equivalent to the global Mouse and Touchpad default settings

        SIM_MOUSE_WHEEL_ACTION_SET actions;
        actions.vertical_unmodified = SIM_MOUSE_WHEEL_ACTION::ZOOM;
        actions.vertical_with_ctrl  = SIM_MOUSE_WHEEL_ACTION::PAN_LEFT_RIGHT;
        actions.vertical_with_shift = SIM_MOUSE_WHEEL_ACTION::PAN_UP_DOWN;
        actions.vertical_with_alt   = SIM_MOUSE_WHEEL_ACTION::NONE;
        actions.horizontal          = SIM_MOUSE_WHEEL_ACTION::NONE;
        return actions;
    }

    static SIM_MOUSE_WHEEL_ACTION_SET GetTrackpadDefaults()
    {
        // Returns defaults equivalent to the global Mouse and Touchpad default settings

        SIM_MOUSE_WHEEL_ACTION_SET actions;
        actions.vertical_unmodified = SIM_MOUSE_WHEEL_ACTION::PAN_UP_DOWN;
        actions.vertical_with_ctrl  = SIM_MOUSE_WHEEL_ACTION::ZOOM;
        actions.vertical_with_shift = SIM_MOUSE_WHEEL_ACTION::PAN_LEFT_RIGHT;
        actions.vertical_with_alt   = SIM_MOUSE_WHEEL_ACTION::NONE;
        actions.horizontal          = SIM_MOUSE_WHEEL_ACTION::PAN_LEFT_RIGHT;
        return actions;
    }
};

/**
 * Contains preferences pertaining to the simulator.
 */
struct SIM_PREFERENCES
{
    SIM_MOUSE_WHEEL_ACTION_SET mouse_wheel_actions;
};

#endif // __SIM_PREFERENCES__
