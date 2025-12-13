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

#include <iostream>
#include <sstream>
#include <wx/log.h>

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


WRL1BASE::WRL1BASE() : WRL1NODE( nullptr )
{
    m_Type = WRL1NODES::WRL1_BASE;
    m_dictionary = new NAMEREGISTER;
    return;
}


WRL1BASE::~WRL1BASE()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying virtual base node." ) );

    cancelDict();
}


bool WRL1BASE::SetParent( WRL1NODE* aParent, bool /* doUnlink */ )
{
    wxCHECK_MSG( false, false, wxT( "Attempt to set parent on WRL1BASE node." ) );
}


std::string WRL1BASE::GetName( void )
{
    wxCHECK_MSG( false, std::string( "" ),
                 wxT( "Attempt to extract name from virtual base node." ) );
}


bool WRL1BASE::SetName( const std::string& aName )
{
    wxCHECK_MSG( false, false, wxT( "Attempt to set name on virtual base node." ) );
}


bool WRL1BASE::Read( WRLPROC& proc )
{
    wxCHECK_MSG( proc.GetVRMLType() == WRLVERSION::VRML_V1, false,
                 wxT( "No open file or file is not a VRML1 file" ) );

    // Note: according to the VRML1 specification, a file may contain
    // only one grouping node at the top level. The following code
    // supports non-conformant VRML1 files by processing all top level
    // nodes as if the vrml1_base were the equivalent of a vrml1_separator

    while( proc.Peek() )
    {
        if( !ReadNode( proc, this, nullptr ) )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n * [INFO] bad file format; unexpected eof %s" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

            return false;
        }
    }

    if( !proc.eof() )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n%s" ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetError() );

        return false;
    }

    return true;
}


bool WRL1BASE::implementUse( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    wxCHECK_MSG( aParent, false, wxT( "Invoked with invalid parent." ) );

    std::string glob;

    if( !proc.ReadName( glob ) )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s" ), proc.GetError() );

        return false;
    }

    WRL1NODE* ref = aParent->FindNode( glob );

    // return 'true' - the file may be defective but it may still be somewhat OK
    if( nullptr == ref )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n * [INFO] node '%s' not found." ),
                    __FILE__, __FUNCTION__, __LINE__, glob );

        return true;
    }

    if( !aParent->AddRefNode( ref ) )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n * [INFO] failed to add node '%s' (%s) to parent of type %s." ),
                    __FILE__, __FUNCTION__, __LINE__, glob,
                    ref->GetNodeTypeName( ref->GetNodeType() ),
                    aParent->GetNodeTypeName( aParent->GetNodeType() ) );

        return false;
    }

    if( nullptr != aNode )
        *aNode = ref;

    return true;
}


bool WRL1BASE::implementDef( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    wxCHECK_MSG( nullptr != aParent, false, wxT( "Invalid parent pointer." ) );

    std::string glob;
    WRL1NODE* lnode = nullptr;

    if( !proc.ReadName( glob ) )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n%s" ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetError() );

        return false;
    }

    if( ReadNode( proc, aParent, &lnode ) )
    {
        if( nullptr != aNode )
            *aNode = lnode;

        if( lnode && !lnode->SetName( glob ) )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n * [INFO] bad formatting (invalid name) %s." ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

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

    if( nullptr != aNode )
        *aNode = nullptr;

    wxCHECK_MSG( aParent, false, wxT( "Invalid parent pointer." ) );

    std::string glob;
    WRL1NODES ntype;

    if( !proc.ReadName( glob ) )
    {
        if( !proc.eof() )
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n%s" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetError() );
        }

        return false;
    }

    // Process node name:
    // the names encountered at this point should be one of the
    // built-in node names or one of:
    // DEF, USE
    if( !glob.compare( "USE" ) )
    {
        if( !implementUse( proc, aParent, aNode ) )
            return false;

        return true;
    }

    if( !glob.compare( "DEF" ) )
    {
        if( !implementDef( proc, aParent, aNode ) )
            return false;

        return true;
    }

    ntype = getNodeTypeID( glob );

    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Processing node '%s' ID: %d." ), glob, ntype );

    switch( ntype )
    {
    case WRL1NODES::WRL1_GROUP:
        if( !readGroup( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1NODES::WRL1_SEPARATOR:
        if( !readSeparator( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1NODES::WRL1_SWITCH:
        if( !readSwitch( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1NODES::WRL1_MATERIAL:
        if( !readMaterial( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1NODES::WRL1_MATERIALBINDING:
        if( !readMatBinding( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1NODES::WRL1_COORDINATE3:
        if( !readCoords( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1NODES::WRL1_INDEXEDFACESET:
        if( !readFaceSet( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1NODES::WRL1_TRANSFORM:
    case WRL1NODES::WRL1_TRANSLATION:
    case WRL1NODES::WRL1_ROTATION:
    case WRL1NODES::WRL1_SCALE:
        if( !readTransform( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1NODES::WRL1_SHAPEHINTS:
        if( !readShapeHints( proc, aParent, aNode ) )
            return false;

        break;

    //
    // items not implemented or for optional future implementation:
    //
    default:
        if( !proc.DiscardNode() )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n * [INFO] could not discard node %s." ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

            return false;
        }
        else
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( " * [INFO] discarded node '%s' %s (currently unsupported)." ),
                        glob, proc.GetFilePosition() );
        }

        break;
    }

    return true;
}


bool WRL1BASE::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    wxCHECK_MSG( false, false, wxT( "This method must never be invoked on a WRL1BASE object" ) );
}


bool WRL1BASE::readGroup( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL1GROUP* np = new WRL1GROUP( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readSeparator( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL1SEPARATOR* np = new WRL1SEPARATOR( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
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

    if( nullptr != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readMaterial( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL1MATERIAL* np = new WRL1MATERIAL( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readMatBinding( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL1MATBINDING* np = new WRL1MATBINDING( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readCoords( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL1COORDS* np = new WRL1COORDS( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readFaceSet( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL1FACESET* np = new WRL1FACESET( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readTransform( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL1TRANSFORM* np = new WRL1TRANSFORM( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readShapeHints( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL1SHAPEHINTS* np = new WRL1SHAPEHINTS( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


SGNODE* WRL1BASE::TranslateToSG( SGNODE* aParent, WRL1STATUS* /*sp*/ )
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Translating VRML1 Base with %zu items." ),
                m_Items.size() );

    if( m_Items.empty() )
        return nullptr;

    if( m_Items.size() == 1 )
        return ( *m_Items.begin() )->TranslateToSG( nullptr, nullptr );

    // Note: according to the VRML1 specification, a file may contain
    // only one grouping node at the top level. The following code
    // supports non-conformant VRML1 files.

    m_current.Init();

    IFSG_TRANSFORM txNode( true );
    bool hasContent = false;

    std::list< WRL1NODE* >::iterator sI = m_Items.begin();
    std::list< WRL1NODE* >::iterator eI = m_Items.end();

    SGNODE* node = txNode.GetRawPtr();

    while( sI != eI )
    {
        if( nullptr != (*sI)->TranslateToSG( node, &m_current ) )
            hasContent = true;

        ++sI;
    }

    if( !hasContent )
    {
        txNode.Destroy();
        return nullptr;
    }

    return node;
}
