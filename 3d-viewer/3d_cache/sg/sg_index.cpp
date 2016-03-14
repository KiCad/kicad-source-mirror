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

#include <fstream>
#include <iostream>
#include <sstream>
#include <wx/log.h>

#include "3d_cache/sg/sg_index.h"


SGINDEX::SGINDEX( SGNODE* aParent ) : SGNODE( aParent )
{
    if( NULL != aParent && S3D::SGTYPE_FACESET != aParent->GetNodeType() )
    {
        m_Parent = NULL;

#ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] inappropriate parent to SGINDEX (type ";
        ostr << aParent->GetNodeType() << ")";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
#endif
    }

    return;
}


SGINDEX::~SGINDEX()
{
    index.clear();
    return;
}


bool SGINDEX::SetParent( SGNODE* aParent, bool notify )
{
    if( NULL != m_Parent )
    {
        if( aParent == m_Parent )
            return true;

        // handle the change in parents
        if( notify )
            m_Parent->unlinkChildNode( this );

        m_Parent = NULL;

        if( NULL == aParent )
            return true;
    }

    // only a SGFACESET may be parent to a SGINDEX and derived types
    if( NULL != aParent && S3D::SGTYPE_FACESET != aParent->GetNodeType() )
        return false;

    m_Parent = aParent;

    if( m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


SGNODE* SGINDEX::FindNode(const char *aNodeName, const SGNODE *aCaller)
{
    if( NULL == aNodeName || 0 == aNodeName[0] )
        return NULL;

    if( !m_Name.compare( aNodeName ) )
        return this;

    return NULL;
}


void SGINDEX::unlinkChildNode( const SGNODE* aCaller )
{
    #ifdef DEBUG
    std::ostringstream ostr;
    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    ostr << " * [BUG] unexpected code branch; node should have no children or refs";
    wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    #endif

    return;
}


void SGINDEX::unlinkRefNode( const SGNODE* aCaller )
{
    #ifdef DEBUG
    std::ostringstream ostr;
    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    ostr << " * [BUG] unexpected code branch; node should have no children or refs";
    wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    #endif

    return;
}


bool SGINDEX::AddRefNode( SGNODE* aNode )
{
    #ifdef DEBUG
    std::ostringstream ostr;
    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    ostr << " * [BUG] this node does not accept children or refs";
    wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    #endif

    return false;
}


bool SGINDEX::AddChildNode( SGNODE* aNode )
{
    #ifdef DEBUG
    std::ostringstream ostr;
    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    ostr << " * [BUG] this node does not accept children or refs";
    wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    #endif

    return false;
}


bool SGINDEX::GetIndices( size_t& nIndices, int*& aIndexList )
{
    if( index.empty() )
    {
        nIndices = 0;
        aIndexList = NULL;
        return false;
    }

    nIndices = index.size();
    aIndexList = & index[0];
    return true;
}


void SGINDEX::SetIndices( size_t nIndices, int* aIndexList )
{
    index.clear();

    if( 0 == nIndices || NULL == aIndexList )
        return;

    for( size_t i = 0; i < nIndices; ++i )
        index.push_back( aIndexList[i] );

    return;
}


void SGINDEX::AddIndex( int aIndex )
{
    index.push_back( aIndex );
    return;
}


void SGINDEX::ReNameNodes( void )
{
    m_written = false;

    // rename this node
    m_Name.clear();
    GetName();
}


bool SGINDEX::WriteVRML( std::ofstream& aFile, bool aReuseFlag )
{
    if( index.empty() )
        return false;

    if( S3D::SGTYPE_COORDINDEX == m_SGtype )
        return writeCoordIndex( aFile );

    return writeColorIndex( aFile );
}


bool SGINDEX::writeCoordIndex( std::ofstream& aFile )
{
    size_t n = index.size();

    if( n % 3 )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] coord index is not divisible by three (violates triangle constraint)";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

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


bool SGINDEX::writeColorIndex( std::ofstream& aFile )
{
    aFile << " colorIndex [\n  ";
    return writeIndexList( aFile );
}


bool SGINDEX::writeIndexList( std::ofstream& aFile )
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


bool SGINDEX::WriteCache( std::ofstream& aFile, SGNODE* parentNode )
{
    if( NULL == parentNode )
    {
        if( NULL == m_Parent )
        {
            #ifdef DEBUG
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] corrupt data; m_aParent is NULL";
            wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
            #endif

            return false;
        }

        SGNODE* np = m_Parent;

        while( NULL != np->GetParent() )
            np = np->GetParent();

        if( np->WriteCache( aFile, NULL ) )
        {
            m_written = true;
            return true;
        }

        return false;
    }

    if( parentNode != m_Parent )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] corrupt data; parentNode != m_aParent";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    if( !aFile.good() )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [INFO] bad stream";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

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


bool SGINDEX::ReadCache( std::ifstream& aFile, SGNODE* parentNode )
{
    if( !index.empty() )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] non-empty node";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    size_t npts;
    aFile.read( (char*)&npts, sizeof(size_t) );
    int tmp;

    if( aFile.fail() )
        return false;

    for( size_t i = 0; i < npts; ++i )
    {
        aFile.read( (char*)&tmp, sizeof(int) );

        if( aFile.fail() )
            return false;

        index.push_back( tmp );
    }

    return true;
}
