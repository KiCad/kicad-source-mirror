/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include "vrml1_base.h"
#include "vrml1_coords.h"


WRL1COORDS::WRL1COORDS( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_COORDINATE3;
}


WRL1COORDS::WRL1COORDS( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_COORDINATE3;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL1COORDS::~WRL1COORDS()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying Coordinate3 node." ) );
}


bool WRL1COORDS::AddRefNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddRefNode is not applicable." ) );
}


bool WRL1COORDS::AddChildNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddChildNode is not applicable." ) );
}


bool WRL1COORDS::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    char tok = proc.Peek();

    if( proc.eof() )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; unexpected eof %s." ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

        return false;
    }

    if( '{' != tok )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; expecting '{' but got '%s' %s" ),
                    __FILE__, __FUNCTION__, __LINE__, tok, proc.GetFilePosition() );

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
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n%s" ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetError() );

        return false;
    }

    // expecting 'point'
    if( !glob.compare( "point" ) )
    {
        if( !proc.ReadMFVec3f( points ) )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] invalid point set %s\n"
                             " * [INFO] file: '%s'\n"
                             "%s" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName(), proc.GetError() );

            return false;
        }
    }
    else
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          "* [INFO] bad Coordinate %s.\n"
                                          "* [INFO] file: '%s'." ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(), proc.GetFileName() );

        return false;
    }

    // assuming legacy KiCad expectation of 1U = 0.1 inch,
    // convert to mm to meet the expectations of the SG structure
    std::vector< WRLVEC3F >::iterator sP = points.begin();
    std::vector< WRLVEC3F >::iterator eP = points.end();

    while( sP != eP )
    {
        sP->x *= 2.54f;
        sP->y *= 2.54f;
        sP->z *= 2.54f;
        ++sP;
    }

    if( proc.Peek() == '}' )
    {
        proc.Pop();
        return true;
    }

    wxLogTrace( traceVrmlPlugin,
                wxT( "%s:%s:%d\n"
                     " * [INFO] bad Coordinate %s (no closing brace)." ),
                __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

    return false;
}


void WRL1COORDS::GetCoords( WRLVEC3F*& aCoordList, size_t& aListSize )
{
    if( points.size() < 3 )
    {
        aCoordList = nullptr;
        aListSize = 0;
        return;
    }

    aCoordList = &points[0];
    aListSize = points.size();
}


SGNODE* WRL1COORDS::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    wxCHECK_MSG( sp, nullptr, wxT( "Invalid base data." ) );

    sp->coord = this;

    return nullptr;
}
