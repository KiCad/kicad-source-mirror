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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_io/orcad/sch_io_orcad.h>

#include <schematic.h>
#include <connection_graph.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_label.h>
#include <sch_pin.h>
#include <lib_symbol.h>
#include <reporter.h>
#include <settings/settings_manager.h>

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>


namespace
{

/// Throw-away temp file; impostor fixtures synthesized at runtime since user designs not redistributable.
struct TEMP_TEST_FILE
{
    TEMP_TEST_FILE( const wxString& aFileName, const wxString& aContents ) :
            m_path( wxFileName( wxFileName::GetTempDir(), aFileName ).GetFullPath() )
    {
        wxFFile file( m_path, wxS( "w" ) );

        if( file.IsOpened() )
            file.Write( aContents );
    }

    ~TEMP_TEST_FILE() { wxRemoveFile( m_path ); }

    wxString m_path;
};

} // namespace


struct ORCAD_SCH_IMPORT_FIXTURE
{
    ORCAD_SCH_IMPORT_FIXTURE() :
            m_schematic( new SCHEMATIC( nullptr ) )
    {
        m_manager.LoadProject( "" );
        m_schematic->SetProject( &m_manager.Prj() );
        m_schematic->CurrentSheet().clear();
        m_schematic->CurrentSheet().push_back( &m_schematic->Root() );
    }

    ~ORCAD_SCH_IMPORT_FIXTURE()
    {
        m_schematic.reset();
    }

    std::string dataPath( const std::string& aRelPath ) const
    {
        return KI_TEST::GetEeschemaTestDataDir() + "io/orcad/" + aRelPath;
    }

    SCH_SHEET* LoadOrcadSchematic( const std::string& aRelPath )
    {
        return m_plugin.LoadSchematicFile( dataPath( aRelPath ), m_schematic.get() );
    }

    SCH_IO_ORCAD               m_plugin;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SETTINGS_MANAGER           m_manager;
};


BOOST_FIXTURE_TEST_SUITE( OrcadSchImport, ORCAD_SCH_IMPORT_FIXTURE )


// ============================================================================
// File discrimination tests
//
// .dsn shared with SPECCTRA PCB text files; OrCAD Capture is OLE2/CFB (magic
// D0 CF 11 E0...) with a "Library" stream and "Views"/"Schematics" storage.
// Anything failing those checks rejected.
// ============================================================================

BOOST_AUTO_TEST_CASE( RejectsSpecctraTextDsn )
{
    TEMP_TEST_FILE specctra( wxS( "qa_orcad_specctra_impostor.dsn" ),
                             wxS( "(pcb \"impostor.dsn\"\n  (parser\n    (string_quote \")\n  )\n)\n" ) );

    BOOST_REQUIRE( wxFileName::FileExists( specctra.m_path ) );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( specctra.m_path ) );
}


BOOST_AUTO_TEST_CASE( RejectsNonexistentFile )
{
    wxFileName missing( wxFileName::GetTempDir(), wxS( "qa_orcad_does_not_exist.dsn" ) );

    BOOST_REQUIRE( !missing.FileExists() );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( missing.GetFullPath() ) );
}


BOOST_AUTO_TEST_CASE( RejectsWrongExtension )
{
    TEMP_TEST_FILE textFile( wxS( "qa_orcad_impostor.txt" ),
                             wxS( "Just some text, not a schematic.\n" ) );

    BOOST_REQUIRE( wxFileName::FileExists( textFile.m_path ) );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( textFile.m_path ) );
}


// No positive-load test until a redistributable .dsn fixture exists under qa/data/eeschema/io/orcad/.


// ============================================================================
// Corpus validation (opt-in via KICAD_ORCAD_CORPUS)
//
// Sample designs not redistributable; skipped unless KICAD_ORCAD_CORPUS names a
// tree of .DSN files. Each design imported, its placed-component refdes set
// cross-checked against OrCAD-exported ground truth beside it: .NET (Cadstar
// RINF, ".ADD_COM <ref>" per part) and/or .BOM (tab-separated, comma-joined
// reference cell). Refdes coverage is the strongest available import check.
// ============================================================================

static std::string trimCell( std::string aText )
{
    auto notSpace = []( unsigned char c ) { return !std::isspace( c ); };
    aText.erase( aText.begin(), std::find_if( aText.begin(), aText.end(), notSpace ) );
    aText.erase( std::find_if( aText.rbegin(), aText.rend(), notSpace ).base(), aText.end() );

    if( aText.size() >= 2 && aText.front() == '"' && aText.back() == '"' )
        aText = aText.substr( 1, aText.size() - 2 );

    return aText;
}


static std::vector<std::string> splitRefs( const std::string& aCell )
{
    std::vector<std::string> refs;
    std::string              token;

    for( char c : aCell )
    {
        if( c == ',' )
        {
            std::string r = trimCell( token );

            if( !r.empty() )
                refs.push_back( r );

            token.clear();
        }
        else
        {
            token += c;
        }
    }

    std::string r = trimCell( token );

    if( !r.empty() )
        refs.push_back( r );

    return refs;
}


static std::set<std::string> parseBomRefs( const std::string& aPath )
{
    std::set<std::string> refs;
    std::ifstream         in( aPath );
    std::string           line;
    int                   refCol = -1;

    while( std::getline( in, line ) )
    {
        if( !line.empty() && line.back() == '\r' )
            line.pop_back();

        std::vector<std::string> cols;
        std::string              cell;

        for( char c : line )
        {
            if( c == '\t' ) { cols.push_back( cell ); cell.clear(); }
            else            { cell += c; }
        }

        cols.push_back( cell );

        // Header row names reference column; capture index once
        if( refCol < 0 )
        {
            for( size_t i = 0; i < cols.size(); ++i )
            {
                if( trimCell( cols[i] ) == "Reference" )
                {
                    refCol = static_cast<int>( i );
                    break;
                }
            }

            continue;
        }

        if( refCol < static_cast<int>( cols.size() ) )
        {
            for( const std::string& r : splitRefs( trimCell( cols[refCol] ) ) )
                refs.insert( r );
        }
    }

    return refs;
}


static std::set<std::string> parseNetComs( const std::string& aPath )
{
    std::set<std::string> refs;
    std::ifstream         in( aPath );
    std::string           line;

    while( std::getline( in, line ) )
    {
        if( line.rfind( ".ADD_COM", 0 ) != 0 )
            continue;

        // .ADD_COM <ref> "<footprint>"
        std::string rest = trimCell( line.substr( 8 ) );
        std::string ref;

        for( char c : rest )
        {
            if( std::isspace( static_cast<unsigned char>( c ) ) )
                break;

            ref += c;
        }

        if( !ref.empty() )
            refs.insert( ref );
    }

    return refs;
}


/// Unique refdes of real (non-power) parts. Annotated refs live in per-sheet-path instance data,
/// so walk sheet list and query GetRef() per path (same screen may recur on several paths).
static std::set<std::string> collectImportedRefs( SCHEMATIC& aSchematic )
{
    std::set<std::string> refs;
    SCH_SHEET_LIST        sheets = aSchematic.BuildSheetListSortedByPageNumbers();

    for( const SCH_SHEET_PATH& path : sheets )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetRef( &path, false );

            // Leading '#' = power/hidden pseudo-part, not BOM; trailing '?' = unannotated.
            if( ref.IsEmpty() || ref.StartsWith( wxS( "#" ) ) || ref.EndsWith( wxS( "?" ) ) )
                continue;

            refs.insert( std::string( ref.ToUTF8() ) );
        }
    }

    return refs;
}


static std::string terminalToken( const std::string& aRef, const std::string& aPin )
{
    return trimCell( aRef ) + "." + trimCell( aPin );
}


/// Net terminal sets from .NET. Net starts at `.ADD_TER <ref> <pin>`, gathers following
/// `.TER`/indented `<ref> <pin>` continuations until next net.
static std::vector<std::set<std::string>> parseNetTerminals( const std::string& aPath )
{
    std::vector<std::set<std::string>> nets;
    std::set<std::string>              current;
    std::ifstream                      in( aPath );
    std::string                        line;

    auto flush = [&]()
    {
        if( current.size() >= 2 )
            nets.push_back( current );

        current.clear();
    };

    while( std::getline( in, line ) )
    {
        if( !line.empty() && line.back() == '\r' )
            line.pop_back();

        bool addTer = line.rfind( ".ADD_TER", 0 ) == 0;
        bool ter = line.rfind( ".TER", 0 ) == 0;
        bool cont = !line.empty() && std::isspace( static_cast<unsigned char>( line[0] ) );

        if( line.rfind( ".END", 0 ) == 0 )
            break;

        if( addTer )
            flush();

        if( addTer || ter || cont )
        {
            std::istringstream ss( addTer ? line.substr( 8 ) : ter ? line.substr( 4 ) : line );
            std::string        ref, pin;

            if( ss >> ref >> pin )
                current.insert( terminalToken( ref, pin ) );
        }
    }

    flush();
    return nets;
}


