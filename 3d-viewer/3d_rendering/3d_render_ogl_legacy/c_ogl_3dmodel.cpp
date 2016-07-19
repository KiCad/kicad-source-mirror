/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  c_ogl_3dmodel.cpp
 * @brief
 */

#include "c_ogl_3dmodel.h"
#include "ogl_legacy_utils.h"
#include "../common_ogl/ogl_utils.h"
#include "../3d_math.h"
#include <wx/debug.h>


C_OGL_3DMODEL::C_OGL_3DMODEL( const S3DMODEL &a3DModel,
                              MATERIAL_MODE aMaterialMode )
{
    m_ogl_idx_list_opaque = 0;
    m_ogl_idx_list_transparent = 0;
    m_nr_meshes = 0;
    m_meshs_bbox = NULL;

    // Validate a3DModel pointers
    wxASSERT( a3DModel.m_Materials != NULL );
    wxASSERT( a3DModel.m_Meshes != NULL );
    wxASSERT( a3DModel.m_MaterialsSize > 0 );
    wxASSERT( a3DModel.m_MeshesSize > 0 );

    if( (a3DModel.m_Materials != NULL) && (a3DModel.m_Meshes != NULL) &&
        (a3DModel.m_MaterialsSize > 0) && (a3DModel.m_MeshesSize > 0) )
    {
        m_nr_meshes = a3DModel.m_MeshesSize;

        m_meshs_bbox = new CBBOX[a3DModel.m_MeshesSize];

        // Generate m_MeshesSize auxiliar lists to render the meshes
        m_ogl_idx_list_meshes = glGenLists( a3DModel.m_MeshesSize );

        // Render each mesh of the model
        // /////////////////////////////////////////////////////////////////////
        for( unsigned int mesh_i = 0; mesh_i < a3DModel.m_MeshesSize; ++mesh_i )
        {
            if( glIsList( m_ogl_idx_list_meshes + mesh_i ) )
            {
                const SMESH &mesh = a3DModel.m_Meshes[mesh_i];

                // Validate the mesh pointers
                wxASSERT( mesh.m_Positions != NULL );
                wxASSERT( mesh.m_FaceIdx != NULL );
                wxASSERT( mesh.m_Normals != NULL );

                if( (mesh.m_Positions != NULL) &&
                    (mesh.m_Normals != NULL) &&
                    (mesh.m_FaceIdx != NULL) &&
                    (mesh.m_FaceIdxSize > 0) && (mesh.m_VertexSize > 0) )
                {
                    SFVEC4F *pColorRGBA = NULL;

                    // Create the bbox for this mesh
                    // /////////////////////////////////////////////////////////
                    m_meshs_bbox[mesh_i].Reset();

                    for( unsigned int vertex_i = 0;
                         vertex_i < mesh.m_VertexSize;
                         ++vertex_i )
                    {
                        m_meshs_bbox[mesh_i].Union( mesh.m_Positions[vertex_i] );
                    }

                    // Make sure we start with client state disabled
                    // /////////////////////////////////////////////////////////
                    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
                    glDisableClientState( GL_COLOR_ARRAY );


                    // Enable arrays client states
                    // /////////////////////////////////////////////////////////
                    glEnableClientState( GL_VERTEX_ARRAY );
                    glEnableClientState( GL_NORMAL_ARRAY );

                    glVertexPointer( 3, GL_FLOAT, 0, mesh.m_Positions );
                    glNormalPointer( GL_FLOAT, 0, mesh.m_Normals );

                    if( mesh.m_Color != NULL )
                    {
                        glEnableClientState( GL_COLOR_ARRAY );

                        float transparency = 0.0f;

                        if( mesh.m_MaterialIdx < a3DModel.m_MaterialsSize )
                            transparency = a3DModel.m_Materials[mesh.m_MaterialIdx].m_Transparency;

                        if( (transparency > FLT_EPSILON) &&
                            (aMaterialMode ==  MATERIAL_MODE_NORMAL) )
                        {
                            // Create a new array of RGBA colors
                            pColorRGBA = new SFVEC4F[mesh.m_VertexSize];

                            // Copy RGB array and add the Alpha value
                            for( unsigned int i = 0; i < mesh.m_VertexSize; ++i )
                                pColorRGBA[i] = SFVEC4F( mesh.m_Color[i],
                                                         1.0f - transparency );

                            // Load an RGBA array
                            glColorPointer( 4, GL_FLOAT, 0, pColorRGBA );
                        }
                        else
                        {
                            switch( aMaterialMode )
                            {
                            case MATERIAL_MODE_NORMAL:
                            case MATERIAL_MODE_DIFFUSE_ONLY:
                                // load the original RGB color array
                                glColorPointer( 3, GL_FLOAT, 0, mesh.m_Color );
                                break;
                            case MATERIAL_MODE_CAD_MODE:
                                // Create a new array of RGBA colors
                                pColorRGBA = new SFVEC4F[mesh.m_VertexSize];

                                // Copy RGB array and add the Alpha value
                                for( unsigned int i = 0; i < mesh.m_VertexSize; ++i )
                                {
                                    pColorRGBA[i] =
                                            SFVEC4F( MaterialDiffuseToColorCAD( mesh.m_Color[i] ),
                                                     1.0f );
                                }

                                // Load an RGBA array
                                glColorPointer( 4, GL_FLOAT, 0, pColorRGBA );
                                break;
                            default:
                                break;
                            }
                        }
                    }

                    if( mesh.m_Texcoords != NULL )
                    {
                        glEnableClientState( GL_TEXTURE_COORD_ARRAY );
                        glTexCoordPointer( 2, GL_FLOAT, 0, mesh.m_Texcoords );
                    }

                    // Compile the display list to store triangles
                    // /////////////////////////////////////////////////////////
                    glNewList( m_ogl_idx_list_meshes + mesh_i, GL_COMPILE );

                    // Set material properties
                    // /////////////////////////////////////////////////////////

                    if( mesh.m_Color != NULL )
                    {
                        // This enables the use of the Color Pointer information
                        glEnable( GL_COLOR_MATERIAL );
                        glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
                    }
                    else
                    {
                        glDisable( GL_COLOR_MATERIAL );
                    }

                    if( mesh.m_MaterialIdx < a3DModel.m_MaterialsSize )
                    {
                        switch( aMaterialMode )
                        {
                        case MATERIAL_MODE_NORMAL:
                            OGL_SetMaterial( a3DModel.m_Materials[mesh.m_MaterialIdx] );
                            break;
                        case MATERIAL_MODE_DIFFUSE_ONLY:
                            OGL_SetDiffuseOnlyMaterial(
                                        a3DModel.m_Materials[mesh.m_MaterialIdx].m_Diffuse );
                            break;
                        case MATERIAL_MODE_CAD_MODE:
                            OGL_SetDiffuseOnlyMaterial(
                                        MaterialDiffuseToColorCAD(
                                            a3DModel.m_Materials[mesh.m_MaterialIdx].m_Diffuse ) );
                            break;
                        default:
                            break;
                        }
                    }

                    // Draw mesh
                    // /////////////////////////////////////////////////////////
                    glDrawElements( GL_TRIANGLES, mesh.m_FaceIdxSize,
                                    GL_UNSIGNED_INT, mesh.m_FaceIdx );

                    glDisable( GL_COLOR_MATERIAL );

                    glEndList();

                    // Disable arrays client states
                    // /////////////////////////////////////////////////////////
                    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
                    glDisableClientState( GL_COLOR_ARRAY );
                    glDisableClientState( GL_NORMAL_ARRAY );
                    glDisableClientState( GL_VERTEX_ARRAY );

                    glFinish();

                    delete [] pColorRGBA;
                }
            }
        }// for each mesh


        m_ogl_idx_list_opaque = glGenLists( 1 );

        // Check if the generated list is valid
        if( glIsList( m_ogl_idx_list_opaque ) )
        {
            bool have_opaque_meshes = false;
            bool have_transparent_meshes = false;

            // Compile the model display list
            glNewList( m_ogl_idx_list_opaque, GL_COMPILE );

            // Render each mesh display list (opaque first)
            // /////////////////////////////////////////////////////////////////
            for( unsigned int mesh_i = 0; mesh_i < a3DModel.m_MeshesSize; ++mesh_i )
            {
                const SMESH &mesh = a3DModel.m_Meshes[mesh_i];

                if( mesh.m_MaterialIdx < a3DModel.m_MaterialsSize )
                {
                    const SMATERIAL &material = a3DModel.m_Materials[mesh.m_MaterialIdx];

                    if( material.m_Transparency == 0.0f )
                    {
                        have_opaque_meshes = true; // Flag that we have at least one opaque mesh
                        glCallList( m_ogl_idx_list_meshes + mesh_i );
                    }
                    else
                    {
                        have_transparent_meshes = true; // Flag that we found a transparent mesh
                    }
                }
            }

            glEndList();

            if( !have_opaque_meshes )
            {
                // If we dont have opaque meshes, we can free the list
                glDeleteLists( m_ogl_idx_list_opaque, 1 );
                m_ogl_idx_list_opaque = 0;
            }

            if( have_transparent_meshes )
            {
                m_ogl_idx_list_transparent = glGenLists( 1 );

                // Check if the generated list is valid
                if( glIsList( m_ogl_idx_list_transparent ) )
                {
                    // Compile the model display list
                    glNewList( m_ogl_idx_list_transparent, GL_COMPILE );

                    glEnable( GL_BLEND );
                    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                    // Render each mesh display list
                    // /////////////////////////////////////////////////////////
                    for( unsigned mesh_i = 0; mesh_i < a3DModel.m_MeshesSize; ++mesh_i )
                    {
                        const SMESH &mesh = a3DModel.m_Meshes[mesh_i];

                        if( mesh.m_MaterialIdx < a3DModel.m_MaterialsSize )
                        {
                            const SMATERIAL &material = a3DModel.m_Materials[mesh.m_MaterialIdx];

                            // Render the transparent mesh if it have a transparency value
                            if( material.m_Transparency != 0.0f )
                                glCallList( m_ogl_idx_list_meshes + mesh_i );
                        }
                    }

                    glDisable( GL_BLEND );

                    glEndList();
                }
                else
                {
                    m_ogl_idx_list_transparent = 0;
                }
            }
        }
        else
        {
            m_ogl_idx_list_opaque = 0;
        }

        // Create the main bbox
        // /////////////////////////////////////////////////////////////////////
        m_model_bbox.Reset();

        for( unsigned int mesh_i = 0; mesh_i < a3DModel.m_MeshesSize; ++mesh_i )
            m_model_bbox.Union( m_meshs_bbox[mesh_i] );

        glFinish();
    }
}


