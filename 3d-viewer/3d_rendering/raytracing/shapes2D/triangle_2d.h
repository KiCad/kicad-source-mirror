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
 * @file triangle_2d.h
 */

#ifndef _TRIANGLE_2D_H_
#define _TRIANGLE_2D_H_

#include "object_2d.h"
#include "../accelerators/container_2d.h"
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

class TRIANGLE_2D : public OBJECT_2D
{
public:
    TRIANGLE_2D( const SFVEC2F& aV1, const SFVEC2F& aV2, const SFVEC2F& aV3,
                 const BOARD_ITEM& aBoardItem );

    const SFVEC2F& GetP1() const { return p1; }
    const SFVEC2F& GetP2() const { return p2; }
    const SFVEC2F& GetP3() const { return p3; }

    bool Overlaps( const BBOX_2D& aBBox ) const override;
    bool Intersects( const BBOX_2D& aBBox ) const override;
    bool Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const override;
    INTERSECTION_RESULT IsBBoxInside( const BBOX_2D& aBBox ) const override;
    bool IsPointInside( const SFVEC2F& aPoint ) const override;

private:
    SFVEC2F p1;
    SFVEC2F p2;
    SFVEC2F p3;

    float m_inv_denominator;
    float m_p2y_minus_p3y;
    float m_p3x_minus_p2x;
    float m_p3y_minus_p1y;
    float m_p1x_minus_p3x;
};


void ConvertPolygonToTriangles( const SHAPE_POLY_SET& aPolyList, CONTAINER_2D_BASE& aDstContainer,
                                float aBiuTo3dUnitsScale, const BOARD_ITEM& aBoardItem );
#endif // _TRIANGLE_2D_H_