/// Count ground-truth nets whose resolvable terminals all land on one KiCad net after
/// connectivity rebuild. Returns {consistent, checkable}.
static std::pair<int, int> checkConnectivity( SCHEMATIC& aSchematic,
                                       const std::vector<std::set<std::string>>& aNets )
{
    SCH_SHEET_LIST sheets = aSchematic.BuildSheetListSortedByPageNumbers();
    aSchematic.ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::string, int> pinNet;
    int                        netId = 0;

    for( const auto& [key, subgraphs] : aSchematic.ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( !symbol )
                    continue;

                wxString ref = symbol->GetRef( &subgraph->GetSheet(), false );

                if( ref.IsEmpty() || ref.StartsWith( wxS( "#" ) ) || ref.EndsWith( wxS( "?" ) ) )
                    continue;

                pinNet[terminalToken( std::string( ref.ToUTF8() ),
                                      std::string( pin->GetNumber().ToUTF8() ) )] = netId;
            }
        }

        ++netId;
    }

    int consistent = 0, checkable = 0;

    for( const std::set<std::string>& net : aNets )
    {
        std::set<int> ids;
        int           resolved = 0;

        for( const std::string& term : net )
        {
            auto it = pinNet.find( term );

            if( it != pinNet.end() )
            {
                ids.insert( it->second );
                ++resolved;
            }
        }

        if( resolved >= 2 )
        {
            ++checkable;

            if( ids.size() == 1 )
                ++consistent;
        }
    }

    return { consistent, checkable };
}


/// Ground-truth companion beside .DSN, .NET preferred over .BOM.
static std::set<std::string> expectedRefsFor( const std::filesystem::path& aDsn,
                                              std::string& aSource )
{
    for( const char* ext : { ".NET", ".net", ".BOM", ".bom" } )
    {
        std::filesystem::path candidate = aDsn;
        candidate.replace_extension( ext );

        if( std::filesystem::exists( candidate ) )
        {
            aSource = candidate.filename().string();

            bool isNet = std::string( ext ) == ".NET" || std::string( ext ) == ".net";
            return isNet ? parseNetComs( candidate.string() )
                         : parseBomRefs( candidate.string() );
        }
    }

    aSource.clear();
    return {};
}


