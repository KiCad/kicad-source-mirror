/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

/**
 * @file  bvh_pbrt.cpp
 * @brief This BVH implementation is based on the source code implementation
 * from the book "Physically Based Rendering" (v2 and v3)
 *
 * Adaptions performed for KiCad:
 *  - Types and class types adapted to KiCad project
 *  - Convert some source to build in the C++ specification of KiCad
 *  - Code style to match KiCad
 *  - Asserts converted
 *  - Use compare functions/structures for std::partition and std::nth_element
 *
 * The original source code has the following license:
 *
 * "pbrt source code is Copyright(c) 1998-2015
 *                      Matt Pharr, Greg Humphreys, and Wenzel Jakob.
 *
 *  This file is part of pbrt.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 *
 */

#include "bvh_pbrt.h"
#include "../../../3d_fastmath.h"
#include <macros.h>

#include <boost/range/algorithm/nth_element.hpp>
#include <boost/range/algorithm/partition.hpp>
#include <cstdlib>
#include <vector>

#include <stack>
#include <wx/debug.h>

#ifdef PRINT_STATISTICS_3D_VIEWER
#include <stdio.h>
#endif

// BVHAccel Local Declarations
struct BVHPrimitiveInfo
{
    BVHPrimitiveInfo()
    {
        primitiveNumber = 0;
        bounds.Reset();
        centroid = SFVEC3F( 0.0f );
    }

    BVHPrimitiveInfo( int aPrimitiveNumber, const BBOX_3D& aBounds ) :
        primitiveNumber( aPrimitiveNumber ),
        bounds( aBounds ),
        centroid( .5f * aBounds.Min() + .5f * aBounds.Max() ) {}

    int     primitiveNumber;
    BBOX_3D bounds;
    SFVEC3F centroid;
};


struct BVHBuildNode
{
    // BVHBuildNode Public Methods
    void InitLeaf( int first, int n, const BBOX_3D& b)
    {
        firstPrimOffset = first;
        nPrimitives = n;
        bounds = b;
        children[0] = children[1] = nullptr;
    }

    void InitInterior( int axis, BVHBuildNode* c0, BVHBuildNode* c1 )
    {
        children[0] = c0;
        children[1] = c1;
        bounds.Set( c0->bounds );
        bounds.Union( c1->bounds );
        splitAxis = axis;
        nPrimitives = 0;
    }

    BBOX_3D bounds;
    BVHBuildNode* children[2];
    int splitAxis, firstPrimOffset, nPrimitives;
};


struct MortonPrimitive
{
    int primitiveIndex;
    uint32_t mortonCode;
};


struct LBVHTreelet
{
    int startIndex, numPrimitives;
    BVHBuildNode *buildNodes;
};


// BVHAccel Utility Functions
inline uint32_t LeftShift3( uint32_t x )
{
    wxASSERT( x <= (1 << 10) );

    if( x == ( 1 << 10 ) )
        --x;

    x = ( x | ( x << 16 ) ) & 0b00000011000000000000000011111111;
    // x = ---- --98 ---- ---- ---- ---- 7654 3210
    x = ( x | ( x << 8 ) ) & 0b00000011000000001111000000001111;
    // x = ---- --98 ---- ---- 7654 ---- ---- 3210
    x = ( x | ( x << 4 ) ) & 0b00000011000011000011000011000011;
    // x = ---- --98 ---- 76-- --54 ---- 32-- --10
    x = ( x | ( x << 2 ) ) & 0b00001001001001001001001001001001;
    // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0

    return x;
}


inline uint32_t EncodeMorton3( const SFVEC3F& v )
{
    wxASSERT( v.x >= 0 && v.x <= ( 1 << 10 ) );
    wxASSERT( v.y >= 0 && v.y <= ( 1 << 10 ) );
    wxASSERT( v.z >= 0 && v.z <= ( 1 << 10 ) );

    return ( LeftShift3( v.z ) << 2 ) | ( LeftShift3( v.y ) << 1 ) | LeftShift3( v.x );
}


