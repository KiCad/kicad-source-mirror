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
 * @file gpu_manager.cpp
 * @brief Class to handle uploading vertices and indices to GPU in drawing purposes.
 */

#include <gal/opengl/gpu_manager.h>
#include <gal/opengl/cached_container.h>
#include <gal/opengl/noncached_container.h>
#include <gal/opengl/shader.h>
#include <gal/opengl/utils.h>

#include <typeinfo>
#include <confirm.h>

#ifdef __WXDEBUG__
#include <profile.h>
#include <wx/log.h>
#endif /* __WXDEBUG__ */

using namespace KIGFX;

GPU_MANAGER* GPU_MANAGER::MakeManager( VERTEX_CONTAINER* aContainer )
{
    if( typeid( *aContainer ) == typeid( CACHED_CONTAINER ) )
        return new GPU_CACHED_MANAGER( aContainer );
    else if( typeid( *aContainer ) == typeid( NONCACHED_CONTAINER ) )
        return new GPU_NONCACHED_MANAGER( aContainer );

    wxASSERT_MSG( false, wxT( "Not handled container type" ) );
    return NULL;
}


GPU_MANAGER::GPU_MANAGER( VERTEX_CONTAINER* aContainer ) :
    m_isDrawing( false ), m_container( aContainer ), m_shader( NULL ), m_shaderAttrib( 0 )
{
}


GPU_MANAGER::~GPU_MANAGER()
{
}


void GPU_MANAGER::SetShader( SHADER& aShader )
{
    m_shader = &aShader;
    m_shaderAttrib = m_shader->GetAttribute( "attrShaderParams" );

    if( m_shaderAttrib == -1 )
    {
        DisplayError( NULL, wxT( "Could not get the shader attribute location" ) );
    }
}


// Cached manager
GPU_CACHED_MANAGER::GPU_CACHED_MANAGER( VERTEX_CONTAINER* aContainer ) :
    GPU_MANAGER( aContainer ), m_buffersInitialized( false ), m_indicesPtr( NULL ),
    m_indicesBuffer( 0 ), m_indicesSize( 0 ), m_indicesCapacity( 0 )
{
    // Allocate the biggest possible buffer for indices
    resizeIndices( aContainer->GetSize() );
}


GPU_CACHED_MANAGER::~GPU_CACHED_MANAGER()
{
    if( m_buffersInitialized )
    {
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glDeleteBuffers( 1, &m_indicesBuffer );
    }
}


void GPU_CACHED_MANAGER::BeginDrawing()
{
    wxASSERT( !m_isDrawing );

    if( !m_buffersInitialized )
    {
        glGenBuffers( 1, &m_indicesBuffer );
        checkGlError( "generating vertices buffer" );
        m_buffersInitialized = true;
    }

    if( m_container->IsDirty() )
        resizeIndices( m_container->GetSize() );

    // Number of vertices to be drawn in the EndDrawing()
    m_indicesSize = 0;
    // Set the indices pointer to the beginning of the indices-to-draw buffer
    m_indicesPtr = m_indices.get();

    m_isDrawing = true;
}


void GPU_CACHED_MANAGER::DrawIndices( unsigned int aOffset, unsigned int aSize )
{
    wxASSERT( m_isDrawing );

    // Copy indices of items that should be drawn to GPU memory
    for( unsigned int i = aOffset; i < aOffset + aSize; *m_indicesPtr++ = i++ );

    m_indicesSize += aSize;
}


void GPU_CACHED_MANAGER::DrawAll()
{
    wxASSERT( m_isDrawing );

    for( unsigned int i = 0; i < m_indicesSize; *m_indicesPtr++ = i++ );

    m_indicesSize = m_container->GetSize();
}


