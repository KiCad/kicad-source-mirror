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
 * @file round_segment_3d.h
 */

#ifndef _ROUND_SEGMENT_H_
#define _ROUND_SEGMENT_H_

#include "object_3d.h"

class ROUND_SEGMENT_2D;

class ROUND_SEGMENT : public OBJECT_3D
{
public:
    ROUND_SEGMENT( const ROUND_SEGMENT_2D& aSeg2D, float aZmin, float aZmax );

    void SetColor( SFVEC3F aObjColor ) { m_diffusecolor = aObjColor; }

    bool Intersect( const RAY& aRay, HITINFO& aHitInfo ) const override;
    bool IntersectP( const RAY& aRay, float aMaxDistance ) const override;
    bool Intersects( const BBOX_3D& aBBox ) const override;
    SFVEC3F GetDiffuseColor( const HITINFO& aHitInfo ) const override;

private:
    RAYSEG2D m_segment;

    SFVEC3F  m_center_left;
    SFVEC3F  m_center_right;
    SFVEC3F  m_plane_dir_left;
    SFVEC3F  m_plane_dir_right;

    float    m_radius;
    float    m_radius_squared;
    float    m_inv_radius;
    float    m_seglen_over_two_squared;

    SFVEC3F m_diffusecolor;
};


#if 0
/**
 * This is a object similar to a round segment but with a ring indside of it.
 *
 * It is used for Oblong holes
 */
class OBLONG_RING : public OBJECT_3D
{
public:
    ROUND_SEGMENT( const SFVEC2F& aStart, const SFVEC2F& aEnd, float aInnerRadius,
                   float aOuterRadius, float aZmin, float aZmax );

    // Imported from OBJECT_3D
    bool Intersect( const RAY& aRay, HITINFO& aHitInfo ) const;
    bool Intersects( const BBOX_3D& aBBox ) const;

private:
    RAYSEG2D m_segment;

    SFVEC3F  m_center_left;
    SFVEC3F  m_center_right;
    SFVEC3F  m_plane_dir_left;
    SFVEC3F  m_plane_dir_right;

    float    m_inner_radius;
    float    m_inner_radius_squared;
    float    m_inner_inv_radius;
    float    m_outer_radius;
    float    m_outer_radius_squared;
    float    m_outer_inv_radius;
    float    m_width;
    float    m_seglen_over_two_squared;
};
#endif

#endif // _ROUND_SEGMENT_H_
