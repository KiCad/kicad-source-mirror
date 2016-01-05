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

#include "vrml1_base.h"
#include "vrml1_separator.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1SEPARATOR::WRL1SEPARATOR( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_Type = WRL1_SEPARATOR;
    return;
}


WRL1SEPARATOR::WRL1SEPARATOR( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_Type = WRL1_SEPARATOR;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL1SEPARATOR::~WRL1SEPARATOR()
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    std::cerr << " * [INFO] Destroying Transform with " << m_Children.size();
    std::cerr << " children, " << m_Refs.size() << " references and ";
    std::cerr << m_BackPointers.size() << " backpointers\n";
    #endif
    return;
}


// functions inherited from WRL1NODE
bool WRL1SEPARATOR::Read( WRLPROC& proc, WRL1BASE* aTopNode )
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
        #ifdef DEBUG_VRML1
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] aTopNode is NULL\n";
        #endif
        return false;
    }

    size_t line, column;
    proc.GetFilePosData( line, column );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected eof at line ";
        std::cerr << line << ", column " << column << "\n";
        #endif
        return false;
    }

    if( '{' != tok )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
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

        proc.GetFilePosData( line, column );

        if( !aTopNode->ReadNode( proc, this, NULL ) )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; unexpected eof at line ";
            std::cerr << line << ", column " << column << "\n";
            #endif

            return false;
        }

        if( proc.Peek() == ',' )
            proc.Pop();

    }   // while( true ) -- reading contents of Separator{}

    return true;
}


SGNODE* WRL1SEPARATOR::TranslateToSG( SGNODE* aParent, bool calcNormals )
{
    return NULL;

    /*
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    std::cerr << " * [INFO] Translating Transform with " << m_Children.size();
    std::cerr << " children, " << m_Refs.size() << " references and ";
    std::cerr << m_BackPointers.size() << " backpointers\n";
    #endif

    if( m_Children.empty() && m_Refs.empty() )
        return NULL;

    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( NULL != aParent && ptype != S3D::SGTYPE_TRANSFORM )
    {
        #ifdef DEBUG_VRML1
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] Transform does not have a Transform parent (parent ID: ";
        std::cerr << ptype << ")\n";
        #endif

        return NULL;
    }

    if( m_sgNode )
    {
        if( NULL != aParent )
        {
            if( NULL == S3D::GetSGNodeParent( m_sgNode )
                && !S3D::AddSGNodeChild( aParent, m_sgNode ) )
            {
                return NULL;
            }
            else if( aParent != S3D::GetSGNodeParent( m_sgNode )
                     && !S3D::AddSGNodeRef( aParent, m_sgNode ) )
            {
                return NULL;
            }
        }

        return m_sgNode;
    }

    IFSG_TRANSFORM txNode( aParent );

    std::list< WRL1NODE* >::iterator sC = m_Children.begin();
    std::list< WRL1NODE* >::iterator eC = m_Children.end();
    WRL1NODES type;

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
            case WRL1_SHAPE:
            case WRL1_SWITCH:
            case WRL1_INLINE:
            case WRL1_SEPARATOR:

                if( NULL != (*sC)->TranslateToSG( txNode.GetRawPtr(), calcNormals ) )
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
        return NULL;
    }

    txNode.SetScale( SGPOINT( scale.x, scale.y, scale.z ) );
    txNode.SetCenter( SGPOINT( center.x, center.y, center.z ) );
    txNode.SetTranslation( SGPOINT( translation.x, translation.y, translation.z ) );
    txNode.SetScaleOrientation( SGVECTOR( scaleOrientation.x, scaleOrientation.y,
        scaleOrientation.z ), scaleOrientation.w );
    txNode.SetRotation( SGVECTOR( rotation.x, rotation.y, rotation.z), rotation.w );

    m_sgNode = txNode.GetRawPtr();

    return m_sgNode;
    */
}
