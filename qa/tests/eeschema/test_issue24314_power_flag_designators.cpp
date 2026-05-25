/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <refdes_tracker.h>


BOOST_AUTO_TEST_SUITE( Issue24314_PowerFlagDesignators )


BOOST_AUTO_TEST_CASE( InsertPowerFlagRefDes )
{
    REFDES_TRACKER tracker;

    BOOST_CHECK( tracker.Insert( "#PWR304" ) );
    BOOST_CHECK( tracker.Contains( "#PWR304" ) );
    BOOST_CHECK( !tracker.Insert( "#PWR304" ) );

    BOOST_CHECK( tracker.Insert( "#PWR305" ) );
    BOOST_CHECK( tracker.Contains( "#PWR305" ) );
}


BOOST_AUTO_TEST_CASE( SerializeRoundTripIncludesPowerFlags )
{
    REFDES_TRACKER tracker;

    tracker.Insert( "C254" );
    tracker.Insert( "C257" );
    tracker.Insert( "C258" );
    tracker.Insert( "#PWR304" );

    const std::string serialized = tracker.Serialize();

    BOOST_CHECK( serialized.find( "C254" ) != std::string::npos );
    BOOST_CHECK( serialized.find( "#PWR304" ) != std::string::npos );

    REFDES_TRACKER tracker2;
    BOOST_REQUIRE( tracker2.Deserialize( serialized ) );

    BOOST_CHECK( tracker2.Contains( "C254" ) );
    BOOST_CHECK( tracker2.Contains( "C257" ) );
    BOOST_CHECK( tracker2.Contains( "C258" ) );
    BOOST_CHECK( tracker2.Contains( "#PWR304" ) );

    // Prefix order in m_prefixData is unordered_map iteration order, so verify
    // round-trip stability by content rather than by string compare.
    REFDES_TRACKER tracker3;
    BOOST_REQUIRE( tracker3.Deserialize( tracker2.Serialize() ) );
    BOOST_CHECK_EQUAL( tracker3.Size(), tracker.Size() );
    BOOST_CHECK( tracker3.Contains( "C254" ) );
    BOOST_CHECK( tracker3.Contains( "C257" ) );
    BOOST_CHECK( tracker3.Contains( "C258" ) );
    BOOST_CHECK( tracker3.Contains( "#PWR304" ) );
}


BOOST_AUTO_TEST_CASE( SinglePowerFlagSurvivesRoundTrip )
{
    REFDES_TRACKER tracker;
    tracker.Insert( "#FLG1" );

    const std::string serialized = tracker.Serialize();
    BOOST_CHECK( !serialized.empty() );

    REFDES_TRACKER tracker2;
    BOOST_REQUIRE( tracker2.Deserialize( serialized ) );

    BOOST_CHECK_EQUAL( tracker2.Size(), 1u );
    BOOST_CHECK( tracker2.Contains( "#FLG1" ) );
}


// Power-flag entries emitted by pre-fix KiCad 10.0.x appeared as prefix-only
// tokens because parseRefDes stored "#PWR304" under an empty-number key.
BOOST_AUTO_TEST_CASE( DeserializeLegacyPrefixOnlyPowerFlag )
{
    REFDES_TRACKER tracker;

    BOOST_REQUIRE( tracker.Deserialize( "C254,C257-258,#PWR304" ) );

    BOOST_CHECK( tracker.Contains( "C254" ) );
    BOOST_CHECK( tracker.Contains( "C257" ) );
    BOOST_CHECK( tracker.Contains( "C258" ) );
    BOOST_CHECK( tracker.Contains( "#PWR304" ) );
}


BOOST_AUTO_TEST_CASE( PrefixContainingDigitRoundTrips )
{
    REFDES_TRACKER tracker;
    BOOST_REQUIRE( tracker.Insert( "U1U2" ) );
    BOOST_REQUIRE( tracker.Insert( "U1U3" ) );

    const std::string serialized = tracker.Serialize();
    BOOST_CHECK( !serialized.empty() );

    REFDES_TRACKER tracker2;
    BOOST_REQUIRE( tracker2.Deserialize( serialized ) );

    BOOST_CHECK( tracker2.Contains( "U1U2" ) );
    BOOST_CHECK( tracker2.Contains( "U1U3" ) );
}


// Pure-numeric input has no non-digit prefix; parseRefDes must store it as a
// prefix-only entry rather than colliding under an empty key.
BOOST_AUTO_TEST_CASE( PureNumericRefDesRoundTrips )
{
    REFDES_TRACKER tracker;
    BOOST_REQUIRE( tracker.Insert( "304" ) );
    BOOST_CHECK( tracker.Contains( "304" ) );

    const std::string serialized = tracker.Serialize();
    BOOST_CHECK( !serialized.empty() );

    REFDES_TRACKER tracker2;
    BOOST_REQUIRE( tracker2.Deserialize( serialized ) );
    BOOST_CHECK( tracker2.Contains( "304" ) );
}


// QA builds install wxAssertThrower around the load path, so a throw from the
// numeric parser would abort the project load.
BOOST_AUTO_TEST_CASE( OverflowNumberFailsCleanly )
{
    REFDES_TRACKER tracker;
    BOOST_CHECK( !tracker.Deserialize( "R99999999999999999999" ) );
    BOOST_CHECK_EQUAL( tracker.Size(), 0u );
}


BOOST_AUTO_TEST_SUITE_END()
