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
#include <sstream>
#include <wx/log.h>
#include "plugins/3dapi/ifsg_appearance.h"
#include "3d_cache/sg/sg_appearance.h"


extern char BadObject[];
extern char BadOperand[];
extern char BadParent[];
extern char WrongParent[];


IFSG_APPEARANCE::IFSG_APPEARANCE( bool create )
{
    m_node = NULL;

    if( !create )
        return ;

    m_node = new SGAPPEARANCE( NULL );

    if( m_node )
        m_node->AssociateWrapper( &m_node );

    return;
}


IFSG_APPEARANCE::IFSG_APPEARANCE( SGNODE* aParent )
{
    m_node = new SGAPPEARANCE( NULL );

    if( m_node )
    {
        if( !m_node->SetParent( aParent ) )
        {
            delete m_node;
            m_node = NULL;

            #ifdef DEBUG
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << WrongParent;
                wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return;
        }

        m_node->AssociateWrapper( &m_node );
    }

    return;
}


IFSG_APPEARANCE::IFSG_APPEARANCE( IFSG_NODE& aParent )
{
    SGNODE* pp = aParent.GetRawPtr();

    #ifdef DEBUG
    if( ! pp )
    {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadParent;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    }
    #endif

    m_node = new SGAPPEARANCE( NULL );

    if( m_node )
    {
        if( !m_node->SetParent( pp ) )
        {
            delete m_node;
            m_node = NULL;

            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << WrongParent;
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return;
        }

        m_node->AssociateWrapper( &m_node );
    }

    return;
}


bool IFSG_APPEARANCE::Attach( SGNODE* aNode )
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );

    m_node = NULL;

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
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] invalid SGNODE parent (";
        ostr << aParent->GetNodeTypeName( aParent->GetNodeType() );
        ostr << ") to SGAPPEARANCE";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        delete m_node;
        m_node = NULL;
        return false;
    }

    m_node->AssociateWrapper( &m_node );

    return true;
}


bool IFSG_APPEARANCE::NewNode( IFSG_NODE& aParent )
{
    SGNODE* np = aParent.GetRawPtr();

    if( NULL == np )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadParent;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return NewNode( np );
}


bool IFSG_APPEARANCE::SetEmissive( float aRVal, float aGVal, float aBVal )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject << "\n";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetEmissive( aRVal, aGVal, aBVal );
}


bool IFSG_APPEARANCE::SetEmissive( const SGCOLOR* aRGBColor )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject << "\n";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetEmissive( aRGBColor );
}


bool IFSG_APPEARANCE::SetEmissive( const SGCOLOR& aRGBColor )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetEmissive( aRGBColor );
}


bool IFSG_APPEARANCE::SetDiffuse( float aRVal, float aGVal, float aBVal )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetDiffuse( aRVal, aGVal, aBVal );
}


bool IFSG_APPEARANCE::SetDiffuse( const SGCOLOR* aRGBColor )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetDiffuse( aRGBColor );
}


bool IFSG_APPEARANCE::SetDiffuse( const SGCOLOR& aRGBColor )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetDiffuse( aRGBColor );
}


bool IFSG_APPEARANCE::SetSpecular( float aRVal, float aGVal, float aBVal )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetSpecular( aRVal, aGVal, aBVal );
}


bool IFSG_APPEARANCE::SetSpecular( const SGCOLOR* aRGBColor )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetSpecular( aRGBColor );
}


bool IFSG_APPEARANCE::SetSpecular( const SGCOLOR& aRGBColor )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetSpecular( aRGBColor );
}


bool IFSG_APPEARANCE::SetAmbient( float aRVal, float aGVal, float aBVal )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetAmbient( aRVal, aGVal, aBVal );
}


bool IFSG_APPEARANCE::SetAmbient( const SGCOLOR* aRGBColor )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetAmbient( aRGBColor );
}


bool IFSG_APPEARANCE::SetAmbient( const SGCOLOR& aRGBColor )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    return ((SGAPPEARANCE*)m_node)->SetAmbient( aRGBColor );
}


bool IFSG_APPEARANCE::SetShininess( float aShininess )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    if( aShininess < 0 || aShininess > 1.0 )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] shininess out of range [0..1]";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    ((SGAPPEARANCE*)m_node)->shininess = aShininess;

    return true;
}


bool IFSG_APPEARANCE::SetTransparency( float aTransparency )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << BadObject;
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    if( aTransparency < 0 || aTransparency > 1.0 )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] transparency out of range [0..1]";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    ((SGAPPEARANCE*)m_node)->transparency = aTransparency;

    return true;
}
