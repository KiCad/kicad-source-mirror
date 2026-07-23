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

#include <boost/test/unit_test.hpp>

#include <preview_items/construction_geom.h>
#include <tool/construction_manager.h>


BOOST_AUTO_TEST_SUITE( ConstructionGeom )


BOOST_AUTO_TEST_CASE( HorizontalDimensionBracketHasPerpendicularEndTicks )
{
    std::array<SEG, 3> segments =
            KIGFX::CONSTRUCTION_GEOM::DimensionBracketSegments( SEG( { 10, 20 }, { 40, 20 } ), 6 );

    BOOST_CHECK_EQUAL( segments[0].A, VECTOR2I( 10, 20 ) );
    BOOST_CHECK_EQUAL( segments[0].B, VECTOR2I( 40, 20 ) );
    BOOST_CHECK_EQUAL( segments[1].A, VECTOR2I( 10, 17 ) );
    BOOST_CHECK_EQUAL( segments[1].B, VECTOR2I( 10, 23 ) );
    BOOST_CHECK_EQUAL( segments[2].A, VECTOR2I( 40, 17 ) );
    BOOST_CHECK_EQUAL( segments[2].B, VECTOR2I( 40, 23 ) );
}


BOOST_AUTO_TEST_CASE( VerticalDimensionBracketHasPerpendicularEndTicks )
{
    std::array<SEG, 3> segments =
            KIGFX::CONSTRUCTION_GEOM::DimensionBracketSegments( SEG( { 10, 20 }, { 10, 50 } ), 6 );

    BOOST_CHECK_EQUAL( segments[0].A, VECTOR2I( 10, 20 ) );
    BOOST_CHECK_EQUAL( segments[0].B, VECTOR2I( 10, 50 ) );
    BOOST_CHECK_EQUAL( segments[1].A, VECTOR2I( 7, 20 ) );
    BOOST_CHECK_EQUAL( segments[1].B, VECTOR2I( 13, 20 ) );
    BOOST_CHECK_EQUAL( segments[2].A, VECTOR2I( 7, 50 ) );
    BOOST_CHECK_EQUAL( segments[2].B, VECTOR2I( 13, 50 ) );
}


BOOST_AUTO_TEST_CASE( DimensionBracketOffsetsOutsideObjectEdge )
{
    std::array<SEG, 3> segments =
            KIGFX::CONSTRUCTION_GEOM::DimensionBracketSegments( SEG( { 10, 20 }, { 10, 50 } ), 6, 8 );

    BOOST_CHECK_EQUAL( segments[0].A, VECTOR2I( 18, 20 ) );
    BOOST_CHECK_EQUAL( segments[0].B, VECTOR2I( 18, 50 ) );
    BOOST_CHECK_EQUAL( segments[1].A, VECTOR2I( 15, 20 ) );
    BOOST_CHECK_EQUAL( segments[1].B, VECTOR2I( 21, 20 ) );
}


BOOST_AUTO_TEST_CASE( SnapManagerOwnsDimensionBracketLifecycle )
{
    KIGFX::CONSTRUCTION_GEOM geometry;
    SNAP_MANAGER             manager( geometry );

    manager.SetDimensionBrackets( { SEG( { 10, 20 }, { 40, 20 } ), SEG( { 50, 20 }, { 80, 20 } ) } );

    BOOST_REQUIRE_EQUAL( geometry.DimensionBrackets().size(), 2 );
    manager.Clear();
    BOOST_CHECK( geometry.DimensionBrackets().empty() );
}


BOOST_AUTO_TEST_SUITE_END()
