/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <iterator>
#include <cctype>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <wx/log.h>

#include "vrml2_node.h"


static std::set< std::string > badNames;

typedef std::pair< std::string, WRL2NODES > NODEITEM;
typedef std::map< std::string, WRL2NODES > NODEMAP;
static NODEMAP nodenames;


WRL2NODE::WRL2NODE()
{
    m_sgNode = nullptr;
    m_Parent = nullptr;
    m_Type = WRL2NODES::WRL2_END;

    if( badNames.empty() )
    {
        badNames.emplace( "DEF" );
        badNames.emplace( "EXTERNPROTO" );
        badNames.emplace( "FALSE" );
        badNames.emplace( "IS" );
        badNames.emplace( "NULL" );
        badNames.emplace( "PROTO" );
        badNames.emplace( "ROUTE" );
        badNames.emplace( "TO" );
        badNames.emplace( "TRUE" );
        badNames.emplace( "USE" );
        badNames.emplace( "eventIn" );
        badNames.emplace( "eventOut" );
        badNames.emplace( "exposedField" );
        badNames.emplace( "field" );
    }

    if( nodenames.empty() )
    {
        nodenames.emplace( NODEITEM( "Anchor", WRL2NODES::WRL2_ANCHOR ) );
        nodenames.emplace( NODEITEM( "Appearance", WRL2NODES::WRL2_APPEARANCE ) );
        nodenames.emplace( NODEITEM( "Audioclip", WRL2NODES::WRL2_AUDIOCLIP ) );
        nodenames.emplace( NODEITEM( "Background", WRL2NODES::WRL2_BACKGROUND ) );
        nodenames.emplace( NODEITEM( "Billboard", WRL2NODES::WRL2_BILLBOARD ) );
        nodenames.emplace( NODEITEM( "Box", WRL2NODES::WRL2_BOX ) );
        nodenames.emplace( NODEITEM( "Collision", WRL2NODES::WRL2_COLLISION ) );
        nodenames.emplace( NODEITEM( "Color", WRL2NODES::WRL2_COLOR ) );
        nodenames.emplace( NODEITEM( "ColorInterpolator", WRL2NODES::WRL2_COLORINTERPOLATOR ) );
        nodenames.emplace( NODEITEM( "Cone", WRL2NODES::WRL2_CONE ) );
        nodenames.emplace( NODEITEM( "Coordinate", WRL2NODES::WRL2_COORDINATE ) );
        nodenames.emplace( NODEITEM( "CoordinateInterpolator",
                                    WRL2NODES::WRL2_COORDINATEINTERPOLATOR ) );
        nodenames.emplace( NODEITEM( "Cylinder", WRL2NODES::WRL2_CYLINDER ) );
        nodenames.emplace( NODEITEM( "CylinderSensor", WRL2NODES::WRL2_CYLINDERSENSOR ) );
        nodenames.emplace( NODEITEM( "DirectionalLight", WRL2NODES::WRL2_DIRECTIONALLIGHT ) );
        nodenames.emplace( NODEITEM( "ElevationGrid", WRL2NODES::WRL2_ELEVATIONGRID ) );
        nodenames.emplace( NODEITEM( "Extrusion", WRL2NODES::WRL2_EXTRUSION ) );
        nodenames.emplace( NODEITEM( "Fog", WRL2NODES::WRL2_FOG ) );
        nodenames.emplace( NODEITEM( "FontStyle", WRL2NODES::WRL2_FONTSTYLE ) );
        nodenames.emplace( NODEITEM( "Group", WRL2NODES::WRL2_GROUP ) );
        nodenames.emplace( NODEITEM( "ImageTexture", WRL2NODES::WRL2_IMAGETEXTURE ) );
        nodenames.emplace( NODEITEM( "IndexedFaceSet", WRL2NODES::WRL2_INDEXEDFACESET ) );
        nodenames.emplace( NODEITEM( "IndexedLineSet", WRL2NODES::WRL2_INDEXEDLINESET ) );
        nodenames.emplace( NODEITEM( "Inline", WRL2NODES::WRL2_INLINE ) );
        nodenames.emplace( NODEITEM( "LOD", WRL2NODES::WRL2_LOD ) );
        nodenames.emplace( NODEITEM( "Material", WRL2NODES::WRL2_MATERIAL ) );
        nodenames.emplace( NODEITEM( "MovieTexture", WRL2NODES::WRL2_MOVIETEXTURE ) );
        nodenames.emplace( NODEITEM( "NavigationInfo", WRL2NODES::WRL2_NAVIGATIONINFO ) );
        nodenames.emplace( NODEITEM( "Normal", WRL2NODES::WRL2_NORMAL ) );
        nodenames.emplace( NODEITEM( "NormalInterpolator", WRL2NODES::WRL2_NORMALINTERPOLATOR ) );
        nodenames.emplace( NODEITEM( "OrientationInterpolator",
                                    WRL2NODES::WRL2_ORIENTATIONINTERPOLATOR ) );
        nodenames.emplace( NODEITEM( "PixelTexture", WRL2NODES::WRL2_PIXELTEXTURE ) );
        nodenames.emplace( NODEITEM( "PlaneSensor", WRL2NODES::WRL2_PLANESENSOR ) );
        nodenames.emplace( NODEITEM( "PointLight", WRL2NODES::WRL2_POINTLIGHT ) );
        nodenames.emplace( NODEITEM( "PointSet", WRL2NODES::WRL2_POINTSET ) );
        nodenames.emplace( NODEITEM( "PositionInterpolator",
                                    WRL2NODES::WRL2_POSITIONINTERPOLATOR ) );
        nodenames.emplace( NODEITEM( "ProximitySensor", WRL2NODES::WRL2_PROXIMITYSENSOR ) );
        nodenames.emplace( NODEITEM( "ScalarInterpolator", WRL2NODES::WRL2_SCALARINTERPOLATOR ) );
        nodenames.emplace( NODEITEM( "Script", WRL2NODES::WRL2_SCRIPT ) );
        nodenames.emplace( NODEITEM( "Shape", WRL2NODES::WRL2_SHAPE ) );
        nodenames.emplace( NODEITEM( "Sound", WRL2NODES::WRL2_SOUND ) );
        nodenames.emplace( NODEITEM( "Sphere", WRL2NODES::WRL2_SPHERE ) );
        nodenames.emplace( NODEITEM( "SphereSensor", WRL2NODES::WRL2_SPHERESENSOR ) );
        nodenames.emplace( NODEITEM( "SpotLight", WRL2NODES::WRL2_SPOTLIGHT ) );
        nodenames.emplace( NODEITEM( "Switch", WRL2NODES::WRL2_SWITCH ) );
        nodenames.emplace( NODEITEM( "Text", WRL2NODES::WRL2_TEXT ) );
        nodenames.emplace( NODEITEM( "TextureCoordinate", WRL2NODES::WRL2_TEXTURECOORDINATE ) );
        nodenames.emplace( NODEITEM( "TextureTransform", WRL2NODES::WRL2_TEXTURETRANSFORM ) );
        nodenames.emplace( NODEITEM( "TimeSensor", WRL2NODES::WRL2_TIMESENSOR ) );
        nodenames.emplace( NODEITEM( "TouchSensor", WRL2NODES::WRL2_TOUCHSENSOR ) );
        nodenames.emplace( NODEITEM( "Transform", WRL2NODES::WRL2_TRANSFORM ) );
        nodenames.emplace( NODEITEM( "ViewPoint", WRL2NODES::WRL2_VIEWPOINT ) );
        nodenames.emplace( NODEITEM( "VisibilitySensor", WRL2NODES::WRL2_VISIBILITYSENSOR ) );
        nodenames.emplace( NODEITEM( "WorldInfo", WRL2NODES::WRL2_WORLDINFO ) );
    }
}


