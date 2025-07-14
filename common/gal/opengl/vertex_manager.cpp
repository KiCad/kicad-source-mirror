/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
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
 * @file vertex_manager.cpp
 * @brief Class to control vertex container and GPU with possibility of emulating old-style OpenGL
 * 1.0 state machine using modern OpenGL methods.
 */

#include <gal/opengl/vertex_manager.h>
#include <gal/opengl/cached_container.h>
#include <gal/opengl/noncached_container.h>
#include <gal/opengl/gpu_manager.h>
#include <gal/opengl/vertex_item.h>
#include <confirm.h>
#include <wx/log.h>


/**
 * Flag to enable #VERTEX_MANAGER debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceVertexManager[] = wxT( "KICAD_VERTEX_MANAGER" );


using namespace KIGFX;

VERTEX_MANAGER::VERTEX_MANAGER( bool aCached ) :
        m_noTransform( true ),
        m_transform( 1.0f ),
        m_reserved( nullptr ),
        m_reservedSpace( 0 )
{
    m_container.reset( VERTEX_CONTAINER::MakeContainer( aCached ) );
    m_gpu.reset( GPU_MANAGER::MakeManager( m_container.get() ) );

    // There is no shader used by default
    for( unsigned int i = 0; i < SHADER_STRIDE; ++i )
        m_shader[i] = 0.0f;
}


void VERTEX_MANAGER::Map()
{
    m_container->Map();
}


void VERTEX_MANAGER::Unmap()
{
    m_container->Unmap();
}


bool VERTEX_MANAGER::Reserve( unsigned int aSize )
{
    if( !aSize )
        return true;

    // flags to avoid hanging by calling DisplayError too many times:
    static bool show_err_reserve = true;
    static bool show_err_alloc = true;

    if( m_reservedSpace != 0 || m_reserved )
    {
        if( show_err_reserve )
        {
            DisplayError( nullptr, wxT( "VERTEX_MANAGER::Reserve: Did not use all previous vertices allocated" ) );
            show_err_reserve = false;
        }
    }

    m_reserved = m_container->Allocate( aSize );

    if( m_reserved == nullptr )
    {
        if( show_err_alloc )
        {
            DisplayError( nullptr, wxT( "VERTEX_MANAGER::Reserve: Vertex allocation error" ) );
            show_err_alloc = false;
        }

        return false;
    }

    m_reservedSpace = aSize;

    return true;
}


bool VERTEX_MANAGER::Vertex( GLfloat aX, GLfloat aY, GLfloat aZ )
{
    // flag to avoid hanging by calling DisplayError too many times:
    static bool show_err = true;

    // Obtain the pointer to the vertex in the currently used container
    VERTEX* newVertex;

    if( m_reservedSpace > 0 )
    {
        newVertex = m_reserved++;
        --m_reservedSpace;

        if( m_reservedSpace == 0 )
            m_reserved = nullptr;
    }
    else
    {
        newVertex = m_container->Allocate( 1 );

        if( newVertex == nullptr )
        {
            if( show_err )
            {
                DisplayError( nullptr, wxT( "VERTEX_MANAGER::Vertex: Vertex allocation error" ) );
                show_err = false;
            }

            return false;
        }
    }

    putVertex( *newVertex, aX, aY, aZ );

    return true;
}


bool VERTEX_MANAGER::Vertices( const VERTEX aVertices[], unsigned int aSize )
{
    // flag to avoid hanging by calling DisplayError too many times:
    static bool show_err = true;

    // Obtain pointer to the vertex in currently used container
    VERTEX* newVertex = m_container->Allocate( aSize );

    if( newVertex == nullptr )
    {
        if( show_err )
        {
            DisplayError( nullptr, wxT( "VERTEX_MANAGER::Vertices: Vertex allocation error" ) );
            show_err = false;
        }

        return false;
    }

    // Put vertices in already allocated memory chunk
    for( unsigned int i = 0; i < aSize; ++i )
    {
        putVertex( newVertex[i], aVertices[i].x, aVertices[i].y, aVertices[i].z );
    }

    return true;
}


void VERTEX_MANAGER::SetItem( VERTEX_ITEM& aItem ) const
{
    m_container->SetItem( &aItem );
}


void VERTEX_MANAGER::FinishItem() const
{
    if( m_reservedSpace != 0 || m_reserved )
        wxLogTrace( traceVertexManager, wxS( "Did not use all previous vertices allocated" ) );

    m_container->FinishItem();
}


void VERTEX_MANAGER::FreeItem( VERTEX_ITEM& aItem ) const
{
    m_container->Delete( &aItem );
}


void VERTEX_MANAGER::ChangeItemColor( const VERTEX_ITEM& aItem, const COLOR4D& aColor ) const
{
    unsigned int size = aItem.GetSize();
    unsigned int offset = aItem.GetOffset();

    VERTEX* vertex = m_container->GetVertices( offset );

    for( unsigned int i = 0; i < size; ++i )
    {
        vertex->r = aColor.r * 255.0;
        vertex->g = aColor.g * 255.0;
        vertex->b = aColor.b * 255.0;
        vertex->a = aColor.a * 255.0;
        vertex++;
    }

    m_container->SetDirty();
}


void VERTEX_MANAGER::ChangeItemDepth( const VERTEX_ITEM& aItem, GLfloat aDepth ) const
{
    unsigned int size = aItem.GetSize();
    unsigned int offset = aItem.GetOffset();

    VERTEX* vertex = m_container->GetVertices( offset );

    for( unsigned int i = 0; i < size; ++i )
    {
        vertex->z = aDepth;
        vertex++;
    }

    m_container->SetDirty();
}


VERTEX* VERTEX_MANAGER::GetVertices( const VERTEX_ITEM& aItem ) const
{
    if( aItem.GetSize() == 0 )
        return nullptr; // The item is not stored in the container

    return m_container->GetVertices( aItem.GetOffset() );
}


void VERTEX_MANAGER::SetShader( SHADER& aShader ) const
{
    m_gpu->SetShader( aShader );
}


void VERTEX_MANAGER::Clear() const
{
    m_container->Clear();
}


void VERTEX_MANAGER::BeginDrawing() const
{
    m_gpu->BeginDrawing();
}


void VERTEX_MANAGER::DrawItem( const VERTEX_ITEM& aItem ) const
{
    m_gpu->DrawIndices( &aItem );
}


void VERTEX_MANAGER::EndDrawing() const
{
    m_gpu->EndDrawing();
}


void VERTEX_MANAGER::putVertex( VERTEX& aTarget, GLfloat aX, GLfloat aY, GLfloat aZ ) const
{
    // Modify the vertex according to the currently used transformations
    if( m_noTransform )
    {
        // Simply copy coordinates, when the transform matrix is the identity matrix
        aTarget.x = aX;
        aTarget.y = aY;
        aTarget.z = aZ;
    }
    else
    {
        // Apply transformations
        glm::vec4 transVertex( aX, aY, aZ, 1.0f );
        transVertex = m_transform * transVertex;

        aTarget.x = transVertex.x;
        aTarget.y = transVertex.y;
        aTarget.z = transVertex.z;
    }

    // Apply currently used color
    aTarget.r = m_color[0];
    aTarget.g = m_color[1];
    aTarget.b = m_color[2];
    aTarget.a = m_color[3];

    // Apply currently used shader
    for( unsigned int j = 0; j < SHADER_STRIDE; ++j )
    {
        aTarget.shader[j] = m_shader[j];
    }
}


void VERTEX_MANAGER::EnableDepthTest( bool aEnabled )
{
    m_gpu->EnableDepthTest( aEnabled );
}
