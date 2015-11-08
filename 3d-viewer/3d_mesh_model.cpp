/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file 3d_mesh_model.cpp
 * @brief
 */

#include <fctsys.h>
#include <3d_mesh_model.h>
#include <boost/geometry/algorithms/area.hpp>
#define GLM_FORCE_RADIANS
#include <gal/opengl/glm/gtc/matrix_transform.hpp>
#include <gal/opengl/glm/glm.hpp>

#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/glu.h>
#  else
#    include <glu.h>
#  endif
#else
#  include <GL/glu.h>
#endif

#ifdef USE_OPENMP
#include <omp.h>
#endif  // USE_OPENMP

#include "info3d_visu.h"


S3D_MESH::S3D_MESH()
{
    isPerFaceNormalsComputed    = false;
    isPointNormalizedComputed   = false;
    isPerPointNormalsComputed   = false;
    isPerVertexNormalsVerified  = false;
    m_Materials = NULL;
    childs.clear();

    m_translation   = glm::vec3( 0.0f, 0.0f, 0.0f );
    m_rotation      = glm::vec4( 0.0f, 0.0f, 0.0f, 0.0f );
    m_scale         = glm::vec3( 1.0f, 1.0f, 1.0f );
}


S3D_MESH::~S3D_MESH()
{
}


CBBOX &S3D_MESH::getBBox( )
{
    if( !m_BBox.IsInitialized() )
        calcBBoxAllChilds();

    return m_BBox;
}


void S3D_MESH::calcBBoxAllChilds( )
{
    // Calc your own boudingbox
    calcBBox();

    for( unsigned int idx = 0; idx < childs.size(); idx++ )
        m_BBox.Union( childs[idx]->getBBox() );

    CBBOX tmpBBox = m_BBox;

    // Calc transformation matrix
    glm::mat4 fullTransformMatrix;
    glm::mat4   translationMatrix       = glm::translate( glm::mat4(),       m_translation );

    if( m_rotation[3] != 0.0f )
    {
        glm::mat4   rotationMatrix      = glm::rotate(    translationMatrix, glm::radians( m_rotation[3] ),
                                                                             S3D_VERTEX( m_rotation[0], m_rotation[1], m_rotation[2] ) );
                    fullTransformMatrix = glm::scale(     rotationMatrix,    m_scale );
    }
    else
        fullTransformMatrix = glm::scale(     translationMatrix, m_scale );


    // Apply transformation
    m_BBox.Set( S3D_VERTEX( fullTransformMatrix * glm::vec4( tmpBBox.Min(), 1.0f ) ),
                S3D_VERTEX( fullTransformMatrix * glm::vec4( tmpBBox.Max(), 1.0f ) ) );
}


void S3D_MESH::calcBBox( )
{
    CBBOX tmpBBox;

    bool firstBBox = true;

    // Calc boudingbox for all coords
    for( unsigned int idx = 0; idx < m_CoordIndex.size(); idx++ )
    {
        for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
            if( firstBBox )
            {
                firstBBox = false;
                tmpBBox = CBBOX( m_Point[m_CoordIndex[idx][ii]] );              // Initialize with the first vertex found
            }
            else
                tmpBBox.Union( m_Point[m_CoordIndex[idx][ii]] );
    }

    m_BBox = tmpBBox;
}


void S3D_MESH::openGL_RenderAllChilds(  bool aIsRenderingJustNonTransparentObjects,
                                        bool aIsRenderingJustTransparentObjects )
{
    glEnable( GL_COLOR_MATERIAL ) ;
    SetOpenGlDefaultMaterial();

    glPushMatrix();
    glTranslatef( m_translation.x, m_translation.y, m_translation.z );
    glRotatef( m_rotation[3], m_rotation[0], m_rotation[1], m_rotation[2] );
    glScalef( m_scale.x, m_scale.y, m_scale.z );

    // Render your self
    openGL_Render( aIsRenderingJustNonTransparentObjects,
                   aIsRenderingJustTransparentObjects );

    // Render childs recursively
    for( unsigned int idx = 0; idx < childs.size(); idx++ )
    {
        childs[idx]->openGL_RenderAllChilds( aIsRenderingJustNonTransparentObjects,
                                             aIsRenderingJustTransparentObjects );
    }

    SetOpenGlDefaultMaterial();

    glPopMatrix();
}


