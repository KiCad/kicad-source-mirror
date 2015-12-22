/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
    return;
}


WRL2TRANSFORM::~WRL2TRANSFORM()
{
    return;
}


void WRL2TRANSFORM::unlinkChildNode( const WRL2NODE* aNode )
{
    // XXX - TO BE IMPLEMENTED
}

void WRL2TRANSFORM::unlinkRefNode( const WRL2NODE* aNode )
{
    // XXX - TO BE IMPLEMENTED
}


bool WRL2TRANSFORM::isDangling( void )
{
    // XXX - TO BE IMPLEMENTED
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
     *      exposedField    SFRotation  ScaleOrientation    0 0 1 0
     *      exposedField    SFVec3f     translation         0 0 0
     *      field           SFVec3f     bboxCenter          0 0 0
     *      field           SFVec3f     bboxSize            0 0 0
     * }
     */

    // XXX - TO BE IMPLEMENTED
    // XXX - at the moment this is half-assed code; it needs to be checked and expanded

    size_t line, column;
    proc.GetFilePosData( line, column );

    char tok = proc.Peek();

    if( '\0' != tok )
        proc.GetFilePosData( line, column );

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
            return true;

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

        // XXX - TO BE IMPLEMENTED
    }

    // XXX - TO BE IMPLEMENTED
    return false;
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
