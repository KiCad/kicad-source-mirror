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
#include <richio.h>

#include <font/font.h>
#include <sch_io/orcad/sch_io_orcad.h>
#include <sch_io/orcad/orcad_converter.h>
#include <sch_io/orcad/orcad_structures.h>
#include <sch_io/ole_image.h>

#include <schematic.h>
#include <import_net_map.h>
#include <connection_graph.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_no_connect.h>
#include <sch_bitmap.h>
#include <sch_shape.h>
#include <sch_text.h>
#include <sch_pin.h>
#include <lib_symbol.h>
#include <geometry/shape_compound.h>
#include <reporter.h>
#include <settings/settings_manager.h>
#include <schematic_utils/schematic_file_util.h>
#include <bitmap_base.h>

#include <wx/filename.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>


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

    ~ORCAD_SCH_IMPORT_FIXTURE() { m_schematic.reset(); }

    SCH_IO_ORCAD               m_plugin;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SETTINGS_MANAGER           m_manager;
};


BOOST_FIXTURE_TEST_SUITE( OrcadSchImport, ORCAD_SCH_IMPORT_FIXTURE )


BOOST_AUTO_TEST_CASE( StructurePrefixDepthIsTypeOwned )
{
    const std::map<int, int> expected = { { 9, 1 },  { 10, 2 }, { 2, 3 },  { 13, 4 }, { 24, 5 },
                                          { 66, 1 }, { 16, 2 }, { 12, 3 }, { 64, 4 } };

    for( const auto& [type, depth] : expected )
    {
        std::optional<size_t> actual = OrcadLongPrefixCount( type );
        BOOST_REQUIRE_MESSAGE( actual, "type " << type );
        BOOST_CHECK_EQUAL( *actual, static_cast<size_t>( depth ) );
    }

    BOOST_CHECK( !OrcadLongPrefixCount( 0 ) );
    BOOST_CHECK( !OrcadLongPrefixCount( 255 ) );
}


BOOST_AUTO_TEST_CASE( CaptureColorPalette )
{
    BOOST_CHECK( OrcadColor( 8 ) == KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 18 ) == KIGFX::COLOR4D( 0.0, 1.0, 0.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 28 ) == KIGFX::COLOR4D( 0.0, 0.0, 1.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 40 ) == KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 47 ) == KIGFX::COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 48 ) == KIGFX::COLOR4D::UNSPECIFIED );
    BOOST_CHECK( OrcadColor( 0 ) == KIGFX::COLOR4D::UNSPECIFIED );
}


BOOST_AUTO_TEST_CASE( CaptureCompoundFileName )
{
    BOOST_CHECK_EQUAL( OrcadNormalizeCfbName( std::string( "Serial I" ) + '\x02' + "O" ), "Serial I/O" );
    BOOST_CHECK_EQUAL( OrcadNormalizeCfbName( std::string( "Sch 2" ) + '\x03' + " PCI Connector" ),
                       "Sch 2: PCI Connector" );
}


BOOST_AUTO_TEST_CASE( CaptureStrokeAndFillSemantics )
{
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 0 ), schIUScale.MilsToIU( 10 ) );
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 1 ), schIUScale.MilsToIU( 30 ) );
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 2 ), schIUScale.MilsToIU( 50 ) );
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 3 ), schIUScale.MilsToIU( 10 ) );
    BOOST_CHECK_EQUAL( OrcadPageGraphicLineWidthIu( 0 ), schIUScale.MilsToIU( 5 ) );
    BOOST_CHECK_EQUAL( OrcadPageGraphicLineWidthIu( 1 ), schIUScale.MilsToIU( 30 ) );

    BOOST_CHECK( OrcadLineStyle( 0 ) == LINE_STYLE::SOLID );
    BOOST_CHECK( OrcadLineStyle( 4 ) == LINE_STYLE::DASHDOTDOT );
    BOOST_CHECK( OrcadLineStyle( 5 ) == LINE_STYLE::DEFAULT );

    BOOST_CHECK_EQUAL( OrcadDashRatios( 2 ).first, 67.0 );
    BOOST_CHECK_EQUAL( OrcadDashRatios( 2 ).second, 21.0 );
    BOOST_CHECK_EQUAL( OrcadDashRatios( 3 ).first, 3.0 );
    BOOST_CHECK_EQUAL( OrcadDashRatios( 3 ).second, 1.0 );

    BOOST_CHECK( OrcadFillType( 0, 0 ) == FILL_T::FILLED_SHAPE );
    BOOST_CHECK( OrcadFillType( 1, 0 ) == FILL_T::NO_FILL );
    BOOST_CHECK( OrcadFillType( 2, 0 ) == FILL_T::HATCH );
    BOOST_CHECK( OrcadFillType( 2, 3 ) == FILL_T::REVERSE_HATCH );
    BOOST_CHECK( OrcadFillType( 2, 4 ) == FILL_T::CROSS_HATCH );
    BOOST_CHECK( OrcadFillType( 2, 5 ) == FILL_T::CROSS_HATCH );
    constexpr uint32_t legacyModified = 0x56A2631C;
    constexpr uint32_t currentModified = 0x652CEBA8;
    BOOST_CHECK_EQUAL( OrcadHatchPitchIu( legacyModified ), schIUScale.MilsToIU( 25 ) );
    BOOST_CHECK_EQUAL( OrcadHatchPitchIu( currentModified ), schIUScale.MilsToIU( 80 ) );
    BOOST_CHECK_EQUAL( OrcadHatchLineWidthIu( legacyModified ), schIUScale.MilsToIU( 3 ) );
    BOOST_CHECK_EQUAL( OrcadHatchLineWidthIu( currentModified ), schIUScale.MilsToIU( 10 ) );

    SCH_SHAPE rect( SHAPE_T::RECTANGLE, LAYER_DEVICE );
    rect.SetPosition( VECTOR2I( 0, 0 ) );
    rect.SetEnd( VECTOR2I( schIUScale.MilsToIU( 600 ), schIUScale.MilsToIU( 500 ) ) );

    std::vector<SEG> reverseDiagonal = OrcadHatchLines( rect, 3, OrcadHatchPitchIu( legacyModified ) );
    BOOST_CHECK_GT( reverseDiagonal.size(), 40u );
    BOOST_CHECK_LT( reverseDiagonal.size(), 48u );
    BOOST_CHECK( std::all_of( reverseDiagonal.begin(), reverseDiagonal.end(),
                              []( const SEG& aLine )
                              {
                                  return static_cast<int64_t>( aLine.B.x - aLine.A.x )
                                             * ( aLine.B.y - aLine.A.y )
                                         < 0;
                              } ) );

    std::vector<SEG> diagonalCross = OrcadHatchLines( rect, 5, OrcadHatchPitchIu( legacyModified ) );
    BOOST_CHECK_GT( diagonalCross.size(), 80u );
    BOOST_CHECK_LT( diagonalCross.size(), 96u );

    std::vector<SEG> orthogonalCross = OrcadHatchLines( rect, 4, OrcadHatchPitchIu( legacyModified ) );
    BOOST_CHECK_GT( orthogonalCross.size(), 40u );
    BOOST_CHECK_LT( orthogonalCross.size(), 48u );
    BOOST_CHECK( std::any_of( orthogonalCross.begin(), orthogonalCross.end(),
                              []( const SEG& aLine ) { return aLine.A.x == aLine.B.x; } ) );
    BOOST_CHECK( std::any_of( orthogonalCross.begin(), orthogonalCross.end(),
                              []( const SEG& aLine ) { return aLine.A.y == aLine.B.y; } ) );

    std::vector<SEG> currentCross = OrcadHatchLines( rect, 5, OrcadHatchPitchIu( currentModified ) );
    BOOST_CHECK_GT( currentCross.size(), 20u );
    BOOST_CHECK_LT( currentCross.size(), 40u );
}


BOOST_AUTO_TEST_CASE( CapturePageOrder )
{
    wxString dashed = wxS( "03 - CAN" );
    wxString dotted = wxS( "02.uC" );
    wxString colon = wxS( "13:IMU" );
    wxString folder = wxS( "Sch 7: CAN Drivers" );
    wxString pager = wxS( "PAGER 8" );
    wxString underscored = wxS( "PAGE_03_HSMC CONNECTOR" );
    wxString compact = wxS( "PAGE01 OVERALL BLOCK DIAGRAM" );
    wxString plain = wxS( "Overview" );

    BOOST_CHECK_EQUAL( OrcadPageOrder( dashed ), 3 );
    BOOST_CHECK_EQUAL( dashed, wxS( "CAN" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( dotted ), 2 );
    BOOST_CHECK_EQUAL( dotted, wxS( "02.uC" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( colon ), 13 );
    BOOST_CHECK_EQUAL( colon, wxS( "13:IMU" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( folder ), 7 );
    BOOST_CHECK_EQUAL( folder, wxS( "Sch 7: CAN Drivers" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( pager ), -1 );
    BOOST_CHECK_EQUAL( pager, wxS( "PAGER 8" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( underscored ), 3 );
    BOOST_CHECK_EQUAL( underscored, wxS( "PAGE_03_HSMC CONNECTOR" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( compact ), 1 );
    BOOST_CHECK_EQUAL( compact, wxS( "PAGE01 OVERALL BLOCK DIAGRAM" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( plain ), -1 );
    BOOST_CHECK_EQUAL( plain, wxS( "Overview" ) );
}


BOOST_AUTO_TEST_CASE( EmbeddedImageFillsNonSquareSourceBox )
{
    BOOST_CHECK_EQUAL( OrcadStretchedImageSize( 718, 720, 830, 360 ), VECTOR2I( 1660, 720 ) );
    BOOST_CHECK_EQUAL( OrcadStretchedImageSize( 1702, 528, 1702, 528 ), VECTOR2I( 1702, 528 ) );
    BOOST_CHECK_EQUAL( OrcadStretchedImageSize( 800, 400, 300, 600 ), VECTOR2I( 800, 1600 ) );
}


BOOST_AUTO_TEST_CASE( WmfRenderUsesEmbeddedBoxAspectBeforeRasterization )
{
    BOOST_CHECK_EQUAL( OleWmfRenderSize( 718, 720, 4096, 720, 830.0 / 360.0 ), VECTOR2I( 1660, 720 ) );
    BOOST_CHECK_EQUAL( OleWmfRenderSize( 1702, 528, 2048, 2048, 0.0 ), VECTOR2I( 1702, 528 ) );
    BOOST_CHECK_EQUAL( OleWmfRenderSize( 1702, 528, 1000, 1000, 0.0 ), VECTOR2I( 1000, 310 ) );
}


BOOST_AUTO_TEST_CASE( OlePreviewWithMultiplePresentationStreams )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping multi-preview OLE check." );
        return;
    }

    std::filesystem::path dsn;

    for( const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator( corpusEnv ) )
    {
        if( entry.is_regular_file() && entry.path().filename() == "reServer industrial J401 Carrier Board v11.DSN" )
        {
            dsn = entry.path();
            break;
        }
    }

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "reServer industrial J401 design not present; skipping multi-preview OLE check." );
        return;
    }

    std::ifstream                               stream( dsn, std::ios::binary );
    std::vector<uint8_t>                        bytes( std::istreambuf_iterator<char>( stream ), {} );
    const std::array<std::array<uint8_t, 8>, 2> markers = {
        std::array<uint8_t, 8>{ 0x6A, 0x31, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00 },
        std::array<uint8_t, 8>{ 0x16, 0x94, 0x05, 0x00, 0x00, 0x01, 0x00, 0x00 }
    };

    for( size_t markerIndex = 0; markerIndex < markers.size(); ++markerIndex )
    {
        const auto& marker = markers[markerIndex];
        auto markerPos = std::search( bytes.begin(), bytes.end(), marker.begin(), marker.end() );
        BOOST_REQUIRE( markerPos != bytes.end() );

        size_t offset = static_cast<size_t>( std::distance( bytes.begin(), markerPos ) );
        size_t size = static_cast<size_t>( bytes[offset] ) | ( static_cast<size_t>( bytes[offset + 1] ) << 8 )
                      | ( static_cast<size_t>( bytes[offset + 2] ) << 16 )
                      | ( static_cast<size_t>( bytes[offset + 3] ) << 24 );
        BOOST_REQUIRE_LE( offset + size + 4, bytes.size() );

        OLE_IMAGE_PAYLOAD preview =
                ExtractOleImageFromPayload( { bytes.begin() + offset, bytes.begin() + offset + size + 4 } );
        BOOST_CHECK( preview.type == OLE_IMAGE_TYPE::WMF );
        BOOST_CHECK_GT( preview.data.size(), 200000u );

        wxImage image;
        BOOST_REQUIRE_MESSAGE( OleRenderWmf( preview.data, 2048, 2048, image ),
                               "marker " << markerIndex << ", preview bytes " << preview.data.size() << ", "
                                         << OleDescribeImagePayload( preview.data ) );
        BOOST_CHECK_GT( image.GetWidth(), 1000 );
        BOOST_CHECK_GT( image.GetHeight(), 500 );
    }
}


static std::string         terminalToken( const std::string& aRef, const std::string& aPin );
static std::pair<int, int> checkConnectivity( SCHEMATIC& aSchematic, const std::vector<std::set<std::string>>& aNets,
                                              std::vector<std::set<std::string>>* aInconsistent = nullptr );


static SCH_SYMBOL* findConvertedSymbol( SCH_SCREEN& aScreen, const SCH_SHEET_PATH& aPath, const wxString& aReference )
{
    for( SCH_ITEM* item : aScreen.Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &aPath, false ) == aReference )
            return symbol;
    }

    return nullptr;
}


static std::filesystem::path findCorpusDesign( const std::filesystem::path& aRoot, const std::string& aFileName );


BOOST_AUTO_TEST_CASE( OccurrencePowerAliasChainUsesAuthoritativeName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2331A-4.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    wxString c1Pin1Net;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "C1" )
                    && pin->GetNumber() == wxS( "1" ) )
                {
                    c1Pin1Net = key.Name;
                }
            }
        }
    }

    BOOST_CHECK_EQUAL( c1Pin1Net.AfterLast( '/' ), wxS( "GND" ) );
}


BOOST_AUTO_TEST_CASE( OccurrencePowerAliasJoinsDistinctChildNetNames )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "M5275EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    std::map<std::pair<wxString, wxString>, CONNECTION_SUBGRAPH*> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( !subgraph->GetSheet().LastScreen()->GetFileName().Contains( wxS( "_PSU" ) ) )
                continue;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                    pinNets[{ symbol->GetRef( &subgraph->GetSheet(), false ), pin->GetNumber() }] = subgraph;
            }
        }
    }

    BOOST_REQUIRE( pinNets.count( { wxS( "C126" ), wxS( "1" ) } ) );
    BOOST_REQUIRE( pinNets.count( { wxS( "U15" ), wxS( "3" ) } ) );
    BOOST_REQUIRE( pinNets.count( { wxS( "U15" ), wxS( "8" ) } ) );
    BOOST_CHECK_EQUAL( pinNets.at( { wxS( "C126" ), wxS( "1" ) } ),
                       pinNets.at( { wxS( "U15" ), wxS( "3" ) } ) );
    BOOST_CHECK_EQUAL( pinNets.at( { wxS( "U15" ), wxS( "8" ) } ),
                       pinNets.at( { wxS( "U15" ), wxS( "3" ) } ) );
}


BOOST_AUTO_TEST_CASE( RejectsNonexistentFile )
{
    wxFileName missing( wxFileName::GetTempDir(), wxS( "qa_orcad_does_not_exist.dsn" ) );

    BOOST_REQUIRE( !missing.FileExists() );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( missing.GetFullPath() ) );
}


// Set KICAD_ORCAD_CORPUS to test private DSN files against adjacent NET and BOM exports.

static std::string trimCell( std::string aText )
{
    auto notSpace = []( unsigned char c )
    {
        return !std::isspace( c );
    };
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
            if( c == '\t' )
            {
                cols.push_back( cell );
                cell.clear();
            }
            else
            {
                cell += c;
            }
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
static std::pair<int, int> checkConnectivity( SCHEMATIC& aSchematic, const std::vector<std::set<std::string>>& aNets,
                                              std::vector<std::set<std::string>>* aInconsistent )
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

                pinNet[terminalToken( std::string( ref.ToUTF8() ), std::string( pin->GetNumber().ToUTF8() ) )] = netId;
            }
        }

        ++netId;
    }

    struct CHECKED_NET
    {
        const std::set<std::string>* terminals;
        std::set<int>                ids;
    };

    std::vector<CHECKED_NET> checkedNets;
    std::map<int, int>       sourceNetsPerImportedNet;

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
            if( ids.size() == 1 )
                sourceNetsPerImportedNet[*ids.begin()]++;

            checkedNets.push_back( { &net, std::move( ids ) } );
        }
    }

    int consistent = 0;

    for( const CHECKED_NET& net : checkedNets )
    {
        bool exact = net.ids.size() == 1 && sourceNetsPerImportedNet[*net.ids.begin()] == 1;

        if( exact )
            ++consistent;
        else if( aInconsistent )
            aInconsistent->push_back( *net.terminals );
    }

    return { consistent, static_cast<int>( checkedNets.size() ) };
}


static wxString terminalNetName( SCHEMATIC& aSchematic, const wxString& aReference, const wxString& aPinNumber )
{
    SCH_SHEET_LIST sheets = aSchematic.BuildSheetListSortedByPageNumbers();
    aSchematic.ConnectionGraph()->Recalculate( sheets, true );

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

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == aReference
                    && pin->GetNumber() == aPinNumber )
                {
                    return key.Name;
                }
            }
        }
    }

    return {};
}


