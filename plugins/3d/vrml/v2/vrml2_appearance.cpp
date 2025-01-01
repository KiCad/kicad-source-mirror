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
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying Appearance node with %zu children, %zu"
                     "references, and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
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

    if( WRL2NODES::WRL2_MATERIAL == type )
    {
        if( nullptr != material )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple material nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        material = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2NODES::WRL2_TEXTURETRANSFORM == type )
    {
        if( nullptr != textureTransform )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple textureTransform nodes." ),
                             __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        textureTransform = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( nullptr != texture )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; multiple texture nodes." ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    texture = aNode;
    return WRL2NODE::AddRefNode( aNode );
}


bool WRL2APPEARANCE::AddChildNode( WRL2NODE* aNode )
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

    if( WRL2NODES::WRL2_MATERIAL == type )
    {
        if( nullptr != material )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple material nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        material = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2NODES::WRL2_TEXTURETRANSFORM == type )
    {
        if( nullptr != textureTransform )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple textureTransform nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        textureTransform = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( nullptr != texture )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; multiple texture nodes." ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    texture = aNode;
    return WRL2NODE::AddChildNode( aNode );
}


bool WRL2APPEARANCE::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
    wxCHECK_MSG( aTopNode, false, wxT( "Invalid top node." ) );

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
        // material
        // texture
        // textureTransform

        if( !glob.compare( "material" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] could not read material information." ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }
        }
        else if( !glob.compare( "texture" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] could not read texture information." ),
                                 __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }
        }
        else if( !glob.compare( "textureTransform" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] could not read textureTransform information." ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              " * [INFO] bad Appearance %s.\n"
                                              " * [INFO] file: '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

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

    wxCHECK_MSG( aParent && ( ptype == S3D::SGTYPE_SHAPE ), nullptr,
                 wxString::Format( wxT( " * [BUG] Appearance does not have a Shape parent "
                                        "(parent ID: %d)." ), ptype ) );

    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating Appearance node with %zu children, %zu"
                     "references, and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );

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
