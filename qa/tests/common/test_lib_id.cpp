/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <lib_id.h>


BOOST_AUTO_TEST_SUITE( LibId )


BOOST_AUTO_TEST_CASE( ParseFullyQualified )
{
    LIB_ID id;
    BOOST_CHECK( id.Parse( "Package_SO:DGG56" ) == -1 );
    BOOST_CHECK_EQUAL( id.GetLibNickname(), "Package_SO" );
    BOOST_CHECK_EQUAL( id.GetLibItemName(), "DGG56" );
    BOOST_CHECK( id.IsValid() );
    BOOST_CHECK( !id.IsLegacy() );
    BOOST_CHECK( !id.empty() );
}


BOOST_AUTO_TEST_CASE( ParseLegacy )
{
    LIB_ID id;
    BOOST_CHECK( id.Parse( "DGG56" ) == -1 );
    BOOST_CHECK( id.GetLibNickname().empty() );
    BOOST_CHECK_EQUAL( id.GetLibItemName(), "DGG56" );
    BOOST_CHECK( !id.IsValid() );
    BOOST_CHECK( id.IsLegacy() );
    BOOST_CHECK( !id.empty() );
}


BOOST_AUTO_TEST_CASE( EqualityFullyQualified )
{
    LIB_ID a( wxT( "Package_SO" ), wxT( "DGG56" ) );
    LIB_ID b( wxT( "Package_SO" ), wxT( "DGG56" ) );
    LIB_ID c( wxT( "OtherLib" ), wxT( "DGG56" ) );

    BOOST_CHECK( a == b );
    BOOST_CHECK( a != c );
}


BOOST_AUTO_TEST_CASE( EqualityLegacyVsFullyQualified )
{
    LIB_ID legacy;
    legacy.Parse( "DGG56" );

    LIB_ID qualified( wxT( "Package_SO" ), wxT( "DGG56" ) );

    // Standard equality requires both library and item name to match
    BOOST_CHECK( legacy != qualified );

    // Legacy matching (item name only) should match when we explicitly compare just item names
    BOOST_CHECK( legacy.IsLegacy() );
    BOOST_CHECK_EQUAL( legacy.GetLibItemName(), qualified.GetLibItemName() );
}


BOOST_AUTO_TEST_CASE( FormatRoundTrip )
{
    LIB_ID qualified( wxT( "Package_SO" ), wxT( "DGG56" ) );
    BOOST_CHECK_EQUAL( qualified.Format().wx_str(), wxString( wxT( "Package_SO:DGG56" ) ) );

    LIB_ID legacy;
    legacy.Parse( "DGG56" );
    BOOST_CHECK_EQUAL( legacy.Format().wx_str(), wxString( wxT( "DGG56" ) ) );

    // Re-parse the formatted strings
    LIB_ID reparsed;
    reparsed.Parse( qualified.Format() );
    BOOST_CHECK( reparsed == qualified );

    LIB_ID reparsedLegacy;
    reparsedLegacy.Parse( legacy.Format() );
    BOOST_CHECK( reparsedLegacy.IsLegacy() );
    BOOST_CHECK_EQUAL( reparsedLegacy.GetLibItemName(), legacy.GetLibItemName() );
}


BOOST_AUTO_TEST_CASE( EmptyId )
{
    LIB_ID empty;
    BOOST_CHECK( empty.empty() );
    BOOST_CHECK( !empty.IsValid() );
    BOOST_CHECK( !empty.IsLegacy() );
}


BOOST_AUTO_TEST_SUITE_END()
