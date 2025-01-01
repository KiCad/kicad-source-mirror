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

#include "vrml2_base.h"
#include "vrml2_faceset.h"
#include "vrml2_coords.h"
#include "vrml2_color.h"
#include "wrlfacet.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2FACESET::WRL2FACESET() : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2NODES::WRL2_INDEXEDFACESET;
}


WRL2FACESET::WRL2FACESET( WRL2NODE* aParent ) : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2NODES::WRL2_INDEXEDFACESET;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL2FACESET::~WRL2FACESET()
{
    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Destroying IndexedFaceSet node with %zu children, %zu"
                     "references, and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


void WRL2FACESET::setDefaults( void )
{
    color = nullptr;
    coord = nullptr;
    normal = nullptr;
    texCoord = nullptr;

    ccw = true;
    colorPerVertex = true;
    convex = true;
    normalPerVertex = true;
    solid = true;

    creaseAngle = 0.733f;   // approx 42 degrees; this is larger than VRML spec.
    creaseLimit = 0.74317f; // cos( 0.733 )
}


bool WRL2FACESET::checkNodeType( WRL2NODES aType )
{
    // nodes must be one of:
    // Color
    // Coordinate
    // Normal
    // TextureCoordinate

    switch( aType )
    {
    case WRL2NODES::WRL2_COLOR:
    case WRL2NODES::WRL2_COORDINATE:
    case WRL2NODES::WRL2_NORMAL:
    case WRL2NODES::WRL2_TEXTURECOORDINATE:
        break;

    default:
        return false;
        break;
    }

    return true;
}


bool WRL2FACESET::isDangling( void )
{
    // this node is dangling unless it has a parent of type WRL2_SHAPE

    if( nullptr == m_Parent || m_Parent->GetNodeType() != WRL2NODES::WRL2_SHAPE )
        return true;

    return false;
}


bool WRL2FACESET::AddRefNode( WRL2NODE* aNode )
{
    wxCHECK_MSG( aNode, false, wxT( "Invalid node." ) );

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; unexpected child node '%s'." ),
                    __FILE__, __FUNCTION__, __LINE__, aNode->GetNodeTypeName( type ) );

        return false;
    }

    if( WRL2NODES::WRL2_COLOR == type )
    {
        if( nullptr != color )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple color nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        color = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2NODES::WRL2_COORDINATE == type )
    {
        if( nullptr != coord )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple coord nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        coord = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2NODES::WRL2_NORMAL == type )
    {
        if( nullptr != normal )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple normal nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        normal = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    wxCHECK_MSG( WRL2NODES::WRL2_TEXTURECOORDINATE == type, false,
                 wxT( "Unexpected code branch." ) );

    if( nullptr != texCoord )
    {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple texCoord nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    texCoord = aNode;
    return WRL2NODE::AddRefNode( aNode );
}


bool WRL2FACESET::AddChildNode( WRL2NODE* aNode )
{
    wxCHECK_MSG( aNode, false, wxT( "Invalid node." ) );

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; unexpected child node '%s'." ),
                    __FILE__, __FUNCTION__, __LINE__, aNode->GetNodeTypeName( type ) );

        return false;
    }

    if( WRL2NODES::WRL2_COLOR == type )
    {
        if( nullptr != color )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple color nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        color = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2NODES::WRL2_COORDINATE == type )
    {
        if( nullptr != coord )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple coord nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        coord = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2NODES::WRL2_NORMAL == type )
    {
        if( nullptr != normal )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] bad file format; multiple normal nodes." ),
                        __FILE__, __FUNCTION__, __LINE__ );

            return false;
        }

        normal = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    wxCHECK_MSG( WRL2NODES::WRL2_TEXTURECOORDINATE == type, false,
                 wxT( "Unexpected code branch." ) );

    if( nullptr != texCoord )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
                         " * [INFO] bad file format; multiple texCoord nodes." ),
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    texCoord = aNode;
    return WRL2NODE::AddChildNode( aNode );
}



