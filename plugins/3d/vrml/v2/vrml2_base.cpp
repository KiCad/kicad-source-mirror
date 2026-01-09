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
#include <utility>
#include <wx/string.h>
#include <wx/log.h>

#include <wx_filename.h>

#include "vrml2_base.h"
#include "vrml2_transform.h"
#include "vrml2_shape.h"
#include "vrml2_appearance.h"
#include "vrml2_material.h"
#include "vrml2_faceset.h"
#include "vrml2_lineset.h"
#include "vrml2_pointset.h"
#include "vrml2_coords.h"
#include "vrml2_norms.h"
#include "vrml2_color.h"
#include "vrml2_box.h"
#include "vrml2_switch.h"
#include "vrml2_inline.h"
#include "plugins/3dapi/ifsg_all.h"


SCENEGRAPH* LoadVRML( const wxString& aFileName, bool useInline );


WRL2BASE::WRL2BASE() : WRL2NODE()
{
    m_useInline = false;
    m_applyUnitConversion = true;  // default to legacy behavior
    m_Type = WRL2NODES::WRL2_BASE;
}


WRL2BASE::~WRL2BASE()
{
    std::map< std::string, SGNODE* >::iterator iS = m_inlineModels.begin();
    std::map< std::string, SGNODE* >::iterator eS = m_inlineModels.end();

    while( iS != eS )
    {
        SGNODE* np = iS->second;

        // destroy any orphaned Inline{} node data
        if( np && nullptr == S3D::GetSGNodeParent( np ) )
            S3D::DestroyNode( np );

        ++iS;
    }

    m_inlineModels.clear();
}


bool WRL2BASE::SetParent( WRL2NODE* aParent, bool /* doUnlink */ )
{
    wxCHECK_MSG( false, false, wxT( "Attempt to set parent on WRL2BASE node." ) );
}


void WRL2BASE::SetEnableInline( bool enable )
{
    m_useInline = enable;
}


bool WRL2BASE::GetApplyUnitConversion( void ) const
{
    return m_applyUnitConversion;
}


void WRL2BASE::SetApplyUnitConversion( bool apply )
{
    m_applyUnitConversion = apply;
}


bool WRL2BASE::GetEnableInline( void )
{
    return  m_useInline;
}


SGNODE* WRL2BASE::GetInlineData( const std::string& aName )
{
    if( aName.empty() )
        return nullptr;

    std::map< std::string, SGNODE* >::iterator dp = m_inlineModels.find( aName );

    if( dp != m_inlineModels.end() )
        return dp->second;

    wxString tname;

    if( aName.compare( 0, 7, "file://" ) == 0 )
    {
        if( aName.length() <= 7 )
            return nullptr;

        tname = wxString::FromUTF8Unchecked( aName.substr( 7 ).c_str() );
    }
    else
    {
        tname = wxString::FromUTF8Unchecked( aName.c_str() );
    }

    wxFileName fn;
    fn.Assign( tname );

    if( fn.IsRelative() && !m_dir.empty() )
    {
        wxString fname = wxString::FromUTF8Unchecked( m_dir.c_str() );
        fname.append( tname );
        fn.Assign( fname );
    }

    if( !fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS ) )
    {
        m_inlineModels.emplace( aName, nullptr );
        return nullptr;
    }

    SCENEGRAPH* sp = LoadVRML( fn.GetFullPath(), false );

    if( nullptr == sp )
    {
        m_inlineModels.emplace( aName, nullptr );
        return nullptr;
    }

    m_inlineModels.emplace( aName, (SGNODE*) sp );

    return (SGNODE*)sp;
}


std::string WRL2BASE::GetName( void )
{
    wxCHECK_MSG( false, std::string( "" ), wxT( "Attempt to extract name from base node." ) );
}


bool WRL2BASE::SetName( const std::string& aName )
{
    wxCHECK_MSG( false, false, wxT( "Attempt to set name of base node." ) );
}


