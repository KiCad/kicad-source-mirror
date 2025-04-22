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

#include <gal/opengl/cached_container_gpu.h>
#include <gal/opengl/vertex_manager.h>
#include <gal/opengl/vertex_item.h>
#include <gal/opengl/shader.h>
#include <gal/opengl/utils.h>

#include <wx/log.h>

#include <list>

#include <core/profile.h>
#include <trace_helpers.h>

using namespace KIGFX;

/**
 * Flag to enable debug output of the GAL OpenGL GPU cached container.
 *
 * Use "KICAD_GAL_CACHED_CONTAINER_GPU" to enable GAL OpenGL GPU cached container tracing.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* const traceGalCachedContainerGpu = wxT( "KICAD_GAL_CACHED_CONTAINER_GPU" );


CACHED_CONTAINER_GPU::CACHED_CONTAINER_GPU( unsigned int aSize ) :
        CACHED_CONTAINER( aSize ),
        m_isMapped( false ),
        m_glBufferHandle( -1 )
{
    m_useCopyBuffer = GLEW_ARB_copy_buffer;

    wxString vendor( glGetString( GL_VENDOR ) );

    // workaround for intel GPU drivers:
    // disable glCopyBuffer, causes crashes/freezes on certain driver versions
    // Note, Intel's GL_VENDOR string varies depending on GPU/driver generation
    // But generally always starts with Intel at least
    if( vendor.StartsWith( "Intel" ) || vendor.Contains( "etnaviv" ) )
    {
        m_useCopyBuffer = false;
    }

    KI_TRACE( traceGalProfile, "VBO initial size: %d\n", m_currentSize );

    glGenBuffers( 1, &m_glBufferHandle );
    glBindBuffer( GL_ARRAY_BUFFER, m_glBufferHandle );
    glBufferData( GL_ARRAY_BUFFER, m_currentSize * VERTEX_SIZE, nullptr, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    checkGlError( "allocating video memory for cached container", __FILE__, __LINE__ );
}


CACHED_CONTAINER_GPU::~CACHED_CONTAINER_GPU()
{
    if( m_isMapped )
        Unmap();

    if( glDeleteBuffers )
        glDeleteBuffers( 1, &m_glBufferHandle );
}


void CACHED_CONTAINER_GPU::Map()
{
    wxCHECK( !IsMapped(), /*void*/ );

    // OpenGL version might suddenly stop being available in Windows when an RDP session is started
    if( !glBindBuffer )
        throw std::runtime_error( "OpenGL no longer available!" );

    glBindBuffer( GL_ARRAY_BUFFER, m_glBufferHandle );
    m_vertices = static_cast<VERTEX*>( glMapBuffer( GL_ARRAY_BUFFER, GL_READ_WRITE ) );

    if( checkGlError( "mapping vertices buffer", __FILE__, __LINE__ ) == GL_NO_ERROR )
        m_isMapped = true;
}


void CACHED_CONTAINER_GPU::Unmap()
{
    wxCHECK( IsMapped(), /*void*/ );

    // This gets called from ~CACHED_CONTAINER_GPU.  To avoid throwing an exception from
    // the dtor, catch it here instead.
    try
    {
        glUnmapBuffer( GL_ARRAY_BUFFER );
        checkGlError( "unmapping vertices buffer", __FILE__, __LINE__ );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        m_vertices = nullptr;
        checkGlError( "unbinding vertices buffer", __FILE__, __LINE__ );
    }
    catch( const std::runtime_error& err )
    {
        wxLogError( wxT( "OpenGL did not shut down properly.\n\n%s" ), err.what() );
    }

    m_isMapped = false;
}


bool CACHED_CONTAINER_GPU::defragmentResize( unsigned int aNewSize )
{
    if( !m_useCopyBuffer )
        return defragmentResizeMemcpy( aNewSize );

    wxCHECK( IsMapped(), false );

    wxLogTrace( traceGalCachedContainerGpu,
                wxT( "Resizing & defragmenting container from %d to %d" ), m_currentSize,
                aNewSize );

    // No shrinking if we cannot fit all the data
    if( usedSpace() > aNewSize )
        return false;

#ifdef KICAD_GAL_PROFILE
    PROF_TIMER totalTime;
#endif /* KICAD_GAL_PROFILE */

    GLuint newBuffer;

    // glCopyBufferSubData requires a buffer to be unmapped
    glUnmapBuffer( GL_ARRAY_BUFFER );

    // Create a new destination buffer
    glGenBuffers( 1, &newBuffer );

    // It would be best to use GL_COPY_WRITE_BUFFER here,
    // but it is not available everywhere
#ifdef KICAD_GAL_PROFILE
    GLint eaBuffer = -1;
    glGetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING, &eaBuffer );
    wxASSERT( eaBuffer == 0 );
