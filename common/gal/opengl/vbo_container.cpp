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
#include <algorithm>
#include <list>
#include <wx/log.h>
#ifdef __WXDEBUG__
#include <profile.h>
#endif /* __WXDEBUG__ */

using namespace KiGfx;

VBO_CONTAINER::VBO_CONTAINER( unsigned int aSize ) :
        m_freeSpace( aSize ), m_currentSize( aSize ), m_initialSize( aSize ), m_transform( NULL ),
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


void VBO_CONTAINER::StartItem( VBO_ITEM* aItem )
{
    m_item      = aItem;
    m_itemSize  = aItem->GetSize();
    m_chunkSize = m_itemSize;

    if( m_itemSize == 0 )
        m_items.insert( m_item );   // The item was not stored before
    else
        m_chunkOffset = m_item->GetOffset();
}


void VBO_CONTAINER::EndItem()
{
    if( m_itemSize < m_chunkSize )
    {
        // Add the not used memory back to the pool
        m_freeChunks.insert( Chunk( m_chunkSize - m_itemSize, m_chunkOffset + m_itemSize ) );
        m_freeSpace += ( m_chunkSize - m_itemSize );
    }

    m_item = NULL;
}


void VBO_CONTAINER::Add( const VBO_VERTEX* aVertex, unsigned int aSize )
{
    // Pointer to the vertex that we are currently adding
    VBO_VERTEX* vertexPtr = allocate( aSize );

    if( vertexPtr == NULL )
        return;

    for( unsigned int i = 0; i < aSize; ++i )
    {
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

        vertexPtr++;
    }

}


void VBO_CONTAINER::Clear()
{
    // Change size to the default one
    m_vertices = static_cast<VBO_VERTEX*>( realloc( m_vertices,
                                           m_initialSize * sizeof( VBO_VERTEX ) ) );

    // Set the size of all the stored VERTEX_ITEMs to 0, so it is clear that they are not held
    // in the container anymore
    Items::iterator it;
    for( it = m_items.begin(); it != m_items.end(); ++it )
    {
        ( *it )->setSize( 0 );
    }
    m_items.clear();

    // Reset state variables
    m_transform = NULL;
    m_failed = false;

    // By default no shader is used
    m_shader[0] = 0;

    // In the beginning there is only free space
    m_freeSpace = m_initialSize;
    m_currentSize = m_initialSize;
    m_freeChunks.clear();
    m_freeChunks.insert( Chunk( m_freeSpace, 0 ) );
}


void VBO_CONTAINER::Free( VBO_ITEM* aItem )
{
    freeItem( aItem );

    // Dynamic memory freeing, there is no point in holding
    // a large amount of memory when there is no use for it
    if( m_freeSpace > ( m_currentSize / 2 ) && m_currentSize > defaultInitSize )
    {
        resizeContainer( m_currentSize / 2 );
    }
}


VBO_VERTEX* VBO_CONTAINER::GetAllVertices() const
{
    return m_vertices;
}


VBO_VERTEX* VBO_CONTAINER::GetVertices( const VBO_ITEM* aItem ) const
{
    int offset = aItem->GetOffset();

    return &m_vertices[offset];
}

VBO_VERTEX* VBO_CONTAINER::allocate( unsigned int aSize )
{
    wxASSERT( m_item != NULL );

    if( m_failed )
        return NULL;

    if( m_itemSize + aSize > m_chunkSize )
    {
        // There is not enough space in the currently reserved chunk, so we have to resize it

        // Reserve a bigger memory chunk for the current item
        m_chunkSize = std::max( ( 2 * m_itemSize ) + aSize, (unsigned) 3 );
        // Save the current size before reallocating
        m_chunkOffset = reallocate( m_chunkSize );

        if( m_chunkOffset > m_currentSize )
        {
            m_failed = true;
            return NULL;
        }
    }

    VBO_VERTEX* reserved = &m_vertices[m_chunkOffset + m_itemSize];
    m_itemSize += aSize;
    m_item->setSize( m_itemSize );

    return reserved;
}


unsigned int VBO_CONTAINER::reallocate( unsigned int aSize )
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

        if( !result )
            return UINT_MAX;
    }

    // Look for the free space of at least given size
    FreeChunkMap::iterator newChunk = m_freeChunks.lower_bound( aSize );

    if( newChunk == m_freeChunks.end() )
    {
        // In the case when there is enough space to store the vertices,
        // but the free space is not continous we should defragment the container
        if( !defragment() )
            return UINT_MAX;

        // Update the current offset
        m_chunkOffset = m_item->GetOffset();

        // We can take the first free chunk, as there is only one after defragmentation
        // and we can be sure that it provides enough space to store the object
        newChunk = m_freeChunks.begin();
    }

    // Parameters of the allocated cuhnk
    unsigned int chunkSize = newChunk->first;
    unsigned int chunkOffset = newChunk->second;

    wxASSERT( chunkSize >= aSize );
    wxASSERT( chunkOffset < m_currentSize );

    // Check if the item was previously stored in the container
    if( m_itemSize > 0 )
    {
        // The item was reallocated, so we have to copy all the old data to the new place
        memcpy( &m_vertices[chunkOffset], &m_vertices[m_chunkOffset],
                m_itemSize * VBO_ITEM::VertByteSize );

        // Free the space previously used by the chunk
        m_freeChunks.insert( Chunk( m_itemSize, m_chunkOffset ) );
        m_freeSpace += m_itemSize;
    }
    // Remove the allocated chunk from the free space pool
    m_freeChunks.erase( newChunk );
    m_freeSpace -= chunkSize;

    // If there is some space left, return it to the pool - add an entry for it
    if( chunkSize > aSize )
    {
        m_freeChunks.insert( Chunk( chunkSize - aSize, chunkOffset + aSize ) );
        m_freeSpace += chunkSize - aSize;
    }

    m_item->setOffset( chunkOffset );

    return chunkOffset;
}


