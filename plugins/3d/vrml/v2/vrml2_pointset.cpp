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
#include "vrml2_pointset.h"
#include "vrml2_coords.h"
#include "vrml2_color.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2POINTSET::WRL2POINTSET() : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2NODES::WRL2_POINTSET;
}


WRL2POINTSET::WRL2POINTSET( WRL2NODE* aParent ) : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2NODES::WRL2_POINTSET;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL2POINTSET::~WRL2POINTSET()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying PointSet node with %zu children, %zu"
                     "references, and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


void WRL2POINTSET::setDefaults( void )
{
    color = nullptr;
    coord = nullptr;
}


bool WRL2POINTSET::checkNodeType( WRL2NODES aType )
{
    // nodes must be one of:
    // Color
    // Coordinate

    switch( aType )
    {
    case WRL2NODES::WRL2_COLOR:
    case WRL2NODES::WRL2_COORDINATE:
        break;

    default:
        return false;
        break;
    }

    return true;
}


bool WRL2POINTSET::isDangling( void )
{
    // this node is dangling unless it has a parent of type WRL2_SHAPE
    if( nullptr == m_Parent || m_Parent->GetNodeType() != WRL2NODES::WRL2_SHAPE )
        return true;

    return false;
}


bool WRL2POINTSET::AddRefNode( WRL2NODE* aNode )
{
    wxCHECK_MSG( aNode, false, wxT( "Invalid node." ) );

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; unexpected child node '%s'." ),
                    __FILE__, __FUNCTION__, __LINE__, aNode->GetNodeTypeName( type ) );

        return false;
    }

    if( WRL2NODES::WRL2_COLOR == type )
    {
        if( nullptr != color )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple color nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        color = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2NODES::WRL2_COORDINATE == type )
    {
        if( nullptr != coord )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple coord nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        coord = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    return WRL2NODE::AddRefNode( aNode );
}


bool WRL2POINTSET::AddChildNode( WRL2NODE* aNode )
{
    wxCHECK_MSG( aNode, false, wxT( "Invalid node." ) );

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; unexpected child node '%s'." ),
                    __FILE__, __FUNCTION__, __LINE__, aNode->GetNodeTypeName( type ) );

        return false;
    }

    if( WRL2NODES::WRL2_COLOR == type )
    {
        if( nullptr != color )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple color nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        color = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2NODES::WRL2_COORDINATE == type )
    {
        if( nullptr != coord )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple coord nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        coord = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    return WRL2NODE::AddChildNode( aNode );
}



bool WRL2POINTSET::Read( WRLPROC& proc, WRL2BASE* aTopNode )
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
                        __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

            return false;
        }

        // expecting one of:
        // color
        // coord
        if( !glob.compare( "color" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] could not read color node information." ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }
        }
        else if( !glob.compare( "coord" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] could not read coord node information." ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }
        }
        else
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] invalid PointSet %s (no closing brace)\n"
                             " * [INFO] file: '%s'\n" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }
    }   // while( true ) -- reading contents of PointSet{}

    return true;
}


SGNODE* WRL2POINTSET::TranslateToSG( SGNODE* aParent )
{
    // note: there are no plans to support drawing of points
    return nullptr;
}


void WRL2POINTSET::unlinkChildNode( const WRL2NODE* aNode )
{
    if( nullptr == aNode )
        return;

    if( aNode->GetParent() == this )
    {
        if( aNode == color )
            color = nullptr;
        else if( aNode == coord )
            coord = nullptr;
    }

    WRL2NODE::unlinkChildNode( aNode );
}


void WRL2POINTSET::unlinkRefNode( const WRL2NODE* aNode )
{
    if( nullptr == aNode )
        return;

    if( aNode->GetParent() != this )
    {
        if( aNode == color )
            color = nullptr;
        else if( aNode == coord )
            coord = nullptr;

    }

    WRL2NODE::unlinkRefNode( aNode );
}


bool WRL2POINTSET::HasColors( void )
{
    if( nullptr == color )
        return false;

    return ( (WRL2COLOR*) color )->HasColors();
}
