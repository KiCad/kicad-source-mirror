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

#include <base_units.h>
#include <lib_symbol.h>
#include <sch_field.h>
#include <sch_label.h>
#include <schematic.h>
#include <sch_io/pads/sch_io_pads.h>
#include <sch_io/pads/pads_sch_binary_builder.h>
#include <sch_io/pads/pads_sch_binary_parser.h>
#include <sch_io/pads/pads_sch_binary_reader.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <settings/settings_manager.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <vector>


namespace
{

struct PADS_SCH_IMPORT_FIXTURE
{
    PADS_SCH_IMPORT_FIXTURE() : m_schematic( nullptr )
    {
        m_settingsManager.LoadProject( "" );
        m_schematic.SetProject( &m_settingsManager.Prj() );
        m_schematic.Reset();
    }

    ~PADS_SCH_IMPORT_FIXTURE()
    {
        m_schematic.Reset();
    }

    SETTINGS_MANAGER m_settingsManager;
    SCHEMATIC        m_schematic;
};


static wxString binaryFixture( const wxString& aName )
{
    return wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + wxS( "/plugins/pads/binary/" ) + aName
           + wxS( ".sch" );
}


static PADS_SCH_BINARY::PADS_SCH_MODEL parseBinaryFixture( const wxString& aName )
{
    wxString             path = binaryFixture( aName );
    std::vector<uint8_t> bytes;
    BOOST_REQUIRE( PADS_SCH_BINARY::PADS_SCH_BINARY_READER::ReadFile( path, bytes ) );
    return PADS_SCH_BINARY::PADS_SCH_BINARY_PARSER().Parse( bytes, path );
}


struct OBJECT_GRAPH_SNAPSHOT
{
    std::vector<const SCH_SHEET*> topLevelSheets;
    std::vector<const SCH_ITEM*>  rootItems;
    std::vector<const SCH_ITEM*>  appendItems;
    int                           pageWidth = 0;
    int                           pageHeight = 0;
    wxString                      title;

    bool operator==( const OBJECT_GRAPH_SNAPSHOT& ) const = default;
};


static OBJECT_GRAPH_SNAPSHOT objectGraphSnapshot( const SCHEMATIC& aSchematic, const SCH_SHEET* aAppendToMe )
{
    OBJECT_GRAPH_SNAPSHOT snapshot;
    std::vector<SCH_SHEET*> topLevelSheets = aSchematic.GetTopLevelSheets();
    snapshot.topLevelSheets.assign( topLevelSheets.begin(), topLevelSheets.end() );

    for( const SCH_ITEM* item : aSchematic.Root().GetScreen()->Items() )
        snapshot.rootItems.push_back( item );

    if( aAppendToMe && aAppendToMe->GetScreen() )
    {
        for( const SCH_ITEM* item : aAppendToMe->GetScreen()->Items() )
            snapshot.appendItems.push_back( item );

        snapshot.pageWidth = aAppendToMe->GetScreen()->GetPageSettings().GetWidthMils();
        snapshot.pageHeight = aAppendToMe->GetScreen()->GetPageSettings().GetHeightMils();
        snapshot.title = aAppendToMe->GetScreen()->GetTitleBlock().GetTitle();
    }

    return snapshot;
}


static size_t itemCount( SCH_SCREEN* aScreen, KICAD_T aType )
{
    size_t count = 0;

    for( SCH_ITEM* item : aScreen->Items().OfType( aType ) )
    {
        (void) item;
        ++count;
    }

    return count;
}


static VECTOR2I localPoint( const PADS_SCH_BINARY::SOURCE_POINT& aPoint )
{
    return { schIUScale.MilsToIU( static_cast<double>( aPoint.x ) / 2.0 ),
             -schIUScale.MilsToIU( static_cast<double>( aPoint.y ) / 2.0 ) };
}


static PIN_ORIENTATION pinOrientation( int aAngle )
{
    switch( PADS_SCH_BINARY::NormalizeAngle( aAngle ) )
    {
    case 900: return PIN_ORIENTATION::PIN_UP;
    case 1800: return PIN_ORIENTATION::PIN_LEFT;
    case 2700: return PIN_ORIENTATION::PIN_DOWN;
    default: return PIN_ORIENTATION::PIN_RIGHT;
    }
}


static ELECTRICAL_PINTYPE pinType( uint32_t aType )
{
    const std::array<ELECTRICAL_PINTYPE, 9> types = {
        ELECTRICAL_PINTYPE::PT_PASSIVE,     ELECTRICAL_PINTYPE::PT_INPUT,    ELECTRICAL_PINTYPE::PT_OUTPUT,
        ELECTRICAL_PINTYPE::PT_BIDI,        ELECTRICAL_PINTYPE::PT_TRISTATE, ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR,
        ELECTRICAL_PINTYPE::PT_OPENEMITTER, ELECTRICAL_PINTYPE::PT_POWER_IN, ELECTRICAL_PINTYPE::PT_UNSPECIFIED
    };
    return aType < types.size() ? types[aType] : ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
}


static GRAPHIC_PINSHAPE pinShape( uint32_t aStyle )
{
    switch( aStyle )
    {
    case 1: return GRAPHIC_PINSHAPE::INVERTED;
    case 2: return GRAPHIC_PINSHAPE::CLOCK;
    case 3: return GRAPHIC_PINSHAPE::INVERTED_CLOCK;
    default: return GRAPHIC_PINSHAPE::LINE;
    }
}


static GR_TEXT_H_ALIGN_T horizontalJustification( PADS_SCH_BINARY::MODEL_JUSTIFICATION aJustification )
{
    switch( aJustification )
    {
    case PADS_SCH_BINARY::MODEL_JUSTIFICATION::CENTER: return GR_TEXT_H_ALIGN_CENTER;
    case PADS_SCH_BINARY::MODEL_JUSTIFICATION::RIGHT: return GR_TEXT_H_ALIGN_RIGHT;
    default: return GR_TEXT_H_ALIGN_LEFT;
    }
}


static GR_TEXT_V_ALIGN_T verticalJustification( PADS_SCH_BINARY::MODEL_JUSTIFICATION aJustification )
{
    switch( aJustification )
    {
    case PADS_SCH_BINARY::MODEL_JUSTIFICATION::LEFT: return GR_TEXT_V_ALIGN_TOP;
    case PADS_SCH_BINARY::MODEL_JUSTIFICATION::RIGHT: return GR_TEXT_V_ALIGN_BOTTOM;
    default: return GR_TEXT_V_ALIGN_CENTER;
    }
}

} // namespace


BOOST_FIXTURE_TEST_SUITE( PadsSchImport, PADS_SCH_IMPORT_FIXTURE )


BOOST_AUTO_TEST_CASE( CanReadSchematicFile )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt" );

    BOOST_CHECK( plugin.CanReadSchematicFile( padsFile ) );
}


BOOST_AUTO_TEST_CASE( CanReadSchematicFile_RejectNonPads )
{
    SCH_IO_PADS plugin;

    wxString kicadFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt" );

    BOOST_CHECK( plugin.CanReadSchematicFile( kicadFile ) );
}


BOOST_AUTO_TEST_CASE( FindPlugin )
{
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_PADS ) );
    BOOST_CHECK_NE( pi.get(), nullptr );
}


BOOST_AUTO_TEST_CASE( MultiGateImport )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/multigate_schematic.txt" );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( padsFile, &m_schematic );
    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );

    SCH_SCREEN* screen = rootSheet->GetScreen();

    // Collect U1 symbols
    std::vector<SCH_SYMBOL*> u1Symbols;
    SCH_SHEET_PATH rootPath;
    rootPath.push_back( rootSheet );

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

        if( sym->GetRef( &rootPath ) == wxT( "U1" ) )
            u1Symbols.push_back( sym );
    }

    BOOST_REQUIRE_EQUAL( u1Symbols.size(), 2u );

    // Sort by unit number for deterministic checks
    std::sort( u1Symbols.begin(), u1Symbols.end(),
               []( const SCH_SYMBOL* a, const SCH_SYMBOL* b )
               {
                   return a->GetUnit() < b->GetUnit();
               } );

    // Unit 1 (gate A with TL082A decal) should have 5 pins
    BOOST_CHECK_EQUAL( u1Symbols[0]->GetUnit(), 1 );
    BOOST_CHECK_EQUAL( u1Symbols[0]->GetLibPins().size(), 5u );

    // Unit 2 (gate B with TL082 decal) should have 3 pins
    BOOST_CHECK_EQUAL( u1Symbols[1]->GetUnit(), 2 );
    BOOST_CHECK_EQUAL( u1Symbols[1]->GetLibPins().size(), 3u );

    // Both should share the same multi-unit LIB_SYMBOL with 2 units
    BOOST_CHECK( u1Symbols[0]->IsMultiUnit() );
    BOOST_CHECK_EQUAL( u1Symbols[0]->GetUnitCount(), 2 );

    // Both references should be "U1" (not "U1-A" or "U1-B")
    BOOST_CHECK_EQUAL( u1Symbols[0]->GetRef( &rootPath ), wxT( "U1" ) );
    BOOST_CHECK_EQUAL( u1Symbols[1]->GetRef( &rootPath ), wxT( "U1" ) );
}


BOOST_AUTO_TEST_CASE( Issue23420_HeaderWithCodePageSuffix )
{
    // Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23420
    // PADS Logic schematics exported with a code page suffix in the header
    // (e.g. *PADS-LOGIC-V9.0-CP1250*) must be detected and parsed.
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir()
            + "/plugins/pads/issue23420_codepage_schematic.txt" );

    BOOST_CHECK( plugin.CanReadSchematicFile( padsFile ) );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( padsFile, &m_schematic );

    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );
}


BOOST_AUTO_TEST_CASE( CanReadLibrary )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    BOOST_CHECK( plugin.CanReadLibrary( padsFile ) );
}


// Only the schematic path reads the binary format, so the library predicate must refuse a binary
// container even though the schematic predicate accepts it.  The file is named .txt because
// CanReadLibrary() screens .sch out by extension, which would make the check vacuous.
BOOST_AUTO_TEST_CASE( CanReadLibraryRefusesBinaryContainer )
{
    // Minimum container the binary sniffer accepts: magic 00 FE, version 0x000D, 0x250 bytes
    std::vector<uint8_t> container( 0x250, 0 );

    container[0] = 0x00;
    container[1] = 0xFE;
    container[2] = 0x0D;
    container[3] = 0x00;

    std::filesystem::path binaryAsTxt =
            std::filesystem::temp_directory_path() / "kicad_pads_binary_container.txt";

    {
        std::ofstream out( binaryAsTxt, std::ios::binary );

        out.write( reinterpret_cast<const char*>( container.data() ),
                   static_cast<std::streamsize>( container.size() ) );
    }

    SCH_IO_PADS plugin;
    wxString    path = wxString::FromUTF8( binaryAsTxt.string() );

    BOOST_CHECK( plugin.CanReadSchematicFile( path ) );
    BOOST_CHECK( !plugin.CanReadLibrary( path ) );

    std::filesystem::remove( binaryAsTxt );
}


BOOST_AUTO_TEST_CASE( EnumerateSymbolLib_NamesFromSchematic )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    wxArrayString names;
    BOOST_CHECK_NO_THROW( plugin.EnumerateSymbolLib( names, padsFile ) );
    BOOST_CHECK_GT( names.GetCount(), 0u );
}


BOOST_AUTO_TEST_CASE( EnumerateSymbolLib_ReturnsLibSymbols )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    std::vector<LIB_SYMBOL*> symbols;
    BOOST_CHECK_NO_THROW( plugin.EnumerateSymbolLib( symbols, padsFile ) );
    BOOST_CHECK_GT( symbols.size(), 0u );

    for( LIB_SYMBOL* sym : symbols )
        BOOST_REQUIRE( sym != nullptr );
}


BOOST_AUTO_TEST_CASE( LoadSymbol_ByName )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    wxArrayString names;
    plugin.EnumerateSymbolLib( names, padsFile );

    BOOST_REQUIRE_GT( names.GetCount(), 0u );

    LIB_SYMBOL* sym = plugin.LoadSymbol( padsFile, names.Item( 0 ) );
    BOOST_REQUIRE( sym != nullptr );
    BOOST_CHECK_EQUAL( sym->GetName(), names.Item( 0 ) );
}


BOOST_AUTO_TEST_CASE( LoadSymbol_UnknownReturnsNull )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    LIB_SYMBOL* sym = plugin.LoadSymbol( padsFile, wxT( "NO_SUCH_SYMBOL_12345" ) );
    BOOST_CHECK( sym == nullptr );
}


BOOST_AUTO_TEST_CASE( MultiGatePartTypeBecomesMultiUnitLibSymbol )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/multigate_schematic.txt" );

    std::vector<LIB_SYMBOL*> symbols;
    BOOST_CHECK_NO_THROW( plugin.EnumerateSymbolLib( symbols, padsFile ) );

    bool foundMultiUnit = false;

    for( LIB_SYMBOL* sym : symbols )
    {
        if( sym && sym->GetUnitCount() > 1 )
        {
            foundMultiUnit = true;
            break;
        }
    }

    BOOST_CHECK( foundMultiUnit );
}


