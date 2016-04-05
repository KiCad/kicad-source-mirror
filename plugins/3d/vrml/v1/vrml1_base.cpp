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
#include "vrml1_group.h"
#include "vrml1_separator.h"
#include "vrml1_material.h"
#include "vrml1_matbinding.h"
#include "vrml1_coords.h"
#include "vrml1_switch.h"
#include "vrml1_faceset.h"
#include "vrml1_transform.h"
#include "vrml1_shapehints.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1BASE::WRL1BASE() : WRL1NODE( NULL )
{
    m_Type = WRL1_BASE;
    m_dictionary = new NAMEREGISTER;
    return;
}


WRL1BASE::~WRL1BASE()
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    wxLogTrace( MASK_VRML, " * [INFO] Destroying virtual base node\n" );
    #endif

    cancelDict();
    return;
}


// functions inherited from WRL1NODE
bool WRL1BASE::SetParent( WRL1NODE* aParent, bool /* doUnlink */ )
{
    #ifdef DEBUG_VRML1
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] attempting to set parent on WRL1BASE node";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return false;
}


std::string WRL1BASE::GetName( void )
{
    #ifdef DEBUG_VRML1
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] attempting to extract name from virtual base node";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return std::string( "" );
}


bool WRL1BASE::SetName( const std::string& aName )
{
    #ifdef DEBUG_VRML1
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] attempting to set name on virtual base node";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return false;
}


bool WRL1BASE::Read( WRLPROC& proc )
{
    if( proc.GetVRMLType() != VRML_V1 )
    {
        #ifdef DEBUG_VRML1
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] no open file or file is not a VRML1 file";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    // Note: according to the VRML1 specification, a file may contain
    // only one grouping node at the top level. The following code
    // supports non-conformant VRML1 files by processing all top level
    // nodes as if the vrml1_base were the equivalent of a vrml1_separator

    while( proc.Peek() )
    {
        size_t line, column;
        proc.GetFilePosData( line, column );

        if( !ReadNode( proc, this, NULL ) )
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
    }

    if( !proc.eof() )
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

    return true;
}


bool WRL1BASE::implementUse( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    if( !aParent )
    {
        #ifdef DEBUG_VRML1
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] invoked with NULL parent";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    std::string glob;

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

    WRL1NODE* ref = aParent->FindNode( glob );

    // return 'true' - the file may be defective but it may still be somewhat OK
    if( NULL == ref )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] node '" << glob << "' not found";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return true;
    }

    if( !aParent->AddRefNode( ref ) )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [INFO] failed to add node '" << glob << "' (";
            ostr << ref->GetNodeTypeName( ref->GetNodeType() ) << ") to parent of type ";
            ostr << aParent->GetNodeTypeName( aParent->GetNodeType() );
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( NULL != aNode )
        *aNode = ref;

    return true;
}


bool WRL1BASE::implementDef( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    if( NULL == aParent )
    {
        #ifdef DEBUG_VRML1
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] invalid parent pointer (NULL)";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    std::string glob;
    WRL1NODE* lnode = NULL;

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

    size_t line, column;
    proc.GetFilePosData( line, column );

    if( ReadNode( proc, aParent, &lnode ) )
    {
        if( NULL != aNode )
            *aNode = lnode;

        if( lnode && !lnode->SetName( glob ) )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            do {
                std::ostringstream ostr;
                size_t line, column;
                proc.GetFilePosData( line, column );
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad formatting (invalid name) at line";
                ostr << line << ", column " << column;
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }

        if( !m_dictionary )
            return false;

        m_dictionary->AddName( glob, lnode );

        return true;
    }

    return false;
}