/// Ground-truth companion beside .DSN, .NET preferred over .BOM.
static std::set<std::string> expectedRefsFor( const std::filesystem::path& aDsn, std::string& aSource )
{
    for( const char* ext : { ".NET", ".net", ".BOM", ".bom" } )
    {
        std::filesystem::path candidate = aDsn;
        candidate.replace_extension( ext );

        if( std::filesystem::exists( candidate ) )
        {
            aSource = candidate.filename().string();

            bool isNet = std::string( ext ) == ".NET" || std::string( ext ) == ".net";
            return isNet ? parseNetComs( candidate.string() ) : parseBomRefs( candidate.string() );
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

    for( auto it = fs::recursive_directory_iterator( root, fs::directory_options::skip_permission_denied );
         it != fs::recursive_directory_iterator(); ++it )
    {
        if( !it->is_regular_file() )
            continue;

        std::string ext = it->path().extension().string();
        std::transform( ext.begin(), ext.end(), ext.begin(),
                        []( unsigned char c )
                        {
                            return std::tolower( c );
                        } );

        if( ext == ".dsn" )
            designs.push_back( it->path() );
    }

    std::sort( designs.begin(), designs.end() );

    BOOST_TEST_MESSAGE( "OrCAD corpus: " << designs.size() << " .DSN files under " << root );

    int          imported = 0, crashed = 0, unsupported = 0, rejected = 0, checked = 0;
    unsigned int totalExpected = 0, totalMatched = 0, totalMissing = 0, totalExtra = 0;
    int          netConsistent = 0, netCheckable = 0, netTotal = 0;
    uint64_t     importedPages = 0, importedComponents = 0, importedPowerSymbols = 0;
    uint64_t     importedPins = 0, importedWires = 0, importedBuses = 0;
    uint64_t     importedLabels = 0, importedShapes = 0, importedTexts = 0, importedBitmaps = 0;

    const char* debugEnv = std::getenv( "KICAD_ORCAD_DEBUG" );
    std::string debugFilter = debugEnv ? debugEnv : "";
    const char* filterEnv = std::getenv( "KICAD_ORCAD_FILTER" );
    std::string designFilter = filterEnv ? filterEnv : "";

    for( const fs::path& dsn : designs )
    {
        std::string rel = fs::relative( dsn, root ).string();

        if( !designFilter.empty() && rel.find( designFilter ) == std::string::npos )
            continue;

        BOOST_TEST_INFO_SCOPE( "OrCAD source: " << rel );

        SCH_IO_ORCAD plugin;
        uint64_t     perDesignPages = 0, perDesignComponents = 0, perDesignPowerSymbols = 0;
        uint64_t     perDesignPins = 0, perDesignWires = 0, perDesignBuses = 0;
        uint64_t     perDesignLabels = 0, perDesignShapes = 0, perDesignTexts = 0;
        uint64_t     perDesignBitmaps = 0;
        uint64_t     perDesignRedWires = 0;
        uint64_t     perDesignVddmPowerSymbols = 0;

        bool debug = !debugFilter.empty() && rel.find( debugFilter ) != std::string::npos;

        if( !plugin.CanReadSchematicFile( dsn.string() ) )
        {
            ++rejected;
            continue;
        }

        SETTINGS_MANAGER           manager;
        std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
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

        for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
        {
            SCH_SCREEN* screen = path.LastScreen();

            if( !screen )
                continue;

            ++importedPages;
            ++perDesignPages;

            for( SCH_ITEM* item : screen->Items() )
            {
                switch( item->Type() )
                {
                case SCH_SYMBOL_T:
                {
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                    wxString    ref = symbol->GetRef( &path, false );

                    if( ref.StartsWith( wxS( "#" ) ) )
                    {
                        ++importedPowerSymbols;
                        ++perDesignPowerSymbols;

                        if( symbol->GetValue( &path, RAW_VALUE ) == wxS( "VDDM" ) )
                            ++perDesignVddmPowerSymbols;
                    }
                    else
                    {
                        ++importedComponents;
                        ++perDesignComponents;
                    }

                    size_t pinCount = symbol->GetPins( &path ).size();
                    importedPins += pinCount;
                    perDesignPins += pinCount;
                    break;
                }

                case SCH_LINE_T:
                {
                    SCH_LINE* line = static_cast<SCH_LINE*>( item );

                    if( line->GetLayer() == LAYER_BUS )
                    {
                        ++importedBuses;
                        ++perDesignBuses;
                    }
                    else if( line->GetLayer() == LAYER_WIRE )
                    {
                        ++importedWires;
                        ++perDesignWires;

                        if( line->GetLineColor() == OrcadColor( 8 ) )
                            ++perDesignRedWires;
                    }

                    break;
                }

                case SCH_LABEL_T:
                case SCH_GLOBAL_LABEL_T:
                case SCH_HIER_LABEL_T:
                    ++importedLabels;
                    ++perDesignLabels;
                    break;

                case SCH_SHAPE_T:
                    ++importedShapes;
                    ++perDesignShapes;
                    break;

                case SCH_TEXT_T:
                    ++importedTexts;
                    ++perDesignTexts;
                    break;

                case SCH_BITMAP_T:
                    ++importedBitmaps;
                    ++perDesignBitmaps;
                    break;

                default: break;
                }
            }
        }

        BOOST_TEST_MESSAGE( "  AUDIT   " << rel << "|pages=" << perDesignPages << "|components=" << perDesignComponents
                                         << "|power=" << perDesignPowerSymbols << "|pins=" << perDesignPins
                                         << "|wires=" << perDesignWires << "|buses=" << perDesignBuses
                                         << "|labels=" << perDesignLabels << "|shapes=" << perDesignShapes
                                         << "|texts=" << perDesignTexts << "|bitmaps=" << perDesignBitmaps );

        if( rel == "allegro/beagleboard-xm/SCH/BeagleBoard-xM_ORCAD.DSN" )
        {
            BOOST_CHECK_EQUAL( perDesignBitmaps, 10u );
            BOOST_CHECK_EQUAL( perDesignShapes, 108u );
            BOOST_CHECK_EQUAL( perDesignTexts, 175u );
        }

        if( rel
            == "allegro/OpenCellular-LED/Rev-C/schematic/"
               "OpenCellular_Connect-1_LED_Life-3_Schematic.DSN" )
        {
            BOOST_CHECK_EQUAL( perDesignVddmPowerSymbols, 31u );
        }

        if( rel
            == "orcad/OpenCellular-GBC-Elgon_ARM/Rev-A/schematics/"
               "CN81XX_GBCV2_sch_0530.DSN" )
        {
            BOOST_CHECK_EQUAL( perDesignRedWires, 35u );
        }

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

        BOOST_TEST_MESSAGE( "  CHECK   " << rel << " : " << matched << "/" << expected.size() << " refs ("
                                         << int( 100.0 * matched / expected.size() ) << "%), extra " << extra.size()
                                         << " [" << source << "]" );

        if( debug )
        {
            for( const std::string& ref : missing )
                BOOST_TEST_MESSAGE( "          missing ref: " << ref );

            for( const std::string& ref : extra )
                BOOST_TEST_MESSAGE( "          extra ref: " << ref );
        }

        // .NET ground truth carries terminal connectivity; verify pins group per net after rebuild.
        std::filesystem::path net = dsn;
        net.replace_extension( source.size() >= 4 && source.substr( source.size() - 4 ) == ".net" ? ".net" : ".NET" );

        if( std::filesystem::exists( net ) )
        {
            std::vector<std::set<std::string>> nets = parseNetTerminals( net.string() );

            if( !nets.empty() )
            {
                std::vector<std::set<std::string>> inconsistent;
                auto [consistent, checkableNets] = checkConnectivity( *schematic, nets, &inconsistent );
                netConsistent += consistent;
                netCheckable += checkableNets;
                netTotal += static_cast<int>( nets.size() );

                BOOST_TEST_MESSAGE( "          connectivity: " << consistent << "/" << checkableNets
                                                               << " nets consistent" );

                if( debug )
                {
                    for( const std::set<std::string>& terminals : inconsistent )
                    {
                        std::string joined;

                        for( const std::string& terminal : terminals )
                        {
                            if( !joined.empty() )
                                joined += ", ";

                            joined += terminal;
                        }

                        BOOST_TEST_MESSAGE( "          inconsistent net: " << joined );
                    }
                }
            }
        }
    }

    BOOST_TEST_MESSAGE( "==== OrCAD corpus summary ====" );
    BOOST_TEST_MESSAGE( "  designs: " << designs.size() << "  imported: " << imported << "  crashed: " << crashed
                                      << "  unsupported: " << unsupported << "  rejected: " << rejected );
    BOOST_TEST_MESSAGE( "  objects: pages "
                        << importedPages << "  components " << importedComponents << "  power " << importedPowerSymbols
                        << "  pins " << importedPins << "  wires " << importedWires << "  buses " << importedBuses
                        << "  labels " << importedLabels << "  shapes " << importedShapes << "  texts " << importedTexts
                        << "  bitmaps " << importedBitmaps );

    if( checked )
    {
        BOOST_TEST_MESSAGE( "  refdes coverage: " << totalMatched << "/" << totalExpected << " ("
                                                  << int( 100.0 * totalMatched / totalExpected )
                                                  << "%)  missing: " << totalMissing << "  extra: " << totalExtra );
    }

    if( netCheckable )
    {
        BOOST_TEST_MESSAGE( "  net connectivity: " << netConsistent << "/" << netCheckable << " ("
                                                   << int( 100.0 * netConsistent / netCheckable ) << "%)" );
    }

    // Only pre-2003 format may throw; anything else is a crash
    BOOST_CHECK_MESSAGE( crashed == 0, crashed << " design(s) crashed during import." );

    const char* snapshotEnv = std::getenv( "KICAD_ORCAD_CORPUS_SNAPSHOT" );

    // Guard against vacuous pass when no companion files present.
    if( designFilter.empty() )
        BOOST_REQUIRE_MESSAGE( checked > 0, "No ground-truth .BOM/.NET companions were validated." );

    if( designFilter.empty() && snapshotEnv && *snapshotEnv )
    {
        BOOST_CHECK_EQUAL( imported, 92 );
        BOOST_CHECK_EQUAL( rejected, 1 );
        BOOST_CHECK_EQUAL( importedPages, 854u );
        BOOST_CHECK_EQUAL( importedBitmaps, 616u );
    }

    // Occurrence-annotation decode holds this above 95%; dropped Hierarchy-stream ref overlay collapses it.
    if( checked )
        BOOST_CHECK_GE( 100.0 * totalMatched / totalExpected, 95.0 );

    // Pin-placement/geometry regression breaking connectivity collapses this. Checkable floor
    // (>= 2 resolvable terminals per net) stops broad pin loss passing vacuously.
    if( netTotal )
    {
        BOOST_CHECK_GE( 100.0 * netCheckable / netTotal, 80.0 );
        BOOST_CHECK_EQUAL( netConsistent, netCheckable );
    }
}


static std::filesystem::path findCorpusDesign( const std::filesystem::path& aRoot, const std::string& aFileName )
{
    for( const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator( aRoot ) )
    {
        if( entry.is_regular_file() && entry.path().filename() == aFileName )
            return entry.path();
    }

    return {};
}


BOOST_AUTO_TEST_CASE( CisVariantFallbackUsesBytewiseFirstBomName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn =
            std::filesystem::path( corpusEnv ) / "PADS" / "skyworks-eval" / "SI34061FB12V4KIT"
            / "si34061-evb-ext_1.7.1.20220111" / "schematic" / "SI34061-EVB-EXT.DSN";

    if( !std::filesystem::exists( dsn ) )
    {
        BOOST_TEST_MESSAGE( "SI34061-EVB-EXT 12 V design not present; skipping CIS variant check." );
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

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            symbols.emplace( symbol->GetRef( &path, false ), symbol );
        }
    }

    BOOST_REQUIRE( symbols.count( wxS( "U1" ) ) );
    BOOST_REQUIRE( symbols.count( wxS( "D13" ) ) );
    BOOST_REQUIRE( symbols.count( wxS( "T2" ) ) );
    BOOST_REQUIRE( symbols.count( wxS( "LB1" ) ) );
    BOOST_CHECK_EQUAL( symbols[wxS( "U1" )]->GetField( FIELD_T::VALUE )->GetText(),
                       wxS( "NVMFS5C680NLT1G" ) );
    BOOST_CHECK_EQUAL( symbols[wxS( "D13" )]->GetField( FIELD_T::VALUE )->GetText(), wxS( "PDS5100" ) );
    BOOST_CHECK_EQUAL( symbols[wxS( "T2" )]->GetField( FIELD_T::VALUE )->GetText(), wxS( "LDT1026-50R" ) );
    BOOST_CHECK_EQUAL( symbols[wxS( "LB1" )]->GetField( FIELD_T::VALUE )->GetText(),
                       wxS( "LABEL-Si34061-EVB-EXT-BOM-R1.7-12V" ) );
    BOOST_REQUIRE( symbols[wxS( "U1" )]->GetField( wxS( "Voltage" ) ) );
    BOOST_CHECK_EQUAL( symbols[wxS( "U1" )]->GetField( wxS( "Voltage" ) )->GetText(), wxS( "60V" ) );
    BOOST_REQUIRE( symbols.count( wxS( "D15" ) ) );
    BOOST_REQUIRE( symbols.count( wxS( "R35" ) ) );
    BOOST_REQUIRE( symbols.count( wxS( "TP1" ) ) );
    BOOST_CHECK( symbols[wxS( "D15" )]->GetDNP() );
    BOOST_CHECK( symbols[wxS( "R35" )]->GetDNP() );
    BOOST_CHECK( symbols[wxS( "TP1" )]->GetDNP() );
    BOOST_CHECK_EQUAL( symbols[wxS( "D15" )]->GetField( FIELD_T::VALUE )->GetText(), wxS( "NI" ) );
    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        bool variantNameFound = false;

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_TEXT_T ) )
        {
            const wxString& text = static_cast<SCH_TEXT*>( item )->GetText();
            BOOST_CHECK_NE( text, wxS( "<Core Design>" ) );
            variantNameFound |= text == wxS( "12V" );
        }

        BOOST_CHECK( variantNameFound );
        BOOST_CHECK_EQUAL( path.LastScreen()->GetTitleBlock().GetComment( 1 ), wxS( "Variant Name: 12V" ) );
    }
}


BOOST_AUTO_TEST_CASE( UnreferencedSchematicFoldersRemainVisibleButExcludedFromBoard )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "PROJET INDUS.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    size_t         excluded = 0;
    size_t         excludedSymbols = 0;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        excluded += path.GetExcludedFromBoard();

        if( !path.GetExcludedFromBoard() )
            continue;

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            ++excludedSymbols;
            BOOST_CHECK( static_cast<SCH_SYMBOL*>( item )->GetExcludedFromBoard() );
        }
    }

    BOOST_CHECK_EQUAL( sheets.size(), 5u );
    BOOST_CHECK_EQUAL( excluded, 4u );
    BOOST_CHECK_GT( excludedSymbols, 0u );
}


BOOST_AUTO_TEST_CASE( ViewsDirectoryIgnoresStaleStoredFolders )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC1987A-2.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    BOOST_REQUIRE_EQUAL( sheets.size(), 1u );
}


BOOST_AUTO_TEST_CASE( ShortFramedPackagePinsKeepTheirSlots )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "1822A.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SYMBOL* connector = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        if( !connector )
            connector = findConvertedSymbol( *path.LastScreen(), path, wxS( "J7" ) );
    }

    // Without pin slots the default definition misses an empty slot and a duplicate variant wins
    BOOST_REQUIRE( connector );
    BOOST_CHECK_EQUAL( connector->GetLibId().GetLibItemName().wx_str(), wxS( "Connex_112404_gnd2_0_pins2" ) );
}


BOOST_AUTO_TEST_CASE( OccurrenceFlatNetConnectsAcrossPagesWithoutOffpageSymbols )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "M5275EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "J3", "38" ), terminalToken( "RP49", "7" ),
                                                terminalToken( "U6", "D10" ) } } );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


static std::map<std::string, std::vector<std::string>> collectImportedUuids( const SCHEMATIC& aSchematic )
{
    std::map<std::string, std::vector<std::string>> uuids;

    auto itemIdentity = []( const SCH_ITEM& item, const SCH_SHEET_PATH& path )
    {
        std::ostringstream identity;
        const VECTOR2I position = item.GetPosition();
        const BOX2I bounds = item.GetBoundingBox();
        identity << item.Type() << ':' << item.GetLayer() << ':' << position.x << ',' << position.y
                 << ':' << bounds.GetX() << ',' << bounds.GetY() << ',' << bounds.GetWidth() << ','
                 << bounds.GetHeight();

        auto text = [&]( const wxString& value )
        {
            const std::string utf8 = value.ToStdString( wxConvUTF8 );
            identity << ':' << utf8.size() << ':' << utf8;
        };

        if( const auto* line = dynamic_cast<const SCH_LINE*>( &item ) )
        {
            identity << ':' << line->GetEndPoint().x << ',' << line->GetEndPoint().y;
        }

        if( const auto* label = dynamic_cast<const EDA_TEXT*>( &item ) )
        {
            text( label->GetText() );
            identity << ':' << label->GetTextAngleDegrees() << ':' << label->GetTextWidth() << ','
                     << label->GetTextHeight();
        }

        if( const auto* shape = dynamic_cast<const SCH_SHAPE*>( &item ) )
        {
            identity << ':' << static_cast<int>( shape->GetShape() ) << ':' << shape->GetStart().x
                     << ',' << shape->GetStart().y << ':' << shape->GetEnd().x << ',' << shape->GetEnd().y;

            if( shape->GetShape() == SHAPE_T::POLY )
                identity << ':' << shape->GetPolyShape().Format();
            else if( shape->GetShape() == SHAPE_T::ARC )
                identity << ':' << shape->GetArcMid().x << ',' << shape->GetArcMid().y;
            else if( shape->GetShape() == SHAPE_T::BEZIER )
                identity << ':' << shape->GetBezierC1().x << ',' << shape->GetBezierC1().y << ':'
                         << shape->GetBezierC2().x << ',' << shape->GetBezierC2().y;
        }

        if( const auto* symbol = dynamic_cast<const SCH_SYMBOL*>( &item ) )
        {
            text( symbol->GetRef( &path ) );
            text( symbol->GetLibId().Format() );
            identity << ':' << symbol->GetUnitSelection( &path ) << ':' << symbol->GetOrientation();
        }

        if( const auto* pin = dynamic_cast<const SCH_PIN*>( &item ) )
        {
            text( pin->GetNumber() );
            text( pin->GetName() );
            identity << ':' << pin->GetUnit() << ':' << pin->GetBodyStyle() << ':'
                     << static_cast<int>( pin->GetOrientation() ) << ':' << pin->GetLength();
        }

        if( const auto* sheet = dynamic_cast<const SCH_SHEET*>( &item ) )
        {
            text( sheet->GetName() );
            text( sheet->GetFileName() );
        }

        return identity.str();
    };

    for( const SCH_SHEET_PATH& path : aSchematic.Hierarchy() )
    {
        std::string scope;

        for( size_t index = 0; index < path.size(); ++index )
        {
            SCH_SHEET* sheet = path.at( index );
            const std::string name = sheet->GetName().ToStdString( wxConvUTF8 );
            scope += std::to_string( name.size() ) + ':' + name + '/';
        }

        SCH_SHEET* sheet = path.Last();
        uuids[scope + "sheet"].push_back( sheet->m_Uuid.AsStdString() );
        SCH_SCREEN* screen = path.LastScreen();
        BOOST_REQUIRE( screen );
        uuids[scope + "screen"].push_back( screen->GetUuid().AsStdString() );

        for( SCH_ITEM* item : screen->Items() )
        {
            const std::string identity = scope + itemIdentity( *item, path );
            uuids[identity].push_back( item->m_Uuid.AsStdString() );

            if( auto* symbol = dynamic_cast<SCH_SYMBOL*>( item ) )
            {
                for( const std::unique_ptr<SCH_PIN>& pin : symbol->GetRawPins() )
                    uuids[identity + "/pin/" + itemIdentity( *pin, path )].push_back( pin->m_Uuid.AsStdString() );
            }
            else if( const auto* child = dynamic_cast<const SCH_SHEET*>( item ) )
            {
                for( const SCH_SHEET_PIN* pin : child->GetPins() )
                    uuids[identity + "/pin/" + itemIdentity( *pin, path )].push_back( pin->m_Uuid.AsStdString() );
            }
        }
    }

    // Identical overlapping objects are interchangeable, but UUIDs must stay attached to their geometry and owner.
    for( auto& [identity, values] : uuids )
        std::sort( values.begin(), values.end() );

    return uuids;
}

