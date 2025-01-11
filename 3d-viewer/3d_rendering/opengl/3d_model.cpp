/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Oleg Endo <olegendo@gcc.gnu.org>
 * Copyright (C) 2015-2020 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  3d_model.cpp
 */
#include <algorithm>
#include <stdexcept>
#include <gal/opengl/kiglew.h>    // Must be included first

#include "3d_model.h"
#include "../common_ogl/ogl_utils.h"
#include "../3d_math.h"
#include <utility>
#include <wx/debug.h>
#include <wx/log.h>
#include <chrono>
#include <memory>


/*
 * Flag to enable connectivity profiling.
 *
 * @ingroup trace_env_vars
 */
const wxChar* MODEL_3D::m_logTrace = wxT( "KI_TRACE_EDA_OGL_3DMODEL" );


void MODEL_3D::MakeBbox( const BBOX_3D& aBox, unsigned int aIdxOffset, VERTEX* aVtxOut,
                         GLuint* aIdxOut, const glm::vec4& aColor )
{
    aVtxOut[0].m_pos = { aBox.Min().x, aBox.Min().y, aBox.Min().z };
    aVtxOut[1].m_pos = { aBox.Max().x, aBox.Min().y, aBox.Min().z };
    aVtxOut[2].m_pos = { aBox.Max().x, aBox.Max().y, aBox.Min().z };
    aVtxOut[3].m_pos = { aBox.Min().x, aBox.Max().y, aBox.Min().z };

    aVtxOut[4].m_pos = { aBox.Min().x, aBox.Min().y, aBox.Max().z };
    aVtxOut[5].m_pos = { aBox.Max().x, aBox.Min().y, aBox.Max().z };
    aVtxOut[6].m_pos = { aBox.Max().x, aBox.Max().y, aBox.Max().z };
    aVtxOut[7].m_pos = { aBox.Min().x, aBox.Max().y, aBox.Max().z };

    for( unsigned int i = 0; i < 8; ++i )
        aVtxOut[i].m_color = aVtxOut[i].m_cad_color = glm::clamp( aColor * 255.0f, 0.0f, 255.0f );

    #define bbox_line( vtx_a, vtx_b )\
         do { *aIdxOut++ = vtx_a + aIdxOffset; \
              *aIdxOut++ = vtx_b + aIdxOffset; } while( 0 )

    bbox_line( 0, 1 );
    bbox_line( 1, 2 );
    bbox_line( 2, 3 );
    bbox_line( 3, 0 );

    bbox_line( 4, 5 );
    bbox_line( 5, 6 );
    bbox_line( 6, 7 );
    bbox_line( 7, 4 );

    bbox_line( 0, 4 );
    bbox_line( 1, 5 );
    bbox_line( 2, 6 );
    bbox_line( 3, 7 );

    #undef bbox_line
}


