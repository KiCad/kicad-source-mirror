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

#include <geometry/intersection.h>


BOOST_AUTO_TEST_SUITE( Intersection )


static INTERSECTION_CONTACT contactOf( const INTERSECTABLE_GEOM& aA, const INTERSECTABLE_GEOM& aB )
{
    std::vector<VECTOR2I> intersections;
    INTERSECTION_CONTACT  contact;

    std::visit( INTERSECTION_VISITOR( aB, intersections, contact ), aA );
    return contact;
}


BOOST_AUTO_TEST_CASE( CrossingGeometryReportsNoSpecialContact )
{
    INTERSECTION_CONTACT contact = contactOf( SEG( { 0, 0 }, { 100, 0 } ), SEG( { 50, -50 }, { 50, 50 } ) );

    BOOST_CHECK( !contact.m_Tangent );
    BOOST_CHECK( !contact.m_Overlapping );
}


BOOST_AUTO_TEST_CASE( CollinearSegmentsSharingAnExtentOverlap )
{
    INTERSECTION_CONTACT contact = contactOf( SEG( { 0, 0 }, { 100, 0 } ), SEG( { 20, 0 }, { 80, 0 } ) );

    // The point list gives one point here, same as a crossing.  Hence the flag.
    BOOST_CHECK( contact.m_Overlapping );
    BOOST_CHECK( !contact.m_Tangent );
}


// A shared carrier is not enough.  Only a shared extent counts.
BOOST_AUTO_TEST_CASE( SegmentsWithoutASharedExtentDoNotOverlap )
{
    const SEG source( { 0, 0 }, { 100, 0 } );

    BOOST_CHECK( !contactOf( source, SEG( { 100, 0 }, { 200, 0 } ) ).m_Overlapping );  // meeting at one end
    BOOST_CHECK( !contactOf( source, SEG( { 200, 0 }, { 300, 0 } ) ).m_Overlapping );  // elsewhere on the line
    BOOST_CHECK( !contactOf( source, SEG( { 0, 10 }, { 100, 10 } ) ).m_Overlapping );  // parallel, off the line
}


BOOST_AUTO_TEST_CASE( SegmentGrazingACircleIsTangent )
{
    const SEG source( { 0, 0 }, { 100, 0 } );

    BOOST_CHECK( contactOf( source, CIRCLE( { 50, 50 }, 50 ) ).m_Tangent );
    BOOST_CHECK( !contactOf( SEG( { 0, 25 }, { 100, 25 } ), CIRCLE( { 50, 50 }, 50 ) ).m_Tangent );

    // Carrier line grazes it.  Touch point is off the segment.
    BOOST_CHECK( !contactOf( source, CIRCLE( { 500, 50 }, 50 ) ).m_Tangent );
}


BOOST_AUTO_TEST_CASE( SegmentGrazingAnArcIsTangentOnlyWhereTheArcRuns )
{
    // Upper half of a circle above the segment.  Arc dips down to touch.
    const SHAPE_ARC touching( { 0, 100 }, { 50, 50 }, { 100, 100 }, 0 );
    const SHAPE_ARC awayFromTouch( { 100, 100 }, { 50, 150 }, { 0, 100 }, 0 );

    BOOST_CHECK( contactOf( SEG( { 0, 50 }, { 100, 50 } ), touching ).m_Tangent );
    BOOST_CHECK( !contactOf( SEG( { 0, 50 }, { 100, 50 } ), awayFromTouch ).m_Tangent );
}


BOOST_AUTO_TEST_CASE( TangentCirclesReportTangency )
{
    BOOST_CHECK( contactOf( CIRCLE( { 0, 0 }, 50 ), CIRCLE( { 100, 0 }, 50 ) ).m_Tangent );
    BOOST_CHECK( !contactOf( CIRCLE( { 0, 0 }, 50 ), CIRCLE( { 80, 0 }, 50 ) ).m_Tangent );
    BOOST_CHECK( !contactOf( CIRCLE( { 0, 0 }, 50 ), CIRCLE( { 500, 0 }, 50 ) ).m_Tangent );
}


BOOST_AUTO_TEST_CASE( ArcsOnACommonCircleOverlapOnlyWhenTheirSweepsDo )
{
    // All points sit on r=100 about the origin.  Rebuilt carriers match exactly.
    const SHAPE_ARC first( { 100, 0 }, { 80, 60 }, { 60, 80 }, 0 );
    const SHAPE_ARC overlapping( { 80, 60 }, { 60, 80 }, { 0, 100 }, 0 );
    const SHAPE_ARC touchingAtOneEnd( { 60, 80 }, { 0, 100 }, { -60, 80 }, 0 );

    BOOST_CHECK( contactOf( first, overlapping ).m_Overlapping );
    BOOST_CHECK( !contactOf( first, touchingAtOneEnd ).m_Overlapping );
}


BOOST_AUTO_TEST_CASE( ContactAccumulatesAcrossVisits )
{
    std::vector<VECTOR2I> intersections;
    INTERSECTION_CONTACT  contact;
    const SEG             source( { 0, 0 }, { 100, 0 } );

    std::visit( INTERSECTION_VISITOR( INTERSECTABLE_GEOM( SEG( { 50, -50 }, { 50, 50 } ) ), intersections, contact ),
                INTERSECTABLE_GEOM( source ) );
    BOOST_CHECK( !contact.m_Overlapping );

    std::visit( INTERSECTION_VISITOR( INTERSECTABLE_GEOM( SEG( { 20, 0 }, { 80, 0 } ) ), intersections, contact ),
                INTERSECTABLE_GEOM( source ) );
    BOOST_CHECK( contact.m_Overlapping );
}


BOOST_AUTO_TEST_SUITE_END()
