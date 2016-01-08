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

#include <iostream>

#include "vrml1_base.h"
#include "vrml1_group.h"
#include "vrml1_separator.h"
#include "vrml1_material.h"
#include "vrml1_matbinding.h"
#include "vrml1_coords.h"
#include "vrml1_switch.h"
#include "vrml1_faceset.h"
#include "vrml1_transform.h"
#include "vrml1_shapehints.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1BASE::WRL1BASE() : WRL1NODE( NULL )
{
    m_useInline = false;
    m_Type = WRL1_BASE;
    m_dictionary = new NAMEREGISTER;
    return;
}


WRL1BASE::~WRL1BASE()
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    std::cerr << " * [INFO] Destroying virtual base node\n";
    #endif

    cancelDict();
    return;
}


// functions inherited from WRL1NODE
bool WRL1BASE::SetParent( WRL1NODE* aParent )
{
    #ifdef DEBUG_VRML1
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] attempting to set parent on WRL1BASE node\n";
    #endif

    return false;
}


void WRL1BASE::SetEnableInline( bool enable )
{
    m_useInline = enable;
    return;
}


bool WRL1BASE::GetEnableInline( void )
{
    return  m_useInline;
}


SGNODE* WRL1BASE::AddInlineData( const std::string& aName, WRL1INLINE* aObject )
{
    std::map< std::string, WRL1INLINE* >::iterator dp = m_inlineModels.find( aName );
    // XXX;
    // qwerty;
    return NULL;
}


SGNODE* WRL1BASE::GetInlineData( const std::string& aName, WRL1INLINE* aObject )
{
    // XXX;
    // qwerty;
    return NULL;
}


void WRL1BASE::DelInlineData( const std::string& aName, WRL1INLINE* aObject )
{
    // XXX;
    // qwerty;
    return;
}


std::string WRL1BASE::GetName( void )
{
    #ifdef DEBUG_VRML1
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] attempting to extract name from virtual base node\n";
    #endif

    return std::string( "" );
}


bool WRL1BASE::SetName( const std::string& aName )
{
    #ifdef DEBUG_VRML1
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] attempting to set name on virtual base node\n";
    #endif
    return false;
}


bool WRL1BASE::Read( WRLPROC& proc )
{
    if( proc.GetVRMLType() != VRML_V1 )
    {
        #ifdef DEBUG_VRML1
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] no open file or file is not a VRML1 file\n";
        #endif
        return false;
    }

    std::string glob;
    WRL1NODES ntype;

    if( !proc.ReadName( glob ) )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        if( !proc.eof() )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
        }
        #endif

        return false;
    }

    // Process node name: only Separator, Switch and DEF are acceptable;
    // WWWAnchor and LOD will not be supported
    if( !glob.compare( "DEF" ) )
    {
        // read the name and discard it; we must not add it to the dictionary
        // because that invites the possibility of a circular reference

        for( int i = 0; i < 2; ++i )
        {
            if( !proc.ReadName( glob ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << proc.GetError() <<  "\n";
                #endif

                return false;
            }
        }
    }

    ntype = getNodeTypeID( glob );
    size_t line = 0;
    size_t column = 0;
    proc.GetFilePosData( line, column );
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    std::cerr << " * [INFO] Processing node '" << glob << "' ID: " << ntype << "\n";
    #endif

    if( ntype != WRL1_SEPARATOR && ntype != WRL1_SWITCH && ntype != WRL1_GROUP )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file - top node is not a Separator, Switch, or Group\n";
        #endif

        return false;
    }

    switch( ntype )
    {
    case WRL1_SEPARATOR:

        if( !readSeparator( proc, this, NULL ) )
            return false;

        break;

    case WRL1_GROUP:

        if( !readGroup( proc, this, NULL ) )
            return false;

        break;

    default:

        if( !readSwitch( proc, this, NULL ) )
            return false;
    }

    return true;
}


bool WRL1BASE::implementUse( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    if( !aParent )
    {
        #ifdef DEBUG_VRML1
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invoked with NULL parent\n";
        #endif

        return false;
    }

    std::string glob;

    if( !proc.ReadName( glob ) )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << proc.GetError() <<  "\n";
        #endif

        return false;
    }

    WRL1NODE* ref = aParent->FindNode( glob );

    // return 'true' - the file may be defective but it may still be somewhat OK
    if( NULL == ref )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] node '" << glob << "' not found\n";
        #endif

        return true;
    }

    if( !aParent->AddRefNode( ref ) )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] failed to add node '" << glob << "' (";
        std::cerr << ref->GetNodeTypeName( ref->GetNodeType() ) << ") to parent of type ";
        std::cerr << aParent->GetNodeTypeName( aParent->GetNodeType() ) << "\n";
        #endif

        return false;
    }

    if( NULL != aNode )
        *aNode = ref;

    return true;
}


