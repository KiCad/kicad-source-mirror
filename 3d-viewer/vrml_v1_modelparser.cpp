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
 * @file vrml_v1_modelparser.cpp
 */

#include <fctsys.h>
#include <vector>
#include <macros.h>
#include <kicad_string.h>
#include <info3d_visu.h>

#include "3d_struct.h"
#include "modelparsers.h"
#include "vrml_aux.h"

VRML1_MODEL_PARSER::VRML1_MODEL_PARSER( S3D_MASTER* aMaster ) :
    S3D_MODEL_PARSER( aMaster )
{
    m_model = NULL;
    m_file  = NULL;
}


VRML1_MODEL_PARSER::~VRML1_MODEL_PARSER()
{
    for( unsigned int idx = 0; idx < childs.size(); idx++ )
    {
        delete childs[idx];
    }
}


void VRML1_MODEL_PARSER::Load( const wxString aFilename )
{
    char text[128];

    // DBG( printf( "Load %s\n", static_cast<const char*>(aFilename.mb_str()) ) );
    m_file = wxFopen( aFilename, wxT( "rt" ) );

    if( m_file == NULL )
    {
        return;
    }

    glShadeModel( GL_SMOOTH );
    glEnable( GL_NORMALIZE );

    float vrmlunits_to_3Dunits = g_Parm_3D_Visu.m_BiuTo3Dunits * UNITS3D_TO_UNITSPCB;
    glScalef( vrmlunits_to_3Dunits, vrmlunits_to_3Dunits, vrmlunits_to_3Dunits );

    glm::vec3 matScale( GetMaster()->m_MatScale.x, GetMaster()->m_MatScale.y,
            GetMaster()->m_MatScale.z );
    glm::vec3 matRot( GetMaster()->m_MatRotation.x, GetMaster()->m_MatRotation.y,
            GetMaster()->m_MatRotation.z );
    glm::vec3 matPos( GetMaster()->m_MatPosition.x, GetMaster()->m_MatPosition.y,
            GetMaster()->m_MatPosition.z );


#define SCALE_3D_CONV ( (IU_PER_MILS * 1000.0f) / UNITS3D_TO_UNITSPCB )

    // glPushMatrix();
    glTranslatef( matPos.x * SCALE_3D_CONV, matPos.y * SCALE_3D_CONV, matPos.z * SCALE_3D_CONV );

    glRotatef( -matRot.z, 0.0f, 0.0f, 1.0f );
    glRotatef( -matRot.y, 0.0f, 1.0f, 0.0f );
    glRotatef( -matRot.x, 1.0f, 0.0f, 0.0f );

    glScalef( matScale.x, matScale.y, matScale.z );

    // Switch the locale to standard C (needed to print floating point numbers like 1.3)
    SetLocaleTo_C_standard();

    childs.clear();

    while( GetNextTag( m_file, text ) )
    {
        if( ( text == NULL ) || ( *text == '}' ) || ( *text == ']' ) )
        {
            continue;
        }

        if( strcmp( text, "Separator" ) == 0 )
        {
            m_model = new S3D_MESH();
            childs.push_back( m_model );
            read_separator();
        }
    }

    fclose( m_file );
    SetLocaleTo_Default();       // revert to the current locale


    // DBG( printf( "chils size:%lu\n", childs.size() ) );

    if( GetMaster()->IsOpenGlAllowed() )
    {
        for( unsigned int idx = 0; idx < childs.size(); idx++ )
        {
            childs[idx]->openGL_RenderAllChilds();
        }
    }
}


