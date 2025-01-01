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
#include <wx/xml/xml.h>
#include <wx/log.h>
#include "x3d_ops.h"
#include "x3d_shape.h"
#include "plugins/3dapi/ifsg_all.h"


X3DSHAPE::X3DSHAPE() : X3DNODE()
{
    m_Type = X3D_SHAPE;
    appearance = nullptr;
    geometry = nullptr;
}


X3DSHAPE::X3DSHAPE( X3DNODE* aParent ) : X3DNODE()
{
    m_Type = X3D_SHAPE;
    appearance = nullptr;
    geometry = nullptr;

    if( nullptr != aParent )
    {
        X3DNODES ptype = aParent->GetNodeType();

        if( X3D_TRANSFORM == ptype || X3D_SWITCH == ptype )
            m_Parent = aParent;
    }

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


X3DSHAPE::~X3DSHAPE()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying Shape with %zu children, %zu references, "
                     "%and ul back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


bool X3DSHAPE::Read( wxXmlNode* aNode, X3DNODE* aTopNode, X3D_DICT& aDict )
{
    if( nullptr == aTopNode || nullptr == aNode )
        return false;

    if( nullptr != appearance || nullptr != geometry )
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

    for( wxXmlNode* child = aNode->GetChildren(); child != nullptr; child = child->GetNext() )
    {
        wxString name = child->GetName();

        if( name == wxT( "Appearance" ) && nullptr == appearance )
            X3D::ReadAppearance( child, this, aDict );
        else if( name == wxT( "IndexedFaceSet" ) && nullptr == geometry )
            X3D::ReadIndexedFaceSet( child, this, aDict );
    }

    if( nullptr == appearance || nullptr == geometry )
        return false;

    if( !SetParent( aTopNode ) )
        return false;

    return true;
}


bool X3DSHAPE::SetParent( X3DNODE* aParent, bool doUnlink )
{
    if( aParent == m_Parent )
        return true;

    if( nullptr != aParent )
    {
        X3DNODES nt = aParent->GetNodeType();

        if( nt != X3D_SWITCH && nt != X3D_TRANSFORM )
            return false;
    }

    if( nullptr != m_Parent && doUnlink )
        m_Parent->unlinkChildNode( this );

    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


bool X3DSHAPE::AddChildNode( X3DNODE* aNode )
{
    if( nullptr == aNode )
        return false;

    X3DNODES tchild = aNode->GetNodeType();

    if( X3D_APPEARANCE != tchild && X3D_INDEXED_FACE_SET != tchild )
        return false;

    std::list< X3DNODE* >::iterator sC = m_Children.begin();
    std::list< X3DNODE* >::iterator eC = m_Children.end();

    while( sC != eC )
    {
        if( *sC == aNode )
            return false;

        ++sC;
    }

    if( X3D_APPEARANCE == tchild )
    {
        if( nullptr == appearance )
        {
            m_Children.push_back( aNode );
            appearance = aNode;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if( nullptr == geometry )
        {
            m_Children.push_back( aNode );
            geometry = aNode;
        }
        else
        {
            return false;
        }
    }

    if( aNode->GetParent() != this )
        aNode->SetParent( this );

    return true;
}


bool X3DSHAPE::AddRefNode( X3DNODE* aNode )
{
    if( nullptr == aNode )
        return false;

    X3DNODES tchild = aNode->GetNodeType();

    if( X3D_APPEARANCE != tchild && X3D_INDEXED_FACE_SET != tchild )
        return false;

    std::list< X3DNODE* >::iterator sR = m_Refs.begin();
    std::list< X3DNODE* >::iterator eR = m_Refs.end();

    while( sR != eR )
    {
        if( *sR == aNode )
            return false;

        ++sR;
    }

    if( X3D_APPEARANCE == tchild )
    {
        if( nullptr == appearance )
        {
            m_Refs.push_back( aNode );
            aNode->addNodeRef( this );
            appearance = aNode;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if( nullptr == geometry )
        {
            m_Refs.push_back( aNode );
            aNode->addNodeRef( this );
            geometry = aNode;
        }
        else
        {
            return false;
        }
    }

    return true;
}


SGNODE* X3DSHAPE::TranslateToSG( SGNODE* aParent )
{
    if( nullptr == geometry || nullptr == appearance )
        return nullptr;

    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating Shape with %zu children, %zu references, "
                     "and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );

    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( nullptr != aParent && ptype != S3D::SGTYPE_TRANSFORM )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( " * [BUG] Shape does not have a Transform parent (parent ID: %d)" ),
                    ptype );

        return nullptr;
    }

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
    SGNODE* pApp = appearance->TranslateToSG( pShape );

    if( nullptr == pApp || nullptr == pGeom )
    {
        if( pGeom )
        {
            IFSG_FACESET tmp( false );
            tmp.Attach( pGeom );
            tmp.Destroy();
        }

        if( pApp )
        {
            IFSG_APPEARANCE tmp( false );
            tmp.Attach( pApp );
            tmp.Destroy();
        }

        shNode.Destroy();
        return nullptr;
    }

    m_sgNode = shNode.GetRawPtr();

    return m_sgNode;
}


void X3DSHAPE::unlinkChildNode( const X3DNODE* aNode )
{
    if( nullptr == aNode )
        return;

    if( aNode == appearance )
        appearance = nullptr;
    else if( aNode == geometry )
        geometry = nullptr;

    X3DNODE::unlinkChildNode( aNode );
}


void X3DSHAPE::unlinkRefNode( const X3DNODE* aNode )
{
    if( nullptr == aNode )
        return;

    if( aNode == appearance )
        appearance = nullptr;
    else if( aNode == geometry )
        geometry = nullptr;

    X3DNODE::unlinkRefNode( aNode );
}
