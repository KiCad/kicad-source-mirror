/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 Mario Luzeiro <mrluzeiro@gmail.com>
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

#define BUFLINE_SIZE 1024

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
    m_debugSpacer = "";
}


VRML2_MODEL_PARSER::~VRML2_MODEL_PARSER()
{
}

void VRML2_MODEL_PARSER::debug_enter()
{
    m_debugSpacer.Append(' ');
}

void VRML2_MODEL_PARSER::debug_exit()
{
    m_debugSpacer.RemoveLast();
}

bool VRML2_MODEL_PARSER::Load( const wxString& aFilename )
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "Loading: %s" ), GetChars( aFilename ) );
    debug_enter();

    m_file = wxFopen( aFilename, wxT( "rt" ) );

    if( m_file == NULL )
    {
        debug_exit();
        wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "Failed to open file: %s" ), GetChars( aFilename ) );
        return false;
    }

    m_Filename = aFilename;

    // Switch the locale to standard C (needed to print floating point numbers)
    LOCALE_IO toggle;

    loadFileModel( NULL );

    fclose( m_file );

    debug_exit();
    return true;
}


bool VRML2_MODEL_PARSER::Load( const wxString& aFilename, S3D_MESH *aTransformationModel )
{
    if( aTransformationModel )
    {
        wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "Loading: %s" ), GetChars( aFilename ) );
        debug_enter();

        m_file = wxFopen( aFilename, wxT( "rt" ) );

        if( m_file == NULL )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "Failed to open file: %s" ), GetChars( aFilename ) );
            return false;
        }

        m_Filename = aFilename;

        // Switch the locale to standard C (needed to print floating point numbers)
        LOCALE_IO toggle;

        loadFileModel( aTransformationModel );

        fclose( m_file );

        debug_exit();
        return true;
    }

    debug_exit();
    return false;
}


int VRML2_MODEL_PARSER::loadFileModel( S3D_MESH *aTransformationModel )
{
    char text[BUFLINE_SIZE];

    debug_enter();

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( ( *text == '}' ) || ( *text == ']' ) )
        {
            continue;
        }

        if( strcmp( text, "Transform" ) == 0 )
        {
            m_model = new S3D_MESH();

            S3D_MESH* save_ptr = m_model;

            if( read_Transform() == 0 )
            {
                m_model = save_ptr;

                if( ((m_model->m_Point.size() == 0) || (m_model->m_CoordIndex.size() == 0)) &&
                     (m_model->childs.size() == 0) )
                {
                    delete m_model;
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "loadFileModel: skipping model with no points or childs" ) );
                }
                else
                {
                    if( aTransformationModel )
                    {
                        m_model->m_translation   = aTransformationModel->m_translation;
                        m_model->m_rotation      = aTransformationModel->m_rotation;
                        m_model->m_scale         = aTransformationModel->m_scale;
                    }
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "loadFileModel: Add model with %lu points, %lu coordIndex, %lu childs." ),
                                                                    m_model->m_Point.size(),
                                                                    m_model->m_CoordIndex.size(),
                                                                    m_model->childs.size() );
                    m_ModelParser->childs.push_back( m_model );
                }
            }
            else
            {
                delete m_model;
            }
        }
        else if( strcmp( text, "DEF" ) == 0 )
        {
            m_model = new S3D_MESH();

            S3D_MESH* save_ptr = m_model;

            if( read_DEF() == 0 )
            {
                m_model = save_ptr;

                if( ((m_model->m_Point.size() == 0) || (m_model->m_CoordIndex.size() == 0)) &&
                     (m_model->childs.size() == 0) )
                {
                    delete m_model;
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "loadFileModel: skipping model with no points or childs" ) );
                }
                else
                {
                    if( aTransformationModel )
                    {
                        m_model->m_translation   = aTransformationModel->m_translation;
                        m_model->m_rotation      = aTransformationModel->m_rotation;
                        m_model->m_scale         = aTransformationModel->m_scale;
                    }
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "loadFileModel: Add model with %lu points, %lu coordIndex, %lu childs." ),
                                                m_model->m_Point.size(),
                                                m_model->m_CoordIndex.size(),
                                                m_model->childs.size() );
                    m_ModelParser->childs.push_back( m_model );
                }
            }
            else
            {
                delete m_model;
            }
        }
        else if( strcmp( text, "Shape" ) == 0 )
        {
            m_model = new S3D_MESH();
            
            S3D_MESH* save_ptr = m_model;

            if( read_Shape() == 0 )
            {
                m_model = save_ptr;

                if( ((m_model->m_Point.size() == 0) || (m_model->m_CoordIndex.size() == 0)) &&
                     (m_model->childs.size() == 0) )
                {
                    delete m_model;
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "loadFileModel: skipping model with no points or childs" ) );
                }
                else
                {
                    if( aTransformationModel )
                    {
                        m_model->m_translation   = aTransformationModel->m_translation;
                        m_model->m_rotation      = aTransformationModel->m_rotation;
                        m_model->m_scale         = aTransformationModel->m_scale;
                    }

                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "loadFileModel: Add model with %lu points, %lu coordIndex, %lu childs." ),
                                                m_model->m_Point.size(),
                                                m_model->m_CoordIndex.size(),
                                                m_model->childs.size() );

                    m_ModelParser->childs.push_back( m_model );
                }
            }
            else
            {
                delete m_model;
            }
        }
    }

    debug_exit();
    return 0;
}


