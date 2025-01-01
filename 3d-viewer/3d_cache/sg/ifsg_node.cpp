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

#include "plugins/3dapi/ifsg_node.h"
#include "3d_cache/sg/sg_node.h"
#include "plugins/3dapi/ifsg_api.h"


// collection of common error strings used by the wrappers
char BadObject[] = " * [BUG] operating on an invalid wrapper (object may have been deleted)";
char BadOperand[] = " * [BUG] parameter aNode is an invalid wrapper; its data may have been deleted";
char BadParent[] = " * [BUG] invalid parent node (data may have been deleted)";
char WrongParent[] = " * [BUG] parent node type is incompatible";


IFSG_NODE::IFSG_NODE()
{
    m_node = nullptr;
}


IFSG_NODE::~IFSG_NODE()
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );
}


void IFSG_NODE::Destroy( void )
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );

    delete m_node;
    m_node = nullptr;
}


SGNODE* IFSG_NODE::GetRawPtr( void ) noexcept
{
    return m_node;
}


S3D::SGTYPES IFSG_NODE::GetNodeType( void ) const
{
    wxCHECK( m_node, S3D::SGTYPE_END );

    return m_node->GetNodeType();
}


SGNODE* IFSG_NODE::GetParent( void ) const
{
    wxCHECK( m_node, nullptr );

    return m_node->GetParent();
}


bool IFSG_NODE::SetParent( SGNODE* aParent )
{
    wxCHECK( m_node, false );

    return m_node->SetParent( aParent );
}


const char* IFSG_NODE::GetName( void )
{
    wxCHECK( m_node, nullptr );

    return m_node->GetName();
}


bool IFSG_NODE::SetName( const char* aName )
{
    wxCHECK( m_node, false );

    m_node->SetName( aName );
    return true;
}


const char* IFSG_NODE::GetNodeTypeName( S3D::SGTYPES aNodeType ) const
{
    wxCHECK( m_node, nullptr );

    return m_node->GetNodeTypeName( aNodeType );
}


SGNODE* IFSG_NODE::FindNode( const char* aNodeName )
{
    wxCHECK( m_node, nullptr );

    return m_node->FindNode( aNodeName, nullptr );
}


bool IFSG_NODE::AddRefNode( SGNODE* aNode )
{
    wxCHECK( m_node, false );

    return m_node->AddRefNode( aNode );
}


bool IFSG_NODE::AddRefNode( IFSG_NODE& aNode )
{
    wxCHECK( m_node, false );

    SGNODE* np = aNode.GetRawPtr();

    wxCHECK( np, false );

    return m_node->AddRefNode( np );
}


bool IFSG_NODE::AddChildNode( SGNODE* aNode )
{
    wxCHECK( m_node, false );

    return m_node->AddChildNode( aNode );
}


bool IFSG_NODE::AddChildNode( IFSG_NODE& aNode )
{
    wxCHECK( m_node, false );

    SGNODE* np = aNode.GetRawPtr();

    wxCHECK( np, false );

    return m_node->AddChildNode( np );
}
