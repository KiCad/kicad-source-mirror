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

#include <gal/opengl/gpu_manager.h>
#include <gal/opengl/cached_container_gpu.h>
#include <gal/opengl/cached_container_ram.h>
#include <gal/opengl/noncached_container.h>
#include <gal/opengl/shader.h>
#include <gal/opengl/utils.h>
#include <gal/opengl/vertex_item.h>

#include <core/profile.h>

#include <typeinfo>
#include <confirm.h>
#include <trace_helpers.h>

#ifdef KICAD_GAL_PROFILE
#include <core/profile.h>
#include <wx/log.h>
#endif /* KICAD_GAL_PROFILE */

using namespace KIGFX;

GPU_MANAGER* GPU_MANAGER::MakeManager( VERTEX_CONTAINER* aContainer )
{
    if( aContainer->IsCached() )
        return new GPU_CACHED_MANAGER( aContainer );
    else
        return new GPU_NONCACHED_MANAGER( aContainer );
}


GPU_MANAGER::GPU_MANAGER( VERTEX_CONTAINER* aContainer ) :
        m_isDrawing( false ),
        m_container( aContainer ),
        m_shader( nullptr ),
        m_shaderAttrib( 0 ),
        m_enableDepthTest( true )
{
}


GPU_MANAGER::~GPU_MANAGER()
{
}


void GPU_MANAGER::SetShader( SHADER& aShader )
{
    m_shader = &aShader;
    m_shaderAttrib = m_shader->GetAttribute( "a_shaderParams" );

    if( m_shaderAttrib == -1 )
    {
        DisplayError( nullptr, wxT( "Could not get the shader attribute location" ) );
    }
}


// Cached manager
GPU_CACHED_MANAGER::GPU_CACHED_MANAGER( VERTEX_CONTAINER* aContainer ) :
        GPU_MANAGER( aContainer ),
        m_buffersInitialized( false ),
        m_indicesCapacity( 0 ),
        m_totalHuge( 0 ),
        m_totalNormal( 0 ),
        m_indexBufSize( 0 ),
        m_indexBufMaxSize( 0 ),
        m_curVrangeSize( 0 )
{
}


GPU_CACHED_MANAGER::~GPU_CACHED_MANAGER()
{
}


void GPU_CACHED_MANAGER::BeginDrawing()
{
    wxASSERT( !m_isDrawing );

    m_curVrangeSize = 0;
    m_indexBufMaxSize = 0;
    m_indexBufSize = 0;
    m_vranges.clear();

    m_isDrawing = true;
}


void GPU_CACHED_MANAGER::DrawIndices( const VERTEX_ITEM* aItem )
{
    // Hot path: don't use wxASSERT
    assert( m_isDrawing );

    unsigned int offset = aItem->GetOffset();
    unsigned int size = aItem->GetSize();

    if( size == 0 )
        return;

    if( size <= 1000 )
    {
        m_totalNormal += size;
        m_vranges.emplace_back( offset, offset + size - 1, false );
        m_curVrangeSize += size;
    }
    else
    {
        m_totalHuge += size;
        m_vranges.emplace_back( offset, offset + size - 1, true );
        m_indexBufSize = std::max( m_curVrangeSize, m_indexBufSize );
        m_curVrangeSize = 0;
    }
}


