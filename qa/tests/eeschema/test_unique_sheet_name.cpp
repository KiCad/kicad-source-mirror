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
 * @file test_unique_sheet_name.cpp
 *
 * Placing the same design block as a sheet more than once used to name every sheet after the
 * block verbatim, so the second placement created a duplicate sheet name and the netlist
 * exporter refused to run. UniqueSheetName must hand out collision free names, compared case
 * insensitively to match the ERC duplicate sheet name check.
 */

#include <boost/test/unit_test.hpp>

#include <sch_screen.h>
#include <sch_sheet.h>
#include <tools/sch_tool_utils.h>


namespace
{
SCH_SHEET* addSheet( SCH_SCREEN& aScreen, const wxString& aName )
{
    SCH_SHEET* sheet = new SCH_SHEET();
    sheet->SetName( aName );
    aScreen.Append( sheet );
    return sheet;
}
} // namespace


BOOST_AUTO_TEST_SUITE( UniqueSheetNames )


BOOST_AUTO_TEST_CASE( NullScreenReturnsBase )
{
    BOOST_CHECK_EQUAL( UniqueSheetName( nullptr, wxT( "Amp" ) ), wxString( wxT( "Amp" ) ) );
}


BOOST_AUTO_TEST_CASE( FreeNameIsKept )
{
    SCH_SCREEN screen;
    addSheet( screen, wxT( "Other Sheet" ) );

    BOOST_CHECK_EQUAL( UniqueSheetName( &screen, wxT( "Amp" ) ), wxString( wxT( "Amp" ) ) );
}


BOOST_AUTO_TEST_CASE( CollisionGetsSuffix )
{
    SCH_SCREEN screen;
    addSheet( screen, wxT( "Amp" ) );

    BOOST_CHECK_EQUAL( UniqueSheetName( &screen, wxT( "Amp" ) ), wxString( wxT( "Amp1" ) ) );
}


BOOST_AUTO_TEST_CASE( SuffixSkipsTakenNumbers )
{
    SCH_SCREEN screen;
    addSheet( screen, wxT( "Amp" ) );
    addSheet( screen, wxT( "Amp1" ) );
    addSheet( screen, wxT( "Amp2" ) );

    BOOST_CHECK_EQUAL( UniqueSheetName( &screen, wxT( "Amp" ) ), wxString( wxT( "Amp3" ) ) );
}


BOOST_AUTO_TEST_CASE( ComparisonIsCaseInsensitive )
{
    SCH_SCREEN screen;
    addSheet( screen, wxT( "AMP" ) );
    addSheet( screen, wxT( "amp1" ) );

    BOOST_CHECK_EQUAL( UniqueSheetName( &screen, wxT( "Amp" ) ), wxString( wxT( "Amp2" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