int VRML1_MODEL_PARSER::read_separator()
{
    char text[128];

    // DBG( printf( "Separator\n" ) );

    while( GetNextTag( m_file, text ) )
    {
        if( strcmp( text, "Material" ) == 0 )
        {
            readMaterial();
        }
        else if( strcmp( text, "Coordinate3" ) == 0 )
        {
            readCoordinate3();
        }
        else if( strcmp( text, "IndexedFaceSet" ) == 0 )
        {
            readIndexedFaceSet();
        }
        else if( strcmp( text, "Separator" ) == 0 )
        {
            S3D_MESH* parent = m_model;

            S3D_MESH* new_mesh_model = new S3D_MESH();

            m_model->childs.push_back( new_mesh_model );

            m_model = new_mesh_model;

            // recursive
            read_separator();

            m_model = parent;
        }
        else if( ( *text != '}' ) )
        {
            // DBG( printf( "read_NotImplemented %s\n", text ) );
            read_NotImplemented( m_file, '}' );
        }
        else
        {
            break;
        }
    }

    return 0;
}


int VRML1_MODEL_PARSER::readMaterial()
{
    char text[128];
    S3D_MATERIAL* material = NULL;

    // DBG( printf( "  readMaterial\n" ) );

    wxString mat_name;

    material = new S3D_MATERIAL( GetMaster(), mat_name );

    GetMaster()->Insert( material );

    m_model->m_Materials = material;

    while( GetNextTag( m_file, text ) )
    {
        if( ( text == NULL ) || ( *text == ']' ) )
        {
            continue;
        }

        if( ( *text == '}' ) )
        {
            return 0;
        }

        if( strcmp( text, "ambientColor" ) == 0 )
        {
            readMaterial_ambientColor();
        }
        else if( strcmp( text, "diffuseColor" ) == 0 )
        {
            readMaterial_diffuseColor();
        }
        else if( strcmp( text, "emissiveColor" ) == 0 )
        {
            readMaterial_emissiveColor();
        }
        else if( strcmp( text, "specularColor" ) == 0 )
        {
            readMaterial_specularColor();
        }
        else if( strcmp( text, "shininess" ) == 0 )
        {
            readMaterial_shininess();
        }
        else if( strcmp( text, "transparency" ) == 0 )
        {
            readMaterial_transparency();
        }
    }

    return -1;
}


int VRML1_MODEL_PARSER::readCoordinate3()
{
    char text[128];

    // DBG( printf( "  readCoordinate3\n" ) );

    while( GetNextTag( m_file, text ) )
    {
        if( ( text == NULL ) || ( *text == ']' ) )
        {
            continue;
        }

        if( ( *text == '}' ) )
        {
            return 0;
        }

        if( strcmp( text, "point" ) == 0 )
        {
            readCoordinate3_point();
        }
    }

    return -1;
}


int VRML1_MODEL_PARSER::readIndexedFaceSet()
{
    char text[128];

    // DBG( printf( "  readIndexedFaceSet\n" ) );

    while( GetNextTag( m_file, text ) )
    {
        if( ( text == NULL ) || ( *text == ']' ) )
        {
            continue;
        }

        if( ( *text == '}' ) )
        {
            return 0;
        }

        if( strcmp( text, "coordIndex" ) == 0 )
        {
            readIndexedFaceSet_coordIndex();
        }
        else if( strcmp( text, "materialIndex" ) == 0 )
        {
            readIndexedFaceSet_materialIndex();
        }
    }

    return -1;
}


int VRML1_MODEL_PARSER::readMaterial_ambientColor()
{
    // DBG( printf( "    readMaterial_ambientColor\n" ) );

    return parseVertexList( m_file, m_model->m_Materials->m_AmbientColor );
}


int VRML1_MODEL_PARSER::readMaterial_diffuseColor()
{
    // DBG( printf( "    readMaterial_diffuseColor\n" ) );

    return parseVertexList( m_file, m_model->m_Materials->m_DiffuseColor );
}


int VRML1_MODEL_PARSER::readMaterial_emissiveColor()
{
    // DBG( printf( "    readMaterial_emissiveColor\n" ) );

    int ret = parseVertexList( m_file, m_model->m_Materials->m_EmissiveColor );

    if( GetMaster()->m_use_modelfile_emissiveColor == false )
    {
        m_model->m_Materials->m_EmissiveColor.clear();
    }

    return ret;
}