void C_OGL_3DMODEL::Draw_opaque() const
{
    if( glIsList( m_ogl_idx_list_opaque ) )
        glCallList( m_ogl_idx_list_opaque );
}


void C_OGL_3DMODEL::Draw_transparent() const
{
    if( glIsList( m_ogl_idx_list_transparent ) )
        glCallList( m_ogl_idx_list_transparent );
}


C_OGL_3DMODEL::~C_OGL_3DMODEL()
{
    if( glIsList( m_ogl_idx_list_opaque ) )
        glDeleteLists( m_ogl_idx_list_opaque, 1 );

    if( glIsList( m_ogl_idx_list_transparent ) )
        glDeleteLists( m_ogl_idx_list_transparent, 1 );

    if( glIsList( m_ogl_idx_list_meshes ) )
        glDeleteLists( m_ogl_idx_list_meshes, m_nr_meshes );

    m_ogl_idx_list_meshes = 0;
    m_ogl_idx_list_opaque = 0;
    m_ogl_idx_list_transparent = 0;

    delete[] m_meshs_bbox;
    m_meshs_bbox = NULL;
}


void C_OGL_3DMODEL::Draw_bbox() const
{
    OGL_draw_bbox( m_model_bbox );
}


void C_OGL_3DMODEL::Draw_bboxes() const
{
    for( unsigned int mesh_i = 0; mesh_i < m_nr_meshes; ++mesh_i )
        OGL_draw_bbox( m_meshs_bbox[mesh_i] );
}


bool C_OGL_3DMODEL::Have_opaque() const
{
    return glIsList( m_ogl_idx_list_opaque );
}


bool C_OGL_3DMODEL::Have_transparent() const
{
    return glIsList( m_ogl_idx_list_transparent );
}
