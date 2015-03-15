/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Mario Luzeiro <mrluzeiro@gmail.com>
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


#include <3d_mesh_model.h>
#include <boost/geometry/algorithms/area.hpp>

#ifdef USE_OPENMP
#include <omp.h>
#endif  // USE_OPENMP

S3D_MESH::S3D_MESH()
{
    isPerFaceNormalsComputed = false;
    isPointNormalizedComputed = false;
    isPerPointNormalsComputed = false;
    isPerVertexNormalsVerified = false;
    m_Materials = NULL;
    childs.clear();

    m_translation = glm::vec3( 0.0f, 0.0f, 0.0f );
    m_rotation = glm::vec4( 0.0f, 0.0f, 0.0f, 0.0f );
    m_scale = glm::vec3( 1.0f, 1.0f, 1.0f );
    m_scaleOrientation = glm::vec4( 0.0f, 0.0f, 1.0f, 0.0f );  // not used
    m_center = glm::vec3( 0.0f, 0.0f, 0.0f );                  // not used
}


S3D_MESH::~S3D_MESH()
{
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

    // Render childs
    for( unsigned int idx = 0; idx < childs.size(); idx++ )
    {
        childs[idx]->openGL_Render( aIsRenderingJustNonTransparentObjects,
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

    if( m_Materials && ( m_MaterialIndex.size() == 0 ) )
    {
        bool isTransparent = m_Materials->SetOpenGLMaterial( 0, useMaterial );

        if( isTransparent && aIsRenderingJustNonTransparentObjects )
            return;

        if( !isTransparent && aIsRenderingJustTransparentObjects )
            return;

        if( useMaterial )
            if( m_Materials->m_Transparency.size() > 0 )
                if( m_Materials->m_Transparency[0] >= 1.0f )
                    return;
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

    for( unsigned int idx = 0; idx < m_CoordIndex.size(); idx++ )
    {
        if( m_Materials )
        {
            if ( m_MaterialIndex.size() > 0 )
            {
                bool isTransparent = m_Materials->SetOpenGLMaterial( m_MaterialIndex[idx], useMaterial );
                
                if( isTransparent && aIsRenderingJustNonTransparentObjects )
                    continue;

                if( !isTransparent && aIsRenderingJustTransparentObjects )
                    continue;

                if( useMaterial )
                    if( m_Materials->m_Transparency.size() > idx )
                        if( m_Materials->m_Transparency[idx] >= 1.0f )
                            return;
            }
            else
            {
                // This is only need on debug, because above we are marking the bad elements
                DBG( m_Materials->SetOpenGLMaterial( 0, useMaterial ) );
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
            if( (m_PerVertexNormalsNormalized.size() > 0) &&
                g_Parm_3D_Visu.GetFlag( FL_RENDER_USE_MODEL_NORMALS ) )
            {
                for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                {
                    glm::vec3 normal = m_PerVertexNormalsNormalized[m_NormalIndex[idx][ii]];
                    glNormal3fv( &normal.x );

                    // Flag error vertices
#if defined(DEBUG)
                    if ((normal.x == 0.0) && (normal.y == 0.0) && (normal.z == 0.0))
                        glColor4f( 1.0, 0.0, 1.0, 1.0 );
#endif

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

                    // Flag error vertices
#if defined(DEBUG)
                    if ((normal.x == 0.0) && (normal.y == 0.0) && (normal.z == 0.0))
                        glColor4f( 1.0, 0.0, 1.0, 1.0 );
#endif

                    glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                    glVertex3fv( &point.x );
                }
            }
        }
        else
        {
            // Flat
            if( m_PerFaceNormalsNormalized.size() > 0 )
            {
                glm::vec3 normal = m_PerFaceNormalsNormalized[idx];

                for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                {
                    glNormal3fv( &normal.x );

                    // Flag error vertices
#if defined(DEBUG)
                    if ((normal.x == 0.0) && (normal.y == 0.0) && (normal.z == 0.0))
                        glColor4f( 1.0, 0.0, 1.0, 1.0 );
#endif

                    glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                    glVertex3fv( &point.x );
                }
            }
            else
            {
                for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
                {
                    glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
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

        if( (normal.x == 1.0) && ((normal.y != 0.0) || (normal.z != 0.0)) )
        {
            normal.y = 0.0;
            normal.z = 0.0;
        }
        else
        if( (normal.y == 1.0) && ((normal.x != 0.0) || (normal.z != 0.0)) )
        {
            normal.x = 0.0;
            normal.z = 0.0;
        }
        else
        if( (normal.z == 1.0) && ((normal.x != 0.0) || (normal.y != 0.0)) )
        {
            normal.x = 0.0;
            normal.y = 0.0;
        }
        else
        if( (normal.x < FLT_EPSILON) && (normal.x > -FLT_EPSILON) )
        {
            normal.x = 0.0;
        }
        else
        if( (normal.y < FLT_EPSILON) && (normal.y > -FLT_EPSILON) )
        {
            normal.y = 0.0;
        }
        else
        if( (normal.z < FLT_EPSILON) && (normal.z > -FLT_EPSILON) )
        {
            normal.z = 0.0;
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

    /*
    m_PointNormalized = m_Point;
    */

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

    biggerPoint = 1.0 / biggerPoint;

    for( unsigned int i = 0; i < m_Point.size(); i++ )
    {
        m_PointNormalized[i] = m_Point[i] * biggerPoint;
    }
}


bool IsClockwise( glm::vec3 v0, glm::vec3 v1, glm::vec3 v2 )
{
    double sum = 0.0;

    sum += (v1.x - v0.x) * (v1.y + v0.y);
    sum += (v2.x - v1.x) * (v2.y + v1.y);
    sum += (v0.x - v2.x) * (v0.y + v2.y);

    return sum > FLT_EPSILON;
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
        haveAlreadyNormals_from_model_file = true;
    else
        m_PerFaceNormalsNormalized.clear();

    m_PerFaceNormalsRaw_X_PerFaceSquaredArea.clear();

    m_PerFaceNormalsNormalized.resize( m_CoordIndex.size() );
    m_PerFaceNormalsRaw_X_PerFaceSquaredArea.resize( m_CoordIndex.size() );

    // There are no points defined for the coordIndex
    if( m_PointNormalized.size() == 0 )
    {
        m_CoordIndex.clear();
        return;
    }

    for( unsigned int idx = 0; idx < m_CoordIndex.size(); idx++ )
    {
        glm::vec3 cross_prod;

        cross_prod.x = 0.0;
        cross_prod.y = 0.0;
        cross_prod.z = 0.0;

        // Newell's Method
        // http://www.opengl.org/wiki/Calculating_a_Surface_Normal
        // http://tog.acm.org/resources/GraphicsGems/gemsiii/newell.c
        // http://www.iquilezles.org/www/articles/areas/areas.htm

        for( unsigned int i = 0; i < m_CoordIndex[idx].size(); i++ )
        {
            glm::vec3 u = m_PointNormalized[m_CoordIndex[idx][i]];
            glm::vec3 v = m_PointNormalized[m_CoordIndex[idx][(i + 1) % m_CoordIndex[idx].size()]];

            cross_prod.x +=  (u.y - v.y) * (u.z + v.z);
            cross_prod.y +=  (u.z - v.z) * (u.x + v.x);
            cross_prod.z +=  (u.x - v.x) * (u.y + v.y);

            // This method works same way
            /*
            cross_prod.x += (u.y * v.z) - (u.z * v.y);
            cross_prod.y += (u.z * v.x) - (u.x * v.z);
            cross_prod.z += (u.x * v.y) - (u.y * v.x);*/
        }

        float area = glm::dot( cross_prod, cross_prod );
        area = fabs( area );

        // Dont remmember why this code was used for..
        /*
        if( cross_prod[2] < 0.0 )
            area = -area;

        if( area < FLT_EPSILON )
            area = FLT_EPSILON * 2.0f;
        */

        m_PerFaceNormalsRaw_X_PerFaceSquaredArea[idx] = cross_prod * area;

        if( haveAlreadyNormals_from_model_file == false )
        {
            // normalize vertex normal
            float l = glm::length( cross_prod );

            if( l > FLT_EPSILON ) // avoid division by zero
            {
                cross_prod = cross_prod / l;
            }
            else
            {
                DBG( printf( "Cannot calc normal idx: %u cross(%f, %f, %f) l:%f m_CoordIndex[idx].size: %u\n",
                        idx,
                        cross_prod.x, cross_prod.y, cross_prod.z,
                        l,
                        (unsigned int)m_CoordIndex[idx].size()) );

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

            m_PerFaceNormalsNormalized[idx] = cross_prod;
        }
    }
}


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
