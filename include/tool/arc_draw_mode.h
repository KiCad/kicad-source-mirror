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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

/**
 * Construction sequence used when interactively drawing a circular arc.
 *
 * Values are persisted in the application settings, so new modes must be appended.
 */
enum class ARC_DRAW_MODE
{
    CENTER_START_END = 0,   ///< Center, then start (sets radius), then sweep to the end
    START_END_MID,          ///< Start, end, then the arc midpoint dragged along the chord bisector
    START_END_CENTER,       ///< Start, end, then the center dragged along the chord bisector
    TANGENT,                ///< End only; start and departure direction come from a tangent seed
    START_DIR_END,          ///< Start, a point giving the departure direction, then the end
};


/// Return the next mode in the cycle order used by the cycle-mode action.
inline ARC_DRAW_MODE IncrementArcDrawMode( ARC_DRAW_MODE aMode )
{
    switch( aMode )
    {
    case ARC_DRAW_MODE::CENTER_START_END: return ARC_DRAW_MODE::START_END_MID;
    case ARC_DRAW_MODE::START_END_MID:    return ARC_DRAW_MODE::START_END_CENTER;
    case ARC_DRAW_MODE::START_END_CENTER: return ARC_DRAW_MODE::TANGENT;
    case ARC_DRAW_MODE::TANGENT:          return ARC_DRAW_MODE::START_DIR_END;
    case ARC_DRAW_MODE::START_DIR_END:    return ARC_DRAW_MODE::CENTER_START_END;
    }

    return ARC_DRAW_MODE::CENTER_START_END;
}
