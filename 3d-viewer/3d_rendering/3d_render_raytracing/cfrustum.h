/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  cfrustum.h
 * @brief
 */

#ifndef _CFRUSTUM_H_
#define _CFRUSTUM_H_

#include "plugins/3dapi/xv3d_types.h"
#include "shapes3D/cbbox.h"
#include "ray.h"

// !TODO: optimize wih SSE
//#if(GLM_ARCH != GLM_ARCH_PURE)
#if 0
#error not implemented
#else
GLM_ALIGNED_STRUCT(CLASS_ALIGNMENT) CFRUSTUM
{
    SFVEC3F m_normals[4];
    SFVEC3F m_point[4];

    void GenerateFrustum( const RAY &topLeft, const RAY &topRight, const RAY &bottomLeft, const RAY &bottomRight );
    bool Intersect( const CBBOX &aBBox ) const;
};
#endif


#endif // _CFRUSTUM_H_