BOOST_AUTO_TEST_CASE( RepeatedImportHasDeterministicUuids )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD determinism check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "mc33163.dsn" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "mc33163.dsn not present in corpus; skipping OrCAD determinism check." );
        return;
    }

    auto importUuids = [&]( const std::filesystem::path& aDsn )
    {
        std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
        SETTINGS_MANAGER           manager;
        manager.LoadProject( "" );
        schematic->SetProject( &manager.Prj() );
        schematic->CurrentSheet().clear();
        schematic->CurrentSheet().push_back( &schematic->Root() );

        SCH_IO_ORCAD plugin;
        plugin.LoadSchematicFile( aDsn.string(), schematic.get() );
        return collectImportedUuids( *schematic );
    };

    const auto first = importUuids( dsn );
    const auto second = importUuids( dsn );

    BOOST_REQUIRE( !first.empty() );
    BOOST_REQUIRE_EQUAL( first.size(), second.size() );

    for( const auto& [identity, values] : first )
    {
        BOOST_TEST_CONTEXT( identity )
        {
            const auto found = second.find( identity );
            BOOST_REQUIRE( found != second.end() );
            BOOST_CHECK_EQUAL_COLLECTIONS( values.begin(), values.end(), found->second.begin(), found->second.end() );
        }
    }
}


BOOST_AUTO_TEST_CASE( SheetLoadKeepsExistingItemUuids )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD sheet-load UUID check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "mc33163.dsn" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "mc33163.dsn not present in corpus; skipping OrCAD sheet-load UUID check." );
        return;
    }

    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, wxS( "netlists/complex_hierarchy_shared/complex_hierarchy" ), schematic );

    auto existingUuids = [&]()
    {
        std::vector<std::string> uuids;

        for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
        {
            for( SCH_ITEM* item : path.LastScreen()->Items() )
                uuids.push_back( item->m_Uuid.AsStdString() );
        }

        std::sort( uuids.begin(), uuids.end() );
        return uuids;
    };

    const std::vector<std::string> before = existingUuids();
    BOOST_REQUIRE( !before.empty() );

    // SCH_EDIT_FRAME::LoadSheetFromFile loads into a sheet that is not yet in the hierarchy
    SCH_SHEET sheet( schematic.get() );
    sheet.SetFileName( dsn.string() );
    m_plugin.LoadSchematicFile( dsn.string(), schematic.get(), &sheet );

    BOOST_REQUIRE( sheet.GetScreen() && !sheet.GetScreen()->Items().empty() );

    const std::vector<std::string> after = existingUuids();
    BOOST_CHECK_EQUAL_COLLECTIONS( before.begin(), before.end(), after.begin(), after.end() );
}


BOOST_AUTO_TEST_CASE( PowerSymbolAttachesAtPlacedVariantPin )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD graphic variant pin check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "OC_CONNECT1_SDR_REV_C_V1P1.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "OC_CONNECT1_SDR_REV_C_V1P1.DSN not present in corpus; skipping variant pin check." );
        return;
    }

    m_plugin.LoadSchematicFile( dsn.string(), m_schematic.get() );

    // Only the later cache variant of this GND symbol puts its pin on the free end of its stub wire
    const VECTOR2I stubEnd( schIUScale.mmToIU( 34.29 ), schIUScale.mmToIU( 133.35 ) );
    bool           attached = false;

    for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
    {
        if( !path.LastScreen()->GetFileName().Contains( wxS( "FPGA_1" ) ) )
            continue;

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( !symbol->IsPower() )
                continue;

            for( const SCH_PIN* pin : symbol->GetPins( &path ) )
                attached |= pin->GetPosition() == stubEnd;
        }
    }

    BOOST_CHECK( attached );
}


BOOST_AUTO_TEST_CASE( AutoGeneratedNetNamesAreMappedWithoutNamingLabels )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping generated net-name check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CutiePi_V2.3-20210409.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "CutiePi_V2.3-20210409.DSN not present in corpus; skipping generated "
                            "net-name check." );
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

    const IMPORT_NET_MAP* map = schematic->GetImportNetMap();
    BOOST_REQUIRE( map );

    for( const auto& [sourceName, reference, number] :
         { std::tuple{ wxString( "N12720539" ), wxString( "D10" ), wxString( "A" ) },
           std::tuple{ wxString( "N132252170" ), wxString( "D23" ), wxString( "1" ) } } )
    {
        wxString netName = terminalNetName( *schematic, reference, number );
        BOOST_CHECK_NE( netName, sourceName );
        bool found = false;

        for( const IMPORT_NET_MAP_ENTRY& entry : map->entries )
        {
            if( entry.originalName == sourceName )
            {
                BOOST_CHECK_EQUAL( entry.status, IMPORT_NET_STATUS::RESOLVED );
                BOOST_CHECK_EQUAL( entry.nameAtImport, netName );
                BOOST_CHECK( !entry.terminals.empty() );
                found = true;
            }
        }

        BOOST_CHECK( found );

        for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
        {
            for( SCH_ITEM* item : sheet.LastScreen()->Items() )
            {
                if( auto* label = dynamic_cast<SCH_LABEL_BASE*>( item ) )
                {
                    BOOST_CHECK_NE( label->GetText(), sourceName );
                    BOOST_CHECK( label->GetTextColor() == KIGFX::COLOR4D::UNSPECIFIED
                                 || label->GetTextColor().a > 0 );
                }
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( CaptureNetIdsAndBlankUnitLettersAreAuthoritative )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping Capture net/unit regression check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "BEAGLEBONEBLK_C3.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "BEAGLEBONEBLK_C3.DSN not present in corpus; skipping Capture net/unit "
                            "regression check." );
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

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::tuple<std::string, std::string, std::string>, int> terminalNets;
    int                                                              netId = 0;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            std::string page = subgraph->GetSheet().LastScreen()->GetFileName().ToStdString();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                {
                    terminalNets[{ page, symbol->GetRef( &subgraph->GetSheet(), false ).ToStdString(),
                                   pin->GetNumber().ToStdString() }] = netId;
                }
            }
        }

        ++netId;
    }

    auto findNet = [&]( const std::string& aPage, const std::string& aRef, const std::string& aPin )
    {
        for( const auto& [terminal, id] : terminalNets )
        {
            if( std::get<0>( terminal ).find( aPage ) != std::string::npos && std::get<1>( terminal ) == aRef
                && std::get<2>( terminal ) == aPin )
            {
                return id;
            }
        }

        return -1;
    };

    int ground = findNet( "AM335x 2_3, USB", "J1", "1" );
    int rx = findNet( "AM335x 2_3, USB", "J1", "4" );
    int tx = findNet( "AM335x 2_3, USB", "J1", "5" );

    BOOST_REQUIRE_NE( ground, -1 );
    BOOST_REQUIRE_NE( rx, -1 );
    BOOST_REQUIRE_NE( tx, -1 );
    BOOST_CHECK_NE( ground, rx );
    BOOST_CHECK_NE( ground, tx );
    BOOST_CHECK_NE( rx, tx );

    BOOST_CHECK_EQUAL( findNet( "LED, Config", "D3", "1" ), findNet( "LED, Config", "Q1", "3" ) );
    BOOST_CHECK_EQUAL( findNet( "LED, Config", "R77", "1" ), findNet( "LED, Config", "Q1", "5" ) );

    std::set<int> q1Units;
    std::set<wxString> q1ShownReferences;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.LastScreen()->GetFileName().Find( wxS( "LED, Config" ) ) == wxNOT_FOUND )
            continue;

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "Q1" ) )
            {
                q1Units.insert( symbol->GetUnit() );
                q1ShownReferences.insert( symbol->GetRef( &path, true ) );
            }
        }
    }

    const std::set<int> expectedUnits = { 1, 2 };
    BOOST_CHECK_EQUAL_COLLECTIONS( q1Units.begin(), q1Units.end(), expectedUnits.begin(), expectedUnits.end() );

    const std::set<wxString> expectedShownReferences = { wxS( "Q1" ) };
    BOOST_CHECK_EQUAL_COLLECTIONS( q1ShownReferences.begin(), q1ShownReferences.end(),
                                   expectedShownReferences.begin(), expectedShownReferences.end() );
}


BOOST_AUTO_TEST_CASE( BusMembersWithoutLocalWiresRetainNetMapReferences )
{
    const char* corpus = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpus || !*corpus )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpus, "SCH-20380.DSN" );

    if( dsn.empty() )
        return;

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    SCHEMATIC schematic( &manager.Prj() );
    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), &schematic );
    schematic.RefreshHierarchy();
    const IMPORT_NET_MAP* map = schematic.GetImportNetMap();
    BOOST_REQUIRE( map );
    bool memberFound = false;
    bool bundleFound = false;
    wxString actualName = terminalNetName( schematic, wxS( "J10" ), wxS( "25" ) );
    BOOST_REQUIRE( !actualName.IsEmpty() );

    for( const IMPORT_NET_MAP_ENTRY& entry : map->entries )
    {
        if( entry.view != wxS( "Hierarchical Interconnects" ) )
            continue;

        if( entry.originalName == wxS( "/IRQ[7:1]" ) )
        {
            bundleFound = true;
            BOOST_CHECK_EQUAL( entry.status, IMPORT_NET_STATUS::BUS );
            BOOST_CHECK( !entry.nameAtImport.IsEmpty() );
            BOOST_CHECK( !entry.terminals.empty() );
        }

        if( entry.originalName != wxS( "/IRQ7" ) )
            continue;

        memberFound = true;
        BOOST_CHECK_EQUAL( entry.status, IMPORT_NET_STATUS::RESOLVED );
        BOOST_CHECK_EQUAL( entry.nameAtImport, actualName );
        BOOST_CHECK( !entry.itemUuids.empty() );
        std::set<std::string> mappedTerminals;

        for( const IMPORT_NET_TERMINAL& terminal : entry.terminals )
        {
            SCH_SHEET_PATH path;
            auto* symbol = dynamic_cast<SCH_SYMBOL*>( schematic.ResolveItem( terminal.symbolUuid, &path ) );
            BOOST_REQUIRE( symbol );
            mappedTerminals.insert( terminalToken( symbol->GetRef( &path, false ).ToStdString(),
                                                   terminal.pinNumber.ToStdString() ) );
        }

        const std::set<std::string> expected = { "J10.25", "R77.2", "RP16.7", "U10.R8" };
        BOOST_CHECK( mappedTerminals == expected );
    }

    BOOST_CHECK( memberFound );
    BOOST_CHECK( bundleFound );
}


BOOST_AUTO_TEST_CASE( NetMapIncludesPinsConnectedByImportJunctionCleanup )
{
    const char* corpus = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpus || !*corpus )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpus, "DC1096B-1.DSN" );

    if( dsn.empty() )
        return;

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    SCHEMATIC schematic( &manager.Prj() );
    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), &schematic );
    const IMPORT_NET_MAP* map = schematic.GetImportNetMap();
    BOOST_REQUIRE( map );
    bool found = false;

    for( const IMPORT_NET_MAP_ENTRY& entry : map->entries )
    {
        if( entry.sourceNetId == 16903175 && entry.originalName == wxS( "GND" ) )
        {
            found = true;
            BOOST_CHECK_EQUAL( entry.status, IMPORT_NET_STATUS::RESOLVED );
            BOOST_CHECK_EQUAL( entry.nameAtImport, wxString( "GND" ) );
        }
    }

    BOOST_CHECK( found );

    for( const wxString& reference : { wxString( "U1" ), wxString( "U2" ), wxString( "U4" ) } )
        BOOST_CHECK_EQUAL( terminalNetName( schematic, reference, wxS( "11" ) ), wxString( "GND" ) );
}


BOOST_AUTO_TEST_CASE( HierarchicalBusMembersMapToPropagatedScalarNets )
{
    const char* corpus = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpus || !*corpus )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpus, "meta_carrier_sch_rev1.dsn" );

    if( dsn.empty() )
        return;

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    SCHEMATIC schematic( &manager.Prj() );
    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), &schematic );
    schematic.RefreshHierarchy();
    const IMPORT_NET_MAP* map = schematic.GetImportNetMap();
    BOOST_REQUIRE( map );
    bool found = false;

    for( const IMPORT_NET_MAP_ENTRY& entry : map->entries )
    {
        if( entry.sourceNetId != 9438909 || entry.originalName != wxS( "OUT_P6" )
            || entry.occurrence != std::vector<wxString>( { "TOP", "9136400", "TILE_AD9523" } ) )
            continue;

        found = true;
        BOOST_CHECK_EQUAL( entry.status, IMPORT_NET_STATUS::RESOLVED );
        BOOST_CHECK_EQUAL( entry.nameAtImport, wxString( "CLK_P5" ) );
        BOOST_REQUIRE( !entry.terminals.empty() );

        for( const IMPORT_NET_TERMINAL& terminal : entry.terminals )
        {
            SCH_SHEET_PATH path;
            auto* symbol = dynamic_cast<SCH_SYMBOL*>( schematic.ResolveItem( terminal.symbolUuid, &path ) );
            BOOST_REQUIRE( symbol );
            BOOST_CHECK_EQUAL( terminalNetName( schematic, symbol->GetRef( &path, false ), terminal.pinNumber ),
                               entry.nameAtImport );
        }
    }

    BOOST_CHECK( found );
}


BOOST_AUTO_TEST_CASE( NamedWirelessPinUsesOccurrenceNetName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping named wireless-pin check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CURRENT_SENSOR.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "CURRENT_SENSOR.DSN not present in corpus; skipping named wireless-pin check." );
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

    wxString netName = terminalNetName( *schematic, wxS( "U1" ), wxS( "7" ) );
    BOOST_CHECK_EQUAL( netName.AfterLast( '/' ), wxString( "VCC" ) );
    const IMPORT_NET_MAP* map = schematic->GetImportNetMap();
    BOOST_REQUIRE( map );
    bool mapped = false;

    for( const IMPORT_NET_MAP_ENTRY& entry : map->entries )
    {
        if( entry.originalName == wxS( "VCC" ) )
        {
            BOOST_CHECK_EQUAL( entry.status, IMPORT_NET_STATUS::RESOLVED );
            BOOST_CHECK_EQUAL( entry.nameAtImport, netName );
            BOOST_CHECK( !entry.terminals.empty() );
            mapped = true;
        }
    }

    BOOST_CHECK( mapped );
}


BOOST_AUTO_TEST_CASE( ReservedDatasheetPropertyUsesStandardField )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "reComputer J202_V1.0.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_SYMBOL* resistor = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "R48" ) )
            {
                resistor = symbol;
                break;
            }
        }

        if( resistor )
            break;
    }

    BOOST_REQUIRE( resistor );
    size_t datasheetFields = std::count_if(
            resistor->GetFields().begin(), resistor->GetFields().end(),
            []( const SCH_FIELD& field ) { return field.GetName() == wxS( "Datasheet" ); } );
    BOOST_CHECK_EQUAL( datasheetFields, 1u );
    BOOST_CHECK_EQUAL(
            resistor->GetField( FIELD_T::DATASHEET )->GetText(),
            wxS( "Y:\\01_Cadence_Library\\05_Datasheet\\301010000_YAGEO_RC0402JR-070RL_Datasheet.pdf" ) );
}


BOOST_AUTO_TEST_CASE( ReservedFootprintPropertyUsesDistinctMetadataField )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY4532 Power Board Schematic.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_SYMBOL* capacitor = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "C45" ) )
            {
                capacitor = symbol;
                break;
            }
        }

        if( capacitor )
            break;
    }

    BOOST_REQUIRE( capacitor );
    size_t footprintFields = std::count_if(
            capacitor->GetFields().begin(), capacitor->GetFields().end(),
            []( const SCH_FIELD& field ) { return field.GetName() == wxS( "Footprint" ); } );
    BOOST_CHECK_EQUAL( footprintFields, 1u );
    BOOST_CHECK( capacitor->GetField( FIELD_T::FOOTPRINT )->GetText().IsEmpty() );

    SCH_FIELD* metadata = capacitor->GetField( wxS( "OrCAD Footprint Property" ) );
    BOOST_REQUIRE( metadata );
    BOOST_CHECK_EQUAL( metadata->GetText(), wxS( "0402" ) );
}


BOOST_AUTO_TEST_CASE( DisplayedPropertiesUseCaptureNameMatchingAndStandardFields )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY4532 Power Board Schematic.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_SYMBOL* capacitor = nullptr;
    SCH_SYMBOL* resistor = nullptr;
    SCH_SYMBOL* testPoint = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    reference = symbol->GetRef( &path, false );

            if( reference == wxS( "C104" ) )
                capacitor = symbol;
            else if( reference == wxS( "R58" ) )
                resistor = symbol;
            else if( reference == wxS( "TP8" ) )
                testPoint = symbol;
        }
    }

    BOOST_REQUIRE( capacitor );
    SCH_FIELD* voltage = capacitor->GetField( wxS( "Voltage" ) );
    BOOST_REQUIRE( voltage );
    BOOST_CHECK( voltage->IsVisible() );
    BOOST_CHECK( voltage->GetDrawRotation() == ANGLE_HORIZONTAL );
    BOOST_CHECK( voltage->GetPosition()
                 == OrcadDbuToIu( 722, 929 )
                            + VECTOR2I( 0, OrcadTextBaselineOffset( voltage->GetTextSize().y ) ) );

    BOOST_REQUIRE( resistor );
    BOOST_CHECK( resistor->GetField( FIELD_T::VALUE )->IsVisible() );

    BOOST_REQUIRE( testPoint );
    SCH_FIELD* description = testPoint->GetField( FIELD_T::DESCRIPTION );
    BOOST_REQUIRE( description );
    BOOST_CHECK( description->IsVisible() );
    BOOST_CHECK( description->GetDrawRotation() == ANGLE_HORIZONTAL );
    BOOST_CHECK( description->GetPosition()
                 == OrcadDbuToIu( 810, 945 )
                            + VECTOR2I( 0, OrcadTextBaselineOffset( description->GetTextSize().y ) ) );
}


BOOST_AUTO_TEST_CASE( DisplayedInheritedPartFieldsArePreserved )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC1859A-2.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_SYMBOL* capacitor = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "C2" ) )
            {
                capacitor = symbol;
                break;
            }
        }

        if( capacitor )
            break;
    }

    BOOST_REQUIRE( capacitor );
    SCH_FIELD* voltage = capacitor->GetField( wxS( "1st Part Field" ) );
    BOOST_REQUIRE( voltage );
    BOOST_CHECK_EQUAL( voltage->GetText(), wxS( "10V" ) );
    BOOST_CHECK( voltage->IsVisible() );
}