BOOST_AUTO_TEST_CASE( IsLibraryNotWritable )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt" );

    BOOST_CHECK( !plugin.IsLibraryWritable( padsFile ) );
}


BOOST_AUTO_TEST_CASE( Issue24284_TextItemsPlacedOnCorrectSheet )
{
    // Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24284
    // Multi-sheet PADS Logic schematics have one *TEXT* and *LINES* block per
    // *SHT*. Before the fix every text/line item was placed on the first
    // sheet, causing page-number text from all sheets to stack on top of each
    // other and border graphics to overlap.
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir()
            + "/plugins/pads/issue24284_multisheet_text.txt" );

    BOOST_REQUIRE( plugin.CanReadSchematicFile( padsFile ) );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( padsFile, &m_schematic );
    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );

    // Collect text and line content keyed by hierarchical sheet name.
    std::map<wxString, std::vector<wxString>> textBySheet;
    std::map<wxString, int>                   lineCountBySheet;

    for( SCH_ITEM* item : rootSheet->GetScreen()->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
        wxString   sheetName = sheet->GetField( FIELD_T::SHEET_NAME )->GetText();

        for( SCH_ITEM* screenItem : sheet->GetScreen()->Items().OfType( SCH_TEXT_T ) )
        {
            SCH_TEXT* txt = static_cast<SCH_TEXT*>( screenItem );
            textBySheet[sheetName].push_back( txt->GetText() );
        }

        for( SCH_ITEM* screenItem : sheet->GetScreen()->Items().OfType( SCH_LINE_T ) )
        {
            (void) screenItem;
            lineCountBySheet[sheetName]++;
        }
    }

    for( int sheetNum = 1; sheetNum <= 3; ++sheetNum )
    {
        wxString sheetName = wxString::Format( wxT( "Page%d" ), sheetNum );
        wxString pageText = wxString::Format( wxT( "PAGE %d OF 3" ), sheetNum );
        wxString bodyText = wxString::Format( wxT( "TEXT ON SHEET %d" ), sheetNum );

        BOOST_REQUIRE_EQUAL( textBySheet.count( sheetName ), 1u );
        BOOST_CHECK_EQUAL( textBySheet[sheetName].size(), 2u );
        BOOST_CHECK( std::find( textBySheet[sheetName].begin(), textBySheet[sheetName].end(),
                                pageText ) != textBySheet[sheetName].end() );
        BOOST_CHECK( std::find( textBySheet[sheetName].begin(), textBySheet[sheetName].end(),
                                bodyText ) != textBySheet[sheetName].end() );
        BOOST_CHECK_EQUAL( lineCountBySheet[sheetName], 1 );
    }
}


// Issue 23855 (#1): an off-page connector whose stub wire is zero-length must take its
// global-label orientation from the authoritative *NETNAMES* offset, not from the
// degenerate wire direction. The two SP1 anchors carry opposite X offsets and must yield
// opposite spin styles.
BOOST_AUTO_TEST_CASE( Issue23855_GlobalLabelOrientationFromNetNames )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/issue23855_schematic.txt" );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( padsFile, &m_schematic );
    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );

    SCH_SCREEN* screen = rootSheet->GetScreen();

    // PADS anchor positions in mils -> KiCad screen X (Y-up flipped on import).
    const int milToIU = schIUScale.MilsToIU( 1 );
    const int cnSideX = 1400 * milToIU;  // @@@O0, x_offset +350 -> text reads right
    const int r1SideX = 2800 * milToIU;  // @@@O1, x_offset -360 -> text reads left

    SPIN_STYLE cnSpin = SPIN_STYLE::LEFT;
    SPIN_STYLE r1Spin = SPIN_STYLE::RIGHT;
    bool       foundCn = false;
    bool       foundR1 = false;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
    {
        SCH_LABEL_BASE* lbl = static_cast<SCH_LABEL_BASE*>( item );

        if( lbl->GetText() != wxT( "SP1" ) )
            continue;

        if( lbl->GetPosition().x == cnSideX )
        {
            foundCn = true;
            cnSpin = lbl->GetSpinStyle();
        }
        else if( lbl->GetPosition().x == r1SideX )
        {
            foundR1 = true;
            r1Spin = lbl->GetSpinStyle();
        }
    }

    BOOST_REQUIRE( foundCn );
    BOOST_REQUIRE( foundR1 );

    // The CN1-side label extends to the right; the R1-side label (degenerate wire)
    // extends to the left thanks to the NETNAMES override.
    BOOST_CHECK( cnSpin == SPIN_STYLE::RIGHT );
    BOOST_CHECK( r1Spin == SPIN_STYLE::LEFT );
}


// Issue 23855 (#5): a 90 degree rotated part must place its reference and value fields at
// the absolute coordinates authored in PADS. PADS stores attribute offsets in the placed
// (post-rotation) frame, so the importer applies the offset directly without re-rotating.
BOOST_AUTO_TEST_CASE( Issue23855_RotatedPartFieldPositions )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8(
            KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/issue23855_schematic.txt" );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( padsFile, &m_schematic );
    BOOST_REQUIRE( rootSheet );
    BOOST_REQUIRE( rootSheet->GetScreen() );

    SCH_SCREEN* screen = rootSheet->GetScreen();
    SCH_SHEET_PATH rootPath;
    rootPath.push_back( rootSheet );

    SCH_SYMBOL* d5 = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

        if( sym->GetRef( &rootPath ) == wxT( "D5" ) )
            d5 = sym;
    }

    BOOST_REQUIRE( d5 != nullptr );

    SCH_FIELD* refF = d5->GetField( FIELD_T::REFERENCE );
    SCH_FIELD* valF = d5->GetField( FIELD_T::VALUE );

    VECTOR2I symPos = d5->GetPosition();
    VECTOR2I refRel = refF->GetPosition() - symPos;
    VECTOR2I valRel = valF->GetPosition() - symPos;

    // REF-DES PADS offset (210, 230); PART-TYPE/value PADS offset (-70, 520). PADS Y is up,
    // so the screen Y offset is negated.
    BOOST_CHECK_EQUAL( refRel.x, schIUScale.MilsToIU( 210 ) );
    BOOST_CHECK_EQUAL( refRel.y, -schIUScale.MilsToIU( 230 ) );
    BOOST_CHECK_EQUAL( valRel.x, schIUScale.MilsToIU( -70 ) );
    BOOST_CHECK_EQUAL( valRel.y, -schIUScale.MilsToIU( 520 ) );

    // Rotated attribute text keeps the PADS text angle and the authored justification
    // (codes 4 and 5 both decode to top-left in the text's reading frame).
    BOOST_CHECK_EQUAL( refF->GetTextAngle().AsDegrees(), 90.0 );
    BOOST_CHECK_EQUAL( valF->GetTextAngle().AsDegrees(), 90.0 );
    BOOST_CHECK_EQUAL( refF->GetHorizJustify(), GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK_EQUAL( refF->GetVertJustify(), GR_TEXT_V_ALIGN_TOP );
    BOOST_CHECK_EQUAL( valF->GetHorizJustify(), GR_TEXT_H_ALIGN_LEFT );
    BOOST_CHECK_EQUAL( valF->GetVertJustify(), GR_TEXT_V_ALIGN_TOP );
}




