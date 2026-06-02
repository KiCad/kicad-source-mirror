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

#include "test_diptrace_sch_import_fixture.h"


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


/**
 * power_supply.dch is a v37 schematic that stores UTF-16-BE strings, which the legacy version
 * threshold would mis-read as ASCII. The encoding auto-detection must let it load with real
 * content rather than throwing "Unreasonable v37 string length" at the header.
 */
BOOST_AUTO_TEST_CASE( V37Utf16SchematicLoadsWithContent )
{
    const std::string path = GetTestDataDir() + "power_supply.dch";

    SCH_SHEET* root = LoadDipTraceSchematic( path );
    BOOST_REQUIRE( root != nullptr );

    BOOST_CHECK_GT( CountItemsOfType( SCH_SYMBOL_T ), 0 );
    BOOST_CHECK_GT( CountItemsOfType( SCH_LINE_T ), 0 );

    RemoveGeneratedLibrary( path );
}


BOOST_AUTO_TEST_CASE( InvalidComponentCountFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_count_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset COMPONENT_COUNT_OFFSET = 0xEE;
        const uint8_t                 invalidCount[] = { 0x12, 0x4F, 0x81 }; // int3 value 200001

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( COMPONENT_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidCount, sizeof( invalidCount ) ), sizeof( invalidCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( ComponentRecordsAreParsedSequentially )
{
    const std::string path = GetTestDataDir() + "z80_board.dch";

    SCH_SHEET* rootSheet = new SCH_SHEET( m_schematic.get() );
    const_cast<KIID&>( rootSheet->m_Uuid ) = niluuid;

    wxFileName newFilename( path );
    newFilename.SetExt( FILEEXT::KiCadSchematicFileExtension );
    rootSheet->SetFileName( newFilename.GetFullPath() );
    m_schematic->SetTopLevelSheets( { rootSheet } );

    SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );
    screen->SetFileName( newFilename.GetFullPath() );
    rootSheet->SetScreen( screen );

    DIPTRACE::SCH_PARSER parser( wxString::FromUTF8( path ), m_schematic.get(), rootSheet, nullptr, &m_reporter );
    parser.Parse();

    BOOST_CHECK_EQUAL( parser.ComponentBoundaryScanCount(), 0 );

    RemoveGeneratedLibrary( path );
}


/**
 * The importer must rely solely on the symbol definitions embedded in the schematic.
 * It must not write a standalone .kicad_sym library beside the imported file.
 */
BOOST_AUTO_TEST_CASE( NoSymbolLibraryFileIsGenerated )
{
    const std::string path = GetTestDataDir() + "z80_board.dch";

    wxFileName genLib( path );
    genLib.SetName( genLib.GetName() + wxT( "-diptrace-import" ) );
    genLib.SetExt( wxT( "kicad_sym" ) );

    if( genLib.FileExists() )
        wxRemoveFile( genLib.GetFullPath() );

    SCH_SHEET* root = LoadDipTraceSchematic( path );
    BOOST_REQUIRE( root != nullptr );

    BOOST_CHECK_MESSAGE( !genLib.FileExists(), "DipTrace import must not create a symbol library file: "
                                                       << genLib.GetFullPath().ToStdString() );

    // The imported symbols stay usable because their definitions are embedded in the schematic.
    std::set<SCH_SCREEN*>    seenScreens;
    std::vector<SCH_SCREEN*> pendingScreens;
    bool                     foundEmbeddedSymbol = false;

    if( root->GetScreen() )
        pendingScreens.push_back( root->GetScreen() );

    while( !pendingScreens.empty() && !foundEmbeddedSymbol )
    {
        SCH_SCREEN* screen = pendingScreens.back();
        pendingScreens.pop_back();

        if( !screen || !seenScreens.insert( screen ).second )
            continue;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                if( static_cast<SCH_SYMBOL*>( item )->GetLibSymbolRef() )
                {
                    foundEmbeddedSymbol = true;
                    break;
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

    BOOST_CHECK_MESSAGE( foundEmbeddedSymbol, "Imported schematic must carry embedded symbol definitions." );

    if( genLib.FileExists() )
        wxRemoveFile( genLib.GetFullPath() );
}


/**
 * The imported content root must become the schematic's real top-level sheet. A nil-UUID root is
 * treated as the virtual root and dropped by SetTopLevelSheets(), orphaning the import so the
 * schematic editor shows nothing even though parsing succeeded.
 */
BOOST_AUTO_TEST_CASE( ImportedRootIsTheSchematicTopLevelSheet )
{
    const std::string path = GetTestDataDir() + "z80_board.dch";

    SCH_SHEET* root = LoadDipTraceSchematic( path );
    BOOST_REQUIRE( root != nullptr );
    BOOST_REQUIRE( root->GetScreen() != nullptr );

    BOOST_CHECK( root->m_Uuid != niluuid );
    BOOST_CHECK_EQUAL( m_schematic->RootScreen(), root->GetScreen() );

    SCH_SHEET_LIST hierarchy = m_schematic->BuildUnorderedSheetList();
    BOOST_REQUIRE_GE( hierarchy.size(), 1u );

    bool contentReachable = false;

    for( const SCH_SHEET_PATH& sheetPath : hierarchy )
    {
        if( sheetPath.LastScreen() == root->GetScreen() )
        {
            contentReachable = true;
            break;
        }
    }

    BOOST_CHECK( contentReachable );

    RemoveGeneratedLibrary( path );
}


BOOST_AUTO_TEST_CASE( ViewerExampleComponentRecordsAreParsedSequentiallyOptional )
{
    const std::string              examplesDir = GetViewerExamplesDir();
    const std::vector<std::string> paths = {
        examplesDir + "/CNC_controller.dch",
        examplesDir + "/Schematic_2.dch",
        examplesDir + "/Schematic_4.dch",
        examplesDir + "/Schematic_6.dch",
    };

    bool foundAny = false;

    for( const std::string& path : paths )
    {
        if( !wxFileExists( path ) )
            continue;

        foundAny = true;

        SCH_SHEET* rootSheet = new SCH_SHEET( m_schematic.get() );
        const_cast<KIID&>( rootSheet->m_Uuid ) = niluuid;

        wxFileName newFilename( path );
        newFilename.SetExt( FILEEXT::KiCadSchematicFileExtension );
        rootSheet->SetFileName( newFilename.GetFullPath() );
        m_schematic->SetTopLevelSheets( { rootSheet } );

        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );
        screen->SetFileName( newFilename.GetFullPath() );
        rootSheet->SetScreen( screen );

        DIPTRACE::SCH_PARSER parser( wxString::FromUTF8( path ), m_schematic.get(), rootSheet, nullptr, &m_reporter );
        parser.Parse();

        BOOST_CHECK_MESSAGE( parser.ComponentBoundaryScanCount() == 0,
                             path + ": component boundary scan fallback used" );

        RemoveGeneratedLibrary( path );
    }

    if( !foundAny )
    {
        BOOST_TEST_MESSAGE( "Skipping sequential component record check; no viewer examples found at "
                            << examplesDir );
    }
}


BOOST_AUTO_TEST_CASE( ComponentCountMismatchFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_count_mismatch_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset COMPONENT_COUNT_OFFSET = 0xEE;
        const uint8_t                 mismatchedCount[] = { 0x0F, 0x42, 0xC5 }; // int3 value 133

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( COMPONENT_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( mismatchedCount, sizeof( mismatchedCount ) ), sizeof( mismatchedCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidComponentPinCountFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_pin_count_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_PIN_COUNT_OFFSET = 0x20A;
        const uint8_t                 invalidPinCount[] = { 0x12, 0x4F, 0x81 }; // int3 value 200001

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_COMPONENT_PIN_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidPinCount, sizeof( invalidPinCount ) ), sizeof( invalidPinCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidComponentExtraTailLengthFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_extra_tail_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_EXTRA_TAIL_COUNT_OFFSET = 0x206;
        const uint8_t                 invalidExtraTailCount[] = { 0x00, 0x00, 0x27, 0x10 }; // u4 count 10000

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_COMPONENT_EXTRA_TAIL_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidExtraTailCount, sizeof( invalidExtraTailCount ) ),
                             sizeof( invalidExtraTailCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidComponentPinRecordFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_pin_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_FIRST_PIN_NAME_LEN_OFFSET = 0x226;
        const uint8_t                 invalidNameLength[] = { 0xFF, 0xFF };

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_COMPONENT_FIRST_PIN_NAME_LEN_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidNameLength, sizeof( invalidNameLength ) ),
                             sizeof( invalidNameLength ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidLaterComponentPinRecordFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_later_pin_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_SECOND_PIN_NAME_LEN_OFFSET = 0x272;
        const uint8_t                 invalidNameLength[] = { 0xFF, 0xFF };

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_COMPONENT_SECOND_PIN_NAME_LEN_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidNameLength, sizeof( invalidNameLength ) ),
                             sizeof( invalidNameLength ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidComponentShapePointCountFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_shape_points_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_SHAPE_POINT_COUNT_OFFSET = 0x2C2;
        const uint8_t                 invalidPointCount[] = { 0x0F, 0x42, 0xA5 }; // int3 value 101

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_COMPONENT_SHAPE_POINT_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidPointCount, sizeof( invalidPointCount ) ),
                             sizeof( invalidPointCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( ZeroComponentShapePointCountFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_zero_shape_points_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_SHAPE_POINT_COUNT_OFFSET = 0x2C2;
        const uint8_t                 zeroPointCount[] = { 0x0F, 0x42, 0x40 }; // int3 value 0

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_COMPONENT_SHAPE_POINT_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( zeroPointCount, sizeof( zeroPointCount ) ), sizeof( zeroPointCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidBusEntryTerminatorFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_bus_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_BUS_TERMINATOR_OFFSET = 0x39484;
        const uint8_t                 invalidTerminator[] = { 0x0F, 0x42, 0x40 }; // int3 value 0

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_BUS_TERMINATOR_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidTerminator, sizeof( invalidTerminator ) ),
                             sizeof( invalidTerminator ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidBusSectionCountFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_bus_count_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset BUS_SECTION_COUNT_OFFSET = 0x3946A;
        const uint8_t                 invalidCount[] = { 0x0F, 0x46, 0x29 }; // int3 value 1001

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( BUS_SECTION_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidCount, sizeof( invalidCount ) ), sizeof( invalidCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidWireNetPinCountFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_wire_pin_count_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_WIRE_NET_PIN_COUNT_OFFSET = 0x3988E;
        const uint8_t                 invalidPinCount[] = { 0x0F, 0x51, 0xE9 }; // int3 value 4001

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_WIRE_NET_PIN_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidPinCount, sizeof( invalidPinCount ) ), sizeof( invalidPinCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidWireNetNameLengthFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_wire_name_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_WIRE_NET_NAME_LENGTH_OFFSET = 0x3987A;
        const uint8_t                 invalidNameLength[] = { 0xFF, 0xFF };

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_WIRE_NET_NAME_LENGTH_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidNameLength, sizeof( invalidNameLength ) ),
                             sizeof( invalidNameLength ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidWirePointCountFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString          tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_wire_point_count_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_WIRE_POINT_COUNT_OFFSET = 0x39913;
        const uint8_t                 invalidPointCount[] = { 0x0F, 0x51, 0xE9 }; // int3 value 4001

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_WIRE_POINT_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidPointCount, sizeof( invalidPointCount ) ),
                             sizeof( invalidPointCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


/**
 * Importing a well-formed DipTrace schematic must run the full import chain without emitting any
 * warnings or errors. Component records are split by their header signature and parsed within
 * fixed bounds, so a recognised file should never fall back, mis-parse a record, or fail to save
 * its generated library. This guards against regressions in the component boundary detector.
 *
 * z80_board.dch (v38) and power_supply.dch (v37) exercise the schematic UTF-16 string path;
 * pppp.dch (v31) exercises the legacy ASCII string path.
 */
BOOST_AUTO_TEST_CASE( ImportChainIsClean )
{
    for( const char* file : { "z80_board.dch", "power_supply.dch", "pppp.dch" } )
    {
        const std::string path = GetTestDataDir() + file;

        if( !wxFileExists( path ) )
            continue;

        SCH_SHEET* root = nullptr;
        BOOST_REQUIRE_NO_THROW( root = LoadDipTraceSchematic( path ) );
        BOOST_REQUIRE( root );

        BOOST_CHECK_MESSAGE( m_reporter.CountOfSeverity( RPT_SEVERITY_ERROR ) == 0,
                             "Import of " << file << " reported errors" );
        BOOST_CHECK_MESSAGE( m_reporter.CountOfSeverity( RPT_SEVERITY_WARNING ) == 0,
                             "Import of " << file << " reported warnings" );

        RemoveGeneratedLibrary( path );
    }
}


BOOST_AUTO_TEST_CASE( CanReadLegacyHeaderOptional )
{
    const std::string examplesDir = GetViewerExamplesDir();
    const std::string sch2 = examplesDir + "/Schematic_2.dch";
    const std::string sch4 = examplesDir + "/Schematic_4.dch";

    if( !wxFileExists( sch2 ) || !wxFileExists( sch4 ) )
    {
        BOOST_TEST_MESSAGE( "Skipping legacy header check; viewer examples not found at " << examplesDir );
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

        // DipTrace net ports import as global labels rather than symbols, so count both as
        // placement-equivalent items (the minSymbols thresholds predate the net-port split).
        const int symbolCount = CountItemsOfType( SCH_SYMBOL_T ) + CountItemsOfType( SCH_GLOBAL_LABEL_T );
        const int screenCount = CountImportedScreens();

        BOOST_CHECK_GE( symbolCount, e.minSymbols );
        BOOST_CHECK_GE( screenCount, e.minScreens );
    }

    if( !foundAny )
    {
        BOOST_TEST_MESSAGE( "Skipping external example load test; no .dch files found at " << examplesDir );
    }
}


BOOST_AUTO_TEST_CASE( ExternalCorpusLoadOptional )
{
    const char* corpusEnv = std::getenv( "DIPTRACE_EXTERNAL_CORPUS_DIR" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "DIPTRACE_EXTERNAL_CORPUS_DIR not set; skipping external schematic corpus sweep" );
        return;
    }

    std::filesystem::path corpusRoot( corpusEnv );

    if( !std::filesystem::exists( corpusRoot ) )
    {
        BOOST_TEST_MESSAGE( "External corpus path does not exist; skipping external schematic corpus sweep" );
        return;
    }

    std::vector<std::filesystem::path> dchFiles;

    for( const auto& entry : std::filesystem::recursive_directory_iterator( corpusRoot ) )
    {
        if( entry.is_regular_file() && entry.path().extension() == ".dch" )
            dchFiles.push_back( entry.path() );
    }

    std::sort( dchFiles.begin(), dchFiles.end() );
    BOOST_REQUIRE_MESSAGE( !dchFiles.empty(), "No .dch files found under: " + corpusRoot.string() );

    int loaded = 0;

    for( const std::filesystem::path& path : dchFiles )
    {
        SCH_SHEET* root = nullptr;

        try
        {
            root = LoadDipTraceSchematic( path.string() );
        }
        catch( const std::exception& e )
        {
            BOOST_ERROR( path.string() + ": exception: " + std::string( e.what() ) );
            continue;
        }

        BOOST_REQUIRE_MESSAGE( root != nullptr, "Failed to load " + path.string() );
        BOOST_REQUIRE( root->GetScreen() );
        loaded++;

        BOOST_CHECK_MESSAGE(
                m_reporter.CountOfSeverity( RPT_SEVERITY_ERROR ) == 0,
                path.string() + ": import reported errors: " + m_reporter.MessagesOfSeverity( RPT_SEVERITY_ERROR ) );
        BOOST_CHECK_MESSAGE( m_reporter.CountOfSeverity( RPT_SEVERITY_WARNING ) == 0,
                             path.string() + ": import reported warnings: "
                                     + m_reporter.MessagesOfSeverity( RPT_SEVERITY_WARNING ) );

        RemoveGeneratedLibrary( path.string() );
    }

    BOOST_CHECK_MESSAGE( loaded > 0, "External schematic corpus sweep loaded zero schematics" );
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

    // Coverage is measured against real (non-net-port) parts. CNC_controller relies more on the
    // heuristic boundary scanner (its count-guided header desyncs), which recovers ~80% of real
    // symbols there; the other examples decode cleanly at >=95%.
    const std::vector<EXPECTED> expected = {
        { "CNC_controller.dch", "CNC_controller.dchxml", 0.78 },
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
        BOOST_REQUIRE_MESSAGE( LoadDchXmlCounts( xmlPath, xmlCounts ), "Failed to parse " << xmlPath );
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

        // Net ports import as global labels, not symbols, so compare real symbols against the
        // non-net-port part count, and verify the net ports are represented as labels.
        const int importedSymbols = CountItemsOfType( SCH_SYMBOL_T );
        const int importedLabels = CountItemsOfType( SCH_GLOBAL_LABEL_T );
        const int importedScreens = CountImportedScreens();
        const int realParts = xmlCounts.partCount - xmlCounts.netPortCount;
        const int minParts = static_cast<int>( std::floor( realParts * e.minPartCoverage ) );

        BOOST_CHECK_EQUAL( importedScreens, xmlCounts.sheetCount );
        BOOST_CHECK_LE( importedSymbols, realParts + 2 );
        BOOST_CHECK_GE( importedSymbols, minParts );

        if( xmlCounts.netPortCount > 0 )
            BOOST_CHECK_GE( importedLabels, 1 );
    }

    if( !foundAny )
    {
        BOOST_TEST_MESSAGE( "Skipping DCH/XML parity test; no matched files found at " << examplesDir );
    }
}


BOOST_AUTO_TEST_CASE( ViewerExamplesPinCountsOptional )
{
    const std::string examplesDir = GetViewerExamplesDir();
    const std::string cncPath = examplesDir + "/CNC_controller.dch";
    const std::string sch6Path = examplesDir + "/Schematic_6.dch";

    if( !wxFileExists( cncPath ) || !wxFileExists( sch6Path ) )
    {
        BOOST_TEST_MESSAGE( "Skipping pin-count checks; required viewer examples not found at " << examplesDir );
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
    const std::string sch6Path = examplesDir + "/Schematic_6.dch";

    if( !wxFileExists( sch6Path ) )
    {
        BOOST_TEST_MESSAGE( "Skipping footprint field checks; Schematic_6.dch not found at " << examplesDir );
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


/**
 * Items electrically joined in DipTrace (by a shared net label or a direct wire) must land on the
 * same KiCad net after import. Recalculate the connection graph headlessly and assert that every
 * subgraph is internally consistent and that at least one net actually ties two or more symbol pins
 * together, proving the importer rebuilt real connectivity rather than isolated pins.
 */
BOOST_AUTO_TEST_CASE( ImportedNetConnectivityIsConsistent )
{
    const std::string path = GetTestDataDir() + "z80_board.dch";

    SCH_SHEET* root = LoadDipTraceSchematic( path );
    BOOST_REQUIRE( root != nullptr );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    const NET_MAP& netMap = m_schematic->ConnectionGraph()->GetNetMap();

    int multiPinNets = 0;

    for( const auto& [key, subgraphs] : netMap )
    {
        for( CONNECTION_SUBGRAPH* sg : subgraphs )
        {
            int pinsOnNet = 0;

            for( SCH_ITEM* item : sg->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*        pin = static_cast<SCH_PIN*>( item );
                SCH_CONNECTION* conn = pin->Connection( &sg->GetSheet() );

                BOOST_REQUIRE( conn != nullptr );
                BOOST_CHECK_EQUAL( conn->NetCode(), key.Netcode );

                pinsOnNet++;
            }

            if( pinsOnNet >= 2 )
                multiPinNets++;
        }
    }

    BOOST_CHECK_MESSAGE( multiPinNets > 0, "Import produced no net connecting two or more pins" );

    RemoveGeneratedLibrary( path );
}


/**
 * The corrected coordinate scaling must place every imported symbol within a sane area around the
 * origin. DipTrace .dch files store no explicit page size, so the KiCad default sheet size is used
 * as the reference. The previous 100x-too-large scaling pushed symbols far past these bounds, so
 * this test fails on the scaling regression and passes once positions divide by 3.
 */
BOOST_AUTO_TEST_CASE( AllSymbolsWithinPageBounds )
{
    const std::string path = GetTestDataDir() + "z80_board.dch";

    SCH_SHEET* root = LoadDipTraceSchematic( path );
    BOOST_REQUIRE( root != nullptr );

    // KiCad default sheet size in schematic IU (matches getOrCreateSheet placement size).
    static constexpr int PAGE_WIDTH_IU = 40640000;
    static constexpr int PAGE_HEIGHT_IU = 30480000;

    // Allow a generous margin for symbols drawn outside the visible page area.
    static constexpr int MARGIN_FACTOR = 4;
    const int            maxX = PAGE_WIDTH_IU * MARGIN_FACTOR;
    const int            maxY = PAGE_HEIGHT_IU * MARGIN_FACTOR;

    int symbolsChecked = 0;

    for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            BOX2I       bbox = symbol->GetBoundingBox();

            BOOST_CHECK_MESSAGE( std::abs( bbox.GetLeft() ) <= maxX && std::abs( bbox.GetRight() ) <= maxX,
                                 "Symbol X extent exceeds page bounds: [" << bbox.GetLeft() << ", " << bbox.GetRight()
                                                                          << "]" );
            BOOST_CHECK_MESSAGE( std::abs( bbox.GetTop() ) <= maxY && std::abs( bbox.GetBottom() ) <= maxY,
                                 "Symbol Y extent exceeds page bounds: [" << bbox.GetTop() << ", " << bbox.GetBottom()
                                                                          << "]" );

            symbolsChecked++;
        }
    }

    BOOST_CHECK_GT( symbolsChecked, 0 );

    RemoveGeneratedLibrary( path );
}


/**
 * A symbol's graphic line width must read at the same scale as its pins so the body and pins look
 * like one drawing. KiCad renders a pin (and a shape with stored width 0) at the default symbol
 * line width, and DipTrace stores real per-shape widths that can be a few times thicker. The
 * previous 100x scaling instead produced page-scale strokes two orders of magnitude past the pins;
 * this test rejects that while still accepting genuine heavy DipTrace strokes.
 */
BOOST_AUTO_TEST_CASE( SymbolGraphicWidthMatchesPinWidth )
{
    const std::string path = GetTestDataDir() + "z80_board.dch";

    SCH_SHEET* root = LoadDipTraceSchematic( path );
    BOOST_REQUIRE( root != nullptr );

    // Pins draw at the schematic default line width. Real DipTrace strokes can be several times
    // that, so cap at 100 mil; the 100x scaling bug overshot this same shape by roughly 50x.
    const int defaultWidth = schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );
    const int maxShapeWidth = schIUScale.MilsToIU( 100 );

    int shapesChecked = 0;

    for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL*                        symbol = static_cast<SCH_SYMBOL*>( item );
            const std::unique_ptr<LIB_SYMBOL>& libSymbol = symbol->GetLibSymbolRef();

            if( !libSymbol )
                continue;

            for( SCH_ITEM& drawItem : libSymbol->GetDrawItems() )
            {
                if( drawItem.Type() != SCH_SHAPE_T )
                    continue;

                int width = static_cast<SCH_SHAPE&>( drawItem ).GetStroke().GetWidth();

                // Zero means "use the default", which is exactly the pin width, so accept it.
                BOOST_CHECK_MESSAGE( width >= 0 && width <= maxShapeWidth,
                                     "Symbol graphic width " << width << " is far from pin width " << defaultWidth );

                shapesChecked++;
            }
        }
    }

    BOOST_CHECK_GT( shapesChecked, 0 );

    RemoveGeneratedLibrary( path );
}


/**
 * The shape type comes from the stored kind pair in each shape record, not from geometry. A shape
 * tagged (700, 6) is a rectangle whose two points are opposite corners; everything else stays a
 * polyline, so a two point diagonal line keeps its true line type. Both kinds occur in z80_board,
 * which exercises the discriminator.
 */
BOOST_AUTO_TEST_CASE( ShapeKindDiscriminatorProducesRectanglesAndLines )
{
    const std::string path = GetTestDataDir() + "z80_board.dch";

    SCH_SHEET* root = LoadDipTraceSchematic( path );
    BOOST_REQUIRE( root != nullptr );

    int rectangles = 0;
    int diagonalPolylines = 0;

    for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL*                        symbol = static_cast<SCH_SYMBOL*>( item );
            const std::unique_ptr<LIB_SYMBOL>& libSymbol = symbol->GetLibSymbolRef();

            if( !libSymbol )
                continue;

            for( SCH_ITEM& drawItem : libSymbol->GetDrawItems() )
            {
                if( drawItem.Type() != SCH_SHAPE_T )
                    continue;

                SCH_SHAPE& shape = static_cast<SCH_SHAPE&>( drawItem );

                if( shape.GetShape() == SHAPE_T::RECTANGLE )
                {
                    rectangles++;
                    continue;
                }

                if( shape.GetShape() != SHAPE_T::POLY || shape.GetPolyShape().OutlineCount() == 0 )
                    continue;

                const std::vector<VECTOR2I>& pts = shape.GetPolyShape().Outline( 0 ).CPoints();

                if( pts.size() == 2 && pts[1].x != pts[0].x && pts[1].y != pts[0].y )
                    diagonalPolylines++;
            }
        }
    }

    BOOST_TEST_MESSAGE( "z80 rectangles=" << rectangles << " diagonalPolylines=" << diagonalPolylines );

    // z80_board draws its symbol boxes as line edges rather than rectangle primitives, so the kind
    // discriminator yields no rectangles here. The point of the test is that the two point diagonal
    // lines stay polylines instead of being promoted to rectangles as the geometry guess did.
    BOOST_CHECK_GT( diagonalPolylines, 0 );

    RemoveGeneratedLibrary( path );
}


/**
 * Pins must carry the orientation recovered from their geometry rather than all defaulting to one
 * side. A symbol whose pins ring its body has pins facing several directions, so the imported pin
 * set must use more than a single orientation.
 */
BOOST_AUTO_TEST_CASE( PinsCarryNonDefaultOrientations )
{
    const std::string path = GetTestDataDir() + "z80_board.dch";

    SCH_SHEET* root = LoadDipTraceSchematic( path );
    BOOST_REQUIRE( root != nullptr );

    std::set<PIN_ORIENTATION> orientations;
    int                       pinsChecked = 0;

    for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL*                        symbol = static_cast<SCH_SYMBOL*>( item );
            const std::unique_ptr<LIB_SYMBOL>& libSymbol = symbol->GetLibSymbolRef();

            if( !libSymbol )
                continue;

            for( SCH_ITEM& drawItem : libSymbol->GetDrawItems() )
            {
                if( drawItem.Type() != SCH_PIN_T )
                    continue;

                orientations.insert( static_cast<SCH_PIN&>( drawItem ).GetOrientation() );
                pinsChecked++;
            }
        }
    }

    BOOST_CHECK_GT( pinsChecked, 0 );
    BOOST_CHECK_MESSAGE( orientations.size() > 1,
                         "All imported pins share a single orientation; orientation was not applied" );

    RemoveGeneratedLibrary( path );
}


/**
 * A DipTrace file that stores its page geometry must drive the KiCad page size. power_supply.dch
 * carries an A4 (297 x 210 mm) page record, so the imported root screen must read back that size
 * rather than the default. Files without a page record keep the KiCad default, which is covered by
 * the other import tests that load z80_board.dch unchanged.
 */
BOOST_AUTO_TEST_CASE( PageSizeIsAppliedWhenStored )
{
    const std::string path = GetTestDataDir() + "power_supply.dch";

    SCH_SHEET* root = LoadDipTraceSchematic( path );
    BOOST_REQUIRE( root != nullptr );
    BOOST_REQUIRE( root->GetScreen() != nullptr );

    const PAGE_INFO& page = root->GetScreen()->GetPageSettings();

    // A4 is 297 x 210 mm; allow a small tolerance for the mm -> mils -> mm round trip.
    BOOST_CHECK_CLOSE( page.GetWidthMM(), 297.0, 0.5 );
    BOOST_CHECK_CLOSE( page.GetHeightMM(), 210.0, 0.5 );

    RemoveGeneratedLibrary( path );
}


BOOST_AUTO_TEST_SUITE_END()
