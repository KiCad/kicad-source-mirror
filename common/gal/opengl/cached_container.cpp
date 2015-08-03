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
 * @file cached_container.cpp
 * @brief Class to store instances of VERTEX with caching. It allows storing VERTEX objects and
 * associates them with VERTEX_ITEMs. This leads to a possibility of caching vertices data in the
 * GPU memory and a fast reuse of that data.
 */

#include <gal/opengl/cached_container.h>
#include <gal/opengl/vertex_manager.h>
#include <gal/opengl/vertex_item.h>
#include <gal/opengl/shader.h>
#include <confirm.h>
#include <wx/log.h>
#include <list>
#ifdef __WXDEBUG__
#include <profile.h>
#endif /* __WXDEBUG__ */

using namespace KIGFX;

CACHED_CONTAINER::CACHED_CONTAINER( unsigned int aSize ) :
    VERTEX_CONTAINER( aSize ), m_item( NULL )
{
    // In the beginning there is only free space
    m_freeChunks.insert( CHUNK( aSize, 0 ) );

    // Do not have uninitialized members:
    m_chunkSize = 0;
    m_chunkOffset = 0;
    m_itemSize = 0;
}


void CACHED_CONTAINER::SetItem( VERTEX_ITEM* aItem )
{
    wxASSERT( aItem != NULL );

    m_item      = aItem;
    m_itemSize  = m_item->GetSize();
    m_chunkSize = m_itemSize;

    if( m_itemSize == 0 )
        m_items.insert( m_item ); // The item was not stored before
    else
        m_chunkOffset = m_item->GetOffset();

#if CACHED_CONTAINER_TEST > 1
    wxLogDebug( wxT( "Adding/editing item 0x%08lx (size %d)" ), (long) m_item, m_itemSize );
#endif
}


void CACHED_CONTAINER::FinishItem()
{
    wxASSERT( m_item != NULL );
    wxASSERT( m_item->GetSize() == m_itemSize );

    // Finishing the previously edited item
    if( m_itemSize < m_chunkSize )
    {
        // There is some not used but reserved memory left, so we should return it to the pool
        int itemOffset = m_item->GetOffset();

        // Add the not used memory back to the pool
        m_freeChunks.insert( CHUNK( m_chunkSize - m_itemSize, itemOffset + m_itemSize ) );
        m_freeSpace += ( m_chunkSize - m_itemSize );
        // mergeFreeChunks();   // veery slow and buggy
    }

#if CACHED_CONTAINER_TEST > 1
    wxLogDebug( wxT( "Finishing item 0x%08lx (size %d)" ), (long) m_item, m_itemSize );
    test();
    m_item = NULL;    // electric fence
#endif
}


VERTEX* CACHED_CONTAINER::Allocate( unsigned int aSize )
{
    wxASSERT( m_item != NULL );

    if( m_failed )
        return NULL;

    if( m_itemSize + aSize > m_chunkSize )
    {
        // There is not enough space in the currently reserved chunk, so we have to resize it

        // Reserve a bigger memory chunk for the current item and
        // make it multiple of 3 to store triangles
        m_chunkSize = ( 2 * m_itemSize ) + aSize + ( 3 - aSize % 3 );
        // Save the current size before reallocating
        m_chunkOffset = reallocate( m_chunkSize );

        if( m_chunkOffset > m_currentSize )
        {
            m_failed = true;
            return NULL;
        }
    }

    VERTEX* reserved = &m_vertices[m_chunkOffset + m_itemSize];
    m_itemSize += aSize;
    // Now the item officially possesses the memory chunk
    m_item->setSize( m_itemSize );

    // The content has to be updated
    m_dirty = true;

#if CACHED_CONTAINER_TEST > 1
    test();
#endif
#if CACHED_CONTAINER_TEST > 2
    showFreeChunks();
    showReservedChunks();
#endif

    return reserved;
}


void CACHED_CONTAINER::Delete( VERTEX_ITEM* aItem )
{
    wxASSERT( aItem != NULL );
    wxASSERT( m_items.find( aItem ) != m_items.end() );

    int size   = aItem->GetSize();
    int offset = aItem->GetOffset();

#if CACHED_CONTAINER_TEST > 1
    wxLogDebug( wxT( "Removing 0x%08lx (size %d offset %d)" ), (long) aItem, size, offset );
#endif

    // Insert a free memory chunk entry in the place where item was stored
    if( size > 0 )
    {
        m_freeChunks.insert( CHUNK( size, offset ) );
        m_freeSpace += size;
        // Indicate that the item is not stored in the container anymore
        aItem->setSize( 0 );
    }

    m_items.erase( aItem );

#if CACHED_CONTAINER_TEST > 1
    test();
#endif

    // Dynamic memory freeing, there is no point in holding
    // a large amount of memory when there is no use for it
    if( m_freeSpace > ( 0.75 * m_currentSize ) && m_currentSize > m_initialSize )
    {
        resizeContainer( 0.5 * m_currentSize );
    }
}


void CACHED_CONTAINER::Clear()
{
    // Change size to the default one
    m_vertices = static_cast<VERTEX*>( realloc( m_vertices,
                                                m_initialSize * sizeof( VERTEX ) ) );

    // Reset state variables
    m_freeSpace     = m_initialSize;
    m_currentSize   = m_initialSize;
    m_failed = false;

    // Set the size of all the stored VERTEX_ITEMs to 0, so it is clear that they are not held
    // in the container anymore
    ITEMS::iterator it;

    for( it = m_items.begin(); it != m_items.end(); ++it )
    {
        ( *it )->setSize( 0 );
    }

    m_items.clear();


    // Now there is only free space left
    m_freeChunks.clear();
    m_freeChunks.insert( CHUNK( m_freeSpace, 0 ) );
}


unsigned int CACHED_CONTAINER::reallocate( unsigned int aSize )
{
    wxASSERT( aSize > 0 );

#if CACHED_CONTAINER_TEST > 2
    wxLogDebug( wxT( "Resize 0x%08lx from %d to %d" ), (long) m_item, m_itemSize, aSize );
#endif

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
            // No: grow to the nearest greater power of 2
            result = resizeContainer( pow( 2, ceil( log2( m_currentSize * 2 + aSize ) ) ) );
        }

        if( !result )
            return UINT_MAX;
    }

    // Look for the free space chunk of at least given size
    FREE_CHUNK_MAP::iterator newChunk = m_freeChunks.lower_bound( aSize );

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
    unsigned int chunkSize   = newChunk->first;
    unsigned int chunkOffset = newChunk->second;

    wxASSERT( chunkSize >= aSize );
    wxASSERT( chunkOffset < m_currentSize );

    // Check if the item was previously stored in the container
    if( m_itemSize > 0 )
    {
#if CACHED_CONTAINER_TEST > 3
        wxLogDebug( wxT( "Moving 0x%08x from 0x%08x to 0x%08x" ),
                    (int) m_item, oldChunkOffset, chunkOffset );
#endif
        // The item was reallocated, so we have to copy all the old data to the new place
        memcpy( &m_vertices[chunkOffset], &m_vertices[m_chunkOffset], m_itemSize * VertexSize );

        // Free the space previously used by the chunk
        wxASSERT( m_itemSize > 0 );
        m_freeChunks.insert( CHUNK( m_itemSize, m_chunkOffset ) );
        m_freeSpace += m_itemSize;
    }

    // Remove the allocated chunk from the free space pool
    m_freeChunks.erase( newChunk );

    // If there is some space left, return it to the pool - add an entry for it
    if( chunkSize > aSize )
    {
        m_freeChunks.insert( CHUNK( chunkSize - aSize, chunkOffset + aSize ) );
    }

    m_freeSpace -= aSize;
    // mergeFreeChunks();   // veery slow and buggy

    m_item->setOffset( chunkOffset );

    return chunkOffset;
}


bool CACHED_CONTAINER::defragment( VERTEX* aTarget )
{
#if CACHED_CONTAINER_TEST > 0
    wxLogDebug( wxT( "Defragmenting" ) );

    prof_counter totalTime;
    prof_start( &totalTime );
#endif

    if( aTarget == NULL )
    {
        // No target was specified, so we have to reallocate our own space
        aTarget = static_cast<VERTEX*>( malloc( m_currentSize * sizeof( VERTEX ) ) );

        if( aTarget == NULL )
        {
            DisplayError( NULL, wxT( "Run out of memory" ) );
            return false;
        }
    }

    int newOffset = 0;
    ITEMS::iterator it, it_end;

    for( it = m_items.begin(), it_end = m_items.end(); it != it_end; ++it )
    {
        VERTEX_ITEM* item = *it;
        int itemOffset    = item->GetOffset();
        int itemSize      = item->GetSize();

        // Move an item to the new container
        memcpy( &aTarget[newOffset], &m_vertices[itemOffset], itemSize * VertexSize );

        // Update new offset
        item->setOffset( newOffset );

        // Move to the next free space
        newOffset += itemSize;
    }

    free( m_vertices );
    m_vertices = aTarget;

    // Now there is only one big chunk of free memory
    m_freeChunks.clear();
    wxASSERT( m_freeSpace > 0 );
    m_freeChunks.insert( CHUNK( m_freeSpace, m_currentSize - m_freeSpace ) );

#if CACHED_CONTAINER_TEST > 0
    prof_end( &totalTime );

    wxLogDebug( wxT( "Defragmented the container storing %d vertices / %.1f ms" ),
                m_currentSize - m_freeSpace, totalTime.msecs() );
#endif

    return true;
}


