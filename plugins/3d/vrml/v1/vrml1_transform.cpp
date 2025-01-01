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

#include "vrml1_base.h"
#include "vrml1_transform.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1TRANSFORM::WRL1TRANSFORM( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_TRANSFORM;
}


WRL1TRANSFORM::WRL1TRANSFORM( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_TRANSFORM;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL1TRANSFORM::~WRL1TRANSFORM()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying Transform node with %zu children, %zu"
                     "references, and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


bool WRL1TRANSFORM::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    /*
     * Structure of a Transform node:
     *
     * Transform {
     *      SFVec3f     center              0 0 0
     *      SFRotation  rotation            0 0 1 0
     *      SFVec3f     scale               1 1 1
     *      SFRotation  scaleOrientation    0 0 1 0
     *      SFVec3f     translation         0 0 0
     * }
     */

    wxCHECK_MSG( aTopNode, false, wxT( "Invalid top node." ) );

    center.x = 0.0;
    center.y = 0.0;
    center.z = 0.0;

    translation = center;

    rotation.x = 0.0;
    rotation.y = 0.0;
    rotation.z = 1.0;
    rotation.w = 0.0;

    scaleOrientation = rotation;

    scale.x = 1.0;
    scale.y = 1.0;
    scale.z = 1.0;

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
        // rotation
        // scale
        // ScaleOrientation
        // translation

        if( !glob.compare( "center" ) )
        {
            if( !proc.ReadSFVec3f( center ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d"
                                                  " * [INFO] invalid center %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }

            // convert from 1 VRML Unit = 0.1 inch to 1 VRML Unit = 1 mm
            center.x *= 2.54f;
            center.y *= 2.54f;
            center.z *= 2.54f;
        }
        else if( !glob.compare( "rotation" ) )
        {
            if( !proc.ReadSFRotation( rotation ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d"
                                                  " * [INFO] invalid rotation %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "scaleFactor" ) )
        {
            if( !proc.ReadSFVec3f( scale ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d"
                                                  " * [INFO] invalid scale %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "scaleOrientation" ) )
        {
            if( !proc.ReadSFRotation( scaleOrientation ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d"
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
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d"
                                                  " * [INFO] invalid translation %s\n"
                                                  " * [INFO] file: '%s'\n"
                                                  "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }

            // convert from 1 VRML Unit = 0.1 inch to 1 VRML Unit = 1 mm
            translation.x *= 2.54f;
            translation.y *= 2.54f;
            translation.z *= 2.54f;
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d"
                                              " * [INFO] invalid Transform %s\n"
                                              " * [INFO] file: '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }
    }   // while( true ) -- reading contents of Transform{}

    return true;
}


bool WRL1TRANSFORM::AddRefNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddRefNode is not applicable" ) );
}


bool WRL1TRANSFORM::AddChildNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddChildNode is not applicable." ) );
}


SGNODE* WRL1TRANSFORM::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    if( nullptr == m_Parent )
        return nullptr;

    if( WRL1NODES::WRL1_BASE == m_Parent->GetNodeType() )
        return nullptr;

    wxCHECK_MSG( sp, nullptr, wxT( "Bad model: no base data given" ) );

    // rotation
    float rX, rY, rZ, rW;
    rX = rotation.x;
    rY = rotation.y;
    rZ = rotation.z;
    rW = rotation.w;
    glm::mat4 rM = glm::rotate( glm::mat4( 1.0f ), rW, glm::vec3( rX, rY, rZ ) );

    // translation
    float dX, dY, dZ;
    dX = translation.x;
    dY = translation.y;
    dZ = translation.z;
    glm::mat4 tM = glm::translate( glm::mat4( 1.0f ), glm::vec3( dX, dY, dZ ) );

    // center
    dX = center.x;
    dY = center.y;
    dZ = center.z;
    glm::mat4 cM = glm::translate( glm::mat4( 1.0f ), glm::vec3( dX, dY, dZ ) );
    glm::mat4 ncM = glm::translate( glm::mat4( 1.0f ), glm::vec3( -dX, -dY, -dZ ) );

    // scale
    glm::mat4 sM = glm::scale( glm::mat4( 1.0 ), glm::vec3( scale.x, scale.y, scale.z ) );

    // scaleOrientation
    rX = scaleOrientation.x;
    rY = scaleOrientation.y;
    rZ = scaleOrientation.z;
    rW = scaleOrientation.w;
    glm::mat4 srM = glm::rotate( glm::mat4( 1.0f ), rW, glm::vec3( rX, rY, rZ ) );
    glm::mat4 nsrM = glm::rotate( glm::mat4( 1.0f ), -rW, glm::vec3( rX, rY, rZ ) );

    // resultant transform:
    // tx' = tM * cM * rM * srM * sM * nsrM * ncM
    sp->txmatrix = sp->txmatrix * tM * cM * rM * srM * sM * nsrM * ncM;

    return nullptr;
}