WRL2NODE::~WRL2NODE()
{
    if( m_Parent )
        m_Parent->unlinkChildNode( this );

    std::list< WRL2NODE* >::iterator sBP = m_BackPointers.begin();
    std::list< WRL2NODE* >::iterator eBP = m_BackPointers.end();

    while( sBP != eBP )
    {
        (*sBP)->unlinkRefNode( this );
        ++sBP;
    }

    std::list< WRL2NODE* >::iterator sC = m_Refs.begin();
    std::list< WRL2NODE* >::iterator eC = m_Refs.end();

    while( sC != eC )
    {
        (*sC)->delNodeRef( this );
        ++sC;
    }

    m_Refs.clear();
    sC = m_Children.begin();
    eC = m_Children.end();

    while( sC != eC )
    {
        (*sC)->SetParent( nullptr, false );
        delete *sC;
        ++sC;
    }

    m_Children.clear();
}


void WRL2NODE::addNodeRef( WRL2NODE* aNode )
{
    // the parent node must never be added as a backpointer
    if( aNode == m_Parent )
        return;

    std::list< WRL2NODE* >::iterator np =
        std::find( m_BackPointers.begin(), m_BackPointers.end(), aNode );

    if( np != m_BackPointers.end() )
        return;

    m_BackPointers.push_back( aNode );
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

    wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                      " * [BUG] delNodeRef() did not find its target." ),
                __FILE__, __FUNCTION__, __LINE__ );
}


WRL2NODES WRL2NODE::GetNodeType( void ) const
{
    return m_Type;
}


WRL2NODE* WRL2NODE::GetParent( void ) const
{
    return m_Parent;
}


std::string WRL2NODE::GetName( void )
{
    return m_Name;
}