static void RadixSort( std::vector<MortonPrimitive>* v )
{
    std::vector<MortonPrimitive> tempVector( v->size() );

    const int bitsPerPass = 6;
    const int nBits = 30;

    wxASSERT( ( nBits % bitsPerPass ) == 0 );

    const int nPasses = nBits / bitsPerPass;

    for( int pass = 0; pass < nPasses; ++pass )
    {
        // Perform one pass of radix sort, sorting _bitsPerPass_ bits
        const int lowBit = pass * bitsPerPass;

        // Set in and out vector pointers for radix sort pass
        std::vector<MortonPrimitive>& in = ( pass & 1 ) ? tempVector : *v;
        std::vector<MortonPrimitive>& out = ( pass & 1 ) ? *v : tempVector;

        // Count number of zero bits in array for current radix sort bit
        const int nBuckets = 1 << bitsPerPass;
        int bucketCount[nBuckets] = {0};
        const int bitMask = ( 1 << bitsPerPass ) - 1;

        for( uint32_t i = 0; i < in.size(); ++i )
        {
            const MortonPrimitive &mp = in[i];
            int bucket = ( mp.mortonCode >> lowBit ) & bitMask;

            wxASSERT( ( bucket >= 0 ) && ( bucket < nBuckets ) );

            ++bucketCount[bucket];
        }

        // Compute starting index in output array for each bucket
        int startIndex[nBuckets];
        startIndex[0] = 0;

        for( int i = 1; i < nBuckets; ++i )
            startIndex[i] = startIndex[i - 1] + bucketCount[i - 1];

        // Store sorted values in output array
        for( uint32_t i = 0; i < in.size(); ++i )
        {
            const MortonPrimitive& mp = in[i];
            int bucket = (mp.mortonCode >> lowBit) & bitMask;
            out[startIndex[bucket]++] = mp;
        }
    }

    // Copy final result from _tempVector_, if needed
    if( nPasses & 1 )
        std::swap( *v, tempVector );
}


BVH_PBRT::BVH_PBRT( const CONTAINER_3D_BASE& aObjectContainer, int aMaxPrimsInNode,
                    SPLITMETHOD aSplitMethod ) :
    m_maxPrimsInNode( std::min( 255, aMaxPrimsInNode ) ),
    m_splitMethod( aSplitMethod )
{
    if( aObjectContainer.GetList().empty() )
    {
        m_nodes = nullptr;

        return;
    }

    // Initialize the indexes of ray packet for partition traversal
    for( unsigned int i = 0; i < RAYPACKET_RAYS_PER_PACKET; ++i )
    {
        m_I[i] = i;
    }

    // Convert the objects list to vector of objects
    aObjectContainer.ConvertTo( m_primitives );

    wxASSERT( aObjectContainer.GetList().size() == m_primitives.size() );

    // Initialize _primitiveInfo_ array for primitives
    std::vector<BVHPrimitiveInfo> primitiveInfo( m_primitives.size() );

    for( size_t i = 0; i < m_primitives.size(); ++i )
    {
        wxASSERT( m_primitives[i]->GetBBox().IsInitialized() );

        primitiveInfo[i] = BVHPrimitiveInfo( i, m_primitives[i]->GetBBox() );
    }

    // Build BVH tree for primitives using _primitiveInfo_
    int totalNodes = 0;

    CONST_VECTOR_OBJECT orderedPrims;
    orderedPrims.clear();
    orderedPrims.reserve( m_primitives.size() );

    BVHBuildNode *root;

    if( m_splitMethod == SPLITMETHOD::HLBVH )
        root = HLBVHBuild( primitiveInfo, &totalNodes, orderedPrims );
    else
        root = recursiveBuild( primitiveInfo, 0, m_primitives.size(), &totalNodes, orderedPrims );

    wxASSERT( m_primitives.size() == orderedPrims.size() );

    m_primitives.swap( orderedPrims );

    // Compute representation of depth-first traversal of BVH tree
    m_nodes = static_cast<LinearBVHNode*>( malloc( sizeof( LinearBVHNode ) * totalNodes ) );
    m_nodesToFree.push_back( m_nodes );

    for( int i = 0; i < totalNodes; ++i )
    {
        m_nodes[i].bounds.Reset();
        m_nodes[i].primitivesOffset = 0;
        m_nodes[i].nPrimitives = 0;
        m_nodes[i].axis = 0;
    }

    uint32_t offset = 0;

    flattenBVHTree( root, &offset );

    wxASSERT( offset == (unsigned int)totalNodes );
}


BVH_PBRT::~BVH_PBRT()
{
    if( !m_nodesToFree.empty() )
    {
        for( std::list<void *>::iterator ii = m_nodesToFree.begin(); ii != m_nodesToFree.end();
             ++ii )
        {
            free( *ii );
            *ii = nullptr;
        }
    }

    m_nodesToFree.clear();
}


struct ComparePoints
{
    explicit ComparePoints( int d ) { dim = d; }

    int dim;

    bool operator()( const BVHPrimitiveInfo& a, const BVHPrimitiveInfo& b ) const
    {
        return a.centroid[dim] < b.centroid[dim];
    }
};


struct CompareToMid
{
    explicit CompareToMid( int d, float m ) { dim = d; mid = m; }

    int dim;
    float mid;

    bool operator()( const BVHPrimitiveInfo& a ) const
    {
        return a.centroid[dim] < mid;
    }
};


