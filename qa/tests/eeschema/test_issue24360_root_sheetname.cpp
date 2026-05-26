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
 */

// Regression tests for https://gitlab.com/kicad/code/kicad/-/issues/24360
//
// ${SHEETNAME} expanded to an empty string on the root sheet when the schematic
// was loaded through the CLI/headless path (e.g. `kicad-cli sch export pdf`).
// EESCHEMA_HELPERS::LoadSchematic now mirrors the interactive editor and names
// the root sheet so the title-block text variable resolves correctly.

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>

#include <eeschema_helpers.h>
#include <locale_io.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>


namespace
{

void checkRootSheetName( const wxString& aSchPath, bool aForceDefaultProject,
                         const wxString& aExpected )
{
    LOCALE_IO dummy;

    std::unique_ptr<SCHEMATIC> sch(
            EESCHEMA_HELPERS::LoadSchematic( aSchPath, true, aForceDefaultProject ) );
    BOOST_REQUIRE( sch != nullptr );

    SCH_SHEET* root = sch->GetTopLevelSheet();
    BOOST_REQUIRE( root != nullptr );
    BOOST_CHECK_EQUAL( root->GetName(), aExpected );

    SCH_SHEET_LIST hierarchy = sch->Hierarchy();
    BOOST_REQUIRE_EQUAL( hierarchy.size(), 1u );

    wxString token = wxS( "SHEETNAME" );
    BOOST_REQUIRE( sch->ResolveTextVar( &hierarchy.at( 0 ), &token, 0 ) );
    BOOST_CHECK_EQUAL( token, aExpected );
}

} // namespace


// No .kicad_pro alongside the schematic -> headless loader falls back to the translated "Root".
BOOST_AUTO_TEST_CASE( Issue24360_RootSheetNameFallback )
{
    wxString schPath = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() )
                       + wxS( "issue24360/issue24360.kicad_sch" );

    checkRootSheetName( schPath, /* aForceDefaultProject */ true, _( "Root" ) );
}


// .kicad_pro carries a schematic.top_level_sheets entry whose filename matches the loaded
// schematic.  The headless loader must honor that display name instead of overwriting with "Root".
BOOST_AUTO_TEST_CASE( Issue24360_RootSheetNameFromProject )
{
    wxString schPath = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() )
                       + wxS( "issue24360_project/issue24360_project.kicad_sch" );

    checkRootSheetName( schPath, /* aForceDefaultProject */ false,
                        wxS( "Custom Display Name" ) );
}
