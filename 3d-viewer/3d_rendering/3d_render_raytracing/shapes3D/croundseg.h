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
 * @file  croundseg.h
 * @brief
 */

#ifndef _CROUNDSEG_H_
#define _CROUNDSEG_H_

#include "cobject.h"
#include "../shapes2D/croundsegment2d.h"

/**
 *
 */
class  CROUNDSEG : public COBJECT
{

public:
    /**
     * Constructor CROUNDSEG
     */
    CROUNDSEG( const CROUNDSEGMENT2D &aSeg2D, float aZmin, float aZmax );

    void SetColor( SFVEC3F aObjColor ) { m_diffusecolor = aObjColor; }

    // Imported from COBJECT
    bool Intersect( const RAY &aRay, HITINFO &aHitInfo ) const override;
    bool IntersectP( const RAY &aRay, float aMaxDistance ) const override;
    bool Intersects( const CBBOX &aBBox ) const override;
    SFVEC3F GetDiffuseColor( const HITINFO &aHitInfo ) const override;

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
 * This is a object similar to a round segment but with a ring
 * It is used for Oblong holes
 */
class  COBLONGRING : public COBJECT
{
public:
    /**
     * Constructor CROUNDSEG
     */
    CROUNDSEG( const SFVEC2F &aStart,
               const SFVEC2F &aEnd,
               float aInnerRadius,
               float aOutterRadius,
               float aZmin,
               float aZmax );

    // Imported from COBJECT
    bool Intersect( const RAY &aRay, HITINFO &aHitInfo ) const;
    bool Intersects( const CBBOX &aBBox ) const;

private:
    RAYSEG2D m_segment;

    SFVEC3F  m_center_left;
    SFVEC3F  m_center_right;
    SFVEC3F  m_plane_dir_left;
    SFVEC3F  m_plane_dir_right;

    float    m_inner_radius;
    float    m_inner_radius_squared;
    float    m_inner_inv_radius;
    float    m_outter_radius;
    float    m_outter_radius_squared;
    float    m_outter_inv_radius;
    float    m_width;
    float    m_seglen_over_two_squared;
};
#endif
#endif // _CROUNDSEG_H_