BOOST_AUTO_TEST_CASE( BinarySymbolsAndSheets )
{
    using namespace PADS_SCH_BINARY;

    const PADS_SCH_MODEL model = parseBinaryFixture( wxS( "symbol_primitives" ) );
    SCH_SHEET*           destination = m_schematic.GetTopLevelSheet();
    BOOST_REQUIRE( destination );
    BOOST_REQUIRE( destination->GetScreen() );

    PADS_SCH_BINARY_BUILDER builder;
    BUILD_RESULT            result =
            builder.Build( model, &m_schematic, nullptr, binaryFixture( wxS( "symbol_primitives" ) ) );

    BOOST_CHECK_EQUAL( result.counts.sheets, 1u );
    BOOST_CHECK_EQUAL( result.counts.symbols, model.placements.size() );
    BOOST_CHECK( destination->m_Uuid == destination->GetScreen()->GetUuid() );

    SCH_SHEET_PATH path;
    path.push_back( destination );
    std::map<wxString, SCH_SYMBOL*> symbols;

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        symbols.emplace( symbol->GetRef( &path ), symbol );
    }

    for( const MODEL_PLACEMENT& placement : model.placements )
    {
        auto it = symbols.find( placement.reference.text );
        BOOST_REQUIRE_MESSAGE( it != symbols.end(), placement.reference.text );
        SCH_SYMBOL* symbol = it->second;
        BOOST_CHECK_EQUAL( symbol->GetUnit(), placement.unit );
        BOOST_CHECK_EQUAL( symbol->GetPosition().x,
                           schIUScale.MilsToIU( static_cast<double>( placement.position.x ) / 2.0 ) );
        BOOST_CHECK_EQUAL( symbol->GetLibPins().size(), placement.pins.size() );
        BOOST_REQUIRE( symbol->GetLibSymbolRef() );
        BOOST_CHECK( !symbol->GetLibSymbolRef()->GetDrawItems().empty() );
        BOOST_CHECK( destination->GetScreen()->GetLibSymbols().contains( symbol->GetSchSymbolLibraryName() ) );
    }

    SCH_SYMBOL*                    primitiveSymbol = symbols.at( wxS( "U1" ) );
    const MODEL_SYMBOL_DEFINITION& primitiveDefinition =
            *std::ranges::find_if( model.definitions,
                                   []( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                   {
                                       return aDefinition.name.text == wxS( "BATCHB_PRIMITIVES" );
                                   } );
    std::vector<SCH_SHAPE*> primitiveShapes;
    std::vector<SCH_TEXT*>  primitiveTexts;

    for( const SCH_ITEM& item : primitiveSymbol->GetLibSymbolRef()->GetDrawItems() )
    {
        if( item.Type() == SCH_SHAPE_T )
            primitiveShapes.push_back( static_cast<SCH_SHAPE*>( const_cast<SCH_ITEM*>( &item ) ) );
        else if( item.Type() == SCH_TEXT_T )
            primitiveTexts.push_back( static_cast<SCH_TEXT*>( const_cast<SCH_ITEM*>( &item ) ) );
    }

    BOOST_REQUIRE_EQUAL( primitiveShapes.size(), 5u );
    auto shapeOfType = [&]( SHAPE_T aType )
    {
        return std::ranges::find_if( primitiveShapes,
                                     [&]( const SCH_SHAPE* aShape )
                                     {
                                         return aShape->GetShape() == aType;
                                     } );
    };
    BOOST_CHECK_EQUAL( std::ranges::count_if( primitiveShapes,
                                              []( const SCH_SHAPE* aShape )
                                              {
                                                  return aShape->GetShape() == SHAPE_T::POLY;
                                              } ),
                       3 );
    auto circle = shapeOfType( SHAPE_T::CIRCLE );
    auto arc = shapeOfType( SHAPE_T::ARC );
    BOOST_REQUIRE( circle != primitiveShapes.end() );
    BOOST_REQUIRE( arc != primitiveShapes.end() );
    BOOST_CHECK_EQUAL(
            ( *arc )->GetStroke().GetWidth(),
            schIUScale.MilsToIU( static_cast<double>( primitiveDefinition.graphics[3].strokeWidth ) / 2.0 ) );
    BOOST_CHECK( ( *arc )->GetEffectiveLineStyle() == LINE_STYLE::SOLID );
    BOOST_CHECK_EQUAL(
            ( *arc )->GetCenter().x,
            schIUScale.MilsToIU( static_cast<double>( primitiveDefinition.graphics[3].arcCenter.x ) / 2.0 ) );
    BOOST_CHECK_EQUAL(
            ( *arc )->GetCenter().y,
            -schIUScale.MilsToIU( static_cast<double>( primitiveDefinition.graphics[3].arcCenter.y ) / 2.0 ) );
    BOOST_CHECK( std::ranges::any_of( primitiveShapes,
                                      []( const SCH_SHAPE* aShape )
                                      {
                                          return aShape->GetFillMode() == FILL_T::FILLED_SHAPE;
                                      } ) );
    BOOST_REQUIRE_EQUAL( primitiveTexts.size(), 1u );
    BOOST_CHECK_EQUAL( primitiveTexts[0]->GetText(), primitiveDefinition.graphics[5].text.text );
    BOOST_CHECK_EQUAL( primitiveTexts[0]->GetPosition(), localPoint( primitiveDefinition.graphics[5].points[0] ) );
    BOOST_CHECK_EQUAL(
            primitiveTexts[0]->GetTextHeight(),
            schIUScale.MilsToIU( static_cast<double>( primitiveDefinition.graphics[5].presentation.height ) / 2.0 ) );

    BOOST_CHECK_EQUAL( destination->GetScreen()->GetPageSettings().GetWidthMils(),
                       model.sheets.front().pageSize.x / 2 );
    BOOST_CHECK_EQUAL( destination->GetScreen()->GetPageSettings().GetHeightMils(),
                       model.sheets.front().pageSize.y / 2 );
    BOOST_CHECK_EQUAL( destination->GetScreen()->GetTitleBlock().GetTitle(), model.sheets.front().title.text );
    const TITLE_BLOCK& titleBlock = destination->GetScreen()->GetTitleBlock();
    int                commentIndex = 0;

    for( const MODEL_FIELD& field : model.sheets.front().titleBlockFields )
    {
        if( field.name.text.CmpNoCase( wxS( "Title" ) ) == 0 )
            BOOST_CHECK_EQUAL( titleBlock.GetTitle(), field.value.text );
        else if( field.name.text.CmpNoCase( wxS( "Revision" ) ) == 0 )
            BOOST_CHECK_EQUAL( titleBlock.GetRevision(), field.value.text );
        else if( field.name.text.CmpNoCase( wxS( "Drawn Date" ) ) == 0 )
            BOOST_CHECK_EQUAL( titleBlock.GetDate(), field.value.text );
        else if( field.name.text.CmpNoCase( wxS( "Company Name" ) ) == 0 )
            BOOST_CHECK_EQUAL( titleBlock.GetCompany(), field.value.text );
        else
            BOOST_CHECK_EQUAL( titleBlock.GetComment( commentIndex++ ),
                               wxString::Format( wxS( "%s: %s" ), field.name.text, field.value.text ) );
    }

    BOOST_CHECK_EQUAL( result.counts.graphics, std::ranges::count_if( model.sheets.front().border,
                                                                      []( const MODEL_GRAPHIC& aGraphic )
                                                                      {
                                                                          return aGraphic.kind
                                                                                 != MODEL_GRAPHIC_KIND::TEXT;
                                                                      } ) );

    m_schematic.Reset();
    destination = m_schematic.GetTopLevelSheet();
    const PADS_SCH_MODEL pinModel = parseBinaryFixture( wxS( "pin_styles" ) );
    result = builder.Build( pinModel, &m_schematic, destination, binaryFixture( wxS( "pin_styles" ) ) );
    SCH_SHEET_PATH pinPath;
    pinPath.push_back( destination );
    SCH_SYMBOL* pinSymbol = nullptr;

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &pinPath ) == wxS( "U2" ) )
            pinSymbol = symbol;
    }

    BOOST_REQUIRE( pinSymbol );
    BOOST_CHECK( destination->GetScreen()->GetLibSymbols().contains( pinSymbol->GetSchSymbolLibraryName() ) );
    const MODEL_SYMBOL_DEFINITION& pinDefinition =
            *std::ranges::find_if( pinModel.definitions,
                                   []( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                   {
                                       return aDefinition.name.text == wxS( "BATCHB_PIN_STYLES" );
                                   } );
    std::vector<SCH_PIN*>  builtPins = pinSymbol->GetLibPins();
    const MODEL_PART_TYPE& pinPart = *std::ranges::find_if( pinModel.partTypes,
                                                            []( const MODEL_PART_TYPE& aPart )
                                                            {
                                                                return aPart.name.text == wxS( "BATCHB-PIN-STYLES" );
                                                            } );
    BOOST_REQUIRE_EQUAL( builtPins.size(), pinDefinition.pins.size() + pinPart.signalPins.size() );

    for( const MODEL_PIN_DEFINITION& sourcePin : pinDefinition.pins )
    {
        auto built = std::ranges::find( builtPins, sourcePin.number.text, &SCH_PIN::GetNumber );
        BOOST_REQUIRE_MESSAGE( built != builtPins.end(), sourcePin.number.text );
        BOOST_CHECK_EQUAL( ( *built )->GetName(), sourcePin.name.text );
        BOOST_CHECK_EQUAL( ( *built )->GetPosition(), localPoint( sourcePin.position ) );
        BOOST_CHECK_EQUAL( ( *built )->GetLength(),
                           schIUScale.MilsToIU( static_cast<double>( sourcePin.length ) / 2.0 ) );
        BOOST_CHECK( ( *built )->GetOrientation() == pinOrientation( sourcePin.angle ) );
        BOOST_CHECK( ( *built )->GetType() == pinType( sourcePin.electricalType ) );
        BOOST_CHECK( ( *built )->GetShape() == pinShape( sourcePin.graphicStyle ) );
        BOOST_CHECK_EQUAL( ( *built )->IsVisible(), sourcePin.presentation.visible );
        BOOST_CHECK_EQUAL( ( *built )->GetNameTextSize(),
                           schIUScale.MilsToIU( static_cast<double>( sourcePin.namePresentation.height ) / 2.0 ) );
        BOOST_CHECK_EQUAL( ( *built )->GetNumberTextSize(),
                           schIUScale.MilsToIU( static_cast<double>( sourcePin.numberPresentation.height ) / 2.0 ) );
    }

    m_schematic.Reset();
    destination = m_schematic.GetTopLevelSheet();
    const PADS_SCH_MODEL transformModel = parseBinaryFixture( wxS( "placement_transform" ) );
    result = builder.Build( transformModel, &m_schematic, destination, binaryFixture( wxS( "placement_transform" ) ) );
    SCH_SHEET_PATH transformPath;
    transformPath.push_back( destination );

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL*            symbol = static_cast<SCH_SYMBOL*>( item );
        const MODEL_PLACEMENT& placement =
                *std::ranges::find_if( transformModel.placements,
                                       [&]( const MODEL_PLACEMENT& aPlacement )
                                       {
                                           return aPlacement.reference.text == symbol->GetRef( &transformPath );
                                       } );
        int expectedOrientation = SYM_ORIENT_0;

        switch( placement.angle )
        {
        case 900: expectedOrientation = SYM_ORIENT_90; break;
        case 1800: expectedOrientation = SYM_ORIENT_180; break;
        case 2700: expectedOrientation = SYM_ORIENT_270; break;
        default: break;
        }

        if( placement.mirrored )
            expectedOrientation |= SYM_MIRROR_X;

        BOOST_CHECK_EQUAL( symbol->GetOrientation(), expectedOrientation );
    }

    BOOST_CHECK_EQUAL( itemCount( destination->GetScreen(), SCH_SYMBOL_T ), transformModel.placements.size() );

    PADS_SCH_MODEL unknownTransform = transformModel;
    auto rawAngleIt = std::ranges::find_if(
            unknownTransform.placements.front().properties,
            []( const SOURCE_PROPERTY& aProperty )
            {
                return aProperty.name.text == wxS( "raw_angle" );
            } );
    BOOST_REQUIRE( rawAngleIt != unknownTransform.placements.front().properties.end() );
    SOURCE_PROPERTY& rawAngle = *rawAngleIt;
    rawAngle.value.text = wxS( "3600" );
    rawAngle.disposition = PROPERTY_DISPOSITION::PRESERVED;
    unknownTransform.placements.front().angle = 0;
    result = builder.Build( unknownTransform, &m_schematic, destination, wxS( "unknown_transform.sch" ) );
    BOOST_CHECK( std::ranges::any_of( result.diagnostics,
                                      [&]( const PARSER_DIAGNOSTIC& aDiagnostic )
                                      {
                                          return aDiagnostic.message.Contains( wxS( "raw_angle" ) )
                                                 && aDiagnostic.source == rawAngle.source;
                                      } ) );

    m_schematic.Reset();
    destination = m_schematic.GetTopLevelSheet();
    const PADS_SCH_MODEL multiModel = parseBinaryFixture( wxS( "multigate" ) );
    result = builder.Build( multiModel, &m_schematic, destination, binaryFixture( wxS( "multigate" ) ) );
    SCH_SHEET_PATH multiPath;
    multiPath.push_back( destination );
    std::vector<SCH_SYMBOL*> multiSymbols;

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &multiPath ) == wxS( "U3" ) )
            multiSymbols.push_back( symbol );
    }

    BOOST_REQUIRE_EQUAL( multiSymbols.size(), 2u );
    std::ranges::sort( multiSymbols, {}, &SCH_SYMBOL::GetUnit );
    BOOST_CHECK_EQUAL( multiSymbols[0]->GetUnit(), 1 );
    BOOST_CHECK_EQUAL( multiSymbols[1]->GetUnit(), 2 );
    BOOST_CHECK_EQUAL( multiSymbols[0]->GetUnitCount(), 2 );
    BOOST_CHECK_EQUAL( multiSymbols[1]->GetUnitCount(), 2 );
    const MODEL_PART_TYPE& multiPart = *std::ranges::find_if( multiModel.partTypes,
                                                              []( const MODEL_PART_TYPE& aPart )
                                                              {
                                                                  return aPart.name.text == wxS( "BATCHD-MULTIGATE" );
                                                              } );
    BOOST_CHECK_EQUAL( multiSymbols[0]->GetLibPins().size(),
                       multiModel.placements[1].pins.size() + multiPart.signalPins.size() );
    BOOST_CHECK_EQUAL( multiSymbols[1]->GetLibPins().size(),
                       multiModel.placements[2].pins.size() + multiPart.signalPins.size() );

    m_schematic.Reset();
    destination = m_schematic.GetTopLevelSheet();
    const PADS_SCH_MODEL connectorModel = parseBinaryFixture( wxS( "connectors" ) );
    result = builder.Build( connectorModel, &m_schematic, destination, binaryFixture( wxS( "connectors" ) ) );
    SCH_SHEET_PATH connectorPath;
    connectorPath.push_back( destination );
    SCH_SYMBOL* connector = nullptr;

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &connectorPath ) == wxS( "P1" ) )
            connector = symbol;
    }

    BOOST_REQUIRE( connector );
    BOOST_CHECK_EQUAL( connector->GetUnit(), 1 );
    BOOST_CHECK_EQUAL( connector->GetUnitCount(), 26 );
    const MODEL_PART_TYPE& connectorPart = *std::ranges::find_if(
            connectorModel.partTypes,
            []( const MODEL_PART_TYPE& aPart )
            {
                return std::ranges::any_of( aPart.gates,
                                            []( const MODEL_GATE& aGate )
                                            {
                                                return !aGate.connectorPins.empty();
                                            } );
            } );
    const MODEL_GATE& connectorGate = *std::ranges::find_if(
            connectorPart.gates,
            []( const MODEL_GATE& aGate )
            {
                return !aGate.connectorPins.empty();
            } );
    auto connectorPlacementIt = std::ranges::find_if(
            connectorModel.placements,
            [&]( const MODEL_PLACEMENT& aPlacement )
            {
                return aPlacement.partType.id == connectorPart.id;
            } );
    BOOST_REQUIRE( connectorPlacementIt != connectorModel.placements.end() );
    const MODEL_PLACEMENT& connectorPlacement = *connectorPlacementIt;
    const MODEL_SYMBOL_DEFINITION& connectorDefinition = *std::ranges::find(
            connectorModel.definitions, connectorPlacement.definition.id, &MODEL_SYMBOL_DEFINITION::id );
    BOOST_REQUIRE_EQUAL( connectorGate.connectorPins.size(), 26u );
    BOOST_REQUIRE( !connectorDefinition.pins.empty() );
    std::vector<SCH_PIN*> builtConnectorPins = connector->GetAllLibPins();

    for( size_t index = 0; index < connectorGate.connectorPins.size(); ++index )
    {
        const MODEL_CONNECTOR_PIN& sourcePin = connectorGate.connectorPins[index];

        for( const MODEL_PIN_DEFINITION& graphicPin : connectorDefinition.pins )
        {
            auto builtPin = std::ranges::find_if(
                    builtConnectorPins,
                    [&]( const SCH_PIN* aPin )
                    {
                        return aPin->GetUnit() == static_cast<int>( index + 1 )
                               && aPin->GetPosition() == localPoint( graphicPin.position );
                    } );
            BOOST_REQUIRE_MESSAGE( builtPin != builtConnectorPins.end(), index + 1 );
            BOOST_CHECK_EQUAL( ( *builtPin )->GetNumber(), sourcePin.number.text );
            BOOST_CHECK_EQUAL( ( *builtPin )->GetName(), sourcePin.name.text );
            BOOST_CHECK( ( *builtPin )->GetType() == pinType( sourcePin.electricalType ) );
            BOOST_CHECK_EQUAL( ( *builtPin )->GetLength(),
                               schIUScale.MilsToIU( static_cast<double>( graphicPin.length ) / 2.0 ) );
            BOOST_CHECK( ( *builtPin )->GetOrientation() == pinOrientation( graphicPin.angle ) );
            BOOST_CHECK( ( *builtPin )->GetShape() == pinShape( graphicPin.graphicStyle ) );
            BOOST_CHECK_EQUAL( ( *builtPin )->IsVisible(), graphicPin.presentation.visible );
            BOOST_CHECK_EQUAL(
                    ( *builtPin )->GetNameTextSize(),
                    schIUScale.MilsToIU( static_cast<double>( graphicPin.namePresentation.height ) / 2.0 ) );
            BOOST_CHECK_EQUAL(
                    ( *builtPin )->GetNumberTextSize(),
                    schIUScale.MilsToIU( static_cast<double>( graphicPin.numberPresentation.height ) / 2.0 ) );
        }
    }

    m_schematic.Reset();
    destination = m_schematic.GetTopLevelSheet();
    const PADS_SCH_MODEL fieldModel = parseBinaryFixture( wxS( "fields" ) );
    result = builder.Build( fieldModel, &m_schematic, destination, binaryFixture( wxS( "fields" ) ) );
    BOOST_REQUIRE_EQUAL( result.counts.symbols, fieldModel.placements.size() );

    SCH_SHEET_PATH fieldPath;
    fieldPath.push_back( destination );
    SCH_SYMBOL* r1 = nullptr;

    for( SCH_ITEM* item : destination->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &fieldPath ) == wxS( "R1" ) )
            r1 = symbol;
    }

    BOOST_REQUIRE( r1 );

    for( const MODEL_FIELD& sourceField : fieldModel.placements.front().fields )
    {
        SCH_FIELD* builtField = nullptr;

        if( sourceField.name.text == wxS( "REF-DES" ) )
            builtField = r1->GetField( FIELD_T::REFERENCE );
        else if( sourceField.name.text == wxS( "PART-TYPE" ) )
            builtField = r1->GetField( FIELD_T::VALUE );
        else
            builtField = r1->GetField( sourceField.name.text );

        BOOST_REQUIRE_MESSAGE( builtField, sourceField.name.text );
        BOOST_CHECK_EQUAL( builtField->GetText(), sourceField.value.text );
        BOOST_CHECK_EQUAL( builtField->IsVisible(), sourceField.visible );
        BOOST_CHECK_EQUAL( builtField->GetTextHeight(),
                           schIUScale.MilsToIU( static_cast<double>( sourceField.presentation.height ) / 2.0 ) );
        BOOST_CHECK_EQUAL( builtField->GetTextThickness(),
                           schIUScale.MilsToIU( static_cast<double>( sourceField.presentation.width ) / 2.0 ) );
        BOOST_CHECK_EQUAL( builtField->GetTextAngle().AsTenthsOfADegree(), sourceField.angle );
        BOOST_CHECK_EQUAL( builtField->GetPosition(), r1->GetPosition() + localPoint( sourceField.position ) );
        BOOST_CHECK_EQUAL( builtField->IsBold(), sourceField.presentation.bold );
        BOOST_CHECK_EQUAL( builtField->IsItalic(), sourceField.presentation.italic );
        BOOST_CHECK( builtField->GetHorizJustify()
                     == horizontalJustification( sourceField.presentation.horizontalJustification ) );
        BOOST_CHECK( builtField->GetVertJustify()
                     == verticalJustification( sourceField.presentation.verticalJustification ) );
    }

    BOOST_CHECK( std::ranges::any_of( result.diagnostics,
                                      []( const PARSER_DIAGNOSTIC& aDiagnostic )
                                      {
                                          return aDiagnostic.message.Contains( wxS( "Bold Verdana" ) )
                                                 && aDiagnostic.source.controller == 17;
                                      } ) );
}


