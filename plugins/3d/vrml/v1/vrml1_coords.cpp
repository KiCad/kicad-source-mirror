/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include "vrml1_base.h"
#include "vrml1_coords.h"


WRL1COORDS::WRL1COORDS( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_Type = WRL1_COORDINATE3;
    return;
}


WRL1COORDS::WRL1COORDS( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_Type = WRL1_COORDINATE3;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL1COORDS::~WRL1COORDS()
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    wxLogTrace( MASK_VRML, " * [INFO] Destroying Coordinate3 node\n" );
    #endif

    return;
}


bool WRL1COORDS::AddRefNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML1
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] AddRefNode is not applicable";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return false;
}


bool WRL1COORDS::AddChildNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML1
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] AddChildNode is not applicable";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return false;
}


bool WRL1COORDS::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    size_t line, column;
    proc.GetFilePosData( line, column );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; unexpected eof at line ";
            ostr << line << ", column " << column;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( '{' != tok )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        do {
            std::ostringstream ostr;
            ostr << proc.GetError() << "\n";
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; expecting '{' but got '" << tok;
            ostr << "' at line " << line << ", column " << column << "\n";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    proc.Pop();
    std::string glob;

    if( proc.Peek() == '}' )
    {
        proc.Pop();
        return true;
    }

    if( !proc.ReadName( glob ) )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << proc.GetError();
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    proc.GetFilePosData( line, column );

    // expecting 'point'
    if( !glob.compare( "point" ) )
    {
        if( !proc.ReadMFVec3f( points ) )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] invalid point set at line " << line << ", column ";
                ostr << column << "\n";
                ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                ostr << " * [INFO] message: '" << proc.GetError();
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }
    }
    else
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad Coordinate at line " << line << ", column ";
            ostr << column << "\n";
            ostr << " * [INFO] file: '" << proc.GetFileName();
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    // assuming legacy kicad expectation of 1U = 0.1 inch,
    // convert to mm to meet the expectations of the SG structure
    std::vector< WRLVEC3F >::iterator sP = points.begin();
    std::vector< WRLVEC3F >::iterator eP = points.end();

    while( sP != eP )
    {
        sP->x *= 2.54;
        sP->y *= 2.54;
        sP->z *= 2.54;
        ++sP;
    }

    if( proc.Peek() == '}' )
    {
        proc.Pop();
        return true;
    }

    proc.GetFilePosData( line, column );

    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [INFO] bad Coordinate at line " << line << ", column ";
        ostr << column << " (no closing brace)\n";
        ostr << " * [INFO] file: '" << proc.GetFileName();
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return false;
}


void WRL1COORDS::GetCoords( WRLVEC3F*& aCoordList, size_t& aListSize )
{
    if( points.size() < 3 )
    {
        aCoordList = NULL;
        aListSize = 0;
        return;
    }

    aCoordList = &points[0];
    aListSize = points.size();
    return;
}


SGNODE* WRL1COORDS::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    if( NULL == sp )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        wxLogTrace( MASK_VRML, " * [INFO] bad model: no base data given\n" );
        #endif

        return NULL;
    }

    sp->coord = this;

    return NULL;
}
