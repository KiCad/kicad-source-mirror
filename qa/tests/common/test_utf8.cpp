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

#include <core/utf8.h>

#include <algorithm>
#include <iostream>

#define UTF8_INIT "This is a test of UTF-8: ü‱☺😕😱"
struct Utf8Fixture
{
};


/**
 * Declares a struct as the Boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( Utf8, Utf8Fixture )


/**
 * Check direct and copy construction from std::string
 */
BOOST_AUTO_TEST_CASE( Utf8AndStdString )
{
    std::string str { UTF8_INIT };

    UTF8 utf8_inited { UTF8_INIT };
    UTF8 utf8_copied_from_stdstr = str;

    BOOST_CHECK_EQUAL( utf8_inited, utf8_copied_from_stdstr );

    UTF8 utf8_copied_from_utf8 = utf8_inited;

    BOOST_CHECK_EQUAL( utf8_inited, utf8_copied_from_utf8 );
}


/**
 * Check direct and copy construction from wxString
 */
BOOST_AUTO_TEST_CASE( Utf8AndWx )
{
    UTF8 utf8_inited { UTF8_INIT };
    wxString wx_inited = wxString::FromUTF8( UTF8_INIT );

    // Check that we can copy convert WxString and compare
    wxString wx_copied_from_utf8 = utf8_inited;
    BOOST_CHECK_EQUAL( wx_inited, wx_copied_from_utf8 );

    // Check we can copy-construct from a WxString
    UTF8 utf8_copied_from_wxstring = wx_inited;
    BOOST_CHECK_EQUAL( utf8_inited, utf8_copied_from_wxstring );
}

/**
 * UTF8::uni_iter null tests
 */
BOOST_AUTO_TEST_CASE( UniIterNull )
{
    UTF8::uni_iter it;
    const UTF8::uni_iter null;

    // Check nulls are equivalent
    BOOST_CHECK( it == null );

    // check null string start == end
    UTF8 uNull { "" };
    BOOST_CHECK( uNull.ubegin() == uNull.uend() );
}

/**
 * UTF8::uni_iter increment tests
 */
BOOST_AUTO_TEST_CASE( UniIterIncrement )
{
    UTF8 u0 { "inp\ua740t" };

    UTF8::uni_iter it;
    const UTF8::uni_iter begin = u0.ubegin();
    const UTF8::uni_iter end = u0.uend();

    // Check copy-construction and equality operator
    it = begin;
    BOOST_CHECK( it == begin );
    BOOST_CHECK( it >= begin );

    // Check de-referencing
    BOOST_CHECK_EQUAL( *it, 'i' );

    // postfix increment - normal char and inequality operators
    it++;
    BOOST_CHECK( it != begin );
    BOOST_CHECK( it > begin );
    BOOST_CHECK( !( begin >= it ) );
    BOOST_CHECK( it < end );
    BOOST_CHECK( it <= end );
    BOOST_CHECK_EQUAL( *it, 'n' );

    // prefix increment - normal char
    ++it;
    BOOST_CHECK_EQUAL( *it, 'p' );

    // increment to a unicode char
    ++it;
    BOOST_CHECK_EQUAL( *it, 0xA740 );

    // and again to the next char - normal again
    ++it;
    BOOST_CHECK_EQUAL( *it, 't' );

    // and now we should be at the end
    ++it;
    BOOST_CHECK( it == end );
    BOOST_CHECK( it <= end );
    BOOST_CHECK( !( it < end ) );
}


BOOST_AUTO_TEST_SUITE_END()
