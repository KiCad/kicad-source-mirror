/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file 3d_class.cpp
 */

#include <fctsys.h>
#include <3d_struct.h>
#include <3d_material.h>

#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/glu.h>
#  else
#    include <glu.h>
#  endif
#else
#  include <GL/glu.h>
#endif

S3D_MATERIAL::S3D_MATERIAL( S3D_MASTER* father, const wxString& name ) :
    EDA_ITEM( father, NOT_USED )
{
    m_Name = name;
    m_AmbientColor.clear();
    m_DiffuseColor.clear();
    m_EmissiveColor.clear();
    m_SpecularColor.clear();
    m_Shininess.clear();
    m_Transparency.clear();
}


void SetOpenGlDefaultMaterial()
{
    glm::vec4 ambient( 0.2, 0.2, 0.2, 1.0 );
    glm::vec4 specular( 0.1, 0.1, 0.1, 1.0 );
    glm::vec4 emissive( 0.1, 0.1, 0.1, 1.0 );
    GLint shininess_value = 100;

    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
    //glColor4f( 1.0, 1.0, 1.0, 1.0 );
    glMateriali ( GL_FRONT_AND_BACK, GL_SHININESS, shininess_value );
    glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, &emissive.x );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.x );
    glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,  &ambient.x );

}


void S3D_MATERIAL::SetOpenGLMaterial( unsigned int materialIndex )
{
    S3D_MASTER * s3dParent = (S3D_MASTER *) GetParent();
    
    if( ! s3dParent->IsOpenGlAllowed() )
        return;

    float transparency_value = 0.0f;
    if( m_Transparency.size() > materialIndex )
    {
        transparency_value = m_Transparency[materialIndex];
        s3dParent->SetLastTransparency( transparency_value );
    }

    if( m_DiffuseColor.size() > materialIndex )            
    {
        glm::vec3 color = m_DiffuseColor[materialIndex];

        if( m_AmbientColor.size() == 0 )
        {
            glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
        }
        
        glColor4f( color.x, color.y, color.z, 1.0 - transparency_value );
    }

    if( m_Shininess.size() > materialIndex )
    {
        glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, m_Shininess[materialIndex] );
    }

    // emissive
    if( m_EmissiveColor.size() > materialIndex )
    {
        glm::vec4 emissive;
        emissive[0] = m_EmissiveColor[materialIndex].x;
        emissive[1] = m_EmissiveColor[materialIndex].y;
        emissive[2] = m_EmissiveColor[materialIndex].z;
        emissive[3] = 1.0f;
        glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, &emissive.x );
    }

    // specular
    if( m_SpecularColor.size() > materialIndex )
    {
        glm::vec4 specular;
        specular[0] = m_SpecularColor[materialIndex].x;
        specular[1] = m_SpecularColor[materialIndex].y;
        specular[2] = m_SpecularColor[materialIndex].z;
        specular[3] = 1.0f;
        glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.x );
    }

    // ambient
    if( m_AmbientColor.size() > materialIndex )
    {
        glm::vec4 ambient;
        ambient[0] = m_AmbientColor[materialIndex].x;
        ambient[1] = m_AmbientColor[materialIndex].y;
        ambient[2] = m_AmbientColor[materialIndex].z;
        ambient[3] = 1.0f;
        glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,  &ambient.x );
    }    
}

