/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 2013 Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
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

#define BUFLINE_SIZE 32

/**
 * Trace mask used to enable or disable the trace output of the VRML V2 parser code.
 * The debug output can be turned on by setting the WXTRACE environment variable to
 * "KI_TRACE_VRML_V2_PARSER".  See the wxWidgets documentation on wxLogTrace for
 * more information.
 */
static const wxChar* traceVrmlV2Parser = wxT( "KI_TRACE_VRML_V2_PARSER" );


VRML2_MODEL_PARSER::VRML2_MODEL_PARSER( S3D_MODEL_PARSER* aModelParser )
{
    m_ModelParser = aModelParser;
    m_Master = m_ModelParser->GetMaster();
    m_model = NULL;
    m_file = NULL;
    m_normalPerVertex = true;
    colorPerVertex = true;
}


VRML2_MODEL_PARSER::~VRML2_MODEL_PARSER()
{
}


bool VRML2_MODEL_PARSER::Load( const wxString& aFilename )
{
    char text[BUFLINE_SIZE];

    wxLogTrace( traceVrmlV2Parser, wxT( "Loading: %s" ), GetChars( aFilename ) );

    m_file = wxFopen( aFilename, wxT( "rt" ) );

    if( m_file == NULL )
    {
        wxLogTrace( traceVrmlV2Parser, wxT( "Failed to open file: %s" ), GetChars( aFilename ) );
        return false;
    }

    // Switch the locale to standard C (needed to print floating point numbers)
    LOCALE_IO toggle;

    m_ModelParser->childs.clear();

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( ( *text == '}' ) || ( *text == ']' ) )
        {
            continue;
        }

        if( strcmp( text, "Transform" ) == 0 )
        {
            m_model = new S3D_MESH();
            m_ModelParser->childs.push_back( m_model );

            if( read_Transform() == 0)
            {
                //wxLogTrace( traceVrmlV2Parser, wxT( "  m_Point.size: %u" ), (unsigned int)m_model->m_Point.size() );
                //wxLogTrace( traceVrmlV2Parser, wxT( "  m_CoordIndex.size: %u" ), (unsigned int)m_model->m_CoordIndex.size() );
            }
        }
        else if( strcmp( text, "DEF" ) == 0 )
        {
            m_model = new S3D_MESH();
            m_ModelParser->childs.push_back( m_model );

            if( read_DEF() == 0 )
            {
                //wxLogTrace( traceVrmlV2Parser, wxT( "  m_Point.size: %u" ), (unsigned int)m_model->m_Point.size() );
                //wxLogTrace( traceVrmlV2Parser, wxT( "  m_CoordIndex.size: %u" ), (unsigned int)m_model->m_CoordIndex.size() );
            }
        }
        else if( strcmp( text, "Shape" ) == 0 )
        {
            //wxLogTrace( traceVrmlV2Parser, wxT( "    Shape" ) );

            m_model = new S3D_MESH();
            m_ModelParser->childs.push_back( m_model );

            if( read_Shape() == 0 )
            {
                //wxLogTrace( traceVrmlV2Parser, wxT( "  m_Point.size: %u" ), (unsigned int)m_model->m_Point.size() );
                //wxLogTrace( traceVrmlV2Parser, wxT( "  m_CoordIndex.size: %u" ), (unsigned int)m_model->m_CoordIndex.size() );
            }
        }
    }

    fclose( m_file );

    return true;
}


