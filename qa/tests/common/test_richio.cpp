/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.TXT for contributors.
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
 * Test suite for general string functions
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <richio.h>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( RichIO )

/**
 * Test the #vprint method.
 */
BOOST_AUTO_TEST_CASE( VPrint )
{
    std::string output;

    // Test 1: Basic strings and numbers
    StrPrintf( &output, "Hello %s! ", "World" );
    StrPrintf( &output, "Number: %d, ", 42 );
    StrPrintf( &output, "Float: %.2f, ", 3.14 );
    StrPrintf( &output, "Char: %c. ", 'A' );
    BOOST_CHECK_EQUAL( output, std::string( "Hello World! Number: 42, Float: 3.14, Char: A. " ) );
    output.clear();

    // Test 2: Large string
    std::string longString( 500, 'A' );
    StrPrintf( &output, "%s", longString.c_str() );
    BOOST_CHECK_EQUAL( output, longString );
    output.clear();

    // Test 3: Edge case with zero characters
    #ifdef __GNUC__
    #pragma GCC diagnostic ignored "-Wformat-zero-length"
    #endif
    StrPrintf( &output, "" );
    #ifdef __GNUC__
    #pragma GCC diagnostic warning "-Wformat-zero-length"
    #endif
    BOOST_ASSERT( output.empty() );

    // Test 4: Mixing small and large strings
    StrPrintf( &output, "Small, " );
    StrPrintf( &output, "%s, ", longString.c_str() );
    StrPrintf( &output, "End." );
    BOOST_CHECK_EQUAL( output, std::string( "Small, " + longString + ", End." ) );
    output.clear();

    // Test 5: Formatting with various data types
    StrPrintf( &output, "%d %s %c %.2f", 123, "Hello", 'X', 9.876 );
    BOOST_CHECK_EQUAL( output, std::string( "123 Hello X 9.88" ) );
    output.clear();
}

BOOST_AUTO_TEST_SUITE_END()