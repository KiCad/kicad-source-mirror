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

#include "vrml2_base.h"
#include "vrml2_transform.h"


WRL2TRANSFORM::WRL2TRANSFORM() : WRL2NODE()
{
    m_Type = WRL2_TRANSFORM;
    return;
}


WRL2TRANSFORM::WRL2TRANSFORM( WRL2NODE* aParent ) : WRL2NODE()
{
    m_Type = WRL2_TRANSFORM;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL2TRANSFORM::~WRL2TRANSFORM()
{
    #ifdef DEBUG
    std::cerr << " * [INFO] Destroying Transform with " << m_Children.size();
    std::cerr << " children, " << m_Refs.size() << " references and ";
    std::cerr << m_BackPointers.size() << " backpointers\n";
    #endif
    return;
}


bool WRL2TRANSFORM::isDangling( void )
{
    // a Transform node is never dangling
    return false;
}


// functions inherited from WRL2NODE
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

    if( NULL == aTopNode )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] aTopNode is NULL\n";
        #endif
        return false;
    }

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
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected eof at line ";
        std::cerr << line << ", column " << column << "\n";
        #endif
        return false;
    }

    if( '{' != tok )
    {
        #ifdef DEBUG
        std::cerr << proc.GetError() << "\n";
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; expecting '{' but got '" << tok;
        std::cerr  << "' at line " << line << ", column " << column << "\n";
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
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

            return false;
        }

        // expecting one of:
        // center
        // children
        // rotation
        // scale
        // ScaleOrientation
        // translation

        proc.GetFilePosData( line, column );

        if( !glob.compare( "center" ) )
        {
            if( !proc.ReadSFVec3f( center ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid center at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "rotation" ) )
        {
            if( !proc.ReadSFRotation( rotation ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid rotation at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "scale" ) )
        {
            if( !proc.ReadSFVec3f( scale ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid scale at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "scaleOrientation" ) )
        {
            if( !proc.ReadSFRotation( scaleOrientation ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid scaleOrientation at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "translation" ) )
        {
            if( !proc.ReadSFVec3f( translation ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid translation at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "children" ) )
        {
            if( !readChildren( proc, aTopNode ) )
                return false;
        }
        else
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad Transform at line " << line << ", column ";
            std::cerr << column << "\n";
            std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
            #endif

            return false;
        }
    }   // while( true ) -- reading contents of Transform{}

    return true;
}


bool WRL2TRANSFORM::AddRefNode( WRL2NODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL passed as node pointer\n";
        #endif
        return false;
    }

    if( !WRL2NODE::AddRefNode( aNode ) )
        return false;

    // take possession if the node is dangling WRL2_SHAPE

    if( WRL2_SHAPE == aNode->GetNodeType() && aNode->isDangling() )
    {
        WRL2NODE* np = aNode->GetParent();

        if( NULL != np )
            aNode->SetParent( this );
    }

    return true;
}


bool WRL2TRANSFORM::readChildren( WRLPROC& proc, WRL2BASE* aTopNode )
{
    size_t line, column;
    proc.GetFilePosData( line, column );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected eof at line ";
        std::cerr << line << ", column " << column << "\n";
        #endif
        return false;
    }

    if( '[' != tok )
    {
        // since there are no delimeters we expect a single child
        if( !aTopNode->ReadNode( proc, this, NULL ) )
            return false;

        return true;
    }

    proc.Pop();
    std::string glob;

    while( true )
    {
        if( proc.Peek() == ']' )
        {
            proc.Pop();
            break;
        }

        if( !aTopNode->ReadNode( proc, this, NULL ) )
            return false;
    }

    return true;
}


SGNODE* WRL2TRANSFORM::TranslateToSG( SGNODE* aParent )
{
    // XXX - TO IMPLEMENT
    return NULL;
}
