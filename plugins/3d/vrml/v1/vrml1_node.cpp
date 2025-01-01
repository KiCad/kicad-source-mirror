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
#include <algorithm>
#include <sstream>
#include <wx/log.h>

#include "vrml1_node.h"


bool NAMEREGISTER::AddName( const std::string& aName, WRL1NODE* aNode )
{
    if( aName.empty() )
        return false;

    std::map< std::string, WRL1NODE* >::iterator ir = reg.find( aName );

    if( ir != reg.end() )
        reg.erase( ir );

    reg.emplace( aName, aNode );

    return true;
}


bool NAMEREGISTER::DelName( const std::string& aName, WRL1NODE* aNode )
{
    if( aName.empty() )
        return false;

    std::map< std::string, WRL1NODE* >::iterator ir = reg.find( aName );

    if( ir != reg.end() && ir->second == aNode )
    {
        reg.erase( ir );
        return true;
    }

    return false;
}


WRL1NODE* NAMEREGISTER::FindName( const std::string& aName )
{
    if( aName.empty() )
        return nullptr;

    std::map< std::string, WRL1NODE* >::iterator ir = reg.find( aName );

    if( ir != reg.end() )
        return ir->second;

    return nullptr;
}


typedef std::pair< std::string, WRL1NODES > NODEITEM;
typedef std::map< std::string, WRL1NODES > NODEMAP;
static NODEMAP nodenames;


WRL1NODE::WRL1NODE( NAMEREGISTER* aDictionary )
{
    m_sgNode = nullptr;
    m_Parent = nullptr;
    m_Type = WRL1NODES::WRL1_END;
    m_dictionary = aDictionary;

    if( nodenames.empty() )
    {
        nodenames.emplace( NODEITEM( "AsciiText", WRL1NODES::WRL1_ASCIITEXT ) );
        nodenames.emplace( NODEITEM( "Cone", WRL1NODES::WRL1_CONE ) );
        nodenames.emplace( NODEITEM( "Coordinate3", WRL1NODES::WRL1_COORDINATE3 ) );
        nodenames.emplace( NODEITEM( "Cube", WRL1NODES::WRL1_CUBE ) );
        nodenames.emplace( NODEITEM( "Cylinder", WRL1NODES::WRL1_CYLINDER ) );
        nodenames.emplace( NODEITEM( "DirectionalLight", WRL1NODES::WRL1_DIRECTIONALLIGHT ) );
        nodenames.emplace( NODEITEM( "FontStyle", WRL1NODES::WRL1_FONTSTYLE ) );
        nodenames.emplace( NODEITEM( "Group", WRL1NODES::WRL1_GROUP ) );
        nodenames.emplace( NODEITEM( "IndexedFaceSet", WRL1NODES::WRL1_INDEXEDFACESET ) );
        nodenames.emplace( NODEITEM( "IndexedLineSet", WRL1NODES::WRL1_INDEXEDLINESET ) );
        nodenames.emplace( NODEITEM( "Info", WRL1NODES::WRL1_INFO ) );
        nodenames.emplace( NODEITEM( "LOD", WRL1NODES::WRL1_LOD ) );
        nodenames.emplace( NODEITEM( "Material", WRL1NODES::WRL1_MATERIAL ) );
        nodenames.emplace( NODEITEM( "MaterialBinding", WRL1NODES::WRL1_MATERIALBINDING ) );
        nodenames.emplace( NODEITEM( "MatrixTransform", WRL1NODES::WRL1_MATRIXTRANSFORM ) );
        nodenames.emplace( NODEITEM( "Normal", WRL1NODES::WRL1_NORMAL ) );
        nodenames.emplace( NODEITEM( "NormalBinding", WRL1NODES::WRL1_NORMALBINDING ) );
        nodenames.emplace( NODEITEM( "OrthographicCamera", WRL1NODES::WRL1_ORTHOCAMERA ) );
        nodenames.emplace( NODEITEM( "PerspectiveCamera", WRL1NODES::WRL1_PERSPECTIVECAMERA ) );
        nodenames.emplace( NODEITEM( "PointLight", WRL1NODES::WRL1_POINTLIGHT ) );
        nodenames.emplace( NODEITEM( "PointSet", WRL1NODES::WRL1_POINTSET ) );
        nodenames.emplace( NODEITEM( "Rotation", WRL1NODES::WRL1_ROTATION ) );
        nodenames.emplace( NODEITEM( "Scale", WRL1NODES::WRL1_SCALE ) );
        nodenames.emplace( NODEITEM( "Separator", WRL1NODES::WRL1_SEPARATOR ) );
        nodenames.emplace( NODEITEM( "ShapeHints", WRL1NODES::WRL1_SHAPEHINTS ) );
        nodenames.emplace( NODEITEM( "Sphere", WRL1NODES::WRL1_SPHERE ) );
        nodenames.emplace( NODEITEM( "SpotLight", WRL1NODES::WRL1_SPOTLIGHT ) );
        nodenames.emplace( NODEITEM( "Switch", WRL1NODES::WRL1_SWITCH ) );
        nodenames.emplace( NODEITEM( "Texture2", WRL1NODES::WRL1_TEXTURE2 ) );
        nodenames.emplace( NODEITEM( "Testure2Transform", WRL1NODES::WRL1_TEXTURE2TRANSFORM ) );
        nodenames.emplace( NODEITEM( "TextureCoordinate2", WRL1NODES::WRL1_TEXTURECOORDINATE2 ) );
        nodenames.emplace( NODEITEM( "Transform", WRL1NODES::WRL1_TRANSFORM ) );
        nodenames.emplace( NODEITEM( "Translation", WRL1NODES::WRL1_TRANSLATION ) );
        nodenames.emplace( NODEITEM( "WWWAnchor", WRL1NODES::WRL1_WWWANCHOR ) );
        nodenames.emplace( NODEITEM( "WWWInline", WRL1NODES::WRL1_WWWINLINE ) );
    }
}


