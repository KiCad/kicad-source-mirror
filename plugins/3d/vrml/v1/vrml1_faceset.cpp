/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
#include "vrml1_faceset.h"
#include "vrml1_coords.h"
#include "vrml1_material.h"
#include "wrlfacet.h"
#include "plugins/3dapi/ifsg_all.h"


WRL1FACESET::WRL1FACESET( NAMEREGISTER* aDictionary ) : WRL1NODE( aDictionary )
{
    m_Type = WRL1_INDEXEDFACESET;

    return;
}


WRL1FACESET::WRL1FACESET( NAMEREGISTER* aDictionary, WRL1NODE* aParent ) :
    WRL1NODE( aDictionary )
{
    m_Type = WRL1_INDEXEDFACESET;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL1FACESET::~WRL1FACESET()
{
    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 2 )
    std::cerr << " * [INFO] Destroying IndexedFaceSet with " << m_Children.size();
    std::cerr << " children, " << m_Refs.size() << " references and ";
    std::cerr << m_BackPointers.size() << " backpointers\n";
    #endif

    return;
}


bool WRL1FACESET::AddRefNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML1
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] AddRefNode is not applicable\n";
    #endif

    return false;
}


bool WRL1FACESET::AddChildNode( WRL1NODE* aNode )
{
    // this node may not own or reference any other node

    #ifdef DEBUG_VRML1
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [BUG] AddChildNode is not applicable\n";
    #endif

    return false;
}


bool WRL1FACESET::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
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

        if( !proc.ReadName( glob ) )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

            return false;
        }

        // expecting one of:
        // coordIndex[]
        // materialIndex[]

        proc.GetFilePosData( line, column );

        if( !glob.compare( "coordIndex" ) )
        {
            if( !proc.ReadMFInt( coordIndex ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid coordIndex at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif

                return false;
            }
        }
        else if( !glob.compare( "materialIndex" ) )
        {
            if( !proc.ReadMFInt( matIndex ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid materialIndex at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif

                return false;
            }
        }
        else if( !glob.compare( "normalIndex" ) )
        {
            if( !proc.ReadMFInt( normIndex ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid normalIndex at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif

                return false;
            }
        }
        else if( !glob.compare( "textureCoordIndex" ) )
        {
            if( !proc.ReadMFInt( texIndex ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid textureCoordIndex at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif

                return false;
            }
        }
        else
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad IndexedFaceSet at line " << line << ", column ";
            std::cerr << column << "\n";
            std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
            #endif

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
    // decompose into Rotate,Scale,Transform via an anlytic expression.
    if( !m_Parent )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        std::cerr << " * [INFO] bad model: no parent node\n";
        #endif

        return NULL;
    }
    else
    {
        if( NULL == sp )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            std::cerr << " * [INFO] bad model: no base data given\n";
            #endif

            return NULL;
        }
    }

    m_current = *sp;

    if( NULL == m_current.coord || NULL == m_current.mat )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        if( NULL == m_current.coord )
            std::cerr << " * [INFO] bad model: no vertex set\n";

        if( NULL == m_current.mat )
            std::cerr << " * [INFO] bad model: no material set\n";
        #endif

        return NULL;
    }

    WRLVEC3F* pcoords;
    size_t coordsize;

    m_current.coord->GetCoords( pcoords, coordsize );
    size_t vsize = coordIndex.size();

    if( coordsize < 3 || vsize < 3 )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        std::cerr << " * [INFO] bad model: coordsize, indexsize = " << coordsize;
        std::cerr << ", " << vsize << "\n";
        #endif

        return NULL;
    }

    // 1. create the vertex/normals/colors lists
    std::vector< SGPOINT > vlist;
    std::vector< SGVECTOR > nlist;
    std::vector< SGCOLOR > colorlist;
    SGNODE* sgcolor = NULL;
    WRL1_BINDING mbind = m_current.matbind;
    size_t matSize = matIndex.size();

    switch( mbind )
    {
    case BIND_PER_FACE:
    case BIND_PER_VERTEX:
    case BIND_PER_VERTEX_INDEXED:
        break;

    case BIND_PER_FACE_INDEXED:

        if( matIndex.empty() )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            std::cerr << " * [INFO] bad model: per face indexed but no indices\n";
            #endif

            // support bad models by temporarily switching bindings
            mbind = BIND_OVERALL;
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
    FACET*  fp = NULL;
    size_t  iCoord;
    int     idx;        // coordinate index
    size_t  cidx = 0;   // color index
    SGCOLOR pc1;

    if( mbind == BIND_OVERALL || mbind == BIND_DEFAULT )
    {
        // no per-vertex colors; we can save a few CPU cycles
        for( iCoord = 0; iCoord < vsize; ++iCoord )
        {
            idx = coordIndex[iCoord];

            if( idx < 0 )
            {
                if( NULL != fp )
                {
                    if( fp->HasMinPoints() )
                        fp = NULL;
                    else
                        fp->Init();
                }

                continue;
            }

            // if the coordinate is bad then skip it
            if( idx >= (int)coordsize )
                continue;

            if( NULL == fp )
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
                if( NULL != fp )
                {
                    if( fp->HasMinPoints() )
                        fp = NULL;
                    else
                        fp->Init();
                }

                if( mbind == BIND_PER_FACE || mbind == BIND_PER_FACE_INDEXED )
                    ++cidx;

                continue;
            }

            // if the coordinate is bad then skip it
            if( idx >= (int)coordsize )
                continue;

            if( NULL == fp )
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
            case BIND_PER_FACE:

                if( !fp->HasColors() )
                {
                    m_current.mat->GetColor( &pc1, cidx );
                    fp->AddColor( pc1 );
                }

                break;

            case BIND_PER_VERTEX:
                m_current.mat->GetColor( &pc1, idx );
                fp->AddColor( pc1 );
                ++cidx;
                break;

            case BIND_PER_FACE_INDEXED:
                if( !fp->HasColors() )
                {
                    if( cidx >= matSize )
                        m_current.mat->GetColor( &pc1, matIndex.back() );
                    else
                        m_current.mat->GetColor( &pc1, matIndex[cidx] );

                    fp->AddColor( pc1 );
                }

                break;

            case BIND_PER_VERTEX_INDEXED:

                if( matIndex.empty() )
                {
                    int ic = coordIndex[iCoord];

                    if( ic >= (int)matSize )
                        m_current.mat->GetColor( &pc1, matIndex.back() );
                    else
                        m_current.mat->GetColor( &pc1, matIndex[ic] );
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
    SGNODE* np = lShape.CalcShape( aParent, sgcolor, m_current.order, m_current.creaseAngle );

    return np;
}
