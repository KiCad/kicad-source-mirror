/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include "3d_cache/sg/sg_normals.h"
#include "3d_cache/sg/sg_helpers.h"


SGNORMALS::SGNORMALS( SGNODE* aParent ) : SGNODE( aParent )
{
    m_SGtype = S3D::SGTYPE_NORMALS;

    if( nullptr != aParent && S3D::SGTYPE_FACESET != aParent->GetNodeType() )
    {
        m_Parent = nullptr;

        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] inappropriate parent to SGNORMALS "
                                     "(type %d)" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    aParent->GetNodeType() );
    }
    else if( nullptr != aParent && S3D::SGTYPE_FACESET == aParent->GetNodeType() )
    {
        m_Parent->AddChildNode( this );
    }
}


SGNORMALS::~SGNORMALS()
{
    norms.clear();
}


bool SGNORMALS::SetParent( SGNODE* aParent, bool notify )
{
    if( nullptr != m_Parent )
    {
        if( aParent == m_Parent )
            return true;

        // handle the change in parents
        if( notify )
            m_Parent->unlinkChildNode( this );

        m_Parent = nullptr;

        if( nullptr == aParent )
            return true;
    }

    // only a SGFACESET may be parent to a SGNORMALS
    if( nullptr != aParent && S3D::SGTYPE_FACESET != aParent->GetNodeType() )
        return false;

    m_Parent = aParent;

    if( m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


SGNODE* SGNORMALS::FindNode( const char* aNodeName, const SGNODE* aCaller ) noexcept
{
    if( nullptr == aNodeName || 0 == aNodeName[0] )
        return nullptr;

    if( !m_Name.compare( aNodeName ) )
        return this;

    return nullptr;
}


void SGNORMALS::unlinkChildNode( const SGNODE* aCaller ) noexcept
{
    wxCHECK( false, /* void */ );
}


void SGNORMALS::unlinkRefNode( const SGNODE* aCaller ) noexcept
{
    wxCHECK( false, /* void */ );
}


bool SGNORMALS::AddRefNode( SGNODE* aNode ) noexcept
{
    wxCHECK( false, false );

    return false;
}


bool SGNORMALS::AddChildNode( SGNODE* aNode ) noexcept
{
    wxCHECK( false, false );

    return false;
}


bool SGNORMALS::GetNormalList( size_t& aListSize, SGVECTOR*& aNormalList )
{
    if( norms.empty() )
    {
        aListSize = 0;
        aNormalList = nullptr;
        return false;
    }

    aListSize = norms.size();
    aNormalList = &norms[0];
    return true;
}


void SGNORMALS::SetNormalList( size_t aListSize, const SGVECTOR* aNormalList )
{
    norms.clear();

    if( 0 == aListSize || nullptr == aNormalList )
        return;

    for( int i = 0; i < (int)aListSize; ++i )
        norms.push_back( aNormalList[i] );
}


void SGNORMALS::AddNormal( double aXValue, double aYValue, double aZValue )
{
    norms.emplace_back( aXValue, aYValue, aZValue );
}


void SGNORMALS::AddNormal( const SGVECTOR& aNormal )
{
    norms.push_back( aNormal );
}


void SGNORMALS::ReNameNodes( void )
{
    m_written = false;

    // rename this node
    m_Name.clear();
    GetName();
}


bool SGNORMALS::WriteVRML( std::ostream& aFile, bool aReuseFlag )
{
    if( norms.empty() )
        return false;

    if( aReuseFlag )
    {
        if( !m_written )
        {
            aFile << "  normal DEF " << GetName() << " Normal { vector [\n  ";
            m_written = true;
        }
        else
        {
            aFile << "  normal USE " << GetName() << "\n";
            return true;
        }
    }
    else
    {
        aFile << "  normal Normal { vector [\n  ";
    }

    std::string tmp;
    size_t n = norms.size();
    bool nline = false;

    for( size_t i = 0; i < n; )
    {
        S3D::FormatVector( tmp, norms[i] );
        aFile << tmp ;
        ++i;

        if( i < n )
        {
            aFile << ",";

            if( nline )
            {
                aFile << "\n  ";
                nline = false;
            }
            else
            {
                nline = true;
            }

        }
    }

    aFile << "] }\n";

    return true;
}


bool SGNORMALS::WriteCache( std::ostream& aFile, SGNODE* parentNode )
{
    if( nullptr == parentNode )
    {
        wxCHECK( m_Parent, false );

        SGNODE* np = m_Parent;

        while( nullptr != np->GetParent() )
            np = np->GetParent();

        if( np->WriteCache( aFile, nullptr ) )
        {
            m_written = true;
            return true;
        }

        return false;
    }

    wxCHECK( parentNode == m_Parent, false );

    if( !aFile.good() )
    {
        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [INFO] bad stream" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    aFile << "[" << GetName() << "]";
    size_t npts = norms.size();
    aFile.write( (char*)&npts, sizeof(size_t) );

    for( size_t i = 0; i < npts; ++i )
        S3D::WriteVector( aFile, norms[i] );

    if( aFile.fail() )
        return false;

    m_written = true;
    return true;
}


bool SGNORMALS::ReadCache( std::istream& aFile, SGNODE* parentNode )
{
    wxCHECK( norms.empty(), false );

    size_t npts;
    aFile.read( (char*) &npts, sizeof( size_t ) );
    SGVECTOR tmp;

    if( aFile.fail() )
        return false;

    for( size_t i = 0; i < npts; ++i )
    {
        if( !S3D::ReadVector( aFile, tmp ) || aFile.fail() )
            return false;

        norms.push_back( tmp );
    }

    return true;
}
