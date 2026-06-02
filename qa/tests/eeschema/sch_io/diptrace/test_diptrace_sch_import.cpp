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
#include <eeschema/sch_io/diptrace/diptrace_sch_parser.h>

#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <set>
#include <sstream>
#include <vector>

#include <reporter.h>

#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/xml/xml.h>


/// Reporter that records the severity of every message so a test can assert that an import ran
/// without warnings or errors.
class DIPTRACE_COUNTING_REPORTER : public REPORTER
{
public:
    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
    {
        m_messages.emplace_back( aSeverity, aText );
        return *this;
    }

    bool HasMessage() const override { return !m_messages.empty(); }

    int CountOfSeverity( int aSeverityMask ) const
    {
        int n = 0;

        for( const auto& [sev, text] : m_messages )
        {
            if( sev & aSeverityMask )
                n++;
        }

        return n;
    }

    std::string MessagesOfSeverity( int aSeverityMask ) const
    {
        std::ostringstream out;

        for( const auto& [sev, text] : m_messages )
        {
            if( !( sev & aSeverityMask ) )
                continue;

            if( out.tellp() > 0 )
                out << "; ";

            out << text.ToStdString();
        }

        return out.str();
    }

    void Clear() override { m_messages.clear(); }

    std::vector<std::pair<SEVERITY, wxString>> m_messages;
};


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
    DIPTRACE_COUNTING_REPORTER m_reporter;

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
        int partCount    = 0;
        int netPortCount = 0;   ///< Parts with PartType="Net Port" (imported as global labels)
        int sheetCount   = 0;
    };

    static bool XmlSubtreeContains( wxXmlNode* aNode, const wxString& aNeedle )
    {
        for( wxXmlNode* n = aNode; n; n = n->GetNext() )
        {
            if( n->GetType() == wxXML_TEXT_NODE && n->GetContent().Contains( aNeedle ) )
                return true;

            if( XmlSubtreeContains( n->GetChildren(), aNeedle ) )
                return true;
        }

        return false;
    }

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
        int                   netPortCount = 0;
        int                   sheetCount = 0;
        std::vector<wxXmlNode*> stack = { schematicNode };

        while( !stack.empty() )
        {
            wxXmlNode* node = stack.back();
            stack.pop_back();

            if( !node || node->GetType() != wxXML_ELEMENT_NODE )
                continue;

            if( node->GetName() == "Part" )
            {
                partCount++;

                // Net-port placements carry an auto_net_ports library reference; the importer maps
                // these to global labels rather than symbols, so count them the same way here.
                if( XmlSubtreeContains( node->GetChildren(), wxT( "auto_net_ports" ) ) )
                    netPortCount++;
            }
            else if( node->GetName() == "Sheet" )
            {
                sheetCount++;
            }

            for( wxXmlNode* child = node->GetChildren(); child; child = child->GetNext() )
            {
                stack.push_back( child );
            }
        }

        aCounts.partCount = partCount;
        aCounts.netPortCount = netPortCount;
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

        m_reporter.Clear();
        m_plugin.SetReporter( &m_reporter );
        m_loadedRoot = m_plugin.LoadSchematicFile( aFilePath, m_schematic.get() );
        return m_loadedRoot;
    }

    void RemoveGeneratedLibrary( const std::string& aFilePath )
    {
        wxFileName genLib( aFilePath );
        genLib.SetName( genLib.GetName() + wxT( "-diptrace-import" ) );
        genLib.SetExt( wxT( "kicad_sym" ) );

        if( genLib.FileExists() )
            wxRemoveFile( genLib.GetFullPath() );
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


BOOST_AUTO_TEST_CASE( InvalidComponentCountFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_count_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset COMPONENT_COUNT_OFFSET = 0xEE;
        const uint8_t invalidCount[] = { 0x12, 0x4F, 0x81 }; // int3 value 200001

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( COMPONENT_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidCount, sizeof( invalidCount ) ),
                             sizeof( invalidCount ) );
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

    DIPTRACE::SCH_PARSER parser( wxString::FromUTF8( path ), m_schematic.get(), rootSheet,
                                 nullptr, &m_reporter );
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

    BOOST_CHECK_MESSAGE( !genLib.FileExists(),
                         "DipTrace import must not create a symbol library file: "
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

    BOOST_CHECK_MESSAGE( foundEmbeddedSymbol,
                         "Imported schematic must carry embedded symbol definitions." );

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


/**
 * A multi-sheet DipTrace design must expose every sub-sheet through the schematic hierarchy, not
 * just the root. Optional: only runs when the external dev-driver corpus is present.
 */
BOOST_AUTO_TEST_CASE( MultiSheetImportExposesFullHierarchyOptional )
{
    const char* corpusEnv = std::getenv( "DIPTRACE_EXTERNAL_CORPUS_DIR" );

    if( !corpusEnv || !*corpusEnv )
        return;

    const std::string path = std::string( corpusEnv ) + "/dev-driver/DriverModule.dch";

    if( !wxFileExists( path ) )
        return;

    SCH_SHEET* root = LoadDipTraceSchematic( path );
    BOOST_REQUIRE( root != nullptr );

    BOOST_CHECK_EQUAL( m_schematic->RootScreen(), root->GetScreen() );

    // DriverModule.dch declares 19 sheets; every one must be reachable from the hierarchy.
    SCH_SHEET_LIST hierarchy = m_schematic->BuildUnorderedSheetList();
    BOOST_CHECK_EQUAL( hierarchy.size(), 19u );

    RemoveGeneratedLibrary( path );
}


BOOST_AUTO_TEST_CASE( ViewerExampleComponentRecordsAreParsedSequentiallyOptional )
{
    const std::string examplesDir = GetViewerExamplesDir();
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

        DIPTRACE::SCH_PARSER parser( wxString::FromUTF8( path ), m_schematic.get(), rootSheet,
                                     nullptr, &m_reporter );
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
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_count_mismatch_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset COMPONENT_COUNT_OFFSET = 0xEE;
        const uint8_t mismatchedCount[] = { 0x0F, 0x42, 0xC5 }; // int3 value 133

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( COMPONENT_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( mismatchedCount, sizeof( mismatchedCount ) ),
                             sizeof( mismatchedCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidComponentPinCountFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_pin_count_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_PIN_COUNT_OFFSET = 0x20A;
        const uint8_t invalidPinCount[] = { 0x12, 0x4F, 0x81 }; // int3 value 200001

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_COMPONENT_PIN_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidPinCount, sizeof( invalidPinCount ) ),
                             sizeof( invalidPinCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidComponentExtraTailLengthFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_extra_tail_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_EXTRA_TAIL_COUNT_OFFSET = 0x206;
        const uint8_t invalidExtraTailCount[] = { 0x00, 0x00, 0x27, 0x10 }; // u4 count 10000

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
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_pin_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_FIRST_PIN_NAME_LEN_OFFSET = 0x226;
        const uint8_t invalidNameLength[] = { 0xFF, 0xFF };

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
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_later_pin_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_SECOND_PIN_NAME_LEN_OFFSET = 0x272;
        const uint8_t invalidNameLength[] = { 0xFF, 0xFF };

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
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_shape_points_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_SHAPE_POINT_COUNT_OFFSET = 0x2C2;
        const uint8_t invalidPointCount[] = { 0x0F, 0x42, 0xA5 }; // int3 value 101

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
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_zero_shape_points_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_COMPONENT_SHAPE_POINT_COUNT_OFFSET = 0x2C2;
        const uint8_t zeroPointCount[] = { 0x0F, 0x42, 0x40 }; // int3 value 0

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_COMPONENT_SHAPE_POINT_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( zeroPointCount, sizeof( zeroPointCount ) ),
                             sizeof( zeroPointCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidBusEntryTerminatorFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_bus_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_BUS_TERMINATOR_OFFSET = 0x39484;
        const uint8_t invalidTerminator[] = { 0x0F, 0x42, 0x40 }; // int3 value 0

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
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_bus_count_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset BUS_SECTION_COUNT_OFFSET = 0x3946A;
        const uint8_t invalidCount[] = { 0x0F, 0x46, 0x29 }; // int3 value 1001

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( BUS_SECTION_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidCount, sizeof( invalidCount ) ),
                             sizeof( invalidCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidWireNetPinCountFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_wire_pin_count_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_WIRE_NET_PIN_COUNT_OFFSET = 0x3988E;
        const uint8_t invalidPinCount[] = { 0x0F, 0x51, 0xE9 }; // int3 value 4001

        wxFFile file( tempPath, wxS( "r+b" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.Seek( FIRST_WIRE_NET_PIN_COUNT_OFFSET ) );
        BOOST_REQUIRE_EQUAL( file.Write( invalidPinCount, sizeof( invalidPinCount ) ),
                             sizeof( invalidPinCount ) );
    }

    BOOST_CHECK_THROW( LoadDipTraceSchematic( tempPath.ToStdString() ), IO_ERROR );

    RemoveGeneratedLibrary( tempPath.ToStdString() );
    wxRemoveFile( tempPath );
}


BOOST_AUTO_TEST_CASE( InvalidWireNetNameLengthFailsDeterministically )
{
    const std::string sourcePath = GetTestDataDir() + "z80_board.dch";
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_wire_name_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_WIRE_NET_NAME_LENGTH_OFFSET = 0x3987A;
        const uint8_t invalidNameLength[] = { 0xFF, 0xFF };

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
    wxString tempBase = wxFileName::CreateTempFileName( wxS( "kicad_diptrace_bad_wire_point_count_" ) );
    wxRemoveFile( tempBase );
    wxString tempPath = tempBase + wxS( ".dch" );

    BOOST_REQUIRE( wxCopyFile( sourcePath, tempPath ) );

    {
        static constexpr wxFileOffset FIRST_WIRE_POINT_COUNT_OFFSET = 0x39913;
        const uint8_t invalidPointCount[] = { 0x0F, 0x51, 0xE9 }; // int3 value 4001

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

        // DipTrace net ports import as global labels rather than symbols, so count both as
        // placement-equivalent items (the minSymbols thresholds predate the net-port split).
        const int symbolCount = CountItemsOfType( SCH_SYMBOL_T )
                                + CountItemsOfType( SCH_GLOBAL_LABEL_T );
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

        BOOST_CHECK_MESSAGE( m_reporter.CountOfSeverity( RPT_SEVERITY_ERROR ) == 0,
                             path.string() + ": import reported errors: "
                                     + m_reporter.MessagesOfSeverity( RPT_SEVERITY_ERROR ) );
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

        // Net ports import as global labels, not symbols, so compare real symbols against the
        // non-net-port part count, and verify the net ports are represented as labels.
        const int importedSymbols = CountItemsOfType( SCH_SYMBOL_T );
        const int importedLabels  = CountItemsOfType( SCH_GLOBAL_LABEL_T );
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
