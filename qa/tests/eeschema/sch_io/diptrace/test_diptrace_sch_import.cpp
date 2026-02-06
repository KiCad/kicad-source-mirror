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
 * @file test_diptrace_sch_import.cpp
 * Test suite for import of DipTrace schematic (.dch) files
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <eeschema/sch_io/diptrace/sch_io_diptrace.h>

#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <cmath>
#include <cstdlib>
#include <set>
#include <vector>

#include <wx/filefn.h>
#include <wx/xml/xml.h>


struct DIPTRACE_SCH_IMPORT_FIXTURE
{
    DIPTRACE_SCH_IMPORT_FIXTURE() :
            m_schematic( new SCHEMATIC( nullptr ) )
    {
        m_manager.LoadProject( "" );
        m_schematic->SetProject( &m_manager.Prj() );
        m_schematic->CurrentSheet().clear();
        m_schematic->CurrentSheet().push_back( &m_schematic->Root() );
    }

    SCH_IO_DIPTRACE m_plugin;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SETTINGS_MANAGER m_manager;
    SCH_SHEET* m_loadedRoot = nullptr;

    std::string GetTestDataDir()
    {
        return KI_TEST::GetEeschemaTestDataDir() + "plugins/diptrace/";
    }

    std::string GetViewerExamplesDir()
    {
        const char* examplesEnv = std::getenv( "DIPTRACE_VIEWER_EXAMPLES_DIR" );

        return examplesEnv && *examplesEnv
                ? examplesEnv
                : "/home/seth/Downloads/DipTrace Viewer/Examples";
    }

    int CountItemsOfType( KICAD_T aType )
    {
        std::set<SCH_SCREEN*> seenScreens;
        std::vector<SCH_SCREEN*> pendingScreens;
        int count = 0;

        if( m_loadedRoot && m_loadedRoot->GetScreen() )
            pendingScreens.push_back( m_loadedRoot->GetScreen() );

        while( !pendingScreens.empty() )
        {
            SCH_SCREEN* screen = pendingScreens.back();
            pendingScreens.pop_back();

            if( !screen || !seenScreens.insert( screen ).second )
                continue;

            for( SCH_ITEM* item : screen->Items() )
            {
                if( item->Type() == aType )
                    count++;

                if( item->Type() == SCH_SHEET_T )
                {
                    SCH_SHEET* subSheet = static_cast<SCH_SHEET*>( item );

                    if( subSheet->GetScreen() )
                        pendingScreens.push_back( subSheet->GetScreen() );
                }
            }
        }

        return count;
    }

    int CountImportedScreens()
    {
        std::set<SCH_SCREEN*> seenScreens;
        std::vector<SCH_SCREEN*> pendingScreens;

        if( m_loadedRoot && m_loadedRoot->GetScreen() )
            pendingScreens.push_back( m_loadedRoot->GetScreen() );

        while( !pendingScreens.empty() )
        {
            SCH_SCREEN* screen = pendingScreens.back();
            pendingScreens.pop_back();

            if( !screen || !seenScreens.insert( screen ).second )
                continue;

            for( SCH_ITEM* item : screen->Items() )
            {
                if( item->Type() != SCH_SHEET_T )
                    continue;

                SCH_SHEET* subSheet = static_cast<SCH_SHEET*>( item );

                if( subSheet->GetScreen() )
                    pendingScreens.push_back( subSheet->GetScreen() );
            }
        }

        return static_cast<int>( seenScreens.size() );
    }

    int MaxPinCountForRefdes( const wxString& aRefdes )
    {
        std::set<SCH_SCREEN*> seenScreens;
        std::vector<SCH_SCREEN*> pendingScreens;
        int maxPins = -1;

        if( m_loadedRoot && m_loadedRoot->GetScreen() )
            pendingScreens.push_back( m_loadedRoot->GetScreen() );

        while( !pendingScreens.empty() )
        {
            SCH_SCREEN* screen = pendingScreens.back();
            pendingScreens.pop_back();

            if( !screen || !seenScreens.insert( screen ).second )
                continue;

            for( SCH_ITEM* item : screen->Items() )
            {
                if( item->Type() == SCH_SYMBOL_T )
                {
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                    SCH_FIELD*  refField = symbol->GetField( FIELD_T::REFERENCE );

                    if( refField && refField->GetText() == aRefdes )
                    {
                        int pinCount = static_cast<int>( symbol->GetPins().size() );
                        maxPins = std::max( maxPins, pinCount );
                    }
                }
                else if( item->Type() == SCH_SHEET_T )
                {
                    SCH_SHEET* subSheet = static_cast<SCH_SHEET*>( item );

                    if( subSheet->GetScreen() )
                        pendingScreens.push_back( subSheet->GetScreen() );
                }
            }
        }

        return maxPins;
    }