BOOST_AUTO_TEST_CASE( DisplayTypeTwoPropertiesRenderNamesAndValues )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-48278.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_FIELD* jumper = nullptr;
    bool       renderedJumper = false;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "J2" ) )
            {
                jumper = symbol->GetField( wxS( "JUMPER(DEFAULT)" ) );
                break;
            }
        }

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_TEXT_T ) )
        {
            SCH_TEXT* text = static_cast<SCH_TEXT*>( item );
            renderedJumper = renderedJumper
                             || text->GetText() == wxS( "JUMPER(DEFAULT) = OFF:POLARITY_SEL_L" );
        }

        if( jumper )
            break;
    }

    BOOST_REQUIRE( jumper );
    BOOST_CHECK_EQUAL( jumper->GetText(), wxS( "OFF:POLARITY_SEL_L" ) );
    BOOST_CHECK( renderedJumper );
}


BOOST_AUTO_TEST_CASE( GeneratedWirelessNetOverridesPeerPinName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping wireless peer-net check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY3280_MBR3 EVK Schematic.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "CY3280_MBR3 EVK Schematic.DSN not present; skipping wireless peer-net check." );
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

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    std::map<std::pair<wxString, wxString>, CONNECTION_SUBGRAPH*> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                    pinNets[{ symbol->GetRef( &subgraph->GetSheet(), false ), pin->GetNumber() }] = subgraph;
            }
        }
    }

    BOOST_REQUIRE_EQUAL( pinNets.count( { wxS( "R51" ), wxS( "1" ) } ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( { wxS( "R51" ), wxS( "2" ) } ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( { wxS( "R52" ), wxS( "1" ) } ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( { wxS( "R52" ), wxS( "2" ) } ), 1u );
    CONNECTION_SUBGRAPH* r51Pin1 = pinNets.at( { wxS( "R51" ), wxS( "1" ) } );
    CONNECTION_SUBGRAPH* r51Pin2 = pinNets.at( { wxS( "R51" ), wxS( "2" ) } );
    CONNECTION_SUBGRAPH* r52Pin1 = pinNets.at( { wxS( "R52" ), wxS( "1" ) } );
    CONNECTION_SUBGRAPH* r52Pin2 = pinNets.at( { wxS( "R52" ), wxS( "2" ) } );

    BOOST_CHECK_NE( r51Pin1, r51Pin2 );
    BOOST_CHECK_NE( r52Pin1, r52Pin2 );
}


BOOST_AUTO_TEST_CASE( GlobalNetNamesAreCaseInsensitive )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping global-net case check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "BeagleBoard-xM_ORCAD.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "BeagleBoard-xM_ORCAD.DSN not present in corpus; skipping global-net case check." );
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

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::tuple<std::string, std::string, std::string>, int> terminalNets;
    int                                                              netId = 0;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            std::string page = subgraph->GetSheet().LastScreen()->GetFileName().ToStdString();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                {
                    terminalNets[{ page, symbol->GetRef( &subgraph->GetSheet(), false ).ToStdString(),
                                   pin->GetNumber().ToStdString() }] = netId;
                }
            }
        }

        ++netId;
    }

    auto findNet = [&]( const std::string& aPage, const std::string& aRef, const std::string& aPin )
    {
        for( const auto& [terminal, id] : terminalNets )
        {
            if( std::get<0>( terminal ).find( aPage ) != std::string::npos && std::get<1>( terminal ) == aRef
                && std::get<2>( terminal ) == aPin )
            {
                return id;
            }
        }

        return -1;
    };

    int processor = findNet( "PROCESSOR_C", "C75", "1" );
    int power = findNet( "PMIC _POWER", "C122", "1" );

    BOOST_REQUIRE_NE( processor, -1 );
    BOOST_REQUIRE_NE( power, -1 );
    BOOST_CHECK_EQUAL( processor, power );
}


BOOST_AUTO_TEST_CASE( CaptureBusRangesUseKiCadSyntax )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping bus-range syntax check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "parallella_e16_z7020_schematic.dsn" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "parallella_e16_z7020_schematic.dsn not present in corpus; skipping bus-range check." );
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

    bool found = false;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            wxString text = static_cast<SCH_GLOBALLABEL*>( item )->GetText();
            BOOST_CHECK_NE( text, wxS( "DDR_DQ[31:0]" ) );
            found |= text == wxS( "DDR_DQ[31..0]" );
        }
    }

    BOOST_CHECK( found );
}


BOOST_AUTO_TEST_CASE( CanonicalPropertiesIgnoreCaseInsensitiveDuplicates )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "parallella_e16_z7020_schematic.dsn" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    SCH_SYMBOL* platedHole = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "PTH1" ) )
            {
                platedHole = symbol;
                break;
            }
        }

        if( platedHole )
            break;
    }

    BOOST_REQUIRE( platedHole );
    size_t valueFields = std::count_if(
            platedHole->GetFields().begin(), platedHole->GetFields().end(),
            []( const SCH_FIELD& aField ) { return aField.GetName().CmpNoCase( wxS( "Value" ) ) == 0; } );
    BOOST_CHECK_EQUAL( valueFields, 1u );
    BOOST_CHECK_EQUAL( platedHole->GetField( FIELD_T::VALUE )->GetShownText( FOR_CANVAS ), wxS( "PTH125_200PAD" ) );
}


BOOST_AUTO_TEST_CASE( CollidingOccurrenceAliasesRemainSeparate )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "parallella_e16_z7020_schematic.dsn" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::string, int> terminalNets;
    int                        netId = 0;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                {
                    terminalNets[terminalToken( symbol->GetRef( &subgraph->GetSheet(), false ).ToStdString(),
                                                 pin->GetNumber().ToStdString() )] = netId;
                }
            }
        }

        ++netId;
    }

    BOOST_REQUIRE( terminalNets.count( "R41.2" ) );
    BOOST_REQUIRE( terminalNets.count( "U26.53" ) );
    BOOST_REQUIRE( terminalNets.count( "R56.2" ) );
    BOOST_REQUIRE( terminalNets.count( "U11.1" ) );
    BOOST_REQUIRE( terminalNets.count( "U11.3" ) );
    BOOST_REQUIRE( terminalNets.count( "U14.1" ) );
    BOOST_CHECK_EQUAL( terminalNets["R41.2"], terminalNets["U26.53"] );
    BOOST_CHECK_EQUAL( terminalNets["R56.2"], terminalNets["U11.1"] );
    BOOST_CHECK_EQUAL( terminalNets["R56.2"], terminalNets["U11.3"] );
    BOOST_CHECK_EQUAL( terminalNets["R56.2"], terminalNets["U14.1"] );
    BOOST_CHECK_NE( terminalNets["R41.2"], terminalNets["R56.2"] );
}


BOOST_AUTO_TEST_CASE( CollidingOffpageOccurrenceAliasesRemainSeparate )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "parallella_gen0.dsn" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] = checkConnectivity(
            *schematic,
            { { terminalToken( "R41", "2" ), terminalToken( "U26", "53" ) },
              { terminalToken( "R57", "2" ), terminalToken( "R97", "1" ), terminalToken( "U14", "1" ) } } );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( FlatFolderOccurrenceAliasesConnectDisplacedPins )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-20380.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] = checkConnectivity(
            *schematic, { { terminalToken( "J10", "29" ), terminalToken( "RP15", "3" ), terminalToken( "U10", "R9" ),
                            terminalToken( "U33", "30" ) },
                          { terminalToken( "J10", "30" ), terminalToken( "JP62", "1" ), terminalToken( "RP16", "1" ),
                            terminalToken( "U10", "P9" ) },
                          { terminalToken( "J7", "50" ), terminalToken( "RP14", "5" ), terminalToken( "U10", "D12" ),
                            terminalToken( "U33", "21" ), terminalToken( "U5", "C" ) } } );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( PowerNetNameWinsOverSecondaryOccurrencePortAlias )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-38863_CX1.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    const IMPORT_NET_MAP* map = schematic->GetImportNetMap();
    BOOST_REQUIRE( map );
    std::set<wxString> netNames;

    for( const auto& [reference, peer, sourceName] :
         { std::tuple{ wxString( "J16" ), wxString( "C520" ), wxString( "GNDISOHU" ) },
           std::tuple{ wxString( "J17" ), wxString( "C590" ), wxString( "GNDISOHV" ) },
           std::tuple{ wxString( "J18" ), wxString( "C661" ), wxString( "GNDISOHW" ) } } )
    {
        wxString netName = terminalNetName( *schematic, reference, wxS( "1" ) );
        BOOST_CHECK_EQUAL( netName.AfterLast( '/' ).Upper(), sourceName );
        BOOST_CHECK_EQUAL( terminalNetName( *schematic, peer, wxS( "2" ) ), netName );
        netNames.insert( netName );
        bool mapped = false;

        for( const IMPORT_NET_MAP_ENTRY& entry : map->entries )
        {
            if( entry.originalName.CmpNoCase( sourceName ) == 0 )
            {
                BOOST_CHECK_EQUAL( entry.status, IMPORT_NET_STATUS::RESOLVED );
                BOOST_CHECK_EQUAL( entry.nameAtImport, netName );
                mapped = true;
            }
        }

        BOOST_CHECK( mapped );
    }

    BOOST_CHECK_EQUAL( netNames.size(), 3u );
}


BOOST_AUTO_TEST_CASE( DuplicatePageNetIdsPreserveAliases )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping duplicate page-net check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "MTB4.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "MTB4.DSN not present in corpus; skipping duplicate page-net check." );
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

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::string, wxString> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( subgraph->GetSheet().LastScreen()->GetFileName().Find( wxS( "PAGE 2 - MCU" ) ) == wxNOT_FOUND )
                continue;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "U1" ) )
                    pinNets[pin->GetNumber().ToStdString()] = key.Name;
            }
        }
    }

    BOOST_REQUIRE_EQUAL( pinNets.count( "F3" ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( "F8" ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( "G3" ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( "G7" ), 1u );
    BOOST_CHECK( pinNets["F3"].EndsWith( wxS( "I2C2_SDA" ) ) );
    BOOST_CHECK( pinNets["F8"].EndsWith( wxS( "I2C2_SDA" ) ) );
    BOOST_CHECK( pinNets["G3"].EndsWith( wxS( "I2C2_SCL" ) ) );
    BOOST_CHECK( pinNets["G7"].EndsWith( wxS( "I2C2_SCL" ) ) );
}


BOOST_AUTO_TEST_CASE( DuplicatePageNetIdsDoNotShortDistinctNets )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping duplicate page-net isolation check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2125A-2.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "DC2125A-2.DSN not present; skipping duplicate page-net isolation check." );
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

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    std::map<std::pair<wxString, wxString>, CONNECTION_SUBGRAPH*> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                    pinNets[{ symbol->GetRef( &subgraph->GetSheet(), false ), pin->GetNumber() }] = subgraph;
            }
        }
    }

    CONNECTION_SUBGRAPH* earth = pinNets.at( { wxS( "J2" ), wxS( "9" ) } );
    CONNECTION_SUBGRAPH* vportn = pinNets.at( { wxS( "C1" ), wxS( "2" ) } );
    BOOST_CHECK_NE( earth, vportn );
}


BOOST_AUTO_TEST_CASE( AliasAtCrossingDoesNotShortCaptureNets )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping alias-at-crossing isolation check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "OC_CONNECT1_FRONTEND_REV_C_V1P1.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "OC_CONNECT1_FRONTEND_REV_C_V1P1.DSN not present; skipping alias-at-crossing check." );
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

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    std::map<std::pair<wxString, wxString>, CONNECTION_SUBGRAPH*> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                    pinNets[{ symbol->GetRef( &subgraph->GetSheet(), false ), pin->GetNumber() }] = subgraph;
            }
        }
    }

    CONNECTION_SUBGRAPH* a0 = pinNets.at( { wxS( "U7157" ), wxS( "3" ) } );
    CONNECTION_SUBGRAPH* a2 = pinNets.at( { wxS( "U7157" ), wxS( "5" ) } );
    CONNECTION_SUBGRAPH* io1 = pinNets.at( { wxS( "U7157" ), wxS( "7" ) } );
    BOOST_CHECK_EQUAL( a0, pinNets.at( { wxS( "R1150" ), wxS( "2" ) } ) );
    BOOST_CHECK_EQUAL( a0, pinNets.at( { wxS( "R1153" ), wxS( "1" ) } ) );
    BOOST_CHECK_EQUAL( a2, pinNets.at( { wxS( "R1152" ), wxS( "2" ) } ) );
    BOOST_CHECK_EQUAL( a2, pinNets.at( { wxS( "R1155" ), wxS( "1" ) } ) );
    BOOST_CHECK_EQUAL( io1, pinNets.at( { wxS( "R954" ), wxS( "2" ) } ) );
    BOOST_CHECK_NE( a0, a2 );
    BOOST_CHECK_NE( a0, io1 );
    BOOST_CHECK_NE( a2, io1 );
}


BOOST_AUTO_TEST_CASE( WirelessPinUsesPageNetId )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping wireless page-net check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "Si828X-BW-GDB.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "Si828X-BW-GDB.DSN not present; skipping wireless page-net check." );
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

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    std::map<std::pair<wxString, wxString>, wxString> pinNames;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                    pinNames[{ symbol->GetRef( &subgraph->GetSheet(), false ), pin->GetNumber() }] = key.Name;
            }
        }
    }

    BOOST_CHECK_EQUAL( pinNames.at( { wxS( "CB28" ), wxS( "2" ) } ), wxS( "5V" ) );
    BOOST_CHECK_EQUAL( pinNames.at( { wxS( "RT17" ), wxS( "2" ) } ), wxS( "5V" ) );
}


BOOST_AUTO_TEST_CASE( AmbiguousWirelessPinUsesOccurrenceNetName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping ambiguous wireless-net check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2047A-3-A.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "DC2047A-3-A.DSN not present; skipping ambiguous wireless-net check." );
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

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    wxString d15Pin2;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "D15" )
                    && pin->GetNumber() == wxS( "2" ) )
                {
                    d15Pin2 = key.Name;
                }
            }
        }
    }

    BOOST_CHECK_EQUAL( d15Pin2, wxS( "AUX_RECTIFIED-" ) );
}


BOOST_AUTO_TEST_CASE( BlankPackagePinNumbersUseLogicalNames )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2084A-3.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, std::set<wxString>> pinNumbers;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            wxString reference = symbol->GetRef( &sheet, false );

            if( reference != wxS( "C31" ) && reference != wxS( "C42" ) )
                continue;

            for( const std::unique_ptr<SCH_PIN>& pin : symbol->GetRawPins() )
                pinNumbers[reference].insert( pin->GetNumber() );
        }
    }

    BOOST_CHECK( pinNumbers[wxS( "C31" )] == std::set<wxString>( { wxS( "1" ), wxS( "2" ) } ) );
    BOOST_CHECK( pinNumbers[wxS( "C42" )] == std::set<wxString>( { wxS( "1" ), wxS( "2" ) } ) );
}


BOOST_AUTO_TEST_CASE( FallbackPackagePrefersNumberedPins )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2091A-3.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, std::set<wxString>> pinNumbers;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    reference = symbol->GetRef( &sheet, false );

            if( reference != wxS( "C1" ) && reference != wxS( "C2" ) )
                continue;

            for( const std::unique_ptr<SCH_PIN>& pin : symbol->GetRawPins() )
                pinNumbers[reference].insert( pin->GetNumber() );
        }
    }

    const std::set<wxString> expected = { wxS( "1" ), wxS( "2" ) };
    BOOST_CHECK( pinNumbers[wxS( "C1" )] == expected );
    BOOST_CHECK( pinNumbers[wxS( "C2" )] == expected );
}


BOOST_AUTO_TEST_CASE( FallbackPackageUsesLogicalPinOrder )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2228A-3.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "R188", "2" ), terminalToken( "C100", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( NumericUnitNamesUseNaturalOrder )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2228A-3.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<int, size_t> pinCounts;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &sheet, false ) == wxS( "U9" ) )
                pinCounts[symbol->GetUnit()] = symbol->GetPins().size();
        }
    }

    const std::map<int, size_t> expected = {
        { 1, 2 },   { 2, 21 }, { 3, 14 }, { 4, 9 },  { 5, 11 }, { 6, 8 },   { 7, 13 },
        { 8, 8 },   { 9, 9 },  { 10, 7 }, { 11, 5 }, { 12, 7 }, { 13, 15 }, { 14, 17 },
        { 15, 16 }, { 16, 12 }, { 17, 32 }, { 18, 6 }, { 19, 22 }, { 20, 22 },
    };
    BOOST_CHECK( pinCounts == expected );
}


BOOST_AUTO_TEST_CASE( OccurrenceReferencesOverridePlacedTemplateReferences )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI82AX-CX_NB8_EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    bool     foundR2 = false;
    bool     foundR17 = false;
    wxString refsAtR2Position;
    wxString refsAtR17Position;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    reference = symbol->GetRef( &sheet, false );

            if( symbol->GetPosition() == OrcadDbuToIu( 890, 200 ) )
            {
                refsAtR2Position += reference + wxS( " " );
                foundR2 = foundR2 || reference == wxS( "R2" );
            }

            if( symbol->GetPosition() == OrcadDbuToIu( 1060, 395 ) )
            {
                refsAtR17Position += reference + wxS( " " );
                foundR17 = foundR17 || reference == wxS( "R17" );
            }
        }
    }

    BOOST_CHECK_MESSAGE( foundR2, "references at R2 position: " << refsAtR2Position );
    BOOST_CHECK_MESSAGE( foundR17, "references at R17 position: " << refsAtR17Position );
}


BOOST_AUTO_TEST_CASE( RootOccurrenceTargetIdsOverridePlacedTemplateReferences )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "Low EMI demo board.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::set<std::string> refs = collectImportedRefs( *schematic );

    BOOST_CHECK( refs.count( "C1" ) );
    BOOST_CHECK( refs.count( "U1" ) );
    BOOST_CHECK( !refs.count( "C71" ) );
    BOOST_CHECK( !refs.count( "U12" ) );
}


BOOST_AUTO_TEST_CASE( FlatModernOccurrenceReferencesOverridePlacedReferences )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn;

    for( const std::filesystem::directory_entry& entry :
         std::filesystem::recursive_directory_iterator( corpusEnv ) )
    {
        if( entry.is_regular_file() && entry.path().filename() == "BDC.DSN"
            && entry.path().string().find( "backpack-bdc" ) != std::string::npos )
        {
            dsn = entry.path();
            break;
        }
    }

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    wxString reference;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetPosition() == OrcadDbuToIu( 485, 270 ) )
                reference = symbol->GetRef( &sheet, false );
        }
    }

    BOOST_CHECK_EQUAL( reference, wxString( "U10" ) );
}


