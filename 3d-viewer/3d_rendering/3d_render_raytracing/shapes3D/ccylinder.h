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
 * @file  ccylinder.h
 * @brief
 */

#ifndef _CCYLINDER_H_
#define _CCYLINDER_H_

#include "cobject.h"

/**
 * A vertical cylinder
 */
class  CVCYLINDER : public COBJECT
{

public:
    /**
     * Constructor CVCYLINDER
     * @param aCenterPoint = position of the center of the plane
     * @param aXSize = size by X axis
     * @param aYSize = size by Y axis
     */
    CVCYLINDER( SFVEC2F aCenterPoint, float aZmin, float aZmax, float aRadius );

    void SetColor( SFVEC3F aObjColor ) { m_diffusecolor = aObjColor; }

    // Imported from COBJECT
    bool Intersect( const RAY &aRay, HITINFO &aHitInfo ) const override;
    bool IntersectP(const RAY &aRay , float aMaxDistance ) const override;
    bool Intersects( const CBBOX &aBBox ) const override;
    SFVEC3F GetDiffuseColor( const HITINFO &aHitInfo ) const override;

private:
    SFVEC2F m_center;
    float   m_radius_squared;
    float   m_inv_radius;
    SFVEC3F m_diffusecolor;
};


#endif // _CCYLINDER_H_
