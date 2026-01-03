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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <kiid.h>
#include <pcb_shape.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>


BOOST_AUTO_TEST_SUITE( PolySimplify )


/**
 * Test that polygon simplification works on the reproduction case from issue #22597.
 * The test file contains two polygons:
 *   1. A rotated rounded rectangle converted to explicit xy points (164 points)
 *   2. A rounded rectangle defined with 4 arcs
 * The xy-point polygon should simplify significantly with a 2mm tolerance.
 * The arc-based polygon preserves its arc data during simplification.
 */
BOOST_AUTO_TEST_CASE( Issue22597SimplifyPolygon )
{
    const auto doBoardTest =
            [&]( BOARD& aBoard )
            {
                PCB_SHAPE* xyPolygon = nullptr;
                PCB_SHAPE* arcPolygon = nullptr;

                for( BOARD_ITEM* item : aBoard.Drawings() )
                {
                    if( item->Type() == PCB_SHAPE_T )
                    {
                        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

                        if( shape->GetShape() == SHAPE_T::POLY )
                        {
                            SHAPE_POLY_SET& poly = shape->GetPolyShape();

                            if( poly.OutlineCount() > 0 )
                            {
                                SHAPE_LINE_CHAIN& outline = poly.Outline( 0 );

                                if( outline.ArcCount() == 0 )
                                    xyPolygon = shape;
                                else if( outline.ArcCount() == 4 )
                                    arcPolygon = shape;
                            }
                        }
                    }
                }

                BOOST_REQUIRE_MESSAGE( xyPolygon != nullptr,
                                       "Could not find the xy-point polygon in the test file" );
                BOOST_REQUIRE_MESSAGE( arcPolygon != nullptr,
                                       "Could not find the 4-arc polygon in the test file" );

                // Test the xy-point polygon - this is the original problematic case
                {
                    SHAPE_POLY_SET& poly = xyPolygon->GetPolyShape();
                    SHAPE_LINE_CHAIN& outline = poly.Outline( 0 );

                    int originalPointCount = outline.PointCount();

                    BOOST_TEST_MESSAGE( "XY polygon original: " << originalPointCount
                                        << " points, " << outline.ArcCount() << " arcs" );
                    BOOST_CHECK_EQUAL( originalPointCount, 164 );

                    poly.SimplifyOutlines( 2000000 );

                    int simplifiedCount = outline.PointCount();

                    BOOST_TEST_MESSAGE( "XY polygon simplified: " << simplifiedCount
                                        << " points, " << outline.ArcCount() << " arcs" );

                    BOOST_CHECK_LT( simplifiedCount, originalPointCount );
                    BOOST_CHECK_LE( simplifiedCount, 20 );
                }

                // Test the arc-based polygon - arcs should be preserved
                {
                    SHAPE_POLY_SET& poly = arcPolygon->GetPolyShape();
                    SHAPE_LINE_CHAIN& outline = poly.Outline( 0 );

                    int originalPointCount = outline.PointCount();
                    int originalArcCount = outline.ArcCount();

                    BOOST_TEST_MESSAGE( "Arc polygon original: " << originalPointCount
                                        << " points, " << originalArcCount << " arcs" );
                    BOOST_CHECK_EQUAL( originalArcCount, 4 );

                    poly.SimplifyOutlines( 2000000 );

                    int simplifiedPointCount = outline.PointCount();
                    int simplifiedArcCount = outline.ArcCount();

                    BOOST_TEST_MESSAGE( "Arc polygon simplified: " << simplifiedPointCount
                                        << " points, " << simplifiedArcCount << " arcs" );

                    // Arc polygon should remain valid and arcs should be preserved
                    BOOST_CHECK_EQUAL( simplifiedArcCount, originalArcCount );
                    BOOST_CHECK( poly.OutlineCount() > 0 );
                }
            };

    KI_TEST::LoadAndTestBoardFile( "issue22597/issue22597", false, doBoardTest, std::nullopt );
}


BOOST_AUTO_TEST_SUITE_END()