void GPU_CACHED_MANAGER::EndDrawing()
{
#ifdef __WXDEBUG__
    prof_counter totalRealTime;
    prof_start( &totalRealTime );
#endif /* __WXDEBUG__ */

    wxASSERT( m_isDrawing );

    CACHED_CONTAINER* cached = static_cast<CACHED_CONTAINER*>( m_container );

    if( cached->IsMapped() )
        cached->Unmap();

    if( m_indicesSize == 0 )
    {
        m_isDrawing = false;
        return;
    }

    // Prepare buffers
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );

    // Bind vertices data buffers
    glBindBuffer( GL_ARRAY_BUFFER, cached->GetBufferHandle() );
    glVertexPointer( CoordStride, GL_FLOAT, VertexSize, 0 );
    glColorPointer( ColorStride, GL_UNSIGNED_BYTE, VertexSize, (GLvoid*) ColorOffset );

    if( m_shader != NULL )    // Use shader if applicable
    {
        m_shader->Use();
        glEnableVertexAttribArray( m_shaderAttrib );
        glVertexAttribPointer( m_shaderAttrib, ShaderStride, GL_FLOAT, GL_FALSE,
                               VertexSize, (GLvoid*) ShaderOffset );
    }

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indicesBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_indicesSize * sizeof(int),
            (GLvoid*) m_indices.get(), GL_DYNAMIC_DRAW );

    glDrawElements( GL_TRIANGLES, m_indicesSize, GL_UNSIGNED_INT, 0 );

#ifdef __WXDEBUG__
    wxLogTrace( "GAL_PROFILE", wxT( "Cached manager size: %d" ), m_indicesSize );
#endif /* __WXDEBUG__ */

    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    // Deactivate vertex array
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );

    if( m_shader != NULL )
    {
        glDisableVertexAttribArray( m_shaderAttrib );
        m_shader->Deactivate();
    }

    m_isDrawing = false;

#ifdef __WXDEBUG__
    prof_end( &totalRealTime );
    wxLogTrace( "GAL_PROFILE",
                wxT( "GPU_CACHED_MANAGER::EndDrawing(): %.1f ms" ), totalRealTime.msecs() );
#endif /* __WXDEBUG__ */
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


void GPU_NONCACHED_MANAGER::DrawIndices( unsigned int aOffset, unsigned int aSize )
{
    wxASSERT_MSG( false, wxT( "Not implemented yet" ) );
}


void GPU_NONCACHED_MANAGER::DrawAll()
{
    // This is the default use case, nothing has to be done
    // The real rendering takes place in the EndDrawing() function
}


void GPU_NONCACHED_MANAGER::EndDrawing()
{
#ifdef __WXDEBUG__
    prof_counter totalRealTime;
    prof_start( &totalRealTime );
#endif /* __WXDEBUG__ */

    if( m_container->GetSize() == 0 )
        return;

    VERTEX*  vertices       = m_container->GetAllVertices();
    GLfloat* coordinates    = (GLfloat*) ( vertices );
    GLubyte* colors         = (GLubyte*) ( vertices ) + ColorOffset;

    // Prepare buffers
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );

    glVertexPointer( CoordStride, GL_FLOAT, VertexSize, coordinates );
    glColorPointer( ColorStride, GL_UNSIGNED_BYTE, VertexSize, colors );

    if( m_shader != NULL )    // Use shader if applicable
    {
        GLfloat* shaders = (GLfloat*) ( vertices ) + ShaderOffset / sizeof(GLfloat);

        m_shader->Use();
        glEnableVertexAttribArray( m_shaderAttrib );
        glVertexAttribPointer( m_shaderAttrib, ShaderStride, GL_FLOAT, GL_FALSE,
                               VertexSize, shaders );
    }

    glDrawArrays( GL_TRIANGLES, 0, m_container->GetSize() );

#ifdef __WXDEBUG__
    wxLogTrace( "GAL_PROFILE", wxT( "Noncached manager size: %d" ), m_container->GetSize() );
#endif /* __WXDEBUG__ */

    // Deactivate vertex array
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );

    if( m_shader != NULL )
    {
        glDisableVertexAttribArray( m_shaderAttrib );
        m_shader->Deactivate();
    }

#ifdef __WXDEBUG__
    prof_end( &totalRealTime );
    wxLogTrace( "GAL_PROFILE",
                wxT( "GPU_NONCACHED_MANAGER::EndDrawing(): %.1f ms" ), totalRealTime.msecs() );
#endif /* __WXDEBUG__ */
}
