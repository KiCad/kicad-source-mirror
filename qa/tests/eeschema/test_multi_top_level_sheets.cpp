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

    // Create a virtual root
    SCH_SHEET* virtualRoot = new SCH_SHEET();
    const_cast<KIID&>( virtualRoot->m_Uuid ) = niluuid;
    virtualRoot->SetScreen( nullptr );
    virtualRoot->SetParent( &schematic );
    schematic.SetRoot( virtualRoot );

    // Create and add a top-level sheet
    SCH_SHEET* sheet1 = new SCH_SHEET();
    sheet1->SetName( "Sheet1" );
    SCH_SCREEN* screen1 = new SCH_SCREEN();
    screen1->SetParent( &schematic );
    sheet1->SetScreen( screen1 );
    sheet1->SetParent( &schematic );

    schematic.AddTopLevelSheet( sheet1 );

    // Verify the sheet was added
    const std::vector<SCH_SHEET*>& topSheets = schematic.GetTopLevelSheets();
    BOOST_CHECK_EQUAL( topSheets.size(), 1 );
    BOOST_CHECK_EQUAL( topSheets[0], sheet1 );
    BOOST_CHECK_EQUAL( topSheets[0]->GetName(), "Sheet1" );
}

BOOST_AUTO_TEST_CASE( TestAddMultipleTopLevelSheets )
{
    SCHEMATIC schematic( nullptr );

    // Create a virtual root
    SCH_SHEET* virtualRoot = new SCH_SHEET();
    const_cast<KIID&>( virtualRoot->m_Uuid ) = niluuid;
    virtualRoot->SetScreen( nullptr );
    virtualRoot->SetParent( &schematic );
    schematic.SetRoot( virtualRoot );

    // Create and add multiple top-level sheets
    for( int i = 1; i <= 3; i++ )
    {
        SCH_SHEET* sheet = new SCH_SHEET();
        sheet->SetName( wxString::Format( "Sheet%d", i ) );
        SCH_SCREEN* screen = new SCH_SCREEN();
        screen->SetParent( &schematic );
        sheet->SetScreen( screen );
        sheet->SetParent( &schematic );
        schematic.AddTopLevelSheet( sheet );
    }

    // Verify all sheets were added
    const std::vector<SCH_SHEET*>& topSheets = schematic.GetTopLevelSheets();
    BOOST_CHECK_EQUAL( topSheets.size(), 3 );
    BOOST_CHECK_EQUAL( topSheets[0]->GetName(), "Sheet1" );
    BOOST_CHECK_EQUAL( topSheets[1]->GetName(), "Sheet2" );
    BOOST_CHECK_EQUAL( topSheets[2]->GetName(), "Sheet3" );
}

BOOST_AUTO_TEST_CASE( TestRemoveTopLevelSheet )
{
    SCHEMATIC schematic( nullptr );

    // Create a virtual root
    SCH_SHEET* virtualRoot = new SCH_SHEET();
    const_cast<KIID&>( virtualRoot->m_Uuid ) = niluuid;
    virtualRoot->SetScreen( nullptr );
    virtualRoot->SetParent( &schematic );
    schematic.SetRoot( virtualRoot );

    // Create and add sheets
    SCH_SHEET* sheet1 = new SCH_SHEET();
    sheet1->SetName( "Sheet1" );
    SCH_SCREEN* screen1 = new SCH_SCREEN();
    screen1->SetParent( &schematic );
    sheet1->SetScreen( screen1 );
    sheet1->SetParent( &schematic );
    schematic.AddTopLevelSheet( sheet1 );

    SCH_SHEET* sheet2 = new SCH_SHEET();
    sheet2->SetName( "Sheet2" );
    SCH_SCREEN* screen2 = new SCH_SCREEN();
    screen2->SetParent( &schematic );
    sheet2->SetScreen( screen2 );
    sheet2->SetParent( &schematic );
    schematic.AddTopLevelSheet( sheet2 );

    // Remove first sheet
    schematic.RemoveTopLevelSheet( sheet1 );

    // Verify only sheet2 remains
    const std::vector<SCH_SHEET*>& topSheets = schematic.GetTopLevelSheets();
    BOOST_CHECK_EQUAL( topSheets.size(), 1 );
    BOOST_CHECK_EQUAL( topSheets[0], sheet2 );
    BOOST_CHECK_EQUAL( topSheets[0]->GetName(), "Sheet2" );
}

BOOST_AUTO_TEST_CASE( TestBuildSheetListWithMultipleRoots )
{
    SCHEMATIC schematic( nullptr );

    // Create a virtual root
    SCH_SHEET* virtualRoot = new SCH_SHEET();
    const_cast<KIID&>( virtualRoot->m_Uuid ) = niluuid;
    virtualRoot->SetScreen( nullptr );
    virtualRoot->SetParent( &schematic );
    schematic.SetRoot( virtualRoot );

    // Create two top-level sheets, each with one child
    for( int i = 1; i <= 2; i++ )
    {
        SCH_SHEET* topSheet = new SCH_SHEET();
        topSheet->SetName( wxString::Format( "TopSheet%d", i ) );
        topSheet->SetFileName( wxString::Format( "top_sheet_%d.kicad_sch", i ) );
        SCH_SCREEN* topScreen = new SCH_SCREEN();
        topScreen->SetParent( &schematic );
        topSheet->SetScreen( topScreen );
        topSheet->SetParent( &schematic );

        // Add a child sheet
        SCH_SHEET* childSheet = new SCH_SHEET();
        childSheet->SetName( wxString::Format( "ChildSheet%d", i ) );
        childSheet->SetFileName( wxString::Format( "child_sheet_%d.kicad_sch", i ) );
        SCH_SCREEN* childScreen = new SCH_SCREEN();
        childScreen->SetParent( &schematic );
        childSheet->SetScreen( childScreen );
        childSheet->SetParent( &schematic );
        topScreen->Append( childSheet );

        schematic.AddTopLevelSheet( topSheet );
    }

    // Build sheet list
    schematic.RefreshHierarchy();
    SCH_SHEET_LIST sheetList = schematic.Hierarchy();

    // Should have 4 sheet paths: 2 top-level + 2 children
    BOOST_CHECK_EQUAL( sheetList.size(), 4 );
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
