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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <project/net_settings.h>


BOOST_AUTO_TEST_SUITE( BusParsing )


BOOST_AUTO_TEST_CASE( ParsesFormattedVectorBus )
{
    wxString              name;
    std::vector<wxString> members;

    BOOST_CHECK( NET_SETTINGS::ParseBusVector( wxS( "D_{[1..2]}" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxString( wxS( "D" ) ) );

    std::vector<wxString> expected = { wxS( "D1" ), wxS( "D2" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( ParsesFormattedGroupWithVectorMember )
{
    wxString              name;
    std::vector<wxString> members;

    BOOST_CHECK( NET_SETTINGS::ParseBusGroup( wxS( "MEM{D_{[1..2]} ~{LATCH}}" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxString( wxS( "MEM" ) ) );

    std::vector<wxString> expected = { wxS( "D[1..2]" ), wxS( "LATCH" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( RejectsUnescapedSpacesInBusVector )
{
    wxString              name;
    std::vector<wxString> members;

    // Unescaped space in bus vector prefix should fail
    BOOST_CHECK( !NET_SETTINGS::ParseBusVector( wxS( "Data Bus[1..4]" ), &name, &members ) );
}


BOOST_AUTO_TEST_CASE( ParsesBackslashEscapedSpacesInBusVector )
{
    wxString              name;
    std::vector<wxString> members;

    // Backslash-escaped space in bus vector prefix should work
    BOOST_CHECK( NET_SETTINGS::ParseBusVector( wxS( "Data\\ Bus[1..2]" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxString( wxS( "Data Bus" ) ) );

    std::vector<wxString> expected = { wxS( "Data Bus1" ), wxS( "Data Bus2" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( ParsesQuotedSpacesInBusVector )
{
    wxString              name;
    std::vector<wxString> members;

    // Quoted string with space in bus vector prefix should work
    BOOST_CHECK( NET_SETTINGS::ParseBusVector( wxS( "\"Data Bus\"[1..2]" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxString( wxS( "Data Bus" ) ) );

    std::vector<wxString> expected = { wxS( "Data Bus1" ), wxS( "Data Bus2" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( RejectsUnescapedSpacesInBusGroupPrefix )
{
    wxString              name;
    std::vector<wxString> members;

    // Unescaped space in bus group prefix should fail
    BOOST_CHECK( !NET_SETTINGS::ParseBusGroup( wxS( "My Bus{NET1 NET2}" ), &name, &members ) );
}


BOOST_AUTO_TEST_CASE( ParsesBackslashEscapedSpacesInBusGroupPrefix )
{
    wxString              name;
    std::vector<wxString> members;

    // Backslash-escaped space in bus group prefix should work
    BOOST_CHECK( NET_SETTINGS::ParseBusGroup( wxS( "My\\ Bus{NET1 NET2}" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxString( wxS( "My Bus" ) ) );

    std::vector<wxString> expected = { wxS( "NET1" ), wxS( "NET2" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( ParsesQuotedSpacesInBusGroupPrefix )
{
    wxString              name;
    std::vector<wxString> members;

    // Quoted string with space in bus group prefix should work
    BOOST_CHECK( NET_SETTINGS::ParseBusGroup( wxS( "\"My Bus\"{NET1 NET2}" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxString( wxS( "My Bus" ) ) );

    std::vector<wxString> expected = { wxS( "NET1" ), wxS( "NET2" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( ParsesBackslashEscapedSpacesInBusGroupMembers )
{
    wxString              name;
    std::vector<wxString> members;

    // Backslash-escaped space in bus group member name should work.
    // Members are stored with escaped spaces so ForEachBusMember recursion works correctly.
    BOOST_CHECK( NET_SETTINGS::ParseBusGroup( wxS( "BUS{Net\\ One Net\\ Two}" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxString( wxS( "BUS" ) ) );

    std::vector<wxString> expected = { wxS( "Net\\ One" ), wxS( "Net\\ Two" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( ParsesQuotedSpacesInBusGroupMembers )
{
    wxString              name;
    std::vector<wxString> members;

    // Quoted member names with spaces should work.
    // Members are stored with escaped spaces so ForEachBusMember recursion works correctly.
    BOOST_CHECK( NET_SETTINGS::ParseBusGroup( wxS( "BUS{\"Net One\" \"Net Two\"}" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxString( wxS( "BUS" ) ) );

    std::vector<wxString> expected = { wxS( "Net\\ One" ), wxS( "Net\\ Two" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( ParsesMixedEscapingInBusGroup )
{
    wxString              name;
    std::vector<wxString> members;

    // Mix of quoted and backslash-escaped names should work.
    // Members are stored with escaped spaces so ForEachBusMember recursion works correctly.
    BOOST_CHECK( NET_SETTINGS::ParseBusGroup( wxS( "BUS{\"Net One\" Net\\ Two PLAIN}" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxString( wxS( "BUS" ) ) );

    std::vector<wxString> expected = { wxS( "Net\\ One" ), wxS( "Net\\ Two" ), wxS( "PLAIN" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( ForEachBusMemberExpandsVectorWithSpacesInGroup )
{
    // Test that vector members with spaces inside bus groups are correctly expanded.
    // This tests the fix for the bug where quoted vector members like "Data Bus"[1..2]
    // would fail recursive parsing because the quotes were stripped without preserving
    // space escaping.
    std::vector<wxString> expandedMembers;
    auto collector = [&expandedMembers]( const wxString& member )
                     {
                         expandedMembers.push_back( member );
                     };

    // Quoted vector member inside a bus group
    NET_SETTINGS::ForEachBusMember( wxS( "BUS{\"Data Bus\"[1..2] PLAIN}" ), collector );

    std::vector<wxString> expected = { wxS( "Data Bus1" ), wxS( "Data Bus2" ), wxS( "PLAIN" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( expandedMembers.begin(), expandedMembers.end(),
                                   expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( ForEachBusMemberExpandsEscapedVectorInGroup )
{
    // Test backslash-escaped vector member inside a bus group
    std::vector<wxString> expandedMembers;
    auto collector = [&expandedMembers]( const wxString& member )
                     {
                         expandedMembers.push_back( member );
                     };

    NET_SETTINGS::ForEachBusMember( wxS( "BUS{Data\\ Bus[1..2] PLAIN}" ), collector );

    std::vector<wxString> expected = { wxS( "Data Bus1" ), wxS( "Data Bus2" ), wxS( "PLAIN" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( expandedMembers.begin(), expandedMembers.end(),
                                   expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_SUITE_END()
