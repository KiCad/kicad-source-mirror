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
 * @file  ogl_legacy_utils.h
 * @brief
 */

#ifndef OGL_LEGACY_UTILS_H_
#define OGL_LEGACY_UTILS_H_

#include "../3d_render_raytracing/shapes3D/cbbox.h"
#include "../3d_render_raytracing/shapes2D/croundsegment2d.h"

/**
 * @brief OGL_draw_arrow - draw a round arrow
 * @param aPosition: start position of the arrow
 * @param aTargetPos: end position of the arror
 * @param aSize: diameter size
 */
void OGL_draw_arrow( SFVEC3F aPosition, SFVEC3F aTargetPos, float aSize );


/**
 * @brief OGL_draw_bbox - draw the bounding box lines
 * @param aBBox
 */
void OGL_draw_bbox( const CBBOX &aBBox );


/**
 * @brief OGL_draw_half_open_cylinder - draws an open half cylinder
 * with diameter 1.0f and Height 1.0f
 * the bottom center is at (0,0,0) and top center is at (0,0,1)
 */
void OGL_draw_half_open_cylinder( unsigned int aNrSidesPerCircle );


/**
 * @brief OGL_Draw_segment
 * @param aSegment
 */
void OGL_Draw_segment( const CROUNDSEGMENT2D &aSegment,
                       unsigned int aNrSidesPerCircle );

#endif // OGL_LEGACY_UTILS_H_