BOOST_AUTO_TEST_CASE( SuperSpeedOccurrencePropertiesOverrideReusableComponents )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SuperSpeed Explorer Kit Schematic.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, SCH_SYMBOL*> switches;

    for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    reference = symbol->GetRef( &sheet, false );

            if( reference == wxS( "SW1" ) || reference == wxS( "SW2" ) )
                switches[reference] = symbol;
        }
    }

    BOOST_REQUIRE_EQUAL( switches.size(), 2u );

    for( const wxString& reference : { wxS( "SW1" ), wxS( "SW2" ) } )
    {
        SCH_SYMBOL* symbol = switches.at( reference );
        BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetText(), wxS( "434 123 050 816" ) );
        BOOST_REQUIRE( symbol->GetField( wxS( "Manufacturer" ) ) );
        BOOST_CHECK_EQUAL( symbol->GetField( wxS( "Manufacturer" ) )->GetText(),
                           wxS( "Wurth Electronics Inc" ) );
        BOOST_REQUIRE( symbol->GetField( wxS( "OrCAD Footprint" ) ) );
        BOOST_CHECK_EQUAL( symbol->GetField( wxS( "OrCAD Footprint" ) )->GetText(), wxS( "EVQ-PE105K" ) );
    }
}


BOOST_AUTO_TEST_CASE( EmptyOccurrencePropertiesClearTemplateMetadata )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    auto checkFieldCleared = [&]( const wxString& aFileName, const wxString& aReference,
                                  const wxString& aFieldName )
    {
        std::filesystem::path dsn = findCorpusDesign( corpusEnv, aFileName.ToStdString() );
        BOOST_REQUIRE( !dsn.empty() );

        std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
        SETTINGS_MANAGER           manager;
        manager.LoadProject( "" );
        schematic->SetProject( &manager.Prj() );
        schematic->CurrentSheet().clear();
        schematic->CurrentSheet().push_back( &schematic->Root() );

        SCH_IO_ORCAD plugin;
        plugin.LoadSchematicFile( dsn.string(), schematic.get() );

        SCH_FIELD* field = nullptr;

        for( const SCH_SHEET_PATH& sheet : schematic->BuildSheetListSortedByPageNumbers() )
        {
            for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheet, false ) == aReference )
                    field = symbol->GetField( aFieldName );
            }
        }

        BOOST_CHECK( !field || field->GetText().IsEmpty() );
    };

    checkFieldCleared( wxS( "DC2596A-3.DSN" ), wxS( "L2" ), wxS( "4th Part Field" ) );
    checkFieldCleared( wxS( "CY4532 Power Board Schematic.DSN" ), wxS( "J2" ), wxS( "PART_NUMBER" ) );
}


BOOST_AUTO_TEST_CASE( CapturePseudoGlobalWirelessPinsConnectAcrossPages )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2641A3-SCH.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "L1", "3" ), terminalToken( "L4", "3" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
    size_t nativePowerPins = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                wxString ref = symbol->GetRef( &path, false );

                if( ref != wxS( "L1" ) && ref != wxS( "L4" ) )
                    continue;

                for( SCH_PIN* pin : symbol->GetPins() )
                {
                    if( pin->GetNumber() != wxS( "3" ) )
                        continue;

                    BOOST_CHECK( pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN );
                    BOOST_CHECK( !pin->IsVisible() );
                    BOOST_CHECK( pin->IsGlobalPower() );
                    BOOST_CHECK_EQUAL( pin->GetName(), wxString( "$$$1" ) );
                    ++nativePowerPins;
                }
            }
            else if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( item ) )
            {
                BOOST_CHECK_NE( label->GetText(), wxString( "$$$1" ) );
            }
        }
    }

    BOOST_CHECK_EQUAL( nativePowerPins, 2 );

}


BOOST_AUTO_TEST_CASE( PowerAliasesConnectAcrossPages )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-54852_A5.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "R711", "2" ), terminalToken( "C104", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( ReusedPowerNetIdsDoNotJoinDisconnectedNets )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-31399_C4.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C105", "1" ), terminalToken( "C106", "1" ) },
                                                    { terminalToken( "C127", "1" ), terminalToken( "C128", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( ReusedPowerNetAliasesRemainPhysicallyScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC607A.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C1", "2" ), terminalToken( "C13", "2" ) },
                                                    { terminalToken( "C14", "2" ), terminalToken( "C2", "2" ) },
                                                    { terminalToken( "C3", "1" ), terminalToken( "C4", "1" ) },
                                                    { terminalToken( "C13", "1" ), terminalToken( "C31", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 4 );
    BOOST_CHECK_EQUAL( consistent, 4 );
}


BOOST_AUTO_TEST_CASE( SinglePowerMeaningPropagatesAcrossReusedNetId )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn =
            findCorpusDesign( corpusEnv, "630-60651-01_04_CYW9BTM2BASE3_20829_BaseBoard_Schematics.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C1", "2" ), terminalToken( "U5", "62" ),
                                                      terminalToken( "U5", "63" ), terminalToken( "U5", "65" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( RepeatedLocalNetNamesRemainSheetScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "parallella_gen0.dsn" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "R41", "2" ), terminalToken( "U26", "53" ) },
                                                    { terminalToken( "R57", "2" ), terminalToken( "R97", "1" ),
                                                      terminalToken( "U14", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( RepeatedHierarchicalPortNamesRemainSheetScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI347X-DC-EB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected;

    for( int channel = 1; channel <= 8; ++channel )
    {
        std::string q = "Q" + std::to_string( channel );
        std::string r = "R" + std::to_string( channel );
        std::string gatePin = std::to_string( std::array{ 1, 7, 8, 14, 29, 35, 36, 42 }[channel - 1] );
        std::string sourcePin = std::to_string( std::array{ 2, 6, 9, 13, 30, 34, 37, 41 }[channel - 1] );
        expected.push_back( { terminalToken( q, "G" ), terminalToken( "U1", gatePin ) } );
        expected.push_back( { terminalToken( q, "2" ), terminalToken( q, "3" ), terminalToken( q, "S" ),
                              terminalToken( r, "2" ), terminalToken( "U1", sourcePin ) } );
    }

    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 16 );
    BOOST_CHECK_EQUAL( consistent, 16 );
}


BOOST_AUTO_TEST_CASE( RepeatedHierarchicalOffpageNamesRemainSheetScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "Si828X-BW-GDB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = {
        { terminalToken( "D317", "K" ), terminalToken( "JT1", "2" ), terminalToken( "Q13", "E" ) },
        { terminalToken( "D322", "K" ), terminalToken( "JT6", "2" ), terminalToken( "Q12", "E" ) },
        { terminalToken( "Q6", "C" ), terminalToken( "Q7", "C" ), terminalToken( "Q15", "C" ),
          terminalToken( "U204", "9" ) },
        { terminalToken( "Q10", "C" ), terminalToken( "Q11", "C" ), terminalToken( "Q20", "C" ),
          terminalToken( "U2", "13" ) }
    };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 4 );
    BOOST_CHECK_EQUAL( consistent, 4 );
}


BOOST_AUTO_TEST_CASE( NestedOccurrenceReferencesOverridePlacedTemplateReferences )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI8284v2-EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::set<std::string> refs = collectImportedRefs( *schematic );

    BOOST_CHECK( refs.count( "Q1-1" ) );
    BOOST_CHECK( refs.count( "Q1-2" ) );
    BOOST_CHECK( refs.count( "Q8-1" ) );
    BOOST_CHECK( refs.count( "Q8-2" ) );
}


BOOST_AUTO_TEST_CASE( SharedOccurrenceNetWithoutTerminalPeerKeepsBaseName )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI8284v2-EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "J15" ), wxS( "2" ) ).AfterLast( '/' ).Lower(),
                       wxString( wxS( "s3" ) ) );
}


BOOST_AUTO_TEST_CASE( PowerNetNameOverridesLocalWireAlias )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI8284v2-EVB.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    wxString boostNet = terminalNetName( *schematic, wxS( "C36" ), wxS( "1" ) );
    wxString supplyNet = terminalNetName( *schematic, wxS( "C12" ), wxS( "1" ) );
    BOOST_CHECK_EQUAL( boostNet.AfterLast( '/' ), wxString( "VDDB" ) );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "Q4" ), wxS( "C" ) ), boostNet );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "U2" ), wxS( "20" ) ), supplyNet );
    BOOST_CHECK_NE( boostNet, supplyNet );

    const IMPORT_NET_MAP* map = schematic->GetImportNetMap();
    BOOST_REQUIRE( map );
    auto mapped = std::find_if( map->entries.begin(), map->entries.end(),
                               []( const IMPORT_NET_MAP_ENTRY& entry )
                               {
                                   return entry.sourceNetId == 16939716
                                          && entry.originalName == wxS( "N16864826" );
                               } );
    BOOST_REQUIRE( mapped != map->entries.end() );
    BOOST_CHECK_EQUAL( mapped->status, IMPORT_NET_STATUS::RESOLVED );
    BOOST_CHECK_EQUAL( mapped->nameAtImport, boostNet );
}


BOOST_AUTO_TEST_CASE( NativePowerNamesIgnoreSourceCase )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    for( const char* filename : { "Si828X-BW-GDB.DSN", "SI828X-AW-GDB.DSN" } )
    {
        BOOST_TEST_CONTEXT( filename )
        {
            std::filesystem::path dsn = findCorpusDesign( corpusEnv, filename );
            BOOST_REQUIRE_MESSAGE( !dsn.empty(), filename << " not present in corpus." );
            SETTINGS_MANAGER manager;
            manager.LoadProject( "" );
            SCHEMATIC schematic( &manager.Prj() );
            SCH_IO_ORCAD plugin;
            plugin.LoadSchematicFile( dsn.string(), &schematic );

            wxString netName = terminalNetName( schematic, wxS( "C306" ), wxS( "1" ) );
            BOOST_CHECK_EQUAL( netName.Upper(), wxString( "LS-SOURCE" ) );

            for( const auto& [reference, pin] :
                 { std::pair{ wxString( "U2" ), wxString( "16" ) },
                   std::pair{ wxString( "C307" ), wxString( "2" ) },
                   std::pair{ wxString( "C309" ), wxString( "1" ) },
                   std::pair{ wxString( "C310" ), wxString( "1" ) },
                   std::pair{ wxString( "R333" ), wxString( "1" ) },
                   std::pair{ wxString( "UB8" ), wxString( "3" ) },
                   std::pair{ wxString( "UT6" ), wxString( "5" ) } } )
            {
                BOOST_CHECK_EQUAL( terminalNetName( schematic, reference, pin ), netName );
            }

            std::set<wxString> powerNames;

            for( const SCH_SHEET_PATH& sheet : schematic.BuildSheetListSortedByPageNumbers() )
            {
                for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
                {
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                    if( symbol->GetField( FIELD_T::VALUE )->GetText().CmpNoCase( wxS( "LS-SOURCE" ) ) == 0
                        && symbol->GetLibSymbolRef()->IsGlobalPower() )
                    {
                        powerNames.insert( symbol->GetField( FIELD_T::VALUE )->GetText() );
                    }
                }
            }

            BOOST_REQUIRE_EQUAL( powerNames.size(), 1u );
            BOOST_CHECK_EQUAL( *powerNames.begin(), netName );
        }
    }
}


BOOST_AUTO_TEST_CASE( HiddenWireLabelsAvoidBusCrossings )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CN81XX_GBCV2_sch_0530.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    wxString r13Pin1Net;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "R13" )
                    && pin->GetNumber() == wxS( "1" ) )
                {
                    r13Pin1Net = key.Name;
                }
            }
        }
    }

    BOOST_CHECK_EQUAL( r13Pin1Net.AfterLast( '/' ), wxS( "DDR0_DM1" ) );
}


BOOST_AUTO_TEST_CASE( IncompleteCachedSymbolsUsePlacedPins )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "1979A.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "J1", "1" ), terminalToken( "R5", "1" ) },
                                                    { terminalToken( "J2", "1" ), terminalToken( "R6", "2" ) },
                                                    { terminalToken( "J1", "2" ), terminalToken( "J1", "3" ),
                                                      terminalToken( "J2", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( LegacyOffpageConnectorsKeepDistinctPinPositions )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC726A-1.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "J1", "4" ), terminalToken( "R40", "1" ),
                                                      terminalToken( "R9", "1" ), terminalToken( "U5", "14" ) },
                                                    { terminalToken( "J1", "6" ), terminalToken( "R10", "1" ),
                                                      terminalToken( "U5", "13" ) },
                                                    { terminalToken( "J1", "7" ), terminalToken( "R39", "1" ),
                                                      terminalToken( "R5", "1" ), terminalToken( "U5", "16" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( EmptyPackagePinNumbersUseLogicalPinNames )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2283A-2.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C41", "1" ), terminalToken( "C42", "1" ) },
                                                    { terminalToken( "C42", "2" ), terminalToken( "J12", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( EmptyPackagePinNumbersPreserveAlphabeticNames )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "EMS4_0.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "DL1", "C" ), terminalToken( "R14", "1" ) },
                                                    { terminalToken( "TP1", "A" ), terminalToken( "U1", "7" ) },
                                                    { terminalToken( "U10", "PAD" ), terminalToken( "U11", "PAD" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( EmbeddedTwoPinPackageNamesDoNotReplacePhysicalNumbers )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2793A-3.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "D1", "1" ), terminalToken( "R6", "2" ) },
                                                    { terminalToken( "D1", "2" ), terminalToken( "R4", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( DirectPowerSymbolSharesComponentPin )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY4532 Power Board Schematic.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C70", "2" ), terminalToken( "C72", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( DirectPowerSymbolUsesComponentPinNet )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2047A-3-A.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "D15", "2" ), terminalToken( "D16", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( NearbyOffpageConnectorsRemainDistinct )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn =
            findCorpusDesign( corpusEnv, "630-60651-01_04_CYW9BTM2BASE3_20829_BaseBoard_Schematics.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "J14", "1" ), terminalToken( "J16", "17" ),
                                                      terminalToken( "R110", "2" ) },
                                                    { terminalToken( "J16", "19" ), terminalToken( "J2", "2" ) },
                                                    { terminalToken( "J16", "67" ), terminalToken( "J2", "3" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( MatchingSymbolAndPackageVariantsPreservePhysicalPinNames )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY4532 Power Board Schematic.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "D5", "A" ), terminalToken( "U2", "24" ) },
                                                    { terminalToken( "D5", "K" ), terminalToken( "U2", "20" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( OffpageParentBindingKeepsVisibleLabelsApart )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI347XY_MB_EVB.DSN" );

    if( dsn.empty() )
        return;

    SCHEMATIC schematic( nullptr );
    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    schematic.SetProject( &manager.Prj() );
    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), &schematic );
    int checked = 0;

    for( const SCH_SHEET_PATH& sheet : schematic.BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( item );

            if( label->GetText() != "VDD" )
                continue;

            for( SCH_ITEM* other : sheet.LastScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
            {
                SCH_HIERLABEL* hierLabel = static_cast<SCH_HIERLABEL*>( other );

                if( hierLabel->GetText() != label->GetText() || hierLabel->GetPosition() != label->GetPosition() )
                    continue;

                ++checked;
                BOOST_CHECK( hierLabel->GetSpinStyle()
                             == label->GetSpinStyle().RotateCCW().RotateCCW().Spin() );
                BOOST_CHECK( label->GetTextColor() == KIGFX::COLOR4D::UNSPECIFIED
                             || label->GetTextColor().a > 0 );
                BOOST_CHECK( hierLabel->GetTextColor() == KIGFX::COLOR4D::UNSPECIFIED
                             || hierLabel->GetTextColor().a > 0 );
            }
        }
    }

    BOOST_CHECK_EQUAL( checked, 1 );
}


BOOST_AUTO_TEST_CASE( OffpageAtWireBranchHasVisibleSafeAnchor )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2659A-4.DSN" );

    if( dsn.empty() )
        return;

    SCHEMATIC schematic( nullptr );
    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    schematic.SetProject( &manager.Prj() );
    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), &schematic );
    int checked = 0;

    for( const SCH_SHEET_PATH& sheet : schematic.BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            if( item->Type() != SCH_GLOBAL_LABEL_T )
                continue;

            SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( item );

            // PAGE2's source connector starts at a three-wire branch; nearby labels belong to other source objects.
            if( label->GetText() != "VOUT2"
                || ( label->GetPosition() - VECTOR2I( 4140200, 901700 ) ).SquaredEuclideanNorm()
                           > int64_t( 15000 ) * 15000 )
                continue;

            ++checked;
            BOOST_CHECK( label->GetTextColor() == KIGFX::COLOR4D::UNSPECIFIED
                         || label->GetTextColor().a > 0 );
            int incident = 0;

            for( SCH_ITEM* other : sheet.LastScreen()->Items() )
            {
                if( other->Type() != SCH_LINE_T )
                    continue;

                SCH_LINE* line = static_cast<SCH_LINE*>( other );

                if( ( line->GetLayer() == LAYER_WIRE || line->GetLayer() == LAYER_BUS )
                    && line->GetSeg().Contains( label->GetPosition() ) )
                    ++incident;
            }

            BOOST_CHECK_LE( incident, 1 );
        }
    }

    BOOST_CHECK_EQUAL( checked, 1 );
}


BOOST_AUTO_TEST_CASE( OffpageDisplayNameDoesNotChangeConnectivity )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping off-page display-name check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2509A-1.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "DC2509A-1.DSN not present in corpus; skipping off-page display-name check." );
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

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::string, CONNECTION_SUBGRAPH*> pinNets;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( !symbol || pin->GetNumber() != wxS( "1" ) )
                    continue;

                wxString ref = symbol->GetRef( &subgraph->GetSheet(), false );

                if( ref == wxS( "U3" ) || ref == wxS( "U4" ) )
                    pinNets[ref.ToStdString()] = subgraph;
            }
        }
    }

    BOOST_REQUIRE_EQUAL( pinNets.count( "U3" ), 1u );
    BOOST_REQUIRE_EQUAL( pinNets.count( "U4" ), 1u );
    BOOST_CHECK_EQUAL( pinNets["U3"], pinNets["U4"] );
}


BOOST_AUTO_TEST_CASE( SingleLeafOccurrencePreservesNamedNet )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping single leaf occurrence check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-28988.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "SCH-28988.DSN not present in corpus; skipping single leaf occurrence check." );
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

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    wxString netName;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "C3" )
                    && pin->GetNumber() == wxS( "1" ) )
                {
                    netName = key.Name;
                }
            }
        }
    }

    BOOST_CHECK_EQUAL( netName.AfterLast( '/' ), wxString( "ANALOG5V" ) );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "SH2" ), wxS( "2" ) ), netName );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "U1" ), wxS( "4" ) ), netName );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "VOUT_5" ), wxS( "1" ) ), netName );

    const IMPORT_NET_MAP* map = schematic->GetImportNetMap();
    BOOST_REQUIRE( map );
    auto mapped = std::find_if( map->entries.begin(), map->entries.end(),
                               []( const IMPORT_NET_MAP_ENTRY& entry )
                               {
                                   return entry.sourceNetId == 16645429
                                          && entry.originalName == wxS( "ANALOG5V" );
                               } );
    BOOST_REQUIRE( mapped != map->entries.end() );
    BOOST_CHECK_EQUAL( mapped->status, IMPORT_NET_STATUS::RESOLVED );
    BOOST_CHECK_EQUAL( mapped->nameAtImport, netName );
    BOOST_REQUIRE( !mapped->occurrence.empty() );
    BOOST_CHECK_EQUAL( mapped->occurrence.back(), wxString( "BRKTSTBCDP5004" ) );
}


