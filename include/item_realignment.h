/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <optional>
#include <vector>

#include <geometry/eda_angle.h>
#include <math/vector2d.h>


/**
 * This is a general class that takes two lists of points, and finds the best fit translation
 * and rotation to align them.
 *
 * For example, this can be used to compensate for a library component being moved and/or
 * rotated.
 *
 * There are two parts to this process:
 *   - Identify matching points in the "old" and "new" states
 *   - Compute a transform that best aligns the two sets of points
 *
 * This interface is the interface only for the second part - identifying matching points
 * is a domain-specific problem that the caller will need to handle.
 */
class ITEM_REALIGNER_BASE
{
public:
    struct TRANSFORM
    {
        EDA_ANGLE m_Rotation;
        VECTOR2I  m_Translation;
    };

    /**
     * Compute the best fit transform to align the two sets of points.
     *
     * The point lists have to be the same length.
     *
     * @param aPtsA The "old" points
     * @param aPtsB The "new" points
     * @return The transform to apply to the NEW points to best align them to the old points.
     * Rotations are to be applied around the origin.
     */
    virtual std::optional<TRANSFORM> GetTransform( const std::vector<VECTOR2I>& aPtsA,
                                                   const std::vector<VECTOR2I>& aPtsB ) const = 0;
};

/**
 * This is a relatively straight-forward implementation of the ITEM_REALIGNER_BASE that
 * should handle most practical cases.
 *
 * Basically, it computes a baseline between two matching points. This gives the approximate rotation,
 * which is then snapped to a multiple of 90 degrees. After rotation, the translation is
 * computed by a best fit between the two sets of points.
 *
 * By constraining the baseline rotation to 90 degree increments, this avoids various ways
 * the result gets numerically strange. For example if one baseline pad is adjusted by a small
 * amount, even if the footprint isn't rotated, the baseline may shift by a small angle.
 * This doesn't mean we should rotate the footprint.
 */
class ORTHO_ITEM_REALIGNER : public ITEM_REALIGNER_BASE
{
public:
    std::optional<TRANSFORM> GetTransform( const std::vector<VECTOR2I>& aPtsA,
                                           const std::vector<VECTOR2I>& aPtsB ) const override;
};