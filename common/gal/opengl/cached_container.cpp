/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2013-2017 CERN
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
 * @file cached_container.cpp
 * @brief Class to store instances of VERTEX with caching. It allows storing VERTEX objects and
 * associates them with VERTEX_ITEMs. This leads to a possibility of caching vertices data in the
 * GPU memory and a fast reuse of that data.
 */

#include <gal/opengl/cached_container.h>
#include <gal/opengl/vertex_manager.h>
#include <gal/opengl/vertex_item.h>
#include <gal/opengl/utils.h>

#include <list>
#include <algorithm>
#include <cassert>

#ifdef __WIN32__
#include <excpt.h>
#endif

#ifdef KICAD_GAL_PROFILE
#include <wx/log.h>
#include <core/profile.h>
#endif /* KICAD_GAL_PROFILE */

using namespace KIGFX;

CACHED_CONTAINER::CACHED_CONTAINER( unsigned int aSize ) :
        VERTEX_CONTAINER( aSize ),
        m_item( nullptr ),
        m_chunkSize( 0 ),
        m_chunkOffset( 0 ),
        m_maxIndex( 0 )
{
    // In the beginning there is only free space
    m_freeChunks.insert( std::make_pair( aSize, 0 ) );
}


void CACHED_CONTAINER::SetItem( VERTEX_ITEM* aItem )
{
    assert( aItem != nullptr );

    unsigned int itemSize = aItem->GetSize();
    m_item = aItem;
    m_chunkSize = itemSize;

    // Get the previously set offset if the item was stored previously
    m_chunkOffset = itemSize > 0 ? aItem->GetOffset() : -1;
}


void CACHED_CONTAINER::FinishItem()
{
    assert( m_item != nullptr );

    unsigned int itemSize = m_item->GetSize();

    // Finishing the previously edited item
    if( itemSize < m_chunkSize )
    {
        // There is some not used but reserved memory left, so we should return it to the pool
        int itemOffset = m_item->GetOffset();

        // Add the not used memory back to the pool
        addFreeChunk( itemOffset + itemSize, m_chunkSize - itemSize );

        // mergeFreeChunks();   // veery slow and buggy

        m_maxIndex = std::max( itemOffset + itemSize, m_maxIndex );
    }

    if( itemSize > 0 )
        m_items.insert( m_item );

    m_item = nullptr;
    m_chunkSize = 0;
    m_chunkOffset = 0;

#if CACHED_CONTAINER_TEST > 1
    test();
#endif
}


VERTEX* CACHED_CONTAINER::Allocate( unsigned int aSize )
{
    assert( m_item != nullptr );
    assert( IsMapped() );

    if( m_failed )
        return nullptr;

    unsigned int itemSize = m_item->GetSize();
    unsigned int newSize = itemSize + aSize;

    if( newSize > m_chunkSize )
    {
        // There is not enough space in the currently reserved chunk, so we have to resize it
        if( !reallocate( newSize ) )
        {
            m_failed = true;
            return nullptr;
        }
    }

    VERTEX* reserved = &m_vertices[m_chunkOffset + itemSize];

    // Now the item officially possesses the memory chunk
    m_item->setSize( newSize );

    // The content has to be updated
    m_dirty = true;

#if CACHED_CONTAINER_TEST > 0
    test();
#endif
#if CACHED_CONTAINER_TEST > 2
    showFreeChunks();
    showUsedChunks();
#endif

    return reserved;
}


void CACHED_CONTAINER::Delete( VERTEX_ITEM* aItem )
{
    assert( aItem != nullptr );
    assert( m_items.find( aItem ) != m_items.end() || aItem->GetSize() == 0 );

    int size = aItem->GetSize();

    if( size == 0 )
        return; // Item is not stored here

    int offset = aItem->GetOffset();

    // Insert a free memory chunk entry in the place where item was stored
    addFreeChunk( offset, size );

    // Indicate that the item is not stored in the container anymore
    aItem->setSize( 0 );

    m_items.erase( aItem );

#if CACHED_CONTAINER_TEST > 0
    test();
#endif

    // This dynamic memory freeing optimize memory usage, but in fact can create
    // out of memory issues because freeing and reallocation large chunks of memory
    // can create memory fragmentation and no room to reallocate large chunks
    // after many free/reallocate cycles during a session using the same complex board
    // So it can be disable.
    // Currently: it is disable to avoid "out of memory" issues
#if 0
    // Dynamic memory freeing, there is no point in holding
    // a large amount of memory when there is no use for it
    if( m_freeSpace > ( 0.75 * m_currentSize ) && m_currentSize > m_initialSize )
    {
        defragmentResize( 0.5 * m_currentSize );
    }
#endif
}


void CACHED_CONTAINER::Clear()
{
    m_freeSpace = m_currentSize;
    m_maxIndex = 0;
    m_failed = false;

    // Set the size of all the stored VERTEX_ITEMs to 0, so it is clear that they are not held
    // in the container anymore
    for( ITEMS::iterator it = m_items.begin(); it != m_items.end(); ++it )
        ( *it )->setSize( 0 );

    m_items.clear();

    // Now there is only free space left
    m_freeChunks.clear();
    m_freeChunks.insert( std::make_pair( m_freeSpace, 0 ) );
}


