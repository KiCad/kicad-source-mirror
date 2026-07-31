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

#include <filesystem>
#include <iostream>
#include <string>

#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <board.h>
#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid.hpp>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_edit_frame.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_io/pcb_io.h>
#include <pcb_marker.h>
#include <pcb_track.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <project.h>
#include <project/project_file.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <wx/string.h>

struct FileCleaner
{
    std::vector<wxString> m_files_to_delete;

    FileCleaner() = default;
    ~FileCleaner()
    {
        for( const auto& f_path : m_files_to_delete )
        {
            if( wxFileName::Exists( f_path ) )
            {
                if( !wxRemoveFile( f_path ) )
                {
                    BOOST_TEST_MESSAGE( "Warning: Failed to delete temporary file " << f_path );
                }
            }
        }
    }

    void AddFile( const wxString& f_path ) { m_files_to_delete.push_back( f_path ); }
};

struct DRC_BASE_FIXTURE
{
    DRC_BASE_FIXTURE()
    {
    }

    std::string generate_uuid();
    bool        SaveBoardToFile( BOARD* board, const wxString& filename );
    void        loadBoardAndVerifyInitialExclusions( const wxString& aBoardNameStem, int aExpectedInitialExclusions );
    void        createAndVerifyInitialExclusionMarkers();
    int         createAndVerifyAdditionalUnconnectedExclusions( int aAdditionalExclusions, int aInitialExclusions );
    void        runDrcOnBoard();
    void        saveBoardAndProjectToTempFiles( const wxString& aBoardNameStem, FileCleaner& aCleaner,
                                                wxString& aTempBoardFullPath, wxString& aTempProjectFullPath,
                                                wxString& aTempBoardStemName, bool aCopyBoardVerbatim = false );
    void        reloadBoardAndVerifyExclusions( const wxString& aTempBoardStemName, int aExpectedExclusions );


    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};

struct DRC_REGRESSION_TEST_FIXTURE : public DRC_BASE_FIXTURE
{
    DRC_REGRESSION_TEST_FIXTURE() :
            DRC_BASE_FIXTURE()
    {
    }
};


struct DRC_UNCONNECTED_SAVE_FIXTURE : public DRC_BASE_FIXTURE
{
    DRC_UNCONNECTED_SAVE_FIXTURE() :
            DRC_BASE_FIXTURE()
    {
        m_board = std::make_unique<BOARD>();
    }
};

std::string DRC_BASE_FIXTURE::generate_uuid()
{
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return boost::uuids::to_string( uuid );
}

void DRC_BASE_FIXTURE::loadBoardAndVerifyInitialExclusions( const wxString& aBoardNameStem,
                                                            int             aExpectedInitialExclusions )
{
    KI_TEST::LoadBoard( m_settingsManager, aBoardNameStem, m_board );
    BOOST_REQUIRE_MESSAGE( m_board,
                           "Could not load board " + aBoardNameStem ); // Ensure board loaded from test data directory
    PROJECT* pcb_project = m_board->GetProject();
    BOOST_REQUIRE_MESSAGE( pcb_project, "Get project pointer after initial loading." );

    // Board test file comes with initial exclusions, check if they are preserved after loading
    const BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    size_t                       initialExclusionsCount = bds.m_DrcExclusions.size();
    size_t                       initialExclusionsCommentsCount = bds.m_DrcExclusionComments.size();
    BOOST_TEST_MESSAGE( "Initial DRC exclusions count: " << initialExclusionsCount );
    BOOST_CHECK_EQUAL( initialExclusionsCount, (size_t) aExpectedInitialExclusions );
    BOOST_TEST_MESSAGE( "Initial DRC exclusion comments count: " << initialExclusionsCommentsCount );
    BOOST_CHECK_EQUAL( initialExclusionsCommentsCount, (size_t) aExpectedInitialExclusions );
}

void DRC_BASE_FIXTURE::createAndVerifyInitialExclusionMarkers()
{
    const BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    std::vector<PCB_MARKER*>     markers;
    for( const wxString exclusion : bds.m_DrcExclusions )
    {
        PCB_MARKER* marker = PCB_MARKER::DeserializeFromString( exclusion );
        if( marker )
        {
            wxString comment = bds.m_DrcExclusionComments.at( exclusion );
            marker->SetExcluded( true, comment );
            markers.push_back( marker );
            m_board->Add( marker );
        }
    }
    size_t actualExclusionsCount = bds.m_DrcExclusions.size();
    size_t initialExclusionsCount = markers.size();
    BOOST_CHECK_EQUAL( actualExclusionsCount, initialExclusionsCount );
    BOOST_TEST_MESSAGE( std::string( "Actual DRC exclusions count: " ) + std::to_string( actualExclusionsCount )
                        + " after adding initial markers." );
}

