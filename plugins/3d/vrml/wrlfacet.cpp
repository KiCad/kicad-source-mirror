/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2017 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

#include "wrlfacet.h"

#define LOWER_LIMIT (1e-12)


static bool VDegenerate( glm::vec3* pts )
{
    // note: only checks the degenerate case of zero length sides; it
    // does not detect the case of 3 distinct collinear points

    double dx, dy, dz;

    dx = double{ pts[1].x } - pts[0].x;
    dy = double{ pts[1].y } - pts[0].y;
    dz = double{ pts[1].z } - pts[0].z;

    if( ( dx*dx + dy*dy + dz*dz ) < LOWER_LIMIT )
        return true;

    dx = double{ pts[2].x } - pts[0].x;
    dy = double{ pts[2].y } - pts[0].y;
    dz = double{ pts[2].z } - pts[0].z;

    if( ( dx*dx + dy*dy + dz*dz ) < LOWER_LIMIT )
        return true;

    dx = double{ pts[2].x } - pts[1].x;
    dy = double{ pts[2].y } - pts[1].y;
    dz = double{ pts[2].z } - pts[1].z;

    if( ( dx*dx + dy*dy + dz*dz ) < LOWER_LIMIT )
        return true;

    return false;
}


static WRLVEC3F VCalcTriNorm( const WRLVEC3F& p1, const WRLVEC3F& p2, const WRLVEC3F& p3 )
{
    // note: p1 = reference vertex
    glm::vec3 tri = glm::vec3( 0.0, 0.0, 0.0 );
    glm::vec3 pts[3];

    pts[0] = p1;
    pts[1] = p2;
    pts[2] = p3;

    // degenerate points are given a default 0, 0, 0 normal
    if( VDegenerate( pts ) )
        return tri;

    // normal
    tri = glm::cross( pts[2] - pts[0], pts[1] - pts[0] );

    float dn = sqrtf( tri.x * tri.x + tri.y * tri.y + tri.z * tri.z );

    if( dn > LOWER_LIMIT )
    {
        tri.x /= dn;
        tri.y /= dn;
        tri.z /= dn;
    }

    return tri;
}


static float VCalcCosAngle( const WRLVEC3F& p1, const WRLVEC3F& p2, const WRLVEC3F& p3 )
{
    // note: p1 = reference vertex
    float l12, l13;
    float dx, dy, dz;

    dx = p2.x - p1.x;
    dy = p2.y - p1.y;
    dz = p2.z - p1.z;
    float p12 = dx*dx + dy*dy + dz*dz;
    l12 = sqrtf( p12 );

    dx = p3.x - p2.x;
    dy = p3.y - p2.y;
    dz = p3.z - p2.z;
    float p23 = dx*dx + dy*dy + dz*dz;

    dx = p3.x - p1.x;
    dy = p3.y - p1.y;
    dz = p3.z - p1.z;
    float p13 = dx*dx + dy*dy + dz*dz;
    l13 = sqrtf( p13 );

    float dn = 2.0f * l12 * l13;

    // place a limit to prevent calculations from blowing up
    if( dn < LOWER_LIMIT )
    {
        if( ( p12 + p13 - p23 ) < FLT_EPSILON )
            return -1.0f;

        if( ( p12 + p13 - p23 ) > FLT_EPSILON )
            return 1.0f;

        return 0.0f;
    }

    float cosAngle = ( p12 + p13 - p23 ) / dn;

    // check the domain; errors in the cosAngle calculation can result in domain errors
    if( cosAngle > 1.0f )
        cosAngle = 1.0f;
    else if( cosAngle < -1.0f )
        cosAngle = -1.0f;

    // note: we are guaranteed that acosf() is never negative
    return cosAngle;
}


FACET::FACET()
{
    face_normal.x = 0.0;
    face_normal.y = 0.0;
    face_normal.z = 0.0;
    maxIdx = 0;
}


void FACET::Init()
{
    vertices.clear();
    colors.clear();
    indices.clear();
    norms.clear();
    vnweight.clear();

    face_normal.x = 0.0;
    face_normal.y = 0.0;
    face_normal.z = 0.0;
    maxIdx = 0;
}


bool FACET::HasMinPoints()
{
    if( vertices.size() < 3 )
        return false;

    return true;
}


bool FACET::HasColors()
{
    if( colors.empty() )
        return false;

    return true;
}


