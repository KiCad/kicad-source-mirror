/*
 * This program source code file is part of KiCad, a free EDA CAD application.

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
 */

#ifndef TOOL_EVENT_UTILS_H
#define TOOL_EVENT_UTILS_H

#include <tool/tool_interactive.h>
#include <geometry/eda_angle.h>


class PCB_BASE_EDIT_FRAME;


/**
 * Namespace TOOL_EVT_UTILS
 *
 * Utility functions for dealing with various tool events. These are free functions, so they
 * interface with any classes exclusively via the public interfaces, so they don't need to be
 * subsumed into the "helped" classes.
 */
namespace TOOL_EVT_UTILS
{
    /**
     * Function isRotateToolEvt()
     *
     * @param aEvt event to check
     * @return true if the event is a rotation action tool event
     */
    bool IsRotateToolEvt( const TOOL_EVENT& aEvt );

    /**
     * Function getEventRotationAngle()
     *
     * Helper function to get a rotation angle based on a frame's configured angle and the
     * direction indicated by a rotation action event
     *
     * @param aFrame the PCB edit frame to use to get the base rotation step value from
     * @param aEvent the tool event - should be a rotation action event and should have a rotation
     *                              multiplier parameter
     *
     * @return the clockwise rotation angle
     */
    EDA_ANGLE GetEventRotationAngle( const PCB_BASE_EDIT_FRAME& aFrame, const TOOL_EVENT& aEvent );
}

#endif // TOOL_EVENT_UTILS_H
