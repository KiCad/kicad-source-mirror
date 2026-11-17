/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sim/spice_value.h>


BOOST_AUTO_TEST_SUITE( SpiceValue )


BOOST_AUTO_TEST_CASE( AutoRangeBelowFemtoDropsPrefix )
{
    SPICE_VALUE_FORMAT format{ 3, wxS( "~V" ) };

    // Smaller than the femto prefix can show, so an exponent and a prefix would both appear
    BOOST_CHECK_EQUAL( SPICE_VALUE( -2.69e-26 ).ToString( format ), wxS( "-2.69e-26V" ) );

    BOOST_CHECK_EQUAL( SPICE_VALUE( 4.7e-6 ).ToString( format ), wxS( "4.70uV" ) );
    BOOST_CHECK_EQUAL( SPICE_VALUE( 1.5 ).ToString( format ), wxS( "1.50V" ) );
}


BOOST_AUTO_TEST_CASE( NegativeKeepsPrefixAndPrecision )
{
    SPICE_VALUE_FORMAT format{ 3, wxS( "~V" ) };

    BOOST_CHECK_EQUAL( SPICE_VALUE( -3.68e-3 ).ToString( format ), wxS( "-3.68mV" ) );
    BOOST_CHECK_EQUAL( SPICE_VALUE( -1.5 ).ToString( format ), wxS( "-1.50V" ) );
}


BOOST_AUTO_TEST_CASE( RoundingUpPicksNextPrefix )
{
    SPICE_VALUE_FORMAT format{ 3, wxS( "~V" ) };

    BOOST_CHECK_EQUAL( SPICE_VALUE( 0.99995 ).ToString( format ), wxS( "1.00V" ) );
    BOOST_CHECK_EQUAL( SPICE_VALUE( -0.99995 ).ToString( format ), wxS( "-1.00V" ) );
    BOOST_CHECK_EQUAL( SPICE_VALUE( 999.96e-6 ).ToString( format ), wxS( "1.00mV" ) );
    BOOST_CHECK_EQUAL( SPICE_VALUE( 0.0 ).ToString( format ), wxS( "0.00V" ) );
}


BOOST_AUTO_TEST_SUITE_END()