bool VBO_CONTAINER::defragment( VBO_VERTEX* aTarget )
{
    if( aTarget == NULL )
    {
        // No target was specified, so we have to reallocate our own space
        aTarget = static_cast<VBO_VERTEX*>( malloc( m_currentSize * sizeof( VBO_VERTEX ) ) );
        if( aTarget == NULL )
        {
            wxLogError( wxT( "Run out of memory" ) );
            return false;
        }
    }

    int newOffset = 0;
    Items::iterator it, it_end;
    for( it = m_items.begin(), it_end = m_items.end(); it != it_end; ++it )
    {
        VBO_ITEM* item = *it;
        int itemOffset = item->GetOffset();
        int itemSize = item->GetSize();

        // Move an item to the new container
        memcpy( &aTarget[newOffset], &m_vertices[itemOffset], itemSize * VBO_ITEM::VertByteSize );

        // Update its offset
        item->setOffset( newOffset );

        // Move to the next free space
        newOffset += itemSize;
    }

    free( m_vertices );
    m_vertices = aTarget;

    // Now there is only one big chunk of free memory
    m_freeChunks.clear();
    m_freeChunks.insert( Chunk( m_freeSpace, reservedSpace() ) );

    return true;
}


void VBO_CONTAINER::mergeFreeChunks()
{
    if( m_freeChunks.size() < 2 )  // There are no chunks that can be merged
        return;

    // Reversed free chunks map - this one stores chunk size with its offset as the key
    std::list<Chunk> freeChunks;

    FreeChunkMap::const_iterator it, it_end;
    for( it = m_freeChunks.begin(), it_end = m_freeChunks.end(); it != it_end; ++it )
    {
        freeChunks.push_back( std::make_pair( it->second, it->first ) );
    }
    m_freeChunks.clear();
    freeChunks.sort();

    std::list<Chunk>::const_iterator itf, itf_end;
    unsigned int offset = freeChunks.front().first;
    unsigned int size   = freeChunks.front().second;
    freeChunks.pop_front();
    for( itf = freeChunks.begin(), itf_end = freeChunks.end(); itf != itf_end; ++itf )
    {
        if( itf->first == offset + size )
        {
            // These chunks can be merged, so just increase the current chunk size and go on
            size += itf->second;
        }
        else
        {
            // These chunks cannot be merged
            // So store the previous one
            m_freeChunks.insert( std::make_pair( size, offset ) );
            // and let's check the next chunk
            offset = itf->first;
            size   = itf->second;
        }
    }

    // Add the last one
    m_freeChunks.insert( std::make_pair( size, offset ) );
}


bool VBO_CONTAINER::resizeContainer( unsigned int aNewSize )
{
    VBO_VERTEX* newContainer;

    if( aNewSize < m_currentSize )
    {
        // Shrinking container
        // Sanity check, no shrinking if we cannot fit all the data
        if( reservedSpace() > aNewSize )
            return false;

        newContainer = static_cast<VBO_VERTEX*>( malloc( aNewSize * sizeof( VBO_VERTEX ) ) );
        if( newContainer == NULL )
        {
            wxLogError( wxT( "Run out of memory" ) );
            return false;
        }

        // Defragment directly to the new, smaller container
        defragment( newContainer );

        // We have to correct freeChunks after defragmentation
        m_freeChunks.clear();
        m_freeChunks.insert( Chunk( aNewSize - reservedSpace(), reservedSpace() ) );
    }
    else
    {
        // Enlarging container
        newContainer = static_cast<VBO_VERTEX*>( realloc( m_vertices, aNewSize * sizeof( VBO_VERTEX ) ) );
        if( newContainer == NULL )
        {
            wxLogError( wxT( "Run out of memory" ) );
            return false;
        }

        // Add an entry for the new memory chunk at the end of the container
        m_freeChunks.insert( Chunk( aNewSize - m_currentSize, m_currentSize ) );
    }

    m_vertices = newContainer;

    m_freeSpace += ( aNewSize - m_currentSize );
    m_currentSize = aNewSize;

    return true;
}


void VBO_CONTAINER::freeItem( VBO_ITEM* aItem )
{
    int size    = aItem->GetSize();
    int offset  = aItem->GetOffset();

    m_freeChunks.insert( Chunk( size, offset ) );
    m_freeSpace += size;
    m_items.erase( aItem );

    // Item size is set to 0, so it means that it is not stored in the container
    aItem->setSize( 0 );
}


void VBO_CONTAINER::test() const
{
    unsigned int freeSpace = 0;
    FreeChunkMap::const_iterator it, it_end;

    // Check if the amount of free memory stored as chunks is the same as reported by m_freeSpace
    for( it = m_freeChunks.begin(), it_end = m_freeChunks.end(); it != it_end; ++it )
        freeSpace += it->first;

    wxASSERT( freeSpace == m_freeSpace );
}
