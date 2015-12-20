/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#include <set>
#include <map>
#include <utility>
#include <cctype>
#include "vrml2_node.h"



static std::set< std::string > badNames;

typedef std::pair< std::string, WRLNODES > NODEITEM;
typedef std::map< std::string, WRLNODES > NODEMAP;
static NODEMAP nodenames;


WRL2NODE::WRL2NODE()
{
    m_Parent = NULL;
    m_Type = V2_END;

    if( badNames.empty() )
    {
        badNames.insert( "DEF" );
        badNames.insert( "EXTERNPROTO" );
        badNames.insert( "FALSE" );
        badNames.insert( "IS" );
        badNames.insert( "NULL" );
        badNames.insert( "PROTO" );
        badNames.insert( "ROUTE" );
        badNames.insert( "TO" );
        badNames.insert( "TRUE" );
        badNames.insert( "USE" );
        badNames.insert( "eventIn" );
        badNames.insert( "eventOut" );
        badNames.insert( "exposedField" );
        badNames.insert( "field" );
    }

    if( nodenames.empty() )
    {
        node_names.insert( NODEITEM( "Anchor", WRL2_ANCHOR ) );
        node_names.insert( NODEITEM( "Appearance", WRL2_APPEARANCE ) );
        node_names.insert( NODEITEM( "Audioclip", WRL2_AUDIOCLIP );
        node_names.insert( NODEITEM( "Background", WRL2_BACKGROUND ) );
        node_names.insert( NODEITEM( "Billboard", WRL2_BILLBOARD ) );
        node_names.insert( NODEITEM( "Box", WRL2_BOX ) );
        node_names.insert( NODEITEM( "Collision", WRL2_COLLISION ) );
        node_names.insert( NODEITEM( "Color", WRL2_COLOR ) );
        node_names.insert( NODEITEM( "ColorInterpolator", WRL2_COLORINTERPOLATOR ) );
        node_names.insert( NODEITEM( "Cone", WRL2_CONE ) );
        node_names.insert( NODEITEM( "Coordinate", WRL2_COORDINATE ) );
        node_names.insert( NODEITEM( "CoordinateInterpolator", WRL2_COORDINATEINTERPOLATOR ) );
        node_names.insert( NODEITEM( "Cylinder", WRL2_CYLINDER ) );
        node_names.insert( NODEITEM( "CylinderSensor", WRL2_CYLINDERSENSOR ) );
        node_names.insert( NODEITEM( "DirectionalLight", WRL2_DIRECTIONALLIGHT ) );
        node_names.insert( NODEITEM( "ElevationGrid", WRL2_ELEVATIONGRID ) );
        node_names.insert( NODEITEM( "Extrusion", WRL2_EXTRUSION ) );
        node_names.insert( NODEITEM( "Fog", WRL2_FOG ) );
        node_names.insert( NODEITEM( "FontStyle", WRL2_FONTSTYLE ) );
        node_names.insert( NODEITEM( "Group", WRL2_GROUP ) );
        node_names.insert( NODEITEM( "ImageTexture", WRL2_IMAGETEXTURE ) );
        node_names.insert( NODEITEM( "IndexedFaceSet", WRL2_INDEXEDFACESET ) );
        node_names.insert( NODEITEM( "IndexedLineSet", WRL2_INDEXEDLINESET ) );
        node_names.insert( NODEITEM( "Inline", WRL2_INLINE ) );
        node_names.insert( NODEITEM( "LOD", WRL2_LOD ) );
        node_names.insert( NODEITEM( "Material", WRL2_MATERIAL ) );
        node_names.insert( NODEITEM( "MovieTexture", WRL2_MOVIETEXTURE ) );
        node_names.insert( NODEITEM( "NavigationInfo", WRL2_NAVIGATIONINFO ) );
        node_names.insert( NODEITEM( "Normal", WRL2_NORMAL ) );
        node_names.insert( NODEITEM( "NormalInterpolator", WRL2_NORMALINTERPOLATOR ) );
        node_names.insert( NODEITEM( "OrientationInterpolator", WRL2_ORIENTATIONINTERPOLATOR ) );
        node_names.insert( NODEITEM( "PixelTexture", WRL2_PIXELTEXTURE ) );
        node_names.insert( NODEITEM( "PlaneSensor", WRL2_PLANESENSOR ) );
        node_names.insert( NODEITEM( "PointLight", WRL2_POINTLIGHT ) );
        node_names.insert( NODEITEM( "PointSet", WRL2_POINTSET ) );
        node_names.insert( NODEITEM( "PositionInterpolator", WRL2_POSITIONINTERPOLATOR ) );
        node_names.insert( NODEITEM( "ProximitySensor", WRL2_PROXIMITYSENSOR ) );
        node_names.insert( NODEITEM( "ScalarInterpolator", WRL2_SCALARINTERPOLATOR ) );
        node_names.insert( NODEITEM( "Script", WRL2_SCRIPT ) );
        node_names.insert( NODEITEM( "Shape", WRL2_SHAPE ) );
        node_names.insert( NODEITEM( "Sound", WRL2_SOUND ) );
        node_names.insert( NODEITEM( "Sphere", WRL2_SPHERE ) );
        node_names.insert( NODEITEM( "SphereSensor", WRL2_SPHERESENSOR ) );
        node_names.insert( NODEITEM( "SpotLight", WRL2_SPOTLIGHT ) );
        node_names.insert( NODEITEM( "Switch", WRL2_SWITCH ) );
        node_names.insert( NODEITEM( "Text", WRL2_TEXT ) );
        node_names.insert( NODEITEM( "TextureCoordinate", WRL2_TEXTURECOORDINATE ) );
        node_names.insert( NODEITEM( "TextureTransform", WRL2_TEXTURETRANSFORM ) );
        node_names.insert( NODEITEM( "TimeSensor", WRL2_TIMESENSOR ) );
        node_names.insert( NODEITEM( "TouchSensor", WRL2_TOUCHSENSOR ) );
        node_names.insert( NODEITEM( "Transform", WRL2_TRANSFORM ) );
        node_names.insert( NODEITEM( "ViewPoint", WRL2_VIEWPOINT ) );
        node_names.insert( NODEITEM( "VisibilitySensor", WRL2_VISIBILITYSENSOR ) );
        node_names.insert( NODEITEM( "WorldInfo", WRL2_WORLDINFO ) );
    }

    return;
}


WRL2NODE::~WRL2NODE()
{
    std::list< WRL2NODE* > m_BackPointers;  // nodes which hold a reference to this
    std::list< WRL2NODE* > m_Children;      // nodes owned by this node
    std::list< WRL2NODE* > m_References;    // nodes not owned but referenced by this node

    if( m_Parent )
        m_Parent->unlinkChildNode( this );

    std::list< WRL2NODE* >::iterator sBP = m_BackPointers.begin();
    std::list< WRL2NODE* >::iterator eBP = m_BackPointers.end();

    while( sBP != eBP )
    {
        (*sBP)->unlinkRefNode( this );
        ++sBP;
    }

    return;
}


void WRL2NODE::addNodeRef( WRL2NODE* aNode )
{
    std::list< WRL2NODE* >::iterator np =
        std::find( m_BackPointers.begin(), m_BackPointers.end(), aNode );

    if( np != m_BackPointers.end() )
        return;

    m_BackPointers.push_back( aNode );

    return;
}


void WRL2NODE::delNodeRef( WRL2NODE* aNode )
{
    std::list< WRL2NODE* >::iterator np =
        std::find( m_BackPointers.begin(), m_BackPointers.end(), aNode );

    if( np != m_BackPointers.end() )
    {
        m_BackPointers.erase( np );
        return;
    }

    #ifdef DEBUG
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] delNodeRef() did not find its target\n";
    #endif

    return;
}


