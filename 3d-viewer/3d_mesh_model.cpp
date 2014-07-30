/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

S3D_MESH::S3D_MESH()
{
    isPerFaceNormalsComputed = false;
    isPointNormalizedComputed = false;
    isPerPointNormalsComputed = false;
    m_Materials = NULL;
    childs.clear();

    m_translation = glm::vec3( 0.0f, 0.0f, 0.0f );
    m_rotation = glm::vec4( 0.0f, 0.0f, 0.0f, 0.0f );
    m_scale = glm::vec3( 1.0f, 1.0f, 1.0f );
    m_scaleOrientation = glm::vec4( 0.0f, 0.0f, 1.0f, 0.0f ); // not used
    m_center = glm::vec3( 0.0f, 0.0f, 0.0f ); // not used
}


S3D_MESH::~S3D_MESH()
{
    for( unsigned int idx = 0; idx < childs.size(); idx++ )
    {
       delete childs[idx];
    }
}

void S3D_MESH::openGL_RenderAllChilds()
{
    glPushMatrix();
    glTranslatef( m_translation.x, m_translation.y, m_translation.z );
    glRotatef( m_rotation[3], m_rotation[0], m_rotation[1], m_rotation[2] );
    glScalef( m_scale.x, m_scale.y, m_scale.z );

    SetOpenGlDefaultMaterial();

    // Render your self
    openGL_Render();

    // Render childs
    for( unsigned int idx = 0; idx < childs.size(); idx++ )
    {
        childs[idx]->openGL_Render();
    }

    SetOpenGlDefaultMaterial();

    glPopMatrix();
}


void S3D_MESH::openGL_Render()
{
    //DBG( printf( "      render\n" ) );

    if( m_Materials )
    {
        m_Materials->SetOpenGLMaterial( 0 );
    }

    if( m_CoordIndex.size() == 0)
    {
        return;
    }

    glPushMatrix();
    glTranslatef( m_translation.x, m_translation.y, m_translation.z );
    glRotatef( m_rotation[3], m_rotation[0], m_rotation[1], m_rotation[2] );
    glScalef( m_scale.x, m_scale.y, m_scale.z );

    std::vector< glm::vec3 > normals;

    calcPointNormalized();
    calcPerFaceNormals();

    if( m_PerVertexNormalsNormalized.size() == 0 )
    {
        if( g_Parm_3D_Visu.IsRealisticMode() && g_Parm_3D_Visu.HightQualityMode() )
        {
            calcPerPointNormals();
        }
    }

    for( unsigned int idx = 0; idx < m_CoordIndex.size(); idx++ )
    {
        if( m_MaterialIndex.size() > 1 )
        {       
            if( m_Materials )
            {
                m_Materials->SetOpenGLMaterial(m_MaterialIndex[idx]);
            }
        }          
        
        
        switch( m_CoordIndex[idx].size() )
        {
            case 3:     glBegin( GL_TRIANGLES );break;
            case 4:     glBegin( GL_QUADS );    break;
            default:    glBegin( GL_POLYGON );  break;
        }
        

        if( m_PerVertexNormalsNormalized.size() > 0 )
        {
            for(unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
            {
                glm::vec3 normal = m_PerVertexNormalsNormalized[m_CoordIndex[idx][ii]];
                glNormal3fv( &normal.x );

                glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                glVertex3fv( &point.x );
            }
        } else if( g_Parm_3D_Visu.IsRealisticMode() && g_Parm_3D_Visu.HightQualityMode() )
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
        } else
        {
            // Flat
            glm::vec3 normal = m_PerFaceNormalsNormalized[idx];

            for( unsigned int ii = 0; ii < m_CoordIndex[idx].size(); ii++ )
            {
                glNormal3fv( &normal.x );

                glm::vec3 point = m_Point[m_CoordIndex[idx][ii]];
                glVertex3fv( &point.x );                        
            }
        }

        glEnd();
    }

    glPopMatrix();
}


void S3D_MESH::calcPointNormalized ()
{
    if( isPointNormalizedComputed == true )
    {
        return;
    }
    isPointNormalizedComputed = true;

    if( m_PerVertexNormalsNormalized.size() > 0 )
    {
        return;
    }

    m_PointNormalized.clear();

    float biggerPoint = 0.0f;
    for( unsigned int i = 0; i< m_Point.size(); i++ )
    {
        if( fabs( m_Point[i].x ) > biggerPoint) biggerPoint = fabs( m_Point[i].x );
        if( fabs( m_Point[i].y ) > biggerPoint) biggerPoint = fabs( m_Point[i].y );
        if( fabs( m_Point[i].z ) > biggerPoint) biggerPoint = fabs( m_Point[i].z );
    }

    biggerPoint = 1.0 / biggerPoint;

    for( unsigned int i= 0; i< m_Point.size(); i++ )
    {
        glm::vec3 p;
        p = m_Point[i] * biggerPoint;
        m_PointNormalized.push_back( p );
    }
}