void S3D_MESH::openGL_Render( bool aIsRenderingJustNonTransparentObjects,
                              bool aIsRenderingJustTransparentObjects )
{
    if( (aIsRenderingJustNonTransparentObjects == true) &&
        (aIsRenderingJustTransparentObjects == true) )
    {
        return;
    }

    //DBG( printf( "openGL_Render" ) );
    bool useMaterial = g_Parm_3D_Visu.GetFlag( FL_RENDER_MATERIAL );
    bool smoothShapes = g_Parm_3D_Visu.IsRealisticMode()
                        && g_Parm_3D_Visu.GetFlag( FL_RENDER_SMOOTH_NORMALS );

    if( m_CoordIndex.size() == 0 )
    {
        return;
    }
/*
    // DEBUG INFO
    printf("aIsRenderingJustNonTransparentObjects %d aIsRenderingJustTransparentObjects %d\n", aIsRenderingJustNonTransparentObjects, aIsRenderingJustTransparentObjects);

    printf("m_CoordIndex.size() %lu\n", m_CoordIndex.size() );
    printf("m_MaterialIndexPerFace.size() %lu\n", m_MaterialIndexPerFace.size() );
    printf("m_MaterialIndexPerVertex.size() %lu\n", m_MaterialIndexPerVertex.size() );
    printf("m_PerVertexNormalsNormalized.size() %lu\n", m_PerVertexNormalsNormalized.size() );
    printf("m_PerFaceVertexNormals.size() %lu\n", m_PerFaceVertexNormals.size() );
    printf("m_PerFaceNormalsNormalized.size() %lu\n", m_PerFaceNormalsNormalized.size() );

    printf("smoothShapes %d\n", smoothShapes );

    if( m_Materials )
    {
        printf(" m_Name %s\n", static_cast<const char*>(m_Materials->m_Name.c_str()) );
        printf(" m_ColorPerVertex %d\n", m_Materials->m_ColorPerVertex );
        printf(" m_Transparency.size() %lu\n", m_Materials->m_Transparency.size() );
        printf(" m_DiffuseColor.size() %lu\n", m_Materials->m_DiffuseColor.size() );
        printf(" m_Shininess.size() %lu\n", m_Materials->m_Shininess.size() );
        printf(" m_EmissiveColor.size() %lu\n", m_Materials->m_EmissiveColor.size() );
        printf(" m_SpecularColor.size() %lu\n", m_Materials->m_SpecularColor.size() );
        printf(" m_AmbientColor.size() %lu\n", m_Materials->m_AmbientColor.size() );
    }
    printf("m_Materials %p\n", ( void * )m_Materials );
*/

    float lastTransparency_value = 0.0f;

    if( m_Materials )
    {
        bool isTransparent = m_Materials->SetOpenGLMaterial( 0, useMaterial );

        if( isTransparent && aIsRenderingJustNonTransparentObjects )
            return;

        if( !isTransparent && aIsRenderingJustTransparentObjects )
            return;

        // Skip total transparent models
        if( useMaterial )
            if( m_Materials->m_Transparency.size() > 0 )
            {
                lastTransparency_value = m_Materials->m_Transparency[0];

                if( lastTransparency_value >= 1.0f )
                    return;
            }
    }

    glPushMatrix();
    glTranslatef( m_translation.x, m_translation.y, m_translation.z );
    glRotatef( m_rotation[3], m_rotation[0], m_rotation[1], m_rotation[2] );
    glScalef( m_scale.x, m_scale.y, m_scale.z );

    calcPointNormalized();
    calcPerFaceNormals();

    if( smoothShapes )
    {
        if( (m_PerVertexNormalsNormalized.size() > 0) &&
            g_Parm_3D_Visu.GetFlag( FL_RENDER_USE_MODEL_NORMALS ) )
            perVertexNormalsVerify_and_Repair();
        else
            calcPerPointNormals();

    }
/*
#if defined(DEBUG)
    // Debug Normals
    glColor4f( 1.0, 0.0, 1.0, 0.7 );
    for( unsigned int idx = 0; idx < m_CoordIndex.size(); idx++ )
    {
        if( m_PerFaceNormalsNormalized.size() > 0 )
        {
            S3D_VERTEX normal = m_PerFaceNormalsNormalized[idx];
            //glNormal3fv( &normal.x );

            glm::vec3 point = m_Point[m_CoordIndex[idx][0]];
            for( unsigned int ii = 1; ii < m_CoordIndex[idx].size(); ii++ )
            {
                point += m_Point[m_CoordIndex[idx][ii]];
            }

            point /= m_CoordIndex[idx].size();

            glBegin( GL_LINES );
            glVertex3fv( &point.x );
            point += normal * 0.01f;
            glVertex3fv( &point.x );
            glEnd();
        }
    }

    // Restore material
    if( m_Materials )
        m_Materials->SetOpenGLMaterial( 0, useMaterial );
#endif
*/

/*
#if defined(DEBUG)
    if( smoothShapes )
    {
        // Debug Per Vertex Normals
        glColor4f( 0.0, 1.0, 1.0, 0.7 );
        for( unsigned int idx = 0; idx < m_CoordIndex.size(); idx++ )
        {
            if( (m_PerVertexNormalsNormalized.size() > 0) &&
                g_Parm_3D_Visu.GetFlag( FL_RENDER_USE_MODEL_NORMALS ) )
            {
                for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                {
                    glm::vec3 normal = m_PerVertexNormalsNormalized[m_NormalIndex[idx][ii]];
                    //glNormal3fv( &normal.x );

                    glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                    glBegin( GL_LINES );
                    glVertex3fv( &point.x );
                    point += normal * 1.00f;
                    glVertex3fv( &point.x );
                    glEnd();
                }
            }
            else
            {
                std::vector< glm::vec3 > normals_list;
                normals_list = m_PerFaceVertexNormals[idx];

                for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                {
                    glm::vec3 normal = normals_list[ii];
                    printf("normal(%f, %f, %f), ", normal.x, normal.y, normal.z );

                    //glNormal3fv( &normal.x );

                    glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                    glBegin( GL_LINES );
                    glVertex3fv( &point.x );
                    point += normal * 1.00f;
                    glVertex3fv( &point.x );
                    glEnd();
                }
                printf("\n");
            }
        }

        // Restore material
        if( m_Materials )
            m_Materials->SetOpenGLMaterial( 0, useMaterial );
    }
#endif
*/

    if( m_Materials && m_Materials->m_ColorPerVertex == false )
    {
        if( m_Materials->m_DiffuseColor.size() == m_Point.size() )
            m_Materials->m_ColorPerVertex = true;
    }

    for( unsigned int idx = 0; idx < m_CoordIndex.size(); idx++ )
    {
        if( m_Materials )
        {
            // http://accad.osu.edu/~pgerstma/class/vnv/resources/info/AnnotatedVrmlRef/ch3-323.htm
            // "If colorPerVertex is FALSE, colours are applied to each face, as follows:"
            if( ( m_Materials->m_ColorPerVertex == false ) &&
                ( m_Materials->m_DiffuseColor.size() > 1 ) )
            {
                bool isTransparent;

                // "If the colorIndex field is not empty, then one colour is
                //  used for each face of the IndexedFaceSet. There must be
                //  at least as many indices in the colorIndex field as
                //  there are faces in the IndexedFaceSet."
                if ( m_MaterialIndexPerFace.size() == m_CoordIndex.size() )
                {
                    isTransparent = m_Materials->SetOpenGLMaterial( m_MaterialIndexPerFace[idx], useMaterial );

                    // Skip total transparent faces
                    if( useMaterial )
                        if( (int)m_Materials->m_Transparency.size() > m_MaterialIndexPerFace[idx] )
                        {
                            if( m_Materials->m_Transparency[m_MaterialIndexPerFace[idx]] >= 1.0f )
                                continue;
                        }
                }
                else
                {
                    // "If the colorIndex field is empty, then the colours in the
                    //  Color node are applied to each face of the IndexedFaceSet
                    //  in order. There must be at least as many colours in the
                    //  Color node as there are faces."
                    isTransparent = m_Materials->SetOpenGLMaterial( idx, useMaterial );

                    // Skip total transparent faces
                    if( useMaterial )
                        if( m_Materials->m_Transparency.size() > idx )
                        {
                            if( m_Materials->m_Transparency[idx] >= 1.0f )
                                continue;
                        }
                }

                if( isTransparent && aIsRenderingJustNonTransparentObjects )
                    continue;

                if( !isTransparent && aIsRenderingJustTransparentObjects )
                    continue;
            }
        }

        switch( m_CoordIndex[idx].size() )
        {
        case 3:
            glBegin( GL_TRIANGLES );
            break;
        case 4:
            glBegin( GL_QUADS );
            break;
        default:
            glBegin( GL_POLYGON );
            break;
        }


        if( smoothShapes )
        {
            if( m_Materials )
            {
                // for VRML2:
                // http://accad.osu.edu/~pgerstma/class/vnv/resources/info/AnnotatedVrmlRef/ch3-323.htm
                // "If colorPerVertex is TRUE, colours are applied to each vertex, as follows:
                if( ( m_Materials->m_ColorPerVertex == true ) &&
                    ( m_Materials->m_DiffuseColor.size() > 1 ) )
                {
                    // "If the colorIndex field is not empty, then colours
                    // are applied to each vertex of the IndexedFaceSet in
                    // exactly the same manner that the coordIndex field is
                    // used to choose coordinates for each vertex from the
                    // Coordinate node. The colorIndex field must contain at
                    // least as many indices as the coordIndex field, and
                    // must contain end-of-face markers (-1) in exactly the
                    // same places as the coordIndex field. If the greatest
                    // index in the colorIndex field is N, then there must
                    // be N+1 colours in the Color node."
                    if ( m_MaterialIndexPerVertex.size() != 0 )
                    {
                        if( (m_PerVertexNormalsNormalized.size() > 0) &&
                            g_Parm_3D_Visu.GetFlag( FL_RENDER_USE_MODEL_NORMALS ) )
                        {
                            for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                            {
                                S3D_VERTEX color = m_Materials->m_DiffuseColor[m_MaterialIndexPerVertex[idx][ii]];
                                glColor4f( color.x, color.y, color.z, 1.0f - lastTransparency_value );

                                glm::vec3 normal = m_PerVertexNormalsNormalized[m_NormalIndex[idx][ii]];
                                glNormal3fv( &normal.x );

                                glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                                glVertex3fv( &point.x );
                            }
                        }
                        else
                        {
                            std::vector< glm::vec3 > normals_list;
                            normals_list = m_PerFaceVertexNormals[idx];

                            for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                            {
                                S3D_VERTEX color = m_Materials->m_DiffuseColor[m_MaterialIndexPerVertex[idx][ii]];
                                glColor4f( color.x, color.y, color.z, 1.0f - lastTransparency_value );

                                glm::vec3 normal = normals_list[ii];
                                glNormal3fv( &normal.x );

                                glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                                glVertex3fv( &point.x );
                            }
                        }
                    }
                    else
                    {
                        // "If the colorIndex field is empty, then the
                        // coordIndex field is used to choose colours from
                        // the Color node. If the greatest index in the
                        // coordIndex field is N, then there must be N+1
                        // colours in the Color node."


                        if( (m_PerVertexNormalsNormalized.size() > 0) &&
                            g_Parm_3D_Visu.GetFlag( FL_RENDER_USE_MODEL_NORMALS ) )
                        {
                            for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                            {
                                S3D_VERTEX color = m_Materials->m_DiffuseColor[m_CoordIndex[idx][ii]];
                                glColor4f( color.x, color.y, color.z, 1.0f - lastTransparency_value );

                                glm::vec3 normal = m_PerVertexNormalsNormalized[m_NormalIndex[idx][ii]];
                                glNormal3fv( &normal.x );

                                glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                                glVertex3fv( &point.x );
                            }
                        }
                        else
                        {
                            std::vector< glm::vec3 > normals_list;
                            normals_list = m_PerFaceVertexNormals[idx];

                            for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                            {
                                S3D_VERTEX color = m_Materials->m_DiffuseColor[m_CoordIndex[idx][ii]];
                                glColor4f( color.x, color.y, color.z, 1.0f - lastTransparency_value );

                                glm::vec3 normal = normals_list[ii];
                                glNormal3fv( &normal.x );

                                glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                                glVertex3fv( &point.x );
                            }
                        }
                    }
                }
                else
                {
                    if( (m_PerVertexNormalsNormalized.size() > 0) &&
                        g_Parm_3D_Visu.GetFlag( FL_RENDER_USE_MODEL_NORMALS ) )
                    {
                        for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                        {
                            glm::vec3 normal = m_PerVertexNormalsNormalized[m_NormalIndex[idx][ii]];
                            glNormal3fv( &normal.x );

                            glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                            glVertex3fv( &point.x );
                        }
                    }
                    else
                    {
                        std::vector< glm::vec3 > normals_list;
                        normals_list = m_PerFaceVertexNormals[idx];

                        for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                        {
                            glm::vec3 normal = normals_list[ii];
                            glNormal3fv( &normal.x );

                            glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                            glVertex3fv( &point.x );
                        }
                    }
                }
            }
            else
            {
                if( (m_PerVertexNormalsNormalized.size() > 0) &&
                    g_Parm_3D_Visu.GetFlag( FL_RENDER_USE_MODEL_NORMALS ) )
                {
                    for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                    {
                        glm::vec3 normal = m_PerVertexNormalsNormalized[m_NormalIndex[idx][ii]];
                        glNormal3fv( &normal.x );

                        glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                        glVertex3fv( &point.x );
                    }
                }
                else
                {
                    std::vector< glm::vec3 > normals_list;
                    normals_list = m_PerFaceVertexNormals[idx];

                    for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                    {
                        glm::vec3 normal = normals_list[ii];
                        glNormal3fv( &normal.x );

                        glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                        glVertex3fv( &point.x );
                    }
                }
            }
        }
        else
        {
            // Flat
            if( m_PerFaceNormalsNormalized.size() > 0 )
            {
                S3D_VERTEX normal = m_PerFaceNormalsNormalized[idx];
                glNormal3fv( &normal.x );

                if( m_Materials )
                {
                    // for VRML2:
                    // http://accad.osu.edu/~pgerstma/class/vnv/resources/info/AnnotatedVrmlRef/ch3-323.htm
                    // "If colorPerVertex is TRUE, colours are applied to each vertex, as follows:
                    if( ( m_Materials->m_ColorPerVertex == true ) &&
                        ( m_Materials->m_DiffuseColor.size() > 1 ) )
                    {
                        // "If the colorIndex field is not empty, then colours
                        // are applied to each vertex of the IndexedFaceSet in
                        // exactly the same manner that the coordIndex field is
                        // used to choose coordinates for each vertex from the
                        // Coordinate node. The colorIndex field must contain at
                        // least as many indices as the coordIndex field, and
                        // must contain end-of-face markers (-1) in exactly the
                        // same places as the coordIndex field. If the greatest
                        // index in the colorIndex field is N, then there must
                        // be N+1 colours in the Color node."
                        if ( m_MaterialIndexPerVertex.size() != 0 )
                        {
                            for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                            {
                                S3D_VERTEX color = m_Materials->m_DiffuseColor[m_MaterialIndexPerVertex[idx][ii]];
                                glColor4f( color.x, color.y, color.z, 1.0f - lastTransparency_value );

                                S3D_VERTEX point = m_Point[m_CoordIndex[idx][ii]];
                                glVertex3fv( &point.x );
                            }
                        }
                        else
                        {
                            // "If the colorIndex field is empty, then the
                            // coordIndex field is used to choose colours from
                            // the Color node. If the greatest index in the
                            // coordIndex field is N, then there must be N+1
                            // colours in the Color node."

                            for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                            {
                                S3D_VERTEX color = m_Materials->m_DiffuseColor[m_CoordIndex[idx][ii]];
                                glColor4f( color.x, color.y, color.z, 1.0f - lastTransparency_value );

                                S3D_VERTEX point = m_Point[m_CoordIndex[idx][ii]];
                                glVertex3fv( &point.x );
                            }
                        }
                    }
                    else
                    {
                        for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                        {
                            S3D_VERTEX point = m_Point[m_CoordIndex[idx][ii]];
                            glVertex3fv( &point.x );
                        }
                    }
                }
                else
                {
                    for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                    {
                        S3D_VERTEX point = m_Point[m_CoordIndex[idx][ii]];
                        glVertex3fv( &point.x );
                    }
                }
            }
            else
            {
                for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                {
                    S3D_VERTEX point = m_Point[m_CoordIndex[idx][ii]];
                    glVertex3fv( &point.x );
                }
            }
        }

        glEnd();
    }

    glPopMatrix();
}


