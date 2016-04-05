/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

    reg.insert( std::pair< std::string, WRL1NODE* >( aName, aNode ) );

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
        return NULL;

    std::map< std::string, WRL1NODE* >::iterator ir = reg.find( aName );

    if( ir != reg.end() )
        return ir->second;

    return NULL;
}


typedef std::pair< std::string, WRL1NODES > NODEITEM;
typedef std::map< std::string, WRL1NODES > NODEMAP;
static NODEMAP nodenames;

#if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
std::string WRL1NODE::tabs = "";
#endif

WRL1NODE::WRL1NODE( NAMEREGISTER* aDictionary )
{
    m_sgNode = NULL;
    m_Parent = NULL;
    m_Type = WRL1_END;
    m_dictionary = aDictionary;

    if( nodenames.empty() )
    {
        nodenames.insert( NODEITEM( "AsciiText", WRL1_ASCIITEXT ) );
        nodenames.insert( NODEITEM( "Cone", WRL1_CONE ) );
        nodenames.insert( NODEITEM( "Coordinate3", WRL1_COORDINATE3 ) );
        nodenames.insert( NODEITEM( "Cube", WRL1_CUBE ) );
        nodenames.insert( NODEITEM( "Cylinder", WRL1_CYLINDER ) );
        nodenames.insert( NODEITEM( "DirectionalLight", WRL1_DIRECTIONALLIGHT ) );
        nodenames.insert( NODEITEM( "FontStyle", WRL1_FONTSTYLE ) );
        nodenames.insert( NODEITEM( "Group", WRL1_GROUP ) );
        nodenames.insert( NODEITEM( "IndexedFaceSet", WRL1_INDEXEDFACESET ) );
        nodenames.insert( NODEITEM( "IndexedLineSet", WRL1_INDEXEDLINESET ) );
        nodenames.insert( NODEITEM( "Info", WRL1_INFO ) );
        nodenames.insert( NODEITEM( "LOD", WRL1_LOD ) );
        nodenames.insert( NODEITEM( "Material", WRL1_MATERIAL ) );
        nodenames.insert( NODEITEM( "MaterialBinding", WRL1_MATERIALBINDING ) );
        nodenames.insert( NODEITEM( "MatrixTransform", WRL1_MATRIXTRANSFORM ) );
        nodenames.insert( NODEITEM( "Normal", WRL1_NORMAL ) );
        nodenames.insert( NODEITEM( "NormalBinding", WRL1_NORMALBINDING ) );
        nodenames.insert( NODEITEM( "OrthographicCamera", WRL1_ORTHOCAMERA ) );
        nodenames.insert( NODEITEM( "PerspectiveCamera", WRL1_PERSPECTIVECAMERA ) );
        nodenames.insert( NODEITEM( "PointLight", WRL1_POINTLIGHT ) );
        nodenames.insert( NODEITEM( "PointSet", WRL1_POINTSET ) );
        nodenames.insert( NODEITEM( "Rotation", WRL1_ROTATION ) );
        nodenames.insert( NODEITEM( "Scale", WRL1_SCALE ) );
        nodenames.insert( NODEITEM( "Separator", WRL1_SEPARATOR ) );
        nodenames.insert( NODEITEM( "ShapeHints", WRL1_SHAPEHINTS ) );
        nodenames.insert( NODEITEM( "Sphere", WRL1_SPHERE ) );
        nodenames.insert( NODEITEM( "SpotLight", WRL1_SPOTLIGHT ) );
        nodenames.insert( NODEITEM( "Switch", WRL1_SWITCH ) );
        nodenames.insert( NODEITEM( "Texture2", WRL1_TEXTURE2 ) );
        nodenames.insert( NODEITEM( "Testure2Transform", WRL1_TEXTURE2TRANSFORM ) );
        nodenames.insert( NODEITEM( "TextureCoordinate2", WRL1_TEXTURECOORDINATE2 ) );
        nodenames.insert( NODEITEM( "Transform", WRL1_TRANSFORM ) );
        nodenames.insert( NODEITEM( "Translation", WRL1_TRANSLATION ) );
        nodenames.insert( NODEITEM( "WWWAnchor", WRL1_WWWANCHOR ) );
        nodenames.insert( NODEITEM( "WWWInline", WRL1_WWWINLINE ) );
    }

    return;
}


