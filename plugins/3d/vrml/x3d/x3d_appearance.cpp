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
#include <wx/xml/xml.h>
#include "x3d_ops.h"
#include "x3d_appearance.h"
#include "plugins/3dapi/ifsg_all.h"


X3DAPP::X3DAPP() : X3DNODE()
{
    m_Type = X3D_APPEARANCE;
    init();
}


X3DAPP::X3DAPP( X3DNODE* aParent ) : X3DNODE()
{
    m_Type = X3D_APPEARANCE;
    init();

    if( nullptr != aParent )
    {
        X3DNODES ptype = aParent->GetNodeType();

        if( X3D_SHAPE == ptype )
            m_Parent = aParent;
    }

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


X3DAPP::~X3DAPP()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying Appearance" ) );

    if( !m_MatName.empty() && m_Dict )
        m_Dict->DelName( m_MatName, this );
}


void X3DAPP::init()
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

    return;
}


void X3DAPP::readFields( wxXmlNode* aNode )
{
    // DEF
    // diffuseColor
    // emissiveColor
    // specularColor
    // ambientIntensity
    // shininess
    // transparency

    wxXmlAttribute* prop;

    for( prop = aNode->GetAttributes(); prop != nullptr; prop = prop->GetNext() )
    {
        const wxString& pname = prop->GetName();

        if( pname == wxT( "DEF" ) )
        {
            m_MatName = prop->GetValue();
            m_Dict->AddName( m_MatName, this );
        }
        else if( pname == wxT( "USE" ) )
        {
            X3DNODE* np = m_Dict->FindName( prop->GetValue() );

            if( nullptr != np && np->GetNodeType() == X3D_APPEARANCE )
            {
                X3DAPP* ap = (X3DAPP*) np;
                diffuseColor = ap->diffuseColor;
                emissiveColor = ap->emissiveColor;
                specularColor = ap->specularColor;
                ambientIntensity = ap->ambientIntensity;
                shininess = ap->shininess;
                transparency = ap->transparency;
            }
        }
        else if( pname == wxT( "diffuseColor" ) )
        {
            X3D::ParseSFVec3( prop->GetValue(), diffuseColor );
        }
        else if( pname == wxT( "emissiveColor" ) )
        {
            X3D::ParseSFVec3( prop->GetValue(), emissiveColor );
        }
        else if( pname == wxT( "specularColor" ) )
        {
            X3D::ParseSFVec3( prop->GetValue(), specularColor );
        }
        else if( pname == wxT( "ambientIntensity" ) )
        {
            X3D::ParseSFFloat( prop->GetValue(), ambientIntensity );
        }
        else if( pname == wxT( "shininess" ) )
        {
            X3D::ParseSFFloat( prop->GetValue(), shininess );
        }
        else if( pname == wxT( "transparency" ) )
        {
            X3D::ParseSFFloat( prop->GetValue(), transparency );
        }
    }
}


bool X3DAPP::Read( wxXmlNode* aNode, X3DNODE* aTopNode, X3D_DICT& aDict )
{
    if( nullptr == aTopNode || nullptr == aNode )
        return false;

    m_Dict = &aDict;
    wxXmlAttribute* prop;

    for( prop = aNode->GetAttributes(); prop != nullptr; prop = prop->GetNext() )
    {
        const wxString& pname = prop->GetName();

        if( pname == wxT( "DEF" ) )
        {
            m_Name = prop->GetValue();
            m_Dict->AddName( m_Name, this );
        }
    }

    wxXmlNode* pmat = nullptr;

    for( wxXmlNode* child = aNode->GetChildren(); child != nullptr; child = child->GetNext() )
    {
        if( child->GetName() == wxT( "Material" ) )
            pmat = child;

    }

    if( nullptr == pmat )
        return false;

    readFields( pmat );

    if( !SetParent( aTopNode ) )
        return false;

    return true;
}


bool X3DAPP::SetParent( X3DNODE* aParent, bool doUnlink )
{
    if( aParent == m_Parent )
        return true;

    if( nullptr != aParent )
    {
        X3DNODES nt = aParent->GetNodeType();

        if( nt != X3D_SHAPE )
            return false;
    }

    if( nullptr != m_Parent && doUnlink )
        m_Parent->unlinkChildNode( this );

    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


bool X3DAPP::AddChildNode( X3DNODE* aNode )
{
    return false;
}


bool X3DAPP::AddRefNode( X3DNODE* aNode )
{
    return false;
}


SGNODE* X3DAPP::TranslateToSG( SGNODE* aParent )
{
    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( nullptr != aParent && ptype != S3D::SGTYPE_SHAPE )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( " * [BUG] Appearance does not have a Shape parent (parent ID: %d)" ),
                    ptype );

        return nullptr;
    }

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