int VRML2_MODEL_PARSER::read_Transform()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "  read_Transform" ) );

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            break;
        }

        if( strcmp( text, "translation" ) == 0 )
        {
            parseVertex( m_file, m_model->m_translation );
            
            //wxLogTrace( traceVrmlV2Parser, wxT( "    translation (%f,%f,%f)" ),
            //                               m_model->m_translation.x,
            //                               m_model->m_translation.y,
            //                               m_model->m_translation.z );
        }
        else if( strcmp( text, "rotation" ) == 0 )
        {
            if( fscanf( m_file, "%f %f %f %f", &m_model->m_rotation[0],
                        &m_model->m_rotation[1],
                        &m_model->m_rotation[2],
                        &m_model->m_rotation[3] ) != 4 )
            {
                m_model->m_rotation[0]  = 0.0f;
                m_model->m_rotation[1]  = 0.0f;
                m_model->m_rotation[2]  = 0.0f;
                m_model->m_rotation[3]  = 0.0f;

                wxLogTrace( traceVrmlV2Parser, wxT( "    rotation failed, setting to zeros" ) );
            }
            else
            {
                m_model->m_rotation[3] = m_model->m_rotation[3] * 180.0f / 3.14f;    // !TODO: use constants or functions
            }

            //wxLogTrace( traceVrmlV2Parser, wxT( "    rotation (%f,%f,%f,%f)" ),
            //                   m_model->m_rotation[0],
            //                   m_model->m_rotation[1],
            //                   m_model->m_rotation[2],
            //                   m_model->m_rotation[3] );
        }
        else if( strcmp( text, "scale" ) == 0 )
        {
            parseVertex( m_file, m_model->m_scale );

            //wxLogTrace( traceVrmlV2Parser, wxT( "    scale (%f,%f,%f)" ), m_model->m_scale.x, m_model->m_scale.y, m_model->m_scale.z );
        }
        else if( strcmp( text, "scaleOrientation" ) == 0 )
        {
            // this m_scaleOrientation is not implemented, but it will be parsed
            if( fscanf( m_file, "%f %f %f %f", &m_model->m_scaleOrientation[0],
                        &m_model->m_scaleOrientation[1],
                        &m_model->m_scaleOrientation[2],
                        &m_model->m_scaleOrientation[3] ) != 4 )
            {
                m_model->m_scaleOrientation[0]  = 0.0f;
                m_model->m_scaleOrientation[1]  = 0.0f;
                m_model->m_scaleOrientation[2]  = 0.0f;
                m_model->m_scaleOrientation[3]  = 0.0f;

                wxLogTrace( traceVrmlV2Parser, wxT( "    scaleOrientation failed, setting to zeros" ) );
            }

            //wxLogTrace( traceVrmlV2Parser, wxT( "    scaleOrientation (%f,%f,%f,%f)" ),
            //                   m_model->m_scaleOrientation[0],
            //                   m_model->m_scaleOrientation[1],
            //                   m_model->m_scaleOrientation[2],
            //                   m_model->m_scaleOrientation[3] );
        }
        else if( strcmp( text, "center" ) == 0 )
        {
            parseVertex( m_file, m_model->m_center );

            //wxLogTrace( traceVrmlV2Parser, wxT( "    center (%f,%f,%f)" ), m_model->m_center.x, m_model->m_center.y, m_model->m_center.z );
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
            //wxLogTrace( traceVrmlV2Parser, wxT( "    Shape" ) );

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
            wxLogTrace( traceVrmlV2Parser, wxT( "    %s NotImplemented" ), text );
            read_NotImplemented( m_file, '}' );
        }
    }

    return 0;
}


int VRML2_MODEL_PARSER::read_DEF_Coordinate()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_DEF_Coordinate" ) );

    char text[BUFLINE_SIZE];

    // Get the name of the definition.
    if( !GetNextTag( m_file, text, sizeof(text) ) )
        return -1;

    std::string coordinateName = text;

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
            continue;

        if( *text == '}' )
            return 0;

        if( strcmp( text, "Coordinate" ) == 0 )
        {
            int retVal = read_CoordinateDef();

            if( retVal == 0 )
                m_defCoordinateMap.insert( std::make_pair( coordinateName, m_model->m_Point ) );

            return retVal;
        }
    }

    wxLogTrace( traceVrmlV2Parser, wxT( "  read_DEF_Coordinate failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_DEF()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_DEF" ) );

    char text[BUFLINE_SIZE];
    char tagName[BUFLINE_SIZE];

    if( !GetNextTag( m_file, tagName, sizeof(tagName) ) )
    {
        wxLogTrace( traceVrmlV2Parser, wxT( "  DEF failed GetNextTag first" ) );
        return -1;
    }

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            wxLogTrace( traceVrmlV2Parser, wxT( "  skipping %c" ), *text );
            continue;
        }

        if( *text == '}' )
        {
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
        else
        {
            wxLogTrace( traceVrmlV2Parser, wxT( "    DEF %s %s NotImplemented, skipping." ), tagName, text );
            read_NotImplemented( m_file, '}' );

            return -1;
        }
    }

    wxLogTrace( traceVrmlV2Parser, wxT( "  DEF failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_USE()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_USE" ) );

    char text[BUFLINE_SIZE];

    // Get the name of the definition.
    if( !GetNextTag( m_file, text, sizeof(text) ) )
        return -1;

    std::string coordinateName = text;

    // Look for it in our coordinate map.
    VRML2_COORDINATE_MAP::iterator coordinate;
    coordinate = m_defCoordinateMap.find( coordinateName );

    // Not previously defined.
    if( coordinate == m_defCoordinateMap.end() )
    {
        wxLogTrace( traceVrmlV2Parser, wxT( "USE: coordinate %s not previously defined "
                                            "in a DEF section." ), text );
        return -1;
    }

    m_model->m_Point = coordinate->second;
    return 0;
}