BOOST_AUTO_TEST_CASE( CorpusValidation )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD corpus validation." );
        return;
    }

    namespace fs = std::filesystem;
    fs::path root( corpusEnv );

    BOOST_REQUIRE_MESSAGE( fs::exists( root ), "KICAD_ORCAD_CORPUS path does not exist." );

    std::vector<fs::path> designs;

    for( auto it = fs::recursive_directory_iterator( root,
                                                     fs::directory_options::skip_permission_denied );
         it != fs::recursive_directory_iterator(); ++it )
    {
        if( !it->is_regular_file() )
            continue;

        std::string ext = it->path().extension().string();
        std::transform( ext.begin(), ext.end(), ext.begin(),
                        []( unsigned char c ) { return std::tolower( c ); } );

        if( ext == ".dsn" )
            designs.push_back( it->path() );
    }

    std::sort( designs.begin(), designs.end() );

    BOOST_TEST_MESSAGE( "OrCAD corpus: " << designs.size() << " .DSN files under " << root );

    int          imported = 0, crashed = 0, unsupported = 0, rejected = 0, checked = 0;
    unsigned int totalExpected = 0, totalMatched = 0, totalMissing = 0, totalExtra = 0;
    int          netConsistent = 0, netCheckable = 0, netTotal = 0;

    const char* debugEnv = std::getenv( "KICAD_ORCAD_DEBUG" );
    std::string debugFilter = debugEnv ? debugEnv : "";

    for( const fs::path& dsn : designs )
    {
        std::string rel = fs::relative( dsn, root ).string();
        SCH_IO_ORCAD plugin;

        bool debug = !debugFilter.empty() && rel.find( debugFilter ) != std::string::npos;

        if( !plugin.CanReadSchematicFile( dsn.string() ) )
        {
            ++rejected;
            continue;
        }

        std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
        SETTINGS_MANAGER           manager;
        manager.LoadProject( "" );
        schematic->SetProject( &manager.Prj() );
        schematic->CurrentSheet().clear();
        schematic->CurrentSheet().push_back( &schematic->Root() );

        WX_STRING_REPORTER reporter;
        plugin.SetReporter( &reporter );

        try
        {
            plugin.LoadSchematicFile( dsn.string(), schematic.get() );
            schematic->CurrentSheet().UpdateAllScreenReferences();
        }
        catch( const std::exception& e )
        {
            // Pre-2003 designs out of scope, rejected cleanly
            if( std::string( e.what() ).find( "pre-2003" ) != std::string::npos )
                ++unsupported;
            else
                ++crashed;

            BOOST_TEST_MESSAGE( "  THROW   " << rel << " : " << e.what() );
            continue;
        }

        ++imported;

        if( debug )
            BOOST_TEST_MESSAGE( "  DEBUG   " << rel << " warnings:\n"
                                             << std::string( reporter.GetMessages().ToUTF8() ) );

        std::set<std::string> got = collectImportedRefs( *schematic );
        std::string           source;
        std::set<std::string> expected = expectedRefsFor( dsn, source );

        if( debug )
            BOOST_TEST_MESSAGE( "  DEBUG   " << rel << " imported " << got.size() << " refs" );

        if( expected.empty() )
            continue;

        std::set<std::string> missing, extra;
        std::set_difference( expected.begin(), expected.end(), got.begin(), got.end(),
                             std::inserter( missing, missing.begin() ) );
        std::set_difference( got.begin(), got.end(), expected.begin(), expected.end(),
                             std::inserter( extra, extra.begin() ) );

        unsigned int matched = static_cast<unsigned int>( expected.size() - missing.size() );

        ++checked;
        totalExpected += expected.size();
        totalMatched += matched;
        totalMissing += missing.size();
        totalExtra += extra.size();

        BOOST_TEST_MESSAGE( "  CHECK   " << rel << " : " << matched << "/" << expected.size()
                                         << " refs (" << int( 100.0 * matched / expected.size() )
                                         << "%), extra " << extra.size() << " [" << source << "]" );

        // .NET ground truth carries terminal connectivity; verify pins group per net after rebuild.
        std::filesystem::path net = dsn;
        net.replace_extension( source.size() >= 4 && source.substr( source.size() - 4 ) == ".net"
                                       ? ".net"
                                       : ".NET" );

        if( std::filesystem::exists( net ) )
        {
            std::vector<std::set<std::string>> nets = parseNetTerminals( net.string() );

            if( !nets.empty() )
            {
                auto [consistent, checkableNets] = checkConnectivity( *schematic, nets );
                netConsistent += consistent;
                netCheckable += checkableNets;
                netTotal += static_cast<int>( nets.size() );

                BOOST_TEST_MESSAGE( "          connectivity: " << consistent << "/" << checkableNets
                                    << " nets consistent" );
            }
        }
    }

    BOOST_TEST_MESSAGE( "==== OrCAD corpus summary ====" );
    BOOST_TEST_MESSAGE( "  designs: " << designs.size() << "  imported: " << imported
                                      << "  crashed: " << crashed << "  unsupported: " << unsupported
                                      << "  rejected: " << rejected );

    if( checked )
    {
        BOOST_TEST_MESSAGE( "  refdes coverage: " << totalMatched << "/" << totalExpected << " ("
                                                  << int( 100.0 * totalMatched / totalExpected )
                                                  << "%)  missing: " << totalMissing
                                                  << "  extra: " << totalExtra );
    }

    if( netCheckable )
    {
        BOOST_TEST_MESSAGE( "  net connectivity: " << netConsistent << "/" << netCheckable << " ("
                                                   << int( 100.0 * netConsistent / netCheckable )
                                                   << "%)" );
    }

    // Only pre-2003 format may throw; anything else is a crash
    BOOST_CHECK_MESSAGE( crashed == 0, crashed << " design(s) crashed during import." );

    // Guard against vacuous pass when no companion files present
    BOOST_REQUIRE_MESSAGE( checked > 0, "No ground-truth .BOM/.NET companions were validated." );

    // Occurrence-annotation decode holds this above 95%; dropped Hierarchy-stream ref overlay collapses it.
    BOOST_CHECK_GE( 100.0 * totalMatched / totalExpected, 95.0 );

    // Pin-placement/geometry regression breaking connectivity collapses this. Checkable floor
    // (>= 2 resolvable terminals per net) stops broad pin loss passing vacuously.
    if( netTotal )
    {
        BOOST_CHECK_GE( 100.0 * netCheckable / netTotal, 80.0 );
        BOOST_CHECK_GE( 100.0 * netConsistent / netCheckable, 90.0 );
    }
}