void FACET::AddVertex( WRLVEC3F& aVertex, int aIndex )
{
    if( aIndex < 0 )
        return;

    vertices.push_back( aVertex );
    indices.push_back( aIndex );

    if( aIndex > maxIdx )
        maxIdx = aIndex;
}


void FACET::AddColor( const SGCOLOR& aColor )
{
    colors.push_back( aColor );

    return;
}


float FACET::CalcFaceNormal()
{
    // note: this calculation assumes that the face is a convex polygon;
    // concave polygons may be supported in the future via functions which
    // split the polygon into triangles

    if( vertices.size() < 3 )
        return 0.0;

    // check if the values were already calculated
    if( vertices.size() == vnweight.size() )
        return 0.0;

    WRLVEC3F lCPts[3];

    std::vector< WRLVEC3F >::iterator sV = vertices.begin();
    std::vector< WRLVEC3F >::iterator eV = vertices.end();

    lCPts[0] = vertices.back();
    lCPts[1] = *sV;
    ++sV;
    lCPts[2] = *sV;
    ++sV;

    face_normal = VCalcTriNorm( lCPts[1], lCPts[0], lCPts[2] );

    vnweight.clear();
    WRLVEC3F wnorm = face_normal;

    // calculate area:
    size_t nv = vertices.size();
    float a1 = 0.0;
    glm::vec3 sum( 0.0, 0.0, 0.0 );
    size_t j = 0;

    for( size_t i = 1; i < nv; ++i, ++j )
        sum += glm::cross( vertices[j], vertices[i] );

    a1 = fabs( glm::dot( face_normal, sum ) );
    float a2 = acosf( VCalcCosAngle(  lCPts[1], lCPts[0], lCPts[2] ) );

    wnorm.x *= a1 * a2;
    wnorm.y *= a1 * a2;
    wnorm.z *= a1 * a2;
    vnweight.push_back( wnorm );

    float maxV = fabs( wnorm.x );
    float tV = fabs( wnorm.y );

    if( tV > maxV )
        maxV = tV;

    tV = fabs( wnorm.z );

    if( tV > maxV )
        maxV = tV;

    while( sV != eV )
    {
        lCPts[0] = lCPts[1];
        lCPts[1] = lCPts[2];
        lCPts[2] = *sV;
        ++sV;

        wnorm = face_normal;
        a2 = acosf( VCalcCosAngle(  lCPts[1], lCPts[0], lCPts[2] ) );
        wnorm.x *= a1 * a2;
        wnorm.y *= a1 * a2;
        wnorm.z *= a1 * a2;
        vnweight.push_back( wnorm );

        tV = fabs( wnorm.x );

        if( tV > maxV )
            maxV = tV;

        tV = fabs( wnorm.y );

        if( tV > maxV )
            maxV = tV;

        tV = fabs( wnorm.z );

        if( tV > maxV )
            maxV = tV;
    }

    lCPts[0] = lCPts[1];
    lCPts[1] = lCPts[2];
    lCPts[2] = vertices.front();

    wnorm = face_normal;
    a2 = acosf( VCalcCosAngle(  lCPts[1], lCPts[0], lCPts[2] ) );
    wnorm.x *= a1 * a2;
    wnorm.y *= a1 * a2;
    wnorm.z *= a1 * a2;
    vnweight.push_back( wnorm );

    tV = fabs( wnorm.x );

    if( tV > maxV )
        maxV = tV;

    tV = fabs( wnorm.y );

    if( tV > maxV )
        maxV = tV;

    tV = fabs( wnorm.z );

    if( tV > maxV )
        maxV = tV;

    return maxV;
}