struct CompareToBucket
{
    CompareToBucket( int split, int num, int d, const BBOX_3D& b )
        : centroidBounds( b )
    {
        splitBucket = split;
        nBuckets = num;
        dim = d;
    }

    bool operator()( const BVHPrimitiveInfo& p ) const;

    int splitBucket, nBuckets, dim;

    const BBOX_3D& centroidBounds;
};


bool CompareToBucket::operator()( const BVHPrimitiveInfo& p ) const
{
    const float centroid = p.centroid[dim];

    int b = nBuckets *
            // Computes the offset (0.0 - 1.0) for one axis
            ( ( centroid - centroidBounds.Min()[dim] ) /
              ( centroidBounds.Max()[dim] - centroidBounds.Min()[dim] ) );

    if( b == nBuckets )
        b = nBuckets - 1;

    wxASSERT( ( b >= 0 ) && ( b < nBuckets ) );

    return b <= splitBucket;
}


struct HLBVH_SAH_Evaluator
{
    HLBVH_SAH_Evaluator( int split, int num, int d, const BBOX_3D& b )
        : centroidBounds(b)
    {
        minCostSplitBucket = split;
        nBuckets = num; dim = d;
    }

    bool operator()( const BVHBuildNode* node ) const;

    int minCostSplitBucket, nBuckets, dim;
    const BBOX_3D& centroidBounds;
};


bool HLBVH_SAH_Evaluator::operator()( const BVHBuildNode* node ) const
{
    const float centroid = node->bounds.GetCenter( dim );

    int b = nBuckets *
            // Computes the offset (0.0 - 1.0) for one axis
            ( ( centroid - centroidBounds.Min()[dim] ) /
              ( centroidBounds.Max()[dim] - centroidBounds.Min()[dim] ) );

    if( b == nBuckets )
        b = nBuckets - 1;

    wxASSERT( b >= 0 && b < nBuckets );

    return b <= minCostSplitBucket;
}


struct BucketInfo
{
    int count;
    BBOX_3D bounds;
};


