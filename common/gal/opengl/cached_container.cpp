/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
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
#include <gal/opengl/utils.h>

#include <confirm.h>
#include <list>
#include <cassert>

#ifdef __WXDEBUG__
#include <wx/log.h>
#include <profile.h>
#endif /* __WXDEBUG__ */

using namespace KIGFX;

CACHED_CONTAINER::CACHED_CONTAINER( unsigned int aSize ) :
    VERTEX_CONTAINER( aSize ), m_item( NULL ),
    m_chunkSize( 0 ), m_chunkOffset( 0 ), m_isMapped( false ),
    m_isInitialized( false ), m_glBufferHandle( -1 )
{
    // In the beginning there is only free space
    m_freeChunks.insert( std::make_pair( aSize, 0 ) );
}


CACHED_CONTAINER::~CACHED_CONTAINER()
{
    if( m_isMapped )
        Unmap();

    if( m_isInitialized )
    {
        glDeleteBuffers( 1, &m_glBufferHandle );
    }
}


void CACHED_CONTAINER::SetItem( VERTEX_ITEM* aItem )
{
    assert( aItem != NULL );

    unsigned int itemSize = aItem->GetSize();
    m_item      = aItem;
    m_chunkSize = itemSize;
    m_useCopyBuffer = GLEW_ARB_copy_buffer;

    // Get the previously set offset if the item was stored previously
    m_chunkOffset = itemSize > 0 ? aItem->GetOffset() : -1;

#if CACHED_CONTAINER_TEST > 1
    wxLogDebug( wxT( "Adding/editing item 0x%08lx (size %d)" ), (long) m_item, itemSize );
#endif
}


void CACHED_CONTAINER::FinishItem()
{
    assert( m_item != NULL );

    unsigned int itemSize = m_item->GetSize();

    // Finishing the previously edited item
    if( itemSize < m_chunkSize )
    {
        // There is some not used but reserved memory left, so we should return it to the pool
        int itemOffset = m_item->GetOffset();

        // Add the not used memory back to the pool
        addFreeChunk( itemOffset + itemSize, m_chunkSize - itemSize );
        // mergeFreeChunks();   // veery slow and buggy
    }

    if( itemSize > 0 )
        m_items.insert( m_item );

    m_item = NULL;
    m_chunkSize = 0;
    m_chunkOffset = 0;

#if CACHED_CONTAINER_TEST > 1
    wxLogDebug( wxT( "Finishing item 0x%08lx (size %d)" ), (long) m_item, itemSize );
    test();
#endif
}


