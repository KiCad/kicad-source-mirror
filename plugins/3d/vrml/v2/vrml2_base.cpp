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

#include "vrml2_base.h"
#include "vrml2_transform.h"


WRL2BASE::WRL2BASE() : WRL2NODE()
{
    m_Type = WRL2_BASE;
    return;
}


WRL2BASE::~WRL2BASE()
{
    return;
}


// functions inherited from WRL2NODE
bool WRL2BASE::SetParent( WRL2NODE* aParent )
{
    #ifdef DEBUG
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] attempting to set parent on WRL2BASE node\n";
    #endif

    return false;
}


std::string WRL2BASE::GetName( void )
{
    #ifdef DEBUG
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] attempting to extract name from virtual base node\n";
    #endif

    return std::string( "" );
}


bool WRL2BASE::SetName( const std::string& aName )
{
    #ifdef DEBUG
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] attempting to set name on virtual base node\n";
    #endif
    return false;
}


void WRL2BASE::unlinkChildNode( const WRL2NODE* aNode )
{
    std::list< WRL2NODE* >::iterator sL = m_Children.begin();
    std::list< WRL2NODE* >::iterator eL = m_Children.end();

    while( sL != eL )
    {
        if( *sL == aNode )
        {
            m_Children.erase( sL );
            return;
        }

        ++sL;
    }

    return;
}


void WRL2BASE::unlinkRefNode( const WRL2NODE* aNode )
{
    #ifdef DEBUG
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] WRL2BASE was invoked to unlink a referenced node\n";
    #endif

    return;
}


bool WRL2BASE::Read( WRLPROC& proc )
{
    if( proc.GetVRMLType() != VRML_V2 )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] no open file or file is not a VRML2 file\n";
        #endif
        return false;
    }

    WRL2NODE* node = NULL;

    while( !ReadNode( proc, this, &node ) );

    if( proc.eof() )
        return true;

    return false;
}


bool WRL2BASE::isDangling( void )
{
    return false;
}


bool WRL2BASE::implementUse( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invalid node handle (NULL)\n";
        #endif

        return false;
    }

    if( !aParent )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invoked with NULL parent\n";
        #endif

        return false;
    }

    *aNode = NULL;
    std::string glob;

    if( !proc.ReadName( glob ) )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << proc.GetError() <<  "\n";
        #endif

        return false;
    }

    WRL2NODE* ref = aParent->FindNode( glob, NULL );

    // return 'true' - the file may be defective but it may still be somewhat OK
    if( NULL == ref )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] node '" << glob << "' not found\n";
        #endif

        return true;
    }

    if( !aParent->AddRefNode( ref ) )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] failed to add node '" << glob << "' (";
        std::cerr << ref->GetNodeTypeName( ref->GetNodeType() ) << ") to parent of type ";
        std::cerr << aParent->GetNodeTypeName( aParent->GetNodeType() ) << "\n";
        #endif

        return false;
    }

    *aNode = ref;

    return true;
}


bool WRL2BASE::implementDef( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invalid node handle (NULL)\n";
        #endif

        return false;
    }

    if( NULL == aParent )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invalid parent pointer (NULL)\n";
        #endif

        return false;
    }

    *aNode = NULL;

    std::string glob;

    if( !proc.ReadName( glob ) )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << proc.GetError() <<  "\n";
        #endif

        return false;
    }

    size_t line, column;
    proc.GetFilePosData( line, column );

    if( ReadNode( proc, aParent, aNode ) )
    {
        if( *aNode && !(*aNode)->SetName( glob ) )
        {
            #ifdef DEBUG
            size_t line, column;
            proc.GetFilePosData( line, column );
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad formatting (invalid name) at line";
            std::cerr << line << ", column " << column << "\n";
            #endif

            return false;
        }
    }

    return true;
}