#endif /* KICAD_GAL_PROFILE */
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, newBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, aNewSize * VERTEX_SIZE, nullptr, GL_DYNAMIC_DRAW );
    checkGlError( "creating buffer during defragmentation", __FILE__, __LINE__ );

    ITEMS::iterator it, it_end;
    int             newOffset = 0;

    // Defragmentation
    for( it = m_items.begin(), it_end = m_items.end(); it != it_end; ++it )
    {
        VERTEX_ITEM* item = *it;
        int          itemOffset = item->GetOffset();
        int          itemSize = item->GetSize();

        // Move an item to the new container
        glCopyBufferSubData( GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, itemOffset * VERTEX_SIZE,
                             newOffset * VERTEX_SIZE, itemSize * VERTEX_SIZE );

        // Update new offset
        item->setOffset( newOffset );

        // Move to the next free space
        newOffset += itemSize;
    }

    // Move the current item and place it at the end
    if( m_item->GetSize() > 0 )
    {
        glCopyBufferSubData( GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER,
                             m_item->GetOffset() * VERTEX_SIZE, newOffset * VERTEX_SIZE,
                             m_item->GetSize() * VERTEX_SIZE );

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
    checkGlError( "switching buffers during defragmentation", __FILE__, __LINE__ );

#ifdef KICAD_GAL_PROFILE
    totalTime.Stop();

    wxLogTrace( traceGalCachedContainerGpu, "Defragmented container storing %d vertices / %.1f ms",
                m_currentSize - m_freeSpace, totalTime.msecs() );
#endif /* KICAD_GAL_PROFILE */

    m_freeSpace += ( aNewSize - m_currentSize );
    m_currentSize = aNewSize;

    KI_TRACE( traceGalProfile, "VBO size %d used %d\n", m_currentSize, AllItemsSize() );

    // Now there is only one big chunk of free memory
    m_freeChunks.clear();
    m_freeChunks.insert( std::make_pair( m_freeSpace, m_currentSize - m_freeSpace ) );

    return true;
}


bool CACHED_CONTAINER_GPU::defragmentResizeMemcpy( unsigned int aNewSize )
{
    wxCHECK( IsMapped(), false );

    wxLogTrace( traceGalCachedContainerGpu,
                wxT( "Resizing & defragmenting container (memcpy) from %d to %d" ), m_currentSize,
                aNewSize );

    // No shrinking if we cannot fit all the data
    if( usedSpace() > aNewSize )
        return false;

#ifdef KICAD_GAL_PROFILE
    PROF_TIMER totalTime;
#endif /* KICAD_GAL_PROFILE */

    GLuint  newBuffer;
    VERTEX* newBufferMem;

    // Create the destination buffer
    glGenBuffers( 1, &newBuffer );

    // It would be best to use GL_COPY_WRITE_BUFFER here,
    // but it is not available everywhere
#ifdef KICAD_GAL_PROFILE
    GLint eaBuffer = -1;
    glGetIntegerv( GL_ELEMENT_ARRAY_BUFFER_BINDING, &eaBuffer );
    wxASSERT( eaBuffer == 0 );
#endif /* KICAD_GAL_PROFILE */

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, newBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, aNewSize * VERTEX_SIZE, nullptr, GL_DYNAMIC_DRAW );
    newBufferMem = static_cast<VERTEX*>( glMapBuffer( GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY ) );
    checkGlError( "creating buffer during defragmentation", __FILE__, __LINE__ );

    defragment( newBufferMem );

    // Cleanup
    glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    Unmap();
    glDeleteBuffers( 1, &m_glBufferHandle );

    // Switch to the new vertex buffer
    m_glBufferHandle = newBuffer;
    Map();
    checkGlError( "switching buffers during defragmentation", __FILE__, __LINE__ );

#ifdef KICAD_GAL_PROFILE
    totalTime.Stop();

    wxLogTrace( traceGalCachedContainerGpu, "Defragmented container storing %d vertices / %.1f ms",
                m_currentSize - m_freeSpace, totalTime.msecs() );
#endif /* KICAD_GAL_PROFILE */

    m_freeSpace += ( aNewSize - m_currentSize );
    m_currentSize = aNewSize;

    KI_TRACE( traceGalProfile, "VBO size %d used: %d \n", m_currentSize, AllItemsSize() );

    // Now there is only one big chunk of free memory
    m_freeChunks.clear();
    m_freeChunks.insert( std::make_pair( m_freeSpace, m_currentSize - m_freeSpace ) );

    return true;
}


unsigned int CACHED_CONTAINER_GPU::AllItemsSize() const
{
    unsigned int size = 0;

    for( const auto& item : m_items )
    {
        size += item->GetSize();
    }

    return size;
}

