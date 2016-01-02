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
#include "vrml2_coords.h"


WRL2COORDS::WRL2COORDS() : WRL2NODE()
{
    m_Type = WRL2_COORDINATE;
    return;
}


WRL2COORDS::WRL2COORDS( WRL2NODE* aParent ) : WRL2NODE()
{
    m_Type = WRL2_COORDINATE;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL2COORDS::~WRL2COORDS()
{
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 2 )
    std::cerr << " * [INFO] Destroying Coordinate node\n";
    #endif
    return;
}


bool WRL2COORDS::isDangling( void )
{
    // this node is dangling unless it has a parent of type WRL2_INDEXEDFACESET

    if( NULL == m_Parent || m_Parent->GetNodeType() != WRL2_INDEXEDFACESET )
        return true;

    return false;
}


bool WRL2COORDS::AddRefNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML2
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] AddRefNode is not applicable\n";
    #endif

    return false;
}


bool WRL2COORDS::AddChildNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML2
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] AddChildNode is not applicable\n";
    #endif

    return false;
}


bool WRL2COORDS::Read( WRLPROC& proc, WRL2BASE* aTopNode )
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

    // expecting 'point'
    if( !glob.compare( "point" ) )
    {
        if( !proc.ReadMFVec3f( points ) )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] invalid point set at line " << line << ", column ";
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
        std::cerr << " * [INFO] bad Coordinate at line " << line << ", column ";
        std::cerr << column << "\n";
        std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
        #endif

        return false;
    }

    if( proc.Peek() == '}' )
    {
        proc.Pop();
        return true;
    }

    proc.GetFilePosData( line, column );

    #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [INFO] bad Coordinate at line " << line << ", column ";
    std::cerr << column << " (no closing brace)\n";
    std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
    #endif

    return false;
}


void WRL2COORDS::GetCoords( WRLVEC3F*& aCoordList, size_t& aListSize )
{
    if( points.size() < 3 )
    {
        aCoordList = NULL;
        aListSize = 0;
        return;
    }

    aCoordList = &points[0];
    aListSize = points.size();
    return;
}


SGNODE* WRL2COORDS::TranslateToSG( SGNODE* aParent, bool calcNormals )
{
    // any data manipulation must be performed by the parent node
    return NULL;
}
