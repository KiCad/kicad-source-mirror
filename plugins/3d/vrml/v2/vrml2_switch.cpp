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
#include <wx/log.h>

#include "vrml2_base.h"
#include "vrml2_switch.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2SWITCH::WRL2SWITCH() : WRL2NODE()
{
    m_Type = WRL2NODES::WRL2_SWITCH;
    whichChoice = -1;
}


WRL2SWITCH::WRL2SWITCH( WRL2NODE* aParent ) : WRL2NODE()
{
    m_Type = WRL2NODES::WRL2_SWITCH;
    m_Parent = aParent;
    whichChoice = -1;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL2SWITCH::~WRL2SWITCH()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying Switch node with %zu children, %zu"
                     "references, and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


bool WRL2SWITCH::isDangling( void )
{
    // a Switch node is never dangling
    return false;
}


bool WRL2SWITCH::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
    /*
     * Structure of a Switch node (p.113):
     *
     * Switch {
     *      exposedField    MFNode      choice          []
     *      exposedField    SFInt32     whichChoice     -1
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
                         " * [INFO] bad file format; expecting '{' but got '%s' %s." ),
                    __FILE__, __FUNCTION__, __LINE__, tok, proc.GetFilePosition() );

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
        // choice
        // whichChoice
        if( !glob.compare( "whichChoice" ) )
        {
            if( !proc.ReadSFInt( whichChoice ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid whichChoice %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "choice" ) )
        {
            if( !readChildren( proc, aTopNode ) )
                return false;
        }
        else
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] invalid Switch %s.\n"
                             " * [INFO] file: '%s'\n" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }
    }   // while( true ) -- reading contents of Switch{}

    return true;
}


bool WRL2SWITCH::AddRefNode( WRL2NODE* aNode )
{
    wxCHECK_MSG( aNode, false, wxT( "Invalid node." ) );

    // take possession if the node is dangling WRL2_SHAPE
    if( WRL2NODES::WRL2_SHAPE == aNode->GetNodeType() && aNode->isDangling() )
    {
        WRL2NODE* np = aNode->GetParent();

        if( nullptr != np )
            aNode->SetParent( this );

        if( !WRL2NODE::AddChildNode( aNode ) )
        {
            aNode->SetParent( nullptr );
            return false;
        }
    }

    if( !WRL2NODE::AddRefNode( aNode ) )
        return false;

    return true;
}


bool WRL2SWITCH::readChildren( WRLPROC& proc, WRL2BASE* aTopNode )
{
    char tok = proc.Peek();

    if( proc.eof() )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          " * [INFO] bad file format; unexpected eof %s." ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

        return false;
    }

    WRL2NODE* child = nullptr;

    if( '[' != tok )
    {
        // since there are no delimiters we expect a single child
        if( !aTopNode->ReadNode( proc, this, &child ) )
            return false;

        if( nullptr != child )
            choices.push_back( child );

        if( proc.Peek() == ',' )
            proc.Pop();

        return true;
    }

    proc.Pop();

    while( true )
    {
        if( proc.Peek() == ']' )
        {
            proc.Pop();
            break;
        }

        if( !aTopNode->ReadNode( proc, this, &child ) )
            return false;

        if( nullptr != child )
            choices.push_back( child );

        if( proc.Peek() == ',' )
            proc.Pop();

    }

    return true;
}


SGNODE* WRL2SWITCH::TranslateToSG( SGNODE* aParent )
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating Switch with %zu children, %zu references, and"
                     "%zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );

    if( choices.empty() )
    {
        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Switch translation: no choices." ) );

        return nullptr;
    }

    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    wxCHECK_MSG( aParent && ( ptype == S3D::SGTYPE_TRANSFORM ), nullptr,
                 wxString::Format( wxT( "Switch does not have a Transform parent (parent "
                                        "ID: %d)." ), ptype ) );

    if( whichChoice < 0 || whichChoice >= (int)choices.size() )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( " * [INFO] Switch translation: no choice (choices = %zu), "
                         "whichChoice = %d." ), choices.size(), whichChoice );

        return nullptr;
    }

    WRL2NODES type = choices[whichChoice]->GetNodeType();

    switch( type )
    {
    case WRL2NODES::WRL2_SHAPE:
    case WRL2NODES::WRL2_SWITCH:
    case WRL2NODES::WRL2_INLINE:
    case WRL2NODES::WRL2_TRANSFORM:
        break;

    default:
        return nullptr;
    }

    return choices[whichChoice]->TranslateToSG( aParent );
}