MODEL_3D::MODEL_3D( const S3DMODEL& a3DModel, MATERIAL_MODE aMaterialMode )
{
    wxLogTrace( m_logTrace, wxT( "MODEL_3D::MODEL_3D %u meshes %u materials" ),
                static_cast<unsigned int>( a3DModel.m_MeshesSize ),
                static_cast<unsigned int>( a3DModel.m_MaterialsSize ) );

    auto start_time = std::chrono::high_resolution_clock::now();
    GLuint buffers[8];

    /**
     * WARNING: Horrible hack here!
     * Somehow, buffer values are being shared between Pcbnew and the 3d viewer, which then frees
     * the buffer, resulting in errors in Pcbnew.  To resolve this temporarily, we generate
     * extra buffers in 3dviewer and use the higher numbers.  These are freed on close.
     * todo: Correctly separate the OpenGL contexts to prevent overlapping buffer vals
     */
    glGenBuffers( 6, buffers );
    m_bbox_vertex_buffer = buffers[2];
    m_bbox_index_buffer = buffers[3];
    m_vertex_buffer = buffers[4];
    m_index_buffer = buffers[5];

    // Validate a3DModel pointers
    wxASSERT( a3DModel.m_Materials != nullptr );
    wxASSERT( a3DModel.m_Meshes != nullptr );
    wxASSERT( a3DModel.m_MaterialsSize > 0 );
    wxASSERT( a3DModel.m_MeshesSize > 0 );

    m_materialMode = aMaterialMode;

    if( a3DModel.m_Materials == nullptr || a3DModel.m_Meshes == nullptr
      || a3DModel.m_MaterialsSize == 0 || a3DModel.m_MeshesSize == 0 )
        return;

    // create empty bbox for each mesh.  it will be updated when the vertices are copied.
    m_meshes_bbox.resize( a3DModel.m_MeshesSize );

    // copy materials for later use during rendering.
    m_materials.reserve( a3DModel.m_MaterialsSize );

    for( unsigned int i = 0; i < a3DModel.m_MaterialsSize; ++i )
        m_materials.emplace_back( a3DModel.m_Materials[i] );

    // build temporary vertex and index buffers for bounding boxes.
    // the first box is the outer box.
    std::vector<VERTEX> bbox_tmp_vertices( ( m_meshes_bbox.size() + 1 ) * bbox_vtx_count );
    std::vector<GLuint> bbox_tmp_indices( ( m_meshes_bbox.size() + 1 ) * bbox_idx_count );

    // group all meshes by material.
    // for each material create a combined vertex and index buffer.
    // some models might have many sub-meshes.  so iterate over the
    // input meshes only once.
    struct MESH_GROUP
    {
        std::vector<VERTEX> m_vertices;
        std::vector<GLuint> m_indices;
    };

    std::vector<MESH_GROUP> mesh_groups( m_materials.size() );

    for( unsigned int mesh_i = 0; mesh_i < a3DModel.m_MeshesSize; ++mesh_i )
    {
        const SMESH& mesh = a3DModel.m_Meshes[mesh_i];

        // silently ignore meshes that have invalid material references or invalid geometry.
        if( mesh.m_MaterialIdx >= m_materials.size()
              || mesh.m_Positions == nullptr
              || mesh.m_FaceIdx == nullptr
              || mesh.m_Normals == nullptr
              || mesh.m_FaceIdxSize == 0
              || mesh.m_VertexSize == 0 )
        {
            continue;
        }

        MESH_GROUP& mesh_group = mesh_groups[mesh.m_MaterialIdx];
        MATERIAL& material = m_materials[mesh.m_MaterialIdx];

        if( material.IsTransparent() && m_materialMode != MATERIAL_MODE::DIFFUSE_ONLY )
            m_have_transparent_meshes = true;
        else
            m_have_opaque_meshes = true;

        const unsigned int vtx_offset = mesh_group.m_vertices.size();
        mesh_group.m_vertices.resize( mesh_group.m_vertices.size() + mesh.m_VertexSize );

        // copy vertex data and update the bounding box.
        // use material color for mesh bounding box or some sort of average vertex color.
        glm::vec3 avg_color = material.m_Diffuse;

        BBOX_3D &mesh_bbox = m_meshes_bbox[mesh_i];

        for( unsigned int vtx_i = 0; vtx_i < mesh.m_VertexSize; ++vtx_i )
        {
            mesh_bbox.Union( mesh.m_Positions[vtx_i] );

            VERTEX& vtx_out = mesh_group.m_vertices[vtx_offset + vtx_i];

            vtx_out.m_pos = mesh.m_Positions[vtx_i];
            vtx_out.m_nrm = glm::clamp( glm::vec4( mesh.m_Normals[vtx_i], 1.0f ) * 127.0f,
                                        -127.0f, 127.0f );

            vtx_out.m_tex_uv = mesh.m_Texcoords != nullptr ? mesh.m_Texcoords[vtx_i]
                                                           : glm::vec2 (0);

            if( mesh.m_Color != nullptr )
            {
              avg_color = ( avg_color + mesh.m_Color[vtx_i] ) * 0.5f;

              vtx_out.m_color =
                  glm::clamp( glm::vec4( mesh.m_Color[vtx_i],
                                         1 - material.m_Transparency ) * 255.0f,
                              0.0f, 255.0f );

              vtx_out.m_cad_color =
                  glm::clamp( glm::vec4( MaterialDiffuseToColorCAD( mesh.m_Color[vtx_i] ),
                                         1 ) * 255.0f, 0.0f, 255.0f );
            }
            else
            {
                // the mesh will be rendered with other meshes that might have
                // vertex colors.  thus, we can't enable/disable vertex colors
                // for individual meshes during rendering.

                // if there are no vertex colors, use material color instead.
                vtx_out.m_color =
                    glm::clamp( glm::vec4( material.m_Diffuse,
                                           1 - material.m_Transparency ) * 255.0f,
                                0.0f, 255.0f );

                vtx_out.m_cad_color =
                    glm::clamp( glm::vec4 ( MaterialDiffuseToColorCAD( material.m_Diffuse ),
                                            1 ) * 255.0f,
                                0.0f, 255.0f );
            }
        }

        if( mesh_bbox.IsInitialized() )
        {
            // generate geometry for the bounding box
            MakeBbox( mesh_bbox, ( mesh_i + 1 ) * bbox_vtx_count,
                      &bbox_tmp_vertices[( mesh_i + 1 ) * bbox_vtx_count],
                      &bbox_tmp_indices[( mesh_i + 1 ) * bbox_idx_count],
                      { avg_color, 1.0f } );

            // bump the outer bounding box
            m_model_bbox.Union( mesh_bbox );

            // add to the material group
            material.m_bbox.Union( mesh_bbox );
        }


        // append indices of this mesh to the mesh group.
        const unsigned int idx_offset = mesh_group.m_indices.size();
        unsigned int use_idx_count = mesh.m_FaceIdxSize;

        if( use_idx_count % 3 != 0 )
        {
            wxLogTrace( m_logTrace, wxT( "  index count %u not multiple of 3, truncating" ),
                        static_cast<unsigned int>( use_idx_count ) );
            use_idx_count = ( use_idx_count / 3 ) * 3;
        }

        mesh_group.m_indices.resize( mesh_group.m_indices.size() + use_idx_count );

        for( unsigned int idx_i = 0; idx_i < use_idx_count; ++idx_i )
        {
            if( mesh.m_FaceIdx[idx_i] >= mesh.m_VertexSize )
            {
                wxLogTrace( m_logTrace, wxT( " index %u out of range (%u)" ),
                            static_cast<unsigned int>( mesh.m_FaceIdx[idx_i] ),
                            static_cast<unsigned int>( mesh.m_VertexSize ) );

                // FIXME: should skip this triangle
            }

            mesh_group.m_indices[idx_offset + idx_i] = mesh.m_FaceIdx[idx_i] + vtx_offset;
        }
    }

    // generate geometry for the outer bounding box
    if( m_model_bbox.IsInitialized() )
        MakeBbox( m_model_bbox, 0, &bbox_tmp_vertices[0], &bbox_tmp_indices[0],
                  { 0.0f, 1.0f, 0.0f, 1.0f } );

    // create bounding box buffers
    glGenBuffers( 1, &m_bbox_vertex_buffer );
    glBindBuffer( GL_ARRAY_BUFFER, m_bbox_vertex_buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof( VERTEX ) * bbox_tmp_vertices.size(),
                  bbox_tmp_vertices.data(), GL_STATIC_DRAW );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_bbox_index_buffer );

    if( bbox_tmp_vertices.size() <= std::numeric_limits<GLushort>::max() )
    {
        m_bbox_index_buffer_type = GL_UNSIGNED_SHORT;

        auto u16buf = std::make_unique<GLushort[]>( bbox_tmp_indices.size() );

        for( unsigned int i = 0; i < bbox_tmp_indices.size(); ++i )
          u16buf[i] = static_cast<GLushort>( bbox_tmp_indices[i] );

        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( GLushort ) * bbox_tmp_indices.size(),
                      u16buf.get(), GL_STATIC_DRAW );
    }
    else
    {
        m_bbox_index_buffer_type = GL_UNSIGNED_INT;
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( GLuint ) * bbox_tmp_indices.size(),
                      bbox_tmp_indices.data(), GL_STATIC_DRAW );
    }

    // merge the mesh group geometry data.
    unsigned int total_vertex_count = 0;
    unsigned int total_index_count = 0;

    for( const MESH_GROUP& mg : mesh_groups )
    {
        total_vertex_count += mg.m_vertices.size();
        total_index_count += mg.m_indices.size();
    }

    wxLogTrace( m_logTrace, wxT( "  total %u vertices, %u indices" ),
                total_vertex_count, total_index_count );

    glBindBuffer( GL_ARRAY_BUFFER, m_vertex_buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof( VERTEX ) * total_vertex_count,
                  nullptr, GL_STATIC_DRAW );

    unsigned int idx_size = 0;

    if( total_vertex_count <= std::numeric_limits<GLushort>::max() )
    {
        m_index_buffer_type = GL_UNSIGNED_SHORT;
        idx_size = sizeof( GLushort );
    }
    else
    {
        m_index_buffer_type = GL_UNSIGNED_INT;
        idx_size = sizeof( GLuint );
    }

    // temporary index buffer which will contain either GLushort or GLuint
    // type indices.  allocate with a bit of meadow at the end.
    auto tmp_idx =
            std::make_unique<GLuint[]>( ( idx_size * total_index_count + 8 ) / sizeof( GLuint ) );

    unsigned int prev_vtx_count = 0;
    unsigned int idx_offset = 0;
    unsigned int vtx_offset = 0;

    for( unsigned int mg_i = 0; mg_i < mesh_groups.size (); ++mg_i )
    {
        MESH_GROUP& mg = mesh_groups[mg_i];
        MATERIAL&   mat = m_materials[mg_i];
        uintptr_t   tmp_idx_ptr = reinterpret_cast<uintptr_t>( tmp_idx.get() );

        if( m_index_buffer_type == GL_UNSIGNED_SHORT )
        {
            GLushort* idx_out = reinterpret_cast<GLushort*>( tmp_idx_ptr + idx_offset );

            for( GLuint idx : mg.m_indices )
                *idx_out++ = static_cast<GLushort>( idx + prev_vtx_count );
        }
        else if( m_index_buffer_type == GL_UNSIGNED_INT )
        {
            GLuint* idx_out = reinterpret_cast<GLuint*>( tmp_idx_ptr + idx_offset );

            for( GLuint idx : mg.m_indices )
                *idx_out++ = static_cast<GLuint>( idx + prev_vtx_count );
        }

        glBufferSubData( GL_ARRAY_BUFFER, vtx_offset, mg.m_vertices.size() * sizeof( VERTEX ),
                         mg.m_vertices.data() );

        mat.m_render_idx_buffer_offset = idx_offset;
        mat.m_render_idx_count = mg.m_indices.size();

        prev_vtx_count += mg.m_vertices.size();
        idx_offset += mg.m_indices.size() * idx_size;
        vtx_offset += mg.m_vertices.size() * sizeof( VERTEX );
    }

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_index_buffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, idx_size * total_index_count, tmp_idx.get(),
                  GL_STATIC_DRAW );

    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    auto end_time = std::chrono::high_resolution_clock::now();

    wxLogTrace( m_logTrace, wxT( "  loaded in %u ms\n" ),
               (unsigned int)std::chrono::duration_cast<std::chrono::milliseconds> (
                   end_time - start_time).count() );
}


