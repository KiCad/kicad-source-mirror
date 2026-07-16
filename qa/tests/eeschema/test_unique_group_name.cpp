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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_unique_group_name.cpp
 *
 * Tests for UniqueGroupName, used when placing and pasting named groups.
 */

#include <boost/test/unit_test.hpp>

#include <sch_screen.h>
#include <sch_group.h>
#include <tools/sch_tool_utils.h>


namespace
{
SCH_GROUP* addGroup( SCH_SCREEN& aScreen, const wxString& aName )
{
    SCH_GROUP* group = new SCH_GROUP( &aScreen );
    group->SetName( aName );
    aScreen.Append( group );
    return group;
}
} // namespace


BOOST_AUTO_TEST_SUITE( UniqueGroupNames )


BOOST_AUTO_TEST_CASE( NullScreenReturnsBase )
{
    BOOST_CHECK_EQUAL( UniqueGroupName( nullptr, wxT( "Amp" ) ), wxString( wxT( "Amp" ) ) );
}


BOOST_AUTO_TEST_CASE( FreeNameIsKept )
{
    SCH_SCREEN screen;
    addGroup( screen, wxT( "Other Group" ) );

    BOOST_CHECK_EQUAL( UniqueGroupName( &screen, wxT( "Amp" ) ), wxString( wxT( "Amp" ) ) );
}


BOOST_AUTO_TEST_CASE( CollisionGetsSuffix )
{
    SCH_SCREEN screen;
    addGroup( screen, wxT( "Amp" ) );

    BOOST_CHECK_EQUAL( UniqueGroupName( &screen, wxT( "Amp" ) ), wxString( wxT( "Amp1" ) ) );
}


BOOST_AUTO_TEST_CASE( SuffixSkipsTakenNumbers )
{
    SCH_SCREEN screen;
    addGroup( screen, wxT( "Amp" ) );
    addGroup( screen, wxT( "Amp1" ) );

    BOOST_CHECK_EQUAL( UniqueGroupName( &screen, wxT( "Amp" ) ), wxString( wxT( "Amp2" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
