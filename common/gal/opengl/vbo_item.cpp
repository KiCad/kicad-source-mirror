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
        m_offset( 0 ),
        m_size( 0 ),
        m_isDirty( true ),
        m_transform( NULL )
{
    // By default no shader is used
    m_shader[0] = 0;

    // Prepare a block for storing vertices & indices
    useNewBlock();
}


VBO_ITEM::~VBO_ITEM()
{
    if( m_isDirty )
    {
        // Data is still stored in blocks
        std::list<VBO_VERTEX*>::const_iterator v_it, v_end;
        for( v_it = m_vertBlocks.begin(), v_end = m_vertBlocks.end(); v_it != v_end; ++v_it )
            delete[] *v_it;
    }

    if( m_vertices )
        delete m_vertices;
}


void VBO_ITEM::PushVertex( const GLfloat* aVertex )
{
    if( m_spaceLeft == 0 )
        useNewBlock();

    if( m_transform != NULL )
    {
        // Apply transformations
        //                X,          Y,          Z coordinates
        glm::vec4 vertex( aVertex[0], aVertex[1], aVertex[2], 1.0f );
        vertex = *m_transform * vertex;

        // Replace only coordinates, leave color as it is
        memcpy( &m_vertPtr->x, &vertex[0], CoordByteSize );
    }
    else
    {
        // Add the new vertex
        memcpy( &m_vertPtr->x, aVertex, CoordByteSize );
    }

    // Apply currently used color
    memcpy( &m_vertPtr->r, m_color, ColorByteSize );

    // Apply currently used shader
    memcpy( &m_vertPtr->shader, m_shader, ShaderByteSize );

    // Move to the next free space
    m_vertPtr++;

    m_size++;
    m_isDirty = true;
    m_spaceLeft--;
}


void VBO_ITEM::PushVertices( const GLfloat* aVertices, GLuint aSize )
{
    for( unsigned int i = 0; i < aSize; ++i )
    {
        PushVertex( &aVertices[i * VertStride] );
    }
}


GLfloat* VBO_ITEM::GetVertices()
{
    if( m_isDirty )
        prepareFinal();

    return m_vertices;
}


void VBO_ITEM::ChangeColor( const COLOR4D& aColor )
{
    if( m_isDirty )
        prepareFinal();

    // Point to color of vertices
    GLfloat* vertexPtr = m_vertices + ColorOffset;
    const GLfloat newColor[] = { aColor.r, aColor.g, aColor.b, aColor.a };

    for( int i = 0; i < m_size; ++i )
    {
        memcpy( vertexPtr, newColor, ColorByteSize );

        // Move on to the next vertex
        vertexPtr++;
    }
}


void VBO_ITEM::useNewBlock()
{
    VBO_VERTEX* newVertBlock = new VBO_VERTEX[BLOCK_SIZE];

    m_vertPtr = newVertBlock;
    m_vertBlocks.push_back( newVertBlock );

    m_spaceLeft = BLOCK_SIZE;
}


void VBO_ITEM::prepareFinal()
{
    if( m_vertices )
        delete m_vertices;

    // Allocate memory that would store all of vertices
    m_vertices = new GLfloat[m_size * VertStride];
    // Set the pointer that will move along the buffer
    GLfloat* vertPtr = m_vertices;

    // Copy blocks of vertices one after another to m_vertices
    std::list<VBO_VERTEX*>::const_iterator v_it;
    for( v_it = m_vertBlocks.begin(); *v_it != m_vertBlocks.back(); ++v_it )
    {
        memcpy( vertPtr, *v_it, BLOCK_SIZE * VertByteSize );
        delete[] *v_it;
        vertPtr += ( BLOCK_SIZE * VertStride );
    }

    // In the last block we need to copy only used vertices
    memcpy( vertPtr, *v_it, ( BLOCK_SIZE - m_spaceLeft ) * VertByteSize );

    m_isDirty = false;
}
