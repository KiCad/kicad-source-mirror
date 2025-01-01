/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
#include <iterator>
#include <wx/log.h>

#include "vrml1_base.h"
#include "vrml1_switch.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1SWITCH::WRL1SWITCH( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_SWITCH;
    whichChild = -1;
}


WRL1SWITCH::WRL1SWITCH( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_SWITCH;
    m_Parent = aParent;
    whichChild = -1;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL1SWITCH::~WRL1SWITCH()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying Switch node with %zu children, %zu"
                     "references, and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


bool WRL1SWITCH::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    /*
     * Structure of a Switch node:
     *
     * Switch {
     *      exposedField    SFInt32     whichChild     -1
     *      children
     * }
     */

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
        char pchar = proc.Peek();

        if( pchar == '}' )
        {
            proc.Pop();
            break;
        }
        else if ( pchar == 'w' )
        {
            if( !proc.ReadName( glob ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

                return false;
            }

            if( !glob.compare( "whichChild" ) )
            {
                if( !proc.ReadSFInt( whichChild ) )
                {
                    wxLogTrace( traceVrmlPlugin,
                                wxT( "%s:%s:%d"
                                     " * [INFO] invalid whichChild %s (invalid value '%s')\n"
                                     " * [INFO] file: '%s'\n"
                                     "%s" ),
                                __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(), glob,
                                proc.GetFileName(), proc.GetError() );

                    return false;
                }

                continue;
            }

            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] invalid Switch %s (unexpected 'whichChild')\n"
                             " * [INFO] file: '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }

        if( !aTopNode->ReadNode( proc, this, nullptr ) )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; unexpected eof %s."),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

            return false;
        }

        if( proc.Peek() == ',' )
            proc.Pop();

    }   // while( true ) -- reading contents of Switch{}

    return true;
}


SGNODE* WRL1SWITCH::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating Switch node with %zu children, %zu"
                     "references, and %zu back pointers (%zu total items)." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size(), m_Items.size() );

    if( m_Items.empty() )
        return nullptr;

    if( whichChild < 0 || whichChild >= (int)m_Items.size() )
        return nullptr;

    if( sp == nullptr )
    {
        m_current.Init();
        sp = &m_current;
    }

    std::list< WRL1NODE* >::iterator ip = m_Items.begin();
    std::advance( ip, whichChild );

    IFSG_TRANSFORM txNode( aParent );

    SGNODE* np = (*ip)->TranslateToSG( aParent, sp );

    return np;
}