int VRML1_MODEL_PARSER::readMaterial_specularColor()
{
    // DBG( printf( "    readMaterial_specularColor\n" ) );

    int ret = parseVertexList( m_file, m_model->m_Materials->m_SpecularColor );

    if( GetMaster()->m_use_modelfile_specularColor == false )
    {
        m_model->m_Materials->m_SpecularColor.clear();
    }

    return ret;
}


int VRML1_MODEL_PARSER::readMaterial_shininess()
{
    // DBG( printf( "    readMaterial_shininess\n" ) );

    m_model->m_Materials->m_Shininess.clear();

    float shininess_value;

    while( fscanf( m_file, "%f,", &shininess_value ) )
    {
        // VRML value is normalized and openGL expects a value 0 - 128
        shininess_value = shininess_value * 128.0f;
        m_model->m_Materials->m_Shininess.push_back( shininess_value );
    }

    if( GetMaster()->m_use_modelfile_shininess == false )
    {
        m_model->m_Materials->m_Shininess.clear();
    }

    // DBG( printf( "    m_Shininess.size: %ld\n", m_model->m_Materials->m_Shininess.size() ) );

    return 0;
}


int VRML1_MODEL_PARSER::readMaterial_transparency()
{
    // DBG( printf( "    readMaterial_transparency\n" ) );

    m_model->m_Materials->m_Transparency.clear();

    float tmp;

    while( fscanf( m_file, "%f,", &tmp ) )
    {
        m_model->m_Materials->m_Transparency.push_back( tmp );
    }

    if( GetMaster()->m_use_modelfile_transparency == false )
    {
        m_model->m_Materials->m_Transparency.clear();
    }

    // DBG( printf( "    m_Transparency.size: %ld\n", m_model->m_Materials->m_Transparency.size() ) );

    return 0;
}


int VRML1_MODEL_PARSER::readCoordinate3_point()
{
    // DBG( printf( "    readCoordinate3_point\n" ) );

    if( parseVertexList( m_file, m_model->m_Point ) == 0 )
    {
        return 0;
    }

    return -1;
}


int VRML1_MODEL_PARSER::readIndexedFaceSet_coordIndex()
{
    // DBG( printf( "    readIndexedFaceSet_coordIndex\n" ) );

    m_model->m_CoordIndex.clear();

    glm::ivec3 coord;

    int dummy;    // should be -1

    while( fscanf( m_file, "%d,%d,%d,%d,", &coord[0], &coord[1], &coord[2], &dummy ) )
    {
        std::vector<int> coord_list;

        coord_list.resize( 3 );
        coord_list[0] = coord[0];
        coord_list[1] = coord[1];
        coord_list[2] = coord[2];

        if( (coord[0] == coord[1])
            || (coord[0] == coord[2])
            || (coord[2] == coord[1]) )
        {
            // DBG( printf( "    invalid coordIndex at index %lu (%d, %d, %d, %d)\n", m_model->m_CoordIndex.size()+1,coord[0], coord[1], coord[2], dummy ) );
        }

        if( dummy != -1 )
        {
            // DBG( printf( "    Error at index %lu, -1 Expected, got %d\n", m_model->m_CoordIndex.size()+1, dummy  ) );
        }

        m_model->m_CoordIndex.push_back( coord_list );
    }

    // DBG( printf( "    m_CoordIndex.size: %ld\n", m_model->m_CoordIndex.size() ) );

    return 0;
}


int VRML1_MODEL_PARSER::readIndexedFaceSet_materialIndex()
{
    // DBG( printf( "    readIndexedFaceSet_materialIndex\n" ) );

    m_model->m_MaterialIndex.clear();

    int index;

    while( fscanf( m_file, "%d,", &index ) )
    {
        m_model->m_MaterialIndex.push_back( index );
    }

    // DBG( printf( "    m_MaterialIndex.size: %ld\n", m_model->m_MaterialIndex.size() ) );

    return 0;
}
