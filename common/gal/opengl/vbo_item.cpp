/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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
 * @file vbo_item.cpp
 * @brief Class to handle an item held in a Vertex Buffer Object.
 */

#include <gal/opengl/vbo_item.h>
#include <cstring>

using namespace KiGfx;

VBO_ITEM::VBO_ITEM() :
        m_vertices( NULL ),
        m_indices( NULL ),
        m_offset( 0 ),
        m_size( 0 ),
        m_isDirty( true )
{
}


VBO_ITEM::~VBO_ITEM()
{
    if( m_vertices )
        delete m_vertices;

    if( m_indices )
        delete m_indices;
}


void VBO_ITEM::PushVertex( const GLfloat* aVertex )
{
    GLfloat* newVertices = new GLfloat[( m_size + 1 ) * VertStride];
    GLuint*  newIndices  = new GLuint[( m_size + 1 ) * IndStride];

    // Handle a new vertex
    if( m_vertices )
    {
        // Copy all previous vertices data
        memcpy( newVertices, m_vertices, m_size * VertSize );
        delete m_vertices;
    }
    m_vertices = newVertices;

    // Add the new vertex
    memcpy( &newVertices[m_size * VertStride], aVertex, VertSize );

    // Handle a new index
    if( m_indices )
    {
        // Copy all previous vertices data
        memcpy( newIndices, m_indices, m_size * IndSize );
        delete m_indices;
    }
    m_indices = newIndices;

    // Add the new vertex
    m_indices[m_size] = m_offset + m_size;

    m_size++;
    m_isDirty = true;
}


void VBO_ITEM::PushVertices( const GLfloat* aVertices, GLuint aSize )
{
    int newSize = m_size + aSize;
    GLfloat* newVertices = new GLfloat[newSize * VertStride];
    GLuint*  newIndices  = new GLuint[newSize * IndStride];

    // Handle new vertices
    if( m_vertices )
    {
        // Copy all previous vertices data
        memcpy( newVertices, m_vertices, ( m_size ) * VertSize );
        delete m_vertices;
    }
    m_vertices = newVertices;

    // Add new vertices
    memcpy( &newVertices[m_size * VertStride], aVertices, aSize * VertSize );

    // Handle new indices
    if( m_indices )
    {
        // Copy all previous vertices data
        memcpy( newIndices, m_indices, ( m_size ) * IndSize );
        delete m_indices;
    }
    m_indices = newIndices;

    // Add the new vertex
    for( int i = m_size; i < newSize; ++i )
        m_indices[i] = m_offset + i;

    m_size += aSize;
    m_isDirty = true;
}


GLfloat* VBO_ITEM::GetVertices() const
{
    return m_vertices;
}


GLuint* VBO_ITEM::GetIndices() const
{
    return m_indices;
}


int VBO_ITEM::GetSize() const
{
    return m_size;
}


void VBO_ITEM::SetOffset( int aOffset )
{
    if( m_offset == aOffset )
        return;

    int delta = aOffset - m_offset;

    // Change offset for all the stored indices
    for( int i = 0; i < m_size; ++i )
    {
        m_indices += delta;
    }

    m_offset = aOffset;
}


int VBO_ITEM::GetOffset() const
{
    return m_offset;
}


/*
// TODO
void SetVbo( int aVboId )
{
}


int GetVbo() const
{
}
*/
