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

/**
 * @file test_multi_top_level_sheets.cpp
 * Test suite for multiple top-level sheet functionality
 */

#include <boost/test/unit_test.hpp>

#include <sch_sheet.h>
#include <sch_screen.h>
#include <schematic.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <pgm_base.h>

static SCH_SHEET* createTopLevelSheet( SCHEMATIC& aSchematic, const wxString& aName,
                                       const wxString& aFileName )
{
    SCH_SHEET*  sheet = new SCH_SHEET( &aSchematic );
    SCH_SCREEN* screen = new SCH_SCREEN( &aSchematic );

    const_cast<KIID&>( sheet->m_Uuid ) = screen->GetUuid();
    sheet->SetScreen( screen );
    sheet->SetName( aName );
    sheet->SetFileName( aFileName );

    return sheet;
}

BOOST_AUTO_TEST_SUITE( MultiTopLevelSheets )

BOOST_AUTO_TEST_CASE( TestVirtualRootCreation )
{
    // Create a virtual root
    SCH_SHEET virtualRoot;
    const_cast<KIID&>( virtualRoot.m_Uuid ) = niluuid;
    virtualRoot.SetScreen( nullptr );

    // Verify it's recognized as virtual root
    BOOST_CHECK( virtualRoot.m_Uuid == niluuid );
    BOOST_CHECK( virtualRoot.GetScreen() == nullptr );
}

BOOST_AUTO_TEST_CASE( TestAddTopLevelSheet )
{
    SCHEMATIC schematic( nullptr );

    SCH_SHEET* sheet1 = createTopLevelSheet( schematic, "Sheet1", "sheet1.kicad_sch" );

    schematic.SetTopLevelSheets( { sheet1 } );

    // Verify the sheet was added
    const std::vector<SCH_SHEET*>& topSheets = schematic.GetTopLevelSheets();
    BOOST_CHECK_EQUAL( topSheets.size(), 1 );
    BOOST_CHECK_EQUAL( topSheets[0], sheet1 );
    BOOST_CHECK_EQUAL( topSheets[0]->GetName(), "Sheet1" );
    BOOST_CHECK_EQUAL( schematic.Hierarchy().size(), 1 );
}

BOOST_AUTO_TEST_CASE( TestAddMultipleTopLevelSheets )
{
    SCHEMATIC schematic( nullptr );

    SCH_SHEET* sheet1 = createTopLevelSheet( schematic, "Sheet1", "sheet1.kicad_sch" );
    schematic.SetTopLevelSheets( { sheet1 } );

    // Create and add multiple top-level sheets
    for( int i = 2; i <= 3; i++ )
    {
        SCH_SHEET* sheet = createTopLevelSheet( schematic, wxString::Format( "Sheet%d", i ),
                                                wxString::Format( "sheet%d.kicad_sch", i ) );
        schematic.AddTopLevelSheet( sheet );
    }

    // Verify all sheets were added
    const std::vector<SCH_SHEET*>& topSheets = schematic.GetTopLevelSheets();
    BOOST_CHECK_EQUAL( topSheets.size(), 3 );
    BOOST_CHECK_EQUAL( topSheets[0]->GetName(), "Sheet1" );
    BOOST_CHECK_EQUAL( topSheets[1]->GetName(), "Sheet2" );
    BOOST_CHECK_EQUAL( topSheets[2]->GetName(), "Sheet3" );
    BOOST_CHECK_EQUAL( schematic.Hierarchy().size(), 3 );
}