// CutiePi (3 pages) imports as three sibling top-level sheets; off-page connectors keep own
// names; reference/value fields honor source display positions.
BOOST_AUTO_TEST_CASE( MultiPageFlatImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD multi-page import." );
        return;
    }

    namespace fs = std::filesystem;
    fs::path dsn = fs::path( corpusEnv ) / "cutiepi-board" / "CutiePi_V2.3-20210409.DSN";

    if( !fs::exists( dsn ) )
    {
        BOOST_TEST_MESSAGE( "CutiePi design not present in corpus; skipping." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    // Pages become flat ordered top-level sheets, not a stitching root w/ children; "N - " prefix orders them.
    std::vector<SCH_SHEET*> tops = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( tops.size(), 3u );

    BOOST_CHECK_EQUAL( tops[0]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "CONTENTS" ) );
    BOOST_CHECK_EQUAL( tops[1]->GetField( FIELD_T::SHEET_NAME )->GetText(),
                       wxS( "CM4,USB HUB,AUDIO,MIC" ) );
    BOOST_CHECK_EQUAL( tops[2]->GetField( FIELD_T::SHEET_NAME )->GetText(),
                       wxS( "CSI, DSI, HDMI, MCU" ) );

    std::set<wxString> globalLabels;

    for( SCH_SHEET* top : tops )
    {
        for( SCH_ITEM* item : top->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
            globalLabels.insert( static_cast<SCH_LABEL_BASE*>( item )->GetText() );
    }

    // Off-page connectors carry own name (CAM0_IO1), not the local wire net (GPIO19) they sit on.
    BOOST_CHECK( globalLabels.count( wxS( "CAM0_IO1" ) ) );
    BOOST_CHECK( globalLabels.count( wxS( "AMP_SHUTDOWN" ) ) );

    // R3197 reference honors OrCAD display position (left of body), not computed fallback (right).
    bool checkedField = false;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

            if( sym->GetRef( &path, false ) == wxS( "R3197" ) )
            {
                BOOST_CHECK_LT( sym->GetField( FIELD_T::REFERENCE )->GetPosition().x,
                                sym->GetPosition().x );
                checkedField = true;
            }
        }
    }

    BOOST_CHECK( checkedField );
}


