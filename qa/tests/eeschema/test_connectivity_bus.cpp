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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <connectivity/conn_bus.h>

#include <limits>

using namespace SCH_CONNECTIVITY;

BOOST_AUTO_TEST_SUITE( ConnectivityBus )

BOOST_AUTO_TEST_CASE( PreservesFormattedVectorNames )
{
    const auto schema = BUS_SCHEMA::Parse( "~{BE[0..3]}" );
    BOOST_REQUIRE( schema );
    BOOST_CHECK( schema->shape == BUS_SCHEMA::SHAPE::VECTOR );
    BOOST_REQUIRE_EQUAL( schema->leaves.size(), 4 );
    BOOST_CHECK_EQUAL( schema->leaves[0].name, wxString( "~{BE0}" ) );
    BOOST_CHECK_EQUAL( schema->leaves[3].name, wxString( "~{BE3}" ) );

    const auto subscript = BUS_SCHEMA::Parse( "D_{[1..2]}" );
    BOOST_REQUIRE( subscript );
    BOOST_CHECK_EQUAL( subscript->leaves[0].name, wxString( "D1" ) );
}

BOOST_AUTO_TEST_CASE( PreservesZeroPaddedMemberIdentity )
{
    const auto schema = BUS_SCHEMA::Parse( "D[01..03]" );
    BOOST_REQUIRE( schema );
    BOOST_REQUIRE_EQUAL( schema->leaves.size(), 3 );
    BOOST_CHECK_EQUAL( schema->leaves[0].name, wxString( "D01" ) );
    BOOST_CHECK_EQUAL( schema->leaves[2].name, wxString( "D03" ) );

    const auto group = BUS_SCHEMA::Parse( "BUS{D01 D1 D01}" );
    BOOST_REQUIRE( group );
    BOOST_REQUIRE_EQUAL( group->leaves.size(), 3 );
    BOOST_CHECK_EQUAL( group->leaves[0].name, wxString( "BUS.D01" ) );
    BOOST_CHECK_EQUAL( group->leaves[1].name, wxString( "BUS.D1" ) );
    BOOST_CHECK( group->leaves[0] == group->leaves[2] );
}

BOOST_AUTO_TEST_CASE( PreservesQuotingEscapingAndFormatting )
{
    const auto quoted = BUS_SCHEMA::Parse( "\"Data Bus\"[1..2]" );
    const auto escaped = BUS_SCHEMA::Parse( "Data\\ Bus[1..2]" );
    BOOST_REQUIRE( quoted );
    BOOST_REQUIRE( escaped );
    BOOST_CHECK( quoted->leaves == escaped->leaves );
    BOOST_CHECK_EQUAL( quoted->leaves[0].name, wxString( "Data Bus1" ) );

    const auto group = BUS_SCHEMA::Parse( "I^{2}C{R{slash}~{W} D01}" );
    BOOST_REQUIRE( group );
    BOOST_CHECK_EQUAL( group->leaves[0].name, wxString( "I^{2}C.R{slash}~{W}" ) );
}

BOOST_AUTO_TEST_CASE( RejectsAliasCycles )
{
    BOOST_CHECK( !BUS_SCHEMA::Parse( "{A}", { { "A", { "B" } }, { "B", { "A" } } } ) );
    BOOST_CHECK( !BUS_SCHEMA::Parse( "{A}", { { "A", { "A" } } } ) );
}

BOOST_AUTO_TEST_CASE( RejectsScalarAndInvalidGroupText )
{
    BOOST_CHECK( !BUS_SCHEMA::Parse( "D01" ) );
    BOOST_CHECK( !BUS_SCHEMA::Parse( "Data Bus[0..3]" ) );
    BOOST_CHECK( !BUS_SCHEMA::Parse( "BUS{A B" ) );
}

BOOST_AUTO_TEST_CASE( ParsesQuotedAndEscapedGroupMembersOnce )
{
    const auto schema = BUS_SCHEMA::Parse( "BUS{\"Net One\" Net\\ Two R{slash}W}" );
    BOOST_REQUIRE( schema );
    BOOST_REQUIRE_EQUAL( schema->leaves.size(), 3 );
    BOOST_CHECK_EQUAL( schema->leaves[0].name, wxString( "BUS.Net One" ) );
    BOOST_CHECK_EQUAL( schema->leaves[1].name, wxString( "BUS.Net Two" ) );
    BOOST_CHECK_EQUAL( schema->leaves[2].name, wxString( "BUS.R{slash}W" ) );
}

BOOST_AUTO_TEST_CASE( RejectsMalformedAndOverflowingVectorRanges )
{
    BOOST_CHECK( !BUS_SCHEMA::Parse( "D[-1..2]" ) );
    BOOST_CHECK( !BUS_SCHEMA::Parse( "D[1..-2]" ) );
    BOOST_CHECK( !BUS_SCHEMA::Parse( "D[..2]" ) );
    BOOST_CHECK( !BUS_SCHEMA::Parse( "D[1..]" ) );
    BOOST_CHECK( !BUS_SCHEMA::Parse( "D[1..2" ) );
    BOOST_CHECK( !BUS_SCHEMA::Parse( "D[1..999999999999999999999999999999]" ) );
    const wxString largest = wxString::Format( "%ld", std::numeric_limits<long>::max() );
    BOOST_CHECK( !BUS_SCHEMA::Parse( "D[1.." + largest + "0]" ) );
}

BOOST_AUTO_TEST_SUITE_END()