BVHBuildNode *BVH_PBRT::recursiveBuild ( std::vector<BVHPrimitiveInfo>& primitiveInfo,
                                         int start, int end, int* totalNodes,
                                         CONST_VECTOR_OBJECT& orderedPrims )
{
    wxASSERT( totalNodes != nullptr );
    wxASSERT( start >= 0 );
    wxASSERT( end   >= 0 );
    wxASSERT( start != end );
    wxASSERT( start < end );
    wxASSERT( start <= (int)primitiveInfo.size() );
    wxASSERT( end   <= (int)primitiveInfo.size() );

    (*totalNodes)++;

    // !TODO: implement a memory arena
    BVHBuildNode *node = static_cast<BVHBuildNode *>( malloc( sizeof( BVHBuildNode ) ) );
    m_nodesToFree.push_back( node );

    node->bounds.Reset();
    node->firstPrimOffset = 0;
    node->nPrimitives = 0;
    node->splitAxis = 0;
    node->children[0] = nullptr;
    node->children[1] = nullptr;

    // Compute bounds of all primitives in BVH node
    BBOX_3D bounds;
    bounds.Reset();

    for( int i = start; i < end; ++i )
        bounds.Union( primitiveInfo[i].bounds );

    int nPrimitives = end - start;

    if( nPrimitives == 1 )
    {
        // Create leaf _BVHBuildNode_
        int firstPrimOffset = orderedPrims.size();

        for( int i = start; i < end; ++i )
        {
            int primitiveNr = primitiveInfo[i].primitiveNumber;
            wxASSERT( primitiveNr < (int)m_primitives.size() );
            orderedPrims.push_back( m_primitives[ primitiveNr ] );
        }

        node->InitLeaf( firstPrimOffset, nPrimitives, bounds );
    }
    else
    {
        // Compute bound of primitive centroids, choose split dimension _dim_
        BBOX_3D centroidBounds;
        centroidBounds.Reset();

        for( int i = start; i < end; ++i )
            centroidBounds.Union( primitiveInfo[i].centroid );

        const int dim = centroidBounds.MaxDimension();

        // Partition primitives into two sets and build children
        int mid = (start + end) / 2;

        if( fabs( centroidBounds.Max()[dim] -
                  centroidBounds.Min()[dim] ) < (FLT_EPSILON + FLT_EPSILON) )
        {
            // Create leaf _BVHBuildNode_
            const int firstPrimOffset = orderedPrims.size();

            for( int i = start; i < end; ++i )
            {
                int primitiveNr = primitiveInfo[i].primitiveNumber;

                wxASSERT( ( primitiveNr >= 0 ) && ( primitiveNr < (int) m_primitives.size() ) );

                const OBJECT_3D* obj = static_cast<const OBJECT_3D*>( m_primitives[ primitiveNr ] );

                wxASSERT( obj != nullptr );

                orderedPrims.push_back( obj );
            }

            node->InitLeaf( firstPrimOffset, nPrimitives, bounds );
        }
        else
        {
            // Partition primitives based on _splitMethod_
            switch( m_splitMethod )
            {
            case SPLITMETHOD::MIDDLE:
            {
                // Partition primitives through node's midpoint
                float pmid = centroidBounds.GetCenter( dim );

                BVHPrimitiveInfo *midPtr = std::partition( &primitiveInfo[start],
                                                           &primitiveInfo[end - 1] + 1,
                                                           CompareToMid( dim, pmid ) );
                mid = midPtr - &primitiveInfo[0];

                wxASSERT( ( mid >= start ) && ( mid <= end ) );

                if( ( mid != start ) && ( mid != end ) )
                    break;
            }

            // Intentionally fall through to SPLITMETHOD::EQUAL_COUNTS since prims
            // with large overlapping bounding boxes may fail to partition
            KI_FALLTHROUGH;

            case SPLITMETHOD::EQUALCOUNTS:
            {
                // Partition primitives into equally-sized subsets
                mid = ( start + end ) / 2;

                std::nth_element( &primitiveInfo[start], &primitiveInfo[mid],
                                  &primitiveInfo[end - 1] + 1, ComparePoints( dim ) );

                break;
            }

            case SPLITMETHOD::SAH:
            default:
            {
                // Partition primitives using approximate SAH
                if( nPrimitives <= 2 )
                {
                    // Partition primitives into equally-sized subsets
                    mid = ( start + end ) / 2;

                    std::nth_element( &primitiveInfo[start], &primitiveInfo[mid],
                                      &primitiveInfo[end - 1] + 1, ComparePoints( dim ) );
                }
                else
                {
                    // Allocate _BucketInfo_ for SAH partition buckets
                    const int nBuckets = 12;

                    BucketInfo buckets[nBuckets];

                    for( int i = 0; i < nBuckets; ++i )
                    {
                        buckets[i].count = 0;
                        buckets[i].bounds.Reset();
                    }

                    // Initialize _BucketInfo_ for SAH partition buckets
                    for( int i = start; i < end; ++i )
                    {
                        int b = nBuckets *
                                centroidBounds.Offset( primitiveInfo[i].centroid )[dim];

                        if( b == nBuckets )
                            b = nBuckets - 1;

                        wxASSERT( b >= 0 && b < nBuckets );

                        buckets[b].count++;
                        buckets[b].bounds.Union( primitiveInfo[i].bounds );
                    }

                    // Compute costs for splitting after each bucket
                    float cost[nBuckets - 1];

                    for( int i = 0; i < ( nBuckets - 1 ); ++i )
                    {
                        BBOX_3D b0, b1;

                        b0.Reset();
                        b1.Reset();

                        int count0 = 0;
                        int count1 = 0;

                        for( int j = 0; j <= i; ++j )
                        {
                            if( buckets[j].count )
                            {
                                count0 += buckets[j].count;
                                b0.Union( buckets[j].bounds );
                            }
                        }

                        for( int j = i + 1; j < nBuckets; ++j )
                        {
                            if( buckets[j].count )
                            {
                                count1 += buckets[j].count;
                                b1.Union( buckets[j].bounds );
                            }
                        }

                        cost[i] = 1.0f + ( count0 * b0.SurfaceArea() +
                                           count1 * b1.SurfaceArea() ) / bounds.SurfaceArea();
                    }

                    // Find bucket to split at that minimizes SAH metric
                    float minCost = cost[0];
                    int minCostSplitBucket = 0;

                    for( int i = 1; i < ( nBuckets - 1 ); ++i )
                    {
                        if( cost[i] < minCost )
                        {
                            minCost = cost[i];
                            minCostSplitBucket = i;
                        }
                    }

                    // Either create leaf or split primitives at selected SAH bucket
                    if( ( nPrimitives > m_maxPrimsInNode ) || ( minCost < (float) nPrimitives ) )
                    {
                        BVHPrimitiveInfo *pmid =
                            std::partition( &primitiveInfo[start], &primitiveInfo[end - 1] + 1,
                                            CompareToBucket( minCostSplitBucket, nBuckets,
                                                             dim, centroidBounds ) );
                        mid = pmid - &primitiveInfo[0];

                        wxASSERT( ( mid >= start ) && ( mid <= end ) );
                    }
                    else
                    {
                        // Create leaf _BVHBuildNode_
                        const int firstPrimOffset = orderedPrims.size();

                        for( int i = start; i < end; ++i )
                        {
                            const int primitiveNr = primitiveInfo[i].primitiveNumber;

                            wxASSERT( primitiveNr < (int)m_primitives.size() );

                            orderedPrims.push_back( m_primitives[ primitiveNr ] );
                        }

                        node->InitLeaf( firstPrimOffset, nPrimitives, bounds );

                        return node;
                    }
                }
                break;
            }
            }

            node->InitInterior( dim, recursiveBuild( primitiveInfo, start, mid, totalNodes,
                                                     orderedPrims ),
                                recursiveBuild( primitiveInfo, mid, end, totalNodes,
                                                orderedPrims) );
        }
    }

    return node;
}


