/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef POLY_CONTAINMENT_INDEX_H
#define POLY_CONTAINMENT_INDEX_H

#include <climits>
#include <cstdint>
#include <vector>

#include <geometry/rtree/packed_rtree.h>
#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <math/util.h>
#include <math/vector2d.h>

/**
 * Spatial index for efficient point-in-polygon containment testing.
 *
 * Standard SHAPE_LINE_CHAIN::PointInside() is O(V) per query, ray-casting against every edge.
 * For large polygons with many containment queries (e.g. testing thousands of via positions
 * against zone fills with tens of thousands of vertices), this becomes a bottleneck.
 *
 * This class builds an R-tree of polygon edges so containment queries become O(log V + K)
 * where K is the number of edges the horizontal ray actually crosses. The ray-crossing
 * algorithm matches SHAPE_LINE_CHAIN_BASE::PointInside() exactly, including the accuracy
 * semantics where aAccuracy > 1 falls back to edge-distance testing.
 */
class POLY_CONTAINMENT_INDEX
{
public:
    POLY_CONTAINMENT_INDEX() = default;

    /**
     * Build the spatial index from a SHAPE_POLY_SET's outlines.
     *
     * Indexes every edge of every outline in the polygon set. Must be called before Contains().
     * Only indexes outlines, not holes (zone fills are fractured and have no holes).
     */
    void Build( const SHAPE_POLY_SET& aPolySet )
    {
        m_outlineCount = aPolySet.OutlineCount();

        KIRTREE::PACKED_RTREE<intptr_t, int, 2>::Builder builder;

        for( int outlineIdx = 0; outlineIdx < m_outlineCount; outlineIdx++ )
        {
            const SHAPE_LINE_CHAIN& outline = aPolySet.COutline( outlineIdx );
            int                     ptCount = outline.PointCount();

            if( ptCount < 3 )
                continue;

            for( int j = 0; j < ptCount; j++ )
            {
                const VECTOR2I& p1 = outline.CPoint( j );
                const VECTOR2I& p2 = outline.CPoint( ( j + 1 ) % ptCount );

                intptr_t idx = static_cast<intptr_t>( m_segments.size() );
                m_segments.push_back( { p1, p2, outlineIdx } );

                int min[2] = { std::min( p1.x, p2.x ), std::min( p1.y, p2.y ) };
                int max[2] = { std::max( p1.x, p2.x ), std::max( p1.y, p2.y ) };
                builder.Add( min, max, idx );
            }
        }

        m_tree = builder.Build();
    }

    /**
     * Test whether a point is inside the indexed polygon set.
     *
     * Uses the same ray-crossing algorithm as SHAPE_LINE_CHAIN_BASE::PointInside(). When
     * aAccuracy > 1, also checks if the point is within aAccuracy distance of any edge
     * (matching the PointOnEdge fallback behavior).
     *
     * @param aPt       The point to test.
     * @param aAccuracy Distance threshold for edge-proximity fallback. Values <= 1 skip
     *                  the edge test for performance.
     * @return true if the point is inside any outline or (when aAccuracy > 1) within
     *         aAccuracy distance of any edge.
     */
    bool Contains( const VECTOR2I& aPt, int aAccuracy = 0 ) const
    {
        if( m_segments.empty() )
            return false;

        // Most polygon sets have very few outlines, so use a stack buffer to avoid
        // per-query heap allocation.
        int  crossingsStack[8] = {};
        int* crossings = crossingsStack;
        std::vector<int> crossingsHeap;

        if( m_outlineCount > 8 )
        {
            crossingsHeap.resize( m_outlineCount, 0 );
            crossings = crossingsHeap.data();
        }

        // Only segments whose X extent reaches past aPt.x can produce a rightward ray crossing.
        int searchMin[2] = { aPt.x, aPt.y };
        int searchMax[2] = { INT_MAX, aPt.y };

        auto rayCrossVisitor = [&]( intptr_t idx ) -> bool
        {
            const EDGE& seg = m_segments[idx];
            const VECTOR2I& p1 = seg.p1;
            const VECTOR2I& p2 = seg.p2;

            if( ( p1.y >= aPt.y ) == ( p2.y >= aPt.y ) )
                return true;

            const VECTOR2I diff = p2 - p1;
            const int d = rescale( diff.x, ( aPt.y - p1.y ), diff.y );

            if( aPt.x - p1.x < d )
                crossings[seg.outlineIdx]++;

            return true;
        };

        m_tree.Search( searchMin, searchMax, rayCrossVisitor );

        for( int i = 0; i < m_outlineCount; i++ )
        {
            if( crossings[i] & 1 )
                return true;
        }

        if( aAccuracy > 1 )
        {
            int edgeMin[2] = { aPt.x - aAccuracy, aPt.y - aAccuracy };
            int edgeMax[2] = { aPt.x + aAccuracy, aPt.y + aAccuracy };
            SEG::ecoord accuracySq = SEG::Square( aAccuracy );
            bool onEdge = false;

            auto edgeDistVisitor = [&]( intptr_t idx ) -> bool
            {
                const EDGE& seg = m_segments[idx];
                SEG s( seg.p1, seg.p2 );

                if( s.SquaredDistance( aPt ) <= accuracySq )
                {
                    onEdge = true;
                    return false;
                }

                return true;
            };

            m_tree.Search( edgeMin, edgeMax, edgeDistVisitor );

            return onEdge;
        }

        return false;
    }

private:
    struct EDGE
    {
        VECTOR2I p1;
        VECTOR2I p2;
        int      outlineIdx;
    };

    std::vector<EDGE>                      m_segments;
    KIRTREE::PACKED_RTREE<intptr_t, int, 2> m_tree;
    int                                    m_outlineCount = 0;
};

#endif // POLY_CONTAINMENT_INDEX_H