void CACHED_CONTAINER::mergeFreeChunks()
{
    if( m_freeChunks.size() <= 1 ) // There are no chunks that can be merged
        return;

#if CACHED_CONTAINER_TEST > 0
    prof_counter totalTime;
    prof_start( &totalTime );
#endif

    // Reversed free chunks map - this one stores chunk size with its offset as the key
    std::list<CHUNK> freeChunks;

    FREE_CHUNK_MAP::const_iterator it, it_end;

    for( it = m_freeChunks.begin(), it_end = m_freeChunks.end(); it != it_end; ++it )
    {
        freeChunks.push_back( std::make_pair( it->second, it->first ) );
    }

    m_freeChunks.clear();
    freeChunks.sort();

    std::list<CHUNK>::const_iterator itf, itf_end;
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

#if CACHED_CONTAINER_TEST > 0
    prof_end( &totalTime );

    wxLogDebug( wxT( "Merged free chunks / %.1f ms" ), totalTime.msecs() );
#endif

    test();
}


bool CACHED_CONTAINER::resizeContainer( unsigned int aNewSize )
{
    wxASSERT( aNewSize != m_currentSize );

#if CACHED_CONTAINER_TEST > 0
    wxLogDebug( wxT( "Resizing container from %d to %d" ), m_currentSize, aNewSize );
#endif

    VERTEX* newContainer;

    if( aNewSize < m_currentSize )
    {
        // Shrinking container
        // Sanity check, no shrinking if we cannot fit all the data
        if( reservedSpace() > aNewSize )
            return false;

        newContainer = static_cast<VERTEX*>( malloc( aNewSize * sizeof( VERTEX ) ) );

        if( newContainer == NULL )
        {
            DisplayError( NULL, wxT( "Run out of memory" ) );
            return false;
        }

        // Defragment directly to the new, smaller container
        defragment( newContainer );

        // We have to correct freeChunks after defragmentation
        m_freeChunks.clear();
        wxASSERT( aNewSize - reservedSpace() > 0 );
        m_freeChunks.insert( CHUNK( aNewSize - reservedSpace(), reservedSpace() ) );
    }
    else
    {
        // Enlarging container
        newContainer = static_cast<VERTEX*>( realloc( m_vertices, aNewSize * sizeof( VERTEX ) ) );

        if( newContainer == NULL )
        {
            DisplayError( NULL, wxT( "Run out of memory" ) );
            return false;
        }

        // Add an entry for the new memory chunk at the end of the container
        m_freeChunks.insert( CHUNK( aNewSize - m_currentSize, m_currentSize ) );
    }

    m_vertices = newContainer;

    m_freeSpace   += ( aNewSize - m_currentSize );
    m_currentSize = aNewSize;

    return true;
}


#ifdef CACHED_CONTAINER_TEST
void CACHED_CONTAINER::showFreeChunks()
{
    FREE_CHUNK_MAP::iterator it;

    wxLogDebug( wxT( "Free chunks:" ) );

    for( it = m_freeChunks.begin(); it != m_freeChunks.end(); ++it )
    {
        unsigned int offset = getChunkOffset( *it );
        unsigned int size   = getChunkSize( *it );
        wxASSERT( size > 0 );

        wxLogDebug( wxT( "[0x%08x-0x%08x] (size %d)" ),
                    offset, offset + size - 1, size );
    }
}


void CACHED_CONTAINER::showReservedChunks()
{
    ITEMS::iterator it;

    wxLogDebug( wxT( "Reserved chunks:" ) );

    for( it = m_items.begin(); it != m_items.end(); ++it )
    {
        VERTEX_ITEM* item   = *it;
        unsigned int offset = item->GetOffset();
        unsigned int size   = item->GetSize();
        wxASSERT( size > 0 );

        wxLogDebug( wxT( "[0x%08x-0x%08x] @ 0x%08lx (size %d)" ),
                    offset, offset + size - 1, (long) item, size );
    }
}


void CACHED_CONTAINER::test()
{
    // Free space check
    unsigned int freeSpace = 0;
    FREE_CHUNK_MAP::iterator itf;

    for( itf = m_freeChunks.begin(); itf != m_freeChunks.end(); ++itf )
        freeSpace += getChunkSize( *itf );

    wxASSERT( freeSpace == m_freeSpace );

    // Reserved space check
    /*unsigned int reservedSpace = 0;
    ITEMS::iterator itr;
    for( itr = m_items.begin(); itr != m_items.end(); ++itr )
        reservedSpace += ( *itr )->GetSize();
    reservedSpace += m_itemSize;    // Add the current chunk size

    wxASSERT( ( freeSpace + reservedSpace ) == m_currentSize );*/

    // Overlapping check TBD
}

#endif /* CACHED_CONTAINER_TEST */
