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

#include "3d_cache/sg/sg_coords.h"
#include "3d_cache/sg/sg_helpers.h"
#include "3d_cache/sg/sg_normals.h"
#include "3d_cache/sg/sg_faceset.h"


SGCOORDS::SGCOORDS( SGNODE* aParent ) : SGNODE( aParent )
{
    m_SGtype = S3D::SGTYPE_COORDS;

    if( nullptr != aParent && S3D::SGTYPE_FACESET != aParent->GetNodeType() )
    {
        m_Parent = nullptr;

        wxLogTrace( MASK_3D_SG,
                    wxT( "%s:%s:%d * [BUG] inappropriate parent to SGCOORDS (type %s)" ),
                    __FILE__, __FUNCTION__, __LINE__, aParent->GetNodeType() );
    }
    else if( nullptr != aParent && S3D::SGTYPE_FACESET == aParent->GetNodeType() )
    {
        m_Parent->AddChildNode( this );
    }
}


SGCOORDS::~SGCOORDS()
{
    coords.clear();
}


bool SGCOORDS::SetParent( SGNODE* aParent, bool notify )
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

    // only a SGFACESET may be parent to a SGCOORDS
    if( nullptr != aParent && S3D::SGTYPE_FACESET != aParent->GetNodeType() )
        return false;

    m_Parent = aParent;

    if( m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


SGNODE* SGCOORDS::FindNode(const char *aNodeName, const SGNODE *aCaller) noexcept
{
    if( nullptr == aNodeName || 0 == aNodeName[0] )
        return nullptr;

    if( !m_Name.compare( aNodeName ) )
        return this;

    return nullptr;
}


void SGCOORDS::unlinkChildNode( const SGNODE* aCaller ) noexcept
{
    wxCHECK( aCaller, /* void */ );
}


void SGCOORDS::unlinkRefNode( const SGNODE* aCaller ) noexcept
{
    wxCHECK( aCaller, /* void */ );
}


bool SGCOORDS::AddRefNode( SGNODE* aNode ) noexcept
{
    wxCHECK( aNode, false );

    return false;
}


bool SGCOORDS::AddChildNode( SGNODE* aNode ) noexcept
{
    wxCHECK( aNode, false );

    return false;
}


bool SGCOORDS::GetCoordsList( size_t& aListSize, SGPOINT*& aCoordsList )
{
    if( coords.empty() )
    {
        aListSize = 0;
        aCoordsList = nullptr;
        return false;
    }

    aListSize = coords.size();
    aCoordsList = &coords[0];
    return true;
}


void SGCOORDS::SetCoordsList( size_t aListSize, const SGPOINT* aCoordsList )
{
    coords.clear();

    if( 0 == aListSize || nullptr == aCoordsList )
        return;

    for( size_t i = 0; i < aListSize; ++i )
        coords.push_back( aCoordsList[i] );
}


void SGCOORDS::AddCoord( double aXValue, double aYValue, double aZValue )
{
    coords.emplace_back( aXValue, aYValue, aZValue );
}


void SGCOORDS::AddCoord( const SGPOINT& aPoint )
{
    coords.push_back( aPoint );
}


void SGCOORDS::ReNameNodes( void )
{
    m_written = false;

    // rename this node
    m_Name.clear();
    GetName();
}


bool SGCOORDS::WriteVRML( std::ostream& aFile, bool aReuseFlag )
{
    if( coords.empty() )
        return false;

    if( aReuseFlag )
    {
        if( !m_written )
        {
            aFile << "  coord DEF " << GetName() << " Coordinate { point [\n  ";
            m_written = true;
        }
        else
        {
            aFile << "  coord USE " << GetName() << "\n";
            return true;
        }
    }
    else
    {
        aFile << "  coord Coordinate { point [\n  ";
    }

    std::string tmp;
    size_t n = coords.size();
    bool nline = false;
    SGPOINT pt;

    for( size_t i = 0; i < n; )
    {
        // ensure VRML output has 1U = 0.1 inch as per legacy kicad expectations
        pt = coords[i];
        pt.x /= 2.54;
        pt.y /= 2.54;
        pt.z /= 2.54;
        S3D::FormatPoint( tmp, pt );
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


bool SGCOORDS::WriteCache( std::ostream& aFile, SGNODE* parentNode )
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
    size_t npts = coords.size();
    aFile.write( (char*)&npts, sizeof(size_t) );

    for( size_t i = 0; i < npts; ++i )
        S3D::WritePoint( aFile, coords[i] );

    if( aFile.fail() )
        return false;

    m_written = true;
    return true;
}


bool SGCOORDS::ReadCache( std::istream& aFile, SGNODE* parentNode )
{
    wxCHECK( coords.empty(), false );

    size_t npts;
    aFile.read( (char*) &npts, sizeof( size_t ) );
    SGPOINT tmp;

    if( aFile.fail() )
        return false;

    for( size_t i = 0; i < npts; ++i )
    {
        if( !S3D::ReadPoint( aFile, tmp ) || aFile.fail() )
            return false;

        coords.push_back( tmp );
    }

    return true;
}


bool SGCOORDS::CalcNormals( SGFACESET* callingNode, SGNODE** aPtr )
{
    if( aPtr )
        *aPtr = nullptr;

    if( nullptr == m_Parent || nullptr == callingNode )
        return false;

    // the parent and all references must have indices; collect all
    // indices into one std::vector<>
    std::vector< int > ilist;
    SGNORMALS* np = nullptr;

    if( callingNode == m_Parent )
    {
        ((SGFACESET*)m_Parent)->GatherCoordIndices( ilist );

        std::list< SGNODE* >::iterator sB = m_BackPointers.begin();
        std::list< SGNODE* >::iterator eB = m_BackPointers.end();

        while( sB != eB )
        {
            SGFACESET* fp = (SGFACESET*)(*sB);
            fp->GatherCoordIndices( ilist );
            ++sB;
        }

        np = ( (SGFACESET*) m_Parent )->m_Normals;

        if( !np )
            np = new SGNORMALS( m_Parent );

    }
    else
    {
        callingNode->GatherCoordIndices( ilist );
        np = callingNode->m_Normals;

        if( !np )
            np = new SGNORMALS( callingNode );

    }

    if( S3D::CalcTriangleNormals( coords, ilist, np->norms )  )
    {
        if( aPtr )
            *aPtr = np;

        return true;
    }

    delete np;

    return false;
}