void S3D_MESH::perVertexNormalsVerify_and_Repair()
{
    if( isPerVertexNormalsVerified == true )
        return;

    isPerVertexNormalsVerified = true;

    //DBG( printf( "perVertexNormalsVerify_and_Repair\n" ) );

    for( unsigned int idx = 0; idx < m_PerVertexNormalsNormalized.size(); idx++ )
    {
        glm::vec3 normal = m_PerVertexNormalsNormalized[idx];

        if( (normal.x == 1.0f) && ((normal.y != 0.0f) || (normal.z != 0.0f)) )
        {
            normal.y = 0.0f;
            normal.z = 0.0f;
        }
        else
        if( (normal.y == 1.0f) && ((normal.x != 0.0f) || (normal.z != 0.0f)) )
        {
            normal.x = 0.0f;
            normal.z = 0.0f;
        }
        else
        if( (normal.z == 1.0f) && ((normal.x != 0.0f) || (normal.y != 0.0f)) )
        {
            normal.x = 0.0f;
            normal.y = 0.0f;
        }
        else
        if( (normal.x < FLT_EPSILON) && (normal.x > -FLT_EPSILON) )
        {
            normal.x = 0.0f;
        }
        else
        if( (normal.y < FLT_EPSILON) && (normal.y > -FLT_EPSILON) )
        {
            normal.y = 0.0f;
        }
        else
        if( (normal.z < FLT_EPSILON) && (normal.z > -FLT_EPSILON) )
        {
            normal.z = 0.0f;
        }

        float l = glm::length( normal );

        if( l > FLT_EPSILON ) // avoid division by zero
        {
            normal = normal / l;
        }
        else
        {
            DBG( printf( "  Cannot normalize precomputed normal at idx:%u\n", idx ) );
        }

        m_PerVertexNormalsNormalized[idx] = normal;
    }
}