int DRC_BASE_FIXTURE::createAndVerifyAdditionalUnconnectedExclusions( int aAdditionalExclusions,
                                                                      int aInitialExclusions )
{
    for( int i = 0; i < aAdditionalExclusions; ++i )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_UNCONNECTED_ITEMS );
        wxString                  id1 = wxString( generate_uuid() );
        wxString                  id2 = wxString( generate_uuid() );
        drcItem->SetItems( KIID( id1 ), KIID( id2 ) );

        PCB_MARKER* marker = new PCB_MARKER( drcItem, VECTOR2I( 1000 * i, 1000 * i ) );
        m_board->Add( marker );

        // Exclude odd-numbered markers
        if( i % 2 == 1 )
        {
            marker->SetExcluded( true, wxString::Format( "Exclusion %d", i ) );
        }
    }

    // Store the new exclusion markers in the board
    m_board->RecordDRCExclusions();

    // Verify the number of exclusions after adding unconnected items
    const BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    const int                    expectedExclusions =
            aInitialExclusions + aAdditionalExclusions / 2; // Only odd-numbered markers are excluded
    size_t newActualExclusionsCount = bds.m_DrcExclusions.size();
    BOOST_TEST_MESSAGE( std::string( "New actual DRC exclusions count: " ) + std::to_string( newActualExclusionsCount )
                        + " after adding unconnected items." );
    BOOST_CHECK_EQUAL( newActualExclusionsCount, (size_t) expectedExclusions );
    return expectedExclusions;
}

void DRC_BASE_FIXTURE::runDrcOnBoard()
{
    BOOST_TEST_MESSAGE( "Running DRC on board." );
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.m_DRCEngine->InitEngine( wxFileName() );
    m_board->RecordDRCExclusions();
    bool runDRC = true;
    bool runDRCOnAllLayers = true;
    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, runDRC, runDRCOnAllLayers );
    m_board->ResolveDRCExclusions( false );
    BOOST_TEST_MESSAGE( "DRC done." );
}

void DRC_BASE_FIXTURE::saveBoardAndProjectToTempFiles( const wxString& aBoardNameStem, FileCleaner& aCleaner,
                                                       wxString& aTempBoardFullPath, wxString& aTempProjectFullPath,
                                                       wxString& aTempBoardStemName, bool aCopyBoardVerbatim )
{
    wxString tempPrefix = "tmp_test_drc_";
    aTempBoardStemName = tempPrefix + aBoardNameStem.ToStdString();
    aTempBoardFullPath = KI_TEST::GetPcbnewTestDataDir() + aTempBoardStemName + ".kicad_pcb";
    aCleaner.AddFile( aTempBoardFullPath );
    wxString tempProjectStemName = tempPrefix + aBoardNameStem.ToStdString();
    aTempProjectFullPath = KI_TEST::GetPcbnewTestDataDir() + aTempBoardStemName + ".kicad_pro";
    aCleaner.AddFile( aTempProjectFullPath );

    bool boardSaved;

    if( aCopyBoardVerbatim )
    {
        // Re-saving perturbs geometry slightly, which moves violation positions
        boardSaved = wxCopyFile( KI_TEST::GetPcbnewTestDataDir() + aBoardNameStem + ".kicad_pcb", aTempBoardFullPath );
    }
    else
    {
        boardSaved = SaveBoardToFile( m_board->GetBoard(), aTempBoardFullPath );
    }

    BOOST_REQUIRE_MESSAGE( boardSaved, "Save board to temporary file: " << aTempBoardFullPath );

    m_settingsManager.SaveProjectAs( aTempProjectFullPath, m_board->GetProject() );
    BOOST_REQUIRE_MESSAGE( wxFileName::Exists( aTempProjectFullPath ),
                           "Save project to temporary file: " << aTempProjectFullPath );
}

