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
 * @file bbox_3d_ray.cpp
 * @brief 3D bounding box ray intersection tests.
 */

#include "bbox_3d.h"
#include "../ray.h"
#include <wx/debug.h>

// This BBOX Ray intersection test have the following credits:

// "This source code accompanies the Journal of Graphics Tools paper:
//
// "Fast Ray / Axis-Aligned Bounding Box Overlap Tests using Ray Slopes"
// by Martin Eisemann, Thorsten Grosch, Stefan Müller and Marcus Magnor
// Computer Graphics Lab, TU Braunschweig, Germany and
// University of Koblenz-Landau, Germany
//
// This source code is public domain, but please mention us if you use it."


bool BBOX_3D::Intersect( const RAY& aRay, float* t ) const
{
    switch( aRay.m_Classification )
    {
    case RAY_CLASSIFICATION::MMM:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0 )
          || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0 )
          || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        // compute the intersection distance

        *t = ( m_max.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_max.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = ( m_max.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::MMP:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0 )
          || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0 )
          || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        *t = ( m_max.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_max.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = ( m_min.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::MPM:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0 )
          || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0 )
          || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        *t = ( m_max.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_min.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = ( m_max.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::MPP:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0 )
          || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0 )
          || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        *t = ( m_max.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_min.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = ( m_min.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::PMM:
    {
        if( ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0 )
          || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0 )
          || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        *t = ( m_min.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_max.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = ( m_max.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::PMP:
    {
        if( ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0 )
          || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0 )
          || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        *t = ( m_min.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_max.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = ( m_min.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::PPM:
    {
        if( ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0 )
          || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0 )
          || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        *t = ( m_min.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_min.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = ( m_max.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::PPP:
    {
        if( ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0 )
          || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0 )
          || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        *t = ( m_min.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_min.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = ( m_min.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::OMM:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.x > m_max.x )
          || ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0 ) )
            return false;

        *t = ( m_max.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        float t2 = ( m_max.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::OMP:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.x > m_max.x )
          || ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0 ) )
            return false;

        *t = ( m_max.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        float t2 = ( m_min.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::OPM:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.x > m_max.x )
          || ( aRay.m_Origin.y > m_max.y ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0 ) )
            return false;

        *t = ( m_min.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        float t2 = ( m_max.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::OPP:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.x > m_max.x )
          || ( aRay.m_Origin.y > m_max.y ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0 ) )
            return false;

        *t = ( m_min.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        float t2 = ( m_min.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::MOM:
    {
        if( ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        *t = ( m_max.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t2 = ( m_max.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::MOP:
    {
        if( ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        *t = ( m_max.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t2 = ( m_min.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::POM:
    {
        if( ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        *t = ( m_min.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t2 = ( m_max.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::POP:
    {
        if( ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        *t = ( m_min.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t2 = ( m_min.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case RAY_CLASSIFICATION::MMO:
    {
        if( ( aRay.m_Origin.z < m_min.z ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0 ) )
            return false;

        *t = ( m_max.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_max.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        return true;
    }

    case RAY_CLASSIFICATION::MPO:
    {
        if( ( aRay.m_Origin.z < m_min.z ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0 ) )
            return false;

        *t = ( m_max.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_min.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        return true;
    }

    case RAY_CLASSIFICATION::PMO:
    {
        if( ( aRay.m_Origin.z < m_min.z ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0 ) )
            return false;

        *t = ( m_min.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_max.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        return true;
    }

    case RAY_CLASSIFICATION::PPO:
    {
        if( ( aRay.m_Origin.z < m_min.z ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0 ) )
            return false;

        *t = ( m_min.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        float t1 = ( m_min.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        return true;
    }

    case RAY_CLASSIFICATION::MOO:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.y > m_max.y ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.m_Origin.z > m_max.z ) )
            return false;

        *t = ( m_max.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        return true;
    }

    case RAY_CLASSIFICATION::POO:
    {
        if( ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.y > m_max.y ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.m_Origin.z > m_max.z ) )
            return false;

        *t = ( m_min.x - aRay.m_Origin.x ) * aRay.m_InvDir.x;

        return true;
    }

    case RAY_CLASSIFICATION::OMO:
    {
        if( ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.x < m_min.x )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.m_Origin.z > m_max.z ) )
            return false;

        *t = ( m_max.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        return true;
    }

    case RAY_CLASSIFICATION::OPO:
    {
        if( ( aRay.m_Origin.y > m_max.y ) || ( aRay.m_Origin.x < m_min.x )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.m_Origin.z > m_max.z ) )
            return false;

        *t = ( m_min.y - aRay.m_Origin.y ) * aRay.m_InvDir.y;

        return true;
    }

    case RAY_CLASSIFICATION::OOM:
    {
        if( ( aRay.m_Origin.z < m_min.z ) || ( aRay.m_Origin.x < m_min.x )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.y > m_max.y ) )
            return false;

        *t = ( m_max.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        return true;
    }

    case RAY_CLASSIFICATION::OOP:
    {
        if( ( aRay.m_Origin.z > m_max.z ) || ( aRay.m_Origin.x < m_min.x )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.y > m_max.y ) )
            return false;

        *t = ( m_min.z - aRay.m_Origin.z ) * aRay.m_InvDir.z;

        return true;
    }
    }

    return false;
}


bool BBOX_3D::Intersect( const RAY& aRay ) const
{
    switch( aRay.m_Classification )
    {
    case RAY_CLASSIFICATION::MMM:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0 )
          || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0 )
          || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::MMP:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0 )
          || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0 )
          || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::MPM:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0 )
          || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0 )
          || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::MPP:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0 )
          || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0 )
          || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::PMM:
    {
        if( ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0 )
          || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0 )
          || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        return true;
    }


    case RAY_CLASSIFICATION::PMP:
    {
        if( ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0 )
          || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0 )
          || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::PPM:
    {
        if( ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0 )
          || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0 )
          || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::PPP:
    {
        if( ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0 )
          || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0 )
          || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::OMM:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.x > m_max.x )
          || ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::OMP:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.x > m_max.x )
          || ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0 )
          || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::OPM:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.x > m_max.x )
          || ( aRay.m_Origin.y > m_max.y ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::OPP:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.x > m_max.x )
          || ( aRay.m_Origin.y > m_max.y ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0 )
          || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::MOM:
    {
        if( ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::MOP:
    {
        if( ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::POM:
    {
        if( ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0 )
          || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::POP:
    {
        if( ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0 )
          || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::MMO:
    {
        if( ( aRay.m_Origin.z < m_min.z ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::MPO:
    {
        if( ( aRay.m_Origin.z < m_min.z ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::PMO:
    {
        if( ( aRay.m_Origin.z < m_min.z ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0 )
          || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::PPO:
    {
        if( ( aRay.m_Origin.z < m_min.z ) || ( aRay.m_Origin.z > m_max.z )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y > m_max.y )
          || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0 )
          || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0 ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::MOO:
    {
        if( ( aRay.m_Origin.x < m_min.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.y > m_max.y ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.m_Origin.z > m_max.z ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::POO:
    {
        if( ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.y > m_max.y ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.m_Origin.z > m_max.z ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::OMO:
    {
        if( ( aRay.m_Origin.y < m_min.y ) || ( aRay.m_Origin.x < m_min.x )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.m_Origin.z > m_max.z ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::OPO:
    {
        if( ( aRay.m_Origin.y > m_max.y ) || ( aRay.m_Origin.x < m_min.x )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.z < m_min.z )
          || ( aRay.m_Origin.z > m_max.z ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::OOM:
    {
        if( ( aRay.m_Origin.z < m_min.z ) || ( aRay.m_Origin.x < m_min.x )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.y > m_max.y ) )
            return false;

        return true;
    }

    case RAY_CLASSIFICATION::OOP:
    {
        if( ( aRay.m_Origin.z > m_max.z ) || ( aRay.m_Origin.x < m_min.x )
          || ( aRay.m_Origin.x > m_max.x ) || ( aRay.m_Origin.y < m_min.y )
          || ( aRay.m_Origin.y > m_max.y ) )
            return false;

        return true;
    }
    }

    return false;
}
