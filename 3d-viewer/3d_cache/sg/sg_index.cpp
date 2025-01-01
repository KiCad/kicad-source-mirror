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

#include "3d_cache/sg/sg_index.h"


SGINDEX::SGINDEX( SGNODE* aParent ) : SGNODE( aParent )
{
    if( nullptr != aParent && S3D::SGTYPE_FACESET != aParent->GetNodeType() )
    {
        m_Parent = nullptr;

        wxLogTrace( MASK_3D_SG, wxT( "%s:%s:%d * [BUG] inappropriate parent to SGINDEX (type "
                                     "'%d')" ),
                    __FILE__, __FUNCTION__, __LINE__,
                    aParent->GetNodeType() );
    }
}


SGINDEX::~SGINDEX()
{
    index.clear();
}


bool SGINDEX::SetParent( SGNODE* aParent, bool notify )
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

    // only a SGFACESET may be parent to a SGINDEX and derived types
    if( nullptr != aParent && S3D::SGTYPE_FACESET != aParent->GetNodeType() )
        return false;

    m_Parent = aParent;

    if( m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


SGNODE* SGINDEX::FindNode(const char *aNodeName, const SGNODE *aCaller) noexcept
{
    if( nullptr == aNodeName || 0 == aNodeName[0] )
        return nullptr;

    if( !m_Name.compare( aNodeName ) )
        return this;

    return nullptr;
}


void SGINDEX::unlinkChildNode( const SGNODE* aCaller ) noexcept
{
    // Node should have no children or refs.
    wxCHECK( false, /* void */ );
}


void SGINDEX::unlinkRefNode( const SGNODE* aCaller ) noexcept
{
    // Node should have no children or refs.
    wxCHECK( false, /* void */ );
}


bool SGINDEX::AddRefNode( SGNODE* aNode ) noexcept
{
    // Node should have no children or refs.
    wxCHECK( false, false );

    return false;
}


bool SGINDEX::AddChildNode( SGNODE* aNode ) noexcept
{
    // Node should have no children or refs.
    wxCHECK( false, false );

    return false;
}


bool SGINDEX::GetIndices( size_t& nIndices, int*& aIndexList )
{
    if( index.empty() )
    {
        nIndices = 0;
        aIndexList = nullptr;
        return false;
    }

    nIndices = index.size();
    aIndexList = & index[0];
    return true;
}


void SGINDEX::SetIndices( size_t nIndices, int* aIndexList )
{
    index.clear();

    if( 0 == nIndices || nullptr == aIndexList )
        return;

    for( size_t i = 0; i < nIndices; ++i )
        index.push_back( aIndexList[i] );

    return;
}


void SGINDEX::AddIndex( int aIndex )
{
    index.push_back( aIndex );
}


void SGINDEX::ReNameNodes( void )
{
    m_written = false;

    // rename this node
    m_Name.clear();
    GetName();
}


bool SGINDEX::WriteVRML( std::ostream& aFile, bool aReuseFlag )
{
    if( index.empty() )
        return false;

    if( S3D::SGTYPE_COORDINDEX == m_SGtype )
        return writeCoordIndex( aFile );

    return writeColorIndex( aFile );
}


bool SGINDEX::writeCoordIndex( std::ostream& aFile )
{
    size_t n = index.size();

    wxCHECK_MSG( n % 3 == 0, false, wxT( "Coordinate index is not divisible by three (violates "
                                         "triangle constraint)" ) );

    aFile << " coordIndex [\n  ";

    // indices to control formatting
    int nv0 = 0;
    int nv1 = 0;

    for( size_t i = 0; i < n; )
    {
        aFile << index[i];
        ++i;

        if( ++nv0 == 3 )
        {
            aFile << ",-1";
            ++nv1;
            nv0 = 0;
        }

        if( i < n )
        {
            aFile << ",";

            if( nv1 == 8 )
            {
                nv1 = 0;
                aFile << "\n  ";
            }
        }
    }

    aFile << "]\n";

    return true;
}


bool SGINDEX::writeColorIndex( std::ostream& aFile )
{
    aFile << " colorIndex [\n  ";
    return writeIndexList( aFile );
}


bool SGINDEX::writeIndexList( std::ostream& aFile )
{
    // index to control formatting
    int nv = 0;
    size_t n = index.size();

    for( size_t i = 0; i < n; )
    {
        aFile << index[i];
        ++i;

        if( i < n )
        {
            aFile << ",";

            if( ++nv == 20 )
            {
                aFile << "\n  ";
                nv = 0;
            }
        }
    }

    aFile << "]\n";

    return true;
}


bool SGINDEX::WriteCache( std::ostream& aFile, SGNODE* parentNode )
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
    size_t npts = index.size();
    aFile.write( (char*)&npts, sizeof(size_t) );

    for( size_t i = 0; i < npts; ++i )
        aFile.write( (char*)&index[i], sizeof(int) );

    if( aFile.fail() )
        return false;

    m_written = true;
    return true;
}


bool SGINDEX::ReadCache( std::istream& aFile, SGNODE* parentNode )
{
    wxCHECK( index.empty(), false );

    size_t npts;
    aFile.read( (char*)&npts, sizeof(size_t) );
    int tmp;

    if( aFile.fail() )
        return false;

    for( size_t i = 0; i < npts; ++i )
    {
        aFile.read( (char*) &tmp, sizeof( int ) );

        if( aFile.fail() )
            return false;

        index.push_back( tmp );
    }

    return true;
}
