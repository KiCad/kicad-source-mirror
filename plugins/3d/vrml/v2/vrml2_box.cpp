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
#include "vrml2_box.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2BOX::WRL2BOX() : WRL2NODE()
{
    m_Type = WRL2NODES::WRL2_BOX;
    size.x = 2.0;
    size.y = 2.0;
    size.z = 2.0;
}


WRL2BOX::WRL2BOX( WRL2NODE* aParent ) : WRL2NODE()
{
    m_Type = WRL2NODES::WRL2_BOX;
    m_Parent = aParent;
    size.x = 2.0;
    size.y = 2.0;
    size.z = 2.0;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL2BOX::~WRL2BOX()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying Box node." ) );
}


bool WRL2BOX::isDangling( void )
{
    // this node is dangling unless it has a parent of type WRL2_SHAPE

    if( nullptr == m_Parent || m_Parent->GetNodeType() != WRL2NODES::WRL2_SHAPE )
        return true;

    return false;
}


bool WRL2BOX::Read( WRLPROC& proc, WRL2BASE* aTopNode )
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

    // expecting 'size'
    if( !glob.compare( "size" ) )
    {
        if( !proc.ReadSFVec3f( size ) )
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d"
                                              " * [INFO] invalid size %s\n"
                                              " * [INFO] file: '%s'\n"
                                              "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

            return false;
        }

        // for legacy KiCad support we interpret units as 0.1 inch
        size *= 2.54;
    }
    else
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d"
                                          " * [INFO] invalid Box %s\n"
                                          " * [INFO] file: '%s'\n" ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                    proc.GetFileName() );

        return false;
    }

    if( size.x < 1e-6 || size.y < 1e-6 || size.z < 1e-6 )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d"
                                          " * [INFO] invalid Box size %s\n"
                                          " * [INFO] file: '%s'\n" ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                    proc.GetFileName() );

        /// @note If this box is bad, should false be returned here?
    }

    if( proc.Peek() == '}' )
    {
        proc.Pop();
        return true;
    }

    wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d"
                                      " * [INFO] invalid size %s (no closing brace).\n"
                                      " * [INFO] file: '%s'\n" ),
                __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(), proc.GetFileName() );

    return false;
}


bool WRL2BOX::AddRefNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddRefNode is not applicable." ) );
}


bool WRL2BOX::AddChildNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddChildNode is not applicable." ) );
}


