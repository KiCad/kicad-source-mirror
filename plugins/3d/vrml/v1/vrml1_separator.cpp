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
#include "vrml1_separator.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1SEPARATOR::WRL1SEPARATOR( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_SEPARATOR;
}


WRL1SEPARATOR::WRL1SEPARATOR( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_SEPARATOR;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL1SEPARATOR::~WRL1SEPARATOR()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying Separator with %zu children %zu references, and %zu "
                     "back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


bool WRL1SEPARATOR::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    wxCHECK_MSG( aTopNode, false, wxT( "Invalid top node." ) );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
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
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              " * [INFO] bad file format; unexpected eof %s." ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

            return false;
        }

        if( proc.Peek() == ',' )
            proc.Pop();

    }   // while( true ) -- reading contents of Separator{}

    return true;
}


SGNODE* WRL1SEPARATOR::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    wxCHECK_MSG( m_Parent, nullptr, wxT( "Separator has no parent." )  );

    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating Separator with %zu children, %zu references, and "
                     "%zu back pointers (%zu total items)." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size(), m_Items.size() );

    if( sp != nullptr )
        m_current = *sp;
    else
        m_current.Init();

    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    wxCHECK_MSG( aParent && ( ptype == S3D::SGTYPE_TRANSFORM ), nullptr,
                 wxString::Format( wxT( "Separator does not have a Transform parent (parent "
                                        "ID: %d)." ), ptype ) );

    IFSG_TRANSFORM txNode( aParent );
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