void DRC_BASE_FIXTURE::reloadBoardAndVerifyExclusions( const wxString& aTempBoardStemName, int aExpectedExclusions )
{
    // clear the current board to ensure a fresh load
    m_board.reset();

    KI_TEST::LoadBoard( m_settingsManager, aTempBoardStemName, m_board );
    BOOST_REQUIRE_MESSAGE( m_board, "Could not load board from tempfile:"
                                            + aTempBoardStemName ); // Ensure board loaded from test data directory
    PROJECT* pcb_project = m_board->GetProject();
    BOOST_REQUIRE_MESSAGE( pcb_project, "Get project pointer after initial loading." );

    BOARD_DESIGN_SETTINGS& reloaded_bds = m_board->GetDesignSettings();
    size_t                 reloadedExclusionsCount = reloaded_bds.m_DrcExclusions.size();
    BOOST_TEST_MESSAGE( "Reloaded DRC exclusions count: " << reloadedExclusionsCount );
    BOOST_CHECK_EQUAL( reloadedExclusionsCount, aExpectedExclusions );
}

bool DRC_BASE_FIXTURE::SaveBoardToFile( BOARD* board, const wxString& filename )
{
    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );
        pi->SaveBoard( filename, board, nullptr );
        return true;
    }
    catch( const IO_ERROR& error )
    {
        BOOST_TEST_MESSAGE( wxString::Format( "Save board to %s: %s", filename, error.What() ) );
        return false;
    }
}
BOOST_FIXTURE_TEST_CASE( DRCUnconnectedExclusionsLoss, DRC_UNCONNECTED_SAVE_FIXTURE )
{
    // Test that unconnected item exclusions are not lost after multiple DRC runs.
    // This test is expected to fail if the bug (issue17429) is present.

    std::vector<std::pair<wxString, int>> tests = {
        { "issue17429", 10 }, // board name stem, expected initial exclusions
    };

    const int NUM_DRC_RUNS = 2;

    for( const std::pair<wxString, int>& test_params : tests )
    {
        wxString boardNameStem = test_params.first;
        int      expectedInitialExclusions = test_params.second;

        loadBoardAndVerifyInitialExclusions( boardNameStem, expectedInitialExclusions );
        createAndVerifyInitialExclusionMarkers();
        const int additionalExclusions = 5;
        int       expectedExclusions =
                createAndVerifyAdditionalUnconnectedExclusions( additionalExclusions, expectedInitialExclusions );

        runDrcOnBoard();

        const BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
        BOOST_TEST_MESSAGE( std::string( "DRC exclusions count after DRC run: " ) + std::to_string( expectedExclusions )
                            + " after adding unconnected items." );
        BOOST_CHECK_EQUAL( bds.m_DrcExclusions.size(), expectedExclusions );
    }
}

BOOST_FIXTURE_TEST_CASE( DRCUnconnectedItemsExclusionsSaveLoad, DRC_REGRESSION_TEST_FIXTURE )
{
    namespace fs = std::filesystem;

    // Test that unconnected item exclusions are not lost during save/load.
    // This test is expected to fail if the bug (issue17429) is present.

    std::vector<std::pair<wxString, int>> tests = {
        { "issue17429", 10 }, // board name stem, expected initial exclusions
    };


    for( const std::pair<wxString, int>& test_params : tests )
    {
        FileCleaner tempFileCleaner;
        wxString    boardNameStem = test_params.first;
        int         expectedInitialExclusions = test_params.second;

        loadBoardAndVerifyInitialExclusions( boardNameStem, expectedInitialExclusions );

        wxString tempBoardFullPath, tempProjectFullPath, tempBoardStemName;
        saveBoardAndProjectToTempFiles( boardNameStem, tempFileCleaner, tempBoardFullPath, tempProjectFullPath,
                                        tempBoardStemName );

        createAndVerifyInitialExclusionMarkers();

        const int additionalExclusions = 5;
        int       expectedExclusions =
                createAndVerifyAdditionalUnconnectedExclusions( additionalExclusions, expectedInitialExclusions );

        bool boardSaved = SaveBoardToFile( m_board->GetBoard(), tempBoardFullPath );
        BOOST_REQUIRE_MESSAGE( boardSaved, "Save board to temporary file: " << tempBoardFullPath );

        m_settingsManager.SaveProjectAs( tempProjectFullPath, m_board->GetProject() );
        BOOST_REQUIRE_MESSAGE( wxFileName::Exists( tempProjectFullPath ),
                               "Save project to temporary file: " << tempProjectFullPath );

        reloadBoardAndVerifyExclusions( tempBoardStemName, expectedExclusions );
    }
}


