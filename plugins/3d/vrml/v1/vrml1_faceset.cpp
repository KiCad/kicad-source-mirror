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
    do {
        std::ostringstream ostr;
        ostr << " * [INFO] Destroying IndexedFaceSet with " << m_Children.size();
        ostr << " children, " << m_Refs.size() << " references and ";
        ostr << m_BackPointers.size() << " backpointers";
        wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    return;
}


bool WRL1FACESET::AddRefNode( WRL1NODE* aNode )
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


bool WRL1FACESET::AddChildNode( WRL1NODE* aNode )
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


bool WRL1FACESET::Read( WRLPROC& proc, WRL1BASE* aTopNode )
{
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
        // coordIndex[]
        // materialIndex[]

        proc.GetFilePosData( line, column );

        if( !glob.compare( "coordIndex" ) )
        {
            if( !proc.ReadMFInt( coordIndex ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid coordIndex at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError();
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "materialIndex" ) )
        {
            if( !proc.ReadMFInt( matIndex ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid materialIndex at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError();
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "normalIndex" ) )
        {
            if( !proc.ReadMFInt( normIndex ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid normalIndex at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError();
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else if( !glob.compare( "textureCoordIndex" ) )
        {
            if( !proc.ReadMFInt( texIndex ) )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                do {
                    std::ostringstream ostr;
                    ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                    ostr << " * [INFO] invalid textureCoordIndex at line " << line << ", column ";
                    ostr << column << "\n";
                    ostr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                    ostr << " * [INFO] message: '" << proc.GetError();
                    wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
                } while( 0 );
                #endif

                return false;
            }
        }
        else
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] bad IndexedFaceSet at line " << line << ", column ";
                ostr << column << "\n";
                ostr << " * [INFO] file: '" << proc.GetFileName() << "'";
                wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
            } while( 0 );
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
        wxLogTrace( MASK_VRML, " * [INFO] bad model: no parent node\n" );
        #endif

        return NULL;
    }
    else
    {
        if( NULL == sp )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            wxLogTrace( MASK_VRML, " * [INFO] bad model: no base data given\n" );
            #endif

            return NULL;
        }
    }

    m_current = *sp;

    if( NULL == m_current.coord || NULL == m_current.mat )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        if( NULL == m_current.coord )
        {
            wxLogTrace( MASK_VRML, " * [INFO] bad model: no vertex set\n" );
        }

        if( NULL == m_current.mat )
        {
            wxLogTrace( MASK_VRML, " * [INFO] bad model: no material set\n" );
        }
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
        do {
            std::ostringstream ostr;
            ostr << " * [INFO] bad model: coordsize, indexsize = " << coordsize;
            ostr << ", " << vsize;
            wxLogTrace( MASK_VRML, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return NULL;
    }

    // 1. create the vertex/normals/colors lists
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
            wxLogTrace( MASK_VRML, " * [INFO] bad model: per face indexed but no indices\n" );
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
    SGNODE* np = lShape.CalcShape( aParent, sgcolor, m_current.order, m_current.creaseLimit );

    return np;
}