BVHBuildNode *BVH_PBRT::HLBVHBuild( const std::vector<BVHPrimitiveInfo>& primitiveInfo,
                                    int* totalNodes, CONST_VECTOR_OBJECT& orderedPrims )
{
    // Compute bounding box of all primitive centroids
    BBOX_3D bounds;
    bounds.Reset();

    for( unsigned int i = 0; i < primitiveInfo.size(); ++i )
        bounds.Union( primitiveInfo[i].centroid );

    // Compute Morton indices of primitives
    std::vector<MortonPrimitive> mortonPrims( primitiveInfo.size() );

    for( int i = 0; i < (int)primitiveInfo.size(); ++i )
    {
        // Initialize _mortonPrims[i]_ for _i_th primitive
        const int mortonBits  = 10;
        const int mortonScale = 1 << mortonBits;

        wxASSERT( primitiveInfo[i].primitiveNumber < (int)primitiveInfo.size() );

        mortonPrims[i].primitiveIndex = primitiveInfo[i].primitiveNumber;

        const SFVEC3F centroidOffset = bounds.Offset( primitiveInfo[i].centroid );

        wxASSERT( ( centroidOffset.x >= 0.0f ) && ( centroidOffset.x <= 1.0f ) );
        wxASSERT( ( centroidOffset.y >= 0.0f ) && ( centroidOffset.y <= 1.0f ) );
        wxASSERT( ( centroidOffset.z >= 0.0f ) && ( centroidOffset.z <= 1.0f ) );

        mortonPrims[i].mortonCode = EncodeMorton3( centroidOffset * SFVEC3F( (float)mortonScale ) );
    }

    // Radix sort primitive Morton indices
    RadixSort( &mortonPrims );

    // Create LBVH treelets at bottom of BVH

    // Find intervals of primitives for each treelet
    std::vector<LBVHTreelet> treeletsToBuild;

    for( int start = 0, end = 1; end <= (int)mortonPrims.size(); ++end )
    {
        const uint32_t mask = 0b00111111111111000000000000000000;

        if( ( end == (int) mortonPrims.size() )
          || ( ( mortonPrims[start].mortonCode & mask )
               != ( mortonPrims[end].mortonCode & mask ) ) )
        {
            // Add entry to _treeletsToBuild_ for this treelet
            const int numPrimitives = end - start;
            const int maxBVHNodes = 2 * numPrimitives;

            // !TODO: implement a memory arena
            BVHBuildNode *nodes = static_cast<BVHBuildNode *>( malloc( maxBVHNodes *
                                                                       sizeof( BVHBuildNode ) ) );

            m_nodesToFree.push_back( nodes );

            for( int i = 0; i < maxBVHNodes; ++i )
            {
                nodes[i].bounds.Reset();
                nodes[i].firstPrimOffset = 0;
                nodes[i].nPrimitives = 0;
                nodes[i].splitAxis = 0;
                nodes[i].children[0] = nullptr;
                nodes[i].children[1] = nullptr;
            }

            LBVHTreelet tmpTreelet;

            tmpTreelet.startIndex = start;
            tmpTreelet.numPrimitives = numPrimitives;
            tmpTreelet.buildNodes = nodes;

            treeletsToBuild.push_back( tmpTreelet );

            start = end;
        }
    }

    // Create LBVHs for treelets in parallel
    int atomicTotal = 0;
    int orderedPrimsOffset = 0;

    orderedPrims.resize( m_primitives.size() );

    for( int index = 0; index < (int)treeletsToBuild.size(); ++index )
    {
        // Generate _index_th LBVH treelet
        int nodesCreated = 0;
        const int firstBit = 29 - 12;

        LBVHTreelet &tr = treeletsToBuild[index];

        wxASSERT( tr.startIndex < (int)mortonPrims.size() );

        tr.buildNodes = emitLBVH( tr.buildNodes, primitiveInfo, &mortonPrims[tr.startIndex],
                                  tr.numPrimitives, &nodesCreated, orderedPrims,
                                  &orderedPrimsOffset, firstBit );

        atomicTotal += nodesCreated;
    }

    *totalNodes = atomicTotal;

    // Initialize _finishedTreelets_ with treelet root node pointers
    std::vector<BVHBuildNode *> finishedTreelets;
    finishedTreelets.reserve( treeletsToBuild.size() );

    for( int index = 0; index < (int)treeletsToBuild.size(); ++index )
        finishedTreelets.push_back( treeletsToBuild[index].buildNodes );

    // Create and return SAH BVH from LBVH treelets
    return buildUpperSAH( finishedTreelets, 0, finishedTreelets.size(), totalNodes );
}


