/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

/**
 * @file 4pt_polygon_2d.h
 * @brief A simplified non-intersecting 4 point polygon.
 */

#ifndef _POLYGON_4PT_2D_H_
#define _POLYGON_4PT_2D_H_

#include "object_2d.h"

/**
 * Simple non-intersecting polygon with 4 points.
 *
 * Used for footprint pads. (rectangles, trapezoids, with rotation.etc).  This is a
 * simplified version of the #POLYGON_2D class.
 */
class POLYGON_4PT_2D : public OBJECT_2D
{
public:
    POLYGON_4PT_2D( const SFVEC2F& v1, const SFVEC2F& v2, const SFVEC2F& v3, const SFVEC2F& v4,
                    const BOARD_ITEM& aBoardItem );

    const SFVEC2F& GetV0() const { return m_segments[0]; }
    const SFVEC2F& GetV1() const { return m_segments[1]; }
    const SFVEC2F& GetV2() const { return m_segments[2]; }
    const SFVEC2F& GetV3() const { return m_segments[3]; }

    const SFVEC2F& GetN0() const { return m_seg_normal[0]; }
    const SFVEC2F& GetN1() const { return m_seg_normal[1]; }
    const SFVEC2F& GetN2() const { return m_seg_normal[2]; }
    const SFVEC2F& GetN3() const { return m_seg_normal[3]; }

    bool Overlaps( const BBOX_2D& aBBox ) const override;
    bool Intersects( const BBOX_2D& aBBox ) const override;
    bool Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const override;
    INTERSECTION_RESULT IsBBoxInside( const BBOX_2D& aBBox ) const override;
    bool IsPointInside( const SFVEC2F& aPoint ) const override;

private:
    SFVEC2F m_segments[4];
    SFVEC2F m_precalc_slope[4];
    SFVEC2F m_seg_normal[4];
};


#endif // _POLYGON_4PT_2D_H_
