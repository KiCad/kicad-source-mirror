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

#include "vrml1_base.h"
#include "vrml1_faceset.h"
#include "vrml1_coords.h"
#include "vrml1_material.h"
#include "wrlfacet.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1FACESET::WRL1FACESET( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_INDEXEDFACESET;
}


WRL1FACESET::WRL1FACESET( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_Type = WRL1NODES::WRL1_INDEXEDFACESET;
    m_Parent = aParent;

    if( nullptr != m_Parent )
        m_Parent->AddChildNode( this );
}


WRL1FACESET::~WRL1FACESET()
{
    wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Destroying IndexedFaceSet with %zu children, "
                                      "%zu references, and %zu back pointers." ),
                m_Children.size(), m_Refs.size(), m_BackPointers.size() );
}


bool WRL1FACESET::AddRefNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddRefNode is not applicable." ) );
}


bool WRL1FACESET::AddChildNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node
    wxCHECK_MSG( false, false, wxT( "AddChildNode is not applicable." ) );
}


bool WRL1FACESET::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
    char tok = proc.Peek();

    if( proc.eof() )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( "%s:%s:%d\n"
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
            wxLogTrace( traceVrmlPlugin, wxT( "%s:%s:%d\n%s" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetError() );

            return false;
        }

        // expecting one of:
        // coordIndex[]
        // materialIndex[]

        if( !glob.compare( "coordIndex" ) )
        {
            if( !proc.ReadMFInt( coordIndex ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] invalid coordIndex %s.\n"
                                 " * [INFO] file: '%s'\n"
                                 "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "materialIndex" ) )
        {
            if( !proc.ReadMFInt( matIndex ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] invalid materialIndex %s.\n"
                                 " * [INFO] file: '%s'\n"
                                 "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "normalIndex" ) )
        {
            if( !proc.ReadMFInt( normIndex ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] invalid normalIndex %s\n"
                                 " * [INFO] file: '%s'\n"
                                 "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else if( !glob.compare( "textureCoordIndex" ) )
        {
            if( !proc.ReadMFInt( texIndex ) )
            {
                wxLogTrace( traceVrmlPlugin,
                            wxT( "%s:%s:%d\n"
                                 " * [INFO] invalid textureCoordIndex %s.\n"
                                 " * [INFO] file: '%s'\n"
                                 "%s" ),
                            __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                            proc.GetFileName(), proc.GetError() );

                return false;
            }
        }
        else
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( "%s:%s:%d\n"
                             " * [INFO] invalid IndexedFaceSet %s.\n"
                             " * [INFO] file: '%s'" ),
                        __FILE__, __FUNCTION__, __LINE__, proc.GetFilePosition(),
                        proc.GetFileName() );

            return false;
        }
    }   // while( true ) -- reading contents of IndexedFaceSet{}

    return true;
}


SGNODE* WRL1FACESET::TranslateToSG( SGNODE* aParent, WRL1STATUS* sp )
{
    // note: m_sgNode is unused because we cannot manage everything
    // with a single reused transform due to the fact that VRML1
    // may use a MatrixTransformation entity which is impossible to
    // decompose into Rotate,Scale,Transform via an analytic expression.
    if( !m_Parent )
    {
        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] bad model: no parent node." ) );

        return nullptr;
    }
    else
    {
        if( nullptr == sp )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] bad model: no base data given." ) );

            return nullptr;
        }
    }

    m_current = *sp;

    if( nullptr == m_current.coord )
    {
        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] bad model: no vertex set." ) );
        return nullptr;
    }

    if( nullptr == m_current.mat )
    {
        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] bad model: no material set." ) );
        return nullptr;
    }

    WRLVEC3F* pcoords;
    size_t coordsize;

    m_current.coord->GetCoords( pcoords, coordsize );
    size_t vsize = coordIndex.size();

    if( coordsize < 3 || vsize < 3 )
    {
        wxLogTrace( traceVrmlPlugin,
                    wxT( " * [INFO] bad model: coordsize = %zu, indexsize = %zu" ),
                    coordsize, vsize );

        return nullptr;
    }

    // 1. create the vertex/normals/colors lists
    SGNODE* sgcolor = nullptr;
    WRL1_BINDING mbind = m_current.matbind;
    size_t matSize = matIndex.size();

    switch( mbind )
    {
    case WRL1_BINDING::BIND_PER_FACE:
    case WRL1_BINDING::BIND_PER_VERTEX:
    case WRL1_BINDING::BIND_PER_VERTEX_INDEXED:
        break;

    case WRL1_BINDING::BIND_PER_FACE_INDEXED:

        if( matIndex.empty() )
        {
            wxLogTrace( traceVrmlPlugin,
                        wxT( " * [INFO] bad model: per face indexed but no indices" ) );

            // support bad models by temporarily switching bindings
            mbind = WRL1_BINDING::BIND_OVERALL;
            sgcolor = m_current.mat->GetAppearance( 0 );
        }

        break;

    default:

        // use the first appearance definition
        sgcolor = m_current.mat->GetAppearance( 0 );
        break;
    }

    // copy the data into FACET structures

    SHAPE   lShape;
    FACET*  fp = nullptr;
    size_t  iCoord;
    int     idx;        // coordinate index
    size_t  cidx = 0;   // color index
    SGCOLOR pc1;

    if( mbind == WRL1_BINDING::BIND_OVERALL || mbind == WRL1_BINDING::BIND_DEFAULT )
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
            WRLVEC3F vf;
            glm::vec4 pt = glm::vec4( pcoords[idx].x, pcoords[idx].y, pcoords[idx].z, 1.0 );
            pt = m_current.txmatrix * pt;
            vf.x = pt.x;
            vf.y = pt.y;
            vf.z = pt.z;

            fp->AddVertex( vf, idx );
        }
    }
    else
    {
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

                if( mbind == WRL1_BINDING::BIND_PER_FACE
                    || mbind == WRL1_BINDING::BIND_PER_FACE_INDEXED )
                    ++cidx;

                continue;
            }

            // if the coordinate is bad then skip it
            if( idx >= (int)coordsize )
                continue;

            if( nullptr == fp )
                fp = lShape.NewFacet();

            // push the vertex value and index
            WRLVEC3F vf;
            glm::vec4 pt = glm::vec4( pcoords[idx].x, pcoords[idx].y, pcoords[idx].z, 1.0 );
            pt = m_current.txmatrix * pt;
            vf.x = pt.x;
            vf.y = pt.y;
            vf.z = pt.z;

            fp->AddVertex( vf, idx );

            // push the color if appropriate
            switch( mbind )
            {
            case WRL1_BINDING::BIND_PER_FACE:

                if( !fp->HasColors() )
                {
                    m_current.mat->GetColor( &pc1, cidx );
                    fp->AddColor( pc1 );
                }

                break;

            case WRL1_BINDING::BIND_PER_VERTEX:
                m_current.mat->GetColor( &pc1, idx );
                fp->AddColor( pc1 );
                break;

            case WRL1_BINDING::BIND_PER_FACE_INDEXED:

                if( !fp->HasColors() )
                {
                    if( cidx >= matSize )
                        m_current.mat->GetColor( &pc1, matIndex.back() );
                    else
                        m_current.mat->GetColor( &pc1, matIndex[cidx] );

                    fp->AddColor( pc1 );
                }

                break;

            case WRL1_BINDING::BIND_PER_VERTEX_INDEXED:

                if( matIndex.empty() )
                {
                    m_current.mat->GetColor( &pc1, idx );
                }
                else
                {
                    if( iCoord >= matSize )
                        m_current.mat->GetColor( &pc1, matIndex.back() );
                    else
                        m_current.mat->GetColor( &pc1, matIndex[iCoord] );
                }

                fp->AddColor( pc1 );

                break;

            default:
                break;
            }
        }
    }

    // extract the final data set
    SGNODE* np = lShape.CalcShape( aParent, sgcolor, m_current.order, m_current.creaseLimit );

    return np;
}
