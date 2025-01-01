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

#include "vrml2_base.h"
#include "vrml2_norms.h"


WRL2NORMS::WRL2NORMS() : WRL2NODE()
{
    m_Type = WRL2NODES::WRL2_NORMAL;
}


WRL2NORMS::WRL2NORMS( WRL2NODE* aParent ) : WRL2NODE()
{
    m_Type = WRL2NODES::WRL2_NORMAL;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL2NORMS::~WRL2NORMS()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying Normal node." ) );
}


bool WRL2NORMS::isDangling( void )
{
    // this node is dangling unless it has a parent of type WRL2_INDEXEDFACESET

    if( nullptr == m_Parent || m_Parent->GetNodeType() != WRL2NODES::WRL2_INDEXEDFACESET )
        return true;

    return false;
}


bool WRL2NORMS::AddRefNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddRefNode is not applicable." ) );
}


bool WRL2NORMS::AddChildNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddChildNode is not applicable." ) );
}


bool WRL2NORMS::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
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
    std::string glob;

    if( proc.Peek() == '}' )
    {
        proc.Pop();
        return true;
    }

    if( !proc.ReadName( glob ) )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          "%s" ),
                    __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

        return false;
    }

    // expecting 'vector'
    if( !glob.compare( "vector" ) )
    {
        if( !proc.ReadMFVec3f( vectors ) )
        {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid vector set %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

            return false;
        }
    }
    else
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          " * [INFO] invalid Normal %s\n"
                                          " * [INFO] file: '%s'" ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(), proc.GetFileName() );

        return false;
    }

    if( proc.Peek() == '}' )
    {
        proc.Pop();
        return true;
    }

    wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                      " * [INFO] invalid Normal %s (no closing brace)\n"
                                      " * [INFO] file: '%s'" ),
                __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(), proc.GetFileName() );

    return false;
}


SGNODE* WRL2NORMS::TranslateToSG( SGNODE* aParent )
{
    // any data manipulation must be performed by the parent node
    return nullptr;
}
