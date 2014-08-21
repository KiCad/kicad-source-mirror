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
#include <info3d_visu.h>

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
    glm::vec4 specular( 0.0, 0.0, 0.0, 1.0 );
    glm::vec4 emissive( 0.0, 0.0, 0.0, 1.0 );
    glm::vec4 diffuse( 0.0, 0.0, 0.0, 1.0 );
    GLint shininess_value = 0;

    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
    glMateriali ( GL_FRONT_AND_BACK, GL_SHININESS, shininess_value );
    glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, &emissive.x );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.x );
    glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,  &ambient.x );
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  &diffuse.x );

}


void S3D_MATERIAL::SetOpenGLMaterial( unsigned int aMaterialIndex, bool aUseMaterial )
{
    S3D_MASTER * s3dParent = (S3D_MASTER *) GetParent();

    if( ! s3dParent->IsOpenGlAllowed() )
        return;

    if( aUseMaterial )
    {
        float transparency_value = 0.0f;
        if( m_Transparency.size() > aMaterialIndex )
        {
            transparency_value = m_Transparency[aMaterialIndex];
            s3dParent->SetLastTransparency( transparency_value );
        }

        if( m_DiffuseColor.size() > aMaterialIndex )
        {
            glm::vec3 color = m_DiffuseColor[aMaterialIndex];

            if( m_AmbientColor.size() == 0 )
            {
                glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
            }

            glColor4f( color.x, color.y, color.z, 1.0 - transparency_value );
        }

        if( m_Shininess.size() > aMaterialIndex )
        {
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m_Shininess[aMaterialIndex] );
        }

        // emissive
        if( m_EmissiveColor.size() > aMaterialIndex )
        {
            glm::vec4 emissive;
            emissive[0] = m_EmissiveColor[aMaterialIndex].x;
            emissive[1] = m_EmissiveColor[aMaterialIndex].y;
            emissive[2] = m_EmissiveColor[aMaterialIndex].z;
            emissive[3] = 1.0f;
            glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, &emissive.x );
        }

        // specular
        if( m_SpecularColor.size() > aMaterialIndex )
        {
            glm::vec4 specular;
            specular[0] = m_SpecularColor[aMaterialIndex].x;
            specular[1] = m_SpecularColor[aMaterialIndex].y;
            specular[2] = m_SpecularColor[aMaterialIndex].z;
            specular[3] = 1.0f;
            glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.x );
        }

        // ambient
        if( m_AmbientColor.size() > aMaterialIndex )
        {
            glm::vec4 ambient;
            ambient[0] = m_AmbientColor[aMaterialIndex].x;
            ambient[1] = m_AmbientColor[aMaterialIndex].y;
            ambient[2] = m_AmbientColor[aMaterialIndex].z;
            ambient[3] = 1.0f;
            glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,  &ambient.x );
        }
    }
    else
    {
        if( m_DiffuseColor.size() > aMaterialIndex )
        {
            glm::vec3 color = m_DiffuseColor[aMaterialIndex];
            glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
            glColor4f( color.x, color.y, color.z, 1.0 );
        }
    }
}