WRL2TYPES WRL2NODE::GetNodeType( void ) const
{
    return m_Type;
}


WRL2NODE* WRL2NODE::GetParent( void )
{
    return m_Parent;
}


const char* WRL2NODE::GetName( void )
{
    return m_Name;
}


bool WRL2NODE::SetName(const char *aName)
{
    if( NULL == aName || '\0' == aName[0] )
    {
        m_Name.clear();
        return true;
    }

    std::set< std::string >::iterator item = badNames.find( aName );

    if( item != badNames.end() )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] invalid node name '" << *item << "' (matches restricted word)\n";
        #endif
        return false;
    }


    if( isdigit( aName[0] ) )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] invalid node name '" << *item << "' (begins with digit)\n";
        #endif
        return false;
    }

    std::string tmpstr( aName );
    #define BAD_CHARS1 "\"\'#+,-.\\[]{}\x00\x01\x02\x03\x04\x05\x06\x09\x0A\x0B\x0C\x0D\x0E\x0F"
    #define BAD_CHARS2 "\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"

    if( std::string::npos != tmpstr.find_first_of( BAD_CHARS1 )
        || std::string::npos != tmpstr.find_first_of( BAD_CHARS2 ) )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] invalid node name '" << *item;
        std::cerr<< "' (contains invalid character)\n";
        #endif
        return false;
    }

    m_Name = aName;

    return true;
}


const char* WRL2NODE::GetNodeTypeName( WRL::V2TYPES aNodeType ) const
{
    if( aNodeType < WRL2_BEGIN || aNodeType >= WRL2_END )
        return NULL;

    return nodenames[aNodeType]->first.c_str();
}