bool WRL2BASE::ReadNode( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invalid node handle (NULL)\n";
        #endif

        return false;
    }

    if( NULL == aParent )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] invalid parent pointer (NULL)\n";
        #endif

        return false;
    }

    *aNode = NULL;

    std::string glob;
    WRL2NODES ntype;

    if( !proc.ReadName( glob ) )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << proc.GetError() <<  "\n";
        #endif

        return false;
    }

    // Process node name:
    // the names encountered at this point should be one of the
    // built-in node names or one of:
    // DEF, USE
    // PROTO, EXTERNPROTO
    // ROUTE
    // any PROTO or EXTERNPROTO defined name
    // since we do not support PROTO or EXTERNPROTO, any unmatched names are
    // assumed to be defined via PROTO/EXTERNPROTO and deleted according to
    // a typical pattern.
    if( !glob.compare( "USE" ) )
    {
        if( !implementUse( proc, aParent, aNode ) )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

            return false;
        }

        return true;
    }

    if( !glob.compare( "DEF" ) )
    {
        if( !implementDef( proc, aParent, aNode ) )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

            return false;
        }

        return true;
    }

    if( !glob.compare( "PROTO" ) )
    {
        if( !proc.ReadName( glob ) || !proc.DiscardList() )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

            return false;
        }

        return true;
    }

    if( !glob.compare( "EXTERNPROTO" ) )
    {
        if( !proc.ReadName( glob ) || !proc.ReadName( glob ) || !proc.DiscardList() )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

            return false;
        }

        return true;
    }

    if( !glob.compare( "ROUTE" ) )
    {
        if( !proc.ReadGlob( glob ) || !proc.ReadGlob( glob ) || !proc.ReadGlob( glob ) )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

            return false;
        }

        return true;
    }

    ntype = getNodeTypeID( glob );
    size_t line = 0;
    size_t column = 0;

    switch( ntype )
    {
        //
        // items to be implemented:
        //
    case WRL2_APPEARANCE:
        // XXX - IMPLEMENT
        break;

    case WRL2_BOX:
        // XXX - IMPLEMENT
        break;

    case WRL2_COLOR:
        // XXX - IMPLEMENT
        break;

    case WRL2_CONE:
        // XXX - IMPLEMENT
        break;

    case WRL2_COORDINATE:
        // XXX - IMPLEMENT
        break;

    case WRL2_CYLINDER:
        // XXX - IMPLEMENT
        break;

    case WRL2_ELEVATIONGRID:
        // XXX - IMPLEMENT
        break;

    case WRL2_EXTRUSION:
        // XXX - IMPLEMENT
        break;

    case WRL2_INDEXEDFACESET:
        // XXX - IMPLEMENT
        break;

    case WRL2_MATERIAL:
        // XXX - IMPLEMENT
        break;

    case WRL2_NORMAL:
        // XXX - IMPLEMENT
        break;

    case WRL2_SHAPE:
        // XXX - IMPLEMENT
        break;

    case WRL2_SPHERE:
        // XXX - IMPLEMENT
        break;

    case WRL2_TRANSFORM:
    case WRL2_GROUP:

        if( !readTransform( proc, aParent, aNode ) )
            return false;

        break;

    //
    // items not implemented or for optional future implementation:
    //
    case WRL2_ANCHOR:
    case WRL2_AUDIOCLIP:
    case WRL2_BACKGROUND:
    case WRL2_BILLBOARD:
    case WRL2_COLLISION:
    case WRL2_COLORINTERPOLATOR:
    case WRL2_COORDINATEINTERPOLATOR:
    case WRL2_CYLINDERSENSOR:
    case WRL2_DIRECTIONALLIGHT:
    case WRL2_FOG:
    case WRL2_FONTSTYLE:
    case WRL2_IMAGETEXTURE:
    case WRL2_INDEXEDLINESET:
    case WRL2_INLINE:
    case WRL2_LOD:
    case WRL2_MOVIETEXTURE:
    case WRL2_NAVIGATIONINFO:
    case WRL2_NORMALINTERPOLATOR:
    case WRL2_ORIENTATIONINTERPOLATOR:
    case WRL2_PIXELTEXTURE:
    case WRL2_PLANESENSOR:
    case WRL2_POINTLIGHT:
    case WRL2_POINTSET:
    case WRL2_POSITIONINTERPOLATOR:
    case WRL2_PROXIMITYSENSOR:
    case WRL2_SCALARINTERPOLATOR:
    case WRL2_SCRIPT:
    case WRL2_SOUND:
    case WRL2_SPHERESENSOR:
    case WRL2_SPOTLIGHT:
    case WRL2_SWITCH:
    case WRL2_TEXT:
    case WRL2_TEXTURECOORDINATE:
    case WRL2_TEXTURETRANSFORM:
    case WRL2_TIMESENSOR:
    case WRL2_TOUCHSENSOR:
    case WRL2_VIEWPOINT:
    case WRL2_VISIBILITYSENSOR:
    case WRL2_WORLDINFO:
    case WRL2_INVALID:
    default:

        proc.GetFilePosData( line, column );

        if( !proc.DiscardNode() )
        {
            #ifdef DEBUG
            std::cerr << proc.GetError() << "\n";
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] could not discard node at line " << line;
            std::cerr << ", column " << column << "\n";
            #endif

            return false;
        }

        break;
    }

    return true;
}


bool WRL2BASE::readTransform( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( NULL != aNode )
        *aNode = NULL;

    WRL2TRANSFORM* np = new WRL2TRANSFORM( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( NULL != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
    // this function makes no sense in the base node
    #ifdef DEBUG
    std::cerr << proc.GetError() << "\n";
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] this method must never be invoked on a WR2BASE object\n";
    #endif

    return false;
}
