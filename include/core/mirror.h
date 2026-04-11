/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

enum class FLIP_DIRECTION
{
    LEFT_RIGHT, ///< Flip left to right (around the Y axis)
    TOP_BOTTOM  ///< Flip top to bottom (around the X axis)
};

/**
 *  Returns the mirror of aPoint relative to the aMirrorRef.
 */
template <typename T>
constexpr T MIRRORVAL( T aPoint, T aMirrorRef )
{
    return -( aPoint - aMirrorRef ) + aMirrorRef;
}

/**
 *  Updates aPoint with the mirror of aPoint relative to the aMirrorRef.
 */
template <typename T>
constexpr void MIRROR( T& aPoint, const T& aMirrorRef )
{
    aPoint = MIRRORVAL( aPoint, aMirrorRef );
}


/**
 * Updates aPoint with the mirror of aPoint relative to the aMirrorRef,
 * in the specified direction.
 */
template <typename VT>
constexpr void MIRROR( VT& aPoint, const VT& aMirrorRef, FLIP_DIRECTION aFlipDirection )
{
    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
        MIRROR( aPoint.x, aMirrorRef.x );
    else
        MIRROR( aPoint.y, aMirrorRef.y );
}
