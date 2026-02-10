/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <boost/test/unit_test.hpp>
#include <sstream>
#include <pcb_io/pads/pads_parser.h>

using namespace PADS_IO;


BOOST_AUTO_TEST_SUITE( PadsArcStructures )


BOOST_AUTO_TEST_CASE( Arc_DefaultConstruction )
{
    ARC arc{};
    BOOST_CHECK_EQUAL( arc.cx, 0.0 );
    BOOST_CHECK_EQUAL( arc.cy, 0.0 );
    BOOST_CHECK_EQUAL( arc.radius, 0.0 );
    BOOST_CHECK_EQUAL( arc.start_angle, 0.0 );
    BOOST_CHECK_EQUAL( arc.delta_angle, 0.0 );
}


BOOST_AUTO_TEST_CASE( Arc_AggregateInitialization )
{
    ARC arc{ 100.0, 200.0, 50.0, 45.0, 90.0 };
    BOOST_CHECK_EQUAL( arc.cx, 100.0 );
    BOOST_CHECK_EQUAL( arc.cy, 200.0 );
    BOOST_CHECK_EQUAL( arc.radius, 50.0 );
    BOOST_CHECK_EQUAL( arc.start_angle, 45.0 );
    BOOST_CHECK_EQUAL( arc.delta_angle, 90.0 );
}


BOOST_AUTO_TEST_CASE( ArcPoint_DefaultConstruction )
{
    ARC_POINT pt;
    BOOST_CHECK_EQUAL( pt.x, 0.0 );
    BOOST_CHECK_EQUAL( pt.y, 0.0 );
    BOOST_CHECK_EQUAL( pt.is_arc, false );
    BOOST_CHECK_EQUAL( pt.arc.cx, 0.0 );
    BOOST_CHECK_EQUAL( pt.arc.cy, 0.0 );
}


BOOST_AUTO_TEST_CASE( ArcPoint_LineConstruction )
{
    ARC_POINT pt( 100.0, 200.0 );
    BOOST_CHECK_EQUAL( pt.x, 100.0 );
    BOOST_CHECK_EQUAL( pt.y, 200.0 );
    BOOST_CHECK_EQUAL( pt.is_arc, false );
}


BOOST_AUTO_TEST_CASE( ArcPoint_ArcConstruction )
{
    ARC arc{ 50.0, 50.0, 25.0, 0.0, 180.0 };
    ARC_POINT pt( 75.0, 50.0, arc );

    BOOST_CHECK_EQUAL( pt.x, 75.0 );
    BOOST_CHECK_EQUAL( pt.y, 50.0 );
    BOOST_CHECK_EQUAL( pt.is_arc, true );
    BOOST_CHECK_EQUAL( pt.arc.cx, 50.0 );
    BOOST_CHECK_EQUAL( pt.arc.cy, 50.0 );
    BOOST_CHECK_EQUAL( pt.arc.radius, 25.0 );
    BOOST_CHECK_EQUAL( pt.arc.start_angle, 0.0 );
    BOOST_CHECK_EQUAL( pt.arc.delta_angle, 180.0 );
}


BOOST_AUTO_TEST_CASE( ArcPoint_EmplaceBack_Line )
{
    std::vector<ARC_POINT> points;
    points.emplace_back( 10.0, 20.0 );
    points.emplace_back( 30.0, 40.0 );

    BOOST_REQUIRE_EQUAL( points.size(), 2 );
    BOOST_CHECK_EQUAL( points[0].x, 10.0 );
    BOOST_CHECK_EQUAL( points[0].y, 20.0 );
    BOOST_CHECK_EQUAL( points[0].is_arc, false );
    BOOST_CHECK_EQUAL( points[1].x, 30.0 );
    BOOST_CHECK_EQUAL( points[1].y, 40.0 );
    BOOST_CHECK_EQUAL( points[1].is_arc, false );
}


