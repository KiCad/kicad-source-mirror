/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include "plugins/3dapi/ifsg_transform.h"
#include "3d_cache/sg/scenegraph.h"


extern char WrongParent[];


IFSG_TRANSFORM::IFSG_TRANSFORM( bool create )
{
    m_node = nullptr;

    if( !create )
        return;

    m_node = new SCENEGRAPH( nullptr );

    m_node->AssociateWrapper( &m_node );
}


IFSG_TRANSFORM::IFSG_TRANSFORM( SGNODE* aParent )
{
    m_node = new SCENEGRAPH( nullptr );

    if( !m_node->SetParent( aParent ) )
    {
        delete m_node;
        m_node = nullptr;

        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d %s" ), __FILE__, __FUNCTION__, __LINE__,
                    WrongParent );

        return;
    }

    m_node->AssociateWrapper( &m_node );
}


bool IFSG_TRANSFORM::Attach( SGNODE* aNode )
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );

    m_node = nullptr;

    if( !aNode )
        return false;

    if( S3D::SGTYPE_TRANSFORM != aNode->GetNodeType() )
    {
        return false;
    }

    m_node = aNode;
    m_node->AssociateWrapper( &m_node );

    return true;
}


bool IFSG_TRANSFORM::NewNode( SGNODE* aParent )
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );

    m_node = new SCENEGRAPH( aParent );

    if( aParent != m_node->GetParent() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] invalid SGNODE parent (%s) to SCENEGRAPH" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    aParent->GetNodeTypeName( aParent->GetNodeType() ) );

        delete m_node;
        m_node = nullptr;
        return false;
    }

    m_node->AssociateWrapper( &m_node );

    return true;
}


bool IFSG_TRANSFORM::NewNode( IFSG_NODE& aParent )
{
    SGNODE* np = aParent.GetRawPtr();

    wxCHECK( np, false );

    return NewNode( np );
}


bool IFSG_TRANSFORM::SetRotation( const SGVECTOR& aRotationAxis, double aAngle )
{
    wxCHECK( m_node, false );

    ( (SCENEGRAPH*) m_node )->rotation_axis  = aRotationAxis;
    ( (SCENEGRAPH*) m_node )->rotation_angle = aAngle;

    return true;
}


bool IFSG_TRANSFORM::SetScale( const SGPOINT& aScale ) noexcept
{
    wxCHECK( m_node, false );

    ( (SCENEGRAPH*) m_node )->scale = aScale;

    return true;
}


bool IFSG_TRANSFORM::SetScale( double aScale )
{
    wxCHECK( m_node, false );

    if( aScale < 1e-8 && aScale > -1e-8 )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] |scale| is < 1e-8 - this seems strange" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    ( (SCENEGRAPH*) m_node )->scale = SGPOINT( aScale, aScale, aScale );

    return true;
}


bool IFSG_TRANSFORM::SetTranslation( const SGPOINT& aTranslation ) noexcept
{
    wxCHECK( m_node, false );

    ( (SCENEGRAPH*) m_node )->translation = aTranslation;

    return true;
}


bool IFSG_TRANSFORM::SetScaleOrientation( const SGVECTOR& aScaleAxis, double aAngle )
{
    wxCHECK( m_node, false );

    ( (SCENEGRAPH*) m_node )->scale_axis  = aScaleAxis;
    ( (SCENEGRAPH*) m_node )->scale_angle = aAngle;

    return true;
}


bool IFSG_TRANSFORM::SetCenter( const SGPOINT& aCenter ) noexcept
{
    wxCHECK( m_node, false );

    ( (SCENEGRAPH*) m_node )->center = aCenter;

    return true;
}
