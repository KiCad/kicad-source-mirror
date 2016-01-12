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

    switch( m_current.matbind )
    {
    case BIND_PER_FACE:
    case BIND_PER_VERTEX:
        break;

    case BIND_PER_FACE_INDEXED:

        if( matIndex.empty() )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            std::cerr << " * [INFO] bad model: per face indexed but no indices\n";
            #endif

            return NULL;
        }

        break;

    case BIND_PER_VERTEX_INDEXED:

        if( matIndex.size() < 3 )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            std::cerr << " * [INFO] bad model: per vertex indexed but indexsize = ";
            std::cerr << matIndex.size() << "\n";
            #endif

            return NULL;
        }

        break;

    default:

        // use the first appearance definition
        sgcolor = m_current.mat->GetAppearance( 0 );
        break;
    }

    // create the index list and make sure we have >3 points
    size_t idx;
    int i1 = coordIndex[0];
    int i2 = coordIndex[1];
    int i3 = coordIndex[2];

    // check that all indices are valid
    for( idx = 0; idx < vsize; ++idx )
    {
        if( coordIndex[idx] < 0 )
            continue;

        if( coordIndex[idx] >= (int)coordsize )
        {
            #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
            std::cerr << " * [INFO] bad model: index out of bounds (index = ";
            std::cerr << coordIndex[idx] << ", npts = " << coordsize << ")\n";
            #endif

            m_current.mat->Reclaim( sgcolor );
            return NULL;
        }
    }

    // if the indices are defective just give up
    if( i1 < 0 || i2 < 0 || i3 < 0
        || i1 == i2 || i1 == i3 || i2 == i3 )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        std::cerr << " * [INFO] bad model: defective indices: " << i1;
        std::cerr << ", " << i2 << ", " << i3 << "\n";
        #endif

        m_current.mat->Reclaim( sgcolor );
        return NULL;
    }

    std::vector< SGPOINT > lCPts;   // coordinate points for SG node
    std::vector< int > lCIdx;       // coordinate index list for SG node (must be triads)
    std::vector< SGVECTOR > lCNorm; // per-vertex normals
    std::vector< int > faces;       // tracks the number of polygons for the entire set
    std::vector< SGCOLOR > lColors; // colors points (if any) for SG node
    int nfaces = 0;                 // number of triangles for each face in the list

    if( BIND_OVERALL == m_current.matbind || BIND_DEFAULT == m_current.matbind )
    {
        // no color list
        // assuming convex polygons, create triangles for the SG node
        for( idx = 3; idx <= vsize; )
        {
            switch( m_current.order )
            {
            case ORD_CCW:
                lCIdx.push_back( i1 );
                lCIdx.push_back( i2 );
                lCIdx.push_back( i3 );
                break;

            case ORD_CLOCKWISE:
                lCIdx.push_back( i1 );
                lCIdx.push_back( i3 );
                lCIdx.push_back( i2 );
                break;

            default:
                lCIdx.push_back( i1 );
                lCIdx.push_back( i2 );
                lCIdx.push_back( i3 );
                lCIdx.push_back( i1 );
                lCIdx.push_back( i3 );
                lCIdx.push_back( i2 );
                break;
            }

            ++nfaces;
            i2 = i3;

            if( idx == vsize )
                break;

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
                {
                    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                    std::cerr << " * [INFO] bad model: defective indices: " << i1;
                    std::cerr << ", " << i2 << ", " << i3 << "\n";
                    #endif

                    m_current.mat->Reclaim( sgcolor );
                    return NULL;
                }
            }

            if( i1 < 0 || i2 < 0 || i3 < 0 )
                break;
        }
    }
    else
    {
        // the entity requires a color list
        int cIndex;
        SGCOLOR pc1, pc2, pc3;

        switch( m_current.matbind )
        {
        case BIND_PER_VERTEX:
            cIndex = 3;
            m_current.mat->GetColor( &pc1, 0 );
            m_current.mat->GetColor( &pc2, 1 );
            m_current.mat->GetColor( &pc3, 2 );
            break;

        case BIND_PER_VERTEX_INDEXED:
            cIndex = 3;

            if( matIndex.size() < vsize )
            {
                #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] bad file; colorIndex.size() < coordIndex.size()\n";
                #endif

                return NULL;
            }

            m_current.mat->GetColor( &pc1, matIndex[0] );
            m_current.mat->GetColor( &pc2, matIndex[1] );
            m_current.mat->GetColor( &pc3, matIndex[2] );
            break;

        case BIND_PER_FACE:
            cIndex = 1;
            m_current.mat->GetColor( &pc1, 0 );
            pc2.SetColor( pc1 );
            pc3.SetColor( pc1 );
            break;

        default:
            // BIND_PER_FACE_INDEXED
            cIndex = 1;
            m_current.mat->GetColor( &pc1, matIndex[0] );
            pc2.SetColor( pc1 );
            pc3.SetColor( pc1 );
            break;
        }

        // assuming convex polygons, create triangles for the SG node
        int cMaxIdx = (int) matIndex.size();

        bool colorPerVertex = false;

        if( BIND_PER_VERTEX == m_current.matbind
            || BIND_PER_VERTEX_INDEXED == m_current.matbind )
            colorPerVertex = true;

        bool noidx = false;

        if( matIndex.empty() )
            noidx = true;

        for( idx = 3; idx <= vsize; )
        {
            switch( m_current.order )
            {
            case ORD_CCW:
                lCIdx.push_back( i1 );
                lCIdx.push_back( i2 );
                lCIdx.push_back( i3 );
                lColors.push_back( pc1 );
                lColors.push_back( pc2 );
                lColors.push_back( pc3 );
                break;

            case ORD_CLOCKWISE:
                lCIdx.push_back( i1 );
                lCIdx.push_back( i3 );
                lCIdx.push_back( i2 );
                lColors.push_back( pc1 );
                lColors.push_back( pc3 );
                lColors.push_back( pc2 );
                break;

            default:
                lCIdx.push_back( i1 );
                lCIdx.push_back( i2 );
                lCIdx.push_back( i3 );
                lCIdx.push_back( i1 );
                lCIdx.push_back( i3 );
                lCIdx.push_back( i2 );
                lColors.push_back( pc1 );
                lColors.push_back( pc2 );
                lColors.push_back( pc3 );
                lColors.push_back( pc1 );
                lColors.push_back( pc3 );
                lColors.push_back( pc2 );
                break;
            }

            ++nfaces;
            i2 = i3;

            if( idx == vsize )
                break;

            i3 = coordIndex[idx++];

            if( colorPerVertex && i1 >= 0 && i2 >= 0 && i3 >= 0 )
            {
                pc1.SetColor( pc2 );
                pc2.SetColor( pc3 );

                if( noidx || cIndex >= cMaxIdx )
                    m_current.mat->GetColor( &pc3, cIndex++ );
                else
                    m_current.mat->GetColor( &pc3, matIndex[cIndex++] );

            }

            while( ( i1 < 0 || i2 < 0 || i3 < 0 ) && ( idx < vsize ) )
            {
                if( i3 < 0 )
                {
                    faces.push_back( nfaces );
                    nfaces = 0;

                    if( !colorPerVertex )
                    {
                        if( noidx || cIndex >= cMaxIdx )
                            m_current.mat->GetColor( &pc1, cIndex++ );
                        else
                            m_current.mat->GetColor( &pc1, matIndex[cIndex++] );

                        pc2.SetColor( pc1 );
                        pc3.SetColor( pc1 );
                    }
                }

                i1 = i2;
                i2 = i3;
                i3 = coordIndex[idx++];

                if( colorPerVertex )
                {
                    pc1.SetColor( pc2 );
                    pc2.SetColor( pc3 );

                    if( noidx || cIndex >= cMaxIdx )
                        m_current.mat->GetColor( &pc3, cIndex++ );
                    else
                        m_current.mat->GetColor( &pc3, matIndex[cIndex++] );

                }

                // any invalid polygons shall void the entire faceset; this is a requirement
                // to ensure correct handling of the normals
                if( ( i1 < 0 && i2 < 0 ) || ( i1 < 0 && i3 < 0 ) || ( i2 < 0 && i3 < 0 ) )
                {
                    #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
                    std::cerr << " * [INFO] bad model: defective indices: " << i1;
                    std::cerr << ", " << i2 << ", " << i3 << "\n";
                    #endif

                    return NULL;
                }
            }

            if( i1 < 0 || i2 < 0 || i3 < 0 )
                break;
        }
    }

    if( lCIdx.empty() )
    {
        #if defined( DEBUG_VRML1 ) && ( DEBUG_VRML1 > 1 )
        std::cerr << " * [INFO] bad model: no points in final index list\n";
        #endif

        m_current.mat->Reclaim( sgcolor );
        return NULL;
    }

    // create a vertex list for per-face per-vertex normals
    do {
        std::vector< int >::iterator sI = lCIdx.begin();
        std::vector< int >::iterator eI = lCIdx.end();

        while( sI != eI )
        {
            glm::vec4 pt = glm::vec4( pcoords[*sI].x, pcoords[*sI].y, pcoords[*sI].z, 1.0 );
            pt = m_current.txmatrix * pt;
            lCPts.push_back( SGPOINT( pt.x, pt.y, pt.z ) );
            ++sI;
        }

        switch( m_current.order )
        {
        case ORD_CCW:

            for( size_t i = 0; i < lCPts.size(); i += 3 )
            {
                SGVECTOR sv = S3D::CalcTriNorm( lCPts[i], lCPts[i+1], lCPts[i+2] );
                lCNorm.push_back( sv );
                lCNorm.push_back( sv );
                lCNorm.push_back( sv );
            }
            break;

        case ORD_CLOCKWISE:

            for( size_t i = 0; i < lCPts.size(); i += 3 )
            {
                SGVECTOR sv = S3D::CalcTriNorm( lCPts[i], lCPts[i+2], lCPts[i+1] );
                lCNorm.push_back( sv );
                lCNorm.push_back( sv );
                lCNorm.push_back( sv );
            }
            break;

        default:

            for( size_t i = 0; i < lCPts.size(); i += 6 )
            {
                SGVECTOR sv = S3D::CalcTriNorm( lCPts[i], lCPts[i+1], lCPts[i+2] );
                lCNorm.push_back( sv );
                lCNorm.push_back( sv );
                lCNorm.push_back( sv );
                sv = S3D::CalcTriNorm( lCPts[i], lCPts[i+2], lCPts[i+1] );
                lCNorm.push_back( sv );
                lCNorm.push_back( sv );
                lCNorm.push_back( sv );
            }
            break;
        }

    } while( 0 );

    // create the hierarchy:
    // Shape
    //   + (option) Appearance
    //   + FaceSet
    IFSG_SHAPE shapeNode( aParent );

    if( sgcolor )
    {
        if( NULL == S3D::GetSGNodeParent( sgcolor ) )
            shapeNode.AddChildNode( sgcolor );
        else
            shapeNode.AddRefNode( sgcolor );
    }

    IFSG_FACESET fsNode( shapeNode );
    IFSG_COORDS cpNode( fsNode );
    cpNode.SetCoordsList( lCPts.size(), &lCPts[0] );
    IFSG_COORDINDEX ciNode( fsNode );

    for( int i = 0; i < (int)lCPts.size(); ++i )
        ciNode.AddIndex( i );

    IFSG_NORMALS nmNode( fsNode );
    nmNode.SetNormalList( lCNorm.size(), &lCNorm[0] );

    if( !lColors.empty() )
    {
        IFSG_COLORS nmColor( fsNode );
        nmColor.SetColorList( lColors.size(), &lColors[0] );
    }

    return fsNode.GetRawPtr();
}