void S3D_MESH::calcPointNormalized()
{
    //DBG( printf( "calcPointNormalized\n" ) );

    if( isPointNormalizedComputed == true )
        return;

    isPointNormalizedComputed = true;

    m_PointNormalized.clear();
    m_PointNormalized.resize( m_Point.size() );

    float biggerPoint = 0.0f;
    for( unsigned int i = 0; i < m_Point.size(); i++ )
    {
        float v;
        v = fabs( m_Point[i].x );
        if( v > biggerPoint )
            biggerPoint = v;

        v = fabs( m_Point[i].y );
        if( v > biggerPoint )
            biggerPoint = v;

        v = fabs( m_Point[i].z );
        if( v > biggerPoint )
            biggerPoint = v;
    }

    for( unsigned int i = 0; i < m_Point.size(); i++ )
    {
        m_PointNormalized[i] = m_Point[i] / biggerPoint;
    }
}


void S3D_MESH::calcPerFaceNormals()
{
    //DBG( printf( "calcPerFaceNormals" ) );

    if( isPerFaceNormalsComputed == true )
        return;

    isPerFaceNormalsComputed = true;

    bool haveAlreadyNormals_from_model_file = false;

    if( ( m_PerFaceNormalsNormalized.size() > 0 ) &&
        g_Parm_3D_Visu.GetFlag( FL_RENDER_USE_MODEL_NORMALS ) )
    {
        haveAlreadyNormals_from_model_file = true;

        // !TODO: this is a workarround for some VRML2 modules files (ex: from we-online.de website)
        // are using (incorrectly) the normals with m_CoordIndex as per face normal.
        // This maybe be addressed by the parser in the future.
        if( ( m_PerFaceNormalsNormalized.size() == m_Point.size() ) &&
            ( m_PerFaceNormalsNormalized.size() != m_CoordIndex.size() ) )
        {
            //DBG( printf("m_PerFaceNormalsNormalized.size() != m_CoordIndex.size() Appling a workarroudn recover\n") );
            m_NormalIndex = m_CoordIndex;
            m_PerVertexNormalsNormalized = m_PerFaceNormalsNormalized;
            m_PerFaceNormalsNormalized.clear();
            haveAlreadyNormals_from_model_file = false;
        }
    }
    else
    {
        m_PerFaceNormalsNormalized.clear();
    }

    m_PerFaceNormalsNormalized.resize( m_CoordIndex.size() );

    m_PerFaceNormalsRaw_X_PerFaceSquaredArea.clear();
    m_PerFaceNormalsRaw_X_PerFaceSquaredArea.resize( m_CoordIndex.size() );

    // There are no points defined for the coordIndex
    if( m_PointNormalized.size() == 0 )
    {
        m_CoordIndex.clear();
        return;
    }

    for( unsigned int idx = 0; idx < m_CoordIndex.size(); idx++ )
    {
        glm::dvec3 cross_prod = glm::dvec3( 0.0, 0.0, 0.0 );

        // Newell's Method
        // http://www.opengl.org/wiki/Calculating_a_Surface_Normal
        // http://tog.acm.org/resources/GraphicsGems/gemsiii/newell.c
        // http://www.iquilezles.org/www/articles/areas/areas.htm

        for( unsigned int i = 0; i < m_CoordIndex[idx].size(); i++ )
        {

            glm::dvec3 u = glm::dvec3( m_PointNormalized[m_CoordIndex[idx][i]] );
            glm::dvec3 v = glm::dvec3( m_PointNormalized[m_CoordIndex[idx][(i + 1) % m_CoordIndex[idx].size()]] );

            cross_prod.x +=  (u.y - v.y) * (u.z + v.z);
            cross_prod.y +=  (u.z - v.z) * (u.x + v.x);
            cross_prod.z +=  (u.x - v.x) * (u.y + v.y);
        }

        double area = glm::dot( cross_prod, cross_prod );

        area = fabs( area );

        m_PerFaceNormalsRaw_X_PerFaceSquaredArea[idx] = glm::vec3( cross_prod * area );

        //printf("cross_prod(%g, %g, %g), area:%g m_PerFaceNormalsRaw_X_PerFaceSquaredArea(%f, %f, %f)\n", cross_prod.x, cross_prod.y, cross_prod.z, area,
        //m_PerFaceNormalsRaw_X_PerFaceSquaredArea[idx].x,
        //m_PerFaceNormalsRaw_X_PerFaceSquaredArea[idx].y,
        //m_PerFaceNormalsRaw_X_PerFaceSquaredArea[idx].z);

        if( haveAlreadyNormals_from_model_file == false )
        {
            if( g_Parm_3D_Visu.GetFlag( FL_RENDER_USE_MODEL_NORMALS ) &&
                (m_PerVertexNormalsNormalized.size() > 0) )
            {
                glm::dvec3 normalSum;

                for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                {
                    normalSum += glm::dvec3( m_PerVertexNormalsNormalized[m_NormalIndex[idx][ii]] );
                }

                double l = glm::length( normalSum );

                if( l > DBL_EPSILON ) // avoid division by zero
                {
                    normalSum = normalSum / l;
                }
                else
                {
                    if( ( normalSum.x > normalSum.y ) && ( normalSum.x > normalSum.z ) )
                    {
                        normalSum.x = 0.0;
                        normalSum.y = 1.0;
                        normalSum.z = 0.0;
                    }
                    else if( ( normalSum.y > normalSum.x ) && ( normalSum.y > normalSum.z ) )
                    {
                        normalSum.x = 0.0;
                        normalSum.y = 1.0;
                        normalSum.z = 0.0;
                    }
                    else if( ( normalSum.z > normalSum.x ) && ( normalSum.z > normalSum.y ) )
                    {
                        normalSum.x = 0.0;
                        normalSum.y = 0.0;
                        normalSum.z = 1.0;
                    }
                    else
                    {
                        normalSum.x = 0.0;
                        normalSum.y = 0.0;
                        normalSum.z = 0.0;
                    }
                }

                m_PerFaceNormalsNormalized[idx] = glm::vec3( normalSum );
            }
            else
            {
                // normalize vertex normal
                double l = glm::length( cross_prod );

                if( l > DBL_EPSILON ) // avoid division by zero
                {
                    cross_prod = cross_prod / l;
                }
                else
                {

                    /*
                    for( unsigned int i = 0; i < m_CoordIndex[idx].size(); i++ )
                    {
                        glm::vec3 v = m_Point[m_CoordIndex[idx][i]];
                        DBG( printf( "v[%u](%f, %f, %f)", i, v.x, v.y, v.z ) );
                    }
                    DBG( printf( "Cannot calc normal idx: %u cross(%g, %g, %g) l:%g m_CoordIndex[idx].size: %u\n",
                            idx,
                            cross_prod.x, cross_prod.y, cross_prod.z,
                            l,
                            (unsigned int)m_CoordIndex[idx].size()) );

                    */

                    if( ( cross_prod.x > cross_prod.y ) && ( cross_prod.x > cross_prod.z ) )
                    {
                        cross_prod.x = 0.0;
                        cross_prod.y = 1.0;
                        cross_prod.z = 0.0;
                    }
                    else if( ( cross_prod.y > cross_prod.x ) && ( cross_prod.y > cross_prod.z ) )
                    {
                        cross_prod.x = 0.0;
                        cross_prod.y = 1.0;
                        cross_prod.z = 0.0;
                    }
                    else if( ( cross_prod.z > cross_prod.x ) && ( cross_prod.z > cross_prod.y ) )
                    {
                        cross_prod.x = 0.0;
                        cross_prod.y = 0.0;
                        cross_prod.z = 1.0;
                    }
                    else
                    {
                        cross_prod.x = 0.0;
                        cross_prod.y = 0.0;
                        cross_prod.z = 0.0;
                    }
                }

                m_PerFaceNormalsNormalized[idx] = glm::vec3( cross_prod );
                //printf("normal(%g, %g, %g)\n", m_PerFaceNormalsNormalized[idx].x, m_PerFaceNormalsNormalized[idx].y, m_PerFaceNormalsNormalized[idx].z );
            }
        }
    }
}