    wxString GetFootprintForRefdes( const wxString& aRefdes )
    {
        std::set<SCH_SCREEN*>    seenScreens;
        std::vector<SCH_SCREEN*> pendingScreens;

        if( m_loadedRoot && m_loadedRoot->GetScreen() )
            pendingScreens.push_back( m_loadedRoot->GetScreen() );

        while( !pendingScreens.empty() )
        {
            SCH_SCREEN* screen = pendingScreens.back();
            pendingScreens.pop_back();

            if( !screen || !seenScreens.insert( screen ).second )
                continue;

            for( SCH_ITEM* item : screen->Items() )
            {
                if( item->Type() == SCH_SYMBOL_T )
                {
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                    SCH_FIELD*  refField = symbol->GetField( FIELD_T::REFERENCE );

                    if( refField && refField->GetText() == aRefdes )
                    {
                        SCH_FIELD* fpField = symbol->GetField( FIELD_T::FOOTPRINT );
                        return fpField ? fpField->GetText() : wxString();
                    }
                }
                else if( item->Type() == SCH_SHEET_T )
                {
                    SCH_SHEET* subSheet = static_cast<SCH_SHEET*>( item );

                    if( subSheet->GetScreen() )
                        pendingScreens.push_back( subSheet->GetScreen() );
                }
            }
        }

        return wxString();
    }

    struct DCH_XML_COUNTS
    {
        int partCount  = 0;
        int sheetCount = 0;
    };

    bool LoadDchXmlCounts( const std::string& aXmlPath, DCH_XML_COUNTS& aCounts )
    {
        if( !wxFileExists( aXmlPath ) )
            return false;

        wxXmlDocument doc;

        if( !doc.Load( aXmlPath ) )
            return false;

        wxXmlNode* root = doc.GetRoot();

        if( !root || root->GetName() != "Source" )
            return false;

        wxXmlNode* schematicNode = nullptr;

        for( wxXmlNode* child = root->GetChildren(); child; child = child->GetNext() )
        {
            if( child->GetType() == wxXML_ELEMENT_NODE && child->GetName() == "Schematic" )
            {
                schematicNode = child;
                break;
            }
        }

        if( !schematicNode )
            return false;

        int                   partCount = 0;
        int                   sheetCount = 0;
        std::vector<wxXmlNode*> stack = { schematicNode };

        while( !stack.empty() )
        {
            wxXmlNode* node = stack.back();
            stack.pop_back();

            if( !node || node->GetType() != wxXML_ELEMENT_NODE )
                continue;

            if( node->GetName() == "Part" )
                partCount++;
            else if( node->GetName() == "Sheet" )
                sheetCount++;

            for( wxXmlNode* child = node->GetChildren(); child; child = child->GetNext() )
            {
                stack.push_back( child );
            }
        }

        aCounts.partCount = partCount;
        aCounts.sheetCount = sheetCount;
        return true;
    }

    SCH_SHEET* LoadDipTraceSchematic( const std::string& aFilePath )
    {
        m_schematic.reset( new SCHEMATIC( nullptr ) );
        m_manager.LoadProject( "" );
        m_schematic->SetProject( &m_manager.Prj() );
        m_schematic->CurrentSheet().clear();
        m_schematic->CurrentSheet().push_back( &m_schematic->Root() );

        m_loadedRoot = m_plugin.LoadSchematicFile( aFilePath, m_schematic.get() );
        return m_loadedRoot;
    }
};


BOOST_FIXTURE_TEST_SUITE( DipTraceSchImport, DIPTRACE_SCH_IMPORT_FIXTURE )


