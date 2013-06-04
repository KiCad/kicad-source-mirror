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

        std::list<GLuint*>::const_iterator i_it, i_end;
        for( i_it = m_indBlocks.begin(), i_end = m_indBlocks.end(); i_it != i_end; ++i_it )
            delete[] *i_it;
    }

    if( m_vertices )
        delete m_vertices;

    if( m_indices )
        delete m_indices;
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
        memcpy( &m_vertPtr->struc.coord, &vertex[0], CoordByteSize );
    }
    else
    {
        // Add the new vertex
        memcpy( &m_vertPtr->struc.coord, aVertex, CoordByteSize );
    }

    // Apply currently used color
    memcpy( &m_vertPtr->struc.color, m_color, ColorByteSize );

    // Apply currently used shader
    memcpy( &m_vertPtr->struc.shader, m_shader, ShaderByteSize );

    // Move to the next free space
    m_vertPtr++;

    // Add the new index
    *m_indPtr = m_offset + m_size;
    m_indPtr++;

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


GLuint* VBO_ITEM::GetIndices()
{
    if( m_isDirty )
        prepareFinal();

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


void VBO_ITEM::SetTransformMatrix( const glm::mat4* aMatrix )
{
    m_transform = aMatrix;
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


void VBO_ITEM::UseColor( const COLOR4D& aColor )
{
    m_color[0] = aColor.r;
    m_color[1] = aColor.g;
    m_color[2] = aColor.b;
    m_color[3] = aColor.a;
}


void VBO_ITEM::UseShader( const GLfloat* aShader )
{
    memcpy( m_shader, aShader, ShaderByteSize );
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


void VBO_ITEM::useNewBlock()
{
    VBO_VERTEX* newVertBlock = new VBO_VERTEX[BLOCK_SIZE];
    GLuint*     newIndBlock  = new GLuint[BLOCK_SIZE];

    m_vertPtr = newVertBlock;
    m_indPtr  = newIndBlock;

    m_vertBlocks.push_back( newVertBlock );
    m_indBlocks.push_back( newIndBlock );

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

    if( m_indices )
        delete m_indices;

    // Allocate memory that would store all of indices
    m_indices = new GLuint[m_size * IndStride];
    // Set the pointer that will move along the buffer
    GLuint* indPtr = m_indices;

    // Copy blocks of indices one after another to m_indices
    std::list<GLuint*>::const_iterator i_it;
    for( i_it = m_indBlocks.begin(); *i_it != m_indBlocks.back(); ++i_it )
    {
        memcpy( indPtr, *i_it, BLOCK_SIZE * IndByteSize );
        delete[] *i_it;
        indPtr += ( BLOCK_SIZE * IndStride );
    }

    // In the last block we need to copy only used indices
    memcpy( indPtr, *i_it, ( BLOCK_SIZE - m_spaceLeft ) * IndByteSize );

    m_isDirty = false;
}