BVHBuildNode *BVH_PBRT::emitLBVH( BVHBuildNode* &buildNodes,
                                  const std::vector<BVHPrimitiveInfo>& primitiveInfo,
                                  MortonPrimitive* mortonPrims, int nPrimitives, int* totalNodes,
                                  CONST_VECTOR_OBJECT& orderedPrims, int *orderedPrimsOffset,
                                  int bit )
{
    wxASSERT( nPrimitives > 0 );
    wxASSERT( totalNodes != nullptr );
    wxASSERT( orderedPrimsOffset != nullptr );
    wxASSERT( nPrimitives > 0 );
    wxASSERT( mortonPrims != nullptr );

    if( ( bit == -1 ) || ( nPrimitives < m_maxPrimsInNode ) )
    {
        // Create and return leaf node of LBVH treelet
        (*totalNodes)++;

        BVHBuildNode *node = buildNodes++;
        BBOX_3D bounds;
        bounds.Reset();

        int firstPrimOffset = *orderedPrimsOffset;
        *orderedPrimsOffset += nPrimitives;

        wxASSERT( ( firstPrimOffset + ( nPrimitives - 1 ) ) < (int) orderedPrims.size() );

        for( int i = 0; i < nPrimitives; ++i )
        {
            const int primitiveIndex = mortonPrims[i].primitiveIndex;

            wxASSERT( primitiveIndex < (int)m_primitives.size() );

            orderedPrims[firstPrimOffset + i] = m_primitives[primitiveIndex];
            bounds.Union( primitiveInfo[primitiveIndex].bounds );
        }

        node->InitLeaf( firstPrimOffset, nPrimitives, bounds );

        return node;
    }
    else
    {
        int mask = 1 << bit;

        // Advance to next subtree level if there's no LBVH split for this bit
        if( ( mortonPrims[0].mortonCode & mask ) ==
            ( mortonPrims[nPrimitives - 1].mortonCode & mask ) )
            return emitLBVH( buildNodes, primitiveInfo, mortonPrims, nPrimitives, totalNodes,
                             orderedPrims, orderedPrimsOffset, bit - 1 );

        // Find LBVH split point for this dimension
        int searchStart = 0;
        int searchEnd = nPrimitives - 1;

        while( searchStart + 1 != searchEnd )
        {
            wxASSERT( searchStart != searchEnd );

            const int mid = ( searchStart + searchEnd ) / 2;

            if( ( mortonPrims[searchStart].mortonCode & mask ) ==
                ( mortonPrims[mid].mortonCode & mask ) )
                searchStart = mid;
            else
            {
                wxASSERT( ( mortonPrims[mid].mortonCode & mask ) ==
                          ( mortonPrims[searchEnd].mortonCode & mask ) );
                searchEnd = mid;
            }
        }

        const int splitOffset = searchEnd;

        wxASSERT( splitOffset <= ( nPrimitives - 1 ) );
        wxASSERT( ( mortonPrims[splitOffset - 1].mortonCode & mask )
                  != ( mortonPrims[splitOffset].mortonCode & mask ) );

        // Create and return interior LBVH node
        (*totalNodes)++;

        BVHBuildNode *node = buildNodes++;
        BVHBuildNode *lbvh[2];

       lbvh[0] = emitLBVH( buildNodes, primitiveInfo, mortonPrims, splitOffset,
                           totalNodes, orderedPrims, orderedPrimsOffset, bit - 1 );

       lbvh[1] = emitLBVH( buildNodes, primitiveInfo, &mortonPrims[splitOffset],
                           nPrimitives - splitOffset, totalNodes, orderedPrims,
                           orderedPrimsOffset, bit - 1 );

        const int axis = bit % 3;

        node->InitInterior( axis, lbvh[0], lbvh[1] );

        return node;
    }
}