void FACET::CalcVertexNormal( int aIndex, std::list< FACET* > &aFacetList, float aCreaseLimit )
{
    if( vertices.size() < 3 )
        return;

    if( vnweight.size() != vertices.size() )
        return;

    if( norms.size() != vertices.size() )
        norms.resize( vertices.size() );

    std::vector< int >::iterator sI = indices.begin();
    std::vector< int >::iterator eI = indices.end();
    int idx = 0;

    WRLVEC3F fp[2]; // vectors to calculate facet angle
    fp[0].x = 0.0;
    fp[0].y = 0.0;
    fp[0].z = 0.0;

    while( sI != eI )
    {
        if( *sI == aIndex )
        {
            // first set the default (weighted) normal value
            norms[idx] = vnweight[idx];

            // iterate over adjacent facets
            std::list< FACET* >::iterator sF = aFacetList.begin();
            std::list< FACET* >::iterator eF = aFacetList.end();

            while( sF != eF )
            {
                if( this == *sF )
                {
                    ++sF;
                    continue;
                }

                // check the crease angle limit
                (*sF)->GetFaceNormal( fp[1] );

                float thrs = VCalcCosAngle( fp[0], face_normal, fp[1] );

                if( aCreaseLimit <= thrs && (*sF)->GetWeightedNormal( aIndex, fp[1] ) )
                {
                    norms[idx].x += fp[1].x;
                    norms[idx].y += fp[1].y;
                    norms[idx].z += fp[1].z;
                }

                ++sF;
            }

            // normalize the vector
            float dn = sqrtf( norms[idx].x * norms[idx].x
                            + norms[idx].y * norms[idx].y
                            + norms[idx].z * norms[idx].z );

            if( dn > LOWER_LIMIT )
            {
                norms[idx].x /= dn;
                norms[idx].y /= dn;
                norms[idx].z /= dn;
            }

            // if the normals is an invalid normal this test will pass
            if( fabs( norms[idx].x ) < 0.5
                && fabs( norms[idx].y ) < 0.5
                && fabs( norms[idx].z ) < 0.5 )
            {
                norms[idx] = face_normal;
            }

            return;
        }

        ++idx;
        ++sI;
    }
}


bool FACET::GetWeightedNormal( int aIndex, WRLVEC3F& aNorm )
{
    // the default weighted normal shall have no effect even if accidentally included
    aNorm.x = 0.0;
    aNorm.y = 0.0;
    aNorm.z = 0.0;

    if( vertices.size() < 3 )
        return false;

    if( vnweight.size() != vertices.size() )
        return false;

    std::vector< int >::iterator sI = indices.begin();
    std::vector< int >::iterator eI = indices.end();
    int idx = 0;

    while( sI != eI )
    {
        if( *sI == aIndex )
        {
            aNorm = vnweight[idx];
            return true;
        }

        ++idx;
        ++sI;
    }

    return false;
}


bool FACET::GetFaceNormal( WRLVEC3F& aNorm )
{
    aNorm.x = 0.0;
    aNorm.y = 0.0;
    aNorm.z = 0.0;

    if( vertices.size() < 3 )
        return false;

    if( vnweight.size() != vertices.size() )
        return false;

    aNorm = face_normal;
    return true;
}


