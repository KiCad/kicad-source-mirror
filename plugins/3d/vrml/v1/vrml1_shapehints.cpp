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
#include <cmath>
#include <wx/log.h>

#include "vrml1_base.h"
#include "vrml1_shapehints.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1SHAPEHINTS::WRL1SHAPEHINTS( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_order = WRL1_ORDER::ORD_UNKNOWN;
    m_Type = WRL1NODES::WRL1_SHAPEHINTS;
    m_crease = 0.733f; // approx 42 degrees; this is larger than VRML spec.
}


WRL1SHAPEHINTS::WRL1SHAPEHINTS( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_order = WRL1_ORDER::ORD_UNKNOWN;
    m_Type = WRL1NODES::WRL1_SHAPEHINTS;
    m_crease = 0.733f; // approx 42 degrees; this is larger than VRML spec.
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL1SHAPEHINTS::~WRL1SHAPEHINTS()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying ShapeHints node." ) );
}


bool WRL1SHAPEHINTS::AddRefNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddRefNode is not applicable." ) );
}


bool WRL1SHAPEHINTS::AddChildNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddChildNode is not applicable." ) );
}


bool WRL1SHAPEHINTS::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    wxCHECK_MSG( aTopNode, false, wxT( "Invalid top node." ) );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          " * [INFO] bad file format; unexpected eof %s." ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

        return false;
    }

    if( '{' != tok )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; expecting '{' but got '%s' %s.\n"
                         "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, tok, proc.GetFilePosition(),
                    proc.GetError() );

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
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

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
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

                return false;
            }

            if( !glob.compare( "UNKNOWN_ORDERING" ) )
            {
                m_order = WRL1_ORDER::ORD_UNKNOWN;
            }
            else if( !glob.compare( "CLOCKWISE" ) )
            {
                m_order = WRL1_ORDER::ORD_CLOCKWISE;
            }
            else if( !glob.compare( "COUNTERCLOCKWISE" ) )
            {
                m_order = WRL1_ORDER::ORD_CCW;
            }
            else
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] bad ShapeHints %s (invalid value '%s')\n"
                                 " * [INFO] file: '%s'" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(), glob,
                            proc.GetFileName() );

                return false;
            }
        }
        else if( !glob.compare( "shapeType" ) )
        {
            if( !proc.ReadName( glob ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

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
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

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
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

                return false;
            }

            if( tmp < 0.0 )
                tmp = 0.0f;
            else if( tmp > M_PI )
                tmp = static_cast<float>( M_PI );

            m_crease = tmp;
        }
        else
        {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] bad ShapeHints %s (unexpected keyword '%s')\n"
                                 " * [INFO] file: '%s'" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(), glob,
                            proc.GetFileName() );

            return false;
        }
    }   // while( true ) -- reading contents of ShapeHints{}

    return true;
}


SGNODE* WRL1SHAPEHINTS::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    // note: this is not fully implemented since it is unlikely we shall
    // ever make use of the fields shapeType, faceType, and creaseAngle
    wxCHECK_MSG( sp, nullptr, wxT( "Invalid base data." ) );

    sp->order = m_order;
    sp->creaseLimit = cosf(m_crease);

    if( sp->creaseLimit < 0.0 )
        sp->creaseLimit = 0.0;

    return nullptr;
}
