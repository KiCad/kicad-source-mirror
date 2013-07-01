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
 * @file vbo_container.cpp
 * @brief Class to store VBO_ITEMs.
 */

#include <gal/opengl/vbo_container.h>
#include <cstring>
#include <cstdlib>
#include <boost/foreach.hpp>
#include <wx/log.h>
#ifdef __WXDEBUG__
#include <profile.h>
#endif /* __WXDEBUG__ */

using namespace KiGfx;

VBO_CONTAINER::VBO_CONTAINER( int aSize ) :
        m_freeSpace( aSize ), m_currentSize( aSize ), itemStarted( false ), m_transform( NULL ),
        m_failed( false )
{
    // By default no shader is used
    m_shader[0] = 0;

    m_vertices = static_cast<VBO_VERTEX*>( malloc( aSize * sizeof( VBO_VERTEX ) ) );

    // In the beginning there is only free space
    m_freeChunks.insert( Chunk( aSize, 0 ) );
}


VBO_CONTAINER::~VBO_CONTAINER()
{
    free( m_vertices );
}


void VBO_CONTAINER::StartItem( VBO_ITEM* aVboItem )
{
    itemStarted = true;
    item = aVboItem;
    itemSize = 0;

    // Reserve minimal sensible chunk size (at least to store a single triangle)
    itemChunkSize = 3;
    allocate( aVboItem, itemChunkSize );
}


void VBO_CONTAINER::EndItem()
{
    if( itemSize < itemChunkSize )
    {
        // There is some memory left, so we should return it to the pool
        int itemChunkOffset = item->GetOffset();

        m_reservedChunks.erase( item );
        m_reservedChunks.insert( ReservedChunk( item, Chunk( itemSize, itemChunkOffset ) ) );

        m_freeChunks.insert( Chunk( itemChunkSize - itemSize, itemChunkOffset + itemSize ) );
        m_freeSpace += ( itemChunkSize - itemSize );
    }

    item = NULL;
    itemStarted = false;
}


void VBO_CONTAINER::Add( VBO_ITEM* aVboItem, const VBO_VERTEX* aVertex, unsigned int aSize )
{
    unsigned int offset;
    VBO_VERTEX* vertexPtr;

    if( m_failed )
        return;

    if( itemStarted )   // There is an item being created with an unknown size..
    {
        unsigned int itemChunkOffset;

        // ..and unfortunately does not fit into currently reserved chunk
        if( itemSize + aSize > itemChunkSize )
        {
            // Find the previous chunk for the item and change mark it as NULL
            // so it will not be removed during a possible defragmentation
            ReservedChunkMap::iterator it = m_reservedChunks.find( item );
            m_reservedChunks.insert( ReservedChunk( static_cast<VBO_ITEM*>( NULL ), it->second ) );
            m_reservedChunks.erase( it );

            // Reserve bigger memory fo r the current item
            int newSize = ( 2 * itemSize ) + aSize;
            itemChunkOffset = allocate( aVboItem, newSize );
            aVboItem->SetOffset( itemChunkOffset );

            // Check if there was no error
            if( itemChunkOffset > m_currentSize )
            {
                m_failed = true;
                return;
            }

            it = m_reservedChunks.find( static_cast<VBO_ITEM*>( NULL ) );
            // Check if the chunk was not reallocated after defragmentation
            int oldItemChunkOffset = getChunkOffset( *it );
            // Free the space previously used by the chunk
            freeChunk( it );

            // Copy all the old data
            memcpy( &m_vertices[itemChunkOffset], &m_vertices[oldItemChunkOffset],
                    itemSize * VBO_ITEM::VertByteSize );

            itemChunkSize = newSize;
        }
        else
        {
            itemChunkOffset = item->GetOffset();
        }

        // Store new vertices in the chunk reserved for the unknown-sized item
        offset = itemChunkOffset + itemSize;
        itemSize += aSize;
    }
    else
    {
        // Add vertices to previously already finished item
        wxASSERT_MSG( false, wxT( "Warning: not tested yet" ) );

        ReservedChunkMap::iterator it = m_reservedChunks.find( aVboItem );
        unsigned int chunkSize = getChunkSize( *it );
        unsigned int itemSize = aVboItem->GetSize();

        if( chunkSize < itemSize + aSize )
        {
            resizeChunk( aVboItem, itemSize + aSize );
            it = m_reservedChunks.find( aVboItem );
        }

        offset = getChunkOffset( *it ) + itemSize;
    }

    for( unsigned int i = 0; i < aSize; ++i )
    {
        // Pointer to the vertex that we are currently adding
        vertexPtr = &m_vertices[offset + i];

        // Modify the vertex according to the currently used transformations
        if( m_transform != NULL )
        {
            // Apply transformations
            glm::vec4 vertex( aVertex[i].x, aVertex[i].y, aVertex[i].z, 1.0f );
            vertex = *m_transform * vertex;

            // Replace only coordinates, leave color as it is
            vertexPtr->x = vertex.x;
            vertexPtr->y = vertex.y;
            vertexPtr->z = vertex.z;
        }
        else
        {
            // Simply copy coordinates
            vertexPtr->x = aVertex[i].x;
            vertexPtr->y = aVertex[i].y;
            vertexPtr->z = aVertex[i].z;
        }

        // Apply currently used color
        vertexPtr->r = m_color[0];
        vertexPtr->g = m_color[1];
        vertexPtr->b = m_color[2];
        vertexPtr->a = m_color[3];

        // Apply currently used shader
        for( unsigned int j = 0; j < VBO_ITEM::ShaderStride; ++j )
        {
            vertexPtr->shader[j] = m_shader[j];
        }
    }

}


