/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_drc_creepage_issue25821.cpp
 *
 * CU_SHAPE_SEGMENT::Paths( const CU_SHAPE_ARC& ) must return the lowest-weight candidate when the
 * closest point of the arc's circle lies outside the arc's angular span.
 */

#include <optional>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <drc/drc_creepage_utils.h>


namespace
{

constexpr double MAX_WEIGHT = 1e9;


std::optional<PATH_CONNECTION> lowestWeightCandidate( const CU_SHAPE_SEGMENT& aSegment, const CU_SHAPE_ARC& aArc )
{
    const double maxSquaredWeight = MAX_WEIGHT * MAX_WEIGHT;
    const double segmentHalfWidth = aSegment.GetWidth() / 2;
    const double arcHalfWidth = aArc.GetWidth() / 2;

    CU_SHAPE_CIRCLE atArcStart( aArc.GetStartPoint(), arcHalfWidth );
    CU_SHAPE_CIRCLE atArcEnd( aArc.GetEndPoint(), arcHalfWidth );
    CU_SHAPE_CIRCLE atSegmentStart( aSegment.GetStart(), segmentHalfWidth );
    CU_SHAPE_CIRCLE atSegmentEnd( aSegment.GetEnd(), segmentHalfWidth );

    std::vector<std::vector<PATH_CONNECTION>> candidateSets;

    candidateSets.push_back( aSegment.Paths( atArcStart, MAX_WEIGHT, maxSquaredWeight ) );
    candidateSets.push_back( aSegment.Paths( atArcEnd, MAX_WEIGHT, maxSquaredWeight ) );
    candidateSets.push_back( atSegmentStart.Paths( aArc, MAX_WEIGHT, maxSquaredWeight ) );
    candidateSets.push_back( atSegmentEnd.Paths( aArc, MAX_WEIGHT, maxSquaredWeight ) );

    std::optional<PATH_CONNECTION> best;

    for( const std::vector<PATH_CONNECTION>& candidateSet : candidateSets )
    {
        for( const PATH_CONNECTION& candidate : candidateSet )
        {
            if( !best || best->weight > candidate.weight )
                best = candidate;
        }
    }

    return best;
}

} // namespace


BOOST_AUTO_TEST_CASE( CreepageSegmentToArcFallbackPicksLowestWeight )
{
    const double radius = 1000000.0;

    // The segment sits off the arc's angular span, so the fallback runs instead of returning early.
    CU_SHAPE_ARC arc( VECTOR2I( 0, 0 ), radius, EDA_ANGLE( 0, DEGREES_T ), EDA_ANGLE( 90, DEGREES_T ),
                      VECTOR2D( radius, 0 ), VECTOR2D( 0, radius ) );

    CU_SHAPE_SEGMENT segment( VECTOR2I( -3000000, -2600000 ), VECTOR2I( -1200000, -2600000 ), 200000 );

    std::vector<PATH_CONNECTION> toFullCircle = segment.Paths(
            CU_SHAPE_CIRCLE( arc.GetPos(), radius + arc.GetWidth() / 2 ), MAX_WEIGHT, MAX_WEIGHT * MAX_WEIGHT );

    BOOST_REQUIRE( !toFullCircle.empty() );
    BOOST_REQUIRE( arc.AngleBetweenStartAndEnd( toFullCircle[0].a1 ) >= arc.GetEndAngle() );

    std::optional<PATH_CONNECTION> expected = lowestWeightCandidate( segment, arc );
    BOOST_REQUIRE( expected.has_value() );

    std::vector<PATH_CONNECTION> paths = segment.Paths( arc, MAX_WEIGHT, MAX_WEIGHT * MAX_WEIGHT );

    BOOST_REQUIRE_EQUAL( paths.size(), 1 );
    BOOST_CHECK_CLOSE( paths[0].weight, expected->weight, 1e-6 );
    BOOST_CHECK( paths[0].a1 == expected->a1 );
    BOOST_CHECK( paths[0].a2 == expected->a2 );
}