BVHBuildNode *BVH_PBRT::buildUpperSAH( std::vector<BVHBuildNode*>& treeletRoots, int start,
                                       int end, int* totalNodes )
{
    wxASSERT( totalNodes != nullptr );
    wxASSERT( start < end );
    wxASSERT( end <= (int)treeletRoots.size() );

    int nNodes = end - start;

    if( nNodes == 1 )
        return treeletRoots[start];

    (*totalNodes)++;

    BVHBuildNode* node = static_cast<BVHBuildNode*>( malloc( sizeof( BVHBuildNode ) ) );

    m_nodesToFree.push_back( node );

    node->bounds.Reset();
    node->firstPrimOffset = 0;
    node->nPrimitives = 0;
    node->splitAxis = 0;
    node->children[0] = nullptr;
    node->children[1] = nullptr;

    // Compute bounds of all nodes under this HLBVH node
    BBOX_3D bounds;
    bounds.Reset();

    for( int i = start; i < end; ++i )
        bounds.Union( treeletRoots[i]->bounds );

    // Compute bound of HLBVH node centroids, choose split dimension _dim_
    BBOX_3D centroidBounds;
    centroidBounds.Reset();

    for( int i = start; i < end; ++i )
    {
        SFVEC3F centroid = ( treeletRoots[i]->bounds.Min() + treeletRoots[i]->bounds.Max() ) * 0.5f;

        centroidBounds.Union( centroid );
    }

    const int dim = centroidBounds.MaxDimension();

    // FIXME: if this hits, what do we need to do?
    // Make sure the SAH split below does something... ?
    wxASSERT( centroidBounds.Max()[dim] != centroidBounds.Min()[dim] );

    // Allocate _BucketInfo_ for SAH partition buckets
    const int nBuckets = 12;

    BucketInfo buckets[nBuckets];

    for( int i = 0; i < nBuckets; ++i )
    {
        buckets[i].count = 0;
        buckets[i].bounds.Reset();
    }

    // Initialize _BucketInfo_ for HLBVH SAH partition buckets
    for( int i = start; i < end; ++i )
    {
        const float centroid = ( treeletRoots[i]->bounds.Min()[dim] +
                                 treeletRoots[i]->bounds.Max()[dim] ) * 0.5f;
        int b = nBuckets * ( ( centroid - centroidBounds.Min()[dim] )
                           / ( centroidBounds.Max()[dim] - centroidBounds.Min()[dim] ) );

        if( b == nBuckets )
            b = nBuckets - 1;

        wxASSERT( ( b >= 0 ) && ( b < nBuckets ) );

        buckets[b].count++;
        buckets[b].bounds.Union( treeletRoots[i]->bounds );
    }

    // Compute costs for splitting after each bucket
    float cost[nBuckets - 1];

    for( int i = 0; i < nBuckets - 1; ++i )
    {
        BBOX_3D b0, b1;
        b0.Reset();
        b1.Reset();

        int count0 = 0, count1 = 0;

        for( int j = 0; j <= i; ++j )
        {
            if( buckets[j].count )
            {
                count0 += buckets[j].count;
                b0.Union( buckets[j].bounds );
            }
        }

        for( int j = i + 1; j < nBuckets; ++j )
        {
            if( buckets[j].count )
            {
                count1 += buckets[j].count;
                b1.Union( buckets[j].bounds );
            }
        }

        cost[i] = .125f + ( count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea() ) /
                  bounds.SurfaceArea();
    }

    // Find bucket to split at that minimizes SAH metric
    float minCost = cost[0];
    int minCostSplitBucket = 0;

    for( int i = 1; i < nBuckets - 1; ++i )
    {
        if( cost[i] < minCost )
        {
            minCost = cost[i];
            minCostSplitBucket = i;
        }
    }

    // Split nodes and create interior HLBVH SAH node
    BVHBuildNode **pmid = std::partition( &treeletRoots[start], &treeletRoots[end - 1] + 1,
                                          HLBVH_SAH_Evaluator( minCostSplitBucket, nBuckets,
                                                               dim, centroidBounds ) );

    const int mid = pmid - &treeletRoots[0];

    wxASSERT( ( mid > start ) && ( mid < end ) );

    node->InitInterior( dim,
                        buildUpperSAH( treeletRoots, start, mid, totalNodes ),
                        buildUpperSAH( treeletRoots, mid,   end, totalNodes ) );

    return node;
}


int BVH_PBRT::flattenBVHTree( BVHBuildNode* node, uint32_t* offset )
{
    LinearBVHNode *linearNode = &m_nodes[*offset];

    linearNode->bounds = node->bounds;

    int myOffset = (*offset)++;

    if( node->nPrimitives > 0 )
    {
        wxASSERT( ( !node->children[0] ) && ( !node->children[1] ) );
        wxASSERT( node->nPrimitives < 65536 );

        linearNode->primitivesOffset = node->firstPrimOffset;
        linearNode->nPrimitives = node->nPrimitives;
    }
    else
    {
        // Create interior flattened BVH node
        linearNode->axis = node->splitAxis;
        linearNode->nPrimitives = 0;
        flattenBVHTree( node->children[0], offset );
        linearNode->secondChildOffset = flattenBVHTree( node->children[1], offset );
    }

    return myOffset;
}


#define MAX_TODOS 64