VBO_VERTEX* VBO_CONTAINER::GetAllVertices() const
{
    return m_vertices;
}


VBO_VERTEX* VBO_CONTAINER::GetVertices( const VBO_ITEM* aVboItem ) const
{
    int offset = aVboItem->GetOffset();

    return &m_vertices[offset];
}


unsigned int VBO_CONTAINER::allocate( VBO_ITEM* aVboItem, unsigned int aSize )
{
    // Is there enough space to store vertices?
    if( m_freeSpace < aSize )
    {
        bool result;

        // Would it be enough to double the current space?
        if( aSize < m_freeSpace + m_currentSize )
        {
            // Yes: exponential growing
            result = resizeContainer( m_currentSize * 2 );
        }
        else
        {
            // No: grow to the nearest bigger power of 2
            result = resizeContainer( getPowerOf2( m_currentSize * 2 + aSize ) );
        }

        // An error has occurred
        if( !result )
        {
            return UINT_MAX;
        }
    }

    // Look for the space with at least given size
    FreeChunkMap::iterator it = m_freeChunks.lower_bound( aSize );

    if( it == m_freeChunks.end() )
    {
        // This means that there is enough space for
        // storing vertices, but the space is not continous
        if( !defragment() )
        {
            return UINT_MAX;
        }

        // We can take the first free chunk, as there is only one after defragmentation
        // and we can be sure that it provides enough space to store the object
        it = m_freeChunks.begin();
    }

    unsigned int chunkSize = it->first;
    unsigned int chunkOffset = it->second;

    m_freeChunks.erase( it );

    wxASSERT( chunkSize >= aSize );

    // If there is some space left, return it to the pool - add an entry for it
    if( chunkSize > aSize )
    {
        m_freeChunks.insert( Chunk( chunkSize - aSize, chunkOffset + aSize ) );
    }
    m_freeSpace -= aSize;
    m_reservedChunks.insert( ReservedChunk( aVboItem, Chunk( aSize, chunkOffset ) ) );

    aVboItem->SetOffset( chunkOffset );

    return chunkOffset;
}


void VBO_CONTAINER::freeChunk( const ReservedChunkMap::iterator& aChunk )
{
    // Remove the chunk from the reserved chunks map and add to the free chunks map
    int size = getChunkSize( *aChunk );
    int offset = getChunkOffset( *aChunk );

    m_reservedChunks.erase( aChunk );
    m_freeChunks.insert( Chunk( size, offset ) );
    m_freeSpace += size;
}


