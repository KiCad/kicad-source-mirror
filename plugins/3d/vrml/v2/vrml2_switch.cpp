/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
    m_Type = WRL2_SWITCH;
    whichChoice = -1;

    return;
}


WRL2SWITCH::WRL2SWITCH( WRL2NODE* aParent ) : WRL2NODE()
{
    m_Type = WRL2_SWITCH;
    m_Parent = aParent;
    whichChoice = -1;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL2SWITCH::~WRL2SWITCH()
{
    #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 2 )
    do {
        std::ostringstream ostr;
        ostr << " * [INFO] Destroying Switch with " << m_Children.size();
        ostr << " children, " << m_Refs.size() << " references and ";
        ostr << m_BackPointers.size() << " backpointers";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return;
}


bool WRL2SWITCH::isDangling( void )
{
    // a Switch node is never dangling
    return false;
}


// functions inherited from WRL2NODE
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

    if( NULL == aTopNode )
    {
        #ifdef DEBUG_VRML2
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
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
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
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
        do {
            std::ostringstream ostr;
            ostr << proc.GetError() << "\n";
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] bad file format; expecting '{' but got '" << tok;
            ostr  << "' at line " << line << ", column " << column;
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
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
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
        // choice
        // whichChoice

        proc.GetFilePosData( line, column );

        if( !glob.compare( "whichChoice" ) )
        {
            if( !proc.ReadSFInt( whichChoice ) )
            {
                #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid whichChoice at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

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
            #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad Switch at line " << line << ", column ";
                ostr << column << "\n";
                ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }
    }   // while( true ) -- reading contents of Switch{}

    return true;
}


bool WRL2SWITCH::AddRefNode( WRL2NODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] NULL passed as node pointer";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    // take possession if the node is dangling WRL2_SHAPE
    if( WRL2_SHAPE == aNode->GetNodeType() && aNode->isDangling() )
    {
        WRL2NODE* np = aNode->GetParent();

        if( NULL != np )
            aNode->SetParent( this );

        if( !WRL2NODE::AddChildNode( aNode ) )
        {
            aNode->SetParent( NULL );
            return false;
        }
    }

    if( !WRL2NODE::AddRefNode( aNode ) )
        return false;

    return true;
}


bool WRL2SWITCH::readChildren( WRLPROC& proc, WRL2BASE* aTopNode )
{
    size_t line, column;
    proc.GetFilePosData( line, column );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 1 )
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

    WRL2NODE* child = NULL;

    if( '[' != tok )
    {
        // since there are no delimeters we expect a single child
        if( !aTopNode->ReadNode( proc, this, &child ) )
            return false;

        if( NULL != child )
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

        if( NULL != child )
            choices.push_back( child );

        if( proc.Peek() == ',' )
            proc.Pop();

    }

    return true;
}


SGNODE* WRL2SWITCH::TranslateToSG( SGNODE* aParent )
{
    #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 2 )
    do {
        std::ostringstream ostr;
        ostr << " * [INFO] Translating Switch with " << m_Children.size();
        ostr << " children, " << m_Refs.size() << " references and ";
        ostr << m_BackPointers.size() << " backpointers";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    if( choices.empty() )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 2 )
        wxLogTrace( MASK_VRML, " * [INFO] Switch translation: no choices\n" );
        #endif

        return NULL;
    }

    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( NULL != aParent && ptype != S3D::SGTYPE_TRANSFORM )
    {
        #ifdef DEBUG_VRML2
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] Switch does not have a Transform parent (parent ID: ";
            ostr << ptype << ")";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return NULL;
    }

    if( whichChoice < 0 || whichChoice >= (int)choices.size() )
    {
        #if defined( DEBUG_VRML2 ) && ( DEBUG_VRML2 > 2 )
        do {
            std::ostringstream ostr;
            ostr << " * [INFO] Switch translation: no choice (choices = ";
            ostr << choices.size() << "), whichChoice = " << whichChoice;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return NULL;
    }

    WRL2NODES type = choices[whichChoice]->GetNodeType();

    switch( type )
    {
    case WRL2_SHAPE:
    case WRL2_SWITCH:
    case WRL2_INLINE:
    case WRL2_TRANSFORM:
        break;

    default:
        return NULL;
    }

    return choices[whichChoice]->TranslateToSG( aParent );
}
