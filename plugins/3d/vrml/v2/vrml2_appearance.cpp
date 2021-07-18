/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "vrml2_appearance.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2APPEARANCE::WRL2APPEARANCE() : WRL2NODE()
{
    material = nullptr;
    texture = nullptr;
    textureTransform = nullptr;
    m_Type = WRL2NODES::WRL2_APPEARANCE;
}


WRL2APPEARANCE::WRL2APPEARANCE( WRL2NODE* aParent ) : WRL2NODE()
{
    material = nullptr;
    texture = nullptr;
    textureTransform = nullptr;
    m_Type = WRL2NODES::WRL2_APPEARANCE;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL2APPEARANCE::~WRL2APPEARANCE()
{
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 2 )
    do {
        std::ostringstream ostr;
        ostr << " * [INFO] Destroying Appearance with " << m_Children.size();
        ostr << " children, " << m_Refs.size() << " references and ";
        ostr << m_BackPointers.size() << " backpointers";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
#endif
}


bool WRL2APPEARANCE::checkNodeType( WRL2NODES aType )
{
    switch( aType )
    {
    case WRL2NODES::WRL2_MATERIAL:
    case WRL2NODES::WRL2_IMAGETEXTURE:
    case WRL2NODES::WRL2_PIXELTEXTURE:
    case WRL2NODES::WRL2_MOVIETEXTURE:
    case WRL2NODES::WRL2_TEXTURETRANSFORM:
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

    if( nullptr == m_Parent || m_Parent->GetNodeType() != WRL2NODES::WRL2_SHAPE )
        return true;

    return false;
}


bool WRL2APPEARANCE::AddRefNode( WRL2NODE* aNode )
{
    if( nullptr == aNode )
    {
#ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] NULL passed for aNode";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
#endif

        return false;
    }

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; unexpected child node '";
            ostr << aNode->GetNodeTypeName( type ) << "'";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
#endif

        return false;
    }

    if( WRL2NODES::WRL2_MATERIAL == type )
    {
        if( nullptr != material )
        {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad file format; multiple material nodes";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
#endif

            return false;
        }

        material = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2NODES::WRL2_TEXTURETRANSFORM == type )
    {
        if( nullptr != textureTransform )
        {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad file format; multiple textureTransform nodes";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
#endif

            return false;
        }

        textureTransform = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( nullptr != texture )
    {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; multiple texture nodes";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
#endif

        return false;
    }

    texture = aNode;
    return WRL2NODE::AddRefNode( aNode );
}


bool WRL2APPEARANCE::AddChildNode( WRL2NODE* aNode )
{
    if( nullptr == aNode )
    {
#ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] NULL passed for aNode";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
#endif

        return false;
    }

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; unexpected child node '";
            ostr << aNode->GetNodeTypeName( type ) << "'";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
#endif

        return false;
    }

    if( WRL2NODES::WRL2_MATERIAL == type )
    {
        if( nullptr != material )
        {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad file format; multiple material nodes";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
#endif

            return false;
        }

        material = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2NODES::WRL2_TEXTURETRANSFORM == type )
    {
        if( nullptr != textureTransform )
        {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad file format; multiple textureTransform nodes";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
#endif

            return false;
        }

        textureTransform = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( nullptr != texture )
    {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; multiple texture nodes";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
#endif

        return false;
    }

    texture = aNode;
    return WRL2NODE::AddChildNode( aNode );
}


bool WRL2APPEARANCE::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
    if( nullptr == aTopNode )
    {
#ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] aTopNode is NULL";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
#endif

        return false;
    }

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
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << proc.GetError();
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
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
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] could not read material information";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
#endif

                return false;
            }
        }
        else if( !glob.compare( "texture" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] could not read texture information";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
#endif

                return false;
            }
        }
        else if( !glob.compare( "textureTransform" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] could not read textureTransform information";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
#endif

                return false;
            }
        }
        else
        {
#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad Appearance at line " << line << ", column ";
                ostr << column << "\n";
                ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
#endif

            return false;
        }
    }   // while( true ) -- reading contents of Appearance{}

    return true;
}


SGNODE* WRL2APPEARANCE::TranslateToSG( SGNODE* aParent )
{
    if( nullptr == material && nullptr == texture )
        return nullptr;

    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( nullptr != aParent && ptype != S3D::SGTYPE_SHAPE )
    {
#ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] Appearance does not have a Shape parent (parent ID: ";
            ostr << ptype << ")";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
#endif

        return nullptr;
    }

#if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 2 )
    do {
        std::ostringstream ostr;
        ostr << " * [INFO] Translating Appearance with " << m_Children.size();
        ostr << " children, " << m_Refs.size() << " references and ";
        ostr << m_BackPointers.size() << " backpointers";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
#endif

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

    if( nullptr != texture )
    {
        // use a default gray appearance
        IFSG_APPEARANCE matNode( aParent );
        matNode.SetEmissive( 0.0f, 0.0f, 0.0f );
        matNode.SetSpecular( 0.65f, 0.65f, 0.65f );
        matNode.SetDiffuse( 0.65f, 0.65f, 0.65f );
        // default ambient
        matNode.SetShininess( 0.2f );
        matNode.SetTransparency( 0.0f );
        m_sgNode = matNode.GetRawPtr();

        return m_sgNode;
    }

    m_sgNode = material->TranslateToSG( aParent );

    return m_sgNode;
}


void WRL2APPEARANCE::unlinkChildNode( const WRL2NODE* aNode )
{
    if( nullptr == aNode )
        return;

    if( aNode->GetParent() == this )
    {
        if( aNode == material )
            material = nullptr;
        else if( aNode == texture )
            texture = nullptr;
        else if( aNode == textureTransform )
            textureTransform = nullptr;

    }

    WRL2NODE::unlinkChildNode( aNode );
}


void WRL2APPEARANCE::unlinkRefNode( const WRL2NODE* aNode )
{
    if( nullptr == aNode )
        return;

    if( aNode->GetParent() != this )
    {
        if( aNode == material )
            material = nullptr;
        else if( aNode == texture )
            texture = nullptr;
        else if( aNode == textureTransform )
            textureTransform = nullptr;
    }

    WRL2NODE::unlinkRefNode( aNode );
}