BOOST_AUTO_TEST_CASE( PackageVariantsAndIgnoredPinsArePreserved )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping package-variant check." );
        return;
    }

    auto load = [&]( const char* aName )
    {
        std::filesystem::path dsn = findCorpusDesign( corpusEnv, aName );
        BOOST_REQUIRE_MESSAGE( !dsn.empty(), aName << " not present in corpus." );

        std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
        SETTINGS_MANAGER*          manager = new SETTINGS_MANAGER;
        manager->LoadProject( "" );
        schematic->SetProject( &manager->Prj() );
        schematic->CurrentSheet().clear();
        schematic->CurrentSheet().push_back( &schematic->Root() );

        SCH_IO_ORCAD plugin;
        plugin.LoadSchematicFile( dsn.string(), schematic.get() );
        return std::pair( std::move( schematic ), std::unique_ptr<SETTINGS_MANAGER>( manager ) );
    };

    auto [breakout, breakoutManager] = load( "OC_CONNECT_1_BRKOUT_BRD.DSN" );
    std::map<wxString, std::set<wxString>> switchPins;

    for( const SCH_SHEET_PATH& path : breakout->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetRef( &path, false );

            if( ref != wxS( "S1" ) && ref != wxS( "S3" ) )
                continue;

            for( SCH_PIN* pin : symbol->GetPins( &path ) )
                switchPins[ref].insert( pin->GetNumber() );
        }
    }

    const std::set<wxString> expectedS1 = { wxS( "1" ), wxS( "2" ), wxS( "3" ), wxS( "4" ) };
    const std::set<wxString> expectedS3 = { wxS( "1" ), wxS( "2" ), wxS( "3" ) };
    BOOST_CHECK_EQUAL_COLLECTIONS( switchPins[wxS( "S1" )].begin(), switchPins[wxS( "S1" )].end(), expectedS1.begin(),
                                   expectedS1.end() );
    BOOST_CHECK_EQUAL_COLLECTIONS( switchPins[wxS( "S3" )].begin(), switchPins[wxS( "S3" )].end(), expectedS3.begin(),
                                   expectedS3.end() );

    auto [j401, j401Manager] = load( "reServer industrial J401 Carrier Board v11.DSN" );
    std::set<wxString> j10Pins;

    for( const SCH_SHEET_PATH& path : j401->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "J10" ) )
                continue;

            for( SCH_PIN* pin : symbol->GetPins( &path ) )
                j10Pins.insert( pin->GetNumber() );
        }
    }

    BOOST_CHECK_EQUAL( j10Pins.size(), 53u );
    BOOST_CHECK_EQUAL( j10Pins.count( wxS( "SS1" ) ), 0u );
    BOOST_CHECK_EQUAL( j10Pins.count( wxS( "SS2" ) ), 0u );
}


BOOST_AUTO_TEST_CASE( LegacyDesignCachePackagePinMapsArePreserved )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping legacy package-map check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "EXAMPLE.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "EXAMPLE.DSN not present in corpus; skipping legacy package-map check." );
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

    std::set<wxString> pins;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "U4" ) )
                continue;

            for( SCH_PIN* pin : symbol->GetPins( &path ) )
                pins.insert( pin->GetNumber() );
        }
    }

    const std::set<wxString> expected = { wxS( "2" ), wxS( "3" ), wxS( "4" ), wxS( "5" ), wxS( "12" ) };
    BOOST_CHECK_EQUAL_COLLECTIONS( pins.begin(), pins.end(), expected.begin(), expected.end() );
}


BOOST_AUTO_TEST_CASE( S593487_PartialConnectorPinOrderIsPreserved )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping S-593487 connector check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "S-593487-REV-B.DSN" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "S-593487-REV-B.DSN not present in corpus." );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "JCA2", "1" ), terminalToken( "RT1", "1" ) },
                                             { terminalToken( "JCA2", "2" ), terminalToken( "RT2", "1" ) },
                                             { terminalToken( "JCA2", "3" ), terminalToken( "RT3", "1" ) },
                                             { terminalToken( "JCA2", "4" ), terminalToken( "RT4", "1" ) } } );
    BOOST_CHECK_EQUAL( checkable, 4 );
    BOOST_CHECK_EQUAL( consistent, 4 );
}


BOOST_AUTO_TEST_CASE( M5275_ExplicitPowerPinsRemainVisible )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping M5275 visible-power-pin check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "M5275EVB.DSN" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "M5275EVB.DSN not present in corpus." );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, bool> visible;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "U21" ) )
                continue;

            for( SCH_PIN* pin : symbol->GetPins( &path ) )
                visible[pin->GetNumber()] = pin->IsVisible();
        }
    }

    BOOST_REQUIRE_EQUAL( visible.size(), 6u );
    BOOST_CHECK( visible[wxS( "2" )] );
    BOOST_CHECK( visible[wxS( "5" )] );
}


BOOST_AUTO_TEST_CASE( SI34062_StackedSwitchPinsShareNet )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping SI34062 stacked-pin check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI34062-ISO-FB-EVB.DSN" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "SI34062-ISO-FB-EVB.DSN not present in corpus." );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] = checkConnectivity(
            *schematic, { { terminalToken( "S2", "1" ), terminalToken( "S2", "2" ), terminalToken( "U5", "K" ) } } );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( DuplicatePowerAliasTextDoesNotMergeDistinctNets )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping duplicate power-alias check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI8284v2-EVB.DSN" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "SI8284v2-EVB.DSN not present in corpus." );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "C12", "1" ), terminalToken( "U2", "20" ) },
                                             { terminalToken( "C36", "1" ), terminalToken( "Q4", "C" ) } } );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( LogicalPowerPinNameDoesNotOverrideConnectedWire )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping logical power-pin check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY8CKIT-041-41XX Schematic.DSN" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "CY8CKIT-041-41XX Schematic.DSN not present in corpus." );

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    bool checkedHeaderPin = false;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "J8" )
                || !path.Last()->GetName().Contains( wxS( "PSoC 5LP Programmer" ) ) )
            {
                continue;
            }

            for( SCH_PIN* pin : symbol->GetPins() )
            {
                if( pin->GetNumber() == wxS( "3" ) )
                {
                    BOOST_CHECK( pin->GetPosition() == OrcadDbuToIu( 1192, 1006 ) );
                    checkedHeaderPin = true;
                }
            }
        }
    }

    BOOST_REQUIRE( checkedHeaderPin );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "L4", "2" ), terminalToken( "U1", "40" ) },
                                             { terminalToken( "C59", "1" ), terminalToken( "U15", "44" ) },
                                             { terminalToken( "J8", "3" ), terminalToken( "U15", "28" ),
                                               terminalToken( "R40", "1" ) } } );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( PinNameOverbarsUseKiCadMarkup )
{
    BOOST_CHECK_EQUAL( OrcadPinNameMarkup( wxS( "\\\\C\\S\\ ADD1" ) ), wxS( "~{CS} ADD1" ) );
    BOOST_CHECK_EQUAL( OrcadPinNameMarkup( wxS( "\\\\I\\N\\T\\" ) ), wxS( "~{INT}" ) );
    BOOST_CHECK_EQUAL( OrcadPinNameMarkup( wxS( "READY" ) ), wxS( "READY" ) );
}


BOOST_AUTO_TEST_CASE( RenamedPowerNetsRemainElectricallyDistinct )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY4605_Schematic.dsn" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C16", "1" ), terminalToken( "TP4", "1" ) },
                                                    { terminalToken( "C14", "1" ), terminalToken( "TP2", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( MinorityPowerOccurrenceAliasDoesNotMergeGlobalNets )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CYW920706WCDEVAL Evaluation Kit Schematics.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C1", "2" ), terminalToken( "C10", "2" ) },
                                                    { terminalToken( "C26", "2" ), terminalToken( "C27", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( OccurrenceSuffixedPowerNetsRemainDistinct )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2263A-2.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C13", "1" ), terminalToken( "C18", "1" ) },
                                                    { terminalToken( "C40", "1" ), terminalToken( "R61", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( GeneratedOccurrenceNetNameRemainsDistinct )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "OC_CONNECT-1_BB_BOARD_20072023_01.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C1", "2" ), terminalToken( "C10", "2" ) },
                                                    { terminalToken( "LED14", "1" ), terminalToken( "R304", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( PackageVariantPreservesPhysicalPinMap )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "710-DC2693A_REV02_PCA_SCHEMATIC.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C101", "1" ), terminalToken( "U101", "6" ) },
                                                    { terminalToken( "R105", "2" ), terminalToken( "U101", "3" ) },
                                                    { terminalToken( "R106", "1" ), terminalToken( "U101", "4" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 3 );
    BOOST_CHECK_EQUAL( consistent, 3 );
}


BOOST_AUTO_TEST_CASE( DistinctPowerAndOffpageInterfaceNamesRemainDistinct )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "OpenCellular_Connect-1_GBC_Life-3_Schematic_v1.2.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C1559", "2" ), terminalToken( "D8", "2" ) },
                                                    { terminalToken( "L28", "3" ), terminalToken( "R1009", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( FlatTopLevelOffpageConnectsAcrossPages )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC1931B.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "J4", "H26" ), terminalToken( "U2", "N1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( PowerAliasConnectsAcrossPages )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "710-DC2222A_REV07_PCA_SCHEMATIC.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<std::set<std::string>> expected = { { terminalToken( "C10", "2" ), terminalToken( "C29", "1" ),
                                                      terminalToken( "C30", "1" ), terminalToken( "C9", "2" ),
                                                      terminalToken( "E2", "1" ), terminalToken( "R19", "1" ),
                                                      terminalToken( "U1", "3" ), terminalToken( "U14", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( PhysicalConnectorPinsUseDefinitionNumbers )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2382A-1.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::set<wxString> pinNumbers;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "J3" ) )
                continue;

            for( SCH_PIN* pin : symbol->GetPins( &path ) )
                pinNumbers.insert( pin->GetNumber() );
        }
    }

    std::set<wxString> expected;

    for( int pin = 1; pin <= 20; ++pin )
        expected.insert( wxString::Format( wxS( "%d" ), pin ) );

    BOOST_CHECK_EQUAL_COLLECTIONS( pinNumbers.begin(), pinNumbers.end(), expected.begin(), expected.end() );

    std::vector<std::set<std::string>> expectedNets = { { terminalToken( "J3", "1" ), terminalToken( "Q1", "3" ) },
                                                        { terminalToken( "J3", "2" ), terminalToken( "J1", "2" ) },
                                                        { terminalToken( "J3", "3" ), terminalToken( "J1", "3" ) },
                                                        { terminalToken( "J3", "10" ), terminalToken( "J1", "10" ) },
                                                        { terminalToken( "J3", "13" ), terminalToken( "R19", "1" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expectedNets );
    BOOST_CHECK_EQUAL( checkable, 5 );
    BOOST_CHECK_EQUAL( consistent, 5 );
}


BOOST_AUTO_TEST_CASE( ModernPackageStreamsSupplyEmbeddedSymbolGeometry )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC2382A-1.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SYMBOL* jumper = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "JP4" ) )
                jumper = symbol;
        }
    }

    BOOST_REQUIRE( jumper );
    BOOST_REQUIRE( jumper->GetLibSymbolRef() );

    int bodyRectangles = 0;

    for( const SCH_ITEM& item : jumper->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_SHAPE_T && static_cast<const SCH_SHAPE&>( item ).GetShape() == SHAPE_T::RECTANGLE )
            ++bodyRectangles;
    }

    BOOST_CHECK_EQUAL( bodyRectangles, 3 );
}


BOOST_AUTO_TEST_CASE( LegacyDisplayTypesRemainVisible )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "X375D_VER72.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    int checked = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "CLKOUT0" ) )
                continue;

            BOOST_CHECK( symbol->GetField( FIELD_T::REFERENCE )->IsVisible() );
            ++checked;
        }
    }

    BOOST_CHECK_EQUAL( checked, 1 );
}


BOOST_AUTO_TEST_CASE( UnreferencedPagesDoNotOverwriteHierarchicalSheets )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SI8281V2-EVB.DSN" );

    if( dsn.empty() )
        return;

    m_plugin.LoadSchematicFile( dsn.string(), m_schematic.get() );
    std::map<wxString, SCH_SCREEN*> screensByFile;
    size_t connectorPins = 0;

    for( const SCH_SHEET_PATH& path : m_schematic->BuildSheetListSortedByPageNumbers() )
    {
        SCH_SCREEN* screen = path.LastScreen();
        auto [entry, inserted] = screensByFile.emplace( screen->GetFileName().Lower(), screen );
        BOOST_CHECK_MESSAGE( inserted || entry->second == screen,
                             "Distinct sheets share output file " << screen->GetFileName() );

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) == wxS( "J24" ) )
                connectorPins += symbol->GetPins().size();
        }
    }

    BOOST_CHECK_EQUAL( connectorPins, 16 );
}


BOOST_AUTO_TEST_CASE( HierarchicalPortsUseVisibleNativeLabels )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "M5275EVB.DSN" );

    if( dsn.empty() )
        return;

    m_plugin.LoadSchematicFile( dsn.string(), m_schematic.get() );
    size_t ports = 0;

    for( const SCH_SHEET_PATH& path : m_schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
        {
            SCH_HIERLABEL* label = static_cast<SCH_HIERLABEL*>( item );
            BOOST_CHECK_MESSAGE( label->GetTextColor() == KIGFX::COLOR4D::UNSPECIFIED
                                         || label->GetTextColor().a > 0,
                                 "Hidden hierarchical port " << label->GetText() );
            ++ports;
        }
    }

    BOOST_CHECK_GT( ports, 0 );
}


BOOST_AUTO_TEST_CASE( LegacyHierarchicalBlockImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping legacy hierarchy check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-20380.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "SCH-20380.DSN not present in corpus; skipping legacy hierarchy check." );
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
    schematic->CurrentSheet().UpdateAllScreenReferences();

    size_t pages = 0;
    size_t components = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        ++pages;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T
                && !static_cast<SCH_SYMBOL*>( item )->GetRef( &path, false ).StartsWith( wxS( "#" ) ) )
            {
                ++components;
            }

        }
    }

    BOOST_CHECK_EQUAL( pages, 16u );
    BOOST_CHECK_EQUAL( components, 491u );
    auto [consistent, checkable] = checkConnectivity(
            *schematic, { { "J7.54", "RP17.3", "U10.C13" },
                          { "J10.35", "J12.9", "RP15.7", "U10.N10" } } );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( LegacyDsnDiodePinsUseLogicalPolarity )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-20380.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    const std::vector<std::set<std::string>> expected = {
        { terminalToken( "D1", "2" ), terminalToken( "C37", "1" ) },
        { terminalToken( "D1", "1" ), terminalToken( "R15", "1" ) }
    };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, expected.size() );
    BOOST_CHECK_EQUAL( consistent, expected.size() );
}


BOOST_AUTO_TEST_CASE( LegacyDsnEmbeddedSlashNetNameIsAuthoritative )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-20380.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    wxString j1Pin7Net;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol && symbol->GetRef( &subgraph->GetSheet(), false ) == wxS( "J1" )
                    && pin->GetNumber() == wxS( "7" ) )
                {
                    j1Pin7Net = key.Name;
                }
            }
        }
    }

    BOOST_CHECK( !j1Pin7Net.IsEmpty() );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "RP13" ), wxS( "7" ) ), j1Pin7Net );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "U26" ), wxS( "B" ) ), j1Pin7Net );

    const IMPORT_NET_MAP* map = schematic->GetImportNetMap();
    BOOST_REQUIRE( map );
    auto mapped = std::find_if( map->entries.begin(), map->entries.end(),
                               []( const IMPORT_NET_MAP_ENTRY& entry )
                               {
                                   return entry.sourceNetId == 3221698
                                          && entry.originalName == wxS( "BDM_/RSTIN" );
                               } );
    BOOST_REQUIRE( mapped != map->entries.end() );
    BOOST_CHECK_EQUAL( mapped->status, IMPORT_NET_STATUS::RESOLVED );
    BOOST_CHECK_EQUAL( mapped->nameAtImport, j1Pin7Net );

    bool sourcePort = false;

    for( const SCH_SHEET_PATH& sheet : sheets )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
            sourcePort |= static_cast<SCH_HIERLABEL*>( item )->GetText() == wxS( "BDM_/RSTIN" );
    }

    BOOST_CHECK( sourcePort );
}


BOOST_AUTO_TEST_CASE( UninstantiatedLegacyPageIsExcludedFromBoard )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SCH-21095.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    schematic->CurrentSheet().UpdateAllScreenReferences();

    SCH_SHEET* mram = nullptr;
    SCH_SHEET* reset = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        SCH_SHEET* sheet = path.Last();

        if( path.LastScreen()->GetFileName().Upper().Contains( wxS( "MRAM" ) ) )
            mram = sheet;
        else if( path.LastScreen()->GetFileName().Upper().Contains( wxS( "RESET" ) ) )
            reset = sheet;
    }

    BOOST_REQUIRE( mram );
    BOOST_REQUIRE( reset );
    BOOST_CHECK( mram->GetExcludedFromBoard() );
    BOOST_CHECK( !reset->GetExcludedFromBoard() );
}


BOOST_AUTO_TEST_CASE( LegacyFlatPageImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping legacy flat-page check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "X375D_VER72.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "X375D_VER72.DSN not present in corpus; skipping legacy flat-page check." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD       plugin;
    WX_STRING_REPORTER reporter;
    plugin.SetReporter( &reporter );

    try
    {
        plugin.LoadSchematicFile( dsn.string(), schematic.get() );
    }
    catch( const std::exception& e )
    {
        BOOST_FAIL( e.what() << "\n" << reporter.GetMessages() );
        return;
    }

    size_t pages = 0;
    size_t components = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        ++pages;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T
                && !static_cast<SCH_SYMBOL*>( item )->GetRef( &path, false ).StartsWith( wxS( "#" ) ) )
            {
                ++components;
            }

        }
    }

    BOOST_CHECK_EQUAL( pages, 1u );
    BOOST_CHECK_EQUAL( components, 256u );
}


