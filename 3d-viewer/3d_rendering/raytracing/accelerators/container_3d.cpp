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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file container_3d.cpp
 */

#include "container_3d.h"
#include <cstdio>

CONTAINER_3D_BASE::CONTAINER_3D_BASE()
{
    m_objects.clear();
    m_bbox.Reset();
}


void CONTAINER_3D_BASE::Clear()
{
    if( !m_objects.empty() )
    {
        for( OBJECT_3D* object : m_objects )
            delete object;

        m_objects.clear();
    }

    m_bbox.Reset();
}


CONTAINER_3D_BASE::~CONTAINER_3D_BASE()
{
    Clear();
}


void CONTAINER_3D_BASE::ConvertTo( std::vector<const OBJECT_3D*> &aOutVector ) const
{
    aOutVector.resize( m_objects.size() );

    if( !m_objects.empty() )
    {
        unsigned int i = 0;

        for( const OBJECT_3D* object : m_objects )
        {
            wxASSERT( object );
            aOutVector[i++] = object;
        }
    }
}


bool CONTAINER_3D::Intersect( const RAY &aRay, HITINFO &aHitInfo ) const
{

    if( !m_bbox.Intersect( aRay ) )
        return false;

    bool hitted = false;

    for( const OBJECT_3D* object : m_objects )
    {
        if( object->Intersect( aRay, aHitInfo) )
            hitted = true;
    }

    return hitted;
}


bool CONTAINER_3D::IntersectP( const RAY &aRay, float aMaxDistance ) const
{
    for( const OBJECT_3D* object : m_objects )
    {
        if( object->IntersectP( aRay, aMaxDistance ) )
            return true;
    }

    return false;
}