VERTEX* CACHED_CONTAINER::Allocate( unsigned int aSize )
{
    assert( m_item != NULL );
    assert( m_isMapped );

    if( m_failed )
        return NULL;

    unsigned int itemSize = m_item->GetSize();
    unsigned int newSize = itemSize + aSize;

    if( newSize > m_chunkSize )
    {
        // There is not enough space in the currently reserved chunk, so we have to resize it
        if( !reallocate( newSize ) )
        {
            m_failed = true;
            return NULL;
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
    assert( aItem != NULL );
    assert( m_items.find( aItem ) != m_items.end() || aItem->GetSize() == 0 );

    int size = aItem->GetSize();

    if( size == 0 )
        return;     // Item is not stored here

    int offset = aItem->GetOffset();

#if CACHED_CONTAINER_TEST > 1
    wxLogDebug( wxT( "Removing 0x%08lx (size %d offset %d)" ), (long) aItem, size, offset );
#endif

    // Insert a free memory chunk entry in the place where item was stored
    addFreeChunk( offset, size );

    // Indicate that the item is not stored in the container anymore
    aItem->setSize( 0 );

    m_items.erase( aItem );

#if CACHED_CONTAINER_TEST > 0
    test();
#endif

    // This dynamic memory freeing optimize memory usage, but in fact can create
    // out of memory issues because freeing and reallocation large chuncks of memory
    // can create memory fragmentation and no room to reallocate large chuncks
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
    m_failed = false;

    // Set the size of all the stored VERTEX_ITEMs to 0, so it is clear that they are not held
    // in the container anymore
    for( ITEMS::iterator it = m_items.begin(); it != m_items.end(); ++it )
    {
        ( *it )->setSize( 0 );
    }

    m_items.clear();

    // Now there is only free space left
    m_freeChunks.clear();
    m_freeChunks.insert( std::make_pair( m_freeSpace, 0 ) );
}


void CACHED_CONTAINER::Map()
{
    assert( !IsMapped() );

    if( !m_isInitialized )
        init();

    glBindBuffer( GL_ARRAY_BUFFER, m_glBufferHandle );
    m_vertices = static_cast<VERTEX*>( glMapBuffer( GL_ARRAY_BUFFER, GL_READ_WRITE ) );
    checkGlError( "mapping vertices buffer" );

    m_isMapped = true;
}


void CACHED_CONTAINER::Unmap()
{
    assert( IsMapped() );

    glUnmapBuffer( GL_ARRAY_BUFFER );
    checkGlError( "unmapping vertices buffer" );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    m_vertices = NULL;
    checkGlError( "unbinding vertices buffer" );

    m_isMapped = false;
}


void CACHED_CONTAINER::init()
{
    glGenBuffers( 1, &m_glBufferHandle );
    glBindBuffer( GL_ARRAY_BUFFER, m_glBufferHandle );
    glBufferData( GL_ARRAY_BUFFER, m_currentSize * VertexSize, NULL, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    checkGlError( "allocating video memory for cached container" );

    m_isInitialized = true;
}


bool CACHED_CONTAINER::reallocate( unsigned int aSize )
{
    assert( aSize > 0 );
    assert( m_isMapped );

    unsigned int itemSize = m_item->GetSize();

#if CACHED_CONTAINER_TEST > 2
    wxLogDebug( wxT( "Resize %p from %d to %d" ), m_item, itemSize, aSize );
#endif

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
    unsigned int newChunkSize   = getChunkSize( *newChunk );
    unsigned int newChunkOffset = getChunkOffset( *newChunk );

    assert( newChunkSize >= aSize );
    assert( newChunkOffset < m_currentSize );

    // Check if the item was previously stored in the container
    if( itemSize > 0 )
    {
#if CACHED_CONTAINER_TEST > 3
        wxLogDebug( wxT( "Moving 0x%08x from 0x%08x to 0x%08x" ),
                    (int) m_item, oldChunkOffset, newChunkOffset );
#endif
        // The item was reallocated, so we have to copy all the old data to the new place
        memcpy( &m_vertices[newChunkOffset], &m_vertices[m_chunkOffset], itemSize * VertexSize );

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


void CACHED_CONTAINER::mergeFreeChunks()
{
    if( m_freeChunks.size() <= 1 ) // There are no chunks that can be merged
        return;

#ifdef __WXDEBUG__
    PROF_COUNTER totalTime;
#endif /* __WXDEBUG__ */

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

#ifdef __WXDEBUG__
    totalTime.Stop();
    wxLogDebug( "Merged free chunks / %.1f ms", totalTime.msecs() );
#endif /* __WXDEBUG__ */
#if CACHED_CONTAINER_TEST > 0
    test();
#endif
}


bool CACHED_CONTAINER::defragmentResize( unsigned int aNewSize )
{
    if( !m_useCopyBuffer )
        return defragmentResizeMemcpy( aNewSize );

    assert( IsMapped() );

    wxLogTrace( "GAL_CACHED_CONTAINER",
            wxT( "Resizing & defragmenting container from %d to %d" ), m_currentSize, aNewSize );

    // No shrinking if we cannot fit all the data
    if( usedSpace() > aNewSize )
        return false;

#ifdef __WXDEBUG__
    PROF_COUNTER totalTime;
#endif /* __WXDEBUG__ */

    GLuint newBuffer;

    // glCopyBufferSubData requires a buffer to be unmapped
    glUnmapBuffer( GL_ARRAY_BUFFER );

    // Create the destination buffer
    glGenBuffers( 1, &newBuffer );

    // It would be best to use GL_COPY_WRITE_BUFFER here,
    // but it is not available everywhere
#ifdef __WXDEBUG__
    GLint eaBuffer = -1;
    glGetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING, &eaBuffer );
    assert( eaBuffer == 0 );
#endif /* __WXDEBUG__ */
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, newBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, aNewSize * VertexSize, NULL, GL_DYNAMIC_DRAW );
    checkGlError( "creating buffer during defragmentation" );

    ITEMS::iterator it, it_end;
    int newOffset = 0;

    // Defragmentation
    for( it = m_items.begin(), it_end = m_items.end(); it != it_end; ++it )
    {
        VERTEX_ITEM* item = *it;
        int itemOffset    = item->GetOffset();
        int itemSize      = item->GetSize();

        // Move an item to the new container
        glCopyBufferSubData( GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER,
                itemOffset * VertexSize, newOffset * VertexSize, itemSize * VertexSize );

        // Update new offset
        item->setOffset( newOffset );

        // Move to the next free space
        newOffset += itemSize;
    }

    // Move the current item and place it at the end
    if( m_item->GetSize() > 0 )
    {
        glCopyBufferSubData( GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER,
                m_item->GetOffset() * VertexSize, newOffset * VertexSize,
                m_item->GetSize() * VertexSize );

        m_item->setOffset( newOffset );
        m_chunkOffset = newOffset;
    }

    // Cleanup
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    // Previously we have unmapped the array buffer, now when it is also
    // unbound, it may be officially marked as unmapped
    m_isMapped = false;
    glDeleteBuffers( 1, &m_glBufferHandle );

    // Switch to the new vertex buffer
    m_glBufferHandle = newBuffer;
    Map();
    checkGlError( "switching buffers during defragmentation" );

#ifdef __WXDEBUG__
    totalTime.Stop();

    wxLogTrace( "GAL_CACHED_CONTAINER",
                "Defragmented container storing %d vertices / %.1f ms",
                m_currentSize - m_freeSpace, totalTime.msecs() );
#endif /* __WXDEBUG__ */

    m_freeSpace += ( aNewSize - m_currentSize );
    m_currentSize = aNewSize;

    // Now there is only one big chunk of free memory
    m_freeChunks.clear();
    m_freeChunks.insert( std::make_pair( m_freeSpace, m_currentSize - m_freeSpace ) );

    return true;
}


bool CACHED_CONTAINER::defragmentResizeMemcpy( unsigned int aNewSize )
{
    assert( IsMapped() );

    wxLogTrace( "GAL_CACHED_CONTAINER",
            wxT( "Resizing & defragmenting container (memcpy) from %d to %d" ),
            m_currentSize, aNewSize );

    // No shrinking if we cannot fit all the data
    if( usedSpace() > aNewSize )
        return false;

#ifdef __WXDEBUG__
    PROF_COUNTER totalTime;
#endif /* __WXDEBUG__ */

    GLuint newBuffer;
    VERTEX* newBufferMem;

    // Create the destination buffer
    glGenBuffers( 1, &newBuffer );

    // It would be best to use GL_COPY_WRITE_BUFFER here,
    // but it is not available everywhere
#ifdef __WXDEBUG__
    GLint eaBuffer = -1;
    glGetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING, &eaBuffer );
    assert( eaBuffer == 0 );
#endif /* __WXDEBUG__ */
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, newBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, aNewSize * VertexSize, NULL, GL_DYNAMIC_DRAW );
    newBufferMem = static_cast<VERTEX*>( glMapBuffer( GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY ) );
    checkGlError( "creating buffer during defragmentation" );

    // Defragmentation
    ITEMS::iterator it, it_end;
    int newOffset = 0;

    for( it = m_items.begin(), it_end = m_items.end(); it != it_end; ++it )
    {
        VERTEX_ITEM* item = *it;
        int itemOffset    = item->GetOffset();
        int itemSize      = item->GetSize();

        // Move an item to the new container
        memcpy( &newBufferMem[newOffset], &m_vertices[itemOffset], itemSize * VertexSize );

        // Update new offset
        item->setOffset( newOffset );

        // Move to the next free space
        newOffset += itemSize;
    }

    // Move the current item and place it at the end
    if( m_item->GetSize() > 0 )
    {
        memcpy( &newBufferMem[newOffset], &m_vertices[m_item->GetOffset()],
                m_item->GetSize() * VertexSize );
        m_item->setOffset( newOffset );
        m_chunkOffset = newOffset;
    }

    // Cleanup
    glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    Unmap();
    glDeleteBuffers( 1, &m_glBufferHandle );

    // Switch to the new vertex buffer
    m_glBufferHandle = newBuffer;
    Map();
    checkGlError( "switching buffers during defragmentation" );

#ifdef __WXDEBUG__
    totalTime.Stop();

    wxLogTrace( "GAL_CACHED_CONTAINER",
                "Defragmented container storing %d vertices / %.1f ms",
                m_currentSize - m_freeSpace, totalTime.msecs() );
#endif /* __WXDEBUG__ */

    m_freeSpace += ( aNewSize - m_currentSize );
    m_currentSize = aNewSize;

    // Now there is only one big chunk of free memory
    m_freeChunks.clear();
    m_freeChunks.insert( std::make_pair( m_freeSpace, m_currentSize - m_freeSpace ) );

    return true;
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
#ifdef __WXDEBUG__
    FREE_CHUNK_MAP::iterator it;

    wxLogDebug( wxT( "Free chunks:" ) );

    for( it = m_freeChunks.begin(); it != m_freeChunks.end(); ++it )
    {
        unsigned int offset = getChunkOffset( *it );
        unsigned int size   = getChunkSize( *it );
        assert( size > 0 );

        wxLogDebug( wxT( "[0x%08x-0x%08x] (size %d)" ),
                    offset, offset + size - 1, size );
    }
#endif /* __WXDEBUG__ */
}


void CACHED_CONTAINER::showUsedChunks()
{
#ifdef __WXDEBUG__
    ITEMS::iterator it;

    wxLogDebug( wxT( "Used chunks:" ) );

    for( it = m_items.begin(); it != m_items.end(); ++it )
    {
        VERTEX_ITEM* item   = *it;
        unsigned int offset = item->GetOffset();
        unsigned int size   = item->GetSize();
        assert( size > 0 );

        wxLogDebug( wxT( "[0x%08x-0x%08x] @ 0x%p (size %d)" ),
                    offset, offset + size - 1, item, size );
    }
#endif /* __WXDEBUG__ */
}


void CACHED_CONTAINER::test()
{
#ifdef __WXDEBUG__
    // Free space check
    unsigned int freeSpace = 0;
    FREE_CHUNK_MAP::iterator itf;

    for( itf = m_freeChunks.begin(); itf != m_freeChunks.end(); ++itf )
        freeSpace += getChunkSize( *itf );

    assert( freeSpace == m_freeSpace );

    // Used space check
    unsigned int usedSpace = 0;
    ITEMS::iterator itr;
    for( itr = m_items.begin(); itr != m_items.end(); ++itr )
        usedSpace += ( *itr )->GetSize();

    // If we have a chunk assigned, then there must be an item edited
    assert( m_chunkSize == 0 || m_item );

    // Currently reserved chunk is also counted as used
    usedSpace += m_chunkSize;

    assert( ( m_freeSpace + usedSpace ) == m_currentSize );

    // Overlapping check TODO
#endif /* __WXDEBUG__ */
}