bool WRL2NODE::SetName( const std::string& aName )
{
    if( aName.empty() )
        return false;

    std::set< std::string >::iterator item = badNames.find( aName );

    if( item != badNames.end() )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] invalid node name '%s' (matches restricted word)" ),
                    __FILE__, __FUNCTION__, __LINE__, *item );

        return false;
    }


    if( isdigit( aName[0] ) )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          " * [INFO] invalid node name '%s' (begins with digit)" ),
                    __FILE__, __FUNCTION__, __LINE__, aName );

        return false;
    }

    // The characters '+' and '-' are not allowed in names as per the VRML2 specification;
    // however many parsers accept them and many bad generators use them so the rules
    // have been relaxed here.
    // #define BAD_CHARS1 "\"\'#+,-.\\[]{}\x00\x01\x02\x03\x04\x05\x06\x09\x0A\x0B\x0C\x0D\x0E\x0F"
    #define BAD_CHARS1 "\"\'#,.\\[]{}\x00\x01\x02\x03\x04\x05\x06\x09\x0A\x0B\x0C\x0D\x0E\x0F"
    #define BAD_CHARS2 "\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"

    if( std::string::npos != aName.find_first_of( BAD_CHARS1 )
        || std::string::npos != aName.find_first_of( BAD_CHARS2 ) )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] invalid node name '%s' (contains invalid character)" ),
                    __FILE__, __FUNCTION__, __LINE__, aName );

        return false;
    }

    m_Name = aName;

    return true;
}


const char* WRL2NODE::GetNodeTypeName( WRL2NODES aNodeType ) const
{
    if( aNodeType < WRL2NODES::WRL2_BASE || aNodeType >= WRL2NODES::WRL2_END )
        return "*INVALID_TYPE*";

    if( aNodeType == WRL2NODES::WRL2_BASE )
        return "*VIRTUAL_BASE*";

    NODEMAP::iterator it = nodenames.begin();
    advance( it, (static_cast<int>( aNodeType ) - static_cast<int>( WRL2NODES::WRL2_BEGIN ) ) );

    return it->first.c_str();
}


WRL2NODES WRL2NODE::getNodeTypeID( const std::string& aNodeName )
{
    NODEMAP::iterator it = nodenames.find( aNodeName );

    if( nodenames.end() != it )
        return it->second;

    return WRL2NODES::WRL2_INVALID;
}


std::string WRL2NODE::GetError( void )
{
    return m_error;
}


WRL2NODE* WRL2NODE::FindNode( const std::string& aNodeName, const WRL2NODE *aCaller )
{
    if( aNodeName.empty() )
        return nullptr;

    if( !m_Name.compare( aNodeName ) )
        return this;

    std::list< WRL2NODE* >::iterator sLA = m_Children.begin();
    std::list< WRL2NODE* >::iterator eLA = m_Children.end();

    WRL2NODE* psg = nullptr;

    while( sLA != eLA )
    {
        if( *sLA != aCaller )
        {
            psg = (*sLA)->FindNode( aNodeName, this );

            if( nullptr != psg )
                return psg;

        }

        ++sLA;
    }

    if( nullptr != m_Parent && aCaller != m_Parent )
        return m_Parent->FindNode( aNodeName, this );

    return nullptr;
}


bool WRL2NODE::SetParent( WRL2NODE* aParent, bool doUnlink )
{
    if( aParent == m_Parent )
        return true;

    if( nullptr != m_Parent && doUnlink )
        m_Parent->unlinkChildNode( this );

    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


bool WRL2NODE::AddChildNode( WRL2NODE* aNode )
{
    wxCHECK_MSG( aNode, false, wxT( "Invalid node pointer." )  );
    wxCHECK_MSG( aNode->GetNodeType() != WRL2NODES::WRL2_BASE, false,
                 wxT( "Attempting to add a base node to another node." ) );


    std::list< WRL2NODE* >::iterator sC = m_Children.begin();
    std::list< WRL2NODE* >::iterator eC = m_Children.end();

    while( sC != eC )
    {
        if( *sC == aNode )
            return false;

        ++sC;
    }

    m_Children.push_back( aNode );

    if( aNode->GetParent() != this )
        aNode->SetParent( this );

    return true;
}


bool WRL2NODE::AddRefNode( WRL2NODE* aNode )
{
    wxCHECK_MSG( aNode, false, wxT( "Invalid node pointer." )  );
    wxCHECK_MSG( aNode->GetNodeType() != WRL2NODES::WRL2_BASE, false,
                 wxT( "Attempt to add a base node reference to another base node" ) );

    std::list< WRL2NODE* >::iterator sR = m_Refs.begin();
    std::list< WRL2NODE* >::iterator eR = m_Refs.end();

    while( sR != eR )
    {
        if( *sR == aNode )
            return true;

        ++sR;
    }

    m_Refs.push_back( aNode );
    aNode->addNodeRef( this );

    return true;
}


void WRL2NODE::unlinkChildNode( const WRL2NODE* aNode )
{
    std::list< WRL2NODE* >::iterator sL = m_Children.begin();
    std::list< WRL2NODE* >::iterator eL = m_Children.end();

    while( sL != eL )
    {
        if( *sL == aNode )
        {
            m_Children.erase( sL );
            return;
        }

        ++sL;
    }
}


void WRL2NODE::unlinkRefNode( const WRL2NODE* aNode )
{
    std::list< WRL2NODE* >::iterator sL = m_Refs.begin();
    std::list< WRL2NODE* >::iterator eL = m_Refs.end();

    while( sL != eL )
    {
        if( *sL == aNode )
        {
            m_Refs.erase( sL );
            return;
        }

        ++sL;
    }
}