bool WRL1BASE::ReadNode( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    // This function reads a node and stores a pointer to it in aNode.
    // A value 'true' is returned if a node is successfully read or,
    // if the node is not supported, successfully discarded. Callers
    // must always check the value of aNode when the function returns
    // 'true' since it will be NULL if the node type is not supported.

    if( NULL != aNode )
        *aNode = NULL;

    if( NULL == aParent )
    {
        #ifdef DEBUG_VRML1
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] invalid parent pointer (NULL)";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    std::string glob;
    WRL1NODES ntype;

    if( !proc.ReadName( glob ) )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        if( !proc.eof() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << proc.GetError();
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        }
        #endif

        return false;
    }

    // Process node name:
    // the names encountered at this point should be one of the
    // built-in node names or one of:
    // DEF, USE
    if( !glob.compare( "USE" ) )
    {
        if( !implementUse( proc, aParent, aNode ) )
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

        return true;
    }

    if( !glob.compare( "DEF" ) )
    {
        if( !implementDef( proc, aParent, aNode ) )
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

        return true;
    }

    ntype = getNodeTypeID( glob );
    size_t line = 0;
    size_t column = 0;
    proc.GetFilePosData( line, column );

    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    do {
        std::ostringstream ostr;
        ostr << " * [INFO] Processing node '" << glob << "' ID: " << ntype;
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    switch( ntype )
    {
    case WRL1_GROUP:

        if( !readGroup( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_SEPARATOR:

        if( !readSeparator( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_SWITCH:

        if( !readSwitch( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_MATERIAL:

        if( !readMaterial( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_MATERIALBINDING:

        if( !readMatBinding( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_COORDINATE3:

        if( !readCoords( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_INDEXEDFACESET:

        if( !readFaceSet( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_TRANSFORM:
    case WRL1_TRANSLATION:
    case WRL1_ROTATION:
    case WRL1_SCALE:

        if( !readTransform( proc, aParent, aNode ) )
            return false;

        break;

    case WRL1_SHAPEHINTS:

        if( !readShapeHints( proc, aParent, aNode ) )
            return false;

        break;

    //
    // items not implemented or for optional future implementation:
    //
    default:

        proc.GetFilePosData( line, column );

        if( !proc.DiscardNode() )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            do {
                std::ostringstream ostr;
                ostr << proc.GetError() << "\n";
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] could not discard node at line " << line;
                ostr << ", column " << column;
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            return false;
        }
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        else
        {
            std::ostringstream ostr;
            ostr << " * [INFO] discarded node '" << glob << "' at line ";
            ostr << line << ", col " << column << " (currently unsupported)";
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        }
        #endif

        break;
    }

    return true;
}


bool WRL1BASE::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    // this function makes no sense in the base node
    #ifdef DEBUG_VRML1
    do {
        std::ostringstream ostr;
        ostr << proc.GetError() << "\n";
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [BUG] this method must never be invoked on a WRL1BASE object";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return false;
}


bool WRL1BASE::readGroup( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1GROUP* np = new WRL1GROUP( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readSeparator( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1SEPARATOR* np = new WRL1SEPARATOR( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readSwitch( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    WRL1SWITCH* np = new WRL1SWITCH( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readMaterial( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1MATERIAL* np = new WRL1MATERIAL( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readMatBinding( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1MATBINDING* np = new WRL1MATBINDING( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readCoords( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1COORDS* np = new WRL1COORDS( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readFaceSet( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1FACESET* np = new WRL1FACESET( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readTransform( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1TRANSFORM* np = new WRL1TRANSFORM( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


bool WRL1BASE::readShapeHints( WRLPROC& proc, WRL1NODE* aParent, WRL1NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL1SHAPEHINTS* np = new WRL1SHAPEHINTS( m_dictionary, aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL1NODE*) np;

    return true;
}


SGNODE* WRL1BASE::TranslateToSG( SGNODE* aParent, WRL1STATUS* /*sp*/ )
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    do {
        std::ostringstream ostr;
        ostr << " * [INFO] Translating VRML1 Base with " << m_Items.size();
        ostr << " items";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    if( m_Items.empty() )
        return NULL;

    if( m_Items.size() == 1 )
        return (*m_Items.begin())->TranslateToSG( NULL, NULL );

    // Note: according to the VRML1 specification, a file may contain
    // only one grouping node at the top level. The following code
    // supports non-conformant VRML1 files.

    m_current.Init();

    IFSG_TRANSFORM txNode( true );
    bool hasContent = false;

    std::list< WRL1NODE* >::iterator sI = m_Items.begin();
    std::list< WRL1NODE* >::iterator eI = m_Items.end();

    SGNODE* node = txNode.GetRawPtr();

    while( sI != eI )
    {
        if( NULL != (*sI)->TranslateToSG( node, &m_current ) )
            hasContent = true;

        ++sI;
    }

    if( !hasContent )
    {
        txNode.Destroy();
        return NULL;
    }

    return node;
}