BOOST_AUTO_TEST_CASE( ArcPoint_EmplaceBack_Arc )
{
    std::vector<ARC_POINT> points;
    points.emplace_back( 0.0, 0.0 );
    points.emplace_back( 100.0, 0.0, ARC{ 50.0, 0.0, 50.0, 180.0, -180.0 } );

    BOOST_REQUIRE_EQUAL( points.size(), 2 );
    BOOST_CHECK_EQUAL( points[1].is_arc, true );
    BOOST_CHECK_EQUAL( points[1].arc.cx, 50.0 );
    BOOST_CHECK_EQUAL( points[1].arc.delta_angle, -180.0 );
}


BOOST_AUTO_TEST_CASE( Arc_FullCircle )
{
    // A 360-degree arc represents a full circle
    ARC arc{ 0.0, 0.0, 100.0, 0.0, 360.0 };
    BOOST_CHECK_EQUAL( arc.delta_angle, 360.0 );
}


BOOST_AUTO_TEST_CASE( Arc_NegativeDelta )
{
    // Negative delta angle means clockwise direction
    ARC arc{ 0.0, 0.0, 50.0, 90.0, -90.0 };
    BOOST_CHECK_EQUAL( arc.start_angle, 90.0 );
    BOOST_CHECK_EQUAL( arc.delta_angle, -90.0 );
}


BOOST_AUTO_TEST_CASE( Polyline_DefaultConstruction )
{
    POLYLINE polyline;
    polyline.layer = 0;
    polyline.width = 10.0;
    polyline.closed = true;

    BOOST_CHECK( polyline.points.empty() );
    BOOST_CHECK_EQUAL( polyline.closed, true );
}


BOOST_AUTO_TEST_CASE( Polyline_WithPoints )
{
    POLYLINE polyline;
    polyline.layer = 1;
    polyline.width = 5.0;
    polyline.closed = false;
    polyline.points.emplace_back( 0.0, 0.0 );
    polyline.points.emplace_back( 100.0, 0.0 );
    polyline.points.emplace_back( 100.0, 100.0 );

    BOOST_REQUIRE_EQUAL( polyline.points.size(), 3 );
    BOOST_CHECK_EQUAL( polyline.closed, false );
    BOOST_CHECK_EQUAL( polyline.points[2].x, 100.0 );
    BOOST_CHECK_EQUAL( polyline.points[2].y, 100.0 );
}


BOOST_AUTO_TEST_CASE( Track_UsesArcPoint )
{
    TRACK track;
    track.layer = 1;
    track.width = 10.0;
    track.points.emplace_back( 0.0, 0.0 );
    track.points.emplace_back( 50.0, 50.0 );
    track.points.emplace_back( 100.0, 50.0, ARC{ 75.0, 50.0, 25.0, 180.0, -180.0 } );

    BOOST_REQUIRE_EQUAL( track.points.size(), 3 );
    BOOST_CHECK_EQUAL( track.points[0].is_arc, false );
    BOOST_CHECK_EQUAL( track.points[1].is_arc, false );
    BOOST_CHECK_EQUAL( track.points[2].is_arc, true );
    BOOST_CHECK_EQUAL( track.points[2].arc.radius, 25.0 );
}


BOOST_AUTO_TEST_CASE( Pour_UsesArcPoint )
{
    POUR pour;
    pour.net_name = "GND";
    pour.layer = 1;
    pour.priority = 0;
    pour.width = 10.0;
    pour.points.emplace_back( 0.0, 0.0 );
    pour.points.emplace_back( 100.0, 0.0 );
    pour.points.emplace_back( 100.0, 100.0 );
    pour.points.emplace_back( 0.0, 100.0 );

    BOOST_REQUIRE_EQUAL( pour.points.size(), 4 );
    BOOST_CHECK_EQUAL( pour.net_name, "GND" );
}


BOOST_AUTO_TEST_CASE( DecalItem_UsesArcPoint )
{
    DECAL_ITEM item;
    item.type = "CLOSED";
    item.layer = 26;  // Silkscreen Top
    item.width = 8.0;
    item.points.emplace_back( 0.0, 0.0 );
    item.points.emplace_back( 50.0, 0.0 );
    item.points.emplace_back( 50.0, 50.0 );
    item.points.emplace_back( 0.0, 50.0 );

    BOOST_REQUIRE_EQUAL( item.points.size(), 4 );
    BOOST_CHECK_EQUAL( item.type, "CLOSED" );
}


