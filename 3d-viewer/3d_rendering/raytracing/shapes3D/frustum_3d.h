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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file frustum_3d.h
 * @brief A truncated cone for raytracing, used for countersink visualization.
 */

#ifndef _TRUNCATED_CONE_3D_H_
#define _TRUNCATED_CONE_3D_H_

#include "object_3d.h"

/**
 * A vertical truncated cone with different radii at top and bottom.
 * Used for visualizing countersink hole shapes.
 */
class TRUNCATED_CONE : public OBJECT_3D
{
public:
    /**
     * @param aCenterPoint = position of the vertical cone axis in the XY plane
     * @param aZmin = bottom position (Z axis)
     * @param aZmax = top position (Z axis)
     * @param aRadiusMin = radius at aZmin
     * @param aRadiusMax = radius at aZmax
     */
    TRUNCATED_CONE( SFVEC2F aCenterPoint, float aZmin, float aZmax, float aRadiusMin,
                    float aRadiusMax );

    void SetColor( SFVEC3F aObjColor ) { m_diffusecolor = aObjColor; }

    bool Intersect( const RAY& aRay, HITINFO& aHitInfo ) const override;
    bool IntersectP( const RAY& aRay, float aMaxDistance ) const override;
    bool Intersects( const BBOX_3D& aBBox ) const override;
    SFVEC3F GetDiffuseColor( const HITINFO& aHitInfo ) const override;

private:
    SFVEC2F m_center;
    float   m_radiusMin;       // Radius at Zmin
    float   m_radiusMax;       // Radius at Zmax
    float   m_zMin;
    float   m_zMax;
    float   m_height;          // zMax - zMin
    float   m_slope;           // (radiusMax - radiusMin) / height
    SFVEC3F m_diffusecolor;
};

#endif // _TRUNCATED_CONE_3D_H_
