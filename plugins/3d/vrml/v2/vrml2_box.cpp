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
#include <sstream>
#include <wx/log.h>

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
    wxLogTrace( MASK_VRML, " * [INFO] Destroying Box node\n" );
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
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; unexpected eof at line ";
            ostr << line << ", column " << column;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( '{' != tok )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << proc.GetError() << "\n";
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; expecting '{' but got '" << tok;
            ostr  << "' at line " << line << ", column " << column;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
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
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << proc.GetError();
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
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
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] invalid size at line " << line << ", column ";
                ostr << column << "\n";
                ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                ostr << " * [INFO] message: '" << proc.GetError() << "'";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }

        // for legacy KiCad support we interpret units as 0.1 inch
        size *= 2.54;
    }
    else
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad Box at line " << line << ", column ";
            ostr << column << "\n";
            ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( size.x < 1e-6 || size.y < 1e-6 || size.z < 1e-6 )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad Box (invalid size) at line " << line << ", column ";
            ostr << column << "\n";
            ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif
    }

    if( proc.Peek() == '}' )
    {
        proc.Pop();
        return true;
    }

    proc.GetFilePosData( line, column );

    #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [INFO] bad Box at line " << line << ", column ";
        ostr << column << " (no closing brace)\n";
        ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return false;
}


bool WRL2BOX::AddRefNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML2
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] AddRefNode is not applicable";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return false;
}


bool WRL2BOX::AddChildNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML2
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] AddChildNode is not applicable";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return false;
}


SGNODE* WRL2BOX::TranslateToSG( SGNODE* aParent )
{
    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( NULL != aParent && ptype != S3D::SGTYPE_SHAPE )
    {
        #ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] Box does not have a Shape parent (parent ID: ";
            ostr << ptype << ")";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return NULL;
    }

    // do not render a bad box
    if( size.x < 1e-6 || size.y < 1e-6 || size.z < 1e-6 )
        return NULL;

    if( m_sgNode )
    {
        if( NULL != aParent )
        {
            if( NULL == S3D::GetSGNodeParent( m_sgNode )
                && !S3D::AddSGNodeChild( aParent, m_sgNode ) )
            {
                return NULL;
            }
            else if( aParent != S3D::GetSGNodeParent( m_sgNode )
                     && !S3D::AddSGNodeRef( aParent, m_sgNode ) )
            {
                return NULL;
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