bool FACET::GetData( std::vector< WRLVEC3F >& aVertexList, std::vector< WRLVEC3F >& aNormalsList,
                     std::vector< SGCOLOR >& aColorsList, WRL1_ORDER aVertexOrder )
{
    // if no normals are calculated we simply return
    if( norms.empty() )
        return false;

    // the output must always be triangle sets in order to conform to the
    // requirements of the SG* classes
    int idx[3];

    idx[0] = 0;
    idx[1] = 1;
    idx[2] = 2;
    WRLVEC3F tnorm;

    if( aVertexOrder != WRL1_ORDER::ORD_CLOCKWISE )
    {
        aVertexList.push_back( vertices[idx[0]] );
        aVertexList.push_back( vertices[idx[1]] );
        aVertexList.push_back( vertices[idx[2]] );

        aNormalsList.push_back( norms[idx[0]] );
        aNormalsList.push_back( norms[idx[1]] );
        aNormalsList.push_back( norms[idx[2]] );
    }

    if( aVertexOrder != WRL1_ORDER::ORD_CCW )
    {
        aVertexList.push_back( vertices[idx[0]] );
        aVertexList.push_back( vertices[idx[2]] );
        aVertexList.push_back( vertices[idx[1]] );

        tnorm = norms[idx[0]];
        tnorm.x = -tnorm.x;
        tnorm.y = -tnorm.y;
        tnorm.z = -tnorm.z;
        aNormalsList.push_back( tnorm );

        tnorm = norms[idx[2]];
        tnorm.x = -tnorm.x;
        tnorm.y = -tnorm.y;
        tnorm.z = -tnorm.z;
        aNormalsList.push_back( tnorm );

        tnorm = norms[idx[1]];
        tnorm.x = -tnorm.x;
        tnorm.y = -tnorm.y;
        tnorm.z = -tnorm.z;
        aNormalsList.push_back( tnorm );
    }

    bool hasColor = false;
    bool perVC = false; // per-vertex colors?

    if( !colors.empty() )
    {
        hasColor = true;

        if( colors.size() >= vertices.size() )
            perVC = true;

        if( perVC )
        {
            if( aVertexOrder != WRL1_ORDER::ORD_CLOCKWISE )
            {
                aColorsList.push_back( colors[idx[0]] );
                aColorsList.push_back( colors[idx[1]] );
                aColorsList.push_back( colors[idx[2]] );
            }

            if( aVertexOrder != WRL1_ORDER::ORD_CCW )
            {
                aColorsList.push_back( colors[idx[0]] );
                aColorsList.push_back( colors[idx[2]] );
                aColorsList.push_back( colors[idx[1]] );
            }
        }
        else
        {
            if( aVertexOrder != WRL1_ORDER::ORD_CLOCKWISE )
            {
                aColorsList.push_back( colors[0] );
                aColorsList.push_back( colors[0] );
                aColorsList.push_back( colors[0] );
            }

            if( aVertexOrder != WRL1_ORDER::ORD_CCW )
            {
                aColorsList.push_back( colors[0] );
                aColorsList.push_back( colors[0] );
                aColorsList.push_back( colors[0] );
            }
        }
    }

    int lim = (int) vertices.size() - 1;

    while( idx[2] <  lim )
    {
        idx[1] = idx[2];
        ++idx[2];

        if( aVertexOrder != WRL1_ORDER::ORD_CLOCKWISE )
        {
            aVertexList.push_back( vertices[idx[0]] );
            aVertexList.push_back( vertices[idx[1]] );
            aVertexList.push_back( vertices[idx[2]] );

            aNormalsList.push_back( norms[idx[0]] );
            aNormalsList.push_back( norms[idx[1]] );
            aNormalsList.push_back( norms[idx[2]] );
        }

        if( aVertexOrder != WRL1_ORDER::ORD_CCW )
        {
            aVertexList.push_back( vertices[idx[0]] );
            aVertexList.push_back( vertices[idx[2]] );
            aVertexList.push_back( vertices[idx[1]] );

            tnorm = norms[idx[0]];
            tnorm.x = -tnorm.x;
            tnorm.y = -tnorm.y;
            tnorm.z = -tnorm.z;
            aNormalsList.push_back( tnorm );

            tnorm = norms[idx[2]];
            tnorm.x = -tnorm.x;
            tnorm.y = -tnorm.y;
            tnorm.z = -tnorm.z;
            aNormalsList.push_back( tnorm );

            tnorm = norms[idx[1]];
            tnorm.x = -tnorm.x;
            tnorm.y = -tnorm.y;
            tnorm.z = -tnorm.z;
            aNormalsList.push_back( tnorm );
        }

        if( hasColor )
        {
            if( perVC )
            {
                if( aVertexOrder != WRL1_ORDER::ORD_CLOCKWISE )
                {
                    aColorsList.push_back( colors[idx[0]] );
                    aColorsList.push_back( colors[idx[1]] );
                    aColorsList.push_back( colors[idx[2]] );
                }

                if( aVertexOrder != WRL1_ORDER::ORD_CCW )
                {
                    aColorsList.push_back( colors[idx[0]] );
                    aColorsList.push_back( colors[idx[2]] );
                    aColorsList.push_back( colors[idx[1]] );
                }
            }
            else
            {
                if( aVertexOrder != WRL1_ORDER::ORD_CLOCKWISE )
                {
                    aColorsList.push_back( colors[0] );
                    aColorsList.push_back( colors[0] );
                    aColorsList.push_back( colors[0] );
                }

                if( aVertexOrder != WRL1_ORDER::ORD_CCW )
                {
                    aColorsList.push_back( colors[0] );
                    aColorsList.push_back( colors[0] );
                    aColorsList.push_back( colors[0] );
                }
            }
        }
    }

    return true;
}


void FACET::CollectVertices( std::vector< std::list< FACET* > >& aFacetList )
{
    // check if this facet may contribute anything at all
    if( vertices.size() < 3 )
        return;

    // note: in principle this should never be invoked
    if( (maxIdx + 1) >= (int)aFacetList.size() )
        aFacetList.resize( static_cast<std::size_t>( maxIdx ) + 1 );

    std::vector< int >::iterator sI = indices.begin();
    std::vector< int >::iterator eI = indices.end();

    while( sI != eI )
    {
        aFacetList[*sI].push_back( this );
        ++sI;
    }
}


void FACET::Renormalize( float aMaxValue )
{
    if( vnweight.empty() || aMaxValue < LOWER_LIMIT )
        return;

    size_t vs = vnweight.size();

    for( size_t i = 0; i < vs; ++i )
    {
        vnweight[i].x /= aMaxValue;
        vnweight[i].y /= aMaxValue;
        vnweight[i].z /= aMaxValue;
    }
}