void GPU_CACHED_MANAGER::EndDrawing()
{
    wxASSERT( m_isDrawing );

    CACHED_CONTAINER* cached = static_cast<CACHED_CONTAINER*>( m_container );

    if( cached->IsMapped() )
        cached->Unmap();

    m_indexBufSize = std::max( m_curVrangeSize, m_indexBufSize );
    m_indexBufMaxSize = std::max( 2*m_indexBufSize, m_indexBufMaxSize );

    resizeIndices( m_indexBufMaxSize );

    if( m_enableDepthTest )
        glEnable( GL_DEPTH_TEST );
    else
        glDisable( GL_DEPTH_TEST );

    // Prepare buffers
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );

    // Bind vertices data buffers
    glBindBuffer( GL_ARRAY_BUFFER, cached->GetBufferHandle() );
    glVertexPointer( COORD_STRIDE, GL_FLOAT, VERTEX_SIZE, (GLvoid*) COORD_OFFSET );
    glColorPointer( COLOR_STRIDE, GL_UNSIGNED_BYTE, VERTEX_SIZE, (GLvoid*) COLOR_OFFSET );

    if( m_shader != nullptr ) // Use shader if applicable
    {
        m_shader->Use();
        glEnableVertexAttribArray( m_shaderAttrib );
        glVertexAttribPointer( m_shaderAttrib, SHADER_STRIDE, GL_FLOAT, GL_FALSE, VERTEX_SIZE,
                               (GLvoid*) SHADER_OFFSET );
    }

    PROF_TIMER cntDraw( "gl-draw-elements" );

    int     n_ranges = m_vranges.size();
    int     n = 0;
    GLuint* iptr = m_indices.get();
    GLuint  icnt = 0;

    int drawCalls = 0;

    while( n < n_ranges )
    {
        VRANGE* cur = &m_vranges[n];

        if( cur->m_isContinuous )
        {
            if( icnt > 0 )
            {
                glDrawElements( GL_TRIANGLES, icnt, GL_UNSIGNED_INT, m_indices.get() );
                drawCalls++;
            }

            icnt = 0;
            iptr = m_indices.get();

            glDrawArrays( GL_TRIANGLES, cur->m_start, cur->m_end - cur->m_start + 1 );
            drawCalls++;
        }
        else
        {
            for( GLuint i = cur->m_start; i <= cur->m_end; i++ )
            {
                *iptr++ = i;
                icnt++;
            }
        }

        n++;
    }

    if( icnt > 0 )
    {
        glDrawElements( GL_TRIANGLES, icnt, GL_UNSIGNED_INT, m_indices.get() );
        drawCalls++;
    }

    cntDraw.Stop();

    KI_TRACE( traceGalProfile,
              "Cached manager size: VBO size %u iranges %zu max elt size %u drawcalls %u\n",
              cached->AllItemsSize(), m_vranges.size(), m_indexBufMaxSize, drawCalls );
    KI_TRACE( traceGalProfile, "Timing: %s\n", cntDraw.to_string() );

    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    cached->ClearDirty();

    // Deactivate vertex array
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );

    if( m_shader != nullptr )
    {
        glDisableVertexAttribArray( m_shaderAttrib );
        m_shader->Deactivate();
    }

    m_isDrawing = false;
}


void GPU_CACHED_MANAGER::resizeIndices( unsigned int aNewSize )
{
    if( aNewSize > m_indicesCapacity )
    {
        m_indicesCapacity = aNewSize;
        m_indices.reset( new GLuint[m_indicesCapacity] );
    }
}


// Noncached manager
GPU_NONCACHED_MANAGER::GPU_NONCACHED_MANAGER( VERTEX_CONTAINER* aContainer ) :
        GPU_MANAGER( aContainer )
{
}


void GPU_NONCACHED_MANAGER::BeginDrawing()
{
    // Nothing has to be prepared
}


void GPU_NONCACHED_MANAGER::DrawIndices( const VERTEX_ITEM* aItem )
{
    wxASSERT_MSG( false, wxT( "Not implemented yet" ) );
}


void GPU_NONCACHED_MANAGER::EndDrawing()
{
#ifdef KICAD_GAL_PROFILE
    PROF_TIMER totalRealTime;
#endif /* KICAD_GAL_PROFILE */

    if( m_container->GetSize() == 0 )
        return;

    VERTEX*  vertices = m_container->GetAllVertices();
    GLfloat* coordinates = (GLfloat*) ( vertices );
    GLubyte* colors = (GLubyte*) ( vertices ) + COLOR_OFFSET;

    if( m_enableDepthTest )
        glEnable( GL_DEPTH_TEST );
    else
        glDisable( GL_DEPTH_TEST );

    // Prepare buffers
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );

    glVertexPointer( COORD_STRIDE, GL_FLOAT, VERTEX_SIZE, coordinates );
    glColorPointer( COLOR_STRIDE, GL_UNSIGNED_BYTE, VERTEX_SIZE, colors );

    if( m_shader != nullptr ) // Use shader if applicable
    {
        GLfloat* shaders = (GLfloat*) ( vertices ) + SHADER_OFFSET / sizeof( GLfloat );

        m_shader->Use();
        glEnableVertexAttribArray( m_shaderAttrib );
        glVertexAttribPointer( m_shaderAttrib, SHADER_STRIDE, GL_FLOAT, GL_FALSE, VERTEX_SIZE,
                               shaders );
    }

    glDrawArrays( GL_TRIANGLES, 0, m_container->GetSize() );

#ifdef KICAD_GAL_PROFILE
    wxLogTrace( traceGalProfile, wxT( "Noncached manager size: %d" ), m_container->GetSize() );
#endif /* KICAD_GAL_PROFILE */

    // Deactivate vertex array
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );

    if( m_shader != nullptr )
    {
        glDisableVertexAttribArray( m_shaderAttrib );
        m_shader->Deactivate();
    }

    m_container->Clear();

#ifdef KICAD_GAL_PROFILE
    totalRealTime.Stop();
    wxLogTrace( traceGalProfile, wxT( "GPU_NONCACHED_MANAGER::EndDrawing(): %.1f ms" ),
                totalRealTime.msecs() );
#endif /* KICAD_GAL_PROFILE */
}

void GPU_MANAGER::EnableDepthTest( bool aEnabled )
{
    m_enableDepthTest = aEnabled;
}