int VRML2_MODEL_PARSER::read_Shape()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_Shape" ) );

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            return 0;
        }

        if( strcmp( text, "appearance" ) == 0 )
        {
            //wxLogTrace( traceVrmlV2Parser, wxT( "     \"appearance\" key word not supported." ) );
            // skip
        }
        else if( strcmp( text, "Appearance" ) == 0 )
        {
            read_Appearance();
        }
        else if( strcmp( text, "geometry" ) == 0 )
        {
            //wxLogTrace( traceVrmlV2Parser, wxT( "     \"geometry\" key word not supported." ) );
            // skip
        }
        else if( strcmp( text, "IndexedFaceSet" ) == 0 )
        {
            read_IndexedFaceSet();
        }
        else if( strcmp( text, "IndexedLineSet" ) == 0 )
        {
            read_IndexedLineSet();
        }
        else
        {
            wxLogTrace( traceVrmlV2Parser, wxT( "    %s NotImplemented" ), text );
            read_NotImplemented( m_file, '}' );
        }
    }

    wxLogTrace( traceVrmlV2Parser, wxT( "  Shape failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Appearance()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_Appearance" ) );

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
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

    wxLogTrace( traceVrmlV2Parser, wxT( "  Appearance failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_material()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_material" ) );

    S3D_MATERIAL* material = NULL;
    char text[BUFLINE_SIZE];

    if( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( strcmp( text, "Material" ) == 0 )
        {
            wxString mat_name;
            material = new S3D_MATERIAL( m_Master, mat_name );
            m_Master->Insert( material );
            m_model->m_Materials = material;

            if( strcmp( text, "Material" ) == 0 )
            {
                return read_Material();
            }
        }
        else if( strcmp( text, "DEF" ) == 0 )
        {
            if( GetNextTag( m_file, text, sizeof(text) ) )
            {
                wxString mat_name;
                mat_name = FROM_UTF8( text );

                material = new S3D_MATERIAL( m_Master, mat_name );
                m_Master->Insert( material );
                m_model->m_Materials = material;

                if( GetNextTag( m_file, text, sizeof(text) ) )
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
            if( GetNextTag( m_file, text, sizeof(text) ) )
            {
                wxString mat_name;
                mat_name = FROM_UTF8( text );

                for( material = m_Master->m_Materials; material; material = material->Next() )
                {
                    if( material->m_Name == mat_name )
                    {
                        m_model->m_Materials = material;
                        return 0;
                    }
                }

                wxLogTrace( traceVrmlV2Parser, wxT( "   read_material error: material not found" ) );
            }
        }
    }

    wxLogTrace( traceVrmlV2Parser, wxT( "  failed material" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Material()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_Material" ) );

    char text[BUFLINE_SIZE];
    glm::vec3 vertex;

    while( GetNextTag( m_file, text, sizeof(text) ) )
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
            parseVertex( m_file, vertex );
            m_model->m_Materials->m_DiffuseColor.push_back( vertex );
        }
        else if( strcmp( text, "emissiveColor" ) == 0 )
        {
            parseVertex( m_file, vertex );

            if( m_Master->m_use_modelfile_emissiveColor == true )
            {
                m_model->m_Materials->m_EmissiveColor.push_back( vertex );
            }
        }
        else if( strcmp( text, "specularColor" ) == 0 )
        {
            parseVertex( m_file, vertex );

            if( m_Master->m_use_modelfile_specularColor == true )
            {
                m_model->m_Materials->m_SpecularColor.push_back( vertex );
            }
        }
        else if( strcmp( text, "ambientIntensity" ) == 0 )
        {
            float ambientIntensity;
            parseFloat( m_file, &ambientIntensity );

            if( m_Master->m_use_modelfile_ambientIntensity == true )
            {
                m_model->m_Materials->m_AmbientColor.push_back( glm::vec3( ambientIntensity,
                                ambientIntensity, ambientIntensity ) );
            }
        }
        else if( strcmp( text, "transparency" ) == 0 )
        {
            float transparency;
            parseFloat( m_file, &transparency );

            if( m_Master->m_use_modelfile_transparency == true )
            {
                m_model->m_Materials->m_Transparency.push_back( transparency );
            }
        }
        else if( strcmp( text, "shininess" ) == 0 )
        {
            float shininess;
            parseFloat( m_file, &shininess );

            // VRML value is normalized and openGL expects a value 0 - 128
            if( m_Master->m_use_modelfile_shininess == true )
            {
                shininess = shininess * 128.0f;
                m_model->m_Materials->m_Shininess.push_back( shininess );
            }
        }
    }

    wxLogTrace( traceVrmlV2Parser, wxT( "  Material failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_IndexedFaceSet()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_IndexedFaceSet" ) );

    char text[BUFLINE_SIZE];

    m_normalPerVertex = false;
    colorPerVertex = false;

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            return 0;
        }

        if( strcmp( text, "normalPerVertex" ) == 0 )
        {
            if( GetNextTag( m_file, text, sizeof(text) ) )
            {
                if( strcmp( text, "TRUE" ) == 0 )
                {
                    m_normalPerVertex = true;
                }
            }
        }
        else if( strcmp( text, "colorPerVertex" ) == 0 )
        {
            GetNextTag( m_file, text, sizeof(text) );

            if( strcmp( text, "TRUE" ) )
            {
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
        else if( strcmp( text, "USE" ) == 0 )
        {
            read_USE();
        }
    }

    wxLogTrace( traceVrmlV2Parser, wxT( "  IndexedFaceSet failed %s" ), text );
    return -1;
}