WRL1NODE::~WRL1NODE()
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    do {
        std::ostringstream ostr;
        ostr << " * [INFO] ^^ Destroying Type " << m_Type << " with " << m_Children.size();
        ostr << " children, " << m_Refs.size() << " references and ";
        ostr << m_BackPointers.size() << " backpointers";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    m_Items.clear();

    if( m_dictionary && !m_Name.empty() )
        m_dictionary->DelName( m_Name, this );

    if( m_Parent )
        m_Parent->unlinkChildNode( this );

    std::list< WRL1NODE* >::iterator sBP = m_BackPointers.begin();
    std::list< WRL1NODE* >::iterator eBP = m_BackPointers.end();

    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    int acc = 0;
    #endif

    while( sBP != eBP )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
        ++acc;
        do {
            std::ostringstream ostr;
            ostr << " * [INFO] " << tabs << "Type " << m_Type << " is Unlinking ref #";
            ostr << acc;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif
        (*sBP)->unlinkRefNode( this );
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
        do {
            std::ostringstream ostr;
            ostr << " * [INFO] " << tabs << "Type " << m_Type << " has unlinked ref #";
            ostr << acc;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif
        ++sBP;
    }

    m_Refs.clear();

    std::list< WRL1NODE* >::iterator sC = m_Children.begin();
    std::list< WRL1NODE* >::iterator eC = m_Children.end();

    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    std::string otabs = tabs;
    tabs.append( "    " );
    #endif

    while( sC != eC )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
        ++acc;
        do {
            std::ostringstream ostr;
            ostr << " * [INFO] " << otabs << "Type " << m_Type << " is Deleting child #";
            ostr << acc;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif
        (*sC)->SetParent( NULL, false );
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
        do {
            std::ostringstream ostr;
            ostr << " * [INFO] " << otabs << "Type " << m_Type << " has unlinked child #";
            ostr << acc;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif
        delete *sC;
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
        do {
            std::ostringstream ostr;
            ostr << " * [INFO] " << otabs << "Type " << m_Type << " has deleted child #";
            ostr << acc;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif
        ++sC;
    }

    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    tabs = otabs;
    #endif

    m_Children.clear();
    return;
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

    if( m_Type == WRL1_BASE && NULL != m_dictionary )
        delete m_dictionary;

    m_dictionary = NULL;
    return;
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

    return;
}


void WRL1NODE::delNodeRef( WRL1NODE* aNode )
{
    std::list< WRL1NODE* >::iterator np =
    std::find( m_BackPointers.begin(), m_BackPointers.end(), aNode );

    if( np != m_BackPointers.end() )
    {
        m_BackPointers.erase( np );
        return;
    }

    #ifdef DEBUG_VRML1
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] delNodeRef() did not find its target";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return;
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
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] invalid node name '" << aName << "' (begins with digit)";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

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
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] invalid node name '" << aName;
            ostr << "' (contains invalid character)";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    m_Name = aName;

    if( m_dictionary )
        m_dictionary->AddName( aName, this );

    return true;
}


const char* WRL1NODE::GetNodeTypeName( WRL1NODES aNodeType ) const
{
    if( aNodeType < WRL1_BASE || aNodeType >= WRL1_END )
        return "*INVALID_TYPE*";

    if( aNodeType == WRL1_BASE )
        return "*VIRTUAL_BASE*";

    NODEMAP::iterator it = nodenames.begin();
    advance( it, (aNodeType - WRL1_BEGIN) );

    return it->first.c_str();
}


WRL1NODES WRL1NODE::getNodeTypeID( const std::string& aNodeName )
{
    NODEMAP::iterator it = nodenames.find( aNodeName );

    if( nodenames.end() != it )
        return it->second;

    return WRL1_INVALID;
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
    if( NULL == m_dictionary )
        return NULL;

    return m_dictionary->FindName( aNodeName );
}


bool WRL1NODE::SetParent( WRL1NODE* aParent, bool doUnlink )
{
    if( aParent == m_Parent )
        return true;

    if( NULL != m_Parent && doUnlink )
        m_Parent->unlinkChildNode( this );

    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


bool WRL1NODE::AddChildNode( WRL1NODE* aNode )
{
    if( aNode->GetNodeType() == WRL1_BASE )
    {
        #ifdef DEBUG_VRML1
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] attempting to add a base node to another node";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

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
    if( NULL == aNode )
    {
        #ifdef DEBUG_VRML1
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] NULL passed as node pointer";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( aNode->GetNodeType() == WRL1_BASE )
    {
        #ifdef DEBUG_VRML1
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] attempting to add a base node ref to another base node";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

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

    return;
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

    return;
}


void WRL1NODE::addItem( WRL1NODE* aNode )
{
    m_Items.push_back( aNode );
    return;
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

    return;
}
