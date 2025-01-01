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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file
 * Test suite for SEXPR::PARSER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <sexpr/sexpr.h>

#include "sexpr_test_utils.h"

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( Sexpr )


BOOST_AUTO_TEST_CASE( BasicConstruction )
{
    SEXPR::SEXPR_INTEGER s_int{ 1 };
    // not sure why cast is needed, but boost doesn't like it without
    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsIntegerWithValue, ( (SEXPR::SEXPR&) s_int )( 1 ) );

    SEXPR::SEXPR_DOUBLE s_double{ 3.14 };
    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsDoubleWithValue, ( (SEXPR::SEXPR&) s_double )( 3.14 ) );

    SEXPR::SEXPR_STRING s_string{ "string" };
    BOOST_CHECK_PREDICATE(
            KI_TEST::SexprIsStringWithValue, ( (SEXPR::SEXPR&) s_string )( "string" ) );

    SEXPR::SEXPR_STRING s_symbol{ "symbol" };
    BOOST_CHECK_PREDICATE(
            KI_TEST::SexprIsStringWithValue, ( (SEXPR::SEXPR&) s_symbol )( "symbol" ) );
}

BOOST_AUTO_TEST_CASE( AsStringInt )
{
    SEXPR::SEXPR_INTEGER s{ 1 };
    // not sure why cast is needed, but boost doesn't like it without
    BOOST_CHECK_PREDICATE( KI_TEST::SexprConvertsToString, ( (SEXPR::SEXPR&) s )( "1" ) );
}

BOOST_AUTO_TEST_CASE( AsStringDouble )
{
    SEXPR::SEXPR_DOUBLE s{ 3.14 };
    BOOST_CHECK_PREDICATE( KI_TEST::SexprConvertsToString, ( (SEXPR::SEXPR&) s )( "3.14" ) );
}

BOOST_AUTO_TEST_CASE( AsStringSymbol )
{
    SEXPR::SEXPR_SYMBOL s{ "symbol" };
    BOOST_CHECK_PREDICATE( KI_TEST::SexprConvertsToString, ( (SEXPR::SEXPR&) s )( "symbol" ) );
}

BOOST_AUTO_TEST_CASE( AsStringString )
{
    SEXPR::SEXPR_STRING s{ "string" };

    // strings get quotes
    BOOST_CHECK_PREDICATE( KI_TEST::SexprConvertsToString, ( (SEXPR::SEXPR&) s )( "\"string\"" ) );
}

BOOST_AUTO_TEST_CASE( AsStringList )
{
    SEXPR::SEXPR_LIST s_list;

    s_list.AddChild( new SEXPR::SEXPR_SYMBOL{ "symbol" } );

    {
        auto* s_SubList = new SEXPR::SEXPR_LIST();
        *s_SubList << 2 << 42.42 << "substring";
        s_list.AddChild( s_SubList );
    }

    s_list << 1 << 3.14 << "string";

    BOOST_CHECK_PREDICATE( KI_TEST::SexprIsListOfLength, ( (SEXPR::SEXPR&) s_list )( 5 ) );

    // REVIEW: should there be a space at the end of the "symbol"?
    BOOST_CHECK_PREDICATE( KI_TEST::SexprConvertsToString,
            ( (SEXPR::SEXPR&) s_list )( "(symbol \n"
                                        "  (2 42.42 \"substring\") 1 3.14 \"string\")" ) );
}

BOOST_AUTO_TEST_SUITE_END()