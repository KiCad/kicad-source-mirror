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
#include <utility>
#include <algorithm>
#include <wx/log.h>
#include "x3d_base.h"
#include "wrltypes.h"


bool X3D_DICT::AddName( const wxString& aName, X3DNODE* aNode )
{
    if( aName.empty() )
        return false;

    std::map< wxString, X3DNODE* >::iterator ir = reg.find( aName );

    if( ir != reg.end() )
        reg.erase( ir );

    reg.emplace( aName, aNode );

    return true;
}


bool X3D_DICT::DelName( const wxString& aName, X3DNODE* aNode )
{
    if( aName.empty() )
        return false;

    std::map< wxString, X3DNODE* >::iterator ir = reg.find( aName );

    if( ir != reg.end() && ir->second == aNode )
    {
        reg.erase( ir );
        return true;
    }

    return false;
}


X3DNODE* X3D_DICT::FindName( const wxString& aName )
{
    if( aName.empty() )
        return nullptr;

    std::map< wxString, X3DNODE* >::iterator ir = reg.find( aName );

    if( ir != reg.end() )
        return ir->second;

    return nullptr;
}


X3DNODE::X3DNODE()
{
    m_Type = X3D_INVALID;
    m_Parent = nullptr;
    m_sgNode = nullptr;
    m_Dict = nullptr;

    return;
}


X3DNODE::~X3DNODE()
{
    if( !m_Name.empty() && nullptr != m_Dict )
        m_Dict->DelName( m_Name, this );

    return;
}


void X3DNODE::unlinkChildNode( const X3DNODE* aNode )
{
    std::list< X3DNODE* >::iterator sL = m_Children.begin();
    std::list< X3DNODE* >::iterator eL = m_Children.end();

    while( sL != eL )
    {
        if( *sL == aNode )
        {
            m_Children.erase( sL );
            return;
        }

        ++sL;
    }

    return;
}


void X3DNODE::unlinkRefNode( const X3DNODE* aNode )
{
    std::list< X3DNODE* >::iterator sL = m_Refs.begin();
    std::list< X3DNODE* >::iterator eL = m_Refs.end();

    while( sL != eL )
    {
        if( *sL == aNode )
        {
            m_Refs.erase( sL );
            return;
        }

        ++sL;
    }

    return;
}


void X3DNODE::addNodeRef( X3DNODE* aNode )
{
    // the parent node must never be added as a backpointer
    if( aNode == m_Parent )
        return;

    std::list< X3DNODE* >::iterator sR = m_BackPointers.begin();
    std::list< X3DNODE* >::iterator eR = m_BackPointers.end();

    while( sR != eR )
    {
        if( *sR == aNode )
            return;

        ++sR;
    }

    m_BackPointers.push_back( aNode );

    return;
}


void X3DNODE::delNodeRef( X3DNODE* aNode )
{
    std::list< X3DNODE* >::iterator np =
        std::find( m_BackPointers.begin(), m_BackPointers.end(), aNode );

    if( np != m_BackPointers.end() )
    {
        m_BackPointers.erase( np );
        return;
    }

    wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                      " * [BUG] delNodeRef() did not find its target." ),
                __FILE__, __FUNCTION__, __LINE__ );

    return;
}


X3DNODES X3DNODE::GetNodeType( void ) const
{
    return m_Type;
}


X3DNODE* X3DNODE::GetParent( void ) const
{
    return m_Parent;
}


wxString X3DNODE::GetName( void ) const
{
    return m_Name;
}


std::string X3DNODE::GetError( void )
{
    return m_error;
}
