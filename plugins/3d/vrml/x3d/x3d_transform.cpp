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
#include "x3d_transform.h"
#include "plugins/3dapi/ifsg_all.h"


X3DTRANSFORM::X3DTRANSFORM() : X3DNODE()
{
    m_Type = X3D_TRANSFORM;
    init();
}


X3DTRANSFORM::X3DTRANSFORM( X3DNODE* aParent ) : X3DNODE()
{
    m_Type = X3D_TRANSFORM;
    init();

    if( nullptr != aParent )
    {
        X3DNODES ptype = aParent->GetNodeType();

        if( X3D_TRANSFORM == ptype || X3D_SWITCH == ptype )
            m_Parent = aParent;
    }

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


X3DTRANSFORM::~X3DTRANSFORM()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying Transform with %zu children, %zu references, "
                     "and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


void X3DTRANSFORM::init()
{
    center.x = 0.0;
    center.y = 0.0;
    center.z = 0.0;

    scale.x = 1.0;
    scale.y = 1.0;
    scale.z = 1.0;

    translation = center;

    rotation.x = 0.0;
    rotation.y = 0.0;
    rotation.z = 1.0;

    scaleOrientation = rotation;

    bboxCenter = center;
    bboxSize = center;
}


void X3DTRANSFORM::readFields( wxXmlNode* aNode )
{
    // DEF
    // center
    // scale
    // translation
    // rotation
    // scaleOrientation
    // bboxCenter           (ignored)
    // bboxSize             (ignored)

    wxXmlAttribute* prop;

    // note: center/translation are multiplied by 2.54 to retain
    // legacy behavior of 1 X3D unit = 0.1 inch; the SG*
    // classes expect all units in mm.

    for( prop = aNode->GetAttributes(); prop != nullptr; prop = prop->GetNext() )
    {
        const wxString& pname = prop->GetName();

        if( pname == wxT( "DEF" ) )
        {
            m_Name = prop->GetValue();
            m_Dict->AddName( m_Name, this );
        }
        else if( pname == wxT( "center" ) )
        {
            X3D::ParseSFVec3( prop->GetValue(), center );
            center *= 2.54;
        }
        else if( pname == wxT( "scale" ) )
        {
            X3D::ParseSFVec3( prop->GetValue(), scale );
        }
        else if( pname == wxT( "translation" ) )
        {
            X3D::ParseSFVec3( prop->GetValue(), translation );
            translation *= 2.54;
        }
        else if( pname == wxT( "rotation" ) )
        {
            X3D::ParseSFRotation( prop->GetValue(), rotation );
        }
        else if( pname == wxT( "scaleOrientation" ) )
        {
            X3D::ParseSFRotation( prop->GetValue(), scaleOrientation );
        }
    }
}


bool X3DTRANSFORM::Read( wxXmlNode* aNode, X3DNODE* aTopNode, X3D_DICT& aDict )
{
    if( nullptr == aTopNode || nullptr == aNode )
        return false;

    m_Dict = &aDict;
    readFields( aNode );
    bool ok = false;

    for( wxXmlNode* child = aNode->GetChildren(); child != nullptr; child = child->GetNext() )
    {
        wxString name = child->GetName();

        if( name == wxT( "Transform" ) || name == wxT( "Group" ) )
            ok |= X3D::ReadTransform( child, this, aDict );
        else if( name == wxT( "Switch" ) )
            ok |= X3D::ReadSwitch( child, this, aDict );
        else if( name == wxT( "Shape" ) )
            ok |= X3D::ReadShape( child, this, aDict );

    }

    if( !ok )
        return false;

    if( !SetParent( aTopNode ) )
        return false;

    return true;
}


bool X3DTRANSFORM::SetParent( X3DNODE* aParent, bool doUnlink )
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


bool X3DTRANSFORM::AddChildNode( X3DNODE* aNode )
{
    if( nullptr == aNode )
        return false;

    X3DNODES tchild = aNode->GetNodeType();

    if( X3D_SWITCH != tchild && X3D_TRANSFORM != tchild && X3D_SHAPE != tchild )
        return false;

    std::list< X3DNODE* >::iterator sC = m_Children.begin();
    std::list< X3DNODE* >::iterator eC = m_Children.end();

    while( sC != eC )
    {
        if( *sC == aNode )
            return false;

        ++sC;
    }

    m_Children.push_back( aNode );

    if( aNode->GetParent() != this )
        aNode->SetParent( this );

    return true;
}


bool X3DTRANSFORM::AddRefNode( X3DNODE* aNode )
{
    if( nullptr == aNode )
        return false;

    X3DNODES tchild = aNode->GetNodeType();

    if( X3D_SWITCH != tchild && X3D_TRANSFORM != tchild && X3D_SHAPE != tchild )
        return false;

    std::list< X3DNODE* >::iterator sR = m_Refs.begin();
    std::list< X3DNODE* >::iterator eR = m_Refs.end();

    while( sR != eR )
    {
        if( *sR == aNode )
            return true;

        ++sR;
    }

    m_Refs.push_back( aNode );
    aNode->addNodeRef( this );

    return true;
}


SGNODE* X3DTRANSFORM::TranslateToSG( SGNODE* aParent )
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating Transform with %zu children, %zu references, "
                     "and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );

    if( m_Children.empty() && m_Refs.empty() )
        return nullptr;

    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( nullptr != aParent && ptype != S3D::SGTYPE_TRANSFORM )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( " * [BUG] Transform does not have a Transform parent (parent ID: %d)" ),
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

    IFSG_TRANSFORM txNode( aParent );

    std::list< X3DNODE* >::iterator sC = m_Children.begin();
    std::list< X3DNODE* >::iterator eC = m_Children.end();
    X3DNODES type;

    // Include only the following in a Transform node:
    // Shape
    // Switch
    // Transform
    // Inline
    bool test = false;  // set to true if there are any subnodes for display

    for( int i = 0; i < 2; ++i )
    {
        while( sC != eC )
        {
            type = (*sC)->GetNodeType();

            switch( type )
            {
            case X3D_SHAPE:
            case X3D_SWITCH:
            case X3D_TRANSFORM:

                if( nullptr != (*sC)->TranslateToSG( txNode.GetRawPtr() ) )
                    test = true;

                break;

            default:
                break;
            }

            ++ sC;
        }

        sC = m_Refs.begin();
        eC = m_Refs.end();
    }

    if( false == test )
    {
        txNode.Destroy();
        return nullptr;
    }

    txNode.SetScale( SGPOINT( scale.x, scale.y, scale.z ) );
    txNode.SetCenter( SGPOINT( center.x, center.y, center.z ) );
    txNode.SetTranslation( SGPOINT( translation.x, translation.y, translation.z ) );
    txNode.SetScaleOrientation( SGVECTOR( scaleOrientation.x, scaleOrientation.y,
                                          scaleOrientation.z ), scaleOrientation.w );
    txNode.SetRotation( SGVECTOR( rotation.x, rotation.y, rotation.z ), rotation.w );

    m_sgNode = txNode.GetRawPtr();

    return m_sgNode;
}
