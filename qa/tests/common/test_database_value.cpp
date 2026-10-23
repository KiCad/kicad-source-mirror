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

#include <database/database_value.h>

BOOST_AUTO_TEST_SUITE( DatabaseValue )


BOOST_AUTO_TEST_CASE( DoublesRoundTrip )
{
    BOOST_CHECK_EQUAL( DatabaseValueToString( 4700000.0 ), "4700000" );
    BOOST_CHECK_EQUAL( DatabaseValueToString( 10012345.0 ), "10012345" );
    BOOST_CHECK_EQUAL( DatabaseValueToString( 1234.5678 ), "1234.5678" );
    BOOST_CHECK_EQUAL( DatabaseValueToString( 0.1 ), "0.1" );
    BOOST_CHECK_EQUAL( DatabaseValueToString( -0.0 ), "0" );
}


BOOST_AUTO_TEST_CASE( DecimalTextTrimmed )
{
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "4700000.0000" ), "4700000" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "1.2300" ), "1.23" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "-0.500" ), "-0.5" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "12" ), "12" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "0.000" ), "0" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "10." ), "10" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "" ), "" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( ".5000" ), "0.5" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "-.25" ), "-0.25" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( ".0000" ), "0" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "-.0" ), "0" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "+1.50" ), "+1.5" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "1.2.300" ), "1.2.300" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "1-2.00" ), "1-2.00" );
    BOOST_CHECK_EQUAL( NormalizeDecimalText( "." ), "." );
}


BOOST_AUTO_TEST_CASE( OtherTypes )
{
    BOOST_CHECK_EQUAL( DatabaseValueToString( std::monostate() ), "" );
    BOOST_CHECK_EQUAL( DatabaseValueToString( true ), "1" );
    BOOST_CHECK_EQUAL( DatabaseValueToString( false ), "0" );
    BOOST_CHECK_EQUAL( DatabaseValueToString( int64_t( 9007199254740993 ) ), "9007199254740993" );
    BOOST_CHECK_EQUAL( DatabaseValueToString( std::string( "1.2300" ) ), "1.2300" );
}


BOOST_AUTO_TEST_SUITE_END()