static void runDrcAndCreateMarkers( BOARD* aBoard )
{
    aBoard->DeleteMARKERs();

    BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

    bds.m_DRCEngine->InitEngine( wxFileName() );
    bds.m_DRCEngine->SetViolationHandler(
            [aBoard]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                      const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                PCB_MARKER* marker = new PCB_MARKER( aItem, aPos, aLayer );

                aPathGenerator( marker );
                aBoard->Add( marker );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, true );
    bds.m_DRCEngine->ClearViolationHandler();
}


// Courtyard overlaps are positioned by SHAPE_POLY_SET::Collide(), which walks a triangulation
// whose triangle order is not reproducible between loads, so their keys move on their own.  That
// is a separate defect in the geometry layer, not in the exclusion bookkeeping under test here.
static bool isCourtyardOverlap( PCB_MARKER* aMarker )
{
    return aMarker->GetRCItem()->GetErrorCode() == DRCE_OVERLAPPING_FOOTPRINTS;
}


BOOST_FIXTURE_TEST_CASE( DRCExclusionsSurviveProjectRoundTrip, DRC_REGRESSION_TEST_FIXTURE )
{
    // Exclude every violation on the board, write the exclusions to the project file, then load
    // the project back and run DRC again.  Nothing may be reported the second time round.  The
    // board file itself is copied rather than re-saved, so both runs see identical geometry.

    FileCleaner tempFileCleaner;
    wxString    tempBoardPath, tempProjectPath, tempStem;

    KI_TEST::LoadBoard( m_settingsManager, "issue25101", m_board );
    BOOST_REQUIRE( m_board );

    runDrcAndCreateMarkers( m_board.get() );

    std::map<wxString, int> keyCounts;
    std::map<wxString, int> expectedKeys;

    for( PCB_MARKER* marker : m_board->Markers() )
    {
        wxString serialized = marker->SerializeToString();

        keyCounts[serialized]++;

        if( !isCourtyardOverlap( marker ) )
            expectedKeys[serialized]++;
    }

    // A violation found on several copper layers is reported once per layer but serializes to a
    // single key, so one stored exclusion has to cover every marker that shares it
    bool sharedKeys = std::any_of( keyCounts.begin(), keyCounts.end(),
                                   []( const std::pair<const wxString, int>& aEntry )
                                   {
                                       return aEntry.second > 1;
                                   } );

    BOOST_REQUIRE_MESSAGE( sharedKeys, "board no longer produces multi-layer violations" );
    BOOST_TEST_MESSAGE( "Markers: " << m_board->Markers().size() << "  distinct keys: " << keyCounts.size() );

    for( PCB_MARKER* marker : m_board->Markers() )
        marker->SetExcluded( true );

    m_board->RecordDRCExclusions();
    BOOST_CHECK_EQUAL( m_board->GetDesignSettings().m_DrcExclusions.size(), keyCounts.size() );

    saveBoardAndProjectToTempFiles( "issue25101", tempFileCleaner, tempBoardPath, tempProjectPath, tempStem, true );

    KI_TEST::LoadBoard( m_settingsManager, tempStem, m_board );
    BOOST_REQUIRE( m_board );

    BOARD_DESIGN_SETTINGS& reloadedBds = m_board->GetDesignSettings();
    BOOST_REQUIRE_EQUAL( reloadedBds.m_DrcExclusions.size(), keyCounts.size() );

    runDrcAndCreateMarkers( m_board.get() );

    // Violations have to be reported against the same items, at the same place, and just as
    // often, on a board that has just been reloaded, or no exclusion can survive the trip
    std::map<wxString, int> reloadedKeys;

    for( PCB_MARKER* marker : m_board->Markers() )
    {
        if( !isCourtyardOverlap( marker ) )
            reloadedKeys[marker->SerializeToString()]++;
    }

    for( const std::pair<const wxString, int>& entry : expectedKeys )
        BOOST_CHECK_MESSAGE( reloadedKeys[entry.first] == entry.second, "key not reproduced: " << entry.first );

    for( const std::pair<const wxString, int>& entry : reloadedKeys )
        BOOST_CHECK_MESSAGE( expectedKeys.count( entry.first ), "key appeared only after reload: " << entry.first );

    m_board->ResolveDRCExclusions( false );

    for( PCB_MARKER* marker : m_board->Markers() )
    {
        BOOST_CHECK_MESSAGE( marker->IsExcluded() || isCourtyardOverlap( marker ),
                             "exclusion lost for " << marker->SerializeToString() );
    }
}
