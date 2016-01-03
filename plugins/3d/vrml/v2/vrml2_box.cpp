/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include "vrml2_base.h"
#include "vrml2_box.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2BOX::WRL2BOX() : WRL2NODE()
{
    m_Type = WRL2_BOX;
    size.x = 2.0;
    size.y = 2.0;
    size.z = 2.0;

    return;
}


WRL2BOX::WRL2BOX( WRL2NODE* aParent ) : WRL2NODE()
{
    m_Type = WRL2_BOX;
    m_Parent = aParent;
    size.x = 2.0;
    size.y = 2.0;
    size.z = 2.0;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL2BOX::~WRL2BOX()
{
    #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 2 )
    std::cerr << " * [INFO] Destroying Box node\n";
    #endif
    return;
}


bool WRL2BOX::isDangling( void )
{
    // this node is dangling unless it has a parent of type WRL2_SHAPE

    if( NULL == m_Parent || m_Parent->GetNodeType() != WRL2_SHAPE )
        return true;

    return false;
}


bool WRL2BOX::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
    size_t line, column;
    proc.GetFilePosData( line, column );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected eof at line ";
        std::cerr << line << ", column " << column << "\n";
        #endif
        return false;
    }

    if( '{' != tok )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        std::cerr << proc.GetError() << "\n";
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; expecting '{' but got '" << tok;
        std::cerr  << "' at line " << line << ", column " << column << "\n";
        #endif

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
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << proc.GetError() <<  "\n";
        #endif

        return false;
    }

    proc.GetFilePosData( line, column );

    // expecting 'size'
    if( !glob.compare( "size" ) )
    {
        if( !proc.ReadSFVec3f( size ) )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] invalid size at line " << line << ", column ";
            std::cerr << column << "\n";
            std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
            std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
            #endif
            return false;
        }
    }
    else
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad Box at line " << line << ", column ";
        std::cerr << column << "\n";
        std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
        #endif

        return false;
    }

    if( size.x < 1e-6 || size.y < 1e-6 || size.z < 1e-6 )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad Box (invalid size) at line " << line << ", column ";
        std::cerr << column << "\n";
        std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
        #endif
    }

    if( proc.Peek() == '}' )
    {
        proc.Pop();
        return true;
    }

    proc.GetFilePosData( line, column );

    #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [INFO] bad Box at line " << line << ", column ";
    std::cerr << column << " (no closing brace)\n";
    std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
    #endif

    return false;
}


bool WRL2BOX::AddRefNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML2
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] AddRefNode is not applicable\n";
    #endif

    return false;
}


bool WRL2BOX::AddChildNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML2
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] AddChildNode is not applicable\n";
    #endif

    return false;
}