void MODEL_3D::BeginDrawMulti( bool aUseColorInformation )
{
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_NORMAL_ARRAY );

    if( aUseColorInformation )
    {
        glEnableClientState( GL_COLOR_ARRAY );
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );
        glEnable( GL_COLOR_MATERIAL );
    }

    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
}


void MODEL_3D::EndDrawMulti()
{
    glDisable( GL_COLOR_MATERIAL );
    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_NORMAL_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_TEXTURE_COORD_ARRAY );

    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}


void MODEL_3D::Draw( bool aTransparent, float aOpacity, bool aUseSelectedMaterial,
                     const SFVEC3F& aSelectionColor,
                     const glm::mat4 *aModelWorldMatrix,
                     const SFVEC3F *aCameraWorldPos ) const
{
    if( aOpacity <= FLT_EPSILON )
        return;

    if( !glBindBuffer )
        throw std::runtime_error( "The OpenGL context no longer exists: unable to draw" );

    glBindBuffer( GL_ARRAY_BUFFER, m_vertex_buffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_index_buffer );

    glVertexPointer( 3, GL_FLOAT, sizeof( VERTEX ),
                     reinterpret_cast<const void*>( offsetof( VERTEX, m_pos ) ) );

    glNormalPointer( GL_BYTE, sizeof( VERTEX ),
                     reinterpret_cast<const void*>( offsetof( VERTEX, m_nrm ) ) );

    glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( VERTEX ),
                    reinterpret_cast<const void*>( m_materialMode == MATERIAL_MODE::CAD_MODE
                                                         ? offsetof( VERTEX, m_cad_color )
                                                         : offsetof( VERTEX, m_color ) ) );

    glTexCoordPointer( 2, GL_FLOAT, sizeof( VERTEX ),
                       reinterpret_cast<const void*>( offsetof( VERTEX, m_tex_uv ) ) );

    const SFVEC4F param = SFVEC4F( 1.0f, 1.0f, 1.0f, aOpacity );

    glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, (const float*)&param.x );

    std::vector<const MODEL_3D::MATERIAL *> materialsToRender;

    materialsToRender.reserve( m_materials.size() );

    if( aModelWorldMatrix && aCameraWorldPos )
    {
        // Sort Material groups

        std::vector<std::pair<const MODEL_3D::MATERIAL*, float>> materialsSorted;

        // Calculate the distance to the camera for each material group
        for( const MODEL_3D::MATERIAL& mat : m_materials )
        {
            if( mat.m_render_idx_count == 0 )
            {
                continue;
            }

            if( ( mat.IsTransparent() != aTransparent )
                && ( aOpacity >= 1.0f )
                && m_materialMode != MATERIAL_MODE::DIFFUSE_ONLY )
            {
                continue;
            }

            const BBOX_3D& bBox = mat.m_bbox;
            const SFVEC3F& bBoxCenter = bBox.GetCenter();
            const SFVEC3F bBoxWorld = *aModelWorldMatrix * glm::vec4( bBoxCenter, 1.0f );

            const float distanceToCamera = glm::length( *aCameraWorldPos - bBoxWorld );

            materialsSorted.emplace_back( &mat, distanceToCamera  );
        }

        // Sort from back to front
        std::sort( materialsSorted.begin(), materialsSorted.end(),
                [&]( std::pair<const MODEL_3D::MATERIAL*, float>& a,
                     std::pair<const MODEL_3D::MATERIAL*, float>& b )
                {
                    bool aInsideB = a.first->m_bbox.Inside( b.first->m_bbox );
                    bool bInsideA = b.first->m_bbox.Inside( a.first->m_bbox );

                    // If A is inside B, then A is rendered first
                    if( aInsideB != bInsideA )
                        return bInsideA;

                    if( a.second != b.second )
                        return a.second > b.second;

                    return a.first > b.first;       // compare pointers as a last resort
                } );

        for( const std::pair<const MODEL_3D::MATERIAL*, float>& mat : materialsSorted )
        {
            materialsToRender.push_back( mat.first );
        }
    }
    else
    {
        for( const MODEL_3D::MATERIAL& mat : m_materials )
        {
            // There is at least one default material created in case a mesh has no declared
            // materials.  Most meshes have a material, so usually the first material will have
            // nothing to render and is skip.  See S3D::GetModel for more details.
            if( mat.m_render_idx_count == 0 )
            {
                continue;
            }

            if( ( mat.IsTransparent() != aTransparent )
                && ( aOpacity >= 1.0f )
                && m_materialMode != MATERIAL_MODE::DIFFUSE_ONLY )
            {
                continue;
            }

            materialsToRender.push_back( &mat );
        }
    }

    for( const MODEL_3D::MATERIAL* mat : materialsToRender )
    {
        switch( m_materialMode )
        {
        case MATERIAL_MODE::NORMAL:
            OglSetMaterial( *mat, aOpacity, aUseSelectedMaterial, aSelectionColor );
            break;

        case MATERIAL_MODE::DIFFUSE_ONLY:
            OglSetDiffuseMaterial( mat->m_Diffuse, aOpacity, aUseSelectedMaterial,
                                   aSelectionColor );
            break;

        case MATERIAL_MODE::CAD_MODE:
            OglSetDiffuseMaterial( MaterialDiffuseToColorCAD( mat->m_Diffuse ), aOpacity,
                                   aUseSelectedMaterial, aSelectionColor );
            break;

        default:
            break;
        }

        glDrawElements( GL_TRIANGLES, mat->m_render_idx_count, m_index_buffer_type,
                        reinterpret_cast<const void*>(
                                static_cast<uintptr_t>( mat->m_render_idx_buffer_offset ) ) );
    }
}