int VRML2_MODEL_PARSER::read_Transform()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Transform" ) );
    
    debug_enter();

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Transform exit" ) );
            return 0;
        }

        if( strcmp( text, "Transform" ) == 0 )
        {
            m_model = new S3D_MESH();
            
            S3D_MESH* save_ptr = m_model;

            if( read_Transform() == 0 )
            {
                m_model = save_ptr;

                if( ((m_model->m_Point.size() == 0) || (m_model->m_CoordIndex.size() == 0)) &&
                    (m_model->childs.size() == 0) )
                {
                    delete m_model;
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Transform: skipping model with no points or childs" ) );
                }
                else
                {
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Transform: Add child model with %lu points, %lu coordIndex, %lu childs." ),
                                                m_model->m_Point.size(),
                                                m_model->m_CoordIndex.size(),
                                                m_model->childs.size() );

                    m_ModelParser->childs.push_back( m_model );
                }
            }
            else
            {
                delete m_model;
            }
        }
        else if( strcmp( text, "translation" ) == 0 )
        {
            ParseVertex( m_file, m_model->m_translation );
            
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "translation (%f,%f,%f)" ),
                                          m_model->m_translation.x,
                                          m_model->m_translation.y,
                                          m_model->m_translation.z );
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

                wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "rotation failed, setting to zeros" ) );
            }
            else
            {
                m_model->m_rotation[3] = m_model->m_rotation[3] * 180.0f / 3.14f;    // !TODO: use constants or functions
            }

            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "rotation (%f,%f,%f,%f)" ),
                              m_model->m_rotation[0],
                              m_model->m_rotation[1],
                              m_model->m_rotation[2],
                              m_model->m_rotation[3] );
        }
        else if( strcmp( text, "scale" ) == 0 )
        {
            ParseVertex( m_file, m_model->m_scale );

            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "scale (%f,%f,%f)" ), m_model->m_scale.x, m_model->m_scale.y, m_model->m_scale.z );
        }
        else if( strcmp( text, "scaleOrientation" ) == 0 )
        {
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "scaleOrientation is not implemented, but it will be parsed" ) );

            glm::vec4 vecDummy;
            if( fscanf( m_file, "%f %f %f %f", &vecDummy[0],
                        &vecDummy[1],
                        &vecDummy[2],
                        &vecDummy[3] ) != 4 )
            {
                vecDummy[0]  = 0.0f;
                vecDummy[1]  = 0.0f;
                vecDummy[2]  = 0.0f;
                vecDummy[3]  = 0.0f;

                wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "scaleOrientation failed, setting to zeros" ) );
            }

            //wxLogTrace( traceVrmlV2Parser, wxT( "    scaleOrientation (%f,%f,%f,%f)" ),
            //                   m_model->m_scaleOrientation[0],
            //                   m_model->m_scaleOrientation[1],
            //                   m_model->m_scaleOrientation[2],
            //                   m_model->m_scaleOrientation[3] );
        }
        else if( strcmp( text, "center" ) == 0 )
        {
            // this is not used
            glm::vec3 vecDummy;
            ParseVertex( m_file, vecDummy );

            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "center is not implemented (%f,%f,%f)" ), vecDummy.x, vecDummy.y, vecDummy.z );
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
            // Keep looking for things in a transform
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "Group" ) );
            read_Transform();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "Transform Group exit" ) );
            //return ret;
        }
        else if( strcmp( text, "Inline" ) == 0 )
        {
            read_Inline();
        }
        else if( strcmp( text, "Shape" ) == 0 )
        {
            // Save the pointer
            S3D_MESH* parent = m_model;

            S3D_MESH* new_mesh_model = new S3D_MESH();

            // Assign the current pointer
            m_model = new_mesh_model;

            S3D_MESH* save_ptr = m_model;

            if( read_Shape() == 0 )
            {
                m_model = save_ptr;

                if( ((m_model->m_Point.size() == 0) || (m_model->m_CoordIndex.size() == 0)) &&
                     (m_model->childs.size() == 0) )
                {
                    delete m_model;
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Transform: Shape, skipping model with no points or childs" ) );
                }
                else
                {
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Transform: Shape, Add child model with %lu points, %lu coordIndex, %lu childs." ),
                                                m_model->m_Point.size(),
                                                m_model->m_CoordIndex.size(),
                                                m_model->childs.size() );

                    parent->childs.push_back( m_model );
                }
            }
            else
            {
                delete m_model;   
            }

            m_model = parent;
        }
        else if( strcmp( text, "DEF" ) == 0 )
        {
            read_DEF();
        }
        else if( strcmp( text, "USE" ) == 0 )
        {
            char useLabel[BUFLINE_SIZE];

            if( GetNextTag( m_file, useLabel, sizeof(useLabel) ) )
            {
                // Check if a ',' is at the end and remove it
                if( useLabel[strlen(useLabel) - 1] == ',' )
                {
                    useLabel[strlen(useLabel) - 1] = 0;
                }

                std::string strUseLabel = useLabel;

                // Look for it in our group map.
                VRML2_DEF_GROUP_MAP::iterator groupIt;

                groupIt = m_defGroupMap.find( strUseLabel );

                // Checf if not previously defined.
                if( groupIt == m_defGroupMap.end() )
                {
                    debug_exit();
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "USE: group %s not previously defined "
                                                        "in a DEF section." ), strUseLabel );
                    return -1;
                }

                S3D_MESH* ptrModel = groupIt->second;

                if( ((ptrModel->m_Point.size() == 0) || (ptrModel->m_CoordIndex.size() == 0)) &&
                     (ptrModel->childs.size() == 0) )
                {
                    // !TODO: delete in the end
                    //delete ptrModel;
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Transform: USE %s, skipping model with no points or childs" ), useLabel );
                }
                else
                {
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Transform: USE %s Add child model with %lu points, %lu coordIndex, %lu childs." ),
                            useLabel,
                            ptrModel->m_Point.size(),
                            ptrModel->m_CoordIndex.size(),
                            ptrModel->childs.size() );

                    m_model->childs.push_back( ptrModel );
                }
            }
            else
            {
                debug_exit();
                wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Transform: USE Failed to get the label name" ) );
                return -1;
            }
        }
        else
        {
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Transform: %s NotImplemented" ), text );
            Read_NotImplemented( m_file, '}' );
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Transform failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Inline()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Inline" ) );
    debug_enter();

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
            continue;

        if( *text == '}' )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Inline exit" ) );
            return 0;
        }

        if( strcmp( text, "url" ) == 0 )
        {
            if( GetString( m_file, text, sizeof(text) ) )
            {
                wxString filename;
                filename = filename.FromUTF8( text );

                #ifdef __WINDOWS__
                    filename.Replace( wxT( "/" ), wxT( "\\" ) );
                #else
                    filename.Replace( wxT( "\\" ), wxT( "/" ) );
                #endif

                bool fileExists = false;

                if( wxFileName::FileExists( filename ) )
                {
                    fileExists = true;
                }
                else
                {
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "URL Failed to open file as a full path: \"%s\", will try now a relative path..." ), filename );    

                    #ifdef __WINDOWS__
                        filename = m_Filename.GetPath() + '\\' + filename;
                    #else
                        filename = m_Filename.GetPath() + '/' + filename;
                    #endif
                    
                    
                    if( wxFileName::FileExists( filename ) )
                    {
                        fileExists = true;
                    }
                    else
                    {
                        wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "URL Failed to open file: \"%s\"" ), filename );    
                    }
                }
                
                if( fileExists )
                {
                    // Will now create a new parser and set the default
                    // transfomation model to apply on the root
                    VRML2_MODEL_PARSER *newParser = new VRML2_MODEL_PARSER( this->m_ModelParser );
                    newParser->Load( filename, m_model );
                    delete newParser;
                }
                else
                {
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "URL Failed to open file: %s" ), text );    
                }
            }
            else
            {
                // If fail get url text, exit with failure
                wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "URL failed read url string" ) );
                break;
            }
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Inline failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_DEF_Coordinate()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_DEF_Coordinate" ) );
    debug_enter();

    char text[BUFLINE_SIZE];

    // Get the name of the definition.
    if( !GetNextTag( m_file, text, sizeof(text) ) )
    {
        debug_exit();
        wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_DEF_Coordinate failed to get next tag" ) );
        return -1;
    }

    std::string coordinateName = text;

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
            continue;

        if( *text == '}' )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_DEF_Coordinate exit" ) );
            return 0;
        }

        if( strcmp( text, "Coordinate" ) == 0 )
        {
            int retVal = read_CoordinateDef();

            if( retVal == 0 )
                m_defCoordinateMap.insert( std::make_pair( coordinateName, m_model->m_Point ) );

            debug_exit();
            return retVal;
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_DEF_Coordinate failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_DEF()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_DEF" ) );
    debug_enter();

    char text[BUFLINE_SIZE];
    char tagName[BUFLINE_SIZE];

    if( !GetNextTag( m_file, tagName, sizeof(tagName) ) )
    {
        debug_exit();
        wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "DEF failed GetNextTag first" ) );
        return -1;
    }

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "skipping %c" ), *text );
            continue;
        }

        if( *text == '}' )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_DEF exit" ) );
            return 0;
        }

        if( strcmp( text, "Transform" ) == 0 )
        {
            int ret = read_Transform();
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_DEF exit after Transform, please check and validate" ) );
            return ret;
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
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "Shape" ) );

            // Save the pointer
            S3D_MESH* parent = m_model;

            S3D_MESH* new_mesh_model = new S3D_MESH();

            // Assign the current pointer
            m_model = new_mesh_model;

            S3D_MESH* save_ptr = m_model;

            if( read_Shape() == 0 )
            {
                m_model = save_ptr;

                if( ((m_model->m_Point.size() == 0) || (m_model->m_CoordIndex.size() == 0)) &&
                     (m_model->childs.size() == 0) )
                {
                    delete m_model;
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_DEF: Shape, skipping model with no points or childs" ) );
                }
                else
                {
                    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_DEF: Shape, Add child model with %lu points, %lu coordIndex, %lu childs." ),
                            m_model->m_Point.size(),
                            m_model->m_CoordIndex.size(),
                            m_model->childs.size() );

                    parent->childs.push_back( m_model );
                }
            }
            else
            {
                delete m_model;   
            }

            m_model = parent;
        }
        else if( strcmp( text, "IndexedFaceSet" ) == 0 )
        {
            read_IndexedFaceSet();
        }
        else if( strcmp( text, "Group" ) == 0 )
        {
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "Group %s" ), tagName );

            // Save the pointer
            S3D_MESH* parent = m_model;

            S3D_MESH* new_mesh_model = new S3D_MESH();

            // Assign the current pointer
            m_model = new_mesh_model;

            // It will be the same as read a new Transform
            if( read_Transform() == 0 )
            {
                std::string groupName = tagName;
                //m_defGroupMap.insert( std::make_pair( groupName, new_mesh_model ) );
                m_defGroupMap[groupName] = new_mesh_model;

                wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "Group %s: inserted model with %lu points, %lu coordIndex, %lu childs." ),
                            tagName,
                            new_mesh_model->m_Point.size(),
                            new_mesh_model->m_CoordIndex.size(),
                            new_mesh_model->childs.size() );
            }
            else
            {
                delete m_model;
            }

            // Restore current model pointer
            m_model = parent;

            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_DEF %s Group exit" ), tagName );
            return 0;
        }
        else
        {
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "DEF %s %s NotImplemented, skipping." ), tagName, text );
            Read_NotImplemented( m_file, '}' );
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "DEF failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_IndexedFaceSet_USE()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_IndexedFaceSet_USE" ) );
    debug_enter();

    char text[BUFLINE_SIZE];

    // Get the name of the definition.
    if( !GetNextTag( m_file, text, sizeof(text) ) )
    {
        debug_exit();
        wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_IndexedFaceSet_USE failed to get next tag" ) );
        return -1;
    }

    std::string coordinateName = text;

    // Look for it in our coordinate map.
    VRML2_COORDINATE_MAP::iterator coordinate;
    coordinate = m_defCoordinateMap.find( coordinateName );

    // Not previously defined.
    if( coordinate == m_defCoordinateMap.end() )
    {
        debug_exit();
        wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "USE: coordinate %s not previously defined "
                                            "in a DEF section." ), text );
        return -1;
    }

    m_model->m_Point = coordinate->second;
    debug_exit();
    return 0;
}