// Documentation literature
// http://www.bytehazard.com/code/vertnorm.html
// http://www.emeyex.com/site/tuts/VertexNormals.pdf
void S3D_MESH::calcPerPointNormals()
{
    //DBG( printf( "calcPerPointNormals" ) );

    if( isPerPointNormalsComputed == true )
        return;

    isPerPointNormalsComputed = true;

    m_PerFaceVertexNormals.clear();

    // Pre-allocate space for the entire vector of vertex normals so we can do parallel writes
    m_PerFaceVertexNormals.resize( m_CoordIndex.size() );

    for( unsigned int each_face_A_idx = 0; each_face_A_idx < m_CoordIndex.size(); each_face_A_idx++ )
    {
        m_PerFaceVertexNormals[each_face_A_idx].resize( m_CoordIndex[each_face_A_idx].size() );
    }


    // Initialize each vertex normal
    for( unsigned int each_face_A_idx = 0; each_face_A_idx < m_CoordIndex.size(); each_face_A_idx++ )
    {
        glm::vec3 initVertexFaceNormal = m_PerFaceNormalsRaw_X_PerFaceSquaredArea[each_face_A_idx];

        std::vector< glm::vec3 >& face_A_normals = m_PerFaceVertexNormals[each_face_A_idx];

        for( unsigned int each_vert_A_idx = 0; each_vert_A_idx < m_CoordIndex[each_face_A_idx].size(); each_vert_A_idx++ )
        {
            face_A_normals[each_vert_A_idx] = initVertexFaceNormal;
        }
    }


    #ifdef USE_OPENMP
    #pragma omp parallel for
    #endif /* USE_OPENMP */

    // for each face A in mesh
    for( unsigned int each_face_A_idx = 0; each_face_A_idx < m_CoordIndex.size(); each_face_A_idx++ )
    {
        // n = face A facet normal
        std::vector< glm::vec3 >& face_A_normals = m_PerFaceVertexNormals[each_face_A_idx];

        // loop through all vertices
        // for each vert in face A
        for( unsigned int each_vert_A_idx = 0; each_vert_A_idx < m_CoordIndex[each_face_A_idx].size(); each_vert_A_idx++ )
        {
            int vertexIndexFromFaceA = (int)(m_CoordIndex[each_face_A_idx][each_vert_A_idx]);
            glm::vec3 vector_face_A = m_PerFaceNormalsNormalized[each_face_A_idx];

            // for each face B in mesh
            for( unsigned int each_face_B_idx = 0; each_face_B_idx < m_CoordIndex.size(); each_face_B_idx++ )
            {
                //if A != B { // ignore self
                if( each_face_A_idx != each_face_B_idx )
                {
                    for( unsigned int ii = 0; ii < m_CoordIndex[each_face_B_idx].size(); ii++ )
                    {
                        // Check if there is any vertice in the face B that touch the vertice in face A
                        if( m_CoordIndex[each_face_B_idx][ii] == vertexIndexFromFaceA )
                        {
                            glm::vec3 vector_face_B = m_PerFaceNormalsNormalized[each_face_B_idx];

                            float dot_prod = glm::dot( vector_face_A, vector_face_B );

                            if( dot_prod > 0.05f )
                                face_A_normals[each_vert_A_idx] += m_PerFaceNormalsRaw_X_PerFaceSquaredArea[each_face_B_idx] * dot_prod;

                            // For each face, only one vertice can touch / share
                            // another vertice from the other face, so we exit here
                            break;
                        }
                    }
                }
            }
        }
    }


    #ifdef USE_OPENMP
    #pragma omp parallel for
    #endif /* USE_OPENMP */

    // Normalize
    for( unsigned int each_face_A_idx = 0; each_face_A_idx < m_CoordIndex.size(); each_face_A_idx++ )
    {
        std::vector< glm::vec3 >& face_A_normals = m_PerFaceVertexNormals[each_face_A_idx];

        for( unsigned int each_vert_A_idx = 0; each_vert_A_idx < m_CoordIndex[each_face_A_idx].size(); each_vert_A_idx++ )
        {
            float l = glm::length( face_A_normals[each_vert_A_idx] );

            if( l > FLT_EPSILON ) // avoid division by zero
                face_A_normals[each_vert_A_idx] /= l;
        }
    }
}