/**
 * Test that CanReadSchematicFile correctly identifies DipTrace .dch files
 * by their magic header bytes.
 */
BOOST_AUTO_TEST_CASE( CanReadSchematic )
{
    BOOST_CHECK( m_plugin.CanReadSchematicFile( GetTestDataDir() + "z80_board.dch" ) );
    BOOST_CHECK( m_plugin.CanReadSchematicFile( GetTestDataDir() + "power_supply.dch" ) );
    BOOST_CHECK( m_plugin.CanReadSchematicFile( GetTestDataDir() + "pppp.dch" ) );
}


BOOST_AUTO_TEST_CASE( CanReadLegacyHeaderOptional )
{
    const std::string examplesDir = GetViewerExamplesDir();
    const std::string sch2 = examplesDir + "/Schematic_2.dch";
    const std::string sch4 = examplesDir + "/Schematic_4.dch";

    if( !wxFileExists( sch2 ) || !wxFileExists( sch4 ) )
    {
        BOOST_TEST_MESSAGE( "Skipping legacy header check; viewer examples not found at "
                            << examplesDir );
        return;
    }

    BOOST_CHECK( m_plugin.CanReadSchematicFile( sch2 ) );
    BOOST_CHECK( m_plugin.CanReadSchematicFile( sch4 ) );
}


BOOST_AUTO_TEST_CASE( ViewerExamplesLoadOptional )
{
    const std::string examplesDir = GetViewerExamplesDir();

    struct EXPECTED
    {
        const char* fileName;
        int         minSymbols;
        int         minScreens;
    };

    const std::vector<EXPECTED> expected = {
        { "CNC_controller.dch", 100, 3 },
        { "Schematic_2.dch", 50, 1 },
        { "Schematic_4.dch", 70, 1 },
        { "Schematic_6.dch", 120, 1 },
    };

    bool foundAny = false;

    for( const EXPECTED& e : expected )
    {
        const std::string path = examplesDir + "/" + e.fileName;

        if( !wxFileExists( path ) )
            continue;

        foundAny = true;

        SCH_SHEET* root = nullptr;

        try
        {
            root = LoadDipTraceSchematic( path );
        }
        catch( const std::exception& eex )
        {
            BOOST_FAIL( "Failed to load " << path << ": " << eex.what() );
        }
        catch( ... )
        {
            BOOST_FAIL( "Failed to load " << path << ": unknown exception" );
        }

        BOOST_REQUIRE_MESSAGE( root != nullptr, "Failed to load " << path );
        BOOST_REQUIRE( root->GetScreen() );

        const int symbolCount = CountItemsOfType( SCH_SYMBOL_T );
        const int screenCount = CountImportedScreens();

        BOOST_CHECK_GE( symbolCount, e.minSymbols );
        BOOST_CHECK_GE( screenCount, e.minScreens );
    }

    if( !foundAny )
    {
        BOOST_TEST_MESSAGE( "Skipping external example load test; no .dch files found at "
                            << examplesDir );
    }
}


