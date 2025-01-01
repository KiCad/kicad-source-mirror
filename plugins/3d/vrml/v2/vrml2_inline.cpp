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

#include "vrml2_inline.h"
#include "vrml2_base.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2INLINE::WRL2INLINE() : WRL2NODE()
{
    m_VRML2Base = nullptr;
    m_Type = WRL2NODES::WRL2_INLINE;
    m_Parent = nullptr;
}


WRL2INLINE::WRL2INLINE( WRL2NODE* aParent ) : WRL2NODE()
{
    m_VRML2Base = nullptr;
    m_Type = WRL2NODES::WRL2_INLINE;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL2INLINE::~WRL2INLINE()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying Inline node." ) );
}


bool WRL2INLINE::isDangling( void )
{
    // this node is never dangling
    return false;
}


bool WRL2INLINE::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
    if( aTopNode == nullptr || aTopNode->GetNodeType() != WRL2NODES::WRL2_BASE )
        return false;

    m_VRML2Base = aTopNode;
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

    while( ( tok = proc.Peek() ) != 0 )
    {
        std::string glob;

        if( tok == '}' )
        {
            proc.Pop();
            return true;
        }

        if( !proc.ReadName( glob ) )
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

            return false;
        }

        // expecting one of 'url', 'bboxCenter', 'bboxSize'
        if( !glob.compare( "url" ) )
        {
            if( !proc.ReadMFString( url ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid url %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "bboxCenter" ) )
        {
            if( !proc.ReadSFVec3f( bboxCenter ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid bboxCenter %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "bboxSize" ) )
        {
            if( !proc.ReadSFVec3f( bboxSize ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid bboxSize %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] invalid Inline %s\n"
                             " * [INFO] file: '%s'\n" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }
    }

    wxLogTrace( traceVrmlPlugin,
                wxT( "%s:%s:%d\n"
                     " * [INFO] invalid Inline %s (no closing brace)\n"
                     " * [INFO] file: '%s'\n" ),
                __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                proc.GetFileName() );

    return false;
}


bool WRL2INLINE::AddRefNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddRefNode is not applicable." ) );
}


bool WRL2INLINE::AddChildNode( WRL2NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddChildNode is not applicable." ) );

    return false;
}


SGNODE* WRL2INLINE::TranslateToSG( SGNODE* aParent )
{
    if( nullptr == aParent || nullptr == m_VRML2Base )
        return nullptr;

    if( url.empty() )
        return nullptr;

    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    wxCHECK_MSG( aParent && ( ptype == S3D::SGTYPE_TRANSFORM ), nullptr,
                 wxString::Format( wxT( " * [BUG] Appearance does not have a Transform parent "
                                        "(parent ID: %d)." ), ptype ) );

    SGNODE* np = m_VRML2Base->GetInlineData( url.front() );

    if( nullptr == np )
        return nullptr;

    bool OK = false;

    if( nullptr == S3D::GetSGNodeParent( np ) )
        OK = S3D::AddSGNodeChild( aParent, np );
    else
        OK = S3D::AddSGNodeRef( aParent, np );

    if( !OK )
        return nullptr;

    return np;
}
