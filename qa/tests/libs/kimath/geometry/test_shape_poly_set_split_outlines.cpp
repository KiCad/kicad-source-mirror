/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/shape_poly_set.h>

#include <qa_utils/geometry/geometry.h>
#include <geometry/geom_test_utils.h>

BOOST_AUTO_TEST_SUITE( ShapePolySetSplitOutlines )

BOOST_AUTO_TEST_CASE( SplitCoincidentOutlineOppositeDirection )
{
    // ASCII art representation of the polygon:
    //   1-----------------0
    //   |                 |
    //   |                 |
    //   |    4------5     |
    //   |    |      |     |
    //  2/9--3/8     |     |
    //   |    |      |     |
    //   10---7------6----11

    SHAPE_POLY_SET poly;
    SHAPE_LINE_CHAIN outline1( { 7600000, 9000000, 6600000, 9000000, 6600000, 8750000, 7000000, 8750000, 7000000,
                                 9000000, 7250000, 9000000, 7250000, 8500000, 7000000, 8500000, 7000000, 8750000,
                                 6600000, 8750000, 6600000, 8000000, 7600000, 8000000 } );
    outline1.SetClosed( true );
    poly.AddOutline( outline1 );

    poly.Simplify();

    BOOST_CHECK_EQUAL( poly.OutlineCount(), 1 );
    BOOST_CHECK_EQUAL( poly.Outline( 0 ).PointCount(), 10 );  //Why is this 10?  I think it should probably be 8
    BOOST_CHECK( GEOM_TEST::IsPolySetValid( poly ) );
}

BOOST_AUTO_TEST_CASE( SplitCoincidentOutlineSameDirection )
{
    // ASCII art representation of the polygon (approximate shape):
    //                        8
    //                    /  /
    //                /     /
    //            /  5     /
    //        /      /\   /
    //  7 /        6/  \ /
    //  1-----------2\  /4
    //    \           \/
    //       \        /3
    //         \     /
    //            \ /
    //             0
    // This polygon has a self-intersecting/overlapping path that creates
    // coincident edges going in the same direction (points 7→2 and 2→7)
    // Original coordinates (scaled): 0:(99,83) 1:(93,89) 2:(80,86) 3:(94,85)
    // 4:(96,87) 5:(96,86) 6:(95,85) 7:(94,85) repeated points: 7:(94,85) 2:(80,86)

    SHAPE_POLY_SET poly;
    SHAPE_LINE_CHAIN outline1( { 9912310, 8325057, 9288816, 8948550, 8000000, 8567586, 9428364,
                                 8547698, 9585009, 8652365, 9613140, 8624234, 9471719, 8482813,
                                 9428364, 8547698, 8000000, 8567586 } );
    outline1.SetClosed( true );
    poly.AddOutline( outline1 );

    poly.Simplify();

    BOOST_CHECK_EQUAL( poly.OutlineCount(), 1 );
    BOOST_CHECK_EQUAL( poly.Outline( 0 ).PointCount(), 7 );
    BOOST_CHECK( GEOM_TEST::IsPolySetValid( poly ) );

}

BOOST_AUTO_TEST_SUITE_END()