WRL1NODE::~WRL1NODE()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] ^^ Destroying Type %d with %lu children, %lu references, and %lu "
                     "back pointers." ),
                m_Type, m_Children.size(), m_Refs.size(), m_BackPointers.size() );

    m_Items.clear();

    if( m_dictionary && !m_Name.empty() )
        m_dictionary->DelName( m_Name, this );

    if( m_Parent )
        m_Parent->unlinkChildNode( this );

    std::list< WRL1NODE* >::iterator sBP = m_BackPointers.begin();
    std::list< WRL1NODE* >::iterator eBP = m_BackPointers.end();

    while( sBP != eBP )
    {
        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO]%sType %d is unlinking ref #%d" ),
                    wxString( ' ', (size_t) std::distance( sBP, m_BackPointers.begin() ) * 2 ),
                    m_Type, std::distance( sBP, m_BackPointers.begin() ) );

        (*sBP)->unlinkRefNode( this );

        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO]%sType %d has unlinked ref #%d" ),
                    wxString( ' ', (size_t) std::distance( sBP, m_BackPointers.begin() ) * 2 ),
                    m_Type, std::distance( sBP, m_BackPointers.begin() ) );

        ++sBP;
    }

    m_Refs.clear();

    std::list< WRL1NODE* >::iterator sC = m_Children.begin();
    std::list< WRL1NODE* >::iterator eC = m_Children.end();

    while( sC != eC )
    {
        (*sC)->SetParent( nullptr, false );

        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO]%sType %d has unlinked child  #%d" ),
                    wxString( ' ', (size_t) std::distance( sC, m_Children.begin() ) * 2 ),
                    m_Type, std::distance( sC, m_Children.begin() ) );

        delete *sC;

        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO]%sType %d has deleted child  #%d" ),
                    wxString( ' ', (size_t) std::distance( sC, m_Children.begin() ) * 2 ),
                    m_Type, std::distance( sC, m_Children.begin() ) );

        ++sC;
    }

    m_Children.clear();
}


void WRL1NODE::cancelDict( void )
{
    std::list< WRL1NODE* >::iterator sC = m_Children.begin();
    std::list< WRL1NODE* >::iterator eC = m_Children.end();

    while( sC != eC )
    {
        (*sC)->cancelDict();
        ++sC;
    }

    if( m_Type == WRL1NODES::WRL1_BASE && nullptr != m_dictionary )
        delete m_dictionary;

    m_dictionary = nullptr;
}


void WRL1NODE::addNodeRef( WRL1NODE* aNode )
{
    // the parent node must never be added as a backpointer
    if( aNode == m_Parent )
        return;

    std::list< WRL1NODE* >::iterator sR = m_BackPointers.begin();
    std::list< WRL1NODE* >::iterator eR = m_BackPointers.end();

    while( sR != eR )
    {
        if( *sR == aNode )
            return;

        ++sR;
    }

    m_BackPointers.push_back( aNode );
}


void WRL1NODE::delNodeRef( WRL1NODE* aNode )
{
    std::list< WRL1NODE* >::iterator np = std::find( m_BackPointers.begin(),
                                                     m_BackPointers.end(), aNode );

    if( np != m_BackPointers.end() )
    {
        m_BackPointers.erase( np );
        return;
    }

    wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                      " * [BUG] delNodeRef() did not find its target." ),
                __FILE__, __FUNCTION__, __LINE__ );
}


WRL1NODES WRL1NODE::GetNodeType( void ) const
{
    return m_Type;
}