bool CACHED_CONTAINER::reallocate( unsigned int aSize )
{
    assert( aSize > 0 );
    assert( IsMapped() );

    unsigned int itemSize = m_item->GetSize();

    // Find a free space chunk >= aSize
    FREE_CHUNK_MAP::iterator newChunk = m_freeChunks.lower_bound( aSize );

    // Is there enough space to store vertices?
    if( newChunk == m_freeChunks.end() )
    {
        bool result;

        // Would it be enough to double the current space?
        if( aSize < m_freeSpace + m_currentSize )
        {
            // Yes: exponential growing
            result = defragmentResize( m_currentSize * 2 );
        }
        else
        {
            // No: grow to the nearest greater power of 2
            result = defragmentResize( pow( 2, ceil( log2( m_currentSize * 2 + aSize ) ) ) );
        }

        if( !result )
            return false;

        newChunk = m_freeChunks.lower_bound( aSize );
        assert( newChunk != m_freeChunks.end() );
    }

    // Parameters of the allocated chunk
    unsigned int newChunkSize = getChunkSize( *newChunk );
    unsigned int newChunkOffset = getChunkOffset( *newChunk );

    assert( newChunkSize >= aSize );
    assert( newChunkOffset < m_currentSize );

    // Check if the item was previously stored in the container
    if( itemSize > 0 )
    {
        // The item was reallocated, so we have to copy all the old data to the new place
        memcpy( &m_vertices[newChunkOffset], &m_vertices[m_chunkOffset], itemSize * VERTEX_SIZE );

        // Free the space used by the previous chunk
        addFreeChunk( m_chunkOffset, m_chunkSize );
    }

    // Remove the new allocated chunk from the free space pool
    m_freeChunks.erase( newChunk );
    m_freeSpace -= newChunkSize;

    m_chunkSize = newChunkSize;
    m_chunkOffset = newChunkOffset;

    m_item->setOffset( m_chunkOffset );

    return true;
}


void CACHED_CONTAINER::defragment( VERTEX* aTarget )
{
    // Defragmentation
    ITEMS::iterator it, it_end;
    int             newOffset = 0;

    [&]()
    {
#ifdef __WIN32__
    #ifdef __MINGW32__
        // currently, because SEH (Structured Exception Handling) is not documented on msys
        // (for instance __try or __try1 exists without doc) or is not supported, do nothing
    #else
        __try
    #endif
#endif
        {
            for( VERTEX_ITEM* item : m_items )
            {
                int itemOffset = item->GetOffset();
                int itemSize = item->GetSize();

                // Move an item to the new container
                memcpy( &aTarget[newOffset], &m_vertices[itemOffset], itemSize * VERTEX_SIZE );

                // Update new offset
                item->setOffset( newOffset );

                // Move to the next free space
                newOffset += itemSize;
            }

            // Move the current item and place it at the end
            if( m_item->GetSize() > 0 )
            {
                memcpy( &aTarget[newOffset], &m_vertices[m_item->GetOffset()],
                        m_item->GetSize() * VERTEX_SIZE );
                m_item->setOffset( newOffset );
                m_chunkOffset = newOffset;
            }
        }
#ifdef __WIN32__
    #ifdef __MINGW32__
        // currently, because SEH (Structured Exception Handling) is not documented on msys
        // (for instance __except1 exists without doc) or is not supported, do nothing
    #else
        __except( GetExceptionCode() == STATUS_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER
                                                                : EXCEPTION_CONTINUE_SEARCH )
        {
            throw std::runtime_error(
                    "Access violation in defragment. This is usually an indicator of "
                    "system or GPU memory running low." );
        };
    #endif
#endif
    }();

    m_maxIndex = usedSpace();
}


void CACHED_CONTAINER::mergeFreeChunks()
{
    if( m_freeChunks.size() <= 1 ) // There are no chunks that can be merged
        return;

#ifdef KICAD_GAL_PROFILE
    PROF_TIMER totalTime;
#endif /* KICAD_GAL_PROFILE */

    // Reversed free chunks map - this one stores chunk size with its offset as the key
    std::list<CHUNK> freeChunks;

    FREE_CHUNK_MAP::const_iterator it, it_end;

    for( it = m_freeChunks.begin(), it_end = m_freeChunks.end(); it != it_end; ++it )
    {
        freeChunks.emplace_back( it->second, it->first );
    }

    m_freeChunks.clear();
    freeChunks.sort();

    std::list<CHUNK>::const_iterator itf, itf_end;
    unsigned int                     offset = freeChunks.front().first;
    unsigned int                     size = freeChunks.front().second;
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
            size = itf->second;
        }
    }

    // Add the last one
    m_freeChunks.insert( std::make_pair( size, offset ) );

#if CACHED_CONTAINER_TEST > 0
    test();
#endif
}


void CACHED_CONTAINER::addFreeChunk( unsigned int aOffset, unsigned int aSize )
{
    assert( aOffset + aSize <= m_currentSize );
    assert( aSize > 0 );

    m_freeChunks.insert( std::make_pair( aSize, aOffset ) );
    m_freeSpace += aSize;
}


void CACHED_CONTAINER::showFreeChunks()
{
}


void CACHED_CONTAINER::showUsedChunks()
{
}


void CACHED_CONTAINER::test()
{
#ifdef KICAD_GAL_PROFILE
    // Free space check
    unsigned int             freeSpace = 0;
    FREE_CHUNK_MAP::iterator itf;

    for( itf = m_freeChunks.begin(); itf != m_freeChunks.end(); ++itf )
        freeSpace += getChunkSize( *itf );

    assert( freeSpace == m_freeSpace );

    // Used space check
    unsigned int    used_space = 0;
    ITEMS::iterator itr;

    for( itr = m_items.begin(); itr != m_items.end(); ++itr )
        used_space += ( *itr )->GetSize();

    // If we have a chunk assigned, then there must be an item edited
    assert( m_chunkSize == 0 || m_item );

    // Currently reserved chunk is also counted as used
    used_space += m_chunkSize;

    assert( ( m_freeSpace + used_space ) == m_currentSize );

    // Overlapping check TODO
#endif /* KICAD_GAL_PROFILE */
}
