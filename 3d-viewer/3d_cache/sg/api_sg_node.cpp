/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
#include "sg_api.h"
#include "sg_node.h"

S3D::API_SGNODE::API_SGNODE()
{
    node = NULL;
    nodeType = SGTYPE_END;  // signal an invalid type by default

    return;
}


SGNODE* S3D::API_SGNODE::GetNode( void )
{
    return node;
}


bool S3D::API_SGNODE::AttachNode( SGNODE* aNode )
{
    if( NULL == aNode )
    {
        node = NULL;
        return true;
    }

    if( aNode->GetNodeType() != nodeType )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] object with node type " << aNode->GetNodeType();
        std::cerr << " is being attached to API node type " << nodeType << "\n";
        return false;
    }

    node = aNode;
    return true;
}


bool S3D::API_SGNODE::GetNodeType( S3D::SGTYPES& aNodeType ) const
{
    if( NULL == node )
    {
        aNodeType = SGTYPE_END;
        return false;
    }

    aNodeType = node->GetNodeType();
    return true;
}


bool S3D::API_SGNODE::GetParent( SGNODE const*& aParent ) const
{
    if( NULL == node )
    {
        aParent = NULL;
        return false;
    }

    aParent = node->GetParent();
    return true;
}


bool S3D::API_SGNODE::SetParent( SGNODE* aParent )
{
    if( NULL == node )
        return false;

    return node->SetParent( aParent );
}


bool S3D::API_SGNODE::GetName( const char*& aName )
{
    if( NULL == node )
    {
        aName = NULL;
        return false;
    }

    aName = node->GetName();
    return true;
}


bool S3D::API_SGNODE::SetName( const char *aName )
{
    if( NULL == node )
        return false;

    node->SetName( aName );
    return true;
}


bool S3D::API_SGNODE::GetNodeTypeName( S3D::SGTYPES aNodeType, const char*& aName ) const
{
    if( NULL == node )
    {
        aName = NULL;
        return false;
    }

    aName = node->GetNodeTypeName( aNodeType );
    return true;
}


bool S3D::API_SGNODE::FindNode( const char *aNodeName, const SGNODE *aCaller, SGNODE*& aNode )
{
    if( NULL == node )
    {
        aNode = NULL;
        return false;
    }

    aNode = node->FindNode( aNodeName, aCaller );

    if( NULL == aNode )
        return false;

    return true;
}
