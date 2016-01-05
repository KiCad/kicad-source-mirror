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


SGNODE* WRL1FACESET::TranslateToSG( SGNODE* aParent, bool calcNormals )
{
    #ifdef NOGO
    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( NULL != aParent && ptype != S3D::SGTYPE_SHAPE )
    {
        #ifdef DEBUG_VRML1
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] IndexedFaceSet does not have a Shape parent (parent ID: ";
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

    size_t vsize = coordIndex.size();

    if( NULL == coord || vsize < 3 )
        return NULL;

    // create the index list and make sure we have >3 points
    size_t idx;
    int i1 = coordIndex[0];
    int i2 = coordIndex[1];
    int i3 = coordIndex[2];

    WRLVEC3F* pcoords;
    size_t coordsize;
    ((WRL1COORDS*) coord)->GetCoords( pcoords, coordsize );

    if( coordsize < 3 )
        return NULL;

    // check that all indices are valid
    for( idx = 0; idx < vsize; ++idx )
    {
        if( coordIndex[idx] < 0 )
            continue;

        if( coordIndex[idx] >= (int)coordsize )
            return NULL;
    }

    // if the indices are defective just give up
    if( i1 < 0 || i2 < 0 || i3 < 0
        || i1 == i2 || i1 == i3 || i2 == i3 )
        return NULL;

    std::vector< SGPOINT > lCPts;   // coordinate points for SG node
    std::vector< int > lCIdx;       // coordinate index list for SG node (must be triads)
    std::vector< SGVECTOR > lCNorm; // per-vertex normals
    std::vector< int > faces;       // tracks the number of polygons for the entire set
    int nfaces = 0;                 // number of triangles for each face in the list

    // assuming convex polygons, create triangles for the SG node
    for( idx = 3; idx < vsize; )
    {
        lCIdx.push_back( i1 );

        if( ccw )
        {
            lCIdx.push_back( i2 );
            lCIdx.push_back( i3 );
        }
        else
        {
            lCIdx.push_back( i3 );
            lCIdx.push_back( i2 );
        }

        ++nfaces;
        i2 = i3;
        i3 = coordIndex[idx++];

        while( ( i1 < 0 || i2 < 0 || i3 < 0 ) && ( idx < vsize ) )
        {
            if( i3 < 0 )
            {
                faces.push_back( nfaces );
                nfaces = 0;
            }

            i1 = i2;
            i2 = i3;
            i3 = coordIndex[idx++];

            // any invalid polygons shall void the entire faceset; this is a requirement
            // to ensure correct handling of the normals
            if( ( i1 < 0 && i2 < 0 ) || ( i1 < 0 && i3 < 0 ) || ( i2 < 0 && i3 < 0 ) )
                return NULL;
        }
    }

    if( lCIdx.empty() )
        return NULL;

    if( calcNormals || NULL == normal )
    {
        // create a vertex list for per-face per-vertex normals
        std::vector< int >::iterator sI = lCIdx.begin();
        std::vector< int >::iterator eI = lCIdx.end();

        while( sI != eI )
        {
            lCPts.push_back( SGPOINT( pcoords[*sI].x, pcoords[*sI].y, pcoords[*sI].z ) );
            ++sI;
        }

        for( size_t i = 0; i < lCPts.size(); i += 3 )
        {
            SGVECTOR sv = S3D::CalcTriNorm( lCPts[i], lCPts[i+1], lCPts[i+2] );
            lCNorm.push_back( sv );
            lCNorm.push_back( sv );
            lCNorm.push_back( sv );
        }

    }
    else
    {
        // XXX - TO IMPLEMENT
        return NULL;
        /*
            // use the vertex list as is
            if( normalPerVertex )
            {
                // normalPerVertex = TRUE
                // rules:
                //  + if normalIndex is not EMPTY, it is used to select a normal for each vertex
                //  + if normalIndex is EMPTY, the normal list is used in order per vertex

                if( normalIndex.empty() )
                {
                    for( size_t i = 0; i < coordsize; ++i )
                    {
                        lCPts.push_back( SGPOINT( pcoords[i].x, pcoords[i].y, pcoords[i].z ) );

                        // XXX - TO IMPLEMENT
                    }
                }
                else
                {
                    // XXX - TO IMPLEMENT: index the normals
                }
            }
            else
            {
                // normalPerVertex = FALSE
                // rules:
                //  + if normalIndex is not EMPTY, it is used to select a normal for each face
                //  + if normalIndex is EMPTY, the normal list is used in order per face

            }
        //*/
    }

    // XXX - TO IMPLEMENT: Per-vertex colors

    IFSG_FACESET fsNode( aParent );
    IFSG_COORDS cpNode( fsNode );
    cpNode.SetCoordsList( lCPts.size(), &lCPts[0] );
    IFSG_COORDINDEX ciNode( fsNode );

    if( calcNormals || NULL == normal )
    {
        for( int i = 0; i < (int)lCPts.size(); ++i )
            ciNode.AddIndex( i );
    }
    else
    {
        ciNode.SetIndices( lCIdx.size(), &lCIdx[0] );
    }

    IFSG_NORMALS nmNode( fsNode );
    nmNode.SetNormalList( lCNorm.size(), &lCNorm[0] );

    m_sgNode = fsNode.GetRawPtr();

    return m_sgNode;
    #endif

    return NULL;
}