int VRML2_MODEL_PARSER::read_Shape()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Shape" ) );
    debug_enter();

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Shape exit" ) );
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
            read_geometry();
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
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "%s NotImplemented" ), text );
            Read_NotImplemented( m_file, '}' );
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "Shape failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_geometry()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_geometry" ) );
    debug_enter();

    char text[BUFLINE_SIZE];
    char tagName[BUFLINE_SIZE];
    tagName[0] = 0;

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_geometry exit" ) );
            return 0;
        }

        if( strcmp( text, "DEF" ) == 0 )
        {
            if( !GetNextTag( m_file, tagName, sizeof(tagName) ) )
            {
                debug_exit();
                wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "DEF failed GetNextTag first" ) );
                return -1;
            }
        }
        else if( strcmp( text, "IndexedFaceSet" ) == 0 )
        {
            int ret = read_IndexedFaceSet();
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_geometry exit, after IndexedFaceSet" ) );
            return ret;
        }
        else
        {
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_geometry: %s NotImplemented" ), text );
            int ret = Read_NotImplemented( m_file, '}' );
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_geometry exit, after %s" ), text);
            return ret;
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_geometry failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Appearance()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Appearance" ) );
    debug_enter();

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Appearance exit" ) );
            return 0;
        }

        if( strcmp( text, "material" ) == 0 )
        {
            read_material();
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "Appearance failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_material()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_material" ) );
    debug_enter();

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
                int ret = read_Material();
                debug_exit();
                return ret;
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
                        int ret = read_Material();
                        debug_exit();
                        return ret;
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
                        debug_exit();
                        return 0;
                    }
                }

                wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_material error: material not found" ) );
            }
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "failed material" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Material()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Material" ) );
    debug_enter();

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
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Material exit" ) );
            return 0;
        }

        if( strcmp( text, "diffuseColor" ) == 0 )
        {
            ParseVertex( m_file, vertex );
            m_model->m_Materials->m_DiffuseColor.push_back( vertex );
        }
        else if( strcmp( text, "emissiveColor" ) == 0 )
        {
            ParseVertex( m_file, vertex );

            if( m_Master->m_use_modelfile_emissiveColor == true )
            {
                m_model->m_Materials->m_EmissiveColor.push_back( vertex );
            }
        }
        else if( strcmp( text, "specularColor" ) == 0 )
        {
            ParseVertex( m_file, vertex );

            if( m_Master->m_use_modelfile_specularColor == true )
            {
                m_model->m_Materials->m_SpecularColor.push_back( vertex );
            }
        }
        else if( strcmp( text, "ambientIntensity" ) == 0 )
        {
            float ambientIntensity;
            ParseFloat( m_file, &ambientIntensity );

            if( m_Master->m_use_modelfile_ambientIntensity == true )
            {
                m_model->m_Materials->m_AmbientColor.push_back( glm::vec3( ambientIntensity,
                                ambientIntensity, ambientIntensity ) );
            }
        }
        else if( strcmp( text, "transparency" ) == 0 )
        {
            float transparency;
            ParseFloat( m_file, &transparency );

            if( m_Master->m_use_modelfile_transparency == true )
            {
                m_model->m_Materials->m_Transparency.push_back( transparency );
            }
        }
        else if( strcmp( text, "shininess" ) == 0 )
        {
            float shininess;
            ParseFloat( m_file, &shininess );

            // VRML value is normalized and openGL expects a value 0 - 128
            if( m_Master->m_use_modelfile_shininess == true )
            {
                shininess = shininess * 128.0f;
                m_model->m_Materials->m_Shininess.push_back( shininess );
            }
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer +  wxT( "Material failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_IndexedFaceSet()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_IndexedFaceSet" ) );
    debug_enter();

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
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_IndexedFaceSet exit" ) );
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
            read_IndexedFaceSet_USE();
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "IndexedFaceSet failed %s" ), text );
    return -1;
}


int VRML2_MODEL_PARSER::read_IndexedLineSet()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_IndexedLineSet" ) );
    debug_enter();

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
            continue;

        if( *text == '}' )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_IndexedLineSet exit" ) );
            return 0;
        }

        if( strcmp( text, "Coordinate" ) == 0 )
            read_Coordinate();
        else if( strcmp( text, "coordIndex" ) == 0 )
            read_coordIndex();
        else if( strcmp( text, "DEF" ) == 0 )
            read_DEF_Coordinate();
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_IndexedLineSet failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_colorIndex()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_colorIndex" ) );
    debug_enter();

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

    //wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_colorIndex m_MaterialIndex.size: %u" ), (unsigned int)m_model->m_MaterialIndex.size() );
    debug_exit();
    return 0;
}


int VRML2_MODEL_PARSER::read_NormalIndex()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_NormalIndex" ) );
    debug_enter();

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

    //wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_NormalIndex m_NormalIndex.size: %u" ), (unsigned int)m_model->m_NormalIndex.size() );
    debug_exit();
    return 0;
}


int VRML2_MODEL_PARSER::read_coordIndex()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_coordIndex" ) );
    debug_enter();

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

    //wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_coordIndex m_CoordIndex.size: %u" ), (unsigned int)m_model->m_CoordIndex.size() );
    debug_exit();
    return 0;
}


