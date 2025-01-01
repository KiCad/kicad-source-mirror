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

#include "plugins/3dapi/ifsg_coords.h"
#include "3d_cache/sg/sg_coords.h"


extern char BadParent[];
extern char WrongParent[];


IFSG_COORDS::IFSG_COORDS( bool create )
{
    m_node = nullptr;

    if( !create )
        return ;

    m_node = new SGCOORDS( nullptr );

    m_node->AssociateWrapper( &m_node );
}


IFSG_COORDS::IFSG_COORDS( SGNODE* aParent )
{
    m_node = new SGCOORDS( nullptr );

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


IFSG_COORDS::IFSG_COORDS( IFSG_NODE& aParent )
{
    SGNODE* pp = aParent.GetRawPtr();

#ifdef DEBUG
    if( !pp )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d %s" ), __FILE__, __FUNCTION__, __LINE__,
                    BadParent );
    }
#endif

    m_node = new SGCOORDS( nullptr );

    if( !m_node->SetParent( pp ) )
    {
        delete m_node;
        m_node = nullptr;

        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d %s" ), __FILE__, __FUNCTION__, __LINE__,
                    WrongParent );

        return;
    }

    m_node->AssociateWrapper( &m_node );
}


bool IFSG_COORDS::Attach( SGNODE* aNode )
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );

    m_node = nullptr;

    if( !aNode )
        return false;

    if( S3D::SGTYPE_COORDS != aNode->GetNodeType() )
    {
        return false;
    }

    m_node = aNode;
    m_node->AssociateWrapper( &m_node );

    return true;
}


bool IFSG_COORDS::NewNode( SGNODE* aParent )
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );

    m_node = new SGCOORDS( aParent );

    if( aParent != m_node->GetParent() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] invalid SGNODE parent (%s) to SGCOORDS" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    aParent->GetNodeTypeName( aParent->GetNodeType() ) );

        delete m_node;
        m_node = nullptr;
        return false;
    }

    m_node->AssociateWrapper( &m_node );

    return true;
}


bool IFSG_COORDS::NewNode( IFSG_NODE& aParent )
{
    SGNODE* np = aParent.GetRawPtr();

    wxCHECK( np, false );

    return NewNode( np );
}


bool IFSG_COORDS::GetCoordsList( size_t& aListSize, SGPOINT*& aCoordsList )
{
    wxCHECK( m_node, false );

    return ( (SGCOORDS*) m_node )->GetCoordsList( aListSize, aCoordsList );
}


bool IFSG_COORDS::SetCoordsList( size_t aListSize, const SGPOINT* aCoordsList )
{
    wxCHECK( m_node, false );

    ( (SGCOORDS*) m_node )->SetCoordsList( aListSize, aCoordsList );

    return true;
}


bool IFSG_COORDS::AddCoord( double aXValue, double aYValue, double aZValue )
{
    wxCHECK( m_node, false );

    ( (SGCOORDS*) m_node )->AddCoord( aXValue, aYValue, aZValue );

    return true;
}


bool IFSG_COORDS::AddCoord( const SGPOINT& aPoint )
{
    wxCHECK( m_node, false );

    ( (SGCOORDS*) m_node )->AddCoord( aPoint );

    return true;
}