BOOST_AUTO_TEST_CASE( LegacyHierarchyPowerTableImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping legacy power-table check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "DC1414B.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "DC1414B.DSN not present in corpus; skipping legacy power-table check." );
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
    schematic->CurrentSheet().UpdateAllScreenReferences();

    size_t pages = 0;
    size_t components = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        ++pages;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T
                && !static_cast<SCH_SYMBOL*>( item )->GetRef( &path, false ).StartsWith( wxS( "#" ) ) )
            {
                ++components;
            }

        }
    }

    BOOST_CHECK_EQUAL( pages, 1u );
    BOOST_CHECK_EQUAL( components, 74u );
}


BOOST_AUTO_TEST_CASE( Issue25005Hierarchy )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping issue 25005." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CFW-002.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "CFW-002.DSN not present in corpus; skipping issue 25005." );
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

    std::vector<SCH_SHEET*> topSheets = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( topSheets.size(), 1u );

    SCH_SCREEN* rootScreen = topSheets.front()->GetScreen();
    size_t      sheets = 0;
    size_t      sheetPins = 0;

    for( SCH_ITEM* item : rootScreen->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
        ++sheets;
        sheetPins += sheet->GetPins().size();
    }

    const std::vector<wxString> expectedNames = { wxS( "PAG_2" ), wxS( "PAG_3" ), wxS( "PAG_4" ), wxS( "PAG_5" ),
                                                  wxS( "PAG_6" ), wxS( "PAG_7" ), wxS( "PAG_8" ), wxS( "PAG_9" ) };
    const std::vector<size_t>   expectedPinCounts = { 31, 24, 39, 47, 40, 33, 26, 10 };
    SCH_SHEET_LIST              hierarchy = schematic->BuildSheetListSortedByPageNumbers();
    std::vector<wxString>       sheetNames;
    std::vector<size_t>         pinCounts;

    for( auto it = std::next( hierarchy.begin() ); it != hierarchy.end(); ++it )
    {
        SCH_SHEET*         sheet = it->Last();
        std::set<wxString> sheetPinNames;
        std::set<wxString> hierarchicalLabelNames;

        sheetNames.push_back( sheet->GetField( FIELD_T::SHEET_NAME )->GetText() );
        pinCounts.push_back( sheet->GetPins().size() );

        for( const SCH_SHEET_PIN* pin : sheet->GetPins() )
            sheetPinNames.insert( pin->GetText() );

        for( SCH_ITEM* item : sheet->GetScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
            hierarchicalLabelNames.insert( static_cast<SCH_HIERLABEL*>( item )->GetText() );

        BOOST_CHECK_EQUAL_COLLECTIONS( sheetPinNames.begin(), sheetPinNames.end(), hierarchicalLabelNames.begin(),
                                       hierarchicalLabelNames.end() );
    }

    schematic->ConnectionGraph()->Recalculate( hierarchy, true );

    BOOST_CHECK_EQUAL( hierarchy.size(), 9u );
    BOOST_CHECK_EQUAL( sheets, 8u );
    BOOST_CHECK_EQUAL( sheetPins, 250u );
    BOOST_CHECK_EQUAL_COLLECTIONS( sheetNames.begin(), sheetNames.end(), expectedNames.begin(), expectedNames.end() );
    BOOST_CHECK_EQUAL_COLLECTIONS( pinCounts.begin(), pinCounts.end(), expectedPinCounts.begin(),
                                   expectedPinCounts.end() );
}


BOOST_AUTO_TEST_CASE( Issue25009PageOrderAndGraphics )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping issue 25009." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SE_NGFOC-L_01.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "SE_NGFOC-L_01.DSN not present in corpus; skipping issue 25009." );
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

    const std::vector<wxString> expectedNames = { wxS( "01.REV.HISTORY" ), wxS( "02.uC" ),         wxS( "03.CAN" ),
                                                  wxS( "04. Ethercat" ),   wxS( "05.EtherSynch" ), wxS( "06.RS-485" ),
                                                  wxS( "11.GPIO" ),        wxS( "12.Analog" ),     wxS( "13:IMU" ),
                                                  wxS( "14.Bridge" ),      wxS( "15.Encoder" ),    wxS( "29.uCPower" ),
                                                  wxS( "30.PowerSupply" ), wxS( "31.Expansion" ) };

    std::vector<SCH_SHEET*> sheets = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( sheets.size(), expectedNames.size() );

    size_t wires = 0;
    size_t shapes = 0;
    size_t texts = 0;
    size_t tables = 0;

    for( size_t i = 0; i < sheets.size(); ++i )
    {
        BOOST_CHECK_EQUAL( sheets[i]->GetField( FIELD_T::SHEET_NAME )->GetText(), expectedNames[i] );

        for( SCH_ITEM* item : sheets[i]->GetScreen()->Items() )
        {
            if( item->Type() == SCH_LINE_T )
            {
                SCH_LINE* line = static_cast<SCH_LINE*>( item );
                BOOST_CHECK_EQUAL( line->GetLineWidth(), 0 );
                ++wires;
            }
            else if( item->Type() == SCH_SHAPE_T )
            {
                SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( item );

                if( i == 0 )
                    BOOST_CHECK( shape->GetFillMode() == FILL_T::NO_FILL );

                ++shapes;
            }
            else if( item->Type() == SCH_TEXT_T )
            {
                ++texts;
            }
            else if( item->Type() == SCH_TABLE_T )
            {
                ++tables;
            }
        }
    }

    BOOST_CHECK_EQUAL( wires, 1921u );
    BOOST_CHECK_EQUAL( shapes, 168u );
    BOOST_CHECK_EQUAL( texts, 206u );
    BOOST_CHECK_EQUAL( tables, 0u );
}


// Set KICAD_ORCAD_CORPUS to verify that private OLB files yield pins or graphics.

BOOST_AUTO_TEST_CASE( OlbLibraryImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD OLB library import." );
        return;
    }

    namespace fs = std::filesystem;
    std::vector<fs::path> libs;

    for( auto it = fs::recursive_directory_iterator( fs::path( corpusEnv ),
                                                     fs::directory_options::skip_permission_denied );
         it != fs::recursive_directory_iterator(); ++it )
    {
        if( !it->is_regular_file() )
            continue;

        std::string ext = it->path().extension().string();
        std::transform( ext.begin(), ext.end(), ext.begin(),
                        []( unsigned char c )
                        {
                            return std::tolower( c );
                        } );

        if( ext == ".olb" )
            libs.push_back( it->path() );
    }

    std::sort( libs.begin(), libs.end() );
    BOOST_TEST_MESSAGE( "OrCAD OLB libraries: " << libs.size() );

    int totalSymbols = 0, emptySymbols = 0, checkedLibs = 0, rejectedLibs = 0, crashedLibs = 0;

    for( const fs::path& lib : libs )
    {
        SCH_IO_ORCAD             plugin;
        std::vector<LIB_SYMBOL*> symbols;

        if( !plugin.CanReadLibrary( lib.string() ) )
        {
            ++rejectedLibs;
            continue;
        }

        try
        {
            // Vector overload materializes all symbols O(n); per-name LoadSymbol rescans O(n^2).
            plugin.EnumerateSymbolLib( symbols, lib.string() );
        }
        catch( const std::exception& e )
        {
            ++crashedLibs;
            BOOST_TEST_MESSAGE( "  THROW  " << lib.filename().string() << " : " << e.what() );
            continue;
        }

        ++checkedLibs;
        int withGeometry = 0;

        for( LIB_SYMBOL* symbol : symbols )
        {
            BOOST_REQUIRE( symbol );
            ++totalSymbols;

            if( symbol->GetPinCount() > 0 || !symbol->GetDrawItems().empty() )
                ++withGeometry;
            else
                ++emptySymbols;
        }

        BOOST_TEST_MESSAGE( "  " << lib.filename().string() << " : " << symbols.size() << " symbols, " << withGeometry
                                 << " with pins/graphics" );
    }

    BOOST_TEST_MESSAGE( "OLB summary: " << checkedLibs << " libs, " << rejectedLibs << " rejected, " << crashedLibs
                                        << " crashed, " << totalSymbols << " symbols, " << emptySymbols << " empty" );

    // Bad streams must degrade gracefully, not throw; wholesale empty result means decode broke.
    BOOST_CHECK_EQUAL( crashedLibs, 0 );
    BOOST_CHECK_GT( totalSymbols, 0 );

    if( totalSymbols )
        BOOST_CHECK_LT( emptySymbols, totalSymbols / 2 );
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

    BOOST_CHECK_EQUAL( schematic->Settings().m_DashedLineDashRatio, 3.0 );
    BOOST_CHECK_EQUAL( schematic->Settings().m_DashedLineGapRatio, 1.0 );

    // Pages become flat ordered top-level sheets, not a stitching root w/ children; "N - " prefix orders them.
    std::vector<SCH_SHEET*> tops = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( tops.size(), 3u );

    BOOST_CHECK_EQUAL( tops[0]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "CONTENTS" ) );
    BOOST_CHECK_EQUAL( tops[1]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "CM4,USB HUB,AUDIO,MIC" ) );
    BOOST_CHECK_EQUAL( tops[2]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "CSI, DSI, HDMI, MCU" ) );

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
                BOOST_CHECK_LT( sym->GetField( FIELD_T::REFERENCE )->GetPosition().x, sym->GetPosition().x );
                checkedField = true;
            }
        }
    }

    BOOST_CHECK( checkedField );
}


BOOST_AUTO_TEST_CASE( HierarchicalSymbolInstancesUseCanonicalSheetPaths )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping flat-page instance-path check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "OCTOPAES_10.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "OCTOPAES_10.DSN not present in corpus; skipping instance-path check." );
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

    int checked = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        if( !path.LastScreen()->GetFileName().Contains( wxS( "CPLD Power" ) ) )
            continue;

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetRef( &path, false ) != wxS( "P?" ) )
                continue;

            bool        canonical = std::any_of( symbol->GetInstances().begin(), symbol->GetInstances().end(),
                                                 [&]( const SCH_SYMBOL_INSTANCE& aInstance )
                                                 {
                                              return aInstance.m_Path == path.Path();
                                          } );
            std::string stored;

            for( const SCH_SYMBOL_INSTANCE& instance : symbol->GetInstances() )
                stored += instance.m_Path.AsString().ToStdString() + " ";

            BOOST_CHECK_MESSAGE( canonical, "expected=" << path.Path().AsString() << " stored=" << stored );
            ++checked;
        }
    }

    BOOST_CHECK_EQUAL( checked, 3 );
}


BOOST_AUTO_TEST_CASE( MultiPageHierarchyPreservesPortConnectivity )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping multi-page hierarchy check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "HB1A-AAFM.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "HB1A-AAFM.DSN not present in corpus; skipping multi-page hierarchy check." );
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

    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::tuple<std::string, std::string, std::string>, int> terminalNets;
    int                                                              netId = 0;

    for( const auto& [key, subgraphs] : schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            std::string page = subgraph->GetSheet().LastScreen()->GetFileName().ToStdString();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( symbol )
                {
                    terminalNets[{ page, symbol->GetRef( &subgraph->GetSheet(), false ).ToStdString(),
                                   pin->GetNumber().ToStdString() }] = netId;
                }
            }
        }

        ++netId;
    }

    auto findNet = [&]( const std::string& aPage, const std::string& aRef, const std::string& aPin )
    {
        for( const auto& [terminal, id] : terminalNets )
        {
            if( std::get<0>( terminal ).find( aPage ) != std::string::npos && std::get<1>( terminal ) == aRef
                && std::get<2>( terminal ) == aPin )
            {
                return id;
            }
        }

        return -1;
    };

    int clockNet = findNet( "Clock Generator", "J1", "1" );
    int fmcNet = findNet( "FMC Connector", "P1", "H38" );
    BOOST_REQUIRE_NE( clockNet, -1 );
    BOOST_REQUIRE_NE( fmcNet, -1 );
    BOOST_CHECK_EQUAL( clockNet, fmcNet );

    BOOST_CHECK_EQUAL( findNet( "Clock Generator", "J1", "2" ), findNet( "FMC Connector", "P1", "G37" ) );
    BOOST_CHECK_EQUAL( findNet( "Clock Generator", "U12", "7" ), findNet( "MAX II CPLD", "U9", "73" ) );
    BOOST_CHECK_EQUAL( findNet( "Current Sense", "U2", "2" ), findNet( "PROM & Misc", "U6", "4" ) );

    int mvddUr = findNet( "Power & Control", "TP13", "1" );
    int mvddUl = findNet( "Power & Control", "TP30", "1" );
    int mvddLr = findNet( "Power & Control", "TP14", "1" );
    int mvddLl = findNet( "Power & Control", "TP31", "1" );
    BOOST_REQUIRE_NE( mvddUr, -1 );
    BOOST_REQUIRE_NE( mvddUl, -1 );
    BOOST_REQUIRE_NE( mvddLr, -1 );
    BOOST_REQUIRE_NE( mvddLl, -1 );
    BOOST_CHECK_NE( mvddUr, mvddUl );
    BOOST_CHECK_NE( mvddUr, mvddLr );
    BOOST_CHECK_NE( mvddUr, mvddLl );
    BOOST_CHECK_NE( mvddUl, mvddLr );
    BOOST_CHECK_NE( mvddUl, mvddLl );
    BOOST_CHECK_NE( mvddLr, mvddLl );

    int urLclk = findNet( "Link Ports NORTH_SOUTH", "U4", "V6" );
    int lrLclk = findNet( "Link Ports NORTH_SOUTH", "U8", "A13" );
    BOOST_REQUIRE_NE( urLclk, -1 );
    BOOST_REQUIRE_NE( lrLclk, -1 );
    BOOST_CHECK_EQUAL( urLclk, lrLclk );
    BOOST_CHECK_NE( urLclk, findNet( "Link Ports NORTH_SOUTH", "U4", "V7" ) );

    int p1c35 = findNet( "FMC Connector", "P1", "C35" );
    int p1c37 = findNet( "FMC Connector", "P1", "C37" );
    BOOST_REQUIRE_NE( p1c35, -1 );
    BOOST_REQUIRE_NE( p1c37, -1 );
    BOOST_CHECK_EQUAL( p1c35, p1c37 );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "R265" ), wxS( "1" ) ).Lower(),
                       wxString( wxS( "ll_ul_ns_data_p_0" ) ) );
}


BOOST_AUTO_TEST_CASE( PowerSymbolPinSharesPartNet )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping power-symbol connectivity check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "POWER_SOURCE_BOARD_20180717.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "POWER_SOURCE_BOARD_20180717.DSN not present; skipping power-symbol connectivity check." );
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

    std::vector<std::set<std::string>> expected = {
        { terminalToken( "R410", "2" ), terminalToken( "R122", "1" ) }
    };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 1 );
    BOOST_CHECK_EQUAL( consistent, 1 );
}


BOOST_AUTO_TEST_CASE( RepeatedHierarchicalBusPinsRemainScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "meta_carrier_sch_rev1.dsn" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "meta_carrier_sch_rev1.dsn not present in corpus; skipping repeated bus check." );
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

    std::vector<std::set<std::string>> expected = {
        { "J1.1", "J11.1" },
        { "J12.239", "J5.239" },
        { "J15.239", "J3.239" },
        { "J4.1", "J6.1" },
    };

    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 4 );
    BOOST_CHECK_EQUAL( consistent, 4 );
    wxString netName = terminalNetName( *schematic, wxS( "C103" ), wxS( "1" ) );
    BOOST_CHECK_EQUAL( netName.AfterLast( '/' ), wxString( "LF2_EXT_CAP" ) );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "U7" ), wxS( "11" ) ), netName );
    const IMPORT_NET_MAP* map = schematic->GetImportNetMap();
    BOOST_REQUIRE( map );
    std::set<wxString> occurrenceNames;

    for( const IMPORT_NET_MAP_ENTRY& entry : map->entries )
    {
        if( entry.sourceNetId == 9438967 && entry.originalName == wxS( "LF2_EXT_CAP" ) )
        {
            BOOST_CHECK_EQUAL( entry.status, IMPORT_NET_STATUS::RESOLVED );
            occurrenceNames.insert( entry.nameAtImport );
        }
    }

    BOOST_CHECK( occurrenceNames.contains( netName ) );
    BOOST_CHECK_EQUAL( occurrenceNames.size(), 2u );
}


BOOST_AUTO_TEST_CASE( NestedHierarchicalBusRangesPreserveConnectivity )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "meta_module_sch_rev1.dsn" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "meta_module_sch_rev1.dsn not present in corpus; skipping nested bus check." );
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

    std::vector<std::set<std::string>> expected = {
        { terminalToken( "J1", "10" ), terminalToken( "U5", "B15" ) },
        { terminalToken( "J1", "100" ), terminalToken( "U7", "D11" ) },
    };

    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
    wxString netName = terminalNetName( *schematic, wxS( "R138" ), wxS( "1" ) );
    BOOST_CHECK_EQUAL( netName.AfterLast( '/' ), wxString( "WE_WAIT_WR_P0" ) );
    BOOST_CHECK_EQUAL( terminalNetName( *schematic, wxS( "U1" ), wxS( "K3" ) ), netName );
    const IMPORT_NET_MAP* map = schematic->GetImportNetMap();
    BOOST_REQUIRE( map );
    std::set<wxString> occurrenceNames;

    for( const IMPORT_NET_MAP_ENTRY& entry : map->entries )
    {
        if( entry.sourceNetId == 7937073 && entry.originalName == wxS( "WE_WAIT_WR_P0" ) )
        {
            BOOST_CHECK_EQUAL( entry.status, IMPORT_NET_STATUS::RESOLVED );
            occurrenceNames.insert( entry.nameAtImport );
        }
    }

    BOOST_CHECK( occurrenceNames.contains( netName ) );
    BOOST_CHECK_EQUAL( occurrenceNames.size(), 4u );
}


BOOST_AUTO_TEST_CASE( RenamedHierarchicalBusMembersPreserveConnectivity )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "HB1A-AAFM.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "HB1A-AAFM.DSN not present in corpus; skipping renamed bus check." );
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

    std::vector<std::set<std::string>> expected = {
        { terminalToken( "P1", "G36" ), terminalToken( "U9", "36" ) },
        { terminalToken( "P1", "H37" ), terminalToken( "U9", "35" ) },
    };

    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( RepeatedMultiPageFoldersKeepLeafNetsScoped )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "HB1A-AAFM.DSN" );

    if( dsn.empty() )
        return;

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto [consistent, checkable] =
            checkConnectivity( *schematic, { { terminalToken( "P1", "C10" ), terminalToken( "U4", "E16" ) },
                                             { terminalToken( "P1", "K10" ), terminalToken( "U8", "E16" ) },
                                             { terminalToken( "P1", "C11" ), terminalToken( "U4", "D16" ) },
                                             { terminalToken( "P1", "K11" ), terminalToken( "U8", "D16" ) } } );
    BOOST_CHECK_EQUAL( checkable, 4 );
    BOOST_CHECK_EQUAL( consistent, 4 );
}


