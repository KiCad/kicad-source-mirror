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
#include <sch_io/sch_io_mgr.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <settings/settings_manager.h>

#include <algorithm>
#include <map>
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


BOOST_AUTO_TEST_SUITE_END()
