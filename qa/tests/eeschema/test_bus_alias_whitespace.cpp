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

/**
 * @file test_bus_alias_whitespace.cpp
 *
 * Test for issue #19971: Bus alias names and members should have leading/trailing
 * whitespace stripped to prevent silent matching failures.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <bus_alias.h>


BOOST_AUTO_TEST_SUITE( BusAliasWhitespace )


BOOST_AUTO_TEST_CASE( SetNameTrimsWhitespace )
{
    BUS_ALIAS alias;

    alias.SetName( wxS( "DATA " ) );
    BOOST_CHECK_EQUAL( alias.GetName(), wxString( wxS( "DATA" ) ) );

    alias.SetName( wxS( " DATA" ) );
    BOOST_CHECK_EQUAL( alias.GetName(), wxString( wxS( "DATA" ) ) );

    alias.SetName( wxS( "  DATA  " ) );
    BOOST_CHECK_EQUAL( alias.GetName(), wxString( wxS( "DATA" ) ) );

    alias.SetName( wxS( "DATA" ) );
    BOOST_CHECK_EQUAL( alias.GetName(), wxString( wxS( "DATA" ) ) );

    alias.SetName( wxS( "  " ) );
    BOOST_CHECK_EQUAL( alias.GetName(), wxString( wxS( "" ) ) );
}


BOOST_AUTO_TEST_CASE( AddMemberTrimsWhitespace )
{
    BUS_ALIAS alias;

    alias.AddMember( wxS( "SDA " ) );
    alias.AddMember( wxS( " SCL" ) );
    alias.AddMember( wxS( "  MOSI  " ) );
    alias.AddMember( wxS( "MISO" ) );

    BOOST_REQUIRE_EQUAL( alias.Members().size(), 4u );
    BOOST_CHECK_EQUAL( alias.Members()[0], wxString( wxS( "SDA" ) ) );
    BOOST_CHECK_EQUAL( alias.Members()[1], wxString( wxS( "SCL" ) ) );
    BOOST_CHECK_EQUAL( alias.Members()[2], wxString( wxS( "MOSI" ) ) );
    BOOST_CHECK_EQUAL( alias.Members()[3], wxString( wxS( "MISO" ) ) );
}


BOOST_AUTO_TEST_CASE( AddMemberRejectsEmpty )
{
    BUS_ALIAS alias;

    alias.AddMember( wxS( "" ) );
    alias.AddMember( wxS( "   " ) );
    alias.AddMember( wxS( "SDA" ) );

    BOOST_REQUIRE_EQUAL( alias.Members().size(), 1u );
    BOOST_CHECK_EQUAL( alias.Members()[0], wxString( wxS( "SDA" ) ) );
}


BOOST_AUTO_TEST_CASE( SetMembersTrimsWhitespace )
{
    BUS_ALIAS alias;

    std::vector<wxString> members = { wxS( " SDA " ), wxS( "SCL " ), wxS( " MOSI" ), wxS( "" ), wxS( "  " ) };
    alias.SetMembers( members );

    BOOST_REQUIRE_EQUAL( alias.Members().size(), 3u );
    BOOST_CHECK_EQUAL( alias.Members()[0], wxString( wxS( "SDA" ) ) );
    BOOST_CHECK_EQUAL( alias.Members()[1], wxString( wxS( "SCL" ) ) );
    BOOST_CHECK_EQUAL( alias.Members()[2], wxString( wxS( "MOSI" ) ) );
}


BOOST_AUTO_TEST_CASE( ClonePreservesTrimmedValues )
{
    BUS_ALIAS alias;

    alias.SetName( wxS( "DATA " ) );
    alias.AddMember( wxS( " SDA " ) );
    alias.AddMember( wxS( "SCL " ) );

    std::shared_ptr<BUS_ALIAS> clone = alias.Clone();

    BOOST_CHECK_EQUAL( clone->GetName(), wxString( wxS( "DATA" ) ) );
    BOOST_REQUIRE_EQUAL( clone->Members().size(), 2u );
    BOOST_CHECK_EQUAL( clone->Members()[0], wxString( wxS( "SDA" ) ) );
    BOOST_CHECK_EQUAL( clone->Members()[1], wxString( wxS( "SCL" ) ) );
}


BOOST_AUTO_TEST_CASE( ClearMembersWorks )
{
    BUS_ALIAS alias;

    alias.AddMember( wxS( "SDA" ) );
    alias.AddMember( wxS( "SCL" ) );
    BOOST_CHECK_EQUAL( alias.Members().size(), 2u );

    alias.ClearMembers();
    BOOST_CHECK_EQUAL( alias.Members().size(), 0u );
}


BOOST_AUTO_TEST_SUITE_END()