int VRML2_MODEL_PARSER::read_Color()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Color" ) );
    debug_enter();

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Color exit" ) );
            return 0;
        }

        if( strcmp( text, "color" ) == 0 )
        {
            ParseVertexList( m_file, m_model->m_Materials->m_DiffuseColor );
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Color failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Normal()
{
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Normal" ) );
    debug_enter();

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            // Debug
            //if( m_normalPerVertex == false )
            //    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Normal m_PerFaceNormalsNormalized.size: %u" ), (unsigned int)m_model->m_PerFaceNormalsNormalized.size() );
            //else
            //    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Normal m_PerVertexNormalsNormalized.size: %u" ), (unsigned int)m_model->m_PerVertexNormalsNormalized.size() );

            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Normal exit" ) );
            return 0;
        }

        if( strcmp( text, "vector" ) == 0 )
        {
            if( m_normalPerVertex == false )
            {
                ParseVertexList( m_file, m_model->m_PerFaceNormalsNormalized );
            }
            else
            {
                ParseVertexList( m_file, m_model->m_PerVertexNormalsNormalized );
            }
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Normal failed" ) );
    return -1;
}


int VRML2_MODEL_PARSER::read_Coordinate()
{
    debug_enter();

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
        {
            continue;
        }

        if( *text == '}' )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Coordinate exit" ) );
            //wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Coordinate m_Point.size: %u" ), (unsigned int)m_model->m_Point.size() );
            return 0;
        }

        if( strcmp( text, "point" ) == 0 )
        {
            ParseVertexList( m_file, m_model->m_Point );
        }
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_Coordinate failed" ) );
    return -1;
}


/**
 * Read the point of the Coordinate for a DEF
 */
int VRML2_MODEL_PARSER::read_CoordinateDef()
{
    debug_enter();

    char text[BUFLINE_SIZE];

    while( GetNextTag( m_file, text, sizeof(text) ) )
    {
        if( *text == ']' )
            continue;

        if( *text == '}' )
        {
            debug_exit();
            wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_CoordinateDef exit" ) );
            //wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_CoordinateDef m_Point.size: %u" ), (unsigned int)m_model->m_Point.size() );
            return 0;
        }

        if( strcmp( text, "point" ) == 0 )
            ParseVertexList( m_file, m_model->m_Point );
    }

    debug_exit();
    wxLogTrace( traceVrmlV2Parser, m_debugSpacer + wxT( "read_CoordinateDef failed" ) );
    return -1;
}
