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
#include "vrml2_appearance.h"


WRL2APPEARANCE::WRL2APPEARANCE() : WRL2NODE()
{
    material = NULL;
    texture = NULL;
    textureTransform = NULL;
    m_Type = WRL2_APPEARANCE;
    return;
}


WRL2APPEARANCE::WRL2APPEARANCE( WRL2NODE* aParent ) : WRL2NODE()
{
    material = NULL;
    texture = NULL;
    textureTransform = NULL;
    m_Type = WRL2_APPEARANCE;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL2APPEARANCE::~WRL2APPEARANCE()
{
    #ifdef DEBUG
    std::cerr << " * [INFO] Destroying Appearance with " << m_Children.size();
    std::cerr << " children, " << m_Refs.size() << " references and ";
    std::cerr << m_BackPointers.size() << " backpointers\n";
    #endif
}


bool WRL2APPEARANCE::checkNodeType( WRL2NODES aType )
{
    switch( aType )
    {
    case WRL2_MATERIAL:
    case WRL2_IMAGETEXTURE:
    case WRL2_PIXELTEXTURE:
    case WRL2_MOVIETEXTURE:
    case WRL2_TEXTURETRANSFORM:
        break;

    default:
        return false;
        break;
    }

    return true;
}


bool WRL2APPEARANCE::isDangling( void )
{
    // this node is dangling unless it has a parent of type WRL2_SHAPE

    if( NULL == m_Parent || m_Parent->GetNodeType() != WRL2_SHAPE )
        return true;

    return false;
}


bool WRL2APPEARANCE::AddRefNode( WRL2NODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL passed for aNode\n";
        #endif

        return false;
    }

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected child node '";
        std::cerr << aNode->GetNodeTypeName( type ) << "'\n";
        #endif

        return false;
    }

    if( WRL2_MATERIAL == type )
    {
        if( NULL != material )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple material nodes\n";
            #endif
            return false;
        }

        material = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2_TEXTURETRANSFORM == type )
    {
        if( NULL != textureTransform )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple textureTransform nodes\n";
            #endif
            return false;
        }

        textureTransform = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( NULL != texture )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple texture nodes\n";
        #endif
        return false;
    }

    texture = aNode;
    return WRL2NODE::AddRefNode( aNode );
}


bool WRL2APPEARANCE::AddChildNode( WRL2NODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL passed for aNode\n";
        #endif

        return false;
    }

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected child node '";
        std::cerr << aNode->GetNodeTypeName( type ) << "'\n";
        #endif

        return false;
    }

    if( WRL2_MATERIAL == type )
    {
        if( NULL != material )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple material nodes\n";
            #endif
            return false;
        }

        material = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2_TEXTURETRANSFORM == type )
    {
        if( NULL != textureTransform )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple textureTransform nodes\n";
            #endif
            return false;
        }

        textureTransform = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( NULL != texture )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple texture nodes\n";
        #endif
        return false;
    }

    texture = aNode;
    return WRL2NODE::AddChildNode( aNode );
}


bool WRL2APPEARANCE::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
    if( NULL == aTopNode )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] aTopNode is NULL\n";
        #endif
        return false;
    }

    size_t line, column;
    proc.GetFilePosData( line, column );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected eof at line ";
        std::cerr << line << ", column " << column << "\n";
        #endif
        return false;
    }

    if( '{' != tok )
    {
        #ifdef DEBUG
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
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

            return false;
        }

        // expecting one of:
        // material
        // texture
        // textureTransform

        proc.GetFilePosData( line, column );

        if( !glob.compare( "material" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] could not read material information\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "texture" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] could not read texture information\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "textureTransform" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] could not read textureTransform information\n";
                #endif
                return false;
            }
        }
        else
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad Appearance at line " << line << ", column ";
            std::cerr << column << "\n";
            std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
            #endif

            return false;
        }
    }   // while( true ) -- reading contents of Appearance{}

    return true;
}


SGNODE* WRL2APPEARANCE::TranslateToSG( SGNODE* aParent )
{
    // XXX - TO IMPLEMENT
    return NULL;
}