SHAPE::~SHAPE()
{
    std::list< FACET* >::iterator sF = facets.begin();
    std::list< FACET* >::iterator eF = facets.end();

    while( sF != eF )
    {
        delete *sF;
        ++sF;
    }

    facets.clear();
    return;
}


FACET* SHAPE::NewFacet()
{
    FACET* fp = new FACET;
    facets.push_back( fp );
    return fp;
}


SGNODE* SHAPE::CalcShape( SGNODE* aParent, SGNODE* aColor, WRL1_ORDER aVertexOrder,
                          float aCreaseLimit, bool isVRML2 )
{
    if( facets.empty() || !facets.front()->HasMinPoints() )
        return nullptr;

    std::vector< std::list< FACET* > > flist;

    // determine the max. index and size flist as appropriate
    std::list< FACET* >::iterator sF = facets.begin();
    std::list< FACET* >::iterator eF = facets.end();

    int maxIdx = 0;
    int tmi;
    float maxV = 0.0;
    float tV = 0.0;

    while( sF != eF )
    {
        tV = ( *sF )->CalcFaceNormal();
        tmi = ( *sF )->GetMaxIndex();

        if( tmi > maxIdx )
            maxIdx = tmi;

        if( tV > maxV )
            maxV = tV;

        ++sF;
    }

    ++maxIdx;

    if( maxIdx < 3 )
        return nullptr;

    flist.resize( maxIdx );

    // create the lists of facets common to indices
    sF = facets.begin();

    while( sF != eF )
    {
        ( *sF )->Renormalize( tV );
        ( *sF )->CollectVertices( flist );
        ++sF;
    }

    // calculate the normals
    size_t vs = flist.size();

    for( size_t i = 0; i < vs; ++i )
    {
        sF = flist[i].begin();
        eF = flist[i].end();

        while( sF != eF )
        {
            ( *sF )->CalcVertexNormal( static_cast<int>( i ), flist[i], aCreaseLimit );
            ++sF;
        }
    }

    std::vector< WRLVEC3F > vertices;
    std::vector< WRLVEC3F > normals;
    std::vector< SGCOLOR >  colors;

    // push the facet data to the final output list
    sF = facets.begin();
    eF = facets.end();

    while( sF != eF )
    {
        ( *sF )->GetData( vertices, normals, colors, aVertexOrder );
        ++sF;
    }

    flist.clear();

    if( vertices.size() < 3 )
        return nullptr;

    IFSG_SHAPE shapeNode( false );

    if( !isVRML2 )
    {
        shapeNode.NewNode( aParent );

        if( aColor )
        {
            if( nullptr == S3D::GetSGNodeParent( aColor ) )
                shapeNode.AddChildNode( aColor );
            else
                shapeNode.AddRefNode( aColor );
        }
    }

    std::vector< SGPOINT >  lCPts;  // vertex points in SGPOINT (double) format
    std::vector< SGVECTOR > lCNorm; // per-vertex normals
    vs = vertices.size();

    for( size_t i = 0; i < vs; ++i )
    {
        SGPOINT pt;
        pt.x = vertices[i].x;
        pt.y = vertices[i].y;
        pt.z = vertices[i].z;
        lCPts.push_back( pt );
        lCNorm.emplace_back( normals[i].x, normals[i].y, normals[i].z );
    }

    vertices.clear();
    normals.clear();

    IFSG_FACESET fsNode( false );

    if( !isVRML2 )
        fsNode.NewNode( shapeNode );
    else
        fsNode.NewNode( aParent );

    IFSG_COORDS cpNode( fsNode );
    cpNode.SetCoordsList( lCPts.size(), &lCPts[0] );
    IFSG_COORDINDEX ciNode( fsNode );

    for( int i = 0; i < (int)lCPts.size(); ++i )
        ciNode.AddIndex( i );

    IFSG_NORMALS nmNode( fsNode );
    nmNode.SetNormalList( lCNorm.size(), &lCNorm[0] );

    if( !colors.empty() )
    {
        IFSG_COLORS nmColor( fsNode );
        nmColor.SetColorList( colors.size(), &colors[0] );
        colors.clear();
    }

    if( !isVRML2 )
        return shapeNode.GetRawPtr();

    return fsNode.GetRawPtr();
}
