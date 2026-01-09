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
#include <cmath>
#include <wx/log.h>

#include "vrml2_base.h"
#include "vrml2_transform.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2TRANSFORM::WRL2TRANSFORM() : WRL2NODE()
{
    m_Type = WRL2NODES::WRL2_TRANSFORM;
}


WRL2TRANSFORM::WRL2TRANSFORM( WRL2NODE* aParent ) : WRL2NODE()
{
    m_Type = WRL2NODES::WRL2_TRANSFORM;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL2TRANSFORM::~WRL2TRANSFORM()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying Transform node with %zu children, %zu"
                     "references, and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


bool WRL2TRANSFORM::isDangling( void )
{
    // a Transform node is never dangling
    return false;
}


bool WRL2TRANSFORM::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
    /*
     * Structure of a Transform node (p.120):
     *
     * Transform {
     *      eventIn         MFNode      addChildren
     *      eventIn         MFNode      removeChildren
     *      exposedField    SFVec3f     center              0 0 0
     *      exposedField    MFNode      children            []
     *      exposedField    SFRotation  rotation            0 0 1 0
     *      exposedField    SFVec3f     scale               1 1 1
     *      exposedField    SFRotation  scaleOrientation    0 0 1 0
     *      exposedField    SFVec3f     translation         0 0 0
     *      field           SFVec3f     bboxCenter          0 0 0
     *      field           SFVec3f     bboxSize            0 0 0
     * }
     */

    wxCHECK_MSG( aTopNode, false, wxT( "Invalid top node." ) );

    center.x = 0.0;
    center.y = 0.0;
    center.z = 0.0;

    translation = center;
    bboxCenter = center;
    bboxSize = center;

    rotation.x = 0.0;
    rotation.y = 0.0;
    rotation.z = 1.0;
    rotation.w = 0.0;

    scaleOrientation = rotation;

    scale.x = 1.0;
    scale.y = 1.0;
    scale.z = 1.0;

    size_t line, column;
    proc.GetFilePosData( line, column );

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
        // center
        // children
        // rotation
        // scale
        // ScaleOrientation
        // translation
        if( !glob.compare( "center" ) )
        {
            if( !proc.ReadSFVec3f( center ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid center %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }

            // Convert from 1 VRML Unit = 0.1 inch to 1 VRML Unit = 1 mm
            // Only apply if using legacy unit conversion mode
            if( aTopNode->GetApplyUnitConversion() )
            {
                center.x *= 2.54f;
                center.y *= 2.54f;
                center.z *= 2.54f;
            }
        }
        else if( !glob.compare( "rotation" ) )
        {
            if( !proc.ReadSFRotation( rotation ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid rotation %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "scale" ) )
        {
            if( !proc.ReadSFVec3f( scale ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid scale %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }

            // If this is a top-level transform (parent is WRL2_BASE) with a non-unity scale,
            // disable unit conversion. This handles PCBnew-exported VRML which includes a
            // top-level scale factor, meaning coordinates are already properly scaled.
            if( m_Parent != nullptr && m_Parent->GetNodeType() == WRL2NODES::WRL2_BASE
                && HasNonUnityScale() )
            {
                aTopNode->SetApplyUnitConversion( false );

                wxLogTrace( traceVrmlPlugin,
                            wxT( " * [INFO] Detected top-level scale in VRML file, "
                                 "disabling unit conversion." ) );
            }
        }
        else if( !glob.compare( "scaleOrientation" ) )
        {
            if( !proc.ReadSFRotation( scaleOrientation ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid scaleOrientation %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "translation" ) )
        {
            if( !proc.ReadSFVec3f( translation ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                  " * [INFO] invalid translation %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }

            // Convert from 1 VRML Unit = 0.1 inch to 1 VRML Unit = 1 mm
            // Only apply if using legacy unit conversion mode
            if( aTopNode->GetApplyUnitConversion() )
            {
                translation.x *= 2.54f;
                translation.y *= 2.54f;
                translation.z *= 2.54f;
            }
        }
        else if( !glob.compare( "children" ) )
        {
            if( !readChildren( proc, aTopNode ) )
                return false;
        }
        else
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] invalid Transform %s.\n"
                             " * [INFO] file: '%s'\n" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }
    }   // while( true ) -- reading contents of Transform{}

    return true;
}


bool WRL2TRANSFORM::HasNonUnityScale() const
{
    // Check if any scale component differs from 1.0 by more than a small tolerance
    const float tolerance = 0.001f;

    return ( std::fabs( scale.x - 1.0f ) > tolerance )
        || ( std::fabs( scale.y - 1.0f ) > tolerance )
        || ( std::fabs( scale.z - 1.0f ) > tolerance );
}


bool WRL2TRANSFORM::AddRefNode( WRL2NODE* aNode )
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


bool WRL2TRANSFORM::readChildren( WRLPROC& proc, WRL2BASE* aTopNode )
{
    char tok = proc.Peek();

    if( proc.eof() )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          " * [INFO] bad file format; unexpected eof %s." ),
                    __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

        return false;
    }

    if( '[' != tok )
    {
        // since there are no delimiters we expect a single child
        if( !aTopNode->ReadNode( proc, this, nullptr ) )
            return false;

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

        if( !aTopNode->ReadNode( proc, this, nullptr ) )
            return false;

        if( proc.Peek() == ',' )
            proc.Pop();
    }

    return true;
}


SGNODE* WRL2TRANSFORM::TranslateToSG( SGNODE* aParent )
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating Switch with %zu children, %zu references, and"
                     "%zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );

    if( m_Children.empty() && m_Refs.empty() )
        return nullptr;

    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    wxCHECK_MSG( aParent && ( ptype == S3D::SGTYPE_TRANSFORM ), nullptr,
                 wxString::Format( wxT( "Transform does not have a Transform parent (parent "
                                        "ID: %d)." ), ptype ) );

    if( m_sgNode )
    {
        if( nullptr != aParent )
        {
            if( nullptr == S3D::GetSGNodeParent( m_sgNode )
                && !S3D::AddSGNodeChild( aParent, m_sgNode ) )
            {
                return nullptr;
            }
            else if( aParent != S3D::GetSGNodeParent( m_sgNode )
                     && !S3D::AddSGNodeRef( aParent, m_sgNode ) )
            {
                return nullptr;
            }
        }

        return m_sgNode;
    }

    IFSG_TRANSFORM txNode( aParent );

    std::list< WRL2NODE* >::iterator sC = m_Children.begin();
    std::list< WRL2NODE* >::iterator eC = m_Children.end();
    WRL2NODES type;

    // Include only the following in a Transform node:
    // Shape
    // Switch
    // Transform
    // Inline
    bool test = false;  // set to true if there are any subnodes for display

    for( int i = 0; i < 2; ++i )
    {
        while( sC != eC )
        {
            type = (*sC)->GetNodeType();

            switch( type )
            {
            case WRL2NODES::WRL2_SHAPE:
            case WRL2NODES::WRL2_SWITCH:
            case WRL2NODES::WRL2_INLINE:
            case WRL2NODES::WRL2_TRANSFORM:

                if( nullptr != (*sC)->TranslateToSG( txNode.GetRawPtr() ) )
                    test = true;

                break;

            default:
                break;
            }

            ++ sC;
        }

        sC = m_Refs.begin();
        eC = m_Refs.end();
    }

    if( false == test )
    {
        txNode.Destroy();
        return nullptr;
    }

    txNode.SetScale( SGPOINT( scale.x, scale.y, scale.z ) );
    txNode.SetCenter( SGPOINT( center.x, center.y, center.z ) );
    txNode.SetTranslation( SGPOINT( translation.x, translation.y, translation.z ) );
    txNode.SetScaleOrientation( SGVECTOR( scaleOrientation.x, scaleOrientation.y,
                                          scaleOrientation.z ), scaleOrientation.w );
    txNode.SetRotation( SGVECTOR( rotation.x, rotation.y, rotation.z), rotation.w );

    m_sgNode = txNode.GetRawPtr();

    return m_sgNode;
}
