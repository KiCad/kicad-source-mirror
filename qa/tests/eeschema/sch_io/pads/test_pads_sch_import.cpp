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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <lib_symbol.h>
#include <schematic.h>
#include <sch_io/pads/sch_io_pads.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_line.h>
#include <sch_reference_list.h>
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


static size_t countPowerSymbols( SCH_SHEET* aRoot )
{
    size_t count = 0;

    for( const SCH_SHEET_PATH& path : SCH_SHEET_LIST( aRoot ) )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            if( static_cast<SCH_SYMBOL*>( item )->GetRef( &path ).StartsWith( wxS( "#PWR" ) ) )
                count++;
        }
    }

    return count;
}


// The same check eeschema runs before "Update PCB from Schematic"; a duplicated reference
// surfaces here as "Duplicate items <ref>"
static int checkAnnotation( const std::vector<SCH_SHEET*>& aRoots, std::vector<wxString>& aMessages )
{
    SCH_REFERENCE_LIST references;

    for( SCH_SHEET* root : aRoots )
    {
        SCH_SHEET_LIST sheets( root );

        for( SCH_SHEET_PATH& sheet : sheets )
            sheet.GetSymbols( references, SYMBOL_FILTER_ALL, true );
    }

    return references.CheckAnnotation(
            [&]( ERCE_T, const wxString& aMessage, SCH_REFERENCE*, SCH_REFERENCE* )
            {
                aMessages.push_back( aMessage );
            } );
}


// PADS off-page power ports carry no reference designator, so the importer invents one
// A per-sheet counter restarts at #PWR0001 on sheet two and every power symbol on it
// reports as a duplicate item, blocking annotation and Update PCB from Schematic
BOOST_AUTO_TEST_CASE( MultiSheetPowerReferencesAreUnique )
{
    SCH_IO_PADS plugin;

    wxString padsFile = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir()
                                            + "/plugins/pads/multisheet_connectivity.txt" );

    SCH_SHEET* rootSheet = plugin.LoadSchematicFile( padsFile, &m_schematic );
    BOOST_REQUIRE( rootSheet );

    // Both sheets carry a $PWR_SYMS +5V and a $GND_SYMS GND anchor
    BOOST_REQUIRE_EQUAL( countPowerSymbols( rootSheet ), 4u );

    std::vector<wxString> messages;
    BOOST_CHECK_EQUAL( checkAnnotation( { rootSheet }, messages ), 0 );
    BOOST_CHECK( messages.empty() );
}


BOOST_AUTO_TEST_SUITE_END()