SGNODE* WRL2BOX::TranslateToSG( SGNODE* aParent, bool calcNormals )
{
    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( NULL != aParent && ptype != S3D::SGTYPE_SHAPE )
    {
        #ifdef DEBUG_VRML2
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] Box does not have a Shape parent (parent ID: ";
        std::cerr << ptype << ")\n";
        #endif

        return NULL;
    }

    // do not render a bad box
    if( size.x < 1e-6 || size.y < 1e-6 || size.z < 1e-6 )
        return NULL;

    if( m_sgNode )
    {
        if( NULL != aParent && aParent != S3D::GetSGNodeParent( m_sgNode )
            && !S3D::AddSGNodeRef( aParent, m_sgNode ) )
        {
            return NULL;
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
    // top
    vertices.push_back( SGPOINT( -x, -y, z ) );
    vertices.push_back( SGPOINT(  x, -y, z ) );
    vertices.push_back( SGPOINT(  x,  y, z ) );
    vertices.push_back( SGPOINT( -x,  y, z ) );
    norms.push_back( SGVECTOR( 0.0, 0.0, 1.0 ) );
    norms.push_back( SGVECTOR( 0.0, 0.0, 1.0 ) );
    norms.push_back( SGVECTOR( 0.0, 0.0, 1.0 ) );
    norms.push_back( SGVECTOR( 0.0, 0.0, 1.0 ) );
    idx.push_back( base );
    idx.push_back( base + 1 );
    idx.push_back( base + 2 );
    idx.push_back( base );
    idx.push_back( base + 2 );
    idx.push_back( base + 3 );
    base += 4;
    // bottom
    vertices.push_back( SGPOINT( -x, -y, -z ) );
    vertices.push_back( SGPOINT(  x, -y, -z ) );
    vertices.push_back( SGPOINT(  x,  y, -z ) );
    vertices.push_back( SGPOINT( -x,  y, -z ) );
    norms.push_back( SGVECTOR( 0.0, 0.0, -1.0 ) );
    norms.push_back( SGVECTOR( 0.0, 0.0, -1.0 ) );
    norms.push_back( SGVECTOR( 0.0, 0.0, -1.0 ) );
    norms.push_back( SGVECTOR( 0.0, 0.0, -1.0 ) );
    idx.push_back( base );
    idx.push_back( base + 2 );
    idx.push_back( base + 1 );
    idx.push_back( base );
    idx.push_back( base + 3 );
    idx.push_back( base + 2 );
    base += 4;
    // front
    vertices.push_back( SGPOINT( -x, -y,  z ) );
    vertices.push_back( SGPOINT( -x, -y, -z ) );
    vertices.push_back( SGPOINT(  x, -y, -z ) );
    vertices.push_back( SGPOINT(  x, -y,  z ) );
    norms.push_back( SGVECTOR( 0.0, -1.0, 0.0 ) );
    norms.push_back( SGVECTOR( 0.0, -1.0, 0.0 ) );
    norms.push_back( SGVECTOR( 0.0, -1.0, 0.0 ) );
    norms.push_back( SGVECTOR( 0.0, -1.0, 0.0 ) );
    idx.push_back( base );
    idx.push_back( base + 1 );
    idx.push_back( base + 2 );
    idx.push_back( base );
    idx.push_back( base + 2 );
    idx.push_back( base + 3 );
    base += 4;
    // back
    vertices.push_back( SGPOINT( -x, y,  z ) );
    vertices.push_back( SGPOINT( -x, y, -z ) );
    vertices.push_back( SGPOINT(  x, y, -z ) );
    vertices.push_back( SGPOINT(  x, y,  z ) );
    norms.push_back( SGVECTOR( 0.0, 1.0, 0.0 ) );
    norms.push_back( SGVECTOR( 0.0, 1.0, 0.0 ) );
    norms.push_back( SGVECTOR( 0.0, 1.0, 0.0 ) );
    norms.push_back( SGVECTOR( 0.0, 1.0, 0.0 ) );
    idx.push_back( base );
    idx.push_back( base + 2 );
    idx.push_back( base + 1 );
    idx.push_back( base );
    idx.push_back( base + 3 );
    idx.push_back( base + 2 );
    base += 4;
    // left
    vertices.push_back( SGPOINT( -x, -y, -z ) );
    vertices.push_back( SGPOINT( -x, -y,  z ) );
    vertices.push_back( SGPOINT( -x,  y,  z ) );
    vertices.push_back( SGPOINT( -x,  y, -z ) );
    norms.push_back( SGVECTOR( -1.0, 0.0, 0.0 ) );
    norms.push_back( SGVECTOR( -1.0, 0.0, 0.0 ) );
    norms.push_back( SGVECTOR( -1.0, 0.0, 0.0 ) );
    norms.push_back( SGVECTOR( -1.0, 0.0, 0.0 ) );
    idx.push_back( base );
    idx.push_back( base + 1 );
    idx.push_back( base + 2 );
    idx.push_back( base );
    idx.push_back( base + 2 );
    idx.push_back( base + 3 );
    base += 4;
    // right
    vertices.push_back( SGPOINT( x, -y, -z ) );
    vertices.push_back( SGPOINT( x, -y,  z ) );
    vertices.push_back( SGPOINT( x,  y,  z ) );
    vertices.push_back( SGPOINT( x,  y, -z ) );
    norms.push_back( SGVECTOR( 1.0, 0.0, 0.0 ) );
    norms.push_back( SGVECTOR( 1.0, 0.0, 0.0 ) );
    norms.push_back( SGVECTOR( 1.0, 0.0, 0.0 ) );
    norms.push_back( SGVECTOR( 1.0, 0.0, 0.0 ) );
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
