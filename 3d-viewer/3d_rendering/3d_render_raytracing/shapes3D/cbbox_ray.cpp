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
 * @file cbbox_ray.cpp
 * @brief Bounding Box - Ray test intersection
 */

#include "cbbox.h"
#include <fctsys.h>
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


bool CBBOX::Intersect( const RAY& aRay, float* t ) const
{
    switch( aRay.m_Classification )
    {
    case MMM:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.y < m_min.y )
            || ( aRay.m_Origin.z < m_min.z )
            || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0)
            || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0)
            || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        // compute the intersection distance

        *t = (m_max.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_max.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = (m_max.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }


    case MMP:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.y < m_min.y )
            || ( aRay.m_Origin.z > m_max.z )
            || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0)
            || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0)
            || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        *t = (m_max.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_max.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = (m_min.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case MPM:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.y > m_max.y )
            || ( aRay.m_Origin.z < m_min.z )
            || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0)
            || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0)
            || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        *t = (m_max.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_min.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = (m_max.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case MPP:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.y > m_max.y )
            || ( aRay.m_Origin.z > m_max.z )
            || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0)
            || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0)
            || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        *t = (m_max.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_min.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = (m_min.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case PMM:
    {
        if( ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y < m_min.y )
            || ( aRay.m_Origin.z < m_min.z )
            || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0)
            || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0)
            || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        *t = (m_min.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_max.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = (m_max.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }


    case PMP:
    {
        if( ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y < m_min.y )
            || ( aRay.m_Origin.z > m_max.z )
            || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0)
            || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0)
            || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        *t = (m_min.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_max.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = (m_min.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case PPM:
    {
        if( ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y > m_max.y )
            || ( aRay.m_Origin.z < m_min.z )
            || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0)
            || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0)
            || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        *t = (m_min.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_min.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = (m_max.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case PPP:
    {
        if( ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y > m_max.y )
            || ( aRay.m_Origin.z > m_max.z )
            || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0)
            || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0)
            || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        *t = (m_min.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_min.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        float t2 = (m_min.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case OMM:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.z < m_min.z)
            || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0)
             )
            return false;

        *t = (m_max.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        float t2 = (m_max.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case OMP:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0)
             )
            return false;

        *t = (m_max.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        float t2 = (m_min.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case OPM:
    {
        if( ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.y > m_max.y) || ( aRay.m_Origin.z < m_min.z)
            || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0)
             )
            return false;

        *t = (m_min.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        float t2 = (m_max.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case OPP:
    {
        if( ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.y > m_max.y) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0)
             )
            return false;

        *t = (m_min.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        float t2 = (m_min.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }


    case MOM:
    {
        if( ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.z < m_min.z)
            || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        *t = (m_max.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t2 = (m_max.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }


    case MOP:
    {
        if( ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        *t = (m_max.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t2 = (m_min.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case POM:
    {
        if( ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.x > m_max.x) || ( aRay.m_Origin.z < m_min.z)
            || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        *t = (m_min.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t2 = (m_max.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }


    case POP:
    {
        if( ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.x > m_max.x) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        *t = (m_min.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t2 = (m_min.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        if( t2 > *t )
            *t = t2;

        return true;
    }

    case MMO:
    {
        if( ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.y < m_min.y)
            || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0)
             )
            return false;

        *t = (m_max.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_max.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        return true;
    }

    case MPO:
    {
        if( ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0)
             )
            return false;

        *t = (m_max.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_min.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        return true;
    }


    case PMO:
    {
        if( ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.m_Origin.x > m_max.x) || ( aRay.m_Origin.y < m_min.y)
            || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0)
             )
            return false;

        *t = (m_min.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_max.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        return true;
    }

    case PPO:
    {
        if( ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.m_Origin.x > m_max.x) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0)
             )
            return false;

        *t = (m_min.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        float t1 = (m_min.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        if( t1 > *t )
            *t = t1;

        return true;
    }


    case MOO:
    {
        if( ( aRay.m_Origin.x < m_min.x)
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
             )
            return false;

        *t = (m_max.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        return true;
    }

    case POO:
    {
        if( ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
             )
            return false;

        *t = (m_min.x - aRay.m_Origin.x) * aRay.m_InvDir.x;

        return true;
    }

    case OMO:
    {
        if( ( aRay.m_Origin.y < m_min.y)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
             )
            return false;

        *t = (m_max.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        return true;
    }

    case OPO:
    {
        if( ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
             )
            return false;

        *t = (m_min.y - aRay.m_Origin.y) * aRay.m_InvDir.y;

        return true;
    }


    case OOM:
    {
        if( ( aRay.m_Origin.z < m_min.z)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
             )
            return false;

        *t = (m_max.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        return true;
    }

    case OOP:
    {
        if( ( aRay.m_Origin.z > m_max.z)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
             )
            return false;

        *t = (m_min.z - aRay.m_Origin.z) * aRay.m_InvDir.z;

        return true;
    }
    }

    return false;
}


bool CBBOX::Intersect( const RAY& aRay ) const
{
    switch( aRay.m_Classification )
    {
    case MMM:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.y < m_min.y )
            || ( aRay.m_Origin.z < m_min.z )
            || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0)
            || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0)
            || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        return true;
    }


    case MMP:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.y < m_min.y )
            || ( aRay.m_Origin.z > m_max.z )
            || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0)
            || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0)
            || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        return true;
    }

    case MPM:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.y > m_max.y )
            || ( aRay.m_Origin.z < m_min.z )
            || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0)
            || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0)
            || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        return true;
    }

    case MPP:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.y > m_max.y )
            || ( aRay.m_Origin.z > m_max.z )
            || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0)
            || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0)
            || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        return true;
    }

    case PMM:
    {
        if( ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y < m_min.y )
            || ( aRay.m_Origin.z < m_min.z )
            || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0)
            || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0)
            || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        return true;
    }


    case PMP:
    {
        if( ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y < m_min.y )
            || ( aRay.m_Origin.z > m_max.z )
            || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0)
            || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0)
            || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        return true;
    }

    case PPM:
    {
        if( ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y > m_max.y )
            || ( aRay.m_Origin.z < m_min.z )
            || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0)
            || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0)
            || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        return true;
    }

    case PPP:
    {
        if( ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y > m_max.y )
            || ( aRay.m_Origin.z > m_max.z )
            || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0)
            || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0)
            || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        return true;
    }

    case OMM:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.z < m_min.z)
            || ( aRay.jbyk * m_min.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_max.z + aRay.c_yz > 0) )
            return false;

        return true;
    }

    case OMP:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.jbyk * m_max.z - m_max.y + aRay.c_zy > 0)
            || ( aRay.kbyj * m_min.y - m_min.z + aRay.c_yz < 0)
             )
            return false;

        return true;
    }

    case OPM:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y > m_max.y) || ( aRay.m_Origin.z < m_min.z)
            || ( aRay.jbyk * m_min.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_max.z + aRay.c_yz > 0)
             )
            return false;

        return true;
    }

    case OPP:
    {
        if( ( aRay.m_Origin.x < m_min.x )
            || ( aRay.m_Origin.x > m_max.x )
            || ( aRay.m_Origin.y > m_max.y) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.jbyk * m_max.z - m_min.y + aRay.c_zy < 0)
            || ( aRay.kbyj * m_max.y - m_min.z + aRay.c_yz < 0)
             )
            return false;

        return true;
    }


    case MOM:
    {
        if( ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.z < m_min.z)
            || ( aRay.kbyi * m_min.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        return true;
    }


    case MOP:
    {
        if( ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.kbyi * m_min.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_max.x + aRay.c_zx > 0)
             )
            return false;

        return true;
    }

    case POM:
    {
        if( ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.x > m_max.x) || ( aRay.m_Origin.z < m_min.z)
            || ( aRay.kbyi * m_max.x - m_max.z + aRay.c_xz > 0)
            || ( aRay.ibyk * m_min.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        return true;
    }


    case POP:
    {
        if( ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.x > m_max.x) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.kbyi * m_max.x - m_min.z + aRay.c_xz < 0)
            || ( aRay.ibyk * m_max.z - m_min.x + aRay.c_zx < 0)
             )
            return false;

        return true;
    }

    case MMO:
    {
        if( ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.y < m_min.y)
            || ( aRay.jbyi * m_min.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_max.x + aRay.c_yx > 0)
             )
            return false;

        return true;
    }

    case MPO:
    {
        if( ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.jbyi * m_min.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_max.x + aRay.c_yx > 0)
             )
            return false;

        return true;
    }


    case PMO:
    {
        if( ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.m_Origin.x > m_max.x) || ( aRay.m_Origin.y < m_min.y)
            || ( aRay.jbyi * m_max.x - m_max.y + aRay.c_xy > 0)
            || ( aRay.ibyj * m_min.y - m_min.x + aRay.c_yx < 0)
             )
            return false;

        return true;
    }

    case PPO:
    {
        if( ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
            || ( aRay.m_Origin.x > m_max.x) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.jbyi * m_max.x - m_min.y + aRay.c_xy < 0)
            || ( aRay.ibyj * m_max.y - m_min.x + aRay.c_yx < 0)
             )
            return false;

        return true;
    }


    case MOO:
    {
        if( ( aRay.m_Origin.x < m_min.x)
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
             )
            return false;

        return true;
    }

    case POO:
    {
        if( ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
             )
            return false;

        return true;
    }

    case OMO:
    {
        if( ( aRay.m_Origin.y < m_min.y)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
             )
            return false;

        return true;
    }

    case OPO:
    {
        if( ( aRay.m_Origin.y > m_max.y)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.z < m_min.z) || ( aRay.m_Origin.z > m_max.z)
             )
            return false;

        return true;
    }


    case OOM:
    {
        if( ( aRay.m_Origin.z < m_min.z)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
             )
            return false;

        return true;
    }

    case OOP:
    {
        if( ( aRay.m_Origin.z > m_max.z)
            || ( aRay.m_Origin.x < m_min.x) || ( aRay.m_Origin.x > m_max.x)
            || ( aRay.m_Origin.y < m_min.y) || ( aRay.m_Origin.y > m_max.y)
             )
            return false;

        return true;
    }
    }

    return false;
}
