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

#ifndef POLY_YSTRIPES_INDEX_H
#define POLY_YSTRIPES_INDEX_H

#include <algorithm>
#include <cmath>
#include <climits>
#include <cstdint>
#include <vector>

#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <math/util.h>
#include <math/vector2d.h>

/**
 * Y-stripe spatial index for efficient point-in-polygon containment testing.
 *
 * Inspired by the YStripes indexing strategy from the TG geometry library
 * (https://github.com/tidwall/tg, MIT license, Copyright (c) 2023 Joshua J Baker).
 *
 * Divides the polygon set's Y extent into uniform horizontal stripes and buckets each
 * non-horizontal edge into every stripe its Y range overlaps. Containment queries ray-cast
 * only against edges in the queried point's stripe, reducing work from O(V) to O(V/sqrt(V))
 * on average.
 *
 * The ray-crossing algorithm matches SHAPE_LINE_CHAIN_BASE::PointInside() exactly, including
 * the accuracy semantics where aAccuracy > 1 falls back to edge-distance testing.
 */
class POLY_YSTRIPES_INDEX
{
public:
    POLY_YSTRIPES_INDEX() = default;

    /**
     * Build the spatial index from a SHAPE_POLY_SET's outlines and holes.
     *
     * Indexes every non-horizontal edge of every outline and hole in the polygon set. Hole
     * edges are indexed with the same outlineIdx as their parent outline so that ray-crossing
     * counts automatically produce even totals for points inside holes. Must be called before
     * Contains().
     */
    void Build( const SHAPE_POLY_SET& aPolySet )
    {
        m_outlineCount = aPolySet.OutlineCount();
        m_edges.clear();
        m_stripes.clear();

        int totalEdges = 0;

        for( int outlineIdx = 0; outlineIdx < m_outlineCount; outlineIdx++ )
        {
            totalEdges += aPolySet.COutline( outlineIdx ).PointCount();

            for( int holeIdx = 0; holeIdx < aPolySet.HoleCount( outlineIdx ); holeIdx++ )
                totalEdges += aPolySet.CHole( outlineIdx, holeIdx ).PointCount();
        }

        if( totalEdges == 0 )
            return;

        m_edges.reserve( totalEdges );

        int64_t yMin = INT_MAX;
        int64_t yMax = INT_MIN;

        for( int outlineIdx = 0; outlineIdx < m_outlineCount; outlineIdx++ )
        {
            const SHAPE_LINE_CHAIN& outline = aPolySet.COutline( outlineIdx );
            int                     ptCount = outline.PointCount();

            if( ptCount >= 3 )
            {
                for( int j = 0; j < ptCount; j++ )
                {
                    const VECTOR2I& p1 = outline.CPoint( j );
                    const VECTOR2I& p2 = outline.CPoint( ( j + 1 ) % ptCount );

                    if( p1.y == p2.y )
                        continue;

                    m_edges.push_back( { p1, p2, outlineIdx, false } );

                    yMin = std::min( yMin, static_cast<int64_t>( std::min( p1.y, p2.y ) ) );
                    yMax = std::max( yMax, static_cast<int64_t>( std::max( p1.y, p2.y ) ) );
                }
            }

            for( int holeIdx = 0; holeIdx < aPolySet.HoleCount( outlineIdx ); holeIdx++ )
            {
                const SHAPE_LINE_CHAIN& hole = aPolySet.CHole( outlineIdx, holeIdx );
                int                     holePtCount = hole.PointCount();

                if( holePtCount < 3 )
                    continue;

                for( int j = 0; j < holePtCount; j++ )
                {
                    const VECTOR2I& p1 = hole.CPoint( j );
                    const VECTOR2I& p2 = hole.CPoint( ( j + 1 ) % holePtCount );

                    if( p1.y == p2.y )
                        continue;

                    m_edges.push_back( { p1, p2, outlineIdx, true } );

                    yMin = std::min( yMin, static_cast<int64_t>( std::min( p1.y, p2.y ) ) );
                    yMax = std::max( yMax, static_cast<int64_t>( std::max( p1.y, p2.y ) ) );
                }
            }
        }

        if( m_edges.empty() )
            return;

        int stripeCount = static_cast<int>( std::sqrt( static_cast<double>( m_edges.size() ) ) );
        stripeCount = std::max( 1, std::min( stripeCount, 65536 ) );

        m_yMin = yMin;
        m_yMax = yMax;
        int64_t yRange = yMax - yMin;

        // Guard against degenerate case where all edges share the same Y coordinate
        if( yRange == 0 )
        {
            stripeCount = 1;
            m_stripeHeight = 1;
        }
        else
        {
            m_stripeHeight = ( yRange + stripeCount - 1 ) / stripeCount;
        }

        m_stripeCount = stripeCount;
        m_stripes.resize( stripeCount );

        for( size_t i = 0; i < m_edges.size(); i++ )
        {
            const EDGE& e = m_edges[i];
            int64_t     eYMin = std::min( static_cast<int64_t>( e.p1.y ),
                                          static_cast<int64_t>( e.p2.y ) );
            int64_t     eYMax = std::max( static_cast<int64_t>( e.p1.y ),
                                          static_cast<int64_t>( e.p2.y ) );

            int sMin = yToStripe( eYMin );
            int sMax = yToStripe( eYMax );

            for( int s = sMin; s <= sMax; s++ )
                m_stripes[s].push_back( static_cast<int>( i ) );
        }
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
        if( m_edges.empty() )
            return false;

        int stripe = yToStripe( static_cast<int64_t>( aPt.y ) );

        if( stripe >= 0 && stripe < m_stripeCount )
        {
            int  crossingsStack[8] = {};
            int* crossings = crossingsStack;
            std::vector<int> crossingsHeap;

            if( m_outlineCount > 8 )
            {
                crossingsHeap.resize( m_outlineCount, 0 );
                crossings = crossingsHeap.data();
            }

            for( int idx : m_stripes[stripe] )
            {
                const EDGE&     seg = m_edges[idx];
                const VECTOR2I& p1 = seg.p1;
                const VECTOR2I& p2 = seg.p2;

                if( ( p1.y >= aPt.y ) == ( p2.y >= aPt.y ) )
                    continue;

                const VECTOR2I diff = p2 - p1;
                const int d = rescale( diff.x, ( aPt.y - p1.y ), diff.y );

                if( aPt.x - p1.x < d )
                    crossings[seg.outlineIdx]++;
            }

            for( int i = 0; i < m_outlineCount; i++ )
            {
                if( crossings[i] & 1 )
                    return true;
            }
        }

        if( aAccuracy > 1 )
        {
            int sMin = yToStripe( static_cast<int64_t>( aPt.y ) - aAccuracy );
            int sMax = yToStripe( static_cast<int64_t>( aPt.y ) + aAccuracy );
            SEG::ecoord accuracySq = SEG::Square( aAccuracy );

            for( int s = sMin; s <= sMax; s++ )
            {
                if( s < 0 || s >= m_stripeCount )
                    continue;

                for( int idx : m_stripes[s] )
                {
                    const EDGE& seg = m_edges[idx];

                    if( seg.isHole )
                        continue;

                    SEG edge( seg.p1, seg.p2 );

                    if( edge.SquaredDistance( aPt ) <= accuracySq )
                        return true;
                }
            }
        }

        return false;
    }

private:
    int yToStripe( int64_t aY ) const
    {
        int64_t offset = aY - m_yMin;
        int     s = static_cast<int>( offset / m_stripeHeight );
        return std::max( 0, std::min( s, m_stripeCount - 1 ) );
    }

    struct EDGE
    {
        VECTOR2I p1;
        VECTOR2I p2;
        int      outlineIdx;
        bool     isHole;
    };

    std::vector<EDGE>             m_edges;
    std::vector<std::vector<int>> m_stripes;
    int64_t                       m_yMin = 0;
    int64_t                       m_yMax = 0;
    int64_t                       m_stripeHeight = 1;
    int                           m_stripeCount = 0;
    int                           m_outlineCount = 0;
};

#endif // POLY_YSTRIPES_INDEX_H