bool VBO_CONTAINER::defragment( VBO_VERTEX* aTarget )
{
    if( m_freeChunks.size() <= 1 )
    {
        // There is no point in defragmenting, as there is only one or no free chunks
        return true;
    }

    if( aTarget == NULL )
    {
        // No target was specified, so we have to allocate our own space
        aTarget = static_cast<VBO_VERTEX*>( malloc( m_currentSize * sizeof( VBO_VERTEX ) ) );
        if( aTarget == NULL )
        {
            wxLogError( wxT( "Run out of memory" ) );
            return false;
        }
    }

    int newOffset = 0;
    ReservedChunkMap::iterator it, it_end;
    for( it = m_reservedChunks.begin(), it_end = m_reservedChunks.end(); it != it_end; ++it )
    {
        VBO_ITEM* vboItem = getChunkVboItem( *it );
        int itemOffset = getChunkOffset( *it );
        int itemSize = getChunkSize( *it );

        // Move an item to the new container
        memcpy( &aTarget[newOffset], &m_vertices[itemOffset], itemSize * VBO_ITEM::VertByteSize );

        // Update new offset
        if( vboItem )
            vboItem->SetOffset( newOffset );
        setChunkOffset( *it, newOffset );

        // Move to the next free space
        newOffset += itemSize;
    }

    free( m_vertices );
    m_vertices = aTarget;

    // Now there is only one big chunk of free memory
    m_freeChunks.clear();
    m_freeChunks.insert( Chunk( m_freeSpace, m_currentSize - m_freeSpace ) );

    return true;
}


void VBO_CONTAINER::resizeChunk( VBO_ITEM* aVboItem, int aNewSize )
{
    wxASSERT_MSG( false, wxT( "Warning: not tested yet" ) );

    // TODO ESPECIALLY test the case of shrinking chunk
    ReservedChunkMap::iterator it = m_reservedChunks.find( aVboItem );
    int size = getChunkSize( *it );
    int offset = getChunkOffset( *it );

    int newOffset = allocate( aVboItem, aNewSize );
    memcpy( &m_vertices[newOffset], &m_vertices[offset], size * VBO_ITEM::VertByteSize );

    // Remove the chunk from the reserved chunks map and add to the free chunks map
    m_reservedChunks.erase( it );
    m_freeChunks.insert( Chunk( size, offset ) );
    m_freeSpace += size;
}


bool VBO_CONTAINER::resizeContainer( unsigned int aNewSize )
{
    VBO_VERTEX* newContainer;

    if( aNewSize < m_currentSize )
    {
        // Sanity check, no shrinking if we cannot fit all the data
        if( ( m_currentSize - m_freeSpace ) > aNewSize )
            return false;

        newContainer = static_cast<VBO_VERTEX*>( malloc( aNewSize * sizeof( VBO_VERTEX ) ) );
        if( newContainer == NULL )
        {
            wxLogError( wxT( "Run out of memory" ) );
            return false;
        }

        // Defragment directly to the new, smaller container
        defragment( newContainer );
    }
    else
    {
        newContainer = static_cast<VBO_VERTEX*>( realloc( m_vertices, aNewSize * sizeof( VBO_VERTEX ) ) );
        if( newContainer == NULL )
        {
            wxLogError( wxT( "Run out of memory" ) );
            return false;
        }
    }

    m_vertices = newContainer;

    // Update variables
    unsigned int lastFreeSize = 0;
    unsigned int lastFreeOffset = 0;

    // Search for the last free chunk *at the end of the container* (not the last chunk in general)
    FreeChunkMap::reverse_iterator lastFree, freeEnd;
    for( lastFree = m_freeChunks.rbegin(), freeEnd = m_freeChunks.rend();
            lastFree != freeEnd && lastFreeSize + lastFreeOffset != m_currentSize; ++lastFree )
    {
        lastFreeSize = getChunkSize( *lastFree );
        lastFreeOffset = getChunkOffset( *lastFree );
    }

    if( lastFreeSize + lastFreeOffset == m_currentSize )
    {
        // We found a chunk at the end of the container
        m_freeChunks.erase( lastFree.base() );
        // so we can merge it with the new freeChunk chunk
        m_freeChunks.insert( Chunk( aNewSize - m_currentSize + lastFreeSize,    // size
                                    m_currentSize - lastFreeSize ) );           // offset
    }
    else
    {
        // As there is no free chunk at the end of container - simply add a new entry
        if( aNewSize > m_currentSize )  // only in the case of enlargement
        {
            m_freeChunks.insert( Chunk( aNewSize - m_currentSize,               // size
                                        m_currentSize ) );                      // offset
        }
    }

    m_freeSpace += ( aNewSize - m_currentSize );
    m_currentSize = aNewSize;

    return true;
}
