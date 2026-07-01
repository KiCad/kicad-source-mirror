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

// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24760

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <map>
#include <memory>
#include <set>

#include <eeschema_helpers.h>
#include <locale_io.h>
#include <sch_sheet_path.h>
#include <schematic.h>


// The reproduction project is an 11-page hierarchy that was saved with duplicate
// page numbers (two sheets numbered "11", two numbered "12") and gaps.  Plotting
// stamps each page's title-block Id from the sheet's stored page number, so the
// duplicates produced two pages sharing an Id (and an impossible "12/11") while
// others were missing.  Loading the project must repair the collisions so every
// sheet carries a unique page number; the CLI/headless loader is used directly
// because kicad-cli sch export pdf goes through it rather than the GUI.
BOOST_AUTO_TEST_CASE( Issue24760PlotPageNumbers )
{
    LOCALE_IO dummy;

    wxString schPath = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) +
                       wxS( "issue24760/issue24760.kicad_sch" );

    std::unique_ptr<SCHEMATIC> sch( EESCHEMA_HELPERS::LoadSchematic( schPath, true, false ) );
    BOOST_REQUIRE( sch != nullptr );

    SCH_SHEET_LIST sheets = sch->Hierarchy();

    // Root sheet plus ten sub-sheets.
    BOOST_REQUIRE_EQUAL( sheets.size(), 11u );

    std::map<wxString, int> pageCounts;

    for( const SCH_SHEET_PATH& sheet : sheets )
    {
        wxString pageNumber = sheet.GetPageNumber();
        BOOST_CHECK_MESSAGE( !pageNumber.IsEmpty(),
                             "Sheet " << sheet.PathHumanReadable().ToStdString()
                                      << " has no page number" );
        pageCounts[pageNumber]++;
    }

    for( const auto& [pageNumber, count] : pageCounts )
    {
        BOOST_CHECK_MESSAGE( count == 1,
                             "Page number " << pageNumber.ToStdString() << " is shared by "
                                            << count << " sheets" );
    }

    BOOST_CHECK_EQUAL( pageCounts.size(), sheets.size() );
}
