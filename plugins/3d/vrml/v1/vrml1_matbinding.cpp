/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
#include "vrml1_matbinding.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1MATBINDING::WRL1MATBINDING( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_binding = WRL1_BINDING::BIND_OVERALL;
    m_Type = WRL1NODES::WRL1_MATERIALBINDING;
}


WRL1MATBINDING::WRL1MATBINDING( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_binding = WRL1_BINDING::BIND_OVERALL;
    m_Type = WRL1NODES::WRL1_MATERIALBINDING;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL1MATBINDING::~WRL1MATBINDING()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying MaterialBinding node\n" ) );
}


bool WRL1MATBINDING::AddRefNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddRefNode is not applicable." ) );
}


bool WRL1MATBINDING::AddChildNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddChildNode is not applicable." ) );
}


bool WRL1MATBINDING::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    wxCHECK_MSG( aTopNode, false, wxT( "aTopNode is NULL." ) );

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
    std::string glob;

    while( true )
    {
        if( proc.Peek() == '}' )
        {
            proc.Pop();
            break;
        }

        if( !proc.ReadName( glob ) )
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetError() );

            return false;
        }

        if( glob.compare( "value" ) )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad MaterialBinding %s (expecting keyword 'value').\n"
                             " * [INFO] file: '%s'." ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }

        if( !proc.ReadName( glob ) )
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetError() );

            return false;
        }

        // expecting one of:
        // DEFAULT
        // OVERALL
        // PER_PART
        // PER_PART_INDEXED
        // PER_FACE
        // PER_FACE_INDEXED
        // PER_VERTEX
        // PER_VERTEX_INDEXED

        if( !glob.compare( "DEFAULT" ) )
        {
            m_binding = WRL1_BINDING::BIND_DEFAULT;
        }
        else if( !glob.compare( "OVERALL" ) )
        {
            m_binding = WRL1_BINDING::BIND_OVERALL;
        }
        else if( !glob.compare( "PER_PART" ) )
        {
            m_binding = WRL1_BINDING::BIND_PER_PART;
        }
        else if( !glob.compare( "PER_PART_INDEXED" ) )
        {
            m_binding = WRL1_BINDING::BIND_PER_PART_INDEXED;
        }
        else if( !glob.compare( "PER_FACE" ) )
        {
            m_binding = WRL1_BINDING::BIND_PER_FACE;
        }
        else if( !glob.compare( "PER_FACE_INDEXED" ) )
        {
            m_binding = WRL1_BINDING::BIND_PER_FACE_INDEXED;
        }
        else if( !glob.compare( "PER_VERTEX" ) )
        {
            m_binding = WRL1_BINDING::BIND_PER_VERTEX;
        }
        else if( !glob.compare( "PER_VERTEX_INDEXED" ) )
        {
            m_binding = WRL1_BINDING::BIND_PER_VERTEX_INDEXED;
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              " * [INFO] bad MaterialBinding %s\n"
                                              " * [INFO] file: '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            m_binding = WRL1_BINDING::BIND_OVERALL;
        }
    }   // while( true ) -- reading contents of MaterialBinding{}

    return true;
}


SGNODE* WRL1MATBINDING::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    wxCHECK_MSG ( sp, nullptr, wxT( "Bad model: no base data given." ) );

    sp->matbind = m_binding;

    return nullptr;
}
