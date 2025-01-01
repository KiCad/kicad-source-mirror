/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 */

#pragma once

#include <optional>

#include <geometry/seg.h>
#include <geometry/shape_arc.h>

/**
 * Parameters that define a simple chamfer operation.
 */
struct CHAMFER_PARAMS
{
    /// Chamfer set-back distance along the first line
    int m_chamfer_setback_a;
    /// Chamfer set-back distance along the second line
    int m_chamfer_setback_b;
};

struct CHAMFER_RESULT
{
    // The chamfer segment
    SEG m_chamfer;

    // The updated original segments
    // These can be empty if the chamfer "consumed" "he original segments
    std::optional<SEG> m_updated_seg_a;
    std::optional<SEG> m_updated_seg_b;
};

/**
 * Compute the chamfer points for a given line pair and chamfer parameters.
 */
std::optional<CHAMFER_RESULT> ComputeChamferPoints( const SEG& aSegA, const SEG& aSegB,
                                                    const CHAMFER_PARAMS& aChamferParams );

struct DOGBONE_RESULT
{
    SHAPE_ARC m_arc;

    // The updated original segments
    // These can be empty if the dogbone "consumed" the original segments
    std::optional<SEG> m_updated_seg_a;
    std::optional<SEG> m_updated_seg_b;

    // If the arc mouth is smaller than the radius
    bool m_small_arc_mouth;
};

/**
 * Compute the dogbone geometry for a given line pair and dogbone parameters.
 *
 * This turns a sharp internal corner into a "dogbone" shape, which allows it to be
 * routed with a cutter that has a radius, and still allows a sharp external corner
 * to fit into the space.
 *
 *        _----_
 * ------/      |
 *  ---------+  /
 *           | |
 *           | |
 */
std::optional<DOGBONE_RESULT> ComputeDogbone( const SEG& aSegA, const SEG& aSegB,
                                              int aDogboneRadius, bool aAddSlots );