bool WRL2BASE::Read( WRLPROC& proc )
{
    wxCHECK_MSG( proc.GetVRMLType() == WRLVERSION::VRML_V2, false,
                 wxT( "No open file or file is not a VRML2 file." ) );

    WRL2NODE* node = nullptr;
    m_dir = proc.GetParentDir();

    while( ReadNode( proc, this, &node ) && !proc.eof() );

    if( proc.eof() )
        return true;

    return false;
}


bool WRL2BASE::isDangling( void )
{
    // the base node is never dangling
    return false;
}


bool WRL2BASE::implementUse( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    wxCHECK_MSG( aParent, false, wxT( "Invalid parent." ) );

    std::string glob;

    if( !proc.ReadName( glob ) )
    {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

        return false;
    }

    WRL2NODE* ref = aParent->FindNode( glob, nullptr );

    // return 'true' - the file may be defective but it may still be somewhat OK
    if( nullptr == ref )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          " * [INFO] node '%s' not found." ),
                    __FILE__, __FUNCTION__, __LINE__, glob );

        return true;
    }

    if( !aParent->AddRefNode( ref ) )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] failed to add node '%s' (%d) to parent of type %d" ),
                    __FILE__, __FUNCTION__, __LINE__, glob, ref->GetNodeType(),
                    aParent->GetNodeType() );

        return false;
    }

    if( nullptr != aNode )
        *aNode = ref;

    return true;
}


bool WRL2BASE::implementDef( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    wxCHECK_MSG( aParent, false, wxT( "Invalid parent." ) );

    std::string glob;
    WRL2NODE* lnode = nullptr;

    if( !proc.ReadName( glob ) )
    {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

        return false;
    }

    if( ReadNode( proc, aParent, &lnode ) )
    {
        if( nullptr != aNode )
            *aNode = lnode;

        if( lnode && !lnode->SetName( glob ) )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad formatting (invalid name) %s." ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition() );

            return false;
        }

        return true;
    }

    return false;
}