WRL1NODE* WRL1NODE::GetParent( void ) const
{
    return m_Parent;
}


std::string WRL1NODE::GetName( void )
{
    return m_Name;
}


bool WRL1NODE::SetName( const std::string& aName )
{
    if( aName.empty() )
        return false;

    if( isdigit( aName[0] ) )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          " * [INFO] invalid node name '%s' (begins with digit)" ),
                    __FILE__, __FUNCTION__, __LINE__, aName );

        return false;
    }

    // The character '+' is not allowed in names as per the VRML1 specification;
    // however many parsers accept them and many bad generators use them so the rules
    // have been relaxed here.
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

    if( m_dictionary )
        m_dictionary->AddName( aName, this );

    return true;
}


const char* WRL1NODE::GetNodeTypeName( WRL1NODES aNodeType ) const
{
    if( aNodeType < WRL1NODES::WRL1_BASE || aNodeType >= WRL1NODES::WRL1_END )
        return "*INVALID_TYPE*";

    if( aNodeType == WRL1NODES::WRL1_BASE )
        return "*VIRTUAL_BASE*";

    NODEMAP::iterator it = nodenames.begin();
    advance( it, ( static_cast<int>( aNodeType  ) - static_cast<int>( WRL1NODES::WRL1_BEGIN ) ) );

    return it->first.c_str();
}


WRL1NODES WRL1NODE::getNodeTypeID( const std::string& aNodeName )
{
    NODEMAP::iterator it = nodenames.find( aNodeName );

    if( nodenames.end() != it )
        return it->second;

    return WRL1NODES::WRL1_INVALID;
}


size_t WRL1NODE::GetNItems( void ) const
{
    return m_Items.size();
}


std::string WRL1NODE::GetError( void )
{
    return m_error;
}


WRL1NODE* WRL1NODE::FindNode( const std::string& aNodeName )
{
    if( nullptr == m_dictionary )
        return nullptr;

    return m_dictionary->FindName( aNodeName );
}


bool WRL1NODE::SetParent( WRL1NODE* aParent, bool doUnlink )
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


bool WRL1NODE::AddChildNode( WRL1NODE* aNode )
{
    wxCHECK_MSG( aNode, false, wxT( "Invalid node pointer." )  );
    wxCHECK_MSG( aNode->GetNodeType() != WRL1NODES::WRL1_BASE, false,
                 wxT( "Attempting to add a base node to another node." ) );

    std::list< WRL1NODE* >::iterator sC = m_Children.begin();
    std::list< WRL1NODE* >::iterator eC = m_Children.end();

    while( sC != eC )
    {
        if( *sC == aNode )
            return false;

        ++sC;
    }

    m_Children.push_back( aNode );
    addItem( aNode );

    if( aNode->GetParent() != this )
        aNode->SetParent( this );

    return true;
}


bool WRL1NODE::AddRefNode( WRL1NODE* aNode )
{
    wxCHECK_MSG( aNode, false, wxT( "Invalid node pointer." )  );
    wxCHECK_MSG( aNode->GetNodeType() != WRL1NODES::WRL1_BASE, false,
                 wxT( "Attempt to add a base node reference to another base node" ) );

    // note: the VRML1 spec does not prevent the reuse of a node at
    // the same level; for example a Coordinate3 node can be recalled
    // at any time to set the current coordinate set.
    m_Refs.push_back( aNode );
    aNode->addNodeRef( this );
    addItem( aNode );

    return true;
}


void WRL1NODE::unlinkChildNode( const WRL1NODE* aNode )
{
    std::list< WRL1NODE* >::iterator sL = m_Children.begin();
    std::list< WRL1NODE* >::iterator eL = m_Children.end();

    while( sL != eL )
    {
        if( *sL == aNode )
        {
            m_Children.erase( sL );
            delItem( aNode );
            return;
        }

        ++sL;
    }
}


void WRL1NODE::unlinkRefNode( const WRL1NODE* aNode )
{
    std::list< WRL1NODE* >::iterator sL = m_Refs.begin();
    std::list< WRL1NODE* >::iterator eL = m_Refs.end();

    while( sL != eL )
    {
        if( *sL == aNode )
        {
            m_Refs.erase( sL );
            delItem( aNode );
            return;
        }

        ++sL;
    }
}


void WRL1NODE::addItem( WRL1NODE* aNode )
{
    m_Items.push_back( aNode );
}


void WRL1NODE::delItem( const WRL1NODE* aNode )
{
    std::list< WRL1NODE* >::iterator sL = m_Items.begin();
    std::list< WRL1NODE* >::iterator eL = m_Items.end();

    while( sL != eL )
    {
        if( *sL == aNode )
        {
            m_Items.erase( sL );
            return;
        }

        ++sL;
    }
}
