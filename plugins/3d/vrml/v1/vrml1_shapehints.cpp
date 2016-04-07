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
#include <cmath>
#include <wx/log.h>

#include "vrml1_base.h"
#include "vrml1_shapehints.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1SHAPEHINTS::WRL1SHAPEHINTS( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_order = ORD_UNKNOWN;
    m_Type = WRL1_SHAPEHINTS;
    m_crease = 0.733;   // approx 42 degrees; this is larger than VRML spec.
    return;
}


WRL1SHAPEHINTS::WRL1SHAPEHINTS( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_order = ORD_UNKNOWN;
    m_Type = WRL1_SHAPEHINTS;
    m_crease = 0.733;   // approx 42 degrees; this is larger than VRML spec.
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL1SHAPEHINTS::~WRL1SHAPEHINTS()
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    wxLogTrace( MASK_VRML, " * [INFO] Destroying ShapeHints node\n" );
    #endif

    return;
}


bool WRL1SHAPEHINTS::AddRefNode( WRL1NODE* aNode )
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


bool WRL1SHAPEHINTS::AddChildNode( WRL1NODE* aNode )
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


bool WRL1SHAPEHINTS::Read( WRLPROC& proc, WRL1BASE* aTopNode )
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

        // expecting one of:
        // vertexOrdering
        // shapeType
        // faceType
        // creaseAngle

        if( !glob.compare( "vertexOrdering" ) )
        {
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

            if( !glob.compare( "UNKNOWN_ORDERING" ) )
                m_order = ORD_UNKNOWN;
            else if( !glob.compare( "CLOCKWISE" ) )
                m_order = ORD_CLOCKWISE;
            else if( !glob.compare( "COUNTERCLOCKWISE" ) )
                m_order = ORD_CCW;
            else
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] bad ShapeHints at line " << line << ", column ";
                    ostr << column << " (invalid value '" << glob << "')\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "shapeType" ) )
        {
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

            // expected values:
            // UNKNOWN_SHAPE_TYPE
            // SOLID
        }
        else if( !glob.compare( "faceType" ) )
        {
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

            // expected values:
            // UNKNOWN_FACE_TYPE
            // CONVEX
        }
        else if( !glob.compare( "creaseAngle" ) )
        {
            float tmp;

            if( !proc.ReadSFFloat( tmp ) )
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

            if( tmp < 0.0 )
                tmp = 0.0;
            else if( tmp > M_PI )
                tmp = M_PI;

            m_crease = tmp;
        }
        else
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad ShapeHints at line " << line << ", column ";
                ostr << column << " (unexpected keyword '" << glob << "')\n";
                ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }
    }   // while( true ) -- reading contents of ShapeHints{}

    return true;
}


SGNODE* WRL1SHAPEHINTS::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    // note: this is not fully implemented since it is unlikely we shall
    // ever make use of the fields shapeType, faceType, and creaseAngle

    if( NULL == sp )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        wxLogTrace( MASK_VRML, " * [INFO] bad model: no base data given\n" );
        #endif

        return NULL;
    }

    sp->order = m_order;
    sp->creaseLimit = cosf(m_crease);

    if( sp->creaseLimit < 0.0 )
        sp->creaseLimit = 0.0;

    return NULL;
}