bool WRL1BASE::implementDef( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    if( NULL == aParent )
    {
        #ifdef DEBUG_VRML1
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invalid parent pointer (NULL)\n";
        #endif

        return false;
    }

    std::string glob;
    WRL1NODE* lnode = NULL;

    if( !proc.ReadName( glob ) )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << proc.GetError() <<  "\n";
        #endif

        return false;
    }

    size_t line, column;
    proc.GetFilePosData( line, column );

    if( ReadNode( proc, aParent, &lnode ) )
    {
        if( NULL != aNode )
            *aNode = lnode;

        if( lnode && !lnode->SetName( glob ) )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            size_t line, column;
            proc.GetFilePosData( line, column );
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad formatting (invalid name) at line";
            std::cerr << line << ", column " << column << "\n";
            #endif

            return false;
        }

        if( !m_dictionary )
            return false;

        m_dictionary->AddName( glob, lnode );

        return true;
    }

    return false;
}


bool WRL1BASE::ReadNode( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    // This function reads a node and stores a pointer to it in aNode.
    // A value 'true' is returned if a node is successfully read or,
    // if the node is not supported, successfully discarded. Callers
    // must always check the value of aNode when the function returns
    // 'true' since it will be NULL if the node type is not supported.

    if( NULL != aNode )
        *aNode = NULL;

    if( NULL == aParent )
    {
        #ifdef DEBUG_VRML1
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invalid parent pointer (NULL)\n";
        #endif

        return false;
    }

    std::string glob;
    WRL1NODES ntype;

    if( !proc.ReadName( glob ) )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        if( !proc.eof() )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
        }
        #endif

        return false;
    }

    // Process node name:
    // the names encountered at this point should be one of the
    // built-in node names or one of:
    // DEF, USE
    if( !glob.compare( "USE" ) )
    {
        if( !implementUse( proc, aParent, aNode ) )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

            return false;
        }

        return true;
    }

    if( !glob.compare( "DEF" ) )
    {
        if( !implementDef( proc, aParent, aNode ) )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

            return false;
        }

        return true;
    }

    ntype = getNodeTypeID( glob );
    size_t line = 0;
    size_t column = 0;
    proc.GetFilePosData( line, column );
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    std::cerr << " * [INFO] Processing node '" << glob << "' ID: " << ntype << "\n";
    #endif

    switch( ntype )
    {
    case WRL1_GROUP:

        if( !readGroup( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_SEPARATOR:

        if( !readSeparator( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_SWITCH:

        if( !readSwitch( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_MATERIAL:

        if( !readMaterial( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_MATERIALBINDING:

        if( !readMatBinding( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_COORDINATE3:

        if( !readCoords( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_INDEXEDFACESET:

        if( !readFaceSet( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_TRANSFORM:
    case WRL1_TRANSLATION:
    case WRL1_ROTATION:
    case WRL1_SCALE:

        if( !readTransform( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_SHAPEHINTS:

        if( !readShapeHints( proc, aParent, aNode ) )
            return false;

        break;

    //
    // items not implemented or for optional future implementation:
    //
    default:

        proc.GetFilePosData( line, column );

        if( !proc.DiscardNode() )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            std::cerr << proc.GetError() << "\n";
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] could not discard node at line " << line;
            std::cerr << ", column " << column << "\n";
            #endif

            return false;
        }
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        else
        {
            std::cerr << " * [INFO] discarded node '" << glob << "' at line ";
            std::cerr << line << ", col " << column << " (currently unsupported)\n";
        }
        #endif

        break;
    }

    return true;
}


bool WRL1BASE::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    // this function makes no sense in the base node
    #ifdef DEBUG_VRML1
    std::cerr << proc.GetError() << "\n";
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] this method must never be invoked on a WRL1BASE object\n";
    #endif

    return false;
}


bool WRL1BASE::readGroup( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1GROUP* np = new WRL1GROUP( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readSeparator( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1SEPARATOR* np = new WRL1SEPARATOR( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readSwitch( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    WRL1SWITCH* np = new WRL1SWITCH( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readMaterial( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1MATERIAL* np = new WRL1MATERIAL( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readMatBinding( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1MATBINDING* np = new WRL1MATBINDING( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readCoords( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1COORDS* np = new WRL1COORDS( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readFaceSet( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1FACESET* np = new WRL1FACESET( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readTransform( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1TRANSFORM* np = new WRL1TRANSFORM( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readShapeHints( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1SHAPEHINTS* np = new WRL1SHAPEHINTS( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


SGNODE* WRL1BASE::TranslateToSG( SGNODE* aParent, WRL1STATUS* /*sp*/ )
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    std::cerr << " * [INFO] Translating VRML1 Base with " << m_Items.size();
    std::cerr << " items\n";
    #endif

    if( m_Items.empty() )
        return NULL;

    if( m_Items.size() != 1 )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        std::cerr << " * [INFO] Bad VRML file, >1 top level transform (";
        std::cerr << m_Items.size() << ")\n";
        #endif

        return NULL;
    }

    return (*m_Items.begin())->TranslateToSG( NULL, NULL );
}