SGNODE* WRL2BOX::TranslateToSG( SGNODE* aParent )
{
    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    wxCHECK_MSG( aParent && ( ptype == S3D::SGTYPE_SHAPE ), nullptr,
                 wxString::Format( wxT( "Box does not have a Shape parent (parent ID: %s)" ),
                                   ptype ) );

    // do not render a bad box
    if( size.x < 1e-6 || size.y < 1e-6 || size.z < 1e-6 )
        return nullptr;

    if( m_sgNode )
    {
        if( nullptr != aParent )
        {
            if( nullptr == S3D::GetSGNodeParent( m_sgNode )
                && !S3D::AddSGNodeChild( aParent, m_sgNode ) )
            {
                return nullptr;
            }
            else if( aParent != S3D::GetSGNodeParent( m_sgNode )
                     && !S3D::AddSGNodeRef( aParent, m_sgNode ) )
            {
                return nullptr;
            }
        }

        return m_sgNode;
    }

    // create the vertices, triangle indices, and normals
    float x = size.x / 2.0;
    float y = size.y / 2.0;
    float z = size.z / 2.0;
    std::vector< SGPOINT > vertices;
    std::vector< SGVECTOR > norms;
    std::vector< int > idx;
    int base = 0;

    vertices.reserve( 6 * 4 );
    norms.reserve( 6 * 4 );
    idx.reserve( 6 * 6 );

    // top
    vertices.emplace_back( -x, -y, z );
    vertices.emplace_back(  x, -y, z );
    vertices.emplace_back(  x,  y, z );
    vertices.emplace_back( -x,  y, z );
    norms.emplace_back( 0.0, 0.0, 1.0 );
    norms.emplace_back( 0.0, 0.0, 1.0 );
    norms.emplace_back( 0.0, 0.0, 1.0 );
    norms.emplace_back( 0.0, 0.0, 1.0 );
    idx.push_back( base );
    idx.push_back( base + 1 );
    idx.push_back( base + 2 );
    idx.push_back( base );
    idx.push_back( base + 2 );
    idx.push_back( base + 3 );
    base += 4;

    // bottom
    vertices.emplace_back( -x, -y, -z );
    vertices.emplace_back(  x, -y, -z );
    vertices.emplace_back(  x,  y, -z );
    vertices.emplace_back( -x,  y, -z );
    norms.emplace_back( 0.0, 0.0, -1.0 );
    norms.emplace_back( 0.0, 0.0, -1.0 );
    norms.emplace_back( 0.0, 0.0, -1.0 );
    norms.emplace_back( 0.0, 0.0, -1.0 );
    idx.push_back( base );
    idx.push_back( base + 2 );
    idx.push_back( base + 1 );
    idx.push_back( base );
    idx.push_back( base + 3 );
    idx.push_back( base + 2 );
    base += 4;

    // front
    vertices.emplace_back( -x, -y,  z );
    vertices.emplace_back( -x, -y, -z );
    vertices.emplace_back(  x, -y, -z );
    vertices.emplace_back(  x, -y,  z );
    norms.emplace_back( 0.0, -1.0, 0.0 );
    norms.emplace_back( 0.0, -1.0, 0.0 );
    norms.emplace_back( 0.0, -1.0, 0.0 );
    norms.emplace_back( 0.0, -1.0, 0.0 );
    idx.push_back( base );
    idx.push_back( base + 1 );
    idx.push_back( base + 2 );
    idx.push_back( base );
    idx.push_back( base + 2 );
    idx.push_back( base + 3 );
    base += 4;

    // back
    vertices.emplace_back( -x, y,  z );
    vertices.emplace_back( -x, y, -z );
    vertices.emplace_back(  x, y, -z );
    vertices.emplace_back(  x, y,  z );
    norms.emplace_back( 0.0, 1.0, 0.0 );
    norms.emplace_back( 0.0, 1.0, 0.0 );
    norms.emplace_back( 0.0, 1.0, 0.0 );
    norms.emplace_back( 0.0, 1.0, 0.0 );
    idx.push_back( base );
    idx.push_back( base + 2 );
    idx.push_back( base + 1 );
    idx.push_back( base );
    idx.push_back( base + 3 );
    idx.push_back( base + 2 );
    base += 4;

    // left
    vertices.emplace_back( -x, -y, -z );
    vertices.emplace_back( -x, -y,  z );
    vertices.emplace_back( -x,  y,  z );
    vertices.emplace_back( -x,  y, -z );
    norms.emplace_back( -1.0, 0.0, 0.0 );
    norms.emplace_back( -1.0, 0.0, 0.0 );
    norms.emplace_back( -1.0, 0.0, 0.0 );
    norms.emplace_back( -1.0, 0.0, 0.0 );
    idx.push_back( base );
    idx.push_back( base + 1 );
    idx.push_back( base + 2 );
    idx.push_back( base );
    idx.push_back( base + 2 );
    idx.push_back( base + 3 );
    base += 4;

    // right
    vertices.emplace_back( x, -y, -z );
    vertices.emplace_back( x, -y,  z );
    vertices.emplace_back( x,  y,  z );
    vertices.emplace_back( x,  y, -z );
    norms.emplace_back( 1.0, 0.0, 0.0 );
    norms.emplace_back( 1.0, 0.0, 0.0 );
    norms.emplace_back( 1.0, 0.0, 0.0 );
    norms.emplace_back( 1.0, 0.0, 0.0 );
    idx.push_back( base );
    idx.push_back( base + 2 );
    idx.push_back( base + 1 );
    idx.push_back( base );
    idx.push_back( base + 3 );
    idx.push_back( base + 2 );

    IFSG_FACESET fsNode( aParent );
    IFSG_COORDS cpNode( fsNode );
    cpNode.SetCoordsList( vertices.size(), &vertices[0] );
    IFSG_COORDINDEX ciNode( fsNode );
    ciNode.SetIndices( idx.size(), &idx[0] );
    IFSG_NORMALS nmNode( fsNode );
    nmNode.SetNormalList( norms.size(), &norms[0] );

    m_sgNode = fsNode.GetRawPtr();

    return m_sgNode;
}