bool WRL2BASE::ReadNode( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    // This function reads a node and stores a pointer to it in aNode.
    // A value 'true' is returned if a node is successfully read or,
    // if the node is not supported, successfully discarded. Callers
    // must always check the value of aNode when the function returns
    // 'true' since it will be NULL if the node type is not supported.

    if( nullptr != aNode )
        *aNode = nullptr;

    wxCHECK_MSG( aParent, false, wxT( "Invalid parent." ) );

    std::string glob;
    WRL2NODES ntype;

    if( !proc.ReadName( glob ) )
    {
        wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                          "%s" ),
                    __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

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
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

            return false;
        }

        return true;
    }

    if( !glob.compare( "DEF" ) )
    {
        if( !implementDef( proc, aParent, aNode ) )
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

            return false;
        }

        return true;
    }

    // pattern to skip:  PROTO name list
    if( !glob.compare( "PROTO" ) )
    {
        if( !proc.ReadName( glob ) || !proc.DiscardList() )
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

            return false;
        }

        return true;
    }

    // pattern to skip:  EXTERNPROTO name1 name2 list
    if( !glob.compare( "EXTERNPROTO" ) )
    {
        if( !proc.ReadName( glob ) || !proc.ReadName( glob ) || !proc.DiscardList() )
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

            return false;
        }

        return true;
    }

    // pattern to skip:  ROUTE glob1 glob2 glob3
    if( !glob.compare( "ROUTE" ) )
    {
        if( !proc.ReadGlob( glob ) || !proc.ReadGlob( glob ) || !proc.ReadGlob( glob ) )
        {
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                              "%s" ),
                        __FILE__, __FUNCTION__, __LINE__ , proc.GetError() );

            return false;
        }

        return true;
    }

    ntype = getNodeTypeID( glob );

    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Processing node '%s' ID: %d" ), glob, ntype );

    switch( ntype )
    {
        //
        // items to be implemented:
        //
    case WRL2NODES::WRL2_APPEARANCE:

        if( !readAppearance( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_BOX:

        if( !readBox( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_COLOR:

        if( !readColor( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_CONE:
        // XXX - IMPLEMENT
        if( !proc.DiscardNode() )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] failed to discard %s node %s." ),
                        glob, proc.GetFilePosition() );

            return false;
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] discarded %s node %s." ),
                        glob, proc.GetFilePosition() );
        }

        break;

    case WRL2NODES::WRL2_COORDINATE:

        if( !readCoords( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_CYLINDER:
        // XXX - IMPLEMENT
        if( !proc.DiscardNode() )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] failed to discard %s node %s." ),
                        glob, proc.GetFilePosition() );

            return false;
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] discarded %s node %s." ),
                        glob, proc.GetFilePosition() );
        }

        break;

    case WRL2NODES::WRL2_ELEVATIONGRID:
        // XXX - IMPLEMENT
        if( !proc.DiscardNode() )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] failed to discard %s node %s." ),
                        glob, proc.GetFilePosition() );

            return false;
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] discarded %s node %s." ),
                        glob, proc.GetFilePosition() );
        }

        break;

    case WRL2NODES::WRL2_EXTRUSION:
        // XXX - IMPLEMENT
        if( !proc.DiscardNode() )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] failed to discard %s node %s." ),
                        glob, proc.GetFilePosition() );

            return false;
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] discarded %s node %s." ),
                        glob, proc.GetFilePosition() );
        }

        break;

    case WRL2NODES::WRL2_INDEXEDFACESET:

        if( !readFaceSet( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_INDEXEDLINESET:

        if( !readLineSet( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_POINTSET:

        if( !readPointSet( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_MATERIAL:

        if( !readMaterial( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_NORMAL:

        if( !readNorms( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_SHAPE:

        if( !readShape( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_SPHERE:
        // XXX - IMPLEMENT
        if( !proc.DiscardNode() )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] failed to discard %s node %s." ),
                        glob, proc.GetFilePosition() );

            return false;
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] discarded %s node %s." ),
                        glob, proc.GetFilePosition() );
        }

        break;

    case WRL2NODES::WRL2_SWITCH:

        if( !readSwitch( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_TRANSFORM:
    case WRL2NODES::WRL2_GROUP:

        if( !readTransform( proc, aParent, aNode ) )
            return false;

        break;

    case WRL2NODES::WRL2_INLINE:

        if( !readInline( proc, aParent, aNode ) )
            return false;

        break;

    //
    // items not implemented or for optional future implementation:
    //
    case WRL2NODES::WRL2_ANCHOR:
    case WRL2NODES::WRL2_AUDIOCLIP:
    case WRL2NODES::WRL2_BACKGROUND:
    case WRL2NODES::WRL2_BILLBOARD:
    case WRL2NODES::WRL2_COLLISION:
    case WRL2NODES::WRL2_COLORINTERPOLATOR:
    case WRL2NODES::WRL2_COORDINATEINTERPOLATOR:
    case WRL2NODES::WRL2_CYLINDERSENSOR:
    case WRL2NODES::WRL2_DIRECTIONALLIGHT:
    case WRL2NODES::WRL2_FOG:
    case WRL2NODES::WRL2_FONTSTYLE:
    case WRL2NODES::WRL2_IMAGETEXTURE:
    case WRL2NODES::WRL2_LOD:
    case WRL2NODES::WRL2_MOVIETEXTURE:
    case WRL2NODES::WRL2_NAVIGATIONINFO:
    case WRL2NODES::WRL2_NORMALINTERPOLATOR:
    case WRL2NODES::WRL2_ORIENTATIONINTERPOLATOR:
    case WRL2NODES::WRL2_PIXELTEXTURE:
    case WRL2NODES::WRL2_PLANESENSOR:
    case WRL2NODES::WRL2_POINTLIGHT:
    case WRL2NODES::WRL2_POSITIONINTERPOLATOR:
    case WRL2NODES::WRL2_PROXIMITYSENSOR:
    case WRL2NODES::WRL2_SCALARINTERPOLATOR:
    case WRL2NODES::WRL2_SCRIPT:
    case WRL2NODES::WRL2_SOUND:
    case WRL2NODES::WRL2_SPHERESENSOR:
    case WRL2NODES::WRL2_SPOTLIGHT:
    case WRL2NODES::WRL2_TEXT:
    case WRL2NODES::WRL2_TEXTURECOORDINATE:
    case WRL2NODES::WRL2_TEXTURETRANSFORM:
    case WRL2NODES::WRL2_TIMESENSOR:
    case WRL2NODES::WRL2_TOUCHSENSOR:
    case WRL2NODES::WRL2_VIEWPOINT:
    case WRL2NODES::WRL2_VISIBILITYSENSOR:
    case WRL2NODES::WRL2_WORLDINFO:
    case WRL2NODES::WRL2_INVALID:
    default:

        if( !proc.DiscardNode() )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] failed to discard %s node %s." ),
                        glob, proc.GetFilePosition() );

            return false;
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] discarded %s node %s." ),
                        glob, proc.GetFilePosition() );
        }

        break;
    }

    return true;
}


bool WRL2BASE::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
    wxCHECK_MSG( false, false, wxT( "This method must never be invoked on a WRL2BASE object." ) );
}


bool WRL2BASE::readTransform( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2TRANSFORM* np = new WRL2TRANSFORM( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readShape( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2SHAPE* np = new WRL2SHAPE( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readAppearance( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2APPEARANCE* np = new WRL2APPEARANCE( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readMaterial( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2MATERIAL* np = new WRL2MATERIAL( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readFaceSet( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2FACESET* np = new WRL2FACESET( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readLineSet( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2LINESET* np = new WRL2LINESET( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readPointSet( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2POINTSET* np = new WRL2POINTSET( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readCoords( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2COORDS* np = new WRL2COORDS( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readNorms( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2NORMS* np = new WRL2NORMS( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readColor( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2COLOR* np = new WRL2COLOR( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readBox( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2BOX* np = new WRL2BOX( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readSwitch( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    WRL2SWITCH* np = new WRL2SWITCH( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


bool WRL2BASE::readInline( WRLPROC& proc, WRL2NODE* aParent, WRL2NODE** aNode )
{
    if( nullptr != aNode )
        *aNode = nullptr;

    if( !m_useInline )
    {
        if( !proc.DiscardNode() )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] failed to discard in line node %s." ),
                        proc.GetFilePosition() );

            return false;
        }

        return true;
    }

    WRL2INLINE* np = new WRL2INLINE( aParent );

    if( !np->Read( proc, this ) )
    {
        delete np;
        return false;
    }

    if( nullptr != aNode )
        *aNode = (WRL2NODE*) np;

    return true;
}


SGNODE* WRL2BASE::TranslateToSG( SGNODE* aParent )
{
    if( m_Children.empty() )
        return nullptr;

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

    IFSG_TRANSFORM topNode( aParent );

    std::list< WRL2NODE* >::iterator sC = m_Children.begin();
    std::list< WRL2NODE* >::iterator eC = m_Children.end();
    WRL2NODES type;

    // Include only Shape and Transform nodes in the top node
    bool test = false;  // set to true if there are any subnodes for display

    while( sC != eC )
    {
        type = (*sC)->GetNodeType();

        switch( type )
        {
        case WRL2NODES::WRL2_SHAPE:
            // wrap the shape in a transform
            do
            {
                IFSG_TRANSFORM wrapper( topNode.GetRawPtr() );
                SGNODE* pshape = (*sC)->TranslateToSG( wrapper.GetRawPtr() );

                if( nullptr != pshape )
                    test = true;
                else
                    wrapper.Destroy();

            } while( 0 );

            break;

        case WRL2NODES::WRL2_TRANSFORM:
        case WRL2NODES::WRL2_SWITCH:
        case WRL2NODES::WRL2_INLINE:

            if( nullptr != (*sC)->TranslateToSG( topNode.GetRawPtr() ) )
                test = true;

            break;

        default:
            break;
        }

        ++ sC;
    }

    if( false == test )
    {
        topNode.Destroy();
        return nullptr;
    }

    m_sgNode = topNode.GetRawPtr();

    return m_sgNode;
}
