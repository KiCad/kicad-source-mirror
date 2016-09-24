/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  cfilledcircle2d.h
 * @brief
 */

#ifndef _CFILLEDCIRCLE2D_H_
#define _CFILLEDCIRCLE2D_H_

#include "cobject2d.h"

class  CFILLEDCIRCLE2D : public COBJECT2D
{
public:
    float GetRadius() const { return m_radius; }
    const SFVEC2F &GetCenter() const { return m_center; }
    float GetRadiusSquared() const { return m_radius_squared; }

private:
    SFVEC2F m_center;
    float   m_radius;
    float   m_radius_squared;

public:
    CFILLEDCIRCLE2D( const SFVEC2F &aCenter, float aRadius, const BOARD_ITEM &aBoardItem );

    // Imported from COBJECT2D
    bool Overlaps( const CBBOX2D &aBBox ) const override;
    bool Intersects( const CBBOX2D &aBBox ) const override;
    bool Intersect( const RAYSEG2D &aSegRay, float *aOutT, SFVEC2F *aNormalOut ) const override;
    INTERSECTION_RESULT IsBBoxInside( const CBBOX2D &aBBox ) const override;
    bool IsPointInside( const SFVEC2F &aPoint ) const override;
};


#endif // _CFILLEDCIRCLE2D_H_
