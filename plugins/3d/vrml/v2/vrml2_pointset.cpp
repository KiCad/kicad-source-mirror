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
#include "vrml2_pointset.h"
#include "vrml2_coords.h"
#include "vrml2_color.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2POINTSET::WRL2POINTSET() : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2_POINTSET;

    return;
}


WRL2POINTSET::WRL2POINTSET( WRL2NODE* aParent ) : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2_POINTSET;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL2POINTSET::~WRL2POINTSET()
{
    #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 2 )
    std::cerr << " * [INFO] Destroying PointSet with " << m_Children.size();
    std::cerr << " children, " << m_Refs.size() << " references and ";
    std::cerr << m_BackPointers.size() << " backpointers\n";
    #endif
    return;
}


void WRL2POINTSET::setDefaults( void )
{
    color = NULL;
    coord = NULL;
}


bool WRL2POINTSET::checkNodeType( WRL2NODES aType )
{
    // nodes must be one of:
    // Color
    // Coordinate

    switch( aType )
    {
    case WRL2_COLOR:
    case WRL2_COORDINATE:
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

    if( NULL == m_Parent || m_Parent->GetNodeType() != WRL2_SHAPE )
        return true;

    return false;
}


bool WRL2POINTSET::AddRefNode( WRL2NODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG_VRML2
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL passed for aNode\n";
        #endif

        return false;
    }

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected child node '";
        std::cerr << aNode->GetNodeTypeName( type ) << "'\n";
        #endif

        return false;
    }

    if( WRL2_COLOR == type )
    {
        if( NULL != color )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple color nodes\n";
            #endif
            return false;
        }

        color = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2_COORDINATE == type )
    {
        if( NULL != coord )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple coordinate nodes\n";
            #endif
            return false;
        }

        coord = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    return WRL2NODE::AddRefNode( aNode );
}


bool WRL2POINTSET::AddChildNode( WRL2NODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG_VRML2
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL passed for aNode\n";
        #endif

        return false;
    }

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected child node '";
        std::cerr << aNode->GetNodeTypeName( type ) << "'\n";
        #endif

        return false;
    }

    if( WRL2_COLOR == type )
    {
        if( NULL != color )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple color nodes\n";
            #endif
            return false;
        }

        color = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2_COORDINATE == type )
    {
        if( NULL != coord )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple coordinate nodes\n";
            #endif
            return false;
        }

        coord = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    return WRL2NODE::AddChildNode( aNode );
}



bool WRL2POINTSET::Read( WRLPROC& proc, WRL2BASE* aTopNode )
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

    while( true )
    {
        if( proc.Peek() == '}' )
        {
            proc.Pop();
            break;
        }

        if( !proc.ReadName( glob ) )
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

            return false;
        }

        // expecting one of:
        // color
        // coord

        proc.GetFilePosData( line, column );

        if( !glob.compare( "color" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] could not read color node information\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "coord" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] could not read coord node information\n";
                #endif
                return false;
            }
        }
        else
        {
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad PointSet at line " << line << ", column ";
            std::cerr << column << "\n";
            std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
            #endif

            return false;
        }
    }   // while( true ) -- reading contents of PointSet{}

    return true;
}


SGNODE* WRL2POINTSET::TranslateToSG( SGNODE* aParent, bool calcNormals )
{
    // note: there are no plans to support drawing of points
    return NULL;
}


void WRL2POINTSET::unlinkChildNode( const WRL2NODE* aNode )
{
    if( NULL == aNode )
        return;

    if( aNode->GetParent() == this )
    {
        if( aNode == color )
            color = NULL;
        else if( aNode == coord )
            coord = NULL;

    }

    WRL2NODE::unlinkChildNode( aNode );
    return;
}


void WRL2POINTSET::unlinkRefNode( const WRL2NODE* aNode )
{
    if( NULL == aNode )
        return;

    if( aNode->GetParent() != this )
    {
        if( aNode == color )
            color = NULL;
        else if( aNode == coord )
            coord = NULL;

    }

    WRL2NODE::unlinkRefNode( aNode );
    return;
}


bool WRL2POINTSET::HasColors( void )
{
    if( NULL == color )
        return false;

    return ((WRL2COLOR*) color)->HasColors();
}
