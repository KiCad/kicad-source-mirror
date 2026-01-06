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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_issue22651_sheet_annotation.cpp
 * Test for issue #22651: Symbols are not reannotated when placing design blocks as sheets
 *
 * When placing a design block as a sheet with "Keep Annotations" disabled, symbols inside
 * the sheet should be reannotated. The root cause was that the schematic hierarchy cache
 * was not refreshed after adding the new sheet to the screen, so the annotation function
 * couldn't find the sheet or its symbols in the hierarchy.
 *
 * This test verifies that after refreshing the hierarchy, newly added sheets and their
 * symbols can be found via GetSheetsWithinPath and GetSymbolsWithinPath, which are the
 * mechanisms used by the annotation system.
 */

#include <boost/test/unit_test.hpp>

#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <sch_reference_list.h>
#include <settings/settings_manager.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>


struct SHEET_ANNOTATION_FIXTURE
{
    SHEET_ANNOTATION_FIXTURE() :
            m_settingsManager()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator()
                               + wxT( "test_sheet_annotation.kicad_pro" );
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_project = &m_settingsManager.Prj();
        m_schematic->SetProject( m_project );
    }

    ~SHEET_ANNOTATION_FIXTURE()
    {
        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }

        m_schematic.reset();
    }

    SETTINGS_MANAGER m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    PROJECT* m_project;
    std::vector<wxString> m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( Issue22651SheetAnnotation, SHEET_ANNOTATION_FIXTURE )


/**
 * Test that after adding a sheet and refreshing the hierarchy, the sheet path
 * can be found via GetSheetsWithinPath.
 */
BOOST_AUTO_TEST_CASE( TestSheetFoundInHierarchyAfterRefresh )
{
    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SHEET* rootSheet = topSheets[0];
    SCH_SCREEN* rootScreen = rootSheet->GetScreen();
    BOOST_REQUIRE( rootScreen != nullptr );

    // Create a new sub-sheet with its own screen
    SCH_SHEET* newSheet = new SCH_SHEET( rootSheet, VECTOR2I( 0, 0 ) );
    SCH_SCREEN* newScreen = new SCH_SCREEN( m_schematic.get() );
    newSheet->SetScreen( newScreen );
    newSheet->GetField( FIELD_T::SHEET_NAME )->SetText( wxT( "TestSubSheet" ) );
    newSheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( wxT( "test_subsheet.kicad_sch" ) );
    newScreen->SetFileName( wxT( "/tmp/test_subsheet.kicad_sch" ) );

    // Add a symbol to the sub-sheet's screen
    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    symbol->SetPosition( VECTOR2I( 1000000, 0 ) );
    newScreen->Append( symbol );

    // Get hierarchy BEFORE adding the sheet (simulating stale cache)
    SCH_SHEET_LIST hierarchyBefore = m_schematic->Hierarchy();
    size_t countBefore = hierarchyBefore.size();

    // Add the new sheet to the root screen
    rootScreen->Append( newSheet );

    // The cached hierarchy should still be stale at this point
    SCH_SHEET_LIST hierarchyStale = m_schematic->Hierarchy();
    BOOST_CHECK_EQUAL( hierarchyStale.size(), countBefore );

    // Refresh the hierarchy
    m_schematic->RefreshHierarchy();

    // Now the hierarchy should include the new sheet
    SCH_SHEET_LIST hierarchyAfter = m_schematic->Hierarchy();
    BOOST_CHECK_EQUAL( hierarchyAfter.size(), countBefore + 1 );

    BOOST_TEST_MESSAGE( "Test passed: Sheet found in hierarchy after refresh" );
}


/**
 * Test that symbols inside a newly added sheet can be found via GetSymbolsWithinPath
 * after refreshing the hierarchy, which is the mechanism used by annotation.
 */
