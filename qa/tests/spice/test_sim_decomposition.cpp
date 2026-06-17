/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <sim/sim_model_multiunit.h>


BOOST_AUTO_TEST_SUITE( SimDecomposition )


BOOST_AUTO_TEST_CASE( ParseRepeatWithShared )
{
    SIM_DECOMPOSITION dec = SIM_DECOMPOSITION::Parse( wxS( "mode=repeat shared=VCC,VEE" ) );

    BOOST_CHECK( dec.mode == SIM_DECOMPOSITION::MODE::REPEAT_PER_UNIT );
    BOOST_REQUIRE_EQUAL( dec.sharedModelPins.size(), 2 );
    BOOST_CHECK_EQUAL( dec.sharedModelPins[0], wxS( "VCC" ) );
    BOOST_CHECK_EQUAL( dec.sharedModelPins[1], wxS( "VEE" ) );
}


BOOST_AUTO_TEST_CASE( FormatRepeatWithShared )
{
    SIM_DECOMPOSITION dec;
    dec.mode            = SIM_DECOMPOSITION::MODE::REPEAT_PER_UNIT;
    dec.sharedModelPins = { wxS( "VCC" ), wxS( "VEE" ) };

    BOOST_CHECK_EQUAL( dec.Format(), wxS( "mode=repeat shared=VCC,VEE" ) );
}


BOOST_AUTO_TEST_CASE( RoundTrip )
{
    const wxString canonical = wxS( "mode=repeat shared=VCC,VEE" );

    BOOST_CHECK_EQUAL( SIM_DECOMPOSITION::Parse( canonical ).Format(), canonical );
}


BOOST_AUTO_TEST_CASE( RepeatWithoutShared )
{
    SIM_DECOMPOSITION dec = SIM_DECOMPOSITION::Parse( wxS( "mode=repeat" ) );

    BOOST_CHECK( dec.mode == SIM_DECOMPOSITION::MODE::REPEAT_PER_UNIT );
    BOOST_CHECK( dec.sharedModelPins.empty() );
    BOOST_CHECK_EQUAL( dec.Format(), wxS( "mode=repeat" ) );
}


BOOST_AUTO_TEST_CASE( EmptyIsWholeDevice )
{
    SIM_DECOMPOSITION dec = SIM_DECOMPOSITION::Parse( wxEmptyString );

    BOOST_CHECK( dec.mode == SIM_DECOMPOSITION::MODE::WHOLE_DEVICE );
    BOOST_CHECK( dec.Format().IsEmpty() );
}


BOOST_AUTO_TEST_CASE( UnknownModeIsWholeDevice )
{
    SIM_DECOMPOSITION dec = SIM_DECOMPOSITION::Parse( wxS( "mode=bogus shared=VCC" ) );

    BOOST_CHECK( dec.mode == SIM_DECOMPOSITION::MODE::WHOLE_DEVICE );
    BOOST_CHECK( dec.Format().IsEmpty() );
}


BOOST_AUTO_TEST_CASE( ExplicitWholeDeviceFormatsEmpty )
{
    SIM_DECOMPOSITION dec = SIM_DECOMPOSITION::Parse( wxS( "mode=whole" ) );

    BOOST_CHECK( dec.mode == SIM_DECOMPOSITION::MODE::WHOLE_DEVICE );
    BOOST_CHECK( dec.Format().IsEmpty() );
}


BOOST_AUTO_TEST_SUITE_END()
