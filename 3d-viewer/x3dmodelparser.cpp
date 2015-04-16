/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
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
 * @file x3dmodelparser.cpp
 */

#include <fctsys.h>
#include <macros.h>
#include <info3d_visu.h>
#include <wx/xml/xml.h>
#include <wx/tokenzr.h>
#include <wx/string.h>
#include <map>
#include <queue>
#include <vector>

#include <3d_struct.h>
#include <modelparsers.h>
#include <xnode.h>

/**
 * Trace mask used to enable or disable the trace output of the X3D parser code.
 * The debug output can be turned on by setting the WXTRACE environment variable to
 * "KI_TRACE_X3D_PARSER".  See the wxWidgets documentation on wxLogTrace for
 * more information.
 */
static const wxChar* traceX3DParser = wxT( "KI_TRACE_X3D_PARSER" );


X3D_MODEL_PARSER::X3D_MODEL_PARSER( S3D_MASTER* aMaster ) :
    S3D_MODEL_PARSER( aMaster )
{
    m_model.reset();
}


X3D_MODEL_PARSER::~X3D_MODEL_PARSER()
{
}


bool X3D_MODEL_PARSER::Load( const wxString& aFilename )
{
    wxLogTrace( traceX3DParser, wxT( "Loading: %s" ), GetChars( aFilename ) );

    wxXmlDocument doc;

    if( !doc.Load( aFilename ) )
    {
        wxLogTrace( traceX3DParser, wxT( "Error while parsing file: %s" ), GetChars( aFilename ) );
        return false;
    }

    if( doc.GetRoot()->GetName() != wxT( "X3D" ) )
    {
        wxLogTrace( traceX3DParser, wxT( "Filetype is not X3D: %s" ), GetChars( aFilename ) );
        return false;
    }

    // Switch the locale to standard C (needed to print floating point numbers)
    LOCALE_IO toggle;

    childs.clear();

    // Shapes are inside of Transform nodes
    // Transform node contains information about
    // transition, scale and rotation of the shape
    NODE_LIST transforms;
    GetChildsByName( doc.GetRoot(), wxT( "Transform" ), transforms );

    for( NODE_LIST::iterator node_it = transforms.begin();
         node_it != transforms.end();
         node_it++ )
    {
        m_model.reset( new S3D_MESH() );
        childs.push_back( m_model );

        wxXmlNode* node = *node_it;
        wxXmlAttribute* prop = node->GetAttributes();

        wxLogTrace( traceX3DParser, wxT( "Transform: %s %s" ), prop->GetName(), prop->GetValue() );

        readTransform( node );


    }

    return true;
}


void X3D_MODEL_PARSER::GetChildsByName( wxXmlNode* aParent,
        const wxString aName,
        std::vector<wxXmlNode*>& aResult )
{
    // (-Breadth-first search (BFS)-)
    // **NOTE** This function was changed to get only the first depth of ocorrences
    //          so it will be an workarround for the Bug #1443431
    std::queue<wxXmlNode*> found;

    found.push( aParent );

    while( !found.empty() )
    {
        wxXmlNode* elem = found.front();

        for( wxXmlNode* child = elem->GetChildren();
             child != NULL;
             child = child->GetNext() )
        {
            if( child->GetName() == aName )
                aResult.push_back( child );
            else                                // **NOTE** This function was changed here to get only the first depth of ocorrences
                found.push( child );
        }

        found.pop();
    }
}


void X3D_MODEL_PARSER::GetNodeProperties( wxXmlNode* aNode, PROPERTY_MAP& aProps )
{
    wxXmlAttribute* prop;

    for( prop = aNode->GetAttributes();
         prop != NULL;
         prop = prop->GetNext() )
    {
        aProps[ prop->GetName() ] = prop->GetValue();
    }
}


/* Private ----- */

void X3D_MODEL_PARSER::readTransform( wxXmlNode* aTransformNode )
{
    NODE_LIST childnodes;

    GetChildsByName( aTransformNode, wxT( "Material" ), childnodes );

    for( NODE_LIST::iterator node = childnodes.begin();
         node != childnodes.end();
         node++ )
    {
        readMaterial( *node );
    }

    childnodes.clear();

    PROPERTY_MAP properties;

    GetNodeProperties( aTransformNode, properties );



    GetChildsByName( aTransformNode, wxT( "IndexedFaceSet" ), childnodes );

    for( NODE_LIST::iterator node = childnodes.begin();
         node != childnodes.end();
         node++ )
    {
        readIndexedFaceSet( *node, properties );
    }

    childnodes.clear();
}


void X3D_MODEL_PARSER::readMaterial( wxXmlNode* aMatNode )
{
    glm::vec3 color;

    PROPERTY_MAP properties;

    GetNodeProperties( aMatNode, properties );

    // DEFine new Material named as value of DEF
    if( properties.find( wxT( "DEF" ) ) != properties.end() )
    {
        double amb, shine, transp;

        S3D_MATERIAL* material = new S3D_MATERIAL( GetMaster(), properties[ wxT( "DEF" ) ] );
        GetMaster()->Insert( material );

        m_model->m_Materials = material;

        if( !parseDoubleTriplet( properties[ wxT( "diffuseColor" ) ], color ) )
        {
            // DBG( printf( "diffuseColor parsing error" ) );
        }
        else
        {
            m_model->m_Materials->m_DiffuseColor.push_back( color );
        }

        if( !parseDoubleTriplet( properties[ wxT( "specularColor" ) ], color ) )
        {
            // DBG( printf( "specularColor parsing error" ) );
        }
        else
        {
            m_model->m_Materials->m_SpecularColor.push_back( color );
        }

        if( !parseDoubleTriplet( properties[ wxT( "emissiveColor" ) ], color ) )
        {
            // DBG( printf( "emissiveColor parsing error" ) );
        }
        else
        {
            m_model->m_Materials->m_EmissiveColor.push_back( color );
        }

        wxStringTokenizer values;
        values.SetString( properties[ wxT( "ambientIntensity" ) ] );

        if( values.GetNextToken().ToDouble( &amb ) )
        {
            m_model->m_Materials->m_AmbientColor.push_back( glm::vec3( amb, amb, amb ) );
        }
        else
        {
            // DBG( printf( "ambienterror" ) );
        }

        values.SetString( properties[ wxT( "shininess" ) ]  );

        if( values.GetNextToken().ToDouble( &shine ) )
        {
            // VRML value is normalized and openGL expects a value 0 - 128
            if( shine > 1.0 )
            {
                shine = 1.0;
            } else if( shine < 0.0 )
            {
                shine = 0.0;
            }
            shine = shine * 128.0f;
            m_model->m_Materials->m_Shininess.push_back( shine );
        }
        else
        {
            // DBG( printf( "shininess error" ) );
        }

        values.SetString( properties[ wxT( "transparency" ) ] );

        if( values.GetNextToken().ToDouble( &transp ) )
        {
            m_model->m_Materials->m_Transparency.push_back( transp );
        }
        else
        {
            // DBG( printf( "trans error" ) );
        }

        // VRML
        wxString vrml_material;
        PROPERTY_MAP::const_iterator p = ++properties.begin();    // skip DEF

        for( ; p != properties.end(); p++ )
        {
            vrml_material.Append( p->first + wxT( " " ) + p->second + wxT( "\n" ) );
        }

        vrml_materials.push_back( vrml_material );
    }
    // USE existing material named by value of USE
    else if( properties.find( wxT( "USE" ) ) != properties.end() )
    {
        S3D_MATERIAL* material = NULL;
        wxString mat_name = properties[ wxT( "USE" ) ];

        for( material = GetMaster()->m_Materials; material; material = material->Next() )
        {
            if( material->m_Name == mat_name )
            {
                wxString vrml_material;

                vrml_material.Append( wxString::Format( wxT( "specularColor %f %f %f\n" ),
                                material->m_SpecularColor[0].x,
                                material->m_SpecularColor[0].y,
                                material->m_SpecularColor[0].z ) );

                vrml_material.Append( wxString::Format( wxT( "diffuseColor %f %f %f\n" ),
                                material->m_DiffuseColor[0].x,
                                material->m_DiffuseColor[0].y,
                                material->m_DiffuseColor[0].z ) );

                vrml_material.Append( wxString::Format( wxT( "emissiveColor %f %f %f\n" ),
                                material->m_EmissiveColor[0].x,
                                material->m_EmissiveColor[0].y,
                                material->m_EmissiveColor[0].z ) );

                vrml_material.Append( wxString::Format( wxT( "ambientIntensity %f\n" ),
                                material->m_AmbientColor[0].x ) );

                vrml_material.Append( wxString::Format( wxT( "shininess %f\n" ),
                                material->m_Shininess[0] ) );

                vrml_material.Append( wxString::Format( wxT( "transparency %f\n" ),
                                material->m_Transparency[0] ) );

                vrml_materials.push_back( vrml_material );

                m_model->m_Materials = material;

                return;
            }
        }

        // DBG( printf( "ReadMaterial error: material not found\n" ) );
    }
}