BOOST_AUTO_TEST_CASE( TestRemoveTopLevelSheet )
{
    SCHEMATIC schematic( nullptr );

    SCH_SHEET* sheet1 = createTopLevelSheet( schematic, "Sheet1", "sheet1.kicad_sch" );
    schematic.SetTopLevelSheets( { sheet1 } );

    SCH_SHEET* sheet2 = createTopLevelSheet( schematic, "Sheet2", "sheet2.kicad_sch" );
    schematic.AddTopLevelSheet( sheet2 );

    // Remove first sheet
    schematic.RemoveTopLevelSheet( sheet1 );

    // Verify only sheet2 remains
    const std::vector<SCH_SHEET*>& topSheets = schematic.GetTopLevelSheets();
    BOOST_CHECK_EQUAL( topSheets.size(), 1 );
    BOOST_CHECK_EQUAL( topSheets[0], sheet2 );
    BOOST_CHECK_EQUAL( topSheets[0]->GetName(), "Sheet2" );
    BOOST_CHECK_EQUAL( schematic.Hierarchy().size(), 1 );

    // Cannot remove the final remaining sheet
    BOOST_CHECK( !schematic.RemoveTopLevelSheet( topSheets[0] ) );
}

BOOST_AUTO_TEST_CASE( TestBuildSheetListWithMultipleRoots )
{
    SCHEMATIC schematic( nullptr );

    SCH_SHEET* top1 = createTopLevelSheet( schematic, "TopSheet1", "top_sheet_1.kicad_sch" );
    SCH_SHEET* top2 = createTopLevelSheet( schematic, "TopSheet2", "top_sheet_2.kicad_sch" );

    SCH_SHEET* child1 = createTopLevelSheet( schematic, "ChildSheet1", "child_sheet_1.kicad_sch" );
    child1->SetParent( top1 );
    top1->GetScreen()->Append( child1 );

    SCH_SHEET* child2 = createTopLevelSheet( schematic, "ChildSheet2", "child_sheet_2.kicad_sch" );
    child2->SetParent( top2 );
    top2->GetScreen()->Append( child2 );

    schematic.SetTopLevelSheets( { top1, top2 } );

    // Build sheet list
    SCH_SHEET_LIST sheetList = schematic.Hierarchy();

    // Should have 4 sheet paths: 2 top-level + 2 children
    BOOST_CHECK_EQUAL( sheetList.size(), 4 );
}


BOOST_AUTO_TEST_CASE( TestHierarchyUpdatesOnSheetOperations )
{
    SCHEMATIC schematic( nullptr );

    SCH_SHEET* baseSheet = createTopLevelSheet( schematic, "Base", "base.kicad_sch" );
    schematic.SetTopLevelSheets( { baseSheet } );

    BOOST_CHECK_EQUAL( schematic.Hierarchy().size(), 1 );

    SCH_SHEET* copiedSheet = createTopLevelSheet( schematic, "Copy", "copy.kicad_sch" );
    schematic.AddTopLevelSheet( copiedSheet );

    BOOST_CHECK_EQUAL( schematic.Hierarchy().size(), 2 );

    // Move the copied sheet ahead of the base sheet
    schematic.SetTopLevelSheets( { copiedSheet, baseSheet } );
    BOOST_CHECK_EQUAL( schematic.GetTopLevelSheet()->GetName(), "Copy" );
    BOOST_CHECK_EQUAL( schematic.Hierarchy().size(), 2 );

    // Removing one sheet should keep hierarchy valid
    BOOST_CHECK( schematic.RemoveTopLevelSheet( baseSheet ) );
    BOOST_CHECK_EQUAL( schematic.Hierarchy().size(), 1 );
    BOOST_CHECK_EQUAL( schematic.GetTopLevelSheet()->GetName(), "Copy" );
}

BOOST_AUTO_TEST_CASE( TestTopLevelSheetInfoSerialization )
{
    TOP_LEVEL_SHEET_INFO info1( KIID(), "TestSheet", "test_sheet.kicad_sch" );

    // Verify members
    BOOST_CHECK_EQUAL( info1.name, "TestSheet" );
    BOOST_CHECK_EQUAL( info1.filename, "test_sheet.kicad_sch" );

    // Test equality
    TOP_LEVEL_SHEET_INFO info2( info1.uuid, "TestSheet", "test_sheet.kicad_sch" );
    BOOST_CHECK( info1 == info2 );

    // Test inequality
    TOP_LEVEL_SHEET_INFO info3( KIID(), "OtherSheet", "other_sheet.kicad_sch" );
    BOOST_CHECK( info1 != info3 );
}

BOOST_AUTO_TEST_SUITE_END()