MODEL_3D::~MODEL_3D()
{
    if( glDeleteBuffers )
    {
        glDeleteBuffers( 1, &m_vertex_buffer );
        glDeleteBuffers( 1, &m_index_buffer );
        glDeleteBuffers( 1, &m_bbox_vertex_buffer );
        glDeleteBuffers( 1, &m_bbox_index_buffer );
    }
}


void MODEL_3D::DrawBbox() const
{
    if( !glBindBuffer )
        throw std::runtime_error( "The OpenGL context no longer exists: unable to draw bbox" );

    glBindBuffer( GL_ARRAY_BUFFER, m_bbox_vertex_buffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_bbox_index_buffer );

    glVertexPointer( 3, GL_FLOAT, sizeof( VERTEX ),
                     reinterpret_cast<const void*>( offsetof( VERTEX, m_pos ) ) );

    glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( VERTEX ),
                    reinterpret_cast<const void*>( offsetof( VERTEX, m_color ) ) );

    glDrawElements( GL_LINES, bbox_idx_count, m_bbox_index_buffer_type,
                    reinterpret_cast<const void*>( 0 ) );
}


void MODEL_3D::DrawBboxes() const
{
    if( !glBindBuffer )
        throw std::runtime_error( "The OpenGL context no longer exists: unable to draw bboxes" );

    glBindBuffer( GL_ARRAY_BUFFER, m_bbox_vertex_buffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_bbox_index_buffer );

    glVertexPointer( 3, GL_FLOAT, sizeof( VERTEX ),
                     reinterpret_cast<const void*>( offsetof( VERTEX, m_pos ) ) );

    glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( VERTEX ),
                    reinterpret_cast<const void*>( offsetof( VERTEX, m_color ) ) );

    unsigned int idx_size = m_bbox_index_buffer_type == GL_UNSIGNED_SHORT ? sizeof( GLushort )
                                                                          : sizeof( GLuint );

    glDrawElements( GL_LINES, bbox_idx_count * m_meshes_bbox.size(), m_bbox_index_buffer_type,
                    reinterpret_cast<const void*>(
                        static_cast<uintptr_t>( bbox_idx_count * idx_size ) ) );
}