bool X3D_MODEL_PARSER::parseDoubleTriplet( const wxString& aData,
        S3D_VERTEX& aResult )
{
    wxStringTokenizer tokens( aData );

    double x = 0;
    double y = 0;
    double z = 0;

    bool ret = tokens.GetNextToken().ToDouble( &x )
               && tokens.GetNextToken().ToDouble( &y )
               && tokens.GetNextToken().ToDouble( &z );

    aResult.x   = x;
    aResult.y   = y;
    aResult.z   = z;

    return ret;
}


void X3D_MODEL_PARSER::rotate( S3D_VERTEX& aV,
        S3D_VERTEX& aU,
        double angle )
{
    S3D_VERTEX rotated;
    double  C   = cos( angle );
    double  S   = sin( angle );
    double  t   = 1.0 - C;

    rotated.x = ( t * aU.x * aU.x + C ) * aV.x +
                ( t * aU.x * aU.y - S * aU.z ) * aV.y +
                ( t * aU.x * aU.z + S * aU.y ) * aV.z;

    rotated.y = ( t * aU.x * aU.y + S * aU.z ) * aV.x +
                ( t * aU.y * aU.y + C ) * aV.y +
                ( t * aU.y * aU.z - S * aU.x ) * aV.z;

    rotated.z = ( t * aU.x * aU.z - S * aU.y ) * aV.x +
                ( t * aU.y * aU.z + S * aU.x ) * aV.y +
                ( t * aU.z * aU.z + C) * aV.z;

    aV.x    = rotated.x;
    aV.y    = rotated.y;
    aV.z    = rotated.z;
}


/* Steps:
 * 1. Read transform data
 * 2. Read vertex triplets
 * 3. Read coordinate indexes
 * 4. Apply geometry to Master object
 */
