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
 * @file test_save_load_schematic.cpp
 * Test suite for saving and loading schematics with virtual root pattern
 */

#include <boost/test/unit_test.hpp>
#include <eeschema_test_utils.h>

#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <kiid.h>
#include <sch_file_versions.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>


struct SAVE_LOAD_FIXTURE
{
    SAVE_LOAD_FIXTURE() :
            m_settingsManager()
    {
        // Create a temporary project file so we have a valid project name
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxT("test_project.kicad_pro");
        m_tempFiles.push_back( projectPath );

        m_settingsManager.LoadProject( projectPath.ToStdString() );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_project = &m_settingsManager.Prj();
        m_schematic->SetProject( m_project );
    }

    ~SAVE_LOAD_FIXTURE()
    {
        // Clean up temp files
        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }

        m_schematic.reset();
    }

    wxString GetTempFileName( const wxString& aPrefix )
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString fileName = wxFileName::CreateTempFileName( tempDir + wxFileName::GetPathSeparator() + aPrefix );
        m_tempFiles.push_back( fileName );
        return fileName;
    }

    SETTINGS_MANAGER m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
    PROJECT* m_project;
    std::vector<wxString> m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( SaveLoadSchematic, SAVE_LOAD_FIXTURE )


/**
 * Test that we can save and reload a schematic with the virtual root pattern
 * and that symbol instances are preserved correctly
 */
BOOST_AUTO_TEST_CASE( TestSaveLoadSimpleSchematic )
{
    // Create a simple schematic
    m_schematic->CreateDefaultScreens();

    // Get the first (and only) top-level sheet
    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SCREEN* screen = topSheets[0]->GetScreen();
    BOOST_REQUIRE( screen != nullptr );

    // Set some basic properties
    screen->SetFileName( "test.kicad_sch" );
    topSheets[0]->SetName( "Main Sheet" );

    // Verify that currentSheet is set to a top-level sheet, not virtual root
    SCH_SHEET_PATH& currentSheet = m_schematic->CurrentSheet();
    BOOST_CHECK( !currentSheet.empty() );
    if( !currentSheet.empty() )
    {
        BOOST_CHECK( currentSheet.Last() != &m_schematic->Root() );
        BOOST_CHECK( currentSheet.Last()->m_Uuid != niluuid );
    }

    // Save the schematic
    wxString fileName = GetTempFileName( "test_schematic" );
    fileName += ".kicad_sch";
    m_tempFiles.push_back( fileName );

    SCH_IO_KICAD_SEXPR io;
    BOOST_CHECK_NO_THROW( io.SaveSchematicFile( fileName, topSheets[0], m_schematic.get() ) );

    // Verify file was created
    BOOST_CHECK( wxFileExists( fileName ) );

    // Now try to load it back
    m_schematic->Reset();
    SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );
    SCH_SHEET* loadedSheet = nullptr;
    BOOST_CHECK_NO_THROW( loadedSheet = io.LoadSchematicFile( fileName, m_schematic.get() ) );
    BOOST_CHECK( loadedSheet != nullptr );

    if( loadedSheet )
    {
        // Set it as root (which should create virtual root and make it a top-level sheet)
        m_schematic->AddTopLevelSheet( loadedSheet );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;

        // Verify we can access the schematic
        BOOST_CHECK( m_schematic->IsValid() );

        // Verify current sheet is NOT the virtual root
        SCH_SHEET_PATH& currentSheet2 = m_schematic->CurrentSheet();
        BOOST_CHECK( !currentSheet2.empty() );
        if( !currentSheet2.empty() )
        {
            BOOST_CHECK( currentSheet2.Last() != &m_schematic->Root() );
            BOOST_CHECK( currentSheet2.Last()->m_Uuid != niluuid );
        }

        // Try to save again (this is where the bug occurs)
        wxString fileName2 = GetTempFileName( "test_schematic_resave" );
        fileName2 += ".kicad_sch";
        m_tempFiles.push_back( fileName2 );

        std::vector<SCH_SHEET*> topSheets2 = m_schematic->GetTopLevelSheets();
        BOOST_REQUIRE( !topSheets2.empty() );

        // This should not assert - this is the key test for the bug fix
        // Symbol instances should be preserved, not dropped
        BOOST_CHECK_NO_THROW( io.SaveSchematicFile( fileName2, topSheets2[0], m_schematic.get() ) );

        // Verify the second file was created
        BOOST_CHECK( wxFileExists( fileName2 ) );
    }
}


/**
 * Test saving/loading a hierarchical schematic
 */
