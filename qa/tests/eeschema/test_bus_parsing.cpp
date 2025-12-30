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
    BOOST_CHECK_EQUAL( name, wxS( "D" ) );

    std::vector<wxString> expected = { wxS( "D1" ), wxS( "D2" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( ParsesFormattedGroupWithVectorMember )
{
    wxString              name;
    std::vector<wxString> members;

    BOOST_CHECK( NET_SETTINGS::ParseBusGroup( wxS( "MEM{D_{[1..2]} ~{LATCH}}" ), &name, &members ) );
    BOOST_CHECK_EQUAL( name, wxS( "MEM" ) );

    std::vector<wxString> expected = { wxS( "D[1..2]" ), wxS( "LATCH" ) };

    BOOST_CHECK_EQUAL_COLLECTIONS( members.begin(), members.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_SUITE_END()
