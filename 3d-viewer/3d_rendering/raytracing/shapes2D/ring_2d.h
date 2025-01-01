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
 * @file ring_2d.h
 */

#ifndef _RING_2D_H_
#define _RING_2D_H_

#include "object_2d.h"

class RING_2D : public OBJECT_2D
{
public:
    RING_2D( const SFVEC2F& aCenter, float aInnerRadius, float aOuterRadius,
             const BOARD_ITEM& aBoardItem );

    bool Overlaps( const BBOX_2D& aBBox ) const override;
    bool Intersects( const BBOX_2D& aBBox ) const override;
    bool Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const override;
    INTERSECTION_RESULT IsBBoxInside( const BBOX_2D& aBBox ) const override;
    bool IsPointInside( const SFVEC2F& aPoint ) const override;

    const SFVEC2F& GetCenter() const { return m_center; }
    float GetInnerRadius() const { return m_inner_radius; }
    float GetOuterRadius() const { return m_outer_radius; }

    float GetInnerRadiusSquared() const { return m_inner_radius_squared; }
    float GetOuterRadiusSquared() const { return m_outer_radius_squared; }

private:
    SFVEC2F m_center;
    float   m_inner_radius;
    float   m_outer_radius;
    float   m_inner_radius_squared;
    float   m_outer_radius_squared;
};


#endif // _RING_2D_H_