BOOST_AUTO_TEST_CASE( Arc_FromBoundingBox )
{
    // PADS arc format: XLOC YLOC ISA IDA CX-R CY-R CX+R CY+R
    // Example: 0 0 900 -900 -50 -50 50 50
    // This represents arc from (0,0), center at (0,0), radius 50, 90deg start, -90deg delta
    double bboxMinX = -50.0, bboxMinY = -50.0;
    double bboxMaxX = 50.0, bboxMaxY = 50.0;
    int startAngleTenths = 900;   // 90.0 degrees
    int deltaAngleTenths = -900;  // -90.0 degrees (clockwise)

    double cx = ( bboxMinX + bboxMaxX ) / 2.0;
    double cy = ( bboxMinY + bboxMaxY ) / 2.0;
    double radius = ( bboxMaxX - bboxMinX ) / 2.0;
    double startAngle = startAngleTenths / 10.0;
    double deltaAngle = deltaAngleTenths / 10.0;

    BOOST_CHECK_CLOSE( cx, 0.0, 0.001 );
    BOOST_CHECK_CLOSE( cy, 0.0, 0.001 );
    BOOST_CHECK_CLOSE( radius, 50.0, 0.001 );
    BOOST_CHECK_CLOSE( startAngle, 90.0, 0.001 );
    BOOST_CHECK_CLOSE( deltaAngle, -90.0, 0.001 );
}


BOOST_AUTO_TEST_CASE( Arc_FromBoundingBox_OffCenter )
{
    // Arc with center at (100, 200), radius 25
    // Bounding box: (75, 175) to (125, 225)
    double bboxMinX = 75.0, bboxMinY = 175.0;
    double bboxMaxX = 125.0, bboxMaxY = 225.0;

    double cx = ( bboxMinX + bboxMaxX ) / 2.0;
    double cy = ( bboxMinY + bboxMaxY ) / 2.0;
    double radius = ( bboxMaxX - bboxMinX ) / 2.0;

    BOOST_CHECK_CLOSE( cx, 100.0, 0.001 );
    BOOST_CHECK_CLOSE( cy, 200.0, 0.001 );
    BOOST_CHECK_CLOSE( radius, 25.0, 0.001 );
}


BOOST_AUTO_TEST_CASE( ArcPoint_ParseFromStream )
{
    // Simulate parsing a PADS arc line: "0 0 900 -900 -50 -50 50 50"
    std::string line = "0 0 900 -900 -50 -50 50 50";
    std::istringstream iss( line );

    double dx, dy;
    int startAngleTenths, deltaAngleTenths;
    double bboxMinX, bboxMinY, bboxMaxX, bboxMaxY;

    iss >> dx >> dy;
    bool hasArc = static_cast<bool>( iss >> startAngleTenths >> deltaAngleTenths
                                          >> bboxMinX >> bboxMinY >> bboxMaxX >> bboxMaxY );

    BOOST_CHECK( hasArc );
    BOOST_CHECK_EQUAL( startAngleTenths, 900 );
    BOOST_CHECK_EQUAL( deltaAngleTenths, -900 );
}


BOOST_AUTO_TEST_CASE( ArcPoint_ParseFromStream_NoArc )
{
    // Simulate parsing a simple PADS line point: "100 200"
    std::string line = "100 200";
    std::istringstream iss( line );

    double dx, dy;
    int startAngleTenths, deltaAngleTenths;
    double bboxMinX, bboxMinY, bboxMaxX, bboxMaxY;

    iss >> dx >> dy;
    bool hasArc = static_cast<bool>( iss >> startAngleTenths >> deltaAngleTenths
                                          >> bboxMinX >> bboxMinY >> bboxMaxX >> bboxMaxY );

    BOOST_CHECK( !hasArc );
    BOOST_CHECK_CLOSE( dx, 100.0, 0.001 );
    BOOST_CHECK_CLOSE( dy, 200.0, 0.001 );
}


BOOST_AUTO_TEST_SUITE_END()
