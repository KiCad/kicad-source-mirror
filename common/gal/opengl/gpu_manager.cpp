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
 * @file gpu_manager.cpp
 * @brief Class to handle uploading vertices and indices to GPU in drawing purposes.
 */

#include <gal/opengl/gpu_manager.h>
#include <gal/opengl/cached_container.h>
#include <gal/opengl/noncached_container.h>
#include <gal/opengl/shader.h>
#include <typeinfo>
#include <wx/msgdlg.h>
#include <confirm.h>
#ifdef PROFILE
#include <profile.h>
#include <wx/debug.h>
#include <wx/log.h>
#endif /* PROFILE */

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
    m_isDrawing( false ), m_container( aContainer ), m_shader( NULL )
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
    GPU_MANAGER( aContainer ), m_buffersInitialized( false ),
    m_indicesSize( 0 )
{
    // Allocate the biggest possible buffer for indices
    m_indices.reset( new GLuint[aContainer->GetSize()] );
}


GPU_CACHED_MANAGER::~GPU_CACHED_MANAGER()
{
    if( m_buffersInitialized )
    {
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glDeleteBuffers( 1, &m_verticesBuffer );
        glDeleteBuffers( 1, &m_indicesBuffer );
    }
}


void GPU_CACHED_MANAGER::Initialize()
{
    wxASSERT( !m_buffersInitialized );

    if( !m_buffersInitialized )
    {
        glGenBuffers( 1, &m_verticesBuffer );
        glGenBuffers( 1, &m_indicesBuffer );
        m_buffersInitialized = true;
    }
}


void GPU_CACHED_MANAGER::BeginDrawing()
{
    wxASSERT( !m_isDrawing );

    if( m_container->IsDirty() )
        uploadToGpu();

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

    m_indicesSize = m_container->GetSize();
    for( unsigned int i = 0; i < m_indicesSize; *m_indicesPtr++ = i++ );
}


void GPU_CACHED_MANAGER::EndDrawing()
{
    wxASSERT( m_isDrawing );

    // Prepare buffers
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );

    // Bind vertices data buffers
    glBindBuffer( GL_ARRAY_BUFFER, m_verticesBuffer );
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
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_indicesSize * sizeof(int), (GLvoid*) m_indices.get(), GL_DYNAMIC_DRAW );

    glDrawElements( GL_TRIANGLES, m_indicesSize, GL_UNSIGNED_INT, 0 );

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
}


void GPU_CACHED_MANAGER::uploadToGpu()
{
#ifdef PROFILE
    prof_counter totalTime;
    prof_start( &totalTime );
#endif /* PROFILE  */

    if( !m_buffersInitialized )
        Initialize();

    int bufferSize    = m_container->GetSize();
    GLfloat* vertices = (GLfloat*) m_container->GetAllVertices();

    // Upload vertices coordinates and shader types to GPU memory
    glBindBuffer( GL_ARRAY_BUFFER, m_verticesBuffer );
    glBufferData( GL_ARRAY_BUFFER, bufferSize * VertexSize, vertices, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // Allocate the biggest possible buffer for indices
    m_indices.reset( new GLuint[bufferSize] );

    if( glGetError() != GL_NO_ERROR )
        DisplayError( NULL, wxT( "Error during data upload to the GPU memory" ) );

#ifdef PROFILE
    prof_end( &totalTime );

    wxLogDebug( wxT( "Uploading %d vertices to GPU / %.1f ms" ), bufferSize, totalTime.msecs() );
#endif /* PROFILE */
}


// Noncached manager
GPU_NONCACHED_MANAGER::GPU_NONCACHED_MANAGER( VERTEX_CONTAINER* aContainer ) :
    GPU_MANAGER( aContainer )
{
}


void GPU_NONCACHED_MANAGER::Initialize()
{
    // Nothing has to be intialized
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

    // Deactivate vertex array
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_VERTEX_ARRAY );

    if( m_shader != NULL )
    {
        glDisableVertexAttribArray( m_shaderAttrib );
        m_shader->Deactivate();
    }
}
