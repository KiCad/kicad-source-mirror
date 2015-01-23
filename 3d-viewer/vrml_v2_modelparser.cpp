/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 2013 Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file vrml_v2_modelparser.cpp
 */

#include <fctsys.h>
#include <vector>
#include <macros.h>
#include <kicad_string.h>
#include <info3d_visu.h>

#include "3d_struct.h"
#include "modelparsers.h"
#include "vrml_aux.h"

VRML2_MODEL_PARSER::VRML2_MODEL_PARSER( S3D_MASTER* aMaster ) :
    S3D_MODEL_PARSER( aMaster )
{
    m_model = NULL;
}


VRML2_MODEL_PARSER::~VRML2_MODEL_PARSER()
{
    for( unsigned int idx = 0; idx < childs.size(); idx++ )
    {
        delete childs[idx];
    }
}


void VRML2_MODEL_PARSER::Load( const wxString& aFilename, double aVrmlunits_to_3Dunits )
{
    char text[128];

    // DBG( printf( "Load %s\n", GetChars(aFilename) ) );
    m_file = wxFopen( aFilename, wxT( "rt" ) );

    if( m_file == NULL )
    {
        return;
    }

    float vrmlunits_to_3Dunits = aVrmlunits_to_3Dunits;
    glScalef( vrmlunits_to_3Dunits, vrmlunits_to_3Dunits, vrmlunits_to_3Dunits );

    glm::vec3 matScale( GetMaster()->m_MatScale.x, GetMaster()->m_MatScale.y,
            GetMaster()->m_MatScale.z );
    glm::vec3 matRot( GetMaster()->m_MatRotation.x, GetMaster()->m_MatRotation.y,
            GetMaster()->m_MatRotation.z );
    glm::vec3 matPos( GetMaster()->m_MatPosition.x, GetMaster()->m_MatPosition.y,
            GetMaster()->m_MatPosition.z );


#define SCALE_3D_CONV ( (IU_PER_MILS * 1000.0f) / UNITS3D_TO_UNITSPCB )

    glTranslatef( matPos.x * SCALE_3D_CONV, matPos.y * SCALE_3D_CONV, matPos.z * SCALE_3D_CONV );

    glRotatef( -matRot.z, 0.0f, 0.0f, 1.0f );
    glRotatef( -matRot.y, 0.0f, 1.0f, 0.0f );
    glRotatef( -matRot.x, 1.0f, 0.0f, 0.0f );

    glScalef( matScale.x, matScale.y, matScale.z );

    LOCALE_IO toggle;   // Temporary switch the locale to standard C to r/w floats

    childs.clear();

    while( GetNextTag( m_file, text ) )
    {
        if( ( *text == '}' ) || ( *text == ']' ) )
        {
            continue;
        }

        if( strcmp( text, "Transform" ) == 0 )
        {
            m_model = new S3D_MESH();
            childs.push_back( m_model );

            read_Transform();
        }
        else if( strcmp( text, "DEF" ) == 0 )
        {
            m_model = new S3D_MESH();
            childs.push_back( m_model );

            read_DEF();
        }
    }

    fclose( m_file );


    // DBG( printf( "chils size:%lu\n", childs.size() ) );

    if( GetMaster()->IsOpenGlAllowed() )
    {
        for( unsigned int idx = 0; idx < childs.size(); idx++ )
        {
            childs[idx]->openGL_RenderAllChilds();
        }
    }
}


