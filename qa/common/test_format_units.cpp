/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <unit_test_utils/unit_test_utils.h>

#include <base_units.h>

#include <algorithm>
#include <iostream>

struct UnitFixture
{
};


/**
 * Declares a struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( UnitConversion, UnitFixture )


/**
 * Check formatting the point
 */
BOOST_AUTO_TEST_CASE( wxPointUnitFormat )
{
    std::string str = FormatInternalUnits( wxPoint( 123456, 52525252 ) );
    std::string strZero = FormatInternalUnits( wxPoint( 0, 0 ) );
    std::string strNeg = FormatInternalUnits( wxPoint( -123456, -52525252 ) );
    std::string strOddNeg = FormatInternalUnits( wxPoint( -350000, -0 ) );

    std::string strMax = FormatInternalUnits( wxPoint( std::numeric_limits<int>::min(),
            std::numeric_limits<int>::max() ) );

    BOOST_CHECK_EQUAL( strZero, "0 0" );

#ifdef EESCHEMA
    BOOST_CHECK_EQUAL( str, "123456 52525252" );
    BOOST_CHECK_EQUAL( strNeg, "-123456 -52525252" );
    BOOST_CHECK_EQUAL( strMax, "-2147483648 2147483647" );
    BOOST_CHECK_EQUAL( strOddNeg, "-350000 0" );
#elif GERBVIEW
    BOOST_CHECK_EQUAL( str, "1.23456 525.25252" );
    BOOST_CHECK_EQUAL( strNeg, "-1.23456 -525.25252" );
    BOOST_CHECK_EQUAL( strMax, "-21474.83648 21474.83647" );
    BOOST_CHECK_EQUAL( strOddNeg, "-3.5 0" );
#elif PCBNEW
    BOOST_CHECK_EQUAL( str, "0.123456 52.525252" );
    BOOST_CHECK_EQUAL( strNeg, "-0.123456 -52.525252" );
    BOOST_CHECK_EQUAL( strMax, "-2147.483648 2147.483647" );
    BOOST_CHECK_EQUAL( strOddNeg, "-0.35 0" );
#endif

}


BOOST_AUTO_TEST_SUITE_END()