bool IsClockwise(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
{
    double sum = 0.0;

    sum += (v1.x - v0.x) * (v1.y + v0.y);
    sum += (v2.x - v1.x) * (v2.y + v1.y);
    sum += (v0.x - v2.x) * (v0.y + v2.y);

    return sum > FLT_EPSILON;
}


void S3D_MESH::calcPerFaceNormals ()
{
    if( isPerFaceNormalsComputed == true )
    {
        return;
    }
    isPerFaceNormalsComputed = true;


    if( m_PerVertexNormalsNormalized.size() > 0 )
    {
        return;
    }

    bool haveAlreadyNormals_from_model_file = false;

    if( m_PerFaceNormalsNormalized.size() > 0 )
    {
        haveAlreadyNormals_from_model_file = true;
    }

    m_PerFaceNormalsRaw.clear();
    m_PerFaceSquaredArea.clear();

    for( unsigned int idx = 0; idx < m_CoordIndex.size(); idx++ )
    {

        // User normalized and multiply to get better resolution
        glm::vec3 v0 = m_PointNormalized[m_CoordIndex[idx][0]];
        glm::vec3 v1 = m_PointNormalized[m_CoordIndex[idx][1]];
        glm::vec3 v2 = m_PointNormalized[m_CoordIndex[idx][m_CoordIndex[idx].size() - 1]];

        /*
        // !TODO: improove and check what is best to calc the normal (check what have more resolution)
        glm::vec3 v0 = m_Point[m_CoordIndex[idx][0]];
        glm::vec3 v1 = m_Point[m_CoordIndex[idx][1]];
        glm::vec3 v2 = m_Point[m_CoordIndex[idx][m_CoordIndex[idx].size() - 1]];
        */

        glm::vec3 cross_prod;

        /*
        // This is not working as good as it is expected :/
        if( IsClockwise( v0, v1, v2 ) )
        {
            // CW
            cross_prod = glm::cross( v1 - v2, v0 - v2 );
        } else
        {*/
            // CCW
            cross_prod = glm::cross( v1 - v0, v2 - v0 );
        //}

        float area = glm::dot( cross_prod, cross_prod );

        if( cross_prod[2] < 0.0 )
        {
            area = -area;
        }

        if (area < FLT_EPSILON)
        {
            area = FLT_EPSILON * 2.0f;
        }

        m_PerFaceSquaredArea.push_back( area );

        m_PerFaceNormalsRaw.push_back( cross_prod );

        if( haveAlreadyNormals_from_model_file == false )
        {

            // normalize vertex normal           
            float l = glm::length( cross_prod );

            if( l > FLT_EPSILON ) // avoid division by zero
            {
                cross_prod =  cross_prod / l;
            }
            else
            {
                // Cannot calc normal
                if( (cross_prod.x > cross_prod.y) && (cross_prod.x > cross_prod.z))
                {
                    cross_prod.x = 1.0;    cross_prod.y = 0.0; cross_prod.z = 0.0;
                } else if( (cross_prod.y > cross_prod.x) && (cross_prod.y > cross_prod.z))
                {
                    cross_prod.x = 0.0;    cross_prod.y = 1.0; cross_prod.z = 0.0;
                } else
                {
                    cross_prod.x = 0.0;    cross_prod.y = 1.0; cross_prod.z = 0.0;
                }
            }

            m_PerFaceNormalsNormalized.push_back( cross_prod );
        }
        
    }
}

// http://www.bytehazard.com/code/vertnorm.html
// http://www.emeyex.com/site/tuts/VertexNormals.pdf
void S3D_MESH::calcPerPointNormals ()
{
    if( isPerPointNormalsComputed == true )
    {
        return;
    }
    isPerPointNormalsComputed = true;

    if( m_PerVertexNormalsNormalized.size() > 0 )
    {
        return;
    }

    m_PerFaceVertexNormals.clear();

    // for each face A in mesh
    for( unsigned int each_face_A_idx = 0; each_face_A_idx < m_CoordIndex.size(); each_face_A_idx++ )
    {
        // n = face A facet normal
        std::vector< glm::vec3 > face_A_normals;
        face_A_normals.clear();
        face_A_normals.resize(m_CoordIndex[each_face_A_idx].size());

        // loop through all 3 vertices
        // for each vert in face A
        for( unsigned int each_vert_A_idx = 0; each_vert_A_idx < m_CoordIndex[each_face_A_idx].size(); each_vert_A_idx++ )
        {
            face_A_normals[each_vert_A_idx] = m_PerFaceNormalsRaw[each_face_A_idx] * (m_PerFaceSquaredArea[each_face_A_idx]);

            // for each face A in mesh
            for( unsigned int each_face_B_idx = 0; each_face_B_idx < m_CoordIndex.size(); each_face_B_idx++ )
            {
                //if A != B { // ignore self
                if ( each_face_A_idx != each_face_B_idx)
                {
                    if( (m_CoordIndex[each_face_B_idx][0] == (int)(m_CoordIndex[each_face_A_idx][each_vert_A_idx])) ||
                        (m_CoordIndex[each_face_B_idx][1] == (int)(m_CoordIndex[each_face_A_idx][each_vert_A_idx])) ||
                        (m_CoordIndex[each_face_B_idx][2] == (int)(m_CoordIndex[each_face_A_idx][each_vert_A_idx])) )
                    {
                        glm::vec3 vector_face_A = m_PerFaceNormalsNormalized[each_face_A_idx];
                        glm::vec3 vector_face_B = m_PerFaceNormalsNormalized[each_face_B_idx];

                        float dot_prod = glm::dot(vector_face_A, vector_face_B);
                        if( dot_prod > 0.05f )
                        {
                            face_A_normals[each_vert_A_idx] += m_PerFaceNormalsRaw[each_face_B_idx] * (m_PerFaceSquaredArea[each_face_B_idx] * dot_prod);
                        }
                    }
                }
            }

            // normalize vertex normal           
            float l = glm::length( face_A_normals[each_vert_A_idx] );

            if( l > FLT_EPSILON ) // avoid division by zero
            {
                face_A_normals[each_vert_A_idx] /= l;
            }
            
        }

        m_PerFaceVertexNormals.push_back( face_A_normals );
    }
}
