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
#include "vrml2_shape.h"
#include "plugins/3dapi/ifsg_all.h"
#include "vrml2_faceset.h"


WRL2SHAPE::WRL2SHAPE() : WRL2NODE()
{
    appearance = nullptr;
    geometry = nullptr;
    m_Type = WRL2NODES::WRL2_SHAPE;
}


WRL2SHAPE::WRL2SHAPE( WRL2NODE* aParent ) : WRL2NODE()
{
    appearance = nullptr;
    geometry = nullptr;
    m_Type = WRL2NODES::WRL2_SHAPE;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL2SHAPE::~WRL2SHAPE()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying Shape node with %zu children, %zu"
                     "references, and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


bool WRL2SHAPE::isDangling( void )
{
    // this node is dangling unless it has a parent of type:
    // WRL2_TRANSFORM
    // WRL2_SWITCH

    if( nullptr == m_Parent
        || ( m_Parent->GetNodeType() != WRL2NODES::WRL2_TRANSFORM
             && m_Parent->GetNodeType() != WRL2NODES::WRL2_SWITCH ) )
        return true;

    return false;
}


bool WRL2SHAPE::AddRefNode( WRL2NODE* aNode )
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

    if( WRL2NODES::WRL2_APPEARANCE == type )
    {
        if( nullptr != appearance )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple appearance nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        appearance = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( nullptr != geometry )
    {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple geometry nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    geometry = aNode;
    return WRL2NODE::AddRefNode( aNode );
}


bool WRL2SHAPE::AddChildNode( WRL2NODE* aNode )
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

    if( WRL2NODES::WRL2_APPEARANCE == type )
    {
        if( nullptr != appearance )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple appearance nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        appearance = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( nullptr != geometry )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; multiple geometry nodes." ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    geometry = aNode;
    return WRL2NODE::AddChildNode( aNode );
}


bool WRL2SHAPE::checkNodeType( WRL2NODES aType )
{
    switch( aType )
    {
    case WRL2NODES::WRL2_APPEARANCE:
    case WRL2NODES::WRL2_BOX:
    case WRL2NODES::WRL2_CONE:
    case WRL2NODES::WRL2_CYLINDER:
    case WRL2NODES::WRL2_ELEVATIONGRID:
    case WRL2NODES::WRL2_EXTRUSION:
    case WRL2NODES::WRL2_INDEXEDFACESET:
    case WRL2NODES::WRL2_INDEXEDLINESET:
    case WRL2NODES::WRL2_POINTSET:
    case WRL2NODES::WRL2_SPHERE:
    case WRL2NODES::WRL2_TEXT:
        break;

    default:
        return false;
        break;
    }

    return true;
}


bool WRL2SHAPE::Read( WRLPROC& proc, WRL2BASE* aTopNode )
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
        // appearance
        // geometry
        if( !glob.compare( "appearance" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] could not read appearance node information." ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }
        }
        else if( !glob.compare( "geometry" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] could not read geometry node information." ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }
        }
        else
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] invalid Shape %s.\n"
                             " * [INFO] file: '%s'\n" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }
    }   // while( true ) -- reading contents of Shape{}

    return true;
}


SGNODE* WRL2SHAPE::TranslateToSG( SGNODE* aParent )
{
    if( nullptr == geometry )
        return nullptr;

    WRL2NODES geomType = geometry->GetNodeType();

    switch( geomType )
    {
    case WRL2NODES::WRL2_INDEXEDLINESET:
    case WRL2NODES::WRL2_POINTSET:
    case WRL2NODES::WRL2_TEXT:
        return nullptr;
        break;

    default:
        break;
    }

    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating Shape with %zu children, %zu references, and"
                     "%zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );

    bool vcolors = false;

    if( WRL2NODES::WRL2_INDEXEDFACESET == geometry->GetNodeType() )
        vcolors = ((WRL2FACESET*)geometry)->HasColors();

    // if there is no appearance, make use of the per vertex colors if available
    if( nullptr == appearance )
    {
        if( WRL2NODES::WRL2_INDEXEDFACESET != geometry->GetNodeType() )
            return nullptr;

        if( !vcolors )
            return nullptr;
    }

    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    wxCHECK_MSG( aParent && ( ptype == S3D::SGTYPE_TRANSFORM ), nullptr,
                 wxString::Format( wxT( "Shape does not have a Transform parent (parent "
                                        "ID: %d)." ), ptype ) );

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

    IFSG_SHAPE shNode( aParent );

    SGNODE* pShape = shNode.GetRawPtr();
    SGNODE* pGeom = geometry->TranslateToSG( pShape );

    if( nullptr == pGeom )
    {
        // this can happen if a VRML file contains
        // empty point or index sets
        shNode.Destroy();
        return nullptr;
    }

    SGNODE* pApp = nullptr;

    if( nullptr != appearance )
        pApp = appearance->TranslateToSG( pShape );

    if( nullptr != appearance && nullptr == pApp )
    {
        IFSG_FACESET tmp( false );
        tmp.Attach( pGeom );
        tmp.Destroy();
        shNode.Destroy();
        return nullptr;
    }

    m_sgNode = shNode.GetRawPtr();

    return m_sgNode;
}


void WRL2SHAPE::unlinkChildNode( const WRL2NODE* aNode )
{
    if( nullptr == aNode )
        return;

    if( aNode == appearance )
        appearance = nullptr;
    else if( aNode == geometry )
        geometry = nullptr;

    WRL2NODE::unlinkChildNode( aNode );
}


void WRL2SHAPE::unlinkRefNode( const WRL2NODE* aNode )
{
    if( nullptr == aNode )
        return;

    if( aNode == appearance )
        appearance = nullptr;
    else if( aNode == geometry )
        geometry = nullptr;

    WRL2NODE::unlinkRefNode( aNode );
}