bool WRL2FACESET::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
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
        // [node]
        // color
        // coord
        // normal
        // texCoord
        // [bool]
        // ccw
        // colorPerVertex
        // convex
        // normalPerVertex
        // solid
        // [ vector<int> ]
        // colorIndex
        // coordIndex
        // normalIndex;
        // [float]
        // creaseAngle

        if( !glob.compare( "ccw" ) )
        {
            if( !proc.ReadSFBool( ccw ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                   " * [INFO] invalid ccw %s\n"
                                                   " * [INFO] file: '%s'\n"
                                                   "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "colorPerVertex" ) )
        {
            if( !proc.ReadSFBool( colorPerVertex ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                   " * [INFO] invalid colorPerVertex %s\n"
                                                   " * [INFO] file: '%s'\n"
                                                   "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "convex" ) )
        {
            if( !proc.ReadSFBool( convex ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                   " * [INFO] invalid convex %s\n"
                                                   " * [INFO] file: '%s'\n"
                                                   "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "normalPerVertex" ) )
        {
            if( !proc.ReadSFBool( normalPerVertex ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                   " * [INFO] invalid normalPerVertex %s\n"
                                                   " * [INFO] file: '%s'\n"
                                                   "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "solid" ) )
        {
            if( !proc.ReadSFBool( solid ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                   " * [INFO] invalid solid %s\n"
                                                   " * [INFO] file: '%s'\n"
                                                   "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "creaseAngle" ) )
        {
            if( !proc.ReadSFFloat( creaseAngle ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                   " * [INFO] invalid creaseAngle %s\n"
                                                   " * [INFO] file: '%s'\n"
                                                   "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }

            if( creaseAngle < 0.0 )
                creaseAngle = 0.0f;
            else if( creaseAngle > M_PI_2 )
                creaseAngle = static_cast<float>( M_PI_2 );

            creaseLimit = cosf( creaseAngle );
        }
        else if( !glob.compare( "colorIndex" ) )
        {
            if( !proc.ReadMFInt( colorIndex ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                   " * [INFO] invalid colorIndex %s\n"
                                                   " * [INFO] file: '%s'\n"
                                                   "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "coordIndex" ) )
        {
            if( !proc.ReadMFInt( coordIndex ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                   " * [INFO] invalid coordIndex %s\n"
                                                   " * [INFO] file: '%s'\n"
                                                   "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "normalIndex" ) )
        {
            if( !proc.ReadMFInt( normalIndex ) )
            {
                wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n"
                                                   " * [INFO] invalid normalIndex %s\n"
                                                   " * [INFO] file: '%s'\n"
                                                   "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "color" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] could not read color node information." ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }
        }
        else if( !glob.compare( "coord" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] could not read coord node information." ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }
        }
        else if( !glob.compare( "normal" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] could not read normal node information." ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }
        }
        else if( !glob.compare( "texCoord" ) )
        {
            if( !aTopNode->ReadNode( proc, this, nullptr ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] could not read texCoord node information." ),
                            __FILE__, __FUNCTION__, __LINE__ );

                return false;
            }
        }
        else
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] invalid IndexedFaceSet %s (no closing brace)\n"
                             " * [INFO] file: '%s'\n" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }
    }   // while( true ) -- reading contents of IndexedFaceSet{}

    return true;
}


SGNODE* WRL2FACESET::TranslateToSG( SGNODE* aParent )
{
    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    wxCHECK_MSG( aParent && ( ptype == S3D::SGTYPE_SHAPE ), nullptr,
                 wxString::Format( wxT( "IndexedFaceSet does not have a Shape parent (parent "
                                        "ID: %d)." ), ptype ) );

    wxLogTrace( traceVrmlPlugin,
                wxT( " * [INFO] Translating IndexedFaceSet with %zu children, %zu references, "
                     "%zu back pointers, and %zu coord indices." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size(), coordIndex.size() );

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

    size_t vsize = coordIndex.size();

    if( nullptr == coord || vsize < 3 )
        return nullptr;

    WRLVEC3F* pcoords;
    size_t coordsize;
    ((WRL2COORDS*) coord)->GetCoords( pcoords, coordsize );

    if( coordsize < 3 )
        return nullptr;

    // check that all indices are valid
    for( size_t idx = 0; idx < vsize; ++idx )
    {
        if( coordIndex[idx] < 0 )
            continue;

        if( coordIndex[idx] >= (int)coordsize )
            return nullptr;
    }

    SHAPE   lShape;
    FACET*  fp = nullptr;
    size_t  iCoord;
    int     idx;        // coordinate index
    size_t  cidx = 0;   // color index
    SGCOLOR pc1;

    if( nullptr == color )
    {
        // no per-vertex colors; we can save a few CPU cycles
        for( iCoord = 0; iCoord < vsize; ++iCoord )
        {
            idx = coordIndex[iCoord];

            if( idx < 0 )
            {
                if( nullptr != fp )
                {
                    if( fp->HasMinPoints() )
                        fp = nullptr;
                    else
                        fp->Init();
                }

                continue;
            }

            // if the coordinate is bad then skip it
            if( idx >= (int)coordsize )
                continue;

            if( nullptr == fp )
                fp = lShape.NewFacet();

            // push the vertex value and index
            fp->AddVertex( pcoords[idx], idx );
        }
    }
    else
    {
        WRL2COLOR* cn = (WRL2COLOR*) color;
        WRLVEC3F tc;

        for( iCoord = 0; iCoord < vsize; ++iCoord )
        {
            idx = coordIndex[iCoord];

            if( idx < 0 )
            {
                if( nullptr != fp )
                {
                    if( fp->HasMinPoints() )
                        fp = nullptr;
                    else
                        fp->Init();
                }

                if( !colorPerVertex )
                    ++cidx;

                continue;
            }

            // if the coordinate is bad then skip it
            if( idx >= (int)coordsize )
                continue;

            if( nullptr == fp )
                fp = lShape.NewFacet();

            // push the vertex value and index
            fp->AddVertex( pcoords[idx], idx );

            // push the color if appropriate
            if( !colorPerVertex )
            {
                if( colorIndex.empty() )
                {
                    cn->GetColor( cidx, tc.x, tc.y, tc.z );
                    pc1.SetColor( tc.x, tc.y, tc.z );
                    fp->AddColor( pc1 );
                }
                else
                {
                    if( cidx < colorIndex.size() )
                        cn->GetColor( colorIndex[cidx], tc.x, tc.y, tc.z );
                    else
                        cn->GetColor( colorIndex.back(), tc.x, tc.y, tc.z );

                    pc1.SetColor( tc.x, tc.y, tc.z );
                    fp->AddColor( pc1 );
                }
            }
            else
            {
                if( colorIndex.empty() )
                {
                    cn->GetColor( idx, tc.x, tc.y, tc.z );
                    pc1.SetColor( tc.x, tc.y, tc.z );
                    fp->AddColor( pc1 );
                }
                else
                {
                    if( iCoord < colorIndex.size() )
                        cn->GetColor( colorIndex[iCoord], tc.x, tc.y, tc.z );
                    else
                        cn->GetColor( colorIndex.back(), tc.x, tc.y, tc.z );

                    pc1.SetColor( tc.x, tc.y, tc.z );
                    fp->AddColor( pc1 );
                }
            }
        }
    }

    SGNODE* np = nullptr;

    if( ccw )
        np = lShape.CalcShape( aParent, nullptr, WRL1_ORDER::ORD_CCW, creaseLimit, true );
    else
        np = lShape.CalcShape( aParent, nullptr, WRL1_ORDER::ORD_CLOCKWISE, creaseLimit, true );

    return np;
}


void WRL2FACESET::unlinkChildNode( const WRL2NODE* aNode )
{
    if( nullptr == aNode )
        return;

    if( aNode->GetParent() == this )
    {
        if( aNode == color )
            color = nullptr;
        else if( aNode == coord )
            coord = nullptr;
        else if( aNode == normal )
            normal = nullptr;
        else if( aNode == texCoord )
            texCoord = nullptr;
    }

    WRL2NODE::unlinkChildNode( aNode );
}


void WRL2FACESET::unlinkRefNode( const WRL2NODE* aNode )
{
    if( nullptr == aNode )
        return;

    if( aNode->GetParent() != this )
    {
        if( aNode == color )
            color = nullptr;
        else if( aNode == coord )
            coord = nullptr;
        else if( aNode == normal )
            normal = nullptr;
        else if( aNode == texCoord )
            texCoord = nullptr;
    }

    WRL2NODE::unlinkRefNode( aNode );
}


bool WRL2FACESET::HasColors( void )
{
    if( nullptr == color )
        return false;

    return ( (WRL2COLOR*) color )->HasColors();
}
