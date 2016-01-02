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

#include "vrml2_base.h"
#include "vrml2_faceset.h"
#include "vrml2_coords.h"
#include "plugins/3dapi/ifsg_all.h"


WRL2FACESET::WRL2FACESET() : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2_INDEXEDFACESET;

    return;
}


WRL2FACESET::WRL2FACESET( WRL2NODE* aParent ) : WRL2NODE()
{
    setDefaults();
    m_Type = WRL2_INDEXEDFACESET;
    m_Parent = aParent;

    if( NULL != m_Parent )
        m_Parent->AddChildNode( this );

    return;
}


WRL2FACESET::~WRL2FACESET()
{
    #ifdef DEBUG
    std::cerr << " * [INFO] Destroying IndexedFaceSet with " << m_Children.size();
    std::cerr << " children, " << m_Refs.size() << " references and ";
    std::cerr << m_BackPointers.size() << " backpointers\n";
    #endif
    return;
}


void WRL2FACESET::setDefaults( void )
{
    color = NULL;
    coord = NULL;
    normal = NULL;
    texCoord = NULL;

    ccw = true;
    colorPerVertex = true;
    convex = true;
    normalPerVertex = true;
    solid = true;

    creaseAngle = 0.0;
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
    case WRL2_COLOR:
    case WRL2_COORDINATE:
    case WRL2_NORMAL:
    case WRL2_TEXTURECOORDINATE:
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

    if( NULL == m_Parent || m_Parent->GetNodeType() != WRL2_SHAPE )
        return true;

    return false;
}


bool WRL2FACESET::AddRefNode( WRL2NODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL passed for aNode\n";
        #endif

        return false;
    }

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected child node '";
        std::cerr << aNode->GetNodeTypeName( type ) << "'\n";
        #endif

        return false;
    }

    if( WRL2_COLOR == type )
    {
        if( NULL != color )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple color nodes\n";
            #endif
            return false;
        }

        color = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2_COORDINATE == type )
    {
        if( NULL != coord )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple coordinate nodes\n";
            #endif
            return false;
        }

        coord = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2_NORMAL == type )
    {
        if( NULL != normal )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple normal nodes\n";
            #endif
            return false;
        }

        normal = aNode;
        return WRL2NODE::AddRefNode( aNode );
    }

    if( WRL2_TEXTURECOORDINATE != type )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] unexpected code branch\n";
        #endif
        return false;
    }

    if( NULL != texCoord )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple texCoord nodes\n";
        #endif
        return false;
    }

    texCoord = aNode;
    return WRL2NODE::AddRefNode( aNode );
}


bool WRL2FACESET::AddChildNode( WRL2NODE* aNode )
{
    if( NULL == aNode )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL passed for aNode\n";
        #endif

        return false;
    }

    WRL2NODES type = aNode->GetNodeType();

    if( !checkNodeType( type ) )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected child node '";
        std::cerr << aNode->GetNodeTypeName( type ) << "'\n";
        #endif

        return false;
    }

    if( WRL2_COLOR == type )
    {
        if( NULL != color )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple color nodes\n";
            #endif
            return false;
        }

        color = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2_COORDINATE == type )
    {
        if( NULL != coord )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple coordinate nodes\n";
            #endif
            return false;
        }

        coord = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2_NORMAL == type )
    {
        if( NULL != normal )
        {
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple normal nodes\n";
            #endif
            return false;
        }

        normal = aNode;
        return WRL2NODE::AddChildNode( aNode );
    }

    if( WRL2_TEXTURECOORDINATE != type )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] unexpected code branch\n";
        #endif
        return false;
    }

    if( NULL != texCoord )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] bad file format; multiple texCoord nodes\n";
        #endif
        return false;
    }

    texCoord = aNode;
    return WRL2NODE::AddChildNode( aNode );
}



bool WRL2FACESET::Read( WRLPROC& proc, WRL2BASE* aTopNode )
{
    size_t line, column;
    proc.GetFilePosData( line, column );

    char tok = proc.Peek();

    if( proc.eof() )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [INFO] bad file format; unexpected eof at line ";
        std::cerr << line << ", column " << column << "\n";
        #endif
        return false;
    }

    if( '{' != tok )
    {
        #ifdef DEBUG
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
            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << proc.GetError() <<  "\n";
            #endif

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

        proc.GetFilePosData( line, column );

        if( !glob.compare( "ccw" ) )
        {
            if( !proc.ReadSFBool( ccw ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid ccw at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "colorPerVertex" ) )
        {
            if( !proc.ReadSFBool( colorPerVertex ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid colorPerVertex at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "convex" ) )
        {
            if( !proc.ReadSFBool( convex ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid convex at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "normalPerVertex" ) )
        {
            if( !proc.ReadSFBool( normalPerVertex ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid normalPerVertex at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "solid" ) )
        {
            if( !proc.ReadSFBool( solid ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid solid at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "creaseAngle" ) )
        {
            if( !proc.ReadSFFloat( creaseAngle ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid creaseAngle at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "colorIndex" ) )
        {
            if( !proc.ReadMFInt( colorIndex ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid colorIndex at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "coordIndex" ) )
        {
            if( !proc.ReadMFInt( coordIndex ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid coordIndex at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "normalIndex" ) )
        {
            if( !proc.ReadMFInt( normalIndex ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] invalid normalIndex at line " << line << ", column ";
                std::cerr << column << "\n";
                std::cerr << " * [INFO] file: '" << proc.GetFileName() << "'\n";
                std::cerr << " * [INFO] message: '" << proc.GetError() << "'\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "color" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] could not read color node information\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "coord" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] could not read coord node information\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "normal" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] could not read normal node information\n";
                #endif
                return false;
            }
        }
        else if( !glob.compare( "texCoord" ) )
        {
            if( !aTopNode->ReadNode( proc, this, NULL ) )
            {
                #ifdef DEBUG
                std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                std::cerr << " * [INFO] could not read texCoord node information\n";
                #endif
                return false;
            }
        }
        else
        {
            #ifdef DEBUG
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


SGNODE* WRL2FACESET::TranslateToSG( SGNODE* aParent, bool calcNormals )
{
    S3D::SGTYPES ptype = S3D::GetSGNodeType( aParent );

    if( NULL != aParent && ptype != S3D::SGTYPE_SHAPE )
    {
        #ifdef DEBUG
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] IndexedFaceSet does not have a Shape parent (parent ID: ";
        std::cerr << ptype << ")\n";
        #endif

        return NULL;
    }

    if( m_sgNode )
    {
        if( NULL != aParent && aParent != S3D::GetSGNodeParent( m_sgNode )
            && !S3D::AddSGNodeRef( aParent, m_sgNode ) )
        {
            return NULL;
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
    ((WRL2COORDS*) coord)->GetCoords( pcoords, coordsize );

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

    std::cerr << "XXX: [face] NPts : " << lCPts.size() << "\n";
    std::cerr << "XXX: [face] NNorm: " << lCNorm.size() << "\n";
    std::cerr << "XXX: [face] NIdx : " << lCIdx.size() << "\n";

    m_sgNode = fsNode.GetRawPtr();

    return m_sgNode;
}