// CutiePi component fidelity: pin number/name visibility, off-page label orientation, hidden
// fields, no-connect markers.
BOOST_AUTO_TEST_CASE( ComponentDetailImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD component detail." );
        return;
    }

    namespace fs = std::filesystem;
    fs::path dsn = fs::path( corpusEnv ) / "cutiepi-board" / "CutiePi_V2.3-20210409.DSN";

    if( !fs::exists( dsn ) )
    {
        BOOST_TEST_MESSAGE( "CutiePi design not present in corpus; skipping." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, SCH_SYMBOL*> symbols;
    std::multimap<wxString, int>    labelSpins;
    int                             noConnects = 0;

    for( SCH_SHEET* top : schematic->GetTopLevelSheets() )
    {
        SCH_SHEET_PATH path;
        path.push_back( top );

        for( SCH_ITEM* item : top->GetScreen()->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
                symbols[sym->GetRef( &path, false )] = sym;
            }
            else if( item->Type() == SCH_GLOBAL_LABEL_T )
            {
                SCH_LABEL_BASE* lbl = static_cast<SCH_LABEL_BASE*>( item );
                labelSpins.emplace( lbl->GetText(), (int) lbl->GetSpinStyle() );
            }
            else if( item->Type() == SCH_NO_CONNECT_T )
            {
                ++noConnects;
            }
        }
    }

    // Pin numbers/names show on ICs (flags 0x3), hide on passives (0x6)
    BOOST_REQUIRE( symbols.count( wxS( "U3" ) ) );
    BOOST_CHECK( symbols[wxS( "U3" )]->GetShowPinNumbers() );
    BOOST_CHECK( symbols[wxS( "U3" )]->GetShowPinNames() );
    BOOST_REQUIRE( symbols.count( wxS( "R3174" ) ) );
    BOOST_CHECK( !symbols[wxS( "R3174" )]->GetShowPinNumbers() );
    BOOST_CHECK( !symbols[wxS( "R3174" )]->GetShowPinNames() );

    // Ferrite bead value hidden, reference visible
    BOOST_REQUIRE( symbols.count( wxS( "FB8" ) ) );
    BOOST_CHECK( !symbols[wxS( "FB8" )]->GetField( FIELD_T::VALUE )->IsVisible() );
    BOOST_CHECK( symbols[wxS( "FB8" )]->GetField( FIELD_T::REFERENCE )->IsVisible() );

    // Display-prop field positions are canvas-space (anchor + offset), not through body-orientation
    // transform. FB8 (90-deg ferrite) reference lands right of origin; rotation transform would flip left.
    SCH_FIELD* fb8Ref = symbols[wxS( "FB8" )]->GetField( FIELD_T::REFERENCE );
    BOOST_CHECK_GT( fb8Ref->GetPosition().x, symbols[wxS( "FB8" )]->GetPosition().x );
    BOOST_CHECK( fb8Ref->GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT );

    // FB8 stored angle compensates for KiCad re-rotating fields on 90-deg symbol, so text stays horizontal.
    BOOST_CHECK( fb8Ref->GetDrawRotation() == ANGLE_HORIZONTAL );

    // References render horizontal even on rotated symbols (ferrites, vertical R/C).
    for( const wxString& ref : { wxS( "R3186" ), wxS( "C2517" ), wxS( "R3189" ),
                                 wxS( "FB13" ), wxS( "FB9" ) } )
    {
        BOOST_REQUIRE_MESSAGE( symbols.count( ref ), ref );
        BOOST_CHECK( symbols[ref]->GetField( FIELD_T::REFERENCE )->GetDrawRotation()
                     == ANGLE_HORIZONTAL );
    }

    // Value rotation is per-field from source: FB9 part number horizontal, C2517 "47pF" stays vertical.
    BOOST_CHECK( symbols[wxS( "FB9" )]->GetField( FIELD_T::VALUE )->GetDrawRotation()
                 == ANGLE_HORIZONTAL );
    BOOST_CHECK( symbols[wxS( "C2517" )]->GetField( FIELD_T::VALUE )->GetDrawRotation()
                 == ANGLE_VERTICAL );

    // Power net names read horizontal even on rotated power symbols (REG1V8/REG3V3).
    bool checkedPower = false;

    for( SCH_SHEET* top : schematic->GetTopLevelSheets() )
    {
        for( SCH_ITEM* item : top->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
            wxString    val = sym->GetField( FIELD_T::VALUE )->GetText();

            if( val == wxS( "REG1V8" ) || val == wxS( "REG3V3" ) )
            {
                BOOST_CHECK( sym->GetField( FIELD_T::VALUE )->GetDrawRotation()
                             == ANGLE_HORIZONTAL );
                checkedPower = true;
            }
        }
    }

    BOOST_CHECK( checkedPower );

    // Unconnected IC pins get no-connect markers (U3 15 NC + U580 NC/ORG)
    BOOST_CHECK_GE( noConnects, 17 );

    // Off-page connectors on vertical wires point up/down, not left/right.
    auto hasSpin = [&]( const wxString& aText, SPIN_STYLE::SPIN aSpin )
    {
        auto range = labelSpins.equal_range( aText );

        for( auto it = range.first; it != range.second; ++it )
        {
            if( it->second == (int) aSpin )
                return true;
        }

        return false;
    };

    BOOST_CHECK( hasSpin( wxS( "VOLDN" ), SPIN_STYLE::UP ) );
    BOOST_CHECK( hasSpin( wxS( "MUTEP" ), SPIN_STYLE::BOTTOM ) );
}


BOOST_AUTO_TEST_SUITE_END()
