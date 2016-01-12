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
#include "plugins/3dapi/ifsg_transform.h"
#include "3d_cache/sg/scenegraph.h"


extern char BadObject[];
extern char BadOperand[];
extern char BadParent[];
extern char WrongParent[];

IFSG_TRANSFORM::IFSG_TRANSFORM( bool create )
{
    m_node = NULL;

    if( !create )
        return;

    m_node = new SCENEGRAPH( NULL );

    if( m_node )
        m_node->AssociateWrapper( &m_node );

    return;
}


IFSG_TRANSFORM::IFSG_TRANSFORM( SGNODE* aParent )
{
    m_node = new SCENEGRAPH( NULL );

    if( m_node )
    {
        if( !m_node->SetParent( aParent ) )
        {
            delete m_node;
            m_node = NULL;

            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << WrongParent << "\n";
            #endif

            return;
        }

        m_node->AssociateWrapper( &m_node );
    }

    return;
}


bool IFSG_TRANSFORM::Attach( SGNODE* aNode )
{
    if( m_node )
        m_node->DisassociateWrapper( &m_node );

    m_node = NULL;

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
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invalid SGNODE parent (";
        std::cerr << aParent->GetNodeTypeName( aParent->GetNodeType() );
        std::cerr << ") to SCENEGRAPH\n";
        #endif

        delete m_node;
        m_node = NULL;
        return false;
    }

    m_node->AssociateWrapper( &m_node );

    return true;
}


bool IFSG_TRANSFORM::NewNode( IFSG_NODE& aParent )
{
    SGNODE* np = aParent.GetRawPtr();

    if( NULL == np )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << BadParent << "\n";
        #endif

        return false;
    }

    return NewNode( np );
}


bool IFSG_TRANSFORM::SetRotation( const SGVECTOR& aRotationAxis, double aAngle )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << BadObject << "\n";
        #endif

        return false;
    }

    ((SCENEGRAPH*)m_node)->rotation_axis = aRotationAxis;
    ((SCENEGRAPH*)m_node)->rotation_angle = aAngle;

    return true;
}


bool IFSG_TRANSFORM::SetScale( const SGPOINT& aScale )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << BadObject << "\n";
        #endif

        return false;
    }

    ((SCENEGRAPH*)m_node)->scale = aScale;

    return true;
}


bool IFSG_TRANSFORM::SetScale( double aScale )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << BadObject << "\n";
        #endif

        return false;
    }

    if( aScale < 1e-8 && aScale > -1e-8 )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] |scale| is < 1e-8 - this seems strange\n";
        #endif

        return false;
    }

    ((SCENEGRAPH*)m_node)->scale = SGPOINT( aScale, aScale, aScale );

    return true;
}


bool IFSG_TRANSFORM::SetTranslation( const SGPOINT& aTranslation )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << BadObject << "\n";
        #endif

        return false;
    }

    ((SCENEGRAPH*)m_node)->translation = aTranslation;

    return true;
}


bool IFSG_TRANSFORM::SetScaleOrientation( const SGVECTOR& aScaleAxis, double aAngle )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << BadObject << "\n";
        #endif

        return false;
    }

    ((SCENEGRAPH*)m_node)->scale_axis = aScaleAxis;
    ((SCENEGRAPH*)m_node)->scale_angle = aAngle;

    return true;
}


bool IFSG_TRANSFORM::SetCenter( const SGPOINT& aCenter )
{
    if( NULL == m_node )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << BadObject << "\n";
        #endif

        return false;
    }

    ((SCENEGRAPH*)m_node)->center = aCenter;

    return true;
}
