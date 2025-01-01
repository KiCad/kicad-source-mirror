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
#include "plugins/3dapi/ifsg_appearance.h"
#include "3d_cache/sg/sg_appearance.h"


extern char BadParent[];
extern char WrongParent[];


IFSG_APPEARANCE::IFSG_APPEARANCE( bool create )
{
    m_node = nullptr;

    if( !create )
        return ;

    m_node = new SGAPPEARANCE( nullptr );

    m_node->AssociateWrapper( &m_node );
}


IFSG_APPEARANCE::IFSG_APPEARANCE( SGNODE* aParent )
{
    m_node = new SGAPPEARANCE( nullptr );

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


IFSG_APPEARANCE::IFSG_APPEARANCE( IFSG_NODE& aParent )
{
    SGNODE* pp = aParent.GetRawPtr();

#ifdef DEBUG
    if( ! pp )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d %s" ), __FILE__, __FUNCTION__, __LINE__,
                    BadParent );
    }
#endif

    m_node = new SGAPPEARANCE( nullptr );

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


bool IFSG_APPEARANCE::Attach( SGNODE* aNode )
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );

    m_node = nullptr;

    if( !aNode )
        return false;

    if( S3D::SGTYPE_APPEARANCE != aNode->GetNodeType() )
    {
        return false;
    }

    m_node = aNode;
    m_node->AssociateWrapper( &m_node );

    return true;
}


bool IFSG_APPEARANCE::NewNode( SGNODE* aParent )
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );

    m_node = new SGAPPEARANCE( aParent );

    if( aParent != m_node->GetParent() )
    {
        wxLogTrace( MASK_3D_SG,
                    wxT( "%s:%s:%d  * [BUG] invalid SGNODE parent (%s) to SGAPPEARANCE" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    aParent->GetNodeTypeName( aParent->GetNodeType() ) );

        delete m_node;
        m_node = nullptr;
        return false;
    }

    m_node->AssociateWrapper( &m_node );

    return true;
}


bool IFSG_APPEARANCE::NewNode( IFSG_NODE& aParent )
{
    SGNODE* np = aParent.GetRawPtr();

    wxCHECK( np, false );

    return NewNode( np );
}


bool IFSG_APPEARANCE::SetEmissive( float aRVal, float aGVal, float aBVal )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetEmissive( aRVal, aGVal, aBVal );
}


bool IFSG_APPEARANCE::SetEmissive( const SGCOLOR* aRGBColor )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetEmissive( aRGBColor );
}


bool IFSG_APPEARANCE::SetEmissive( const SGCOLOR& aRGBColor )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetEmissive( aRGBColor );
}


bool IFSG_APPEARANCE::SetDiffuse( float aRVal, float aGVal, float aBVal )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetDiffuse( aRVal, aGVal, aBVal );
}


bool IFSG_APPEARANCE::SetDiffuse( const SGCOLOR* aRGBColor )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetDiffuse( aRGBColor );
}


bool IFSG_APPEARANCE::SetDiffuse( const SGCOLOR& aRGBColor )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetDiffuse( aRGBColor );
}


bool IFSG_APPEARANCE::SetSpecular( float aRVal, float aGVal, float aBVal )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetSpecular( aRVal, aGVal, aBVal );
}


bool IFSG_APPEARANCE::SetSpecular( const SGCOLOR* aRGBColor )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetSpecular( aRGBColor );
}


bool IFSG_APPEARANCE::SetSpecular( const SGCOLOR& aRGBColor )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetSpecular( aRGBColor );
}


bool IFSG_APPEARANCE::SetAmbient( float aRVal, float aGVal, float aBVal )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetAmbient( aRVal, aGVal, aBVal );
}


bool IFSG_APPEARANCE::SetAmbient( const SGCOLOR* aRGBColor )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetAmbient( aRGBColor );
}


bool IFSG_APPEARANCE::SetAmbient( const SGCOLOR& aRGBColor )
{
    wxCHECK( m_node, false );

    return ( (SGAPPEARANCE*) m_node )->SetAmbient( aRGBColor );
}


bool IFSG_APPEARANCE::SetShininess( float aShininess ) noexcept
{
    wxCHECK( m_node, false );

    if( aShininess < 0 || aShininess > 1.0 )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d  * [BUG] shininess out of range [0..1]" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    ( (SGAPPEARANCE*) m_node )->shininess = aShininess;

    return true;
}


bool IFSG_APPEARANCE::SetTransparency( float aTransparency ) noexcept
{
    wxCHECK( m_node, false );

    if( aTransparency < 0 || aTransparency > 1.0 )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] transparency out of range [0..1]" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    ( (SGAPPEARANCE*) m_node )->transparency = aTransparency;

    return true;
}