bool BVH_PBRT::Intersect( const RAY& aRay, HITINFO& aHitInfo ) const
{
    if( !m_nodes )
        return false;

    bool hit = false;

    // Follow ray through BVH nodes to find primitive intersections
    int todoOffset = 0, nodeNum = 0;
    int todo[MAX_TODOS];

    while( true )
    {
        const LinearBVHNode *node = &m_nodes[nodeNum];

        wxASSERT( todoOffset < MAX_TODOS );

        // Check ray against BVH node
        float hitBox = 0.0f;

        const bool hitted = node->bounds.Intersect( aRay, &hitBox );

        if( hitted && ( hitBox < aHitInfo.m_tHit ) )
        {
            if( node->nPrimitives > 0 )
            {
                // Intersect ray with primitives in leaf BVH node
                for( int i = 0; i < node->nPrimitives; ++i )
                {
                    if( m_primitives[node->primitivesOffset + i]->Intersect( aRay, aHitInfo ) )
                    {
                        aHitInfo.m_acc_node_info = nodeNum;
                        hit = true;
                    }
                }
            }
            else
            {
                // Put far BVH node on _todo_ stack, advance to near node
                if( aRay.m_dirIsNeg[node->axis] )
                {
                    todo[todoOffset++] = nodeNum + 1;
                    nodeNum = node->secondChildOffset;
                }
                else
                {
                    todo[todoOffset++] = node->secondChildOffset;
                    nodeNum = nodeNum + 1;
                }

                continue;
            }
        }

        if( todoOffset == 0 )
            break;

        nodeNum = todo[--todoOffset];
    }

    return hit;
}


/// @todo This may be optimized
bool BVH_PBRT::Intersect( const RAY& aRay, HITINFO& aHitInfo, unsigned int aAccNodeInfo ) const
{
    if( !m_nodes )
        return false;

    bool hit = false;

    // Follow ray through BVH nodes to find primitive intersections
    int todoOffset = 0, nodeNum = aAccNodeInfo;
    int todo[MAX_TODOS];

    while( true )
    {
        const LinearBVHNode *node = &m_nodes[nodeNum];

        wxASSERT( todoOffset < MAX_TODOS );

        // Check ray against BVH node
        float hitBox = 0.0f;

        const bool hitted = node->bounds.Intersect( aRay, &hitBox );

        if( hitted && ( hitBox < aHitInfo.m_tHit ) )
        {
            if( node->nPrimitives > 0 )
            {
                // Intersect ray with primitives in leaf BVH node
                for( int i = 0; i < node->nPrimitives; ++i )
                {
                    if( m_primitives[node->primitivesOffset + i]->Intersect( aRay, aHitInfo ) )
                    {
                        //aHitInfo.m_acc_node_info = nodeNum;
                        hit = true;
                    }
                }
            }
            else
            {
                // Put far BVH node on _todo_ stack, advance to near node
                if( aRay.m_dirIsNeg[node->axis] )
                {
                    todo[todoOffset++] = nodeNum + 1;
                    nodeNum = node->secondChildOffset;
                }
                else
                {
                    todo[todoOffset++] = node->secondChildOffset;
                    nodeNum = nodeNum + 1;
                }

                continue;
            }
        }

        if( todoOffset == 0 )
            break;

        nodeNum = todo[--todoOffset];
    }

    return hit;
}


bool BVH_PBRT::IntersectP( const RAY& aRay, float aMaxDistance ) const
{
    if( !m_nodes )
        return false;

    // Follow ray through BVH nodes to find primitive intersections
    int todoOffset = 0, nodeNum = 0;
    int todo[MAX_TODOS];

    while( true )
    {
        const LinearBVHNode* node = &m_nodes[nodeNum];

        wxASSERT( todoOffset < MAX_TODOS );

        // Check ray against BVH node
        float hitBox = 0.0f;

        const bool hitted = node->bounds.Intersect( aRay, &hitBox );

        if( hitted && ( hitBox < aMaxDistance ) )
        {
            if( node->nPrimitives > 0 )
            {
                // Intersect ray with primitives in leaf BVH node
                for( int i = 0; i < node->nPrimitives; ++i )
                {
                    const OBJECT_3D* obj = m_primitives[node->primitivesOffset + i];

                    if( obj->GetMaterial()->GetCastShadows()
                      && obj->IntersectP( aRay, aMaxDistance ) )
                        return true;
                }
            }
            else
            {
                // Put far BVH node on _todo_ stack, advance to near node
                if( aRay.m_dirIsNeg[node->axis] )
                {
                    todo[todoOffset++] = nodeNum + 1;
                    nodeNum = node->secondChildOffset;
                }
                else
                {
                    todo[todoOffset++] = node->secondChildOffset;
                    nodeNum = nodeNum + 1;
                }

                continue;
            }
        }

        if( todoOffset == 0 )
            break;

        nodeNum = todo[--todoOffset];
    }

    return false;
}
