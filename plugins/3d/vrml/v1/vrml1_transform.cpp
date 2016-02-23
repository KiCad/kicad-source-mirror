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

#include "vrml1_base.h"
#include "vrml1_transform.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1TRANSFORM::WRL1TRANSFORM( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_Type = WRL1_TRANSFORM;
    return;
}


WRL1TRANSFORM::WRL1TRANSFORM( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_Type = WRL1_TRANSFORM;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL1TRANSFORM::~WRL1TRANSFORM()
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    do {
        std::ostringstream ostr;
        ostr << " * [INFO] Destroying Transform with " << m_Children.size();
        ostr << " children, " << m_Refs.size() << " references and ";
        ostr << m_BackPointers.size() << " backpointers";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return;
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
        // center
        // rotation
        // scale
        // ScaleOrientation
        // translation

        proc.GetFilePosData( line, column );

        if( !glob.compare( "center" ) )
        {
            if( !proc.ReadSFVec3f( center ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid center at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }

            // convert from 1 VRML Unit = 0.1 inch to 1 VRML Unit = 1 mm
            center.x *= 2.54;
            center.y *= 2.54;
            center.z *= 2.54;
        }
        else if( !glob.compare( "rotation" ) )
        {
            if( !proc.ReadSFRotation( rotation ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid rotation at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "scaleFactor" ) )
        {
            if( !proc.ReadSFVec3f( scale ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid scale at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "scaleOrientation" ) )
        {
            if( !proc.ReadSFRotation( scaleOrientation ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid scaleOrientation at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "translation" ) )
        {
            if( !proc.ReadSFVec3f( translation ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid translation at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError() << "'";
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }

            // convert from 1 VRML Unit = 0.1 inch to 1 VRML Unit = 1 mm
            translation.x *= 2.54;
            translation.y *= 2.54;
            translation.z *= 2.54;
        }
        else
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad Transform at line " << line << ", column ";
                ostr << column << "\n";
                ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }
    }   // while( true ) -- reading contents of Transform{}

    return true;
}


bool WRL1TRANSFORM::AddRefNode( WRL1NODE* aNode )
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


bool WRL1TRANSFORM::AddChildNode( WRL1NODE* aNode )
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


SGNODE* WRL1TRANSFORM::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    if( NULL == m_Parent )
        return NULL;

    if( WRL1_BASE == m_Parent->GetNodeType() )
        return NULL;

    if( NULL == sp )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        wxLogTrace( MASK_VRML, " * [INFO] bad model: no base data given\n" );
        #endif

        return NULL;
    }

    // rotation
    float rX, rY, rZ, rW;
    rX = rotation.x;
    rY = rotation.y;
    rZ = rotation.z;
    rW = rotation.w;
    glm::mat4 rM = glm::rotate( rW, glm::vec3( rX, rY, rZ ) );
    // translation
    float dX, dY, dZ;
    dX = translation.x;
    dY = translation.y;
    dZ = translation.z;
    glm::mat4 tM = glm::translate( glm::vec3( dX, dY, dZ ) );
    // center
    dX = center.x;
    dY = center.y;
    dZ = center.z;
    glm::mat4 cM = glm::translate( glm::vec3( dX, dY, dZ ) );
    glm::mat4 ncM = glm::translate( glm::vec3( -dX, -dY, -dZ ) );
    // scale
    glm::mat4 sM = glm::scale( glm::mat4( 1.0 ), glm::vec3( scale.x, scale.y, scale.z ) );
    // scaleOrientation
    rX = scaleOrientation.x;
    rY = scaleOrientation.y;
    rZ = scaleOrientation.z;
    rW = scaleOrientation.w;
    glm::mat4 srM = glm::rotate( rW, glm::vec3( rX, rY, rZ ) );
    glm::mat4 nsrM = glm::rotate( -rW, glm::vec3( rX, rY, rZ ) );

    // resultant transform:
    // tx' = tM * cM * rM * srM * sM * nsrM * ncM
    sp->txmatrix = sp->txmatrix * tM * cM * rM * srM * sM * nsrM * ncM;

    return NULL;
}
