/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include "3d_cache/sg/sg_colors.h"
#include "3d_cache/sg/sg_helpers.h"

SGCOLORS::SGCOLORS( SGNODE* aParent ) : SGNODE( aParent )
{
    m_SGtype = S3D::SGTYPE_COLORS;

    if( NULL != aParent && S3D::SGTYPE_FACESET != aParent->GetNodeType() )
    {
        m_Parent = NULL;

#ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] inappropriate parent to SGCOLORS (type ";
        ostr << aParent->GetNodeType() << ")";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
#endif
    }
    else if( NULL != aParent && S3D::SGTYPE_FACESET == aParent->GetNodeType() )
    {
        m_Parent->AddChildNode( this );
    }

    return;
}


SGCOLORS::~SGCOLORS()
{
    colors.clear();
    return;
}


bool SGCOLORS::SetParent( SGNODE* aParent, bool notify )
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

    // only a SGFACESET may be parent to a SGCOLORS
    if( NULL != aParent && S3D::SGTYPE_FACESET != aParent->GetNodeType() )
        return false;

    m_Parent = aParent;

    if( m_Parent )
        m_Parent->AddChildNode( this );

    return true;
}


SGNODE* SGCOLORS::FindNode(const char *aNodeName, const SGNODE *aCaller)
{
    if( NULL == aNodeName || 0 == aNodeName[0] )
        return NULL;

    if( !m_Name.compare( aNodeName ) )
        return this;

    return NULL;
}


void SGCOLORS::unlinkChildNode( const SGNODE* aCaller )
{
    #ifdef DEBUG
    std::ostringstream ostr;
    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    ostr << " * [BUG] unexpected code branch; node should have no children or refs";
    wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    #endif

    return;
}


void SGCOLORS::unlinkRefNode( const SGNODE* aCaller )
{
    #ifdef DEBUG
    std::ostringstream ostr;
    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    ostr << " * [BUG] unexpected code branch; node should have no children or refs";
    wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    #endif

    return;
}


bool SGCOLORS::AddRefNode( SGNODE* aNode )
{
    #ifdef DEBUG
    std::ostringstream ostr;
    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    ostr << " * [BUG] this node does not accept children or refs";
    wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    #endif

    return false;
}


bool SGCOLORS::AddChildNode( SGNODE* aNode )
{
    #ifdef DEBUG
    std::ostringstream ostr;
    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    ostr << " * [BUG] this node does not accept children or refs";
    wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
    #endif

    return false;
}


bool SGCOLORS::GetColorList( size_t& aListSize, SGCOLOR*& aColorList )
{
    if( colors.empty() )
    {
        aListSize = 0;
        aColorList = NULL;
        return false;
    }

    aListSize = colors.size();
    aColorList = &colors[0];
    return true;
}


void SGCOLORS::SetColorList( size_t aListSize, const SGCOLOR* aColorList )
{
    colors.clear();

    if( 0 == aListSize || NULL == aColorList )
        return;

    for( size_t i = 0; i < aListSize; ++i )
        colors.push_back( aColorList[i] );

    return;
}


void SGCOLORS::AddColor( double aRedValue, double aGreenValue, double aBlueValue )
{
    colors.push_back( SGCOLOR( aRedValue, aGreenValue, aBlueValue ) );
    return;
}


void SGCOLORS::AddColor( const SGCOLOR& aColor )
{
    colors.push_back( aColor );
    return;
}


void SGCOLORS::ReNameNodes( void )
{
    m_written = false;

    // rename this node
    m_Name.clear();
    GetName();
}


bool SGCOLORS::WriteVRML( std::ostream& aFile, bool aReuseFlag )
{
    if( colors.empty() )
        return false;

    if( aReuseFlag )
    {
        if( !m_written )
        {
            aFile << "color DEF " << GetName() << " Color { color [\n  ";
            m_written = true;
        }
        else
        {
            aFile << "color USE " << GetName() << "\n";
            return true;
        }
    }
    else
    {
        aFile << "color Color { color [\n  ";
    }

    std::string tmp;
    size_t n = colors.size();
    bool nline = false;

    for( size_t i = 0; i < n; )
    {
        S3D::FormatColor( tmp, colors[i] );
        float r,g,b;
        colors[i].GetColor(r, g, b);
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


bool SGCOLORS::WriteCache( std::ostream& aFile, SGNODE* parentNode )
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
    size_t ncolors = colors.size();
    aFile.write( (char*)&ncolors, sizeof(size_t) );

    for( size_t i = 0; i < ncolors; ++i )
        S3D::WriteColor( aFile, colors[i] );

    if( aFile.fail() )
        return false;

    m_written = true;
    return true;
}


bool SGCOLORS::ReadCache( std::istream& aFile, SGNODE* parentNode )
{
    if( !colors.empty() )
    {
        #ifdef DEBUG
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] non-empty node";
        wxLogTrace( MASK_3D_SG, "%s\n", ostr.str().c_str() );
        #endif

        return false;
    }

    size_t ncolors;
    aFile.read( (char*)&ncolors, sizeof(size_t) );
    SGCOLOR tmp;

    if( aFile.fail() )
        return false;

    for( size_t i = 0; i < ncolors; ++i )
    {
        if( !S3D::ReadColor( aFile, tmp ) || aFile.fail() )
            return false;

        colors.push_back( tmp );
    }

    return true;
}