BOOST_AUTO_TEST_CASE( TestSaveLoadHierarchicalSchematic )
{
    // Create a schematic with hierarchy
    m_schematic->CreateDefaultScreens();

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    SCH_SCREEN* screen = topSheets[0]->GetScreen();
    BOOST_REQUIRE( screen != nullptr );

    // Create a sub-sheet
    SCH_SHEET* subSheet = new SCH_SHEET( m_schematic.get() );
    subSheet->SetName( "SubSheet" );
    SCH_SCREEN* subScreen = new SCH_SCREEN( m_schematic.get() );
    subSheet->SetScreen( subScreen );
    subSheet->SetFileName( "subsheet.kicad_sch" );
    subScreen->SetFileName( "subsheet.kicad_sch" );
    screen->Append( subSheet );

    // Save the main sheet
    wxString mainFileName = GetTempFileName( "test_main" );
    mainFileName += ".kicad_sch";
    m_tempFiles.push_back( mainFileName );

    // Save the sub-sheet
    wxString subFileName = GetTempFileName( "test_sub" );
    subFileName += ".kicad_sch";
    m_tempFiles.push_back( subFileName );

    SCH_IO_KICAD_SEXPR io;
    BOOST_CHECK_NO_THROW( io.SaveSchematicFile( mainFileName, topSheets[0], m_schematic.get() ) );
    BOOST_CHECK_NO_THROW( io.SaveSchematicFile( subFileName, subSheet, m_schematic.get() ) );

    // Load it back
    m_schematic->Reset();
    SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );
    SCH_SHEET* loadedSheet = nullptr;
    BOOST_CHECK_NO_THROW( loadedSheet = io.LoadSchematicFile( mainFileName, m_schematic.get() ) );
    BOOST_CHECK( loadedSheet != nullptr );

    if( loadedSheet )
    {
        m_schematic->AddTopLevelSheet( loadedSheet );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;

        // Build hierarchy
        m_schematic->RefreshHierarchy();

        // Verify hierarchy has 2 sheets (main + sub)
        SCH_SHEET_LIST hierarchy = m_schematic->Hierarchy();
        BOOST_CHECK_EQUAL( hierarchy.size(), 2 );

        // Try to save again
        wxString mainFileName2 = GetTempFileName( "test_main_resave" );
        mainFileName2 += ".kicad_sch";
        m_tempFiles.push_back( mainFileName2 );

        std::vector<SCH_SHEET*> topSheets2 = m_schematic->GetTopLevelSheets();
        BOOST_REQUIRE( !topSheets2.empty() );

        // This should not assert - this is the key test for the bug fix
        BOOST_CHECK_NO_THROW( io.SaveSchematicFile( mainFileName2, topSheets2[0], m_schematic.get() ) );

        // Verify the file was created
        BOOST_CHECK( wxFileExists( mainFileName2 ) );
    }
}


/**
 * Test that symbol instances are properly loaded and preserved from a legacy hierarchical schematic
 * This tests loading a schematic that was converted from the legacy format and has complex hierarchy
 * with the same sub-sheet used multiple times (shared sheet)
 */
