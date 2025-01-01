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
#include "vrml2_material.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2MATERIAL::WRL2MATERIAL() : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2NODES::WRL2_MATERIAL;
}


WRL2MATERIAL::WRL2MATERIAL( WRL2NODE* aParent ) : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2NODES::WRL2_MATERIAL;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL2MATERIAL::~WRL2MATERIAL()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying Material node." ) );
}


void WRL2MATERIAL::setDefaults( void )
{
    // default material values as per VRML2 spec
    diffuseColor.x = 0.8f;
    diffuseColor.y = 0.8f;
    diffuseColor.z = 0.8f;

    emissiveColor.x = 0.0f;
    emissiveColor.y = 0.0f;
    emissiveColor.z = 0.0f;

    specularColor = emissiveColor;

    ambientIntensity = 0.2f;
    shininess = 0.2f;
    transparency = 0.0f;
}


bool WRL2MATERIAL::isDangling( void )
{
    // this node is dangling unless it has a parent of type WRL2NODES::WRL2_APPEARANCE

    if( nullptr == m_Parent || m_Parent->GetNodeType() != WRL2NODES::WRL2_APPEARANCE )
        return true;

    return false;
}


bool WRL2MATERIAL::AddRefNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddRefNode is not applicable." ) );
}


bool WRL2MATERIAL::AddChildNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddChildNode is not applicable." ) );
}


bool WRL2MATERIAL::Read( WRLPROC& proc, WRL2BASE* aTopNode )
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
        // ambientIntensity
        // diffuseColor
        // emissiveColor
        // shininess
        // specularColor
        // transparency

        if( !glob.compare( "specularColor" ) )
        {
            if( !proc.ReadSFVec3f( specularColor ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid specularColor set %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "diffuseColor" ) )
        {
            if( !proc.ReadSFVec3f( diffuseColor ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid diffuseColor set %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "emissiveColor" ) )
        {
            if( !proc.ReadSFVec3f( emissiveColor ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid emissiveColor set %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "shininess" ) )
        {
            if( !proc.ReadSFFloat( shininess ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid shininess set %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "transparency" ) )
        {
            if( !proc.ReadSFFloat( transparency ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid transparency set %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "ambientIntensity" ) )
        {
            if( !proc.ReadSFFloat( ambientIntensity ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid ambientIntensity set %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              " * [INFO] invalid Material %s.\n"
                                              " * [INFO] file: '%s'\n" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }
    }   // while( true ) -- reading contents of Material{}

    return true;
}


SGNODE* WRL2MATERIAL::TranslateToSG( SGNODE* aParent )
{
    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    wxCHECK_MSG( aParent && ( ptype == S3D::SGTYPE_SHAPE ), nullptr,
                 wxString::Format( wxT( "IndexedFaceSet does not have a Shape parent (parent "
                                        "ID: %d)." ), ptype ) );

    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating IndexedFaceSet with %zu children, %zu references, and"
                     "%zu back pointers." ),
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

    IFSG_APPEARANCE matNode( aParent );
    matNode.SetEmissive( emissiveColor.x, emissiveColor.y, emissiveColor.z );
    matNode.SetSpecular( specularColor.x, specularColor.y, specularColor.z );
    matNode.SetDiffuse( diffuseColor.x, diffuseColor.y, diffuseColor.z );
    float ambr = ambientIntensity * diffuseColor.x;
    float ambg = ambientIntensity * diffuseColor.y;
    float ambb = ambientIntensity * diffuseColor.z;
    matNode.SetAmbient( ambr, ambg, ambb );
    matNode.SetShininess( shininess );
    matNode.SetTransparency( transparency );
    m_sgNode = matNode.GetRawPtr();

    return m_sgNode;
}