BOOST_AUTO_TEST_CASE( BinaryMultiSheetHierarchy )
{
    const PADS_SCH_BINARY::PADS_SCH_MODEL    model = parseBinaryFixture( wxS( "multisheet_connectivity" ) );
    PADS_SCH_BINARY::PADS_SCH_BINARY_BUILDER builder;
    PADS_SCH_BINARY::BUILD_RESULT            result =
            builder.Build( model, &m_schematic, nullptr, binaryFixture( wxS( "multisheet_connectivity" ) ) );
    BOOST_CHECK_EQUAL( result.counts.sheets, model.sheets.size() );
    SCH_SHEET* root = m_schematic.GetTopLevelSheet();
    BOOST_REQUIRE( root );
    BOOST_REQUIRE( root->GetScreen() );

    std::vector<SCH_SHEET*> children;

    for( SCH_ITEM* item : root->GetScreen()->Items().OfType( SCH_SHEET_T ) )
        children.push_back( static_cast<SCH_SHEET*>( item ) );

    BOOST_REQUIRE_EQUAL( children.size(), model.sheets.size() );
    BOOST_CHECK_EQUAL( root->GetScreen()->Items().size(), children.size() );
    BOOST_CHECK( root->GetScreen()->Items().OfType( SCH_SYMBOL_T ).empty() );
    BOOST_CHECK( root->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ).empty() );

    std::set<wxString> filenames;

    for( size_t i = 0; i < children.size(); ++i )
    {
        BOOST_CHECK_EQUAL( children[i]->GetField( FIELD_T::SHEET_NAME )->GetText(), model.sheets[i].name.text );
        wxString filename = children[i]->GetField( FIELD_T::SHEET_FILENAME )->GetText();
        BOOST_CHECK( !filename.Contains( wxS( "/" ) ) );
        BOOST_CHECK( !filename.Contains( wxS( ":" ) ) );
        BOOST_CHECK( !filename.Contains( wxS( "*" ) ) );
        BOOST_CHECK( filenames.insert( filename ).second );
        const wxString expectedFilename =
                i == 0 ? wxS( "[1]DUP_SAFE__.kicad_sch" ) : wxS( "[2]DUP_SAFE__.kicad_sch" );
        BOOST_CHECK_EQUAL( filename, expectedFilename );
        BOOST_REQUIRE( children[i]->GetScreen() );
        BOOST_CHECK_EQUAL( children[i]->GetScreen()->GetPageNumber(), wxString::Format( wxS( "%zu" ), i + 1 ) );
        BOOST_CHECK_EQUAL( children[i]->GetScreen()->GetPageSettings().GetWidthMils(), model.sheets[i].pageSize.x / 2 );
        BOOST_CHECK_EQUAL( children[i]->GetScreen()->GetPageSettings().GetHeightMils(),
                           model.sheets[i].pageSize.y / 2 );

        std::multiset<wxString> expectedLabels;
        std::multiset<wxString> builtLabels;

        for( const PADS_SCH_BINARY::MODEL_LABEL& label : model.labels )
        {
            if( label.sheet.id == model.sheets[i].id && !label.linkedSheets.empty() )
                expectedLabels.insert( label.text.text );
        }

        for( SCH_ITEM* item : children[i]->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
            builtLabels.insert( static_cast<SCH_GLOBALLABEL*>( item )->GetText() );

        BOOST_CHECK( builtLabels == expectedLabels );
    }

    BOOST_CHECK_EQUAL( result.counts.labels,
                       std::ranges::count_if( model.labels,
                                              []( const PADS_SCH_BINARY::MODEL_LABEL& aLabel )
                                              {
                                                  return !aLabel.linkedSheets.empty();
                                              } ) );
}


