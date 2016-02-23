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
#include "vrml1_matbinding.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1MATBINDING::WRL1MATBINDING( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_binding = BIND_OVERALL;
    m_Type = WRL1_MATERIALBINDING;
    return;
}


WRL1MATBINDING::WRL1MATBINDING( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_binding = BIND_OVERALL;
    m_Type = WRL1_MATERIALBINDING;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL1MATBINDING::~WRL1MATBINDING()
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    wxLogTrace( MASK_VRML, " * [INFO] Destroying MaterialBinding node\n" );
    #endif

    return;
}


bool WRL1MATBINDING::AddRefNode( WRL1NODE* aNode )
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


bool WRL1MATBINDING::AddChildNode( WRL1NODE* aNode )
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


bool WRL1MATBINDING::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    if( NULL == aTopNode )
    {
        #ifdef DEBUG_VRML1
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] aTopNode is NULL";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

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
            ostr << "' at line " << line << ", column " << column;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    proc.Pop();
    std::string glob;

    while( true )
    {
        if( proc.Peek() == '}' )
        {
            proc.Pop();
            break;
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

        if( glob.compare( "value" ) )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad MaterialBinding at line " << line << ", column ";
                ostr << column << " (did not find keyword 'value')\n";
                ostr << " * [INFO] file: '" << proc.GetFileName();
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
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

        // expecting one of:
        // DEFAULT
        // OVERALL
        // PER_PART
        // PER_PART_INDEXED
        // PER_FACE
        // PER_FACE_INDEXED
        // PER_VERTEX
        // PER_VERTEX_INDEXED

        proc.GetFilePosData( line, column );

        if( !glob.compare( "DEFAULT" ) )
        {
            m_binding = BIND_DEFAULT;
        }
        else if( !glob.compare( "OVERALL" ) )
        {
            m_binding = BIND_OVERALL;
        }
        else if( !glob.compare( "PER_PART" ) )
        {
            m_binding = BIND_PER_PART;
        }
        else if( !glob.compare( "PER_PART_INDEXED" ) )
        {
            m_binding = BIND_PER_PART_INDEXED;
        }
        else if( !glob.compare( "PER_FACE" ) )
        {
            m_binding = BIND_PER_FACE;
        }
        else if( !glob.compare( "PER_FACE_INDEXED" ) )
        {
            m_binding = BIND_PER_FACE_INDEXED;
        }
        else if( !glob.compare( "PER_VERTEX" ) )
        {
            m_binding = BIND_PER_VERTEX;
        }
        else if( !glob.compare( "PER_VERTEX_INDEXED" ) )
        {
            m_binding = BIND_PER_VERTEX_INDEXED;
        }
        else
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad MaterialBinding at line " << line << ", column ";
                ostr << column << "\n";
                ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            m_binding = BIND_OVERALL;
        }
    }   // while( true ) -- reading contents of MaterialBinding{}

    return true;
}


SGNODE* WRL1MATBINDING::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    if( NULL == sp )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        wxLogTrace( MASK_VRML, " * [INFO] bad model: no base data given\n" );
        #endif

        return NULL;
    }

    sp->matbind = m_binding;

    return NULL;
}