int VRML2_MODEL_PARSER::read_Transform()
{
    char text[128];

    // DBG( printf( "Transform\n" ) );

    while( GetNextTag( m_file, text ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            // DBG( printf( "  } Exit Transform\n" ) );
            break;
        }

        if( strcmp( text, "translation" ) == 0 )
        {
            parseVertex( m_file, m_model->m_translation );
        }
        else if( strcmp( text, "rotation" ) == 0 )
        {
            if( fscanf( m_file, "%f %f %f %f", &m_model->m_rotation[0],
                        &m_model->m_rotation[1],
                        &m_model->m_rotation[2],
                        &m_model->m_rotation[3] ) != 4 )
            {
                // !TODO: log errors
                m_model->m_rotation[0]  = 0.0f;
                m_model->m_rotation[1]  = 0.0f;
                m_model->m_rotation[2]  = 0.0f;
                m_model->m_rotation[3]  = 0.0f;
            }
            else
            {
                m_model->m_rotation[3] = m_model->m_rotation[3] * 180.0f / 3.14f;    // !TODO: use constants or functions
            }
        }
        else if( strcmp( text, "scale" ) == 0 )
        {
            parseVertex( m_file, m_model->m_scale );
        }
        else if( strcmp( text, "scaleOrientation" ) == 0 )
        {
            // this m_scaleOrientation is not implemented, but it will be parsed
            if( fscanf( m_file, "%f %f %f %f", &m_model->m_scaleOrientation[0],
                        &m_model->m_scaleOrientation[1],
                        &m_model->m_scaleOrientation[2],
                        &m_model->m_scaleOrientation[3] ) != 4 )
            {
                // !TODO: log errors
                m_model->m_scaleOrientation[0]  = 0.0f;
                m_model->m_scaleOrientation[1]  = 0.0f;
                m_model->m_scaleOrientation[2]  = 0.0f;
                m_model->m_scaleOrientation[3]  = 0.0f;
            }
        }
        else if( strcmp( text, "center" ) == 0 )
        {
            parseVertex( m_file, m_model->m_center );
        }
        else if( strcmp( text, "children" ) == 0 )
        {
            // skip
        }
        else if( strcmp( text, "Switch" ) == 0 )
        {
            // skip
        }
        else if( strcmp( text, "whichChoice" ) == 0 )
        {
            int dummy;

            if( fscanf( m_file, "%d", &dummy ) != 1 )
            {
                // !TODO: log errors
            }
        }
        else if( strcmp( text, "choice" ) == 0 )
        {
            // skip
        }
        else if( strcmp( text, "Group" ) == 0 )
        {
            // skip
        }
        else if( strcmp( text, "Shape" ) == 0 )
        {
            S3D_MESH* parent = m_model;

            S3D_MESH* new_mesh_model = new S3D_MESH();

            m_model->childs.push_back( new_mesh_model );

            m_model = new_mesh_model;

            read_Shape();

            m_model = parent;
        }
        else if( strcmp( text, "DEF" ) == 0 )
        {
            read_DEF();
        }
        else
        {
            // DBG( printf( "    %s NotImplemented\n", text ) );
            read_NotImplemented( m_file, '}' );
        }
    }

    return 0;
}


int VRML2_MODEL_PARSER::read_DEF()
{
    char text[128];

    GetNextTag( m_file, text );
    // DBG( printf( "DEF %s ", text ) );

    while( GetNextTag( m_file, text ) )
    {
        if( *text == ']' )
        {
            // DBG( printf( "  skiping %c\n", *text) );
            continue;
        }

        if( *text == '}' )
        {
            // DBG( printf( "  } Exit DEF\n") );
            return 0;
        }

        if( strcmp( text, "Transform" ) == 0 )
        {
            return read_Transform();
        }
        else if( strcmp( text, "children" ) == 0 )
        {
            // skip
        }
        else if( strcmp( text, "Switch" ) == 0 )
        {
            // skip
        }
        else if( strcmp( text, "whichChoice" ) == 0 )
        {
            // skip
        }
        else if( strcmp( text, "choice" ) == 0 )
        {
            // skip
        }
        else if( strcmp( text, "Shape" ) == 0 )
        {
            S3D_MESH* parent = m_model;
            S3D_MESH* new_mesh_model = new S3D_MESH();
            m_model->childs.push_back( new_mesh_model );

            m_model = new_mesh_model;
            read_Shape();
            m_model = parent;
        }
    }

    // DBG( printf( "  DEF failed\n" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Shape()
{
    char text[128];

    // DBG( printf( "  Shape\n") );

    while( GetNextTag( m_file, text ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            // DBG( printf( "  } Exit Shape\n") );
            return 0;
        }

        if( strcmp( text, "appearance" ) == 0 )
        {
            // skip
        }
        else if( strcmp( text, "Appearance" ) == 0 )
        {
            read_Appearance();
        }
        else if( strcmp( text, "geometry" ) == 0 )
        {
            // skip
        }
        else if( strcmp( text, "IndexedFaceSet" ) == 0 )
        {
            read_IndexedFaceSet();
        }
        else
        {
            // DBG( printf( "    %s NotImplemented\n", text ) );
            read_NotImplemented( m_file, '}' );
        }
    }

    // DBG( printf( "  Shape failed\n" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Appearance()
{
    char text[128];

    // DBG( printf( "  Appearance\n") );

    while( GetNextTag( m_file, text ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            return 0;
        }

        if( strcmp( text, "material" ) == 0 )
        {
            read_material();
        }
    }

    // DBG( printf( "  Appearance failed\n" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_material()
{
    S3D_MATERIAL* material = NULL;
    char text[128];

    // DBG( printf( "  material ") );

    if( GetNextTag( m_file, text ) )
    {
        if( strcmp( text, "Material" ) == 0 )
        {
            wxString mat_name;
            material = new S3D_MATERIAL( GetMaster(), mat_name );
            GetMaster()->Insert( material );
            m_model->m_Materials = material;

            if( strcmp( text, "Material" ) == 0 )
            {
                return read_Material();
            }
        }
        else if( strcmp( text, "DEF" ) == 0 )
        {
            // DBG( printf( "DEF") );

            if( GetNextTag( m_file, text ) )
            {
                // DBG( printf( "%s", text ) );

                wxString mat_name;
                mat_name = FROM_UTF8( text );

                material = new S3D_MATERIAL( GetMaster(), mat_name );
                GetMaster()->Insert( material );
                m_model->m_Materials = material;

                if( GetNextTag( m_file, text ) )
                {
                    if( strcmp( text, "Material" ) == 0 )
                    {
                        return read_Material();
                    }
                }
            }
        }
        else if( strcmp( text, "USE" ) == 0 )
        {
            // DBG( printf( "USE") );

            if( GetNextTag( m_file, text ) )
            {
                // DBG( printf( "%s\n", text ) );

                wxString mat_name;
                mat_name = FROM_UTF8( text );

                for( material = GetMaster()->m_Materials; material; material = material->Next() )
                {
                    if( material->m_Name == mat_name )
                    {
                        m_model->m_Materials = material;
                        return 0;
                    }
                }

                DBG( printf( "   read_material error: material not found\n" ) );
            }
        }
    }

    // DBG( printf( "  failed material\n" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Material()
{
    char text[128];
    glm::vec3 vertex;

    // DBG( printf( "  Material\n") );

    while( GetNextTag( m_file, text ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            return 0;
        }

        if( strcmp( text, "diffuseColor" ) == 0 )
        {
            // DBG( printf( "  diffuseColor") );
            parseVertex( m_file, vertex );
            // DBG( printf( "\n") );
            m_model->m_Materials->m_DiffuseColor.push_back( vertex );
        }
        else if( strcmp( text, "emissiveColor" ) == 0 )
        {
            // DBG( printf( "  emissiveColor") );
            parseVertex( m_file, vertex );

            // DBG( printf( "\n") );
            if( GetMaster()->m_use_modelfile_emissiveColor == true )
            {
                m_model->m_Materials->m_EmissiveColor.push_back( vertex );
            }
        }
        else if( strcmp( text, "specularColor" ) == 0 )
        {
            // DBG( printf( "  specularColor") );
            parseVertex( m_file, vertex );
            // DBG( printf( "\n") );

            if( GetMaster()->m_use_modelfile_specularColor == true )
            {
                m_model->m_Materials->m_SpecularColor.push_back( vertex );
            }
        }
        else if( strcmp( text, "ambientIntensity" ) == 0 )
        {
            float ambientIntensity;
            parseFloat( m_file, &ambientIntensity );
            // DBG( printf( "  ambientIntensity %f\n", ambientIntensity) );

            if( GetMaster()->m_use_modelfile_ambientIntensity == true )
            {
                m_model->m_Materials->m_AmbientColor.push_back( glm::vec3( ambientIntensity,
                                ambientIntensity, ambientIntensity ) );
            }
        }
        else if( strcmp( text, "transparency" ) == 0 )
        {
            float transparency;
            parseFloat( m_file, &transparency );
            // DBG( printf( "  transparency %f\n", transparency) );

            if( GetMaster()->m_use_modelfile_transparency == true )
            {
                m_model->m_Materials->m_Transparency.push_back( transparency );
            }
        }
        else if( strcmp( text, "shininess" ) == 0 )
        {
            float shininess;
            parseFloat( m_file, &shininess );

            // DBG( printf( "  shininess %f\n", shininess) );
            // VRML value is normalized and openGL expects a value 0 - 128
            if( GetMaster()->m_use_modelfile_shininess == true )
            {
                shininess = shininess * 128.0f;
                m_model->m_Materials->m_Shininess.push_back( shininess );
            }
        }
    }

    // DBG( printf( "  Material failed\n" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_IndexedFaceSet()
{
    char text[128];

    // DBG( printf( "  IndexedFaceSet\n") );

    m_normalPerVertex = false;
    colorPerVertex = false;

    while( GetNextTag( m_file, text ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            // DBG( printf( "  } Exit IndexedFaceSet\n") );
            return 0;
        }

        if( strcmp( text, "normalPerVertex" ) == 0 )
        {
            if( GetNextTag( m_file, text ) )
            {
                if( strcmp( text, "TRUE" ) == 0 )
                {
                    // DBG( printf( "  m_normalPerVertex TRUE\n") );
                    m_normalPerVertex = true;
                }
            }
        }
        else if( strcmp( text, "colorPerVertex" ) == 0 )
        {
            GetNextTag( m_file, text );

            if( strcmp( text, "TRUE" ) )
            {
                // DBG( printf( "  colorPerVertex = true\n") );
                colorPerVertex = true;
            }
            else
            {
                colorPerVertex = false;
            }
        }
        else if( strcmp( text, "Coordinate" ) == 0 )
        {
            read_Coordinate();
        }
        else if( strcmp( text, "Normal" ) == 0 )
        {
            read_Normal();
        }
        else if( strcmp( text, "normalIndex" ) == 0 )
        {
            read_NormalIndex();
        }
        else if( strcmp( text, "Color" ) == 0 )
        {
            read_Color();
        }
        else if( strcmp( text, "coordIndex" ) == 0 )
        {
            read_coordIndex();
        }
        else if( strcmp( text, "colorIndex" ) == 0 )
        {
            read_colorIndex();
        }
    }

    // DBG( printf( "  IndexedFaceSet failed %s\n", text ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_colorIndex()
{
    // DBG( printf( "    read_colorIndex\n" ) );

    m_model->m_MaterialIndex.clear();

    if( colorPerVertex == true )
    {
        int index;
        int first_index;

        while( fscanf( m_file, "%d, ", &index ) )
        {
            if( index == -1 )
            {
                // it only implemented color per face, so it will store as the first in the list
                m_model->m_MaterialIndex.push_back( first_index );
            }
            else
            {
                first_index = index;
            }
        }
    }
    else
    {
        int index;

        while( fscanf( m_file, "%d,", &index ) )
        {
            m_model->m_MaterialIndex.push_back( index );
        }
    }

    // DBG( printf( "    m_MaterialIndex.size: %ld\n", m_model->m_MaterialIndex.size() ) );

    return 0;
}


int VRML2_MODEL_PARSER::read_NormalIndex()
{
    // DBG( printf( "    read_NormalIndex\n" ) );

    m_model->m_NormalIndex.clear();

    glm::ivec3 coord;

    int dummy;    // should be -1

    std::vector<int> coord_list;
    coord_list.clear();

    while( fscanf( m_file, "%d, ", &dummy ) == 1 )
    {
        if( dummy == -1 )
        {
            m_model->m_NormalIndex.push_back( coord_list );
            // DBG( printf( " size: %lu ", coord_list.size()) );
            coord_list.clear();
        }
        else
        {
            coord_list.push_back( dummy );
            // DBG( printf( "%d ", dummy) );
        }
    }

    // DBG( printf( "    m_NormalIndex.size: %ld\n", m_model->m_NormalIndex.size() ) );

    return 0;
}


int VRML2_MODEL_PARSER::read_coordIndex()
{
    // DBG( printf( "    read_coordIndex\n" ) );

    m_model->m_CoordIndex.clear();

    glm::ivec3 coord;

    int dummy;    // should be -1

    std::vector<int> coord_list;
    coord_list.clear();

    while( fscanf( m_file, "%d, ", &dummy ) == 1 )
    {
        if( dummy == -1 )
        {
            m_model->m_CoordIndex.push_back( coord_list );
            // DBG( printf( " size: %lu ", coord_list.size()) );
            coord_list.clear();
        }
        else
        {
            coord_list.push_back( dummy );
            // DBG( printf( "%d ", dummy) );
        }
    }

    // DBG( printf( "    m_CoordIndex.size: %ld\n", m_model->m_CoordIndex.size() ) );

    return 0;
}


int VRML2_MODEL_PARSER::read_Color()
{
    char text[128];

    // DBG( printf( "  read_Color\n") );

    while( GetNextTag( m_file, text ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            // DBG( printf( "    m_DiffuseColor.size: %ld\n", m_model->m_Materials->m_DiffuseColor.size() ) );
            return 0;
        }

        if( strcmp( text, "color" ) == 0 )
        {
            parseVertexList( m_file, m_model->m_Materials->m_DiffuseColor );
        }
    }

    // DBG( printf( "  read_Color failed\n") );
    return -1;
}


int VRML2_MODEL_PARSER::read_Normal()
{
    char text[128];

    // DBG( printf( "  Normal\n") );

    while( GetNextTag( m_file, text ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            // DBG( printf( "    m_PerFaceNormalsNormalized.size: %lu\n", m_model->m_PerFaceNormalsNormalized.size() ) );
            return 0;
        }

        if( strcmp( text, "vector" ) == 0 )
        {
            if( m_normalPerVertex == false )
            {
                parseVertexList( m_file, m_model->m_PerFaceNormalsNormalized );
            }
            else
            {
                parseVertexList( m_file, m_model->m_PerVertexNormalsNormalized );

                // DBG( printf( "    m_PerVertexNormalsNormalized.size: %lu\n", m_model->m_PerVertexNormalsNormalized.size() ) );
            }
        }
    }

    return -1;
}


int VRML2_MODEL_PARSER::read_Coordinate()
{
    char text[128];

    // DBG( printf( "  Coordinate\n") );

    while( GetNextTag( m_file, text ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            // DBG( printf( "    m_Point.size: %lu\n", m_model->m_Point.size() ) );
            return 0;
        }

        if( strcmp( text, "point" ) == 0 )
        {
            parseVertexList( m_file, m_model->m_Point );
        }
    }

    return -1;
}
