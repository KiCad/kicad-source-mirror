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
#include "vrml2_color.h"


WRL2COLOR::WRL2COLOR() : WRL2NODE()
{
    m_Type = WRL2NODES::WRL2_COLOR;
}


WRL2COLOR::WRL2COLOR( WRL2NODE* aParent ) : WRL2NODE()
{
    m_Type = WRL2NODES::WRL2_COLOR;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL2COLOR::~WRL2COLOR()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying Color node" ) );
}


bool WRL2COLOR::isDangling( void )
{
    // this node is dangling unless it has a parent of type WRL2_INDEXEDFACESET
    if( nullptr == m_Parent || m_Parent->GetNodeType() != WRL2NODES::WRL2_INDEXEDFACESET )
        return true;

    return false;
}


bool WRL2COLOR::AddRefNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddRefNode is not applicable." ) );
}


bool WRL2COLOR::AddChildNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddChildNode is not applicable." ) );
}


bool WRL2COLOR::Read( WRLPROC& proc, WRL2BASE* aTopNode )
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

    // expecting 'color'
    if( !glob.compare( "color" ) )
    {
        if( !proc.ReadMFVec3f( colors ) )
        {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid color set %s\n"
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
                                          " * [INFO] invalid Color %s\n"
                                          " * [INFO] file: '%s'\n" ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(), proc.GetFileName() );

        return false;
    }

    if( proc.Peek() == '}' )
    {
        proc.Pop();
        return true;
    }

    wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                      " * [INFO] invalid Color %s (no closing brace)\n"
                                      " * [INFO] file: '%s'\n" ),
                __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(), proc.GetFileName() );

    return false;
}


SGNODE* WRL2COLOR::TranslateToSG( SGNODE* aParent )
{
    // any data manipulation must be performed by the parent node
    return nullptr;
}


bool WRL2COLOR::HasColors( void )
{
    if( colors.empty() )
        return false;

    return true;
}


void WRL2COLOR::GetColor( int aIndex, float& red, float& green, float& blue )
{
    if( aIndex < 0 || aIndex >= (int)colors.size() )
    {
        red = 0.8f;
        green = 0.8f;
        blue = 0.8f;
        return;
    }

    red = colors[aIndex].x;
    green = colors[aIndex].y;
    blue = colors[aIndex].z;
}


void WRL2COLOR::GetColors( WRLVEC3F*& aColorList, size_t& aListSize)
{
    if( colors.empty() )
    {
        aColorList = nullptr;
        aListSize = 0;
        return;
    }

    aColorList = &colors[0];
    aListSize = colors.size();
}
