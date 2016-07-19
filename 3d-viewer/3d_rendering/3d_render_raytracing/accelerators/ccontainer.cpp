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
 * @file  ccontainer.cpp
 * @brief
 */

#include "ccontainer.h"
#include <stdio.h>

CGENERICCONTAINER::CGENERICCONTAINER()
{
    m_objects.clear();
    m_bbox.Reset();
}

void CGENERICCONTAINER::Clear()
{
    if( !m_objects.empty() )
    {
        for( LIST_OBJECT::iterator ii = m_objects.begin();
             ii != m_objects.end();
             ++ii )
        {
            delete *ii;
            *ii = NULL;
        }

        m_objects.clear();
    }

    m_bbox.Reset();
}


CGENERICCONTAINER::~CGENERICCONTAINER()
{
    Clear();
}


void CGENERICCONTAINER::ConvertTo( CONST_VECTOR_OBJECT &aOutVector ) const
{
    aOutVector.resize( m_objects.size() );

    if( !m_objects.empty() )
    {
        unsigned int i = 0;

        for( LIST_OBJECT::const_iterator ii = m_objects.begin();
             ii != m_objects.end();
             ++ii )
        {
            wxASSERT( (*ii) != NULL );

            aOutVector[i++] = static_cast<const COBJECT *>(*ii);
        }
    }
}


bool CCONTAINER::Intersect( const RAY &aRay, HITINFO &aHitInfo ) const
{

    if( !m_bbox.Intersect( aRay ) )
        return false;

    bool hitted = false;

    for( LIST_OBJECT::const_iterator ii = m_objects.begin();
         ii != m_objects.end();
         ++ii )
    {
        const COBJECT *object = static_cast<const COBJECT *>(*ii);

        if( object->Intersect( aRay, aHitInfo) )
            hitted = true;
    }

    return hitted;
}


bool CCONTAINER::IntersectP( const RAY &aRay, float aMaxDistance ) const
{
/*
    if( !m_bbox.Inside( aRay.m_Origin ) )
        if( !m_bbox.Intersect( aRay ) )
            return false;
*/
    for( LIST_OBJECT::const_iterator ii = m_objects.begin();
         ii != m_objects.end();
         ++ii )
    {
        const COBJECT *object = static_cast<const COBJECT *>(*ii);

        if( object->IntersectP( aRay, aMaxDistance ) )
            return true;
    }

    return false;
}
