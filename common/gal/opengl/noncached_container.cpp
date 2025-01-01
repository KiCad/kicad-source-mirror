/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * @file noncached_container.cpp
 * @brief Class to store instances of VERTEX without caching. It allows a fast one-frame drawing
 * and then clearing the buffer and starting from scratch.
 */

#include <gal/opengl/noncached_container.h>
#include <cstring>
#include <cstdlib>

using namespace KIGFX;

NONCACHED_CONTAINER::NONCACHED_CONTAINER( unsigned int aSize ) :
        VERTEX_CONTAINER( aSize ),
        m_freePtr( 0 )
{
    m_vertices = static_cast<VERTEX*>( malloc( aSize * sizeof( VERTEX ) ) );

    // Unfortunately we cannot remove the use of malloc here because realloc is used in
    // the Allocate method below.  The new operator behavior is mimicked here so that a
    // malloc failure can be caught in the OpenGL initialization code further up the stack.
    if( !m_vertices )
        throw std::bad_alloc();

    memset( m_vertices, 0x00, aSize * sizeof( VERTEX ) );
}


NONCACHED_CONTAINER::~NONCACHED_CONTAINER()
{
    free( m_vertices );
}


void NONCACHED_CONTAINER::SetItem( VERTEX_ITEM* aItem )
{
    // Nothing has to be done, as the noncached container
    // does not care about VERTEX_ITEMs ownership
}


VERTEX* NONCACHED_CONTAINER::Allocate( unsigned int aSize )
{
    if( m_freeSpace < aSize )
    {
        // Double the space
        VERTEX* newVertices =
                static_cast<VERTEX*>( realloc( m_vertices, m_currentSize * 2 * sizeof( VERTEX ) ) );

        if( newVertices != nullptr )
        {
            m_vertices = newVertices;
            m_freeSpace += m_currentSize;
            m_currentSize *= 2;
        }
        else
        {
            throw std::bad_alloc();
        }
    }

    VERTEX* freeVertex = &m_vertices[m_freePtr];

    // Move to the next free chunk
    m_freePtr += aSize;
    m_freeSpace -= aSize;

    return freeVertex;
}


void NONCACHED_CONTAINER::Clear()
{
    m_freePtr = 0;
    m_freeSpace = m_currentSize;
}