BOOST_AUTO_TEST_CASE( ViewerExamplesDchXmlParityOptional )
{
    const std::string examplesDir = GetViewerExamplesDir();

    struct EXPECTED
    {
        const char* dchFile;
        const char* dchXmlFile;
        double      minPartCoverage;
    };

    const std::vector<EXPECTED> expected = {
        { "CNC_controller.dch", "CNC_controller.dchxml", 0.85 },
        { "Schematic_2.dch", "PCB_2.dchxml", 0.95 },
        { "Schematic_4.dch", "PCB_4.dchxml", 0.95 },
        { "Schematic_6.dch", "PCB_6.dchxml", 0.95 },
    };

    bool foundAny = false;

    for( const EXPECTED& e : expected )
    {
        const std::string dchPath = examplesDir + "/" + e.dchFile;
        const std::string xmlPath = examplesDir + "/" + e.dchXmlFile;

        if( !wxFileExists( dchPath ) || !wxFileExists( xmlPath ) )
            continue;

        foundAny = true;

        DCH_XML_COUNTS xmlCounts;
        BOOST_REQUIRE_MESSAGE( LoadDchXmlCounts( xmlPath, xmlCounts ),
                               "Failed to parse " << xmlPath );
        BOOST_REQUIRE_GT( xmlCounts.partCount, 0 );
        BOOST_REQUIRE_GT( xmlCounts.sheetCount, 0 );

        SCH_SHEET* root = nullptr;

        try
        {
            root = LoadDipTraceSchematic( dchPath );
        }
        catch( const std::exception& eex )
        {
            BOOST_FAIL( "Failed to load " << dchPath << ": " << eex.what() );
        }
        catch( ... )
        {
            BOOST_FAIL( "Failed to load " << dchPath << ": unknown exception" );
        }

        BOOST_REQUIRE( root != nullptr );
        BOOST_REQUIRE( root->GetScreen() );

        const int importedSymbols = CountItemsOfType( SCH_SYMBOL_T );
        const int importedScreens = CountImportedScreens();
        const int minParts = static_cast<int>( std::floor(
                xmlCounts.partCount * e.minPartCoverage ) );

        BOOST_CHECK_EQUAL( importedScreens, xmlCounts.sheetCount );
        BOOST_CHECK_LE( importedSymbols, xmlCounts.partCount + 2 );
        BOOST_CHECK_GE( importedSymbols, minParts );
    }

    if( !foundAny )
    {
        BOOST_TEST_MESSAGE( "Skipping DCH/XML parity test; no matched files found at "
                            << examplesDir );
    }
}


BOOST_AUTO_TEST_CASE( ViewerExamplesPinCountsOptional )
{
    const std::string examplesDir = GetViewerExamplesDir();
    const std::string cncPath     = examplesDir + "/CNC_controller.dch";
    const std::string sch6Path    = examplesDir + "/Schematic_6.dch";

    if( !wxFileExists( cncPath ) || !wxFileExists( sch6Path ) )
    {
        BOOST_TEST_MESSAGE( "Skipping pin-count checks; required viewer examples not found at "
                            << examplesDir );
        return;
    }

    {
        SCH_SHEET* root = LoadDipTraceSchematic( cncPath );
        BOOST_REQUIRE( root != nullptr );
        BOOST_REQUIRE( root->GetScreen() );
        BOOST_CHECK_EQUAL( MaxPinCountForRefdes( wxT( "C17" ) ), 2 );
    }

    {
        SCH_SHEET* root = LoadDipTraceSchematic( sch6Path );
        BOOST_REQUIRE( root != nullptr );
        BOOST_REQUIRE( root->GetScreen() );
        BOOST_CHECK_EQUAL( MaxPinCountForRefdes( wxT( "U1" ) ), 34 );
        BOOST_CHECK_EQUAL( MaxPinCountForRefdes( wxT( "U5" ) ), 72 );
        BOOST_CHECK_EQUAL( MaxPinCountForRefdes( wxT( "U10" ) ), 9 );
    }
}


BOOST_AUTO_TEST_CASE( ViewerExamplesFootprintFieldOptional )
{
    const std::string examplesDir = GetViewerExamplesDir();
    const std::string sch6Path   = examplesDir + "/Schematic_6.dch";

    if( !wxFileExists( sch6Path ) )
    {
        BOOST_TEST_MESSAGE( "Skipping footprint field checks; Schematic_6.dch not found at "
                            << examplesDir );
        return;
    }

    SCH_SHEET* root = LoadDipTraceSchematic( sch6Path );
    BOOST_REQUIRE( root != nullptr );
    BOOST_REQUIRE( root->GetScreen() );

    // Schematic_6 (v41) contains resistors and capacitors with embedded patterns.
    // Pattern names confirmed against PCB_6.dchxml library definitions.
    BOOST_CHECK_EQUAL( GetFootprintForRefdes( wxT( "C1" ) ), wxString( wxT( "CAP_2012_N" ) ) );
    BOOST_CHECK_EQUAL( GetFootprintForRefdes( wxT( "R1" ) ), wxString( wxT( "RES_2012_N" ) ) );

    // Verify several more to confirm consistency
    BOOST_CHECK_EQUAL( GetFootprintForRefdes( wxT( "C10" ) ), wxString( wxT( "CAP_2012_N" ) ) );
    BOOST_CHECK_EQUAL( GetFootprintForRefdes( wxT( "R10" ) ), wxString( wxT( "RES_2012_N" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