void X3D_MODEL_PARSER::readIndexedFaceSet( wxXmlNode* aFaceNode,
        PROPERTY_MAP& aTransformProps )
{
    /* Step 1: Read transform data
     * --------------------------- */

    S3D_VERTEX translation;

    parseDoubleTriplet( aTransformProps[ wxT( "translation" ) ], translation );

    S3D_VERTEX scale;
    parseDoubleTriplet( aTransformProps[ wxT( "scale" ) ], scale );

    S3D_VERTEX rotation;
    double angle = 0.0;
    wxStringTokenizer tokens( aTransformProps[ wxT( "rotation" ) ] );

    double x = 0.0, y = 0.0, z = 0.0;

    if( !( tokens.GetNextToken().ToDouble( &x )
           && tokens.GetNextToken().ToDouble( &y )
           && tokens.GetNextToken().ToDouble( &z )
           && tokens.GetNextToken().ToDouble( &angle ) ) )
    {
        // DBG( printf( "rotation read error" ) );
    }
    else
    {
        rotation.x  = x;
        rotation.y  = y;
        rotation.z  = z;
    }

    /* Step 2: Read all coordinate points
     * ---------------------------- */
    std::vector<double> points;
    NODE_LIST coordinates;
    GetChildsByName( aFaceNode, wxT( "Coordinate" ), coordinates );

    PROPERTY_MAP coordinate_properties;
    // IndexedFaceSet has one Coordinate child node
    GetNodeProperties( coordinates[0], coordinate_properties );

    // Save points to vector as doubles
    wxStringTokenizer point_tokens( coordinate_properties[ wxT( "point" ) ] );
    double point = 0.0;

    while( point_tokens.HasMoreTokens() )
    {
        if( point_tokens.GetNextToken().ToDouble( &point ) )
        {
            points.push_back( point );
        }
        else
        {
            wxLogTrace( traceX3DParser, wxT( "Error converting to double" ) );
        }
    }

    if( points.size() % 3 != 0 )
    {
        // DBG( printf( "Number of points is incorrect" ) );
        return;
    }

    /* Create 3D vertex from 3 points and
     * apply transforms in order of SCALE, ROTATION, TRANSLATION
     */
    wxString vrml_pointlist;
    std::vector<S3D_VERTEX> triplets;

    for( unsigned id = 0; id < points.size() / 3; id++ )
    {
        int triplet_indx = id * 3;
        S3D_VERTEX point( points[ triplet_indx ],
                points[ triplet_indx + 1 ],
                points[ triplet_indx + 2 ] );

        point.x *= scale.x;
        point.y *= scale.y;
        point.z *= scale.z;

        rotate( point, rotation, angle );

        point.x += translation.x;
        point.y += translation.y;
        point.z += translation.z;

        m_model->m_Point.push_back( point );

        // VRML
        vrml_pointlist.Append( wxString::Format( wxT( "%f %f %f\n" ), point.x, point.y, point.z ) );
    }

    vrml_points.push_back( vrml_pointlist );


    /* Step 3: Read all color points
     * ---------------------------- */
    std::vector<double> color_points;
    NODE_LIST color;
    GetChildsByName( aFaceNode, wxT( "Color" ), color );

    // Some models lack color information, need to handle this safely
    if( !color.empty() )
    {
        PROPERTY_MAP color_properties;
        // IndexedFaceSet has one Coordinate child node
        GetNodeProperties( color[0], color_properties );

        // Save points to vector as doubles
        wxStringTokenizer colorpoint_tokens( color_properties[ wxT( "color" ) ] );
        double color_point = 0.0;

        while( colorpoint_tokens.HasMoreTokens() )
        {
            if( colorpoint_tokens.GetNextToken().ToDouble( &color_point ) )
            {
                color_points.push_back( color_point );
            }
            else
            {
                wxLogTrace( traceX3DParser, wxT( "Error converting to double" ) );
            }
        }

        if( color_points.size() % 3 != 0 )
        {
            // DBG( printf( "Number of points is incorrect" ) );
            return;
        }

        /* Create 3D face color from 3 color points
         */
        m_model->m_Materials->m_DiffuseColor.clear();

        for( unsigned id = 0; id < color_points.size() / 3; id++ )
        {
            m_model->m_MaterialIndexPerFace.push_back( id );

            int color_triplet_indx = id * 3;
            glm::vec3 colorface( color_points[ color_triplet_indx + 0 ],
                    color_points[ color_triplet_indx + 1 ],
                    color_points[ color_triplet_indx + 2 ] );

            m_model->m_Materials->m_DiffuseColor.push_back( colorface );
        }
    }


    /* -- Read coordinate indexes -- */
    PROPERTY_MAP faceset_properties;
    GetNodeProperties( aFaceNode, faceset_properties );

    wxString coordIndex_str = faceset_properties[ wxT( "coordIndex" ) ];
    wxStringTokenizer index_tokens( coordIndex_str );

    wxString vrml_coord_indx_list;

    std::vector<int> coord_list;
    coord_list.clear();

    while( index_tokens.HasMoreTokens() )
    {
        long index = 0;

        index_tokens.GetNextToken().ToLong( &index );

        // -1 marks the end of polygon
        if( index < 0 )
        {
            /* Step 4: Apply geometry to Master object
             * --------------------------------------- */
            m_model->m_CoordIndex.push_back( coord_list );

            coord_list.clear();
            vrml_coord_indx_list.Append( wxT( "-1\n" ) );
        }
        else
        {
            coord_list.push_back( index );
            vrml_coord_indx_list.Append( wxString::Format( wxT( "%ld " ), index ) );
        }
    }

    vrml_coord_indexes.push_back( vrml_coord_indx_list );
}
