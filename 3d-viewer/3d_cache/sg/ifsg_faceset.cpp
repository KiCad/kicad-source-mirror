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

#include "plugins/3dapi/ifsg_faceset.h"
#include "3d_cache/sg/sg_faceset.h"


extern char BadObject[];
extern char BadParent[];
extern char WrongParent[];


IFSG_FACESET::IFSG_FACESET( bool create )
{
    m_node = NULL;

    if( !create )
        return ;

    m_node = new SGFACESET( NULL );

    if( m_node )
        m_node->AssociateWrapper( &m_node );

    return;
}


IFSG_FACESET::IFSG_FACESET( SGNODE* aParent )
{
    m_node = new SGFACESET( NULL );

    if( m_node )
    {
        if( !m_node->SetParent( aParent ) )
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


IFSG_FACESET::IFSG_FACESET( IFSG_NODE& aParent )
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

    m_node = new SGFACESET( NULL );

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


bool IFSG_FACESET::Attach( SGNODE* aNode )
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );

    m_node = NULL;

    if( !aNode )
        return false;

    if( S3D::SGTYPE_FACESET != aNode->GetNodeType() )
    {
        return false;
    }

    m_node = aNode;
    m_node->AssociateWrapper( &m_node );

    return true;
}


bool IFSG_FACESET::NewNode( SGNODE* aParent )
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );

    m_node = new SGFACESET( aParent );

    if( aParent != m_node->GetParent() )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] invalid SGNODE parent (";
        ostr << aParent->GetNodeTypeName( aParent->GetNodeType() );
        ostr << ") to SGFACESET";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        delete m_node;
        m_node = NULL;
        return false;
    }

    m_node->AssociateWrapper( &m_node );

    return true;
}


bool IFSG_FACESET::NewNode( IFSG_NODE& aParent )
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


bool IFSG_FACESET::CalcNormals( SGNODE** aPtr )
{
    if( m_node )
        return ((SGFACESET*)m_node)->CalcNormals( aPtr );

    return false;
}
