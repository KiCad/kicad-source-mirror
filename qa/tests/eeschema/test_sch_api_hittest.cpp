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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <api/api_utils.h>
#include <api/common/types/base_types.pb.h>
#include <base_units.h>
#include <sch_line.h>

/**
 * Regression tests for the IU-scale handling in API_HANDLER_EDITOR::handleHitTest.
 *
 * Wire coordinates are always in nanometers. Prior to the fix, the schematic handler
 * inherited the base class default of pcbIUScale, so positions sent in nanometers
 * were decoded as pcb-IU (1 nm/IU) and then handed to schematic items whose native IU
 * is 100 nm. Every position was therefore 100x too large, and every HitTest missed.
 */

BOOST_AUTO_TEST_SUITE( ApiSchHitTest )


BOOST_AUTO_TEST_CASE( UnpackVector2RoundTripsThroughSchIuScale )
{
    const VECTOR2I nativeSchIu( 50000, 75000 );  // 5 mm x 7.5 mm in schematic IU

    kiapi::common::types::Vector2 wire;
    kiapi::common::PackVector2( wire, nativeSchIu, schIUScale );

    BOOST_CHECK_EQUAL( wire.x_nm(), static_cast<int64_t>( 5'000'000 ) );
    BOOST_CHECK_EQUAL( wire.y_nm(), static_cast<int64_t>( 7'500'000 ) );

    VECTOR2I roundTrip = kiapi::common::UnpackVector2( wire, schIUScale );
    BOOST_CHECK_EQUAL( roundTrip.x, nativeSchIu.x );
    BOOST_CHECK_EQUAL( roundTrip.y, nativeSchIu.y );
}


BOOST_AUTO_TEST_CASE( UnpackVector2WithWrongScaleYieldsWrongPosition )
{
    const VECTOR2I nativeSchIu( 50000, 75000 );

    kiapi::common::types::Vector2 wire;
    kiapi::common::PackVector2( wire, nativeSchIu, schIUScale );

    VECTOR2I asPcb = kiapi::common::UnpackVector2( wire, pcbIUScale );

    BOOST_CHECK_EQUAL( asPcb.x, 5'000'000 );
    BOOST_CHECK_EQUAL( asPcb.y, 7'500'000 );
    BOOST_CHECK_NE( asPcb.x, nativeSchIu.x );
}


BOOST_AUTO_TEST_CASE( HitTestHitsWhenPositionUnpackedWithSchIuScale )
{
    SCH_LINE line;
    line.SetStartPoint( VECTOR2I( 100000, 100000 ) );
    line.SetEndPoint( VECTOR2I( 200000, 100000 ) );

    VECTOR2I onLine( 150000, 100000 );

    kiapi::common::types::Vector2 wire;
    kiapi::common::PackVector2( wire, onLine, schIUScale );

    VECTOR2I correct = kiapi::common::UnpackVector2( wire, schIUScale );
    BOOST_CHECK( line.HitTest( correct, 0 ) );

    VECTOR2I broken = kiapi::common::UnpackVector2( wire, pcbIUScale );
    BOOST_CHECK( !line.HitTest( broken, 0 ) );
}


BOOST_AUTO_TEST_CASE( ToleranceScalesFromNanometersToSchIu )
{
    SCH_LINE line;
    line.SetStartPoint( VECTOR2I( 0, 0 ) );
    line.SetEndPoint( VECTOR2I( 100000, 0 ) );

    // Sit outside the line by more than pen-width/2 but within 2 schematic IU. Proves the
    // tolerance argument actually reaches HitTest at the intended magnitude.
    VECTOR2I offLine( 50000, line.GetPenWidth() / 2 + 2 );

    const int toleranceNm = 10 * 100;  // 10 schematic IU worth, expressed in nm (100 nm/IU)
    const int toleranceSchIu = schIUScale.NmToIU( toleranceNm );

    BOOST_CHECK_EQUAL( toleranceSchIu, 10 );
    BOOST_CHECK( line.HitTest( offLine, toleranceSchIu ) );
    BOOST_CHECK( !line.HitTest( offLine, 0 ) );
}


BOOST_AUTO_TEST_SUITE_END()