int VRML2_MODEL_PARSER::read_IndexedLineSet()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_IndexedLineSet" ) );

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
            continue;

        if( *text == '}' )
            return 0;

        if( strcmp( text, "Coordinate" ) == 0 )
            read_Coordinate();
        else if( strcmp( text, "coordIndex" ) == 0 )
            read_coordIndex();
        else if( strcmp( text, "DEF" ) == 0 )
            read_DEF_Coordinate();
    }

    return -1;
}


int VRML2_MODEL_PARSER::read_colorIndex()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_colorIndex" ) );

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

    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_colorIndex m_MaterialIndex.size: %u" ), (unsigned int)m_model->m_MaterialIndex.size() );

    return 0;
}


int VRML2_MODEL_PARSER::read_NormalIndex()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_NormalIndex" ) );

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
            coord_list.clear();
        }
        else
        {
            coord_list.push_back( dummy );
        }
    }

    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_NormalIndex m_NormalIndex.size: %u" ), (unsigned int)m_model->m_NormalIndex.size() );

    return 0;
}


int VRML2_MODEL_PARSER::read_coordIndex()
{
    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_coordIndex" ) );

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
            coord_list.clear();
        }
        else
        {
            coord_list.push_back( dummy );
        }
    }

    //wxLogTrace( traceVrmlV2Parser, wxT( "    read_coordIndex m_CoordIndex.size: %u" ), (unsigned int)m_model->m_CoordIndex.size() );

    return 0;
}


int VRML2_MODEL_PARSER::read_Color()
{
    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            return 0;
        }

        if( strcmp( text, "color" ) == 0 )
        {
            parseVertexList( m_file, m_model->m_Materials->m_DiffuseColor );
        }
    }

    //wxLogTrace( traceVrmlV2Parser, wxT( "  read_Color failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Normal()
{
    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            //if( m_normalPerVertex == false )
            //    wxLogTrace( traceVrmlV2Parser, wxT( "    read_Normal m_PerFaceNormalsNormalized.size: %u" ), (unsigned int)m_model->m_PerFaceNormalsNormalized.size() );
            //else
            //    wxLogTrace( traceVrmlV2Parser, wxT( "    read_Normal m_PerVertexNormalsNormalized.size: %u" ), (unsigned int)m_model->m_PerVertexNormalsNormalized.size() );

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
            }
        }
    }

    wxLogTrace( traceVrmlV2Parser, wxT( "  read_Normal failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Coordinate()
{
    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            //wxLogTrace( traceVrmlV2Parser, wxT( "    read_Coordinate m_Point.size: %u" ), (unsigned int)m_model->m_Point.size() );
            return 0;
        }

        if( strcmp( text, "point" ) == 0 )
        {
            parseVertexList( m_file, m_model->m_Point );
        }
    }

    wxLogTrace( traceVrmlV2Parser, wxT( "  read_Coordinate failed" ) );
    return -1;
}


/**
 * Read the point of the Coordinate for a DEF
 */
int VRML2_MODEL_PARSER::read_CoordinateDef()
{
    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
            continue;

        if( *text == '}' )
        {
            //wxLogTrace( traceVrmlV2Parser, wxT( "    read_CoordinateDef m_Point.size: %u" ), (unsigned int)m_model->m_Point.size() );
            return 0;
        }

        if( strcmp( text, "point" ) == 0 )
            parseVertexList( m_file, m_model->m_Point );
    }

    wxLogTrace( traceVrmlV2Parser, wxT( "  read_CoordinateDef failed" ) );
    return -1;
}
