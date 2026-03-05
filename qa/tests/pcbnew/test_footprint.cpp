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
#include <qa_utils/geometry/geometry.h>

#include <board.h>
#include <geometry/shape_utils.h>
#include <footprint.h>
#include <pcb_shape.h>


static bool CourtyardEqualPredicate( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b )
{
    // The courtyards get a tiny epsilon to handle polygonisaton errors
    const int courtyardEpsilon = pcbIUScale.mmToIU( 0.005 );

    if( a.OutlineCount() != b.OutlineCount() )
        return false;

    // We can only use this predicate on single-outline polys unless we do more work.
    // Because we don't know the sub-outlines are in the same order.
    BOOST_REQUIRE( a.OutlineCount() == 1 );

    return KI_TEST::ChainsAreCyclicallyEqual( a.Outline( 0 ), b.Outline( 0 ), courtyardEpsilon );
};


BOOST_AUTO_TEST_SUITE( Footprint )


BOOST_AUTO_TEST_CASE( FootprintCourtyardAndHull )
{
    // Footprint courtyards are cached internally. Some operations manipulate the
    // cache efficiently, some rebuild it. In any case, the courtyard should always
    // be consistent with the geometry of the footprint. Same for the bounding hull.

    BOARD     board;
    FOOTPRINT fp( &board );

    const int lineW = pcbIUScale.mmToIU( 0.05 );
    const int courtyardH = pcbIUScale.mmToIU( 1.0 );
    const int courtyardW = pcbIUScale.mmToIU( 2.0 );

    const std::vector<VECTOR2I> courtyardPoints = {
        { 0, 0 },
        { courtyardW, 0 },
        { courtyardW, courtyardH },
        { 0, courtyardH },
    };

    const auto assertCourtyardMatches = [&]( PCB_LAYER_ID layer, const SHAPE_LINE_CHAIN& aExpectedCourtyard )
    {
        const SHAPE_POLY_SET& courtyard = fp.GetCourtyard( layer );

        BOOST_REQUIRE( courtyard.OutlineCount() == 1 );
        BOOST_CHECK_PREDICATE( CourtyardEqualPredicate, ( courtyard.Outline( 0 ) )( aExpectedCourtyard ) );
    };

    const auto assertNoCourtyard = [&]( PCB_LAYER_ID layer )
    {
        const SHAPE_POLY_SET& courtyard = fp.GetCourtyard( layer );

        BOOST_TEST( courtyard.OutlineCount() == 0 );
    };

    const auto assertHullMatch = [&]( const SHAPE_LINE_CHAIN& aExpectedHull )
    {
        const SHAPE_POLY_SET& hull = fp.GetBoundingHull();

        BOOST_REQUIRE( hull.OutlineCount() == 1 );
        BOOST_CHECK_PREDICATE( KI_TEST::ChainsAreCyclicallyEqual, ( hull.Outline( 0 ) )(aExpectedHull) ( 0 ) );
    };

    {
        std::unique_ptr<PCB_SHAPE> courtyardPoly = std::make_unique<PCB_SHAPE>( &fp, SHAPE_T::POLY );
        courtyardPoly->SetLayer( F_CrtYd );
        courtyardPoly->SetPolyPoints( courtyardPoints );
        courtyardPoly->SetWidth( lineW );

        fp.Add( courtyardPoly.release() );
    }

    // We'll modify this in lock-step with the footprint
    SHAPE_LINE_CHAIN expectedCourtyard( courtyardPoints, true );
    // The hull is hard to calculate - we'll take the initial one as a given
    SHAPE_LINE_CHAIN expectedHull = fp.GetBoundingHull().Outline( 0 );

    BOOST_TEST_CONTEXT( "Initial courtyard" )
    {
        assertCourtyardMatches( F_CrtYd, expectedCourtyard );
        assertNoCourtyard( B_CrtYd );
    }

    const VECTOR2I moveVector = VECTOR2I( courtyardW, 0 );

    fp.Move( moveVector );
    expectedCourtyard.Move( moveVector );
    expectedHull.Move( moveVector );

    BOOST_TEST_CONTEXT( "Moved courtyard" )
    {
        assertCourtyardMatches( F_CrtYd, expectedCourtyard );
        assertNoCourtyard( B_CrtYd );
        assertHullMatch( expectedHull );
    }

    fp.Rotate( VECTOR2I( 0, 0 ), EDA_ANGLE( 90.0 ) );
    expectedCourtyard.Rotate( EDA_ANGLE( 90.0 ), VECTOR2I( 0, 0 ) );
    expectedHull.Rotate( EDA_ANGLE( 90.0 ), VECTOR2I( 0, 0 ) );

    BOOST_TEST_CONTEXT( "Rotated courtyard" )
    {
        assertCourtyardMatches( F_CrtYd, expectedCourtyard );
        assertNoCourtyard( B_CrtYd );
        assertHullMatch( expectedHull );
    }

    fp.Flip( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );
    expectedCourtyard.Mirror( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );
    expectedHull.Mirror( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );

    const BOX2I flippedExpectedBox = BOX2I::ByCorners( VECTOR2I( -courtyardW, 0 ), VECTOR2I( 0, courtyardH ) );

    BOOST_TEST_CONTEXT( "Flipped courtyard" )
    {
        assertCourtyardMatches( B_CrtYd, expectedCourtyard );
        assertNoCourtyard( F_CrtYd );
        assertHullMatch( expectedHull );
    }
}

BOOST_AUTO_TEST_SUITE_END()