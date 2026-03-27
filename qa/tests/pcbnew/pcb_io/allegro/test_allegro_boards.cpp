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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include "allegro_block_tests.h"
#include "allegro_test_utils.h"

#include <filesystem>
#include <fstream>

#include <boost/test/data/test_case.hpp>

#include <json_common.h>

#include <board.h>
#include <reporter.h>
#include <pcbnew/pcb_io/allegro/pcb_io_allegro.h>

#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_expectations.h>

#include <convert/allegro_parser.h>

/**
 * @file test_allegro_boards.cpp
 *
 * This file contains unit tests for parsing individual Allegro blocks and headers, loading from
 * declarative test definitions in JSON files and binary data files.
 *
 * This allows to put regression/validation tests in place for block parsing, without having to
 * load an entire .brd file and parse all of its blocks. Allows to "lock in" known parsable blocks
 * at various file version.
 */

using namespace ALLEGRO;
using namespace KI_TEST;


/**
 * Just enough information about a block-level test to be able to name and register
 * it with the test runner at static init time.
 */
struct ALLEGRO_BLOCK_TEST_DESCRIPTOR
{
    uint8_t m_BlockType;
    size_t  m_BlockOffset;
    /// Handy ref to the JSON entry for this block test
    const nlohmann::json& m_BlockTestJson;
};

/**
 * Just enough information about the board test to be able to name and register any
 * tests for this board.
 */
struct ALLEGRO_BOARD_TEST_DESCRIPTOR
{
    // The board name in the JSON registry
    std::string m_BrdName;

    bool                                       m_HasHeaderTest;
    std::vector<ALLEGRO_BLOCK_TEST_DESCRIPTOR> m_BlockTests;
    bool                                       m_HasBoardFile;

    /// List of expectation tests found in the JSON for this board
    std::vector<BOARD_EXPECTATION_TEST::DESCRIPTOR> m_ExpectationTests;
};


/**
 * The registry of known Allegro board and block tests,
 * populated at static init time by reading the JSON registry file.
 *
 * (We need to do this at static init time to be able to register
 * named Boost test cases).
 */
struct ALLEGRO_BLOCK_TEST_REGISTRY
{
    nlohmann::json                             m_Json;
    std::vector<ALLEGRO_BOARD_TEST_DESCRIPTOR> m_BoardTests;

    const nlohmann::json& GetBoardJson( const std::string& aBoardName ) const
    {
        return m_Json["boards"][aBoardName];
    }
};


static std::vector<uint8_t> loadDataByUri( const std::string& aDataSource )
{
    // For now, we only support loading from files, but in the future we could also support
    // loading from other sources (e.g. from compiled-in data, base64, etc.)

    // Split off the protocol (e.g. "file://") if present, to get the actual path
    std::string       path, protocol;
    const std::string sep = "://";

    size_t sepPos = aDataSource.find( sep );
    if( sepPos != std::string::npos )
    {
        protocol = aDataSource.substr( 0, sepPos );
        path = aDataSource.substr( sepPos + sep.size() );
    }
    else
    {
        // No protocol
        throw std::runtime_error( "Unsupported data source URI (missing protocol): " + aDataSource );
    }

    if( protocol == "file" )
    {
        // This means get it from a file in the QD data
        return KI_TEST::LoadBinaryData( path, std::nullopt );
    }

    throw std::runtime_error( "Unsupported data source protocol: " + protocol );
}


ALLEGRO::FMT_VER getFormatVersionFromStr( const std::string& aFmtVerStr )
{
    // clang-format off
    static const std::map<std::string, ALLEGRO::FMT_VER> fmtVerStrMap{
        { "16.0", FMT_VER::V_160 },
        { "16.2", FMT_VER::V_162 },
        { "16.4", FMT_VER::V_164 },
        { "16.5", FMT_VER::V_165 },
        { "16.6", FMT_VER::V_166 },
        { "17.2", FMT_VER::V_172 },
        { "17.4", FMT_VER::V_174 },
        { "17.5", FMT_VER::V_175 },
        { "18.0", FMT_VER::V_180 },
    };
    // clang-format on

    auto it = fmtVerStrMap.find( aFmtVerStr );
    if( it != fmtVerStrMap.end() )
    {
        return it->second;
    }

    return FMT_VER::V_UNKNOWN;
}


static std::unique_ptr<HEADER_TEST_INFO> createHeaderTestEntry( const std::string&    boardDir,
                                                                const nlohmann::json& headerTestEntry )
{
    std::unique_ptr<HEADER_TEST_INFO> headerTest;

    if( !headerTestEntry.is_object() )
    {
        throw std::runtime_error( "Header test entry is not a valid JSON object" );
    }

    // Default header location
    const std::string headerDataUri = std::string( "file://" ) + boardDir + "header.bin";

    return std::make_unique<HEADER_TEST_INFO>( headerDataUri, headerTestEntry.value( "skip", false ) );
}


static BLOCK_TEST_INFO createBlockTestEntry( const std::string&                   boardDir,
                                             const ALLEGRO_BLOCK_TEST_DESCRIPTOR& aBlockTestDescriptor )
{
    const nlohmann::json& blockTestJson = aBlockTestDescriptor.m_BlockTestJson;
    const bool skipBlock = blockTestJson.value( "skip", false );

    const uint8_t& blockType = aBlockTestDescriptor.m_BlockType;
    const size_t&  blockOffset = aBlockTestDescriptor.m_BlockOffset;

    // Default block data location is a file with a name based on the block type and offset
    wxString blockDataLoc = wxString::Format( "0x%02x_0x%08zx.bin", blockType, blockOffset );

    const std::string blockDataUri = std::string( "file://" ) + boardDir + blockDataLoc.ToStdString();

    const bool extraBlockTest = blockTestJson.value( "extraBlockTest", false );
    const bool extraDbObjTest = blockTestJson.value( "extraDbObjTest", false );

    return BLOCK_TEST_INFO{
            blockType,
            blockOffset,
            skipBlock,
            extraBlockTest,
            extraDbObjTest,
            blockDataUri,
    };
}


struct BOARD_TEST_INFO
{
    std::string      m_BrdName;
    ALLEGRO::FMT_VER m_FormatVersion;
};


static BOARD_TEST_INFO createBoardTestInfo( const std::string& aBrdName, const nlohmann::json& boardTestEntry )
{
    const ALLEGRO::FMT_VER formatVersion =
            getFormatVersionFromStr( boardTestEntry.value( "formatVersion", "unknown" ) );

    return BOARD_TEST_INFO{ aBrdName, formatVersion };
}


class ALLEGRO_BLOCK_TEST_FIXTURE
{
public:
    void RunHeaderTest( const std::string& aBrdName, const nlohmann::json& aBoardTestJson )
    {
        BOOST_TEST_CONTEXT( wxString::Format( "Testing header from board %s", aBrdName ) )
        {
            BOARD_TEST_INFO brdDef = createBoardTestInfo( aBrdName, aBoardTestJson );

            const std::string boardDir = AllegroBoardDataDir( aBrdName );

            std::unique_ptr<HEADER_TEST_INFO> headerTest = nullptr;

            if( aBoardTestJson.contains( "header" ) && aBoardTestJson["header"].is_object() )
            {
                headerTest = createHeaderTestEntry( boardDir, aBoardTestJson["header"] );
            }

            BOOST_REQUIRE( headerTest != nullptr );

            if( headerTest->m_Skip )
            {
                BOOST_TEST_MESSAGE( "Skipping test for this header" );
                return;
            }

            // Read in the header data
            std::vector<uint8_t> headerData = loadDataByUri( headerTest->m_HeaderDataSource );
            FILE_STREAM          fileStream( headerData.data(), headerData.size() );
            HEADER_PARSER        parser( fileStream );

            std::unique_ptr<FILE_HEADER> header = parser.ParseHeader();

            BOOST_REQUIRE( header != nullptr );

            // We don't expect any known header to be in an unknown format
            BOOST_TEST( parser.GetFormatVersion() != FMT_VER::V_UNKNOWN );

            // Make sure the format version matches what the board is expected to be
            if( brdDef.m_FormatVersion != FMT_VER::V_UNKNOWN )
            {
                BOOST_TEST( parser.GetFormatVersion() == brdDef.m_FormatVersion );
            }

            // Other header tests - validate some commonly used fields?

            // The parser should have consumed all the data in the header
            BOOST_TEST( fileStream.Position() == headerData.size() );
        }
    }


    void RunBlockTest( const std::string& aBrdName, const nlohmann::json& aBoardTestJson,
                       const ALLEGRO_BLOCK_TEST_DESCRIPTOR& aBlockTest )
    {
        const BOARD_TEST_INFO brdDef = createBoardTestInfo( aBrdName, aBoardTestJson );
        const BLOCK_TEST_INFO blockTest = createBlockTestEntry( AllegroBoardDataDir( aBrdName ), aBlockTest );

        BOOST_TEST_CONTEXT( wxString::Format( "Testing block type %#02x at offset %#010zx from board %s",
                                              blockTest.m_BlockType, blockTest.m_BlockOffset, aBrdName ) )
        {
            if( blockTest.m_Skip )
            {
                BOOST_TEST_MESSAGE( "Skipping test for this block" );
                return;
            }

            // Read in the block data, and create a FILE_STREAM for it
            std::vector<uint8_t> data = loadDataByUri( blockTest.m_DataSource );
            FILE_STREAM          fileStream( data.data(), data.size() );

            BLOCK_PARSER parser( fileStream, brdDef.m_FormatVersion );

            bool                        endOfObjectsMarker = false;
            std::unique_ptr<BLOCK_BASE> block = parser.ParseBlock( endOfObjectsMarker );

            // Compare the result to the expected results
            BOOST_REQUIRE( block != nullptr );

            // The parser should have consumed all the data in the block
            BOOST_TEST( fileStream.Position() == data.size() );

            // Look up and run additional ad-hoc block-level tests
            if( blockTest.m_ExtraBlockTest )
            {
                KI_TEST::RunAdditionalBlockTest( aBrdName, blockTest.m_BlockOffset, *block );
            }

            // Now try to convert the block into a DB_OBJ
            ALLEGRO::BRD_DB                  brd;
            ALLEGRO::BRD_DB::OBJ_FACTORY     objFactory( brd );
            // Not all blocks convert to DB_OBJ yet, but at least it mustn't crash.
            // Eventually, this should always succeed.
            std::unique_ptr<ALLEGRO::DB_OBJ> dbObj = objFactory.CreateObject( *block );

            if( blockTest.m_ExtraDbObjTest )
            {
                BOOST_REQUIRE( dbObj != nullptr );
                KI_TEST::RunAdditionalObjectTest( aBrdName, blockTest.m_BlockOffset, *dbObj );
            }
        }
    }
};


static void AssertNoErrors( const CAPTURING_REPORTER& aReporter )
{
    if( aReporter.GetErrorCount() > 0 )
    {
        std::ostringstream ss;
        ss << "Expected no errors, but found " << aReporter.GetErrorCount() << " errors:";

        for( const auto& msg : aReporter.GetMessages() )
        {
            if( msg.severity == RPT_SEVERITY_ERROR )
            {
                ss << "\n  " << msg.text;
            }
        }
        BOOST_TEST_MESSAGE( ss.str() );
    }
    BOOST_TEST( aReporter.GetErrorCount() == 0 );
}


void RunBoardLoad( const std::string& aBrdName, const nlohmann::json& aBoardTestJson )
{
    BOARD* board = nullptr;

    BOOST_TEST_CONTEXT( "Testing load from board: " << aBrdName )
    {
        if( aBoardTestJson.contains( "boardFile" ) )
        {
            const std::string boardFilePath =
                    KI_TEST::AllegroBoardDataDir( aBrdName ) + aBoardTestJson["boardFile"].get<std::string>();

            if( !std::filesystem::exists( boardFilePath ) )
            {
                BOOST_FAIL( "Board file does not exist: " + boardFilePath );
                return;
            }

            CAPTURING_REPORTER reporter;
            board = KI_TEST::ALLEGRO_CACHED_LOADER::GetInstance().LoadAndCache( boardFilePath, &reporter );

            const bool expectLoadFailure = aBoardTestJson.value( "expectLoadFailure", false );

            if( expectLoadFailure )
            {
                BOOST_CHECK( board == nullptr );
                BOOST_TEST_MESSAGE( "Board load was expected to fail, and it did." );
                return;
            }
            else
            {
                BOOST_CHECK_MESSAGE( board != nullptr, "Board load was expected to succeed, but it failed." );

                // Can allow a certain number of warnings, but no errors
                AssertNoErrors( reporter );

                // Can check max warnings here if we want
                if( reporter.GetWarningCount() > 0 )
                {
                    std::ostringstream ss;
                    ss << aBrdName << ": " << reporter.GetWarningCount() << " warnings";

                    for( const auto& msg : reporter.GetMessages() )
                    {
                        if( msg.severity == RPT_SEVERITY_WARNING )
                        {
                            ss << "\n  " << msg.text;
                        }
                    }

                    BOOST_TEST_MESSAGE( ss.str() );
                }

                KI_TEST::PrintBoardStats( board, aBrdName );
            }
        }
    }
}


static uint8_t getByteFromHexStr( const nlohmann::json& aJsonEntry, const std::string& aFieldName )
{
    if( !aJsonEntry.contains( aFieldName ) )
    {
        throw std::runtime_error( "Block test entry is missing the '" + aFieldName + "' field" );
    }

    if( aJsonEntry[aFieldName].is_number_unsigned() )
    {
        const unsigned long value = aJsonEntry[aFieldName].get<unsigned long>();
        if( value > 0xFF )
        {
            throw std::runtime_error( "Value is too large to fit in a byte: " + std::to_string( value ) );
        }
        return static_cast<uint8_t>( value );
    }
    else if( aJsonEntry[aFieldName].is_string() )
    {
        const std::string aHexStr = aJsonEntry[aFieldName];

        const unsigned long value = std::stoul( aHexStr, nullptr, 0 );
        if( value > 0xFF )
        {
            throw std::runtime_error( "Value is too large to fit in a byte: " + aHexStr );
        }
        return static_cast<uint8_t>( value );
    }

    throw std::runtime_error( "Block test entry has an invalid '" + aFieldName
                              + "' field (must be a byte value as a number or hex string)" );
}


static ALLEGRO_BLOCK_TEST_DESCRIPTOR createBlockTestDescriptor( const nlohmann::json& aBlockTestJson )
{
    const uint8_t blockType = getByteFromHexStr( aBlockTestJson, "type" );

    if( !aBlockTestJson.contains( "offset" ) || !aBlockTestJson["offset"].is_string() )
    {
        throw std::runtime_error( "Block test entry is missing a valid 'offset' field" );
    }

    const std::string block_offset = aBlockTestJson["offset"];
    const size_t      blockOffset = std::stoul( block_offset, nullptr, 0 );

    return ALLEGRO_BLOCK_TEST_DESCRIPTOR{
        blockType,
        blockOffset,
        aBlockTestJson,
    };
}


/**
 * Construct a list of test descriptrs (lightweight objects) for the boards we have test data for,
 * by reading the registry JSON file.
 *
 * These definitions will then be bound to the test cases. This all happens at static init
 * time, but the main work of data loading and test setup and running will run at runtime.
 */
static std::vector<ALLEGRO_BOARD_TEST_DESCRIPTOR> getBoardTestDefinitions( const nlohmann::json& aJson )
{
    if( !aJson.contains( "boards" ) || !aJson["boards"].is_object() )
    {
        throw std::runtime_error( "Test register JSON file does not contain a valid 'boards' object." );
    }

    std::vector<ALLEGRO_BOARD_TEST_DESCRIPTOR> boardTests;

    for( auto& [boardName, boardEntry] : aJson["boards"].items() )
    {
        ALLEGRO_BOARD_TEST_DESCRIPTOR boardTestRef{
            .m_BrdName = boardName,
            .m_HasHeaderTest = boardEntry.contains( "header" ),
            .m_BlockTests = {},
            .m_HasBoardFile = boardEntry.contains( "boardFile" ),
            .m_ExpectationTests = {},
        };

        if( boardEntry.contains( "blocks" ) && boardEntry["blocks"].is_array() )
        {
            for( const auto& blockTestEntry : boardEntry["blocks"] )
            {
                boardTestRef.m_BlockTests.emplace_back( createBlockTestDescriptor( blockTestEntry ) );
            }
        }

        if( boardEntry.contains( "boardExpectations" ) )
        {
            const auto& expectationsEntry = boardEntry["boardExpectations"];

            boardTestRef.m_ExpectationTests =
                    BOARD_EXPECTATION_TEST::ExtractExpectationTestsFromJson( expectationsEntry );
        }

        boardTests.push_back( std::move( boardTestRef ) );
    }

    return boardTests;
}


static const ALLEGRO_BLOCK_TEST_REGISTRY& buildTestRegistry()
{
    static ALLEGRO_BLOCK_TEST_REGISTRY registry = []()
    {
        ALLEGRO_BLOCK_TEST_REGISTRY reg;

        // There is currently one registry file, but we could have more, e.g. if we
        // stored the test definitions on a per-board basis in the board data dirs
        const std::filesystem::path testRegisterJsonPath( AllegroBoardDataDir( "" ) + "board_data_registry.json" );
        std::ifstream               jsonFileStream( testRegisterJsonPath );

        reg.m_Json = nlohmann::json::parse( jsonFileStream, nullptr, true, true );
        reg.m_BoardTests = getBoardTestDefinitions( reg.m_Json );

        return reg;
    }();

    return registry;
}


/**
 * Get the labels associated with a board test, which can be used to e.g. filter tests.
 */
static std::vector<std::string> getBoardTestLabels( const nlohmann::json& boardTestJson )
{
    std::vector<std::string> labels;

    if( boardTestJson.contains( "testLabels" ) && boardTestJson["testLabels"].is_array() )
    {
        for( const auto& label : boardTestJson["testLabels"] )
        {
            if( label.is_string() )
            {
                labels.push_back( label.get<std::string>() );
            }
        }
    }

    return labels;
}


/**
 * This function initializes the test suites for Allegro block and board parsing
 *
 * It reads about the minimum information it needs to to construct the test cases
 * (i.e. it needs to know the name and which tests are present).
 *
 * Each test case will call the appropriate test function (e.g. RunHeaderTest or RunBlockTest)
 * at runtime, which will construct more complete test implementations and then run them.
 */
static std::vector<boost::unit_test::test_suite*> buildAllegroBoardSuites()
{
    using BTS = boost::unit_test::test_suite;
    std::vector<BTS*>                  suites;
    BTS*                               blockSuite = suites.emplace_back( BOOST_TEST_SUITE( "AllegroBlocks" ) );
    const ALLEGRO_BLOCK_TEST_REGISTRY* registry = nullptr;

    try
    {
        registry = &buildTestRegistry();
    }
    catch( const std::exception& ex )
    {
        std::string msg = "Failed to load Allegro block test definitions: " + std::string( ex.what() );

        // Register one failing test to report the error, so that we don't just silently skip all the tests
        const auto failingTestFunction = [=]()
        {
            BOOST_TEST( false, msg );
        };

        blockSuite->add(
            boost::unit_test::make_test_case(
                failingTestFunction,
                "FailureToLoadTestDefinitions",
                __FILE__,
                __LINE__
            )
        );

        return suites;
    }

    for( const ALLEGRO_BOARD_TEST_DESCRIPTOR& boardTest : registry->m_BoardTests )
    {
        BTS* boardSuite = BOOST_TEST_SUITE( boardTest.m_BrdName );
        blockSuite->add( boardSuite );

        const nlohmann::json& boardTestJson = registry->GetBoardJson( boardTest.m_BrdName );

        if( boardTest.m_HasHeaderTest )
        {
            // Note: captures ref-to-static
            const auto testRunFunction = [&]()
            {
                ALLEGRO_BLOCK_TEST_FIXTURE fixture;
                fixture.RunHeaderTest( boardTest.m_BrdName, boardTestJson );
            };

            boardSuite->add(
                boost::unit_test::make_test_case(
                    testRunFunction,
                    "Header",
                    __FILE__,
                    __LINE__
                )
            );
        }

        for( const auto& blockTest : boardTest.m_BlockTests )
        {
            // All these captures come from data stored in a static variable, so  we can take them
            // by reference here.
            const auto testRunFunction = [&boardTest, &boardTestJson, &blockTest]()
            {
                ALLEGRO_BLOCK_TEST_FIXTURE fixture;
                fixture.RunBlockTest( boardTest.m_BrdName, boardTestJson, blockTest );
            };

            wxString testName =
                    wxString::Format( "Block_0x%02x_offset_0x%010zx", blockTest.m_BlockType, blockTest.m_BlockOffset );

            boardSuite->add(
                boost::unit_test::make_test_case(
                    testRunFunction,
                    testName.ToStdString(),
                    __FILE__,
                    __LINE__
                )
            );
        }
    }

    // Separate test suite for board expectations, which will run after the header and block parsing tests,
    // and will have access to the fully parsed board (and obviously be much slower)
    BTS* boardSuites = suites.emplace_back( BOOST_TEST_SUITE( "AllegroBoards" ) );

    for( const ALLEGRO_BOARD_TEST_DESCRIPTOR& boardTest : registry->m_BoardTests )
    {
        if( !boardTest.m_HasBoardFile )
        {
            continue;
        }

        // Board-level suite
        BTS* boardSuite = BOOST_TEST_SUITE( boardTest.m_BrdName );
        boardSuites->add( boardSuite );

        const nlohmann::json& boardTestData = registry->GetBoardJson( boardTest.m_BrdName );

        const std::vector<std::string> testLabels = getBoardTestLabels( boardTestData );
        boost::unit_test::test_case*   loadingTestCase = nullptr;

        if( boardTest.m_HasBoardFile )
        {
            const auto testLoadFunction = [&boardTest, &boardTestData]()
            {
                RunBoardLoad( boardTest.m_BrdName, boardTestData );
            };

            // The first test unit loads (and caches) the board.
            loadingTestCase = boost::unit_test::make_test_case(
                    testLoadFunction,
                    "Import",
                    __FILE__,
                    __LINE__
            );

            for( const std::string& label : testLabels )
            {
                loadingTestCase->add_label( label );
            }

            boardSuite->add( loadingTestCase );
        }

        if( boardTest.m_ExpectationTests.size() > 0 )
        {
            BTS* expectationsSuite = BOOST_TEST_SUITE( "Expectations" );
            boardSuite->add( expectationsSuite );

            for( const BOARD_EXPECTATION_TEST::DESCRIPTOR& expectationTestRef : boardTest.m_ExpectationTests )
            {
                const auto testRunFunction = [&boardTest, &boardTestData, &expectationTestRef]()
                {
                    if( !boardTestData.contains( "boardFile" ) )
                    {
                        BOOST_FAIL(
                                "Board test JSON does not contain a 'boardFile' entry - cannot run expectations test" );
                        return;
                    }

                    const std::string boardFilePath = KI_TEST::AllegroBoardDataDir( boardTest.m_BrdName )
                                                      + boardTestData["boardFile"].get<std::string>();

                    const BOARD* board = KI_TEST::ALLEGRO_CACHED_LOADER::GetInstance().GetCachedBoard( boardFilePath );

                    if( !board )
                    {
                        BOOST_FAIL( "Failed to load cached board for expectations test: " + boardFilePath );
                        return;
                    }

                    KI_TEST::BOARD_EXPECTATION_TEST::RunFromRef( boardTest.m_BrdName, *board, expectationTestRef );
                };

                boost::unit_test::test_case* expectationTestCase = boost::unit_test::make_test_case(
                        testRunFunction,
                        expectationTestRef.m_TestName,
                        __FILE__,
                        __LINE__
                );

                // Label the expectations test case with any tags from the JSON
                for( const std::string& label : expectationTestRef.m_Tags )
                {
                    expectationTestCase->add_label( label );
                }

                expectationsSuite->add( expectationTestCase );
            }

            // We can only run the expectations test after the board has been loaded
            // and that test passes.
            BOOST_REQUIRE( loadingTestCase != nullptr );
            expectationsSuite->depends_on( loadingTestCase );
        }
    }

    return suites;
}


/**
 * At static initialization, register the test suite.
 * This is needed to be able to run the tests by name.
 */
static struct RegisterBlockSuites
{
    RegisterBlockSuites()
    {
        std::vector<boost::unit_test::test_suite*> suites = buildAllegroBoardSuites();

        for( boost::unit_test::test_suite* suite : suites )
        {
            boost::unit_test::framework::master_test_suite().add( suite );
        }
    }
} s_registerHeaderBlockSuites;
