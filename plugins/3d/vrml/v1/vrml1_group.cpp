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

// note: this was copied from the vrml1_separator class. the difference
// between a separator and a group is that a group propagates its
// current settings to its parent. While it would be possible to
// implement the separator as a derived class, it is easy enough to
// simply duplicate the code

#include <iostream>
#include <sstream>
#include <wx/log.h>

#include "vrml1_base.h"
#include "vrml1_group.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1GROUP::WRL1GROUP( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_GROUP;
}


WRL1GROUP::WRL1GROUP( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_GROUP;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL1GROUP::~WRL1GROUP()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying Group with %zu children, %zu references, and %zu "
                     "back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


bool WRL1GROUP::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    if( nullptr == aTopNode )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          " * [BUG] aTopNode is NULL" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    char tok = proc.Peek();

    if( proc.eof() )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; unexpected eof %s." ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

        return false;
    }

    if( '{' != tok )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; expecting '{' but got '%s' %s." ),
                    __FILE__, __FUNCTION__, __LINE__, tok, proc.GetFilePosition() );

        return false;
    }

    proc.Pop();

    while( true )
    {
        if( proc.Peek() == '}' )
        {
            proc.Pop();
            break;
        }

        if( !aTopNode->ReadNode( proc, this, nullptr ) )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; unexpected eof %s." ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

            return false;
        }

        if( proc.Peek() == ',' )
            proc.Pop();

    }   // while( true ) -- reading contents of Group{}

    return true;
}


SGNODE* WRL1GROUP::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    wxCHECK_MSG( m_Parent, nullptr, wxT( "Group has no parent." ) );

    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating Group with %zu children, %zu references, %zu back "
                     "pointers, and %zu items." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size(), m_Items.size() );

    if( WRL1NODES::WRL1_BASE != m_Parent->GetNodeType() )
    {
        if( nullptr == sp )
        {
            /// @todo Determine if this is a bug or can actually happen parsing a VRML file.
            ///       If it's a bug, this should be an assertion not a trace.
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] bad model: no base data given." ) );

            return nullptr;
        }
    }
    else if( nullptr == sp )
    {
        m_current.Init();
        sp = &m_current;
    }

    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    wxCHECK_MSG( nullptr == aParent && ptype == S3D::SGTYPE_TRANSFORM, nullptr,
                 wxString::Format(
                         wxT(" * [BUG] Group does not have a Transform parent (parent ID: %d" ),
                         ptype ) );

    IFSG_TRANSFORM txNode( aParent );
    bool hasContent = false;

    std::list< WRL1NODE* >::iterator sI = m_Items.begin();
    std::list< WRL1NODE* >::iterator eI = m_Items.end();

    SGNODE* node = txNode.GetRawPtr();

    while( sI != eI )
    {
        if( nullptr != (*sI)->TranslateToSG( node, sp ) )
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
