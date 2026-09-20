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

#include <boost/test/unit_test.hpp>

#include <geometry/segment_index.h>

#include <algorithm>
#include <limits>
#include <vector>


BOOST_AUTO_TEST_SUITE( SegmentIndex )


static std::vector<int> candidates( const SEGMENT_INDEX& aIndex, const SEG& aQuery, int aPadding )
{
    std::vector<int> result;
    auto             visitor = [&]( int aItem )
    {
        result.push_back( aItem );
        return true;
    };

    aIndex.VisitCandidates( aQuery, aPadding, visitor );
    std::sort( result.begin(), result.end() );
    return result;
}


static bool endpointBoundsOverlap( const SEG& aItem, const SEG& aQuery, int aPadding )
{
    const int64_t queryMinX = static_cast<int64_t>( std::min( aQuery.A.x, aQuery.B.x ) ) - aPadding;
    const int64_t queryMinY = static_cast<int64_t>( std::min( aQuery.A.y, aQuery.B.y ) ) - aPadding;
    const int64_t queryMaxX = static_cast<int64_t>( std::max( aQuery.A.x, aQuery.B.x ) ) + aPadding;
    const int64_t queryMaxY = static_cast<int64_t>( std::max( aQuery.A.y, aQuery.B.y ) ) + aPadding;
    const int64_t itemMinX = std::min( aItem.A.x, aItem.B.x );
    const int64_t itemMinY = std::min( aItem.A.y, aItem.B.y );
    const int64_t itemMaxX = std::max( aItem.A.x, aItem.B.x );
    const int64_t itemMaxY = std::max( aItem.A.y, aItem.B.y );

    return itemMaxX >= queryMinX && itemMinX <= queryMaxX && itemMaxY >= queryMinY && itemMinY <= queryMaxY;
}


BOOST_AUTO_TEST_CASE( EmptyAndDegenerateSegments )
{
    SEGMENT_INDEX empty( {} );
    BOOST_CHECK( candidates( empty, SEG( { 0, 0 }, { 1, 1 } ), 0 ).empty() );

    std::vector<SEG> segments = { SEG( { 5, 5 }, { 5, 5 } ), SEG( { 5, 5 }, { 5, 5 } ), SEG( { 20, 20 }, { 30, 30 } ) };
    SEGMENT_INDEX    index( segments );

    BOOST_CHECK_EQUAL( index.size(), 3 );
    BOOST_CHECK( candidates( index, SEG( { 5, 5 }, { 5, 5 } ), 0 ) == std::vector<int>( { 0, 1 } ) );
    BOOST_CHECK( index.Segment( 2 ) == SEG( { 20, 20 }, { 30, 30 } ) );
}


BOOST_AUTO_TEST_CASE( SaturatesPaddedQueryBounds )
{
    constexpr int min = std::numeric_limits<int>::min();
    constexpr int max = std::numeric_limits<int>::max();

    std::vector<SEG> segments = { SEG( { min, min }, { min + 1, min + 1 } ), SEG( { max - 1, max - 1 }, { max, max } ),
                                  SEG( { -1, -1 }, { 1, 1 } ) };
    SEGMENT_INDEX    index( segments );

    BOOST_CHECK( candidates( index, SEG( { min + 1, min + 1 }, { min + 1, min + 1 } ), max )
                 == std::vector<int>( { 0, 2 } ) );
    BOOST_CHECK( candidates( index, SEG( { max - 1, max - 1 }, { max - 1, max - 1 } ), max )
                 == std::vector<int>( { 1, 2 } ) );

    const std::vector<SEG> queries = { SEG( { min, min }, { min, min } ),
                                       SEG( { min + 1, max - 1 }, { min + 1, max - 1 } ), SEG( { -1, -1 }, { 1, 1 } ),
                                       SEG( { max, max }, { max, max } ) };

    for( int padding : { 0, 1, max } )
    {
        for( const SEG& query : queries )
        {
            std::vector<int> expected;

            for( size_t i = 0; i < segments.size(); ++i )
            {
                if( endpointBoundsOverlap( segments[i], query, padding ) )
                    expected.push_back( static_cast<int>( i ) );
            }

            BOOST_CHECK( candidates( index, query, padding ) == expected );
        }
    }
}


BOOST_AUTO_TEST_CASE( NegativePaddingMatchesZero )
{
    constexpr int min = std::numeric_limits<int>::min();

    SEGMENT_INDEX index( { SEG( { 0, 0 }, { 10, 0 } ), SEG( { 100, 0 }, { 110, 0 } ) } );
    const SEG     query( { 5, 0 }, { 5, 0 } );

    // Unclamped negative padding inverts the query rect, which matches nothing at all
    BOOST_CHECK( candidates( index, query, -1 ) == std::vector<int>( { 0 } ) );
    BOOST_CHECK( candidates( index, query, min ) == std::vector<int>( { 0 } ) );
}


BOOST_AUTO_TEST_SUITE_END()