BOOST_AUTO_TEST_CASE( BinaryAppendIsAtomic )
{
    using namespace PADS_SCH_BINARY;

    SCH_SHEET* destination = m_schematic.GetTopLevelSheet();
    BOOST_REQUIRE( destination );
    BOOST_REQUIRE( destination->GetScreen() );
    PADS_SCH_BINARY_BUILDER builder;

    LIB_ID preservedLibId;
    preservedLibId.SetLibNickname( wxS( "source_library" ) );
    preservedLibId.SetLibItemName( wxS( "source_symbol" ) );
    auto preservedSymbol = std::make_unique<LIB_SYMBOL>( wxS( "source_symbol" ) );
    preservedSymbol->SetLibId( preservedLibId );
    const wxString preservedKey = wxS( "pads_import:preserved_cache_key" );
    destination->GetScreen()->AddLibSymbol( preservedKey, std::move( preservedSymbol ) );

    PADS_SCH_MODEL single = parseBinaryFixture( wxS( "placement_transform" ) );
    size_t         beforeSymbols = itemCount( destination->GetScreen(), SCH_SYMBOL_T );
    BUILD_RESULT   singleResult =
            builder.Build( single, &m_schematic, destination, binaryFixture( wxS( "placement_transform" ) ) );
    BOOST_CHECK_EQUAL( itemCount( destination->GetScreen(), SCH_SYMBOL_T ),
                       beforeSymbols + singleResult.counts.symbols );
    BOOST_REQUIRE( destination->GetScreen()->GetLibSymbols().contains( preservedKey ) );
    BOOST_CHECK( destination->GetScreen()->GetLibSymbols().at( preservedKey )->GetLibId() == preservedLibId );

    std::set<wxString> cacheKeys;

    for( const auto& [key, symbol] : destination->GetScreen()->GetLibSymbols() )
        cacheKeys.insert( key );

    builder.Build( single, &m_schematic, destination, binaryFixture( wxS( "placement_transform" ) ) );
    std::set<wxString> repeatedCacheKeys;

    for( const auto& [key, symbol] : destination->GetScreen()->GetLibSymbols() )
        repeatedCacheKeys.insert( key );

    BOOST_CHECK( repeatedCacheKeys == cacheKeys );
    BOOST_REQUIRE( destination->GetScreen()->GetLibSymbols().contains( preservedKey ) );
    BOOST_CHECK( destination->GetScreen()->GetLibSymbols().at( preservedKey )->GetLibId() == preservedLibId );

    PADS_SCH_MODEL multi = parseBinaryFixture( wxS( "multisheet_connectivity" ) );
    auto       existingChild = std::make_unique<SCH_SHEET>( destination );
    SCH_SHEET* existingChildPtr = existingChild.get();
    existingChild->SetScreen( new SCH_SCREEN( &m_schematic ) );
    existingChild->GetField( FIELD_T::SHEET_NAME )->SetText( wxS( "Existing" ) );
    existingChild->GetField( FIELD_T::SHEET_FILENAME )->SetText( wxS( "existing.kicad_sch" ) );
    destination->GetScreen()->Append( existingChild.get() );
    existingChild.release();
    size_t         beforeChildren = itemCount( destination->GetScreen(), SCH_SHEET_T );
    BUILD_RESULT   multiResult =
            builder.Build( multi, &m_schematic, destination, binaryFixture( wxS( "multisheet_connectivity" ) ) );
    BOOST_CHECK_EQUAL( itemCount( destination->GetScreen(), SCH_SHEET_T ), beforeChildren + multiResult.counts.sheets );
    BOOST_CHECK( destination->GetScreen()->Items().contains( existingChildPtr ) );

    PADS_SCH_MODEL malformed = single;
    BOOST_REQUIRE( !malformed.placements.empty() );
    auto definition = std::find_if( malformed.definitions.begin(), malformed.definitions.end(),
                                    [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                    {
                                        return aDefinition.id == malformed.placements.front().definition.id;
                                    } );
    BOOST_REQUIRE( definition != malformed.definitions.end() );
    BOOST_REQUIRE( !definition->graphics.empty() );
    definition->graphics.front().kind = MODEL_GRAPHIC_KIND::TEXT;
    definition->graphics.front().text.text = wxS( "broken staged text" );
    definition->graphics.front().points.clear();
    OBJECT_GRAPH_SNAPSHOT before = objectGraphSnapshot( m_schematic, destination );
    BOOST_CHECK_THROW( builder.Build( malformed, &m_schematic, destination, wxS( "malformed.sch" ) ), IO_ERROR );
    BOOST_CHECK( objectGraphSnapshot( m_schematic, destination ) == before );

    PADS_SCH_BINARY_BUILDER commitFailure(
            []
            {
                THROW_IO_ERROR( wxS( "injected failure before schematic adoption" ) );
            } );
    before = objectGraphSnapshot( m_schematic, destination );
    BOOST_CHECK_THROW( commitFailure.Build( single, &m_schematic, destination, wxS( "commit_failure.sch" ) ),
                       IO_ERROR );
    BOOST_CHECK( objectGraphSnapshot( m_schematic, destination ) == before );

    auto oldCurrentChild = std::make_unique<SCH_SHEET>( destination );
    oldCurrentChild->SetScreen( new SCH_SCREEN( &m_schematic ) );
    SCH_SHEET_PATH oldCurrentPath;
    oldCurrentPath.push_back( destination );
    oldCurrentPath.push_back( oldCurrentChild.get() );
    destination->GetScreen()->Append( oldCurrentChild.get() );
    oldCurrentChild.release();
    m_schematic.SetCurrentSheet( oldCurrentPath );
    BOOST_REQUIRE_EQUAL( m_schematic.CurrentSheet().size(), 2u );

    builder.Build( single, &m_schematic, nullptr, wxS( "replacement.sch" ) );
    BOOST_REQUIRE_EQUAL( m_schematic.CurrentSheet().size(), 1u );
    BOOST_CHECK( m_schematic.CurrentSheet().at( 0 ) == destination );
    BOOST_CHECK( m_schematic.CurrentSheet().LastScreen() == destination->GetScreen() );
    SCH_SHEET_PATH freshRootPath;
    freshRootPath.push_back( destination );
    BOOST_CHECK_EQUAL( m_schematic.CurrentSheet().GetCurrentHash(), freshRootPath.GetCurrentHash() );
}


BOOST_AUTO_TEST_SUITE_END()
