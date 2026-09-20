/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SEGMENT_INDEX_H
#define SEGMENT_INDEX_H

#include <geometry/rtree/packed_rtree.h>
#include <geometry/seg.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>


/**
 * Immutable owning spatial snapshot of straight segments.
 *
 * Candidate order is unspecified.  Queries conservatively include every segment whose endpoint
 * bounds overlap the padded query bounds.  The owned snapshot is independent of its source and is
 * safe for concurrent const queries.  This is an internal geometry helper; its API is not stable.
 */
class SEGMENT_INDEX
{
public:
    explicit SEGMENT_INDEX( std::vector<SEG> aSegments ) :
            m_segments( std::move( aSegments ) )
    {
        TREE::Builder builder;
        builder.Reserve( m_segments.size() );

        for( size_t i = 0; i < m_segments.size(); ++i )
        {
            const SEG& segment = m_segments[i];
            const int  min[2] = { std::min( segment.A.x, segment.B.x ), std::min( segment.A.y, segment.B.y ) };
            const int  max[2] = { std::max( segment.A.x, segment.B.x ), std::max( segment.A.y, segment.B.y ) };
            builder.Add( min, max, static_cast<int>( i ) );
        }

        m_tree = builder.Build();
    }

    SEGMENT_INDEX( SEGMENT_INDEX&& ) noexcept = default;
    SEGMENT_INDEX& operator=( SEGMENT_INDEX&& ) noexcept = default;
    SEGMENT_INDEX( const SEGMENT_INDEX& ) = delete;
    SEGMENT_INDEX& operator=( const SEGMENT_INDEX& ) = delete;

    size_t size() const { return m_segments.size(); }

    const SEG& Segment( int aIndex ) const { return m_segments[aIndex]; }

    /**
     * Visit candidates overlapping aQuery's endpoint bounds expanded by aPadding.
     *
     * Negative padding is treated as zero so the query never shrinks below the query bounds and
     * stays conservative.  The visitor receives the original segment index and returns true to
     * continue or false to stop.
     */
    template <typename VISITOR>
    void VisitCandidates( const SEG& aQuery, int aPadding, VISITOR&& aVisitor ) const
    {
        const int64_t padding = std::max( aPadding, 0 );
        const int64_t minX = static_cast<int64_t>( std::min( aQuery.A.x, aQuery.B.x ) ) - padding;
        const int64_t minY = static_cast<int64_t>( std::min( aQuery.A.y, aQuery.B.y ) ) - padding;
        const int64_t maxX = static_cast<int64_t>( std::max( aQuery.A.x, aQuery.B.x ) ) + padding;
        const int64_t maxY = static_cast<int64_t>( std::max( aQuery.A.y, aQuery.B.y ) ) + padding;
        const int     min[2] = { clampCoordinate( minX ), clampCoordinate( minY ) };
        const int     max[2] = { clampCoordinate( maxX ), clampCoordinate( maxY ) };
        m_tree.Search( min, max, aVisitor );
    }

private:
    using TREE = KIRTREE::PACKED_RTREE<int, int, 2>;

    static int clampCoordinate( int64_t aValue )
    {
        return static_cast<int>( std::clamp( aValue, static_cast<int64_t>( std::numeric_limits<int>::min() ),
                                             static_cast<int64_t>( std::numeric_limits<int>::max() ) ) );
    }

    std::vector<SEG> m_segments;
    TREE             m_tree;
};

#endif // SEGMENT_INDEX_H