BOOST_AUTO_TEST_CASE( TestLoadLegacyHierarchicalSchematic )
{
    // Load the legacy hierarchical schematic
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( "legacy_hierarchy" );
    fn.SetName( "legacy_hierarchy" );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );
    wxString mainFile = fn.GetFullPath();

    BOOST_TEST_MESSAGE( "Loading schematic: " + mainFile.ToStdString() );
    BOOST_REQUIRE( wxFileExists( mainFile ) );

    SCH_IO_KICAD_SEXPR io;
    SCH_SHEET* loadedSheet = nullptr;

    // Load the schematic
    BOOST_CHECK_NO_THROW( loadedSheet = io.LoadSchematicFile( mainFile, m_schematic.get() ) );
    BOOST_REQUIRE( loadedSheet != nullptr );

    BOOST_TEST_MESSAGE( "Loaded sheet UUID: " + loadedSheet->m_Uuid.AsString().ToStdString() );
    BOOST_TEST_MESSAGE( "Loaded sheet name: " + loadedSheet->GetName().ToStdString() );

    m_schematic->Reset();
    SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );
    m_schematic->AddTopLevelSheet( loadedSheet );
    m_schematic->RemoveTopLevelSheet( defaultSheet );
    delete defaultSheet;

    // Build the hierarchy
    m_schematic->RefreshHierarchy();
    SCH_SHEET_LIST hierarchy = m_schematic->Hierarchy();

    BOOST_TEST_MESSAGE( "Hierarchy size: " + std::to_string( hierarchy.size() ) );

    // This schematic has 1 root + 2 sub-sheets = 3 total sheets
    BOOST_REQUIRE_EQUAL( hierarchy.size(), 3 );

    // Check each sheet path
    for( size_t i = 0; i < hierarchy.size(); i++ )
    {
        const SCH_SHEET_PATH& path = hierarchy[i];
        BOOST_TEST_MESSAGE( "\nSheet path [" + std::to_string(i) + "]: " + path.PathHumanReadable( false ).ToStdString() );
        BOOST_TEST_MESSAGE( "  Path KIID: " + path.Path().AsString().ToStdString() );
        BOOST_TEST_MESSAGE( std::string("  Last screen: ") + (path.LastScreen() ? "YES" : "NO") );
        BOOST_TEST_MESSAGE( "  Path size: " + std::to_string( path.size() ) );

        // Verify path has a screen
        BOOST_CHECK( path.LastScreen() != nullptr );

        if( path.LastScreen() )
        {
            // Count symbols on this sheet
            int symbolCount = 0;
            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                symbolCount++;
            }
            BOOST_TEST_MESSAGE( "  Symbol count: " + std::to_string( symbolCount ) );
        }
    }

    // Check for missing instances (this should create them if needed)
    BOOST_TEST_MESSAGE( "\nChecking for missing symbol instances..." );
    hierarchy.CheckForMissingSymbolInstances( m_project->GetProjectName() );

    // Now verify that symbols have proper instance data
    std::map<wxString, int> referenceCount;
    int totalInstances = 0;
    int emptyPaths = 0;

    for( const SCH_SHEET_PATH& path : hierarchy )
    {
        if( !path.LastScreen() )
            continue;

        BOOST_TEST_MESSAGE( "\nVerifying instances for path: " + path.PathHumanReadable( false ).ToStdString() );

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            SCH_SYMBOL_INSTANCE symbolInstance;

            // Check if symbol has instance for this path
            bool hasInstance = symbol->GetInstance( symbolInstance, path.Path() );

            if( hasInstance )
            {
                totalInstances++;

                // Check that path is not empty
                if( symbolInstance.m_Path.empty() )
                {
                    emptyPaths++;
                    BOOST_TEST_MESSAGE( "  ERROR: Symbol " + symbol->m_Uuid.AsString().ToStdString() +
                                      " has EMPTY instance path!" );
                }
                else
                {
                    referenceCount[symbolInstance.m_Reference]++;
                    BOOST_TEST_MESSAGE( "  Symbol " + symbol->m_Uuid.AsString().ToStdString() +
                                      " -> " + symbolInstance.m_Reference.ToStdString() +
                                      " (path: " + symbolInstance.m_Path.AsString().ToStdString() + ")" );
                }
            }
            else
            {
                BOOST_TEST_MESSAGE( "  WARNING: Symbol " + symbol->m_Uuid.AsString().ToStdString() +
                                  " has NO instance for path " + path.Path().AsString().ToStdString() );
            }
        }
    }

    BOOST_TEST_MESSAGE( "\nSummary:" );
    BOOST_TEST_MESSAGE( "  Total instances: " + std::to_string( totalInstances ) );
    BOOST_TEST_MESSAGE( "  Empty paths: " + std::to_string( emptyPaths ) );
    BOOST_TEST_MESSAGE( "  Unique references: " + std::to_string( referenceCount.size() ) );

    // CRITICAL: No symbol instance should have an empty path
    BOOST_CHECK_EQUAL( emptyPaths, 0 );

    // The schematic should have instances for all symbols
    BOOST_CHECK( totalInstances > 0 );

    // Print reference counts for debugging
    BOOST_TEST_MESSAGE( "\nReference designator counts:" );
    for( const auto& pair : referenceCount )
    {
        BOOST_TEST_MESSAGE( "  " + pair.first.ToStdString() + ": " + std::to_string( pair.second ) );
    }

    // Try to save the schematic - this should NOT crash or assert
    BOOST_TEST_MESSAGE( "\nAttempting to save schematic..." );
    wxString saveFile = GetTempFileName( "legacy_hierarchy_resave" );
    saveFile += "." + FILEEXT::KiCadSchematicFileExtension;
    m_tempFiles.push_back( saveFile );

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();
    BOOST_REQUIRE( !topSheets.empty() );

    // This is the critical test - saving should not assert on empty paths
    BOOST_CHECK_NO_THROW( io.SaveSchematicFile( saveFile, topSheets[0], m_schematic.get() ) );
    BOOST_CHECK( wxFileExists( saveFile ) );

    BOOST_TEST_MESSAGE( "Save successful!" );
}


BOOST_AUTO_TEST_SUITE_END()