BOOST_AUTO_TEST_CASE( TestSymbolsFoundInNewSheetAfterRefresh )
{
    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SHEET* rootSheet = topSheets[0];
    SCH_SCREEN* rootScreen = rootSheet->GetScreen();
    BOOST_REQUIRE( rootScreen != nullptr );

    // Create a new sub-sheet with its own screen
    SCH_SHEET* newSheet = new SCH_SHEET( rootSheet, VECTOR2I( 0, 0 ) );
    SCH_SCREEN* newScreen = new SCH_SCREEN( m_schematic.get() );
    newSheet->SetScreen( newScreen );
    newSheet->GetField( FIELD_T::SHEET_NAME )->SetText( wxT( "DesignBlockSheet" ) );
    newSheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( wxT( "design_block.kicad_sch" ) );
    newScreen->SetFileName( wxT( "/tmp/design_block.kicad_sch" ) );

    // Add two symbols to the sub-sheet's screen (simulating design block content)
    SCH_SYMBOL* symbol1 = new SCH_SYMBOL();
    symbol1->SetPosition( VECTOR2I( 0, 0 ) );
    newScreen->Append( symbol1 );

    SCH_SYMBOL* symbol2 = new SCH_SYMBOL();
    symbol2->SetPosition( VECTOR2I( 1000000, 0 ) );
    newScreen->Append( symbol2 );

    // Add the new sheet to the root screen
    rootScreen->Append( newSheet );

    // Refresh the hierarchy (this is the fix for issue 22651)
    m_schematic->RefreshHierarchy();

    // Build the sheet path for the new sheet
    SCH_SHEET_PATH currentPath;
    currentPath.push_back( rootSheet );

    SCH_SHEET_PATH newSheetPath = currentPath;
    newSheetPath.push_back( newSheet );

    // Use GetSheetsWithinPath to find the new sheet (as annotation does)
    SCH_SHEET_LIST hierarchy = m_schematic->Hierarchy();
    std::vector<SCH_SHEET_PATH> foundSheets;
    hierarchy.GetSheetsWithinPath( foundSheets, newSheetPath );

    BOOST_CHECK_EQUAL( foundSheets.size(), 1 );

    // Use GetSymbolsWithinPath to find symbols (as annotation does)
    // Pass true for aForceIncludeOrphanSymbols since test symbols don't have library refs
    SCH_REFERENCE_LIST references;
    hierarchy.GetSymbolsWithinPath( references, newSheetPath, false, true );

    BOOST_CHECK_EQUAL( references.GetCount(), 2 );

    BOOST_TEST_MESSAGE( "Test passed: Symbols found in new sheet after hierarchy refresh" );
}


/**
 * Test that without refreshing the hierarchy, symbols in newly added sheets
 * cannot be found. This verifies the root cause of issue 22651.
 */
BOOST_AUTO_TEST_CASE( TestSymbolsNotFoundWithoutRefresh )
{
    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SHEET* rootSheet = topSheets[0];
    SCH_SCREEN* rootScreen = rootSheet->GetScreen();
    BOOST_REQUIRE( rootScreen != nullptr );

    // Get hierarchy BEFORE adding any sheets
    SCH_SHEET_LIST hierarchyBefore = m_schematic->Hierarchy();

    // Create a new sub-sheet with its own screen
    SCH_SHEET* newSheet = new SCH_SHEET( rootSheet, VECTOR2I( 0, 0 ) );
    SCH_SCREEN* newScreen = new SCH_SCREEN( m_schematic.get() );
    newSheet->SetScreen( newScreen );
    newSheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( wxT( "stale_test.kicad_sch" ) );
    newScreen->SetFileName( wxT( "/tmp/stale_test.kicad_sch" ) );

    // Add a symbol to the sub-sheet's screen
    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    symbol->SetPosition( VECTOR2I( 0, 0 ) );
    newScreen->Append( symbol );

    // Add the new sheet to the root screen
    rootScreen->Append( newSheet );

    // Build the sheet path for the new sheet
    SCH_SHEET_PATH currentPath;
    currentPath.push_back( rootSheet );

    SCH_SHEET_PATH newSheetPath = currentPath;
    newSheetPath.push_back( newSheet );

    // WITHOUT refreshing the hierarchy, try to find symbols
    // This demonstrates the bug behavior before the fix
    std::vector<SCH_SHEET_PATH> foundSheets;
    hierarchyBefore.GetSheetsWithinPath( foundSheets, newSheetPath );

    // The stale hierarchy should NOT find the new sheet
    BOOST_CHECK_EQUAL( foundSheets.size(), 0 );

    SCH_REFERENCE_LIST references;
    hierarchyBefore.GetSymbolsWithinPath( references, newSheetPath, false, true );

    // The stale hierarchy should NOT find the symbols
    BOOST_CHECK_EQUAL( references.GetCount(), 0 );

    BOOST_TEST_MESSAGE( "Test passed: Stale hierarchy does not find symbols (confirms root cause)" );
}


BOOST_AUTO_TEST_SUITE_END()