BOOST_AUTO_TEST_CASE( DegenerateHierarchicalPinPlacementsUseDefinitionGeometry )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "buddy_sch_rev1.dsn" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "buddy_sch_rev1.dsn not present in corpus; skipping block-pin geometry check." );
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

    std::vector<std::set<std::string>> expected = {
        { "J1.C10", "U1.J19" }, { "J1.C11", "U1.K19" }, { "J3.C10", "U1.W33" },
        { "J3.F28", "U1.J35" }, { "J1.E33", "U1.B38" }, { "J3.E33", "U1.AV40" },
    };

    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 6 );
    BOOST_CHECK_EQUAL( consistent, 6 );
    wxString netName = terminalNetName( *schematic, wxS( "J6" ), wxS( "F35" ) );
    BOOST_CHECK( !netName.IsEmpty() );
    const IMPORT_NET_MAP* map = schematic->GetImportNetMap();
    BOOST_REQUIRE( map );
    auto mapped = std::find_if( map->entries.begin(), map->entries.end(),
                               [&]( const IMPORT_NET_MAP_ENTRY& entry )
                               {
                                   return entry.sourceNetId == 9578237
                                          && entry.originalName == wxS( "CTRL_N3" )
                                          && entry.nameAtImport == netName;
                               } );
    BOOST_REQUIRE( mapped != map->entries.end() );
    BOOST_CHECK_EQUAL( mapped->status, IMPORT_NET_STATUS::RESOLVED );
    BOOST_REQUIRE_EQUAL( mapped->occurrence.size(), 3u );
    BOOST_CHECK_EQUAL( mapped->occurrence[1], wxString( "10280601" ) );
}


BOOST_AUTO_TEST_CASE( PlacedUnitsSelectPackagePinMaps )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping placed-unit check." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "MC2_REV1_16_2.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "MC2_REV1_16_2.DSN not present; skipping placed-unit check." );
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

    std::vector<std::set<std::string>> expected = { { terminalToken( "CC2", "5" ), terminalToken( "FL11", "1" ) },
                                                    { terminalToken( "CC2", "6" ), terminalToken( "FL11", "2" ) } };
    auto [consistent, checkable] = checkConnectivity( *schematic, expected );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( Dc2693aSmaOnlyDisplaysCenterPinNumber )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "710-DC2693A_REV02_PCA_SCHEMATIC.DSN" );

    if( dsn.empty() )
        return;

    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    SCH_SYMBOL* j1 = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        j1 = findConvertedSymbol( *path.LastScreen(), path, wxS( "J1" ) );

        if( j1 )
            break;
    }

    BOOST_REQUIRE( j1 );
    BOOST_REQUIRE_EQUAL( j1->GetPins().size(), 5u );

    std::vector<wxString> displayedNumbers;

    for( const SCH_PIN* pin : j1->GetPins() )
    {
        if( j1->GetShowPinNumbers() && pin->IsVisible() && pin->GetNumberTextSize() > 0 )
            displayedNumbers.push_back( pin->GetNumber() );
    }

    BOOST_REQUIRE_EQUAL( displayedNumbers.size(), 1u );
    BOOST_CHECK_EQUAL( displayedNumbers.front(), wxS( "1" ) );
}


BOOST_AUTO_TEST_CASE( Cy8cproto040tDisplaysComponentPinNumbers )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign(
            corpusEnv, "CY8CPROTO-040T_PSoC_4000T_CapSense_Prototyping_Board_Schematic.DSN" );

    if( dsn.empty() )
        return;

    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    auto displayedPinNumbers = []( SCH_SYMBOL& aSymbol )
    {
        std::set<wxString> numbers;

        if( aSymbol.GetShowPinNumbers() )
        {
            for( const SCH_PIN* pin : aSymbol.GetPins() )
            {
                if( pin->IsVisible() && pin->GetNumberTextSize() > 0 )
                    numbers.insert( pin->GetNumber() );
            }
        }

        return numbers;
    };

    SCH_SYMBOL* j1 = nullptr;
    SCH_SYMBOL* u1 = nullptr;
    SCH_SYMBOL* j11 = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        if( !j1 )
            j1 = findConvertedSymbol( *path.LastScreen(), path, wxS( "J1" ) );

        if( !u1 )
            u1 = findConvertedSymbol( *path.LastScreen(), path, wxS( "U1" ) );

        if( !j11 )
            j11 = findConvertedSymbol( *path.LastScreen(), path, wxS( "J11" ) );
    }

    BOOST_REQUIRE( j1 );
    BOOST_REQUIRE( u1 );
    BOOST_REQUIRE( j11 );

    BOOST_CHECK( j1->GetShowPinNames() );
    BOOST_CHECK( j1->GetShowPinNumbers() );
    BOOST_REQUIRE_EQUAL( j1->GetPins().size(), 20u );

    std::set<wxString> j1PinText;

    for( const SCH_PIN* pin : j1->GetPins() )
    {
        BOOST_CHECK( !pin->GetName().IsEmpty() );
        BOOST_CHECK( !pin->GetNumber().IsEmpty() );
        BOOST_CHECK_GT( pin->GetNameTextSize(), 0 );
        BOOST_CHECK_GT( pin->GetNumberTextSize(), 0 );
        j1PinText.insert( pin->GetName() );
        j1PinText.insert( pin->GetNumber() );
    }

    for( const SCH_ITEM& item : j1->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_TEXT_T )
            BOOST_CHECK( !j1PinText.contains( static_cast<const SCH_TEXT&>( item ).GetText() ) );
    }

    BOOST_CHECK_EQUAL( displayedPinNumbers( *u1 ).size(), 25u );
    BOOST_CHECK_EQUAL( displayedPinNumbers( *j11 ).size(), 10u );
    BOOST_CHECK( displayedPinNumbers( *u1 ).contains( wxS( "23" ) ) );
    BOOST_CHECK( displayedPinNumbers( *u1 ).contains( wxS( "H" ) ) );
    BOOST_CHECK( displayedPinNumbers( *j11 ).contains( wxS( "1" ) ) );
    BOOST_CHECK( displayedPinNumbers( *j11 ).contains( wxS( "10" ) ) );
}


BOOST_AUTO_TEST_CASE( Cy8ckit149EmbeddedBlockDiagramIsComplete )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CY8CKIT-149 Schematic.DSN" );

    if( dsn.empty() )
        return;

    SETTINGS_MANAGER           manager;
    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    const wxImage* blockDiagram = nullptr;
    size_t         redShapesOnBlockDiagram = 0;
    size_t         elephantNotes = 0;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        if( path.Last()->GetName().Contains( wxS( "Block Diagram" ) ) )
        {
            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SHAPE_T ) )
            {
                const SCH_SHAPE& shape = static_cast<const SCH_SHAPE&>( *item );

                if( shape.GetStroke().GetColor() == KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 ) )
                    ++redShapesOnBlockDiagram;
            }
        }

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_TEXT_T ) )
        {
            const SCH_TEXT& text = static_cast<const SCH_TEXT&>( *item );

            if( text.GetText() == wxS( "*All Test Points are No Load" ) )
            {
                BOOST_REQUIRE( text.GetFont() );
                BOOST_CHECK_EQUAL( text.GetFont()->GetName(), wxS( "KiCad OrCAD Elephant" ) );
                int sourceBottom = path.Last()->GetName().Contains( wxS( "PSoC 4100S" ) ) ? 623 : 613;
                BOX2I ink = text.GetEffectiveTextShape( false, BOX2I(), ANGLE_0 )->BBox();
                ink.Offset( text.GetSchematicTextOffset( nullptr )
                            + text.GetOffsetToMatchSCH_FIELD( nullptr ) );
                BOOST_CHECK_SMALL( ink.GetY() - OrcadDbuToIu( 0, sourceBottom - 10 ).y,
                                   OrcadDbuToIu( 0, 1 ).y );
                BOOST_CHECK_SMALL( ink.GetBottom() - OrcadDbuToIu( 0, sourceBottom ).y,
                                   OrcadDbuToIu( 0, 1 ).y );
                ++elephantNotes;
            }
        }

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_BITMAP_T ) )
        {
            const wxImage* image = static_cast<SCH_BITMAP*>( item )->GetReferenceImage().GetImage().GetImageData();

            if( image && image->IsOk() && image->GetWidth() >= 2500 && image->GetHeight() >= 1000
                && ( !blockDiagram
                     || image->GetWidth() * image->GetHeight()
                                > blockDiagram->GetWidth() * blockDiagram->GetHeight() ) )
            {
                blockDiagram = image;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( blockDiagram, "CY8CKIT-149 block diagram was not rendered at full size" );
    BOOST_CHECK_EQUAL( redShapesOnBlockDiagram, 0u );
    BOOST_CHECK_EQUAL( elephantNotes, 2u );
    BOOST_CHECK( schematic->GetEmbeddedFiles()->HasFile( wxS( "KiCadOrCADElephant-Black.ttf" ) ) );
    BOOST_CHECK( schematic->GetAreFontsEmbedded() );

    std::array<size_t, 3> blueColumns{};
    std::array<size_t, 3> blueRows{};
    const int             width = blockDiagram->GetWidth();
    const int             height = blockDiagram->GetHeight();

    for( int y = 0; y < height; ++y )
    {
        for( int x = 0; x < width; ++x )
        {
            int red = blockDiagram->GetRed( x, y );
            int green = blockDiagram->GetGreen( x, y );
            int blue = blockDiagram->GetBlue( x, y );

            if( blue > red + 30 && blue > green + 20 )
            {
                ++blueColumns[std::min( 2, x * 3 / width )];
                ++blueRows[std::min( 2, y * 3 / height )];
            }
        }
    }

    const size_t minimumBluePixels = static_cast<size_t>( width ) * height / 500;

    BOOST_TEST_MESSAGE( "block diagram size=" << width << 'x' << height << " columns=" << blueColumns[0] << ','
                                                << blueColumns[1] << ',' << blueColumns[2] << " rows="
                                                << blueRows[0] << ',' << blueRows[1] << ',' << blueRows[2] );

    for( size_t count : blueColumns )
        BOOST_CHECK_GE( count, minimumBluePixels );

    for( size_t count : blueRows )
        BOOST_CHECK_GE( count, minimumBluePixels );
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
    for( const wxString& ref : { wxS( "R3186" ), wxS( "C2517" ), wxS( "R3189" ), wxS( "FB13" ), wxS( "FB9" ) } )
    {
        BOOST_REQUIRE_MESSAGE( symbols.count( ref ), ref );
        BOOST_CHECK( symbols[ref]->GetField( FIELD_T::REFERENCE )->GetDrawRotation() == ANGLE_HORIZONTAL );
    }

    // Value rotation is per-field from source: FB9 part number horizontal, C2517 "47pF" stays vertical.
    BOOST_CHECK( symbols[wxS( "FB9" )]->GetField( FIELD_T::VALUE )->GetDrawRotation() == ANGLE_HORIZONTAL );
    BOOST_CHECK( symbols[wxS( "C2517" )]->GetField( FIELD_T::VALUE )->GetDrawRotation() == ANGLE_VERTICAL );

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
                BOOST_CHECK( sym->GetField( FIELD_T::VALUE )->GetDrawRotation() == ANGLE_HORIZONTAL );
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


BOOST_AUTO_TEST_CASE( HierarchicalBusAliasesConnectWithoutHiddenLabels )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "buddy_sch_rev1.dsn" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "buddy_sch_rev1.dsn not present in corpus." );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    SCHEMATIC schematic( &manager.Prj() );
    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), &schematic );

    std::set<SCH_SCREEN*> visited;

    for( const SCH_SHEET_PATH& path : schematic.BuildSheetListSortedByPageNumbers() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !visited.insert( screen ).second )
            continue;

        std::vector<SCH_ITEM*> hidden;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() != SCH_LABEL_T && item->Type() != SCH_GLOBAL_LABEL_T )
                continue;

            SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

            if( label->GetTextColor() != KIGFX::COLOR4D::UNSPECIFIED && label->GetTextColor().a == 0 )
                hidden.push_back( item );
        }

        for( SCH_ITEM* item : hidden )
            screen->DeleteItem( item );
    }

    auto [consistent, checkable] =
            checkConnectivity( schematic, { { "U1.BB5", "J6.J30" }, { "U1.BA5", "J6.K31" } } );
    BOOST_CHECK_EQUAL( checkable, 2 );
    BOOST_CHECK_EQUAL( consistent, 2 );
}


BOOST_AUTO_TEST_CASE( EscapedHierarchicalBusMembersConnectWithoutGlobalHelpers )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "M5275EVB.DSN" );
    BOOST_REQUIRE_MESSAGE( !dsn.empty(), "M5275EVB.DSN not present in corpus." );

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    SCHEMATIC schematic( &manager.Prj() );
    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), &schematic );

    std::set<SCH_SCREEN*> visited;

    for( const SCH_SHEET_PATH& path : schematic.BuildSheetListSortedByPageNumbers() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !visited.insert( screen ).second )
            continue;

        std::vector<SCH_LABEL_BASE*> hidden;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

            if( label->GetTextColor() != KIGFX::COLOR4D::UNSPECIFIED && label->GetTextColor().a == 0 )
                hidden.push_back( label );
        }

        for( SCH_LABEL_BASE* label : hidden )
        {
            screen->Append( new SCH_LABEL( label->GetPosition(), label->GetText() ) );
            screen->DeleteItem( label );
        }
    }

    const std::vector<std::set<std::string>> expected = {
        { "J3.38", "RP49.7", "U6.D10" },
        { "J3.40", "RP49.5", "U6.D11" },
        { "J3.42", "RP49.3", "U6.D12" },
        { "J3.44", "RP49.1", "U6.D13" },
        { "J4.43", "RP16.4", "RP45.5", "RP9.8", "TP33.1", "U1.40", "U7.47" },
        { "J4.45", "RP16.8", "RP45.3", "RP9.6", "TP35.1", "U1.39", "U7.20" },
        { "J4.47", "RP9.4" },
        { "J4.49", "RP15.2", "RP9.2", "TP31.1", "U7.24" },
        { "J5.21", "R37.2", "RP50.1", "U6.G13" },
        { "J5.22", "RP47.3", "U6.E13" },
        { "J5.23", "RP50.3", "U6.H16" },
        { "J5.24", "RP47.1", "U2.6", "U6.F13" },
        { "J5.25", "RP50.5", "U6.H15" },
        { "J5.27", "RP50.7", "U6.H14" },
        { "J5.29", "J9.11", "RP48.3", "U6.J14" },
        { "J5.31", "RP48.5", "U6.J13", "U8.25" },
        { "J5.33", "RP48.7", "U6.K13", "U9.25" },
        { "J6.15", "RP47.7", "TP4.1", "U11.12", "U12.A8", "U2.1", "U6.R6" },
        { "J6.21", "RP47.5", "U1.6", "U2.3", "U6.N7" },
    };

    auto [consistent, checkable] = checkConnectivity( schematic, expected );
    BOOST_CHECK_EQUAL( checkable, expected.size() );
    BOOST_CHECK_EQUAL( consistent, expected.size() );
}


BOOST_AUTO_TEST_CASE( RepeatedSheetPinNamesDoNotJoinLocalRepairs )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
        return;

    const std::filesystem::path dsn = findCorpusDesign( corpusEnv, "HB1A-AAFM.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "HB1A-AAFM.DSN not present in corpus; skipping sheet-pin repair check." );
        return;
    }

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    SCHEMATIC schematic( &manager.Prj() );
    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), &schematic );
    schematic.ConnectionGraph()->Recalculate( schematic.BuildSheetListSortedByPageNumbers(), true );
    std::set<std::set<std::string>> partitions;

    for( const auto& [key, subgraphs] : schematic.ConnectionGraph()->GetNetMap() )
    {
        std::set<std::string> terminals;

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN* pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );
                wxString reference = symbol->GetRef( &subgraph->GetSheet(), false );

                if( !reference.IsEmpty() && !reference.StartsWith( wxS( "#" ) ) )
                {
                    terminals.insert( terminalToken( reference.ToStdString( wxConvUTF8 ),
                                                     pin->GetNumber().ToStdString( wxConvUTF8 ) ) );
                }
            }
        }

        partitions.insert( std::move( terminals ) );
    }

    // These distinct Capture clock nets share interface pin names across repeated ANEMONE instances.
    const std::map<std::string, std::set<std::string>> expected = {
        { "UL_UL_WW_LCLK_P", { terminalToken( "R278", "1" ), terminalToken( "U3", "F1" ),
                                terminalToken( "U3", "M1" ) } },
        { "UR_UL_WE_LCLK_P", { terminalToken( "R175", "1" ), terminalToken( "U3", "N18" ),
                                terminalToken( "U4", "F1" ) } }
    };

    for( const auto& [name, terminals] : expected )
    {
        BOOST_TEST_CONTEXT( name )
        {
            BOOST_CHECK( partitions.count( terminals ) == 1 );
        }
    }
}


BOOST_AUTO_TEST_CASE( WireFreePinJunctionsRetainVisibleLocalConnections )
{
    const char* corpus = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpus || !*corpus )
        return;

    std::filesystem::path dsn = findCorpusDesign( corpus, "1822A.DSN" );

    if( dsn.empty() )
        return;

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );
    SCHEMATIC schematic( &manager.Prj() );
    SCH_IO_ORCAD plugin;
    BOOST_REQUIRE_NO_THROW( plugin.LoadSchematicFile( dsn.string(), &schematic ) );
    bool found = false;

    for( const SCH_SHEET_PATH& path : schematic.BuildSheetListSortedByPageNumbers() )
    {
        SCH_SYMBOL* symbol = findConvertedSymbol( *path.LastScreen(), path, wxS( "COUT1" ) );

        if( !symbol )
            continue;

        for( SCH_PIN* pin : symbol->GetPins( &path ) )
        {
            if( pin->GetNumber() != wxS( "3" ) )
                continue;

            found = true;
            bool labelled = false;
            bool junction = false;

            for( SCH_ITEM* item : path.LastScreen()->Items() )
            {
                if( auto* wire = dynamic_cast<SCH_LINE*>( item ); wire && wire->GetLayer() == LAYER_WIRE )
                    BOOST_CHECK( !wire->GetSeg().Contains( pin->GetPosition() ) );

                if( item->GetPosition() != pin->GetPosition() )
                    continue;

                junction |= item->Type() == SCH_JUNCTION_T;

                if( auto* label = dynamic_cast<SCH_LABEL*>( item ) )
                {
                    labelled |= label->GetText() == wxS( "Agnd" );
                    BOOST_CHECK( label->GetTextColor() == KIGFX::COLOR4D::UNSPECIFIED
                                 || label->GetTextColor().a > 0 );
                }
            }

            BOOST_CHECK( junction );
            BOOST_CHECK( labelled );
        }
    }

    BOOST_CHECK( found );

    for( const wxString& reference : { wxString( "COUT1" ), wxString( "COUT2" ), wxString( "COUT3" ) } )
        BOOST_CHECK_EQUAL( terminalNetName( schematic, reference, wxS( "3" ) ), wxString( "Agnd" ) );
}


BOOST_AUTO_TEST_SUITE_END()
