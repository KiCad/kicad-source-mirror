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
 * @file plane_3d.h
 */

#ifndef _CPLANE_H_
#define _CPLANE_H_

#include "object_3d.h"

/**
 * A plane that is parallel to XY plane
 */
class XY_PLANE : public OBJECT_3D
{
public:
    explicit XY_PLANE( const BBOX_3D& aBBox );

    /**
     * @param aCenterPoint = position of the center of the plane
     * @param aXSize = size by X axis
     * @param aYSize = size by Y axis
     */
    XY_PLANE( SFVEC3F aCenterPoint, float aXSize, float aYSize );

    void SetColor( SFVEC3F aObjColor ) { m_diffusecolor = aObjColor; }

    bool Intersect( const RAY& aRay, HITINFO& aHitInfo ) const override;
    bool IntersectP(const RAY& aRay, float aMaxDistance ) const override;
    bool Intersects( const BBOX_3D& aBBox ) const override;
    SFVEC3F GetDiffuseColor( const HITINFO& aHitInfo ) const override;

private:
    SFVEC3F m_centerPoint;
    float m_xsize;
    float m_ysize;
    float m_xsize_inv2;
    float m_ysize_inv2;
    SFVEC3F m_diffusecolor;
};


#endif // _CPLANE_H_
