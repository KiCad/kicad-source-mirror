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
 * @file frustum.h
 * @brief Implement a frustum that is used for ray packet tests.
 */

#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include "shapes3D/bbox_3d.h"
#include "ray.h"

// !TODO: optimize with SSE
//#if(GLM_ARCH != GLM_ARCH_PURE)
#if 0
#error not implemented
#else
struct FRUSTUM
{
public:
    void GenerateFrustum( const RAY& topLeft, const RAY& topRight, const RAY& bottomLeft,
                          const RAY& bottomRight );

    /**
     * Intersect \a aBBox with this frustum.
     *
     * @param aBBox is a bounding box to test.
     * @return true if the bounding box intersects this frustum.
     */
    bool Intersect( const BBOX_3D& aBBox ) const;

private:
        SFVEC3F m_normals[4];
        SFVEC3F m_point[4];
};
#endif


#endif // _FRUSTUM_H_
