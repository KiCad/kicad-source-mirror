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

#define BOOST_TEST_NO_MAIN
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <base_units.h>
#include <string_any_map.h>


BOOST_AUTO_TEST_SUITE( StringAnyMap )


// 1.001 mm scales to 1000999.9999999999 nm, which a generator must not load 1 nm short
BOOST_AUTO_TEST_CASE( GetToIuRoundsIntegralValues )
{
    STRING_ANY_MAP props( pcbIUScale.IU_PER_MM );
    props.set_iu( "width", 1001000 );
    props.set( "offset", -1.001 );
    props.set( "length", 1.001 );

    int       value = 0;
    long long length = 0;

    BOOST_REQUIRE( props.get_to_iu( "width", value ) );
    BOOST_CHECK_EQUAL( value, 1001000 );

    BOOST_REQUIRE( props.get_to_iu( "offset", value ) );
    BOOST_REQUIRE( props.get_to_iu( "length", length ) );
    BOOST_CHECK_EQUAL( value, -1001000 );
    BOOST_CHECK_EQUAL( length, 1001000 );
}


BOOST_AUTO_TEST_SUITE_END()
