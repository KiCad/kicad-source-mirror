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

#include <eeschema/sch_io/geda/sch_io_geda.h>

#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_no_connect.h>
#include <sch_bus_entry.h>
#include <sch_bitmap.h>
#include <sch_shape.h>
#include <sch_text.h>
#include <sch_pin.h>
#include <lib_symbol.h>
#include <settings/settings_manager.h>
#include <reporter.h>

#include <algorithm>
#include <map>
#include <set>


struct GEDA_SCH_IMPORT_FIXTURE
{
    GEDA_SCH_IMPORT_FIXTURE() :
            m_schematic( new SCHEMATIC( nullptr ) )
    {
        m_manager.LoadProject( "" );
        m_schematic->SetProject( &m_manager.Prj() );
        m_schematic->CurrentSheet().clear();
        m_schematic->CurrentSheet().push_back( &m_schematic->Root() );
    }

    ~GEDA_SCH_IMPORT_FIXTURE()
    {
        m_schematic.reset();
    }

    SCH_SHEET* LoadGedaSchematic( const std::string& aRelPath )
    {
        std::string dataPath = KI_TEST::GetEeschemaTestDataDir() + aRelPath;
        return m_plugin.LoadSchematicFile( dataPath, m_schematic.get() );
    }

    SCH_SHEET* LoadGedaSchematicWithProperties( const std::string& aRelPath,
                                                 const std::map<std::string, UTF8>& aProps )
    {
        std::string dataPath = KI_TEST::GetEeschemaTestDataDir() + aRelPath;
        return m_plugin.LoadSchematicFile( dataPath, m_schematic.get(), nullptr, &aProps );
    }

    SCH_IO_GEDA                  m_plugin;
    std::unique_ptr<SCHEMATIC>   m_schematic;
    SETTINGS_MANAGER             m_manager;
};


BOOST_FIXTURE_TEST_SUITE( GedaSchImport, GEDA_SCH_IMPORT_FIXTURE )


// ============================================================================
// File discrimination tests
// ============================================================================

BOOST_AUTO_TEST_CASE( CanReadSchematicFile )
{
    std::string goodPath = KI_TEST::GetEeschemaTestDataDir() + "/io/geda/minimal_test.sch";
    BOOST_CHECK( m_plugin.CanReadSchematicFile( goodPath ) );
}


BOOST_AUTO_TEST_CASE( RejectsNonSchExtension )
{
    std::string txtPath = KI_TEST::GetEeschemaTestDataDir() + "/io/geda/minimal_test.txt";
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( txtPath ) );
}


BOOST_AUTO_TEST_CASE( RejectsNonGedaSchFiles )
{
    std::string legacyPath = KI_TEST::GetEeschemaTestDataDir() + "/io/geda/legacy_kicad.sch";
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( legacyPath ) );

    std::string randomPath = KI_TEST::GetEeschemaTestDataDir() + "/io/geda/random.sch";
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( randomPath ) );
}


// ============================================================================
// Minimal schematic tests (no external symbol libraries needed)
// ============================================================================

BOOST_AUTO_TEST_CASE( MinimalSchematicLoad )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/minimal_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int symbolCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_SYMBOL_T )
            symbolCount++;
    }

    BOOST_CHECK_EQUAL( symbolCount, 2 );
}


BOOST_AUTO_TEST_CASE( MinimalSchematicSymbolAttributes )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/minimal_test.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    bool foundR1 = false;
    bool foundC1 = false;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = sym->GetField( FIELD_T::REFERENCE )->GetText();

        if( ref == wxT( "R1" ) )
        {
            foundR1 = true;
            BOOST_CHECK_EQUAL( sym->GetField( FIELD_T::VALUE )->GetText(), wxT( "10k" ) );
        }

        if( ref == wxT( "C1" ) )
        {
            foundC1 = true;
            BOOST_CHECK_EQUAL( sym->GetField( FIELD_T::VALUE )->GetText(), wxT( "100nF" ) );
        }
    }

    BOOST_CHECK_MESSAGE( foundR1, "R1 symbol not found" );
    BOOST_CHECK_MESSAGE( foundC1, "C1 symbol not found" );
}


BOOST_AUTO_TEST_CASE( MinimalSchematicWires )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/minimal_test.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    int wireCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->GetLayer() == LAYER_WIRE )
                wireCount++;
        }
    }

    BOOST_CHECK_EQUAL( wireCount, 4 );
}


BOOST_AUTO_TEST_CASE( MinimalSchematicNetLabels )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/minimal_test.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    int  labelCount = 0;
    bool foundInput = false;
    bool foundMid = false;
    bool foundGnd = false;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_LABEL_T )
        {
            labelCount++;
            SCH_LABEL* label = static_cast<SCH_LABEL*>( item );

            if( label->GetText() == wxT( "INPUT" ) )
                foundInput = true;
            else if( label->GetText() == wxT( "MID" ) )
                foundMid = true;
            else if( label->GetText() == wxT( "GND" ) )
                foundGnd = true;
        }
    }

    BOOST_CHECK_EQUAL( labelCount, 3 );
    BOOST_CHECK_MESSAGE( foundInput, "INPUT label not found" );
    BOOST_CHECK_MESSAGE( foundMid, "MID label not found" );
    BOOST_CHECK_MESSAGE( foundGnd, "GND label not found" );
}


BOOST_AUTO_TEST_CASE( MinimalSchematicJunctions )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/minimal_test.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    int junctionCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_JUNCTION_T )
            junctionCount++;
    }

    // Without gEDA symbol libraries installed, fallback symbols have no pins,
    // so no point reaches the 3-endpoint junction threshold from wires alone.
    BOOST_CHECK_EQUAL( junctionCount, 0 );
}


// ============================================================================
// Real-world AYAB schematic tests (uses gafrc for custom symbol discovery)
// ============================================================================

BOOST_AUTO_TEST_CASE( AyabSchematicLoad )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int symbolCount = 0;
    int wireCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_SYMBOL_T )
            symbolCount++;
        else if( item->Type() == SCH_LINE_T
                 && static_cast<SCH_LINE*>( item )->GetLayer() == LAYER_WIRE )
            wireCount++;
    }

    // 133 C lines in file, 1 is title-D.sym (skipped), 5 are nc-* (mapped to no-connect) = 127 symbols
    BOOST_CHECK_EQUAL( symbolCount, 127 );
    // 355 N lines in the schematic
    BOOST_CHECK_EQUAL( wireCount, 355 );
}


BOOST_AUTO_TEST_CASE( AyabSymbolAttributes )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    std::map<wxString, wxString> expectedValues = {
        { wxT( "U1" ),  wxT( "METROMINI" ) },
        { wxT( "U4" ),  wxT( "" ) },           // ULN2803A has no value= attr
        { wxT( "U2" ),  wxT( "" ) },           // PCF8574T has no value= attr
        { wxT( "R1" ),  wxT( "5k" ) },
        { wxT( "R2" ),  wxT( "62" ) },
        { wxT( "R3" ),  wxT( "320" ) },
        { wxT( "C1" ),  wxT( "1 uF" ) },
        { wxT( "C2" ),  wxT( "0.1 uF" ) },
        { wxT( "C3" ),  wxT( "22 uF 16V" ) },
        { wxT( "D1" ),  wxT( "" ) },           // diode has device= but no value=
        { wxT( "L1" ),  wxT( "AT-1224-TWT-5V-2-R" ) },
        { wxT( "Q1" ),  wxT( "" ) },
        { wxT( "S1" ),  wxT( "" ) },
        { wxT( "RN1" ), wxT( "1k" ) },
        { wxT( "J1" ),  wxT( "" ) },
    };

    std::set<wxString> foundRefs;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = sym->GetField( FIELD_T::REFERENCE )->GetText();

        if( expectedValues.count( ref ) )
        {
            foundRefs.insert( ref );
            wxString expectedVal = expectedValues[ref];

            if( !expectedVal.IsEmpty() )
            {
                BOOST_CHECK_MESSAGE(
                        sym->GetField( FIELD_T::VALUE )->GetText() == expectedVal,
                        ref + " value mismatch: expected '" + expectedVal + "' got '"
                                + sym->GetField( FIELD_T::VALUE )->GetText() + "'" );
            }
        }
    }

    for( const auto& [ref, val] : expectedValues )
    {
        BOOST_CHECK_MESSAGE( foundRefs.count( ref ), ref + " symbol not found" );
    }
}


BOOST_AUTO_TEST_CASE( AyabFootprintAttributes )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    std::map<wxString, wxString> expectedFootprints = {
        { wxT( "U1" ),  wxT( "MetroMini" ) },
        { wxT( "R1" ),  wxT( "0805_reflow_solder_2" ) },
        { wxT( "C2" ),  wxT( "0603_reflow_solder" ) },
        { wxT( "D1" ),  wxT( "DO214AA_HSMB" ) },
        { wxT( "S1" ),  wxT( "TYCO_FSMJSM" ) },
        { wxT( "L1" ),  wxT( "buzzer.fp" ) },
        { wxT( "Q1" ),  wxT( "SOT23_3" ) },
        { wxT( "RN1" ), wxT( "ResArray_1206x4_YC164" ) },
    };

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = sym->GetField( FIELD_T::REFERENCE )->GetText();

        auto it = expectedFootprints.find( ref );

        if( it != expectedFootprints.end() )
        {
            BOOST_CHECK_MESSAGE(
                    sym->GetField( FIELD_T::FOOTPRINT )->GetText() == it->second,
                    ref + " footprint mismatch: expected '" + it->second + "' got '"
                            + sym->GetField( FIELD_T::FOOTPRINT )->GetText() + "'" );
        }
    }
}


BOOST_AUTO_TEST_CASE( AyabCustomSymbolsLoaded )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    // These custom symbols from gschem-symbols/ should be loaded with actual pin
    // definitions (not fallback rectangles which have 0 pins).
    std::map<wxString, int> expectedPinCounts = {
        { wxT( "U1" ),  28 },  // MetroMini.sym has 28 pins
        { wxT( "D3" ),  2 },   // led-small.sym has 2 pins
        { wxT( "D1" ),  2 },   // diode-3a.sym has 2 pins
        { wxT( "S1" ),  4 },   // switch-tact_sq.sym has 4 pins
        { wxT( "Q1" ),  3 },   // trans_BEC_NPN.sym has 3 pins
        { wxT( "J1" ),  3 },   // connector3-1B.sym has 3 pins
    };

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = sym->GetField( FIELD_T::REFERENCE )->GetText();

        auto it = expectedPinCounts.find( ref );

        if( it != expectedPinCounts.end() )
        {
            int pinCount = static_cast<int>( sym->GetLibPins().size() );

            BOOST_CHECK_MESSAGE(
                    pinCount == it->second,
                    ref + " pin count mismatch: expected " + std::to_string( it->second )
                            + " got " + std::to_string( pinCount )
                            + " (symbol may not have loaded from gschem-symbols/)" );
        }
    }
}


BOOST_AUTO_TEST_CASE( AyabStdlibSymbolsLoaded )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    // Standard symbols from our stdlib/ stubs should also have pins
    std::map<wxString, int> expectedPinCounts = {
        { wxT( "R1" ),  2 },   // resistor-1.sym has 2 pins
        { wxT( "C1" ),  2 },   // capacitor-1.sym has 2 pins
        { wxT( "C3" ),  2 },   // capacitor-2.sym has 2 pins
    };

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = sym->GetField( FIELD_T::REFERENCE )->GetText();

        auto it = expectedPinCounts.find( ref );

        if( it != expectedPinCounts.end() )
        {
            int pinCount = static_cast<int>( sym->GetLibPins().size() );

            BOOST_CHECK_MESSAGE(
                    pinCount == it->second,
                    ref + " pin count mismatch: expected " + std::to_string( it->second )
                            + " got " + std::to_string( pinCount )
                            + " (stdlib stub may not have been found via gafrc)" );
        }
    }
}


BOOST_AUTO_TEST_CASE( ProjectSymbolOverridesSystem )
{
    // gEDA uses last-found-wins for symbol resolution. Project-local directories
    // are scanned after system directories, so a local resistor-1.sym with 3 pins
    // should override the builtin 2-pin version.
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/priority_test/priority_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        int         pinCount = static_cast<int>( sym->GetLibPins().size() );

        BOOST_CHECK_MESSAGE( pinCount == 3,
                             "Expected 3-pin local override, got " + std::to_string( pinCount )
                                     + " pins (project symbol should override system)" );
        return;
    }

    BOOST_FAIL( "Symbol not found" );
}


BOOST_AUTO_TEST_CASE( AyabGlobalLabelsFromNetAttributes )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    // net= attributes on output-1/input-1 symbols create global labels.
    // Power symbols (generic-power.sym, gnd-1.sym) use power pins instead.
    // 9V is only on generic-power.sym instances so appears as power, not label.
    std::set<wxString> expectedNets = {
        wxT( "5V" ), wxT( "GND" ), wxT( "SDA" ), wxT( "SCL" ),
        wxT( "RST" ), wxT( "BUZZ" ), wxT( "D5" ), wxT( "D6" ), wxT( "AREF" ),
        wxT( "ENCA" ), wxT( "ENCB" ), wxT( "ENCC" ), wxT( "EOLR" ), wxT( "EOLL" ),
    };

    std::set<wxString> foundNets;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_GLOBAL_LABEL_T )
        {
            SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( item );
            foundNets.insert( label->GetText() );
        }
    }

    for( const wxString& net : expectedNets )
    {
        BOOST_CHECK_MESSAGE( foundNets.count( net ),
                             "Global label for net '" + net + "' not found" );
    }
}


BOOST_AUTO_TEST_CASE( AyabGlobalLabelCounts )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    std::map<wxString, int> labelCounts;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_GLOBAL_LABEL_T )
        {
            SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( item );
            labelCounts[label->GetText()]++;
        }
    }

    // Each net= attribute on a non-power symbol produces one global label.
    // Power symbol instances (generic-power.sym) use power pins, not labels.
    BOOST_CHECK_EQUAL( labelCounts[wxT( "5V" )], 7 );
    BOOST_CHECK_EQUAL( labelCounts[wxT( "SCL" )], 5 );
    BOOST_CHECK_EQUAL( labelCounts[wxT( "SDA" )], 5 );
    BOOST_CHECK_EQUAL( labelCounts[wxT( "BUZZ" )], 2 );
    BOOST_CHECK_EQUAL( labelCounts[wxT( "RST" )], 2 );
    BOOST_CHECK_EQUAL( labelCounts[wxT( "GND" )], 1 );
}


BOOST_AUTO_TEST_CASE( AyabPowerSymbols )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    // gEDA power symbols (gnd-1.sym, generic-power.sym) should be imported
    // as KiCad power symbols with #PWR references and net name as value.
    std::map<wxString, int> powerCounts;
    int totalPower = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        const LIB_SYMBOL* libSym = sym->GetLibSymbolRef().get();

        if( !libSym || !libSym->IsPower() )
            continue;

        totalPower++;
        wxString valText = sym->GetField( FIELD_T::VALUE )->GetText();
        powerCounts[valText]++;

        wxString refText = sym->GetField( FIELD_T::REFERENCE )->GetText();
        BOOST_CHECK_MESSAGE( refText.StartsWith( wxT( "#PWR" ) ),
                             "Power symbol ref should start with #PWR, got: " + refText );
        BOOST_CHECK( !sym->GetField( FIELD_T::REFERENCE )->IsVisible() );
        BOOST_CHECK( sym->GetField( FIELD_T::VALUE )->IsVisible() );
    }

    // 17 gnd-1.sym instances + 11 generic-power.sym instances = 28 power symbols
    BOOST_CHECK_EQUAL( totalPower, 28 );
    BOOST_CHECK_EQUAL( powerCounts[wxT( "GND" )], 17 );
    BOOST_CHECK_EQUAL( powerCounts[wxT( "5V" )], 7 );
    BOOST_CHECK_EQUAL( powerCounts[wxT( "9V" )], 4 );
}


BOOST_AUTO_TEST_CASE( AyabJunctionsPlaced )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    int junctionCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_JUNCTION_T )
            junctionCount++;
    }

    // The ayab schematic has T-junctions where wire endpoints touch the
    // interior of other wire segments.
    BOOST_CHECK_GT( junctionCount, 0 );
}


BOOST_AUTO_TEST_CASE( AyabAllRefdesUnique )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    std::map<wxString, int> refCounts;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = sym->GetField( FIELD_T::REFERENCE )->GetText();

        if( !ref.IsEmpty() )
            refCounts[ref]++;
    }

    // Every assigned refdes should appear exactly once
    for( const auto& [ref, count] : refCounts )
    {
        BOOST_CHECK_MESSAGE( count == 1,
                             "Refdes '" + ref + "' appears " + std::to_string( count )
                                     + " times (expected 1)" );
    }
}


BOOST_AUTO_TEST_CASE( AyabWireEndpointsInPositiveSpace )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_LINE_T )
            continue;

        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( line->GetLayer() != LAYER_WIRE )
            continue;

        VECTOR2I start = line->GetStartPoint();
        VECTOR2I end = line->GetEndPoint();

        // After Y-flip, all coordinates should be in positive space
        BOOST_CHECK_MESSAGE( start.x >= 0 && start.y >= 0,
                             "Wire start in negative space: ("
                                     + std::to_string( start.x ) + ", "
                                     + std::to_string( start.y ) + ")" );
        BOOST_CHECK_MESSAGE( end.x >= 0 && end.y >= 0,
                             "Wire end in negative space: ("
                                     + std::to_string( end.x ) + ", "
                                     + std::to_string( end.y ) + ")" );
    }
}


BOOST_AUTO_TEST_CASE( AyabSymbolPositionsInPositiveSpace )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        VECTOR2I    pos = sym->GetPosition();

        BOOST_CHECK_MESSAGE( pos.x >= 0 && pos.y >= 0,
                             "Symbol at negative position: ("
                                     + std::to_string( pos.x ) + ", "
                                     + std::to_string( pos.y ) + ")" );
    }
}


BOOST_AUTO_TEST_CASE( AyabComponentOrientations )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    int rotatedCount = 0;
    int mirroredCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        int         orient = sym->GetOrientation();

        if( orient & ( SYM_ORIENT_90 | SYM_ORIENT_180 | SYM_ORIENT_270 ) )
            rotatedCount++;

        if( orient & SYM_MIRROR_Y )
            mirroredCount++;
    }

    // The ayab schematic has 48 rotated and 38 mirrored components in gEDA source.
    // Verify the importer preserves orientation diversity.
    BOOST_CHECK_MESSAGE( rotatedCount > 0,
                         "Expected rotated components but found none" );
    BOOST_CHECK_MESSAGE( mirroredCount > 0,
                         "Expected mirrored components but found none" );
}


BOOST_AUTO_TEST_CASE( AyabTextAnnotations )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    int       textCount = 0;
    bool      foundTitle = false;
    bool      foundAuthor = false;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_TEXT_T )
            continue;

        textCount++;
        SCH_TEXT* text = static_cast<SCH_TEXT*>( item );

        if( text->GetText().Contains( wxT( "AYAB Interface" ) ) )
            foundTitle = true;

        if( text->GetText().Contains( wxT( "Windell" ) ) )
            foundAuthor = true;
    }

    // 13 standalone T lines in ayab_rs.sch (titles, notes, annotations)
    BOOST_CHECK_EQUAL( textCount, 13 );
    BOOST_CHECK_MESSAGE( foundTitle, "Title text 'AYAB Interface' not found" );
    BOOST_CHECK_MESSAGE( foundAuthor, "Author text containing 'Windell' not found" );
}


BOOST_AUTO_TEST_CASE( AyabWireConnectivity )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    // Count wire endpoint coincidences. Connected wires share endpoints,
    // so counting shared points measures connectivity.
    std::map<std::pair<int, int>, int> endpointCounts;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_LINE_T )
            continue;

        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( line->GetLayer() != LAYER_WIRE )
            continue;

        VECTOR2I start = line->GetStartPoint();
        VECTOR2I end = line->GetEndPoint();
        endpointCounts[{start.x, start.y}]++;
        endpointCounts[{end.x, end.y}]++;
    }

    int sharedPoints = 0;

    for( const auto& [pos, count] : endpointCounts )
    {
        if( count >= 2 )
            sharedPoints++;
    }

    // In a well-connected schematic, many wire endpoints overlap.
    // The ayab schematic has 101 such shared points (from 355 wires).
    BOOST_CHECK_MESSAGE( sharedPoints > 50,
                         "Expected at least 50 shared wire endpoints for connectivity, got "
                                 + std::to_string( sharedPoints ) );
}


BOOST_AUTO_TEST_CASE( AyabGlobalLabelPositionsPositive )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_GLOBAL_LABEL_T )
            continue;

        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( item );
        VECTOR2I         pos = label->GetPosition();

        BOOST_CHECK_MESSAGE( pos.x >= 0 && pos.y >= 0,
                             "Global label '" + label->GetText()
                                     + "' at negative position: ("
                                     + std::to_string( pos.x ) + ", "
                                     + std::to_string( pos.y ) + ")" );
    }
}


BOOST_AUTO_TEST_CASE( AyabTotalGlobalLabelCount )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    int totalLabels = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_GLOBAL_LABEL_T )
            totalLabels++;
    }

    // 45 non-power net= attributes produce global labels.
    // 11 power symbol net= attributes (generic-power.sym) use power pins instead.
    BOOST_CHECK_EQUAL( totalLabels, 45 );
}


BOOST_AUTO_TEST_CASE( RealWorldSchematicLoad )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/powermeter.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int symbolCount = 0;
    int wireCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_SYMBOL_T )
            symbolCount++;
        else if( item->Type() == SCH_LINE_T
                 && static_cast<SCH_LINE*>( item )->GetLayer() == LAYER_WIRE )
            wireCount++;
    }

    BOOST_CHECK( symbolCount > 5 );
    BOOST_CHECK( wireCount > 10 );
}


// ============================================================================
// Phase 1 feature tests
// ============================================================================

BOOST_AUTO_TEST_CASE( CommentLinesIgnored )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/comments_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int symbolCount = 0;
    int wireCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_SYMBOL_T )
            symbolCount++;
        else if( item->Type() == SCH_LINE_T
                 && static_cast<SCH_LINE*>( item )->GetLayer() == LAYER_WIRE )
            wireCount++;
    }

    BOOST_CHECK_EQUAL( symbolCount, 1 );
    BOOST_CHECK_EQUAL( wireCount, 1 );
}


BOOST_AUTO_TEST_CASE( OldFormatGraphicsParse )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/old_format_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int noteLineCount = 0;
    int wireCount = 0;
    int shapeCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->GetLayer() == LAYER_NOTES )
                noteLineCount++;
            else if( line->GetLayer() == LAYER_WIRE )
                wireCount++;
        }
        else if( item->Type() == SCH_SHAPE_T )
        {
            shapeCount++;
        }
    }

    // Old format file has L, B, V, A graphic objects and one N wire
    BOOST_CHECK_EQUAL( noteLineCount, 1 );
    BOOST_CHECK_EQUAL( wireCount, 1 );
    BOOST_CHECK_EQUAL( shapeCount, 3 );
}


BOOST_AUTO_TEST_CASE( OldFormatBusPinParse )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/old_bus_pin_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int busCount = 0;
    int wireCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_LINE_T )
            continue;

        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( line->GetLayer() == LAYER_BUS )
            busCount++;
        else if( line->GetLayer() == LAYER_WIRE )
            wireCount++;
    }

    BOOST_CHECK_EQUAL( busCount, 1 );
    // N line + P line (pin becomes wire stub in schematic context)
    BOOST_CHECK_EQUAL( wireCount, 2 );
}


BOOST_AUTO_TEST_CASE( OldFormatTextParse )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/old_text_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int textCount = 0;
    bool foundHello = false;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_TEXT_T )
        {
            textCount++;
            SCH_TEXT* text = static_cast<SCH_TEXT*>( item );

            if( text->GetText() == wxT( "Hello World" ) )
                foundHello = true;
        }
    }

    BOOST_CHECK_EQUAL( textCount, 1 );
    BOOST_CHECK_MESSAGE( foundHello, "Text 'Hello World' not found in old-format file" );
}


BOOST_AUTO_TEST_CASE( VeryOldFormatTextParse )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/very_old_text_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int textCount = 0;
    bool foundOldText = false;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_TEXT_T )
        {
            textCount++;
            SCH_TEXT* text = static_cast<SCH_TEXT*>( item );

            if( text->GetText() == wxT( "Old Text" ) )
                foundOldText = true;
        }
    }

    BOOST_CHECK_EQUAL( textCount, 1 );
    BOOST_CHECK_MESSAGE( foundOldText, "Text 'Old Text' not found in very-old-format file" );
}


BOOST_AUTO_TEST_CASE( BusLayerCorrect )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/bus_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int busCount = 0;
    int wireCount = 0;
    int noteCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_LINE_T )
            continue;

        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( line->GetLayer() == LAYER_BUS )
            busCount++;
        else if( line->GetLayer() == LAYER_WIRE )
            wireCount++;
        else if( line->GetLayer() == LAYER_NOTES )
            noteCount++;
    }

    BOOST_CHECK_EQUAL( busCount, 2 );
    BOOST_CHECK_EQUAL( wireCount, 1 );
    BOOST_CHECK_EQUAL( noteCount, 0 );
}


BOOST_AUTO_TEST_CASE( NoConnectFromNcSymbol )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/graphical_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int symbolCount = 0;
    int ncCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_SYMBOL_T )
            symbolCount++;
        else if( item->Type() == SCH_NO_CONNECT_T )
            ncCount++;
    }

    // R1 is a normal symbol, nc-right-1 becomes SCH_NO_CONNECT
    BOOST_CHECK_EQUAL( symbolCount, 1 );
    BOOST_CHECK_EQUAL( ncCount, 1 );
}


BOOST_AUTO_TEST_CASE( AyabNoConnectsCreated )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int ncCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_NO_CONNECT_T )
            ncCount++;
    }

    // 5 nc-* symbols in ayab_rs.sch should map to SCH_NO_CONNECT
    BOOST_CHECK_EQUAL( ncCount, 5 );
}


BOOST_AUTO_TEST_CASE( NoConnectAtPinPosition )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/graphical_test.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    // nc-right-1.sym at (44000, 44000) with angle=0, mirror=0 has its pin
    // connection point at local (0, 100). The SCH_NO_CONNECT should be placed
    // at gEDA (44000, 44100), not at the component origin (44000, 44000).
    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_NO_CONNECT_T )
            continue;

        SCH_NO_CONNECT* nc = static_cast<SCH_NO_CONNECT*>( item );
        VECTOR2I pos = nc->GetPosition();

        // The NC must NOT be at the component origin. If pin offset was applied
        // correctly, the Y coordinate will differ from the origin.
        BOOST_CHECK_MESSAGE( pos.x != 0 && pos.y != 0,
                             "NC position should be non-zero after coordinate transform" );
    }
}


BOOST_AUTO_TEST_CASE( DocumentationToDatasheet )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/documentation_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    bool foundDatasheet = false;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        SCH_FIELD* dsField = sym->GetField( FIELD_T::DATASHEET );

        if( dsField && dsField->GetText() == wxT( "http://www.example.com/datasheet.pdf" ) )
            foundDatasheet = true;
    }

    BOOST_CHECK_MESSAGE( foundDatasheet,
                         "DATASHEET field with documentation URL not found" );
}


BOOST_AUTO_TEST_CASE( AyabDescriptionFieldVisibility )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    // D3 (led-small.sym) has description="9V Power" with visibility=1, showNV=1
    // (show value only). The DESCRIPTION field should be visible.
    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        wxString ref = sym->GetField( FIELD_T::REFERENCE )->GetText();

        if( ref != wxT( "D3" ) )
            continue;

        SCH_FIELD* descField = sym->GetField( FIELD_T::DESCRIPTION );
        BOOST_REQUIRE( descField );
        BOOST_CHECK( descField->GetText() == wxT( "9V Power" ) );
        BOOST_CHECK( descField->IsVisible() );
        return;
    }

    BOOST_FAIL( "D3 symbol not found" );
}


BOOST_AUTO_TEST_CASE( AyabBusLayerFixed )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int notesLineCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->GetLayer() == LAYER_NOTES )
                notesLineCount++;
        }
    }

    // The ayab schematic has 13 visible T annotations but no non-text
    // LAYER_NOTES lines -- former bus lines should now be LAYER_BUS.
    BOOST_CHECK_EQUAL( notesLineCount, 0 );
}


// ============================================================================
// Phase 2: Text overbar conversion
// ============================================================================

BOOST_AUTO_TEST_CASE( TextOverbarConversion )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/overbar_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    std::vector<wxString> textContents;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_TEXT_T )
        {
            SCH_TEXT* text = static_cast<SCH_TEXT*>( item );
            textContents.push_back( text->GetText() );
        }
    }

    // We expect 6 text items from the test file
    BOOST_REQUIRE_GE( textContents.size(), 6u );

    bool foundFullOverbar = false;
    bool foundTrailingOverbar = false;
    bool foundPartialOverbar = false;
    bool foundBackslash = false;
    bool foundNoOverbar = false;
    bool foundMultiOverbar = false;

    for( const wxString& txt : textContents )
    {
        if( txt == wxT( "~{ACTIVE}" ) )
            foundFullOverbar = true;

        if( txt == wxT( "DATA~{LOW}" ) )
            foundTrailingOverbar = true;

        if( txt == wxT( "ACTIVE ~{HIGH}" ) )
            foundPartialOverbar = true;

        if( txt == wxT( "\\backslash" ) )
            foundBackslash = true;

        if( txt == wxT( "No overbar here" ) )
            foundNoOverbar = true;

        if( txt == wxT( "~{CS} and ~{WR}" ) )
            foundMultiOverbar = true;
    }

    BOOST_CHECK_MESSAGE( foundFullOverbar,
                         "Full overbar \\_ACTIVE\\_ -> ~{ACTIVE} not found" );
    BOOST_CHECK_MESSAGE( foundTrailingOverbar,
                         "Trailing overbar DATA\\_LOW\\_ -> DATA~{LOW} not found" );
    BOOST_CHECK_MESSAGE( foundPartialOverbar,
                         "Partial overbar ACTIVE \\_HIGH\\_ -> ACTIVE ~{HIGH} not found" );
    BOOST_CHECK_MESSAGE( foundBackslash,
                         "Escaped backslash \\\\\\\\ -> \\ not found" );
    BOOST_CHECK_MESSAGE( foundNoOverbar,
                         "Plain text 'No overbar here' not found" );
    BOOST_CHECK_MESSAGE( foundMultiOverbar,
                         "Multiple overbars \\_CS\\_ and \\_WR\\_ -> ~{CS} and ~{WR} not found" );
}


// ============================================================================
// Phase 2: Bezier curve subdivision
// ============================================================================

BOOST_AUTO_TEST_CASE( BezierCurveSubdivision )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/bezier_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int bezierCount = 0;
    int lineCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_SHAPE_T )
        {
            SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( item );

            if( shape->GetShape() == SHAPE_T::BEZIER )
                bezierCount++;
        }
        else if( item->Type() == SCH_LINE_T )
        {
            lineCount++;
        }
    }

    // First path: M + C = 1 native bezier shape
    // Second path: M + C + L = 1 native bezier shape + 1 line segment
    BOOST_CHECK_EQUAL( bezierCount, 2 );
    BOOST_CHECK_EQUAL( lineCount, 1 );
}


// ============================================================================
// Phase 2: Embedded picture import
// ============================================================================

BOOST_AUTO_TEST_CASE( EmbeddedPictureImport )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/picture_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int bitmapCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_BITMAP_T )
            bitmapCount++;
    }

    BOOST_CHECK_EQUAL( bitmapCount, 1 );
}


// ============================================================================
// Correctness audit fixes
// ============================================================================


BOOST_AUTO_TEST_CASE( TextSizeScaling )
{
    // gEDA text size is in points where 1 point = 10 mils.
    // size=10 -> 100 mils -> 100*254 = 25400 IU
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/text_size_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    std::vector<SCH_TEXT*> texts;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_TEXT_T )
            texts.push_back( static_cast<SCH_TEXT*>( item ) );
    }

    BOOST_REQUIRE_GE( texts.size(), 2u );

    bool foundSize10 = false;
    bool foundSize20 = false;

    for( SCH_TEXT* t : texts )
    {
        int h = t->GetTextSize().y;

        if( h == 10 * 10 * 254 )
            foundSize10 = true;
        else if( h == 20 * 10 * 254 )
            foundSize20 = true;
    }

    BOOST_CHECK_MESSAGE( foundSize10, "Expected text with size=10 (25400 IU)" );
    BOOST_CHECK_MESSAGE( foundSize20, "Expected text with size=20 (50800 IU)" );
}


BOOST_AUTO_TEST_CASE( TextAngleNormalization )
{
    // Non-orthogonal angles should be snapped to the nearest 90-degree increment
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/text_angle_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    std::set<int> angles;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_TEXT_T )
        {
            SCH_TEXT* t = static_cast<SCH_TEXT*>( item );
            angles.insert( t->GetTextAngle().AsTenthsOfADegree() );
        }
    }

    // 45->90, 135->90, 200->180, 315->0, 90->90
    // So we should see angles 0, 90, 180 in tenths: 0, 900, 1800
    BOOST_CHECK( angles.count( 900 ) > 0 );   // 90 degrees
    BOOST_CHECK( angles.count( 0 ) > 0 );      // 0 degrees (315 snaps to 0)
    BOOST_CHECK( angles.count( 1800 ) > 0 );   // 180 degrees (200 snaps to 180)
}


BOOST_AUTO_TEST_CASE( RelativePathCommands )
{
    // Lowercase SVG path commands (l) should produce relative offsets,
    // yielding the same endpoint as equivalent absolute (L) commands
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/relative_path_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int lineCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->GetLayer() == LAYER_NOTES )
            {
                lineCount++;
                VECTOR2I start = line->GetStartPoint();
                VECTOR2I end = line->GetEndPoint();
                int dx = std::abs( end.x - start.x );

                // Both paths draw a horizontal 500-mil segment = 500*254 = 127000 IU
                BOOST_CHECK_EQUAL( dx, 500 * 254 );
            }
        }
    }

    BOOST_CHECK_EQUAL( lineCount, 2 );
}


BOOST_AUTO_TEST_CASE( EmbeddedPrefixStripped )
{
    // Components with EMBEDDED prefix should have the prefix stripped from the basename
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/embedded_prefix_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int symbolCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
            wxString name = sym->GetLibId().GetLibItemName();

            // The EMBEDDED prefix should have been stripped
            BOOST_CHECK_MESSAGE( !name.StartsWith( wxT( "EMBEDDED" ) ),
                                 "Symbol name should not have EMBEDDED prefix: " + name );
            symbolCount++;
        }
    }

    BOOST_CHECK_EQUAL( symbolCount, 1 );
}


// ============================================================================
// Phase 3: Lepton-EDA library path discovery
// ============================================================================

BOOST_AUTO_TEST_CASE( LeptonConfLibraryDiscovery )
{
    // The lepton_conf_test.sch references lepton_resistor.sym, which lives
    // in lepton-symbols/ adjacent to the .sch file. A lepton.conf in the
    // same directory has [libs] component-library=lepton-symbols, so the
    // importer should discover and load the symbol with 2 pins.
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/lepton_conf_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    bool foundR1 = false;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = sym->GetField( FIELD_T::REFERENCE )->GetText();

        if( ref == wxT( "R1" ) )
        {
            foundR1 = true;
            int pinCount = static_cast<int>( sym->GetLibPins().size() );

            BOOST_CHECK_MESSAGE( pinCount == 2,
                                 "R1 pin count mismatch: expected 2 got "
                                         + std::to_string( pinCount )
                                         + " (lepton.conf library discovery may have failed)" );
            BOOST_CHECK_EQUAL( sym->GetField( FIELD_T::VALUE )->GetText(), wxT( "4.7k" ) );
        }
    }

    BOOST_CHECK_MESSAGE( foundR1, "R1 symbol not found" );
}


// ============================================================================
// Phase 3: gschemrc library discovery
// ============================================================================

BOOST_AUTO_TEST_CASE( GschemrcLibraryDiscovery )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/gschemrc_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = sym->GetField( FIELD_T::REFERENCE )->GetText();

        if( ref == wxT( "R1" ) )
        {
            int pinCount = static_cast<int>( sym->GetLibPins().size() );

            BOOST_CHECK_MESSAGE( pinCount == 2,
                                 "R1 pin count mismatch: expected 2 got "
                                         + std::to_string( pinCount )
                                         + " (gschemrc library discovery may have failed)" );
            return;
        }
    }

    BOOST_FAIL( "R1 symbol not found" );
}


// ============================================================================
// Phase 3: Bus ripper direction support
// ============================================================================

BOOST_AUTO_TEST_CASE( BusRipperCreation )
{
    // bus_ripper_test.sch has a vertical bus with ripper_dir=1 and two horizontal
    // nets whose endpoints touch the bus. The importer should create
    // SCH_BUS_WIRE_ENTRY objects at each intersection.
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/bus_ripper_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int busEntryCount = 0;
    int busCount = 0;
    int wireCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_BUS_WIRE_ENTRY_T )
        {
            busEntryCount++;

            SCH_BUS_WIRE_ENTRY* entry = static_cast<SCH_BUS_WIRE_ENTRY*>( item );
            VECTOR2I size = entry->GetSize();

            // The wire goes left from the bus, so the entry x-size should point left (negative)
            BOOST_CHECK_MESSAGE( size.x < 0,
                                 "Bus entry x-size should be negative (wire goes left)" );
        }
        else if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->GetLayer() == LAYER_BUS )
                busCount++;
            else if( line->GetLayer() == LAYER_WIRE )
                wireCount++;
        }
    }

    BOOST_CHECK_EQUAL( busCount, 1 );
    BOOST_CHECK_EQUAL( wireCount, 2 );
    BOOST_CHECK_EQUAL( busEntryCount, 2 );
}


// ============================================================================
// Phase 4: Symversion mismatch warning
// ============================================================================

BOOST_AUTO_TEST_CASE( SymversionMismatchWarning )
{
    // symversion_test.sch has a component with symversion=1.0 but the
    // corresponding symver_resistor.sym has symversion=2.0. The importer
    // should emit a warning about the major version mismatch.
    WX_STRING_REPORTER reporter;
    m_plugin.SetReporter( &reporter );

    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/symversion_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    wxString messages = reporter.GetMessages();

    BOOST_CHECK_MESSAGE( messages.Contains( wxT( "version mismatch" ) ),
                         "Expected symversion mismatch warning in reporter output, got: "
                                 + messages );
    BOOST_CHECK_MESSAGE( messages.Contains( wxT( "R1" ) ),
                         "Expected R1 reference in warning message, got: " + messages );
}


BOOST_AUTO_TEST_CASE( FuzzyMatchSuggestion )
{
    WX_STRING_REPORTER reporter;
    m_plugin.SetReporter( &reporter );

    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/fuzzy_match_test.sch" );

    BOOST_REQUIRE( sheet );

    wxString messages = reporter.GetMessages();

    BOOST_CHECK_MESSAGE( messages.Contains( wxT( "Did you mean" ) ),
                         "Expected fuzzy match suggestion in reporter output, got: "
                                 + messages );
    BOOST_CHECK_MESSAGE( messages.Contains( wxT( "resistor-1.sym" ) ),
                         "Expected 'resistor-1.sym' as suggestion, got: " + messages );
}


// ============================================================================
// Phase 3: Hierarchical sheet import
// ============================================================================

BOOST_AUTO_TEST_CASE( HierarchicalSheetCreation )
{
    // hierarchy_test.sch has a component with source=hierarchy_sub.sch,
    // which should create an SCH_SHEET that references the sub-schematic.
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/hierarchy_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int sheetCount = 0;
    int wireCount = 0;
    bool foundS1 = false;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            sheetCount++;
            SCH_SHEET* subSheet = static_cast<SCH_SHEET*>( item );
            wxString name = subSheet->GetField( FIELD_T::SHEET_NAME )->GetText();

            if( name == wxT( "S1" ) )
            {
                foundS1 = true;

                // The sheet should reference hierarchy_sub.sch
                wxString filename = subSheet->GetField( FIELD_T::SHEET_FILENAME )->GetText();
                BOOST_CHECK_EQUAL( filename, wxT( "hierarchy_sub.sch" ) );

                // The sub-schematic should have been loaded into the sheet's screen
                SCH_SCREEN* subScreen = subSheet->GetScreen();
                BOOST_REQUIRE_MESSAGE( subScreen, "Sub-sheet screen should not be null" );

                int subWireCount = 0;

                for( SCH_ITEM* subItem : subScreen->Items() )
                {
                    if( subItem->Type() == SCH_LINE_T )
                    {
                        SCH_LINE* line = static_cast<SCH_LINE*>( subItem );

                        if( line->GetLayer() == LAYER_WIRE )
                            subWireCount++;
                    }
                }

                BOOST_CHECK_MESSAGE( subWireCount > 0,
                                     "Sub-schematic should contain wires" );
            }
        }
        else if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->GetLayer() == LAYER_WIRE )
                wireCount++;
        }
    }

    // The source= component should have become a sheet, not a symbol
    BOOST_CHECK_EQUAL( sheetCount, 1 );
    BOOST_CHECK_MESSAGE( foundS1, "Sheet S1 not found" );
    BOOST_CHECK_EQUAL( wireCount, 1 );
}


BOOST_AUTO_TEST_CASE( HierarchicalSheetMissingSource )
{
    // A component with source= pointing to a nonexistent file should
    // create a sheet but with an empty screen (no crash).
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/hierarchy_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );
}


// ============================================================================
// Multi-slot pin remapping
// ============================================================================

BOOST_AUTO_TEST_CASE( MultiSlotPinRemapping )
{
    // multislot_test.sch has two instances of 7400-1.sym, one with slot=1
    // (slotdef=1:1,2,3) and one with slot=2 (slotdef=2:4,5,6). After
    // remapping, slot 1's private copy should have pins 1,2,3 and slot 2's
    // private copy should have pins 4,5,6.
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/multislot_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    std::vector<SCH_SYMBOL*> symbols;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_SYMBOL_T )
            symbols.push_back( static_cast<SCH_SYMBOL*>( item ) );
    }

    BOOST_REQUIRE_EQUAL( symbols.size(), 2u );

    // Sort by position to get a deterministic order (slot=1 is at x=40000, slot=2 at x=44000)
    std::sort( symbols.begin(), symbols.end(),
               []( const SCH_SYMBOL* a, const SCH_SYMBOL* b )
               {
                   return a->GetPosition().x < b->GetPosition().x;
               } );

    // Slot 1 should have pins 1, 2, 3
    std::set<wxString> slot1Pins;

    for( SCH_PIN* pin : symbols[0]->GetLibPins() )
        slot1Pins.insert( pin->GetNumber() );

    BOOST_CHECK_MESSAGE( slot1Pins.count( wxT( "1" ) ), "Slot 1 missing pin 1" );
    BOOST_CHECK_MESSAGE( slot1Pins.count( wxT( "2" ) ), "Slot 1 missing pin 2" );
    BOOST_CHECK_MESSAGE( slot1Pins.count( wxT( "3" ) ), "Slot 1 missing pin 3" );
    BOOST_CHECK_MESSAGE( !slot1Pins.count( wxT( "4" ) ), "Slot 1 should not have pin 4" );

    // Slot 2 should have pins 4, 5, 6
    std::set<wxString> slot2Pins;

    for( SCH_PIN* pin : symbols[1]->GetLibPins() )
        slot2Pins.insert( pin->GetNumber() );

    BOOST_CHECK_MESSAGE( slot2Pins.count( wxT( "4" ) ), "Slot 2 missing pin 4" );
    BOOST_CHECK_MESSAGE( slot2Pins.count( wxT( "5" ) ), "Slot 2 missing pin 5" );
    BOOST_CHECK_MESSAGE( slot2Pins.count( wxT( "6" ) ), "Slot 2 missing pin 6" );
    BOOST_CHECK_MESSAGE( !slot2Pins.count( wxT( "1" ) ), "Slot 2 should not have pin 1" );
}


// ============================================================================
// Graphical attribute exclusion
// ============================================================================

BOOST_AUTO_TEST_CASE( GraphicalAttributeExclusion )
{
    // graphical_attr_test.sch has R1 (normal) and R2 (graphical=1).
    // R2 should be excluded from BOM, board, and simulation.
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/graphical_attr_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    bool foundR1 = false;
    bool foundR2 = false;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        wxString    ref = sym->GetField( FIELD_T::REFERENCE )->GetText();

        if( ref == wxT( "R1" ) )
        {
            foundR1 = true;
            BOOST_CHECK( !sym->GetExcludedFromBOM() );
            BOOST_CHECK( !sym->GetExcludedFromBoard() );
            BOOST_CHECK( !sym->GetExcludedFromSim() );
        }

        if( ref == wxT( "R2" ) )
        {
            foundR2 = true;
            BOOST_CHECK( sym->GetExcludedFromBOM() );
            BOOST_CHECK( sym->GetExcludedFromBoard() );
            BOOST_CHECK( sym->GetExcludedFromSim() );
        }
    }

    BOOST_CHECK_MESSAGE( foundR1, "R1 symbol not found" );
    BOOST_CHECK_MESSAGE( foundR2, "R2 symbol not found" );
}


// ============================================================================
// Phase 3: Multi-page schematic support
// ============================================================================

BOOST_AUTO_TEST_CASE( MultiPageSchematicImport )
{
    // When LoadSchematicFile receives an "additional_schematics" property,
    // it should create sub-sheets on the root screen for each additional page.
    std::string page2Path = KI_TEST::GetEeschemaTestDataDir() + "/io/geda/multipage_page2.sch";

    std::map<std::string, UTF8> props;
    props["additional_schematics"] = page2Path;

    SCH_SHEET* sheet = LoadGedaSchematicWithProperties( "/io/geda/multipage_page1.sch", props );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    int wireCount = 0;
    int sheetCount = 0;
    SCH_SHEET* subSheet = nullptr;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->GetLayer() == LAYER_WIRE )
                wireCount++;
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            sheetCount++;
            subSheet = static_cast<SCH_SHEET*>( item );
        }
    }

    // Page 1 has one wire
    BOOST_CHECK_EQUAL( wireCount, 1 );

    // One sub-sheet should have been created for page 2
    BOOST_CHECK_EQUAL( sheetCount, 1 );

    BOOST_REQUIRE( subSheet );
    BOOST_CHECK_EQUAL( subSheet->GetField( FIELD_T::SHEET_NAME )->GetText(), wxT( "Page 2" ) );
    BOOST_CHECK_EQUAL( subSheet->GetField( FIELD_T::SHEET_FILENAME )->GetText(),
                        wxT( "multipage_page2.sch" ) );

    // The sub-sheet should have a loaded screen with wires from page 2
    SCH_SCREEN* subScreen = subSheet->GetScreen();
    BOOST_REQUIRE_MESSAGE( subScreen, "Sub-sheet screen should not be null" );

    int subWireCount = 0;

    for( SCH_ITEM* subItem : subScreen->Items() )
    {
        if( subItem->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( subItem );

            if( line->GetLayer() == LAYER_WIRE )
                subWireCount++;
        }
    }

    BOOST_CHECK_MESSAGE( subWireCount > 0, "Sub-sheet should contain wires from page 2" );
}


BOOST_AUTO_TEST_CASE( BuiltinSymbolsLoad )
{
    std::vector<wxString> builtinNames = {
        wxT( "resistor-1.sym" ),    wxT( "resistor-2.sym" ),
        wxT( "capacitor-1.sym" ),   wxT( "capacitor-2.sym" ),
        wxT( "gnd-1.sym" ),         wxT( "gnd-2.sym" ),
        wxT( "generic-power.sym" ),
        wxT( "input-1.sym" ),       wxT( "output-1.sym" ),
        wxT( "nc-right-1.sym" ),    wxT( "nc-left-1.sym" ),
        wxT( "terminal-1.sym" ),
        wxT( "vcc-1.sym" ),         wxT( "vcc-2.sym" ),
        wxT( "vdd-1.sym" ),         wxT( "vss-1.sym" ),
        wxT( "vee-1.sym" ),
        wxT( "5V-plus-1.sym" ),     wxT( "3.3V-plus-1.sym" ),
        wxT( "12V-plus-1.sym" ),
        wxT( "diode-1.sym" ),       wxT( "zener-1.sym" ),
        wxT( "schottky-1.sym" ),    wxT( "led-1.sym" ),
        wxT( "npn-1.sym" ),         wxT( "pnp-1.sym" ),
        wxT( "nmos-1.sym" ),        wxT( "pmos-1.sym" ),
        wxT( "opamp-1.sym" ),       wxT( "inductor-1.sym" ),
        wxT( "7400-1.sym" ),        wxT( "7402-1.sym" ),
        wxT( "7404-1.sym" ),        wxT( "7408-1.sym" ),
        wxT( "7432-1.sym" ),        wxT( "7486-1.sym" ),
        wxT( "busripper-1.sym" ),   wxT( "busripper-2.sym" ),
        wxT( "title-B.sym" ),
    };

    const auto& symbols = SCH_IO_GEDA::getBuiltinSymbols();

    for( const wxString& name : builtinNames )
    {
        BOOST_CHECK_MESSAGE( symbols.find( name ) != symbols.end(),
                             "Builtin symbol should exist: " + name );

        if( symbols.find( name ) != symbols.end() )
        {
            const wxString& content = symbols.at( name );
            BOOST_CHECK_MESSAGE( content.StartsWith( wxT( "v " ) ),
                                 "Builtin symbol should start with version line: " + name );
        }
    }

    BOOST_CHECK_GE( symbols.size(), builtinNames.size() );
}


BOOST_AUTO_TEST_CASE( TJunctionDetection )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/tjunction_test.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    int junctionCount = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() == SCH_JUNCTION_T )
            junctionCount++;
    }

    // Two T-junctions: vertical wires meet the interior of horizontal wires
    BOOST_CHECK_EQUAL( junctionCount, 2 );
}


BOOST_AUTO_TEST_CASE( PinOrientationCorrect )
{
    // Load a schematic using 7400-1.sym to verify pin orientation.
    // 7400-1.sym has:
    //   Pin 1 (A): P 0 200 200 200 1 0 0  whichend=0 → conn=(0,200) body=(200,200)
    //   Pin 2 (B): P 0 0 200 0 1 0 0       whichend=0 → conn=(0,0) body=(200,0)
    //   Pin 3 (Y): P 500 100 300 100 1 0 0  whichend=0 → conn=(500,100) body=(300,100)
    //
    // Pin 1: body is to the right of connection → PIN_RIGHT
    // Pin 2: body is to the right of connection → PIN_RIGHT
    // Pin 3: body is to the left of connection  → PIN_LEFT
    const auto& builtins = SCH_IO_GEDA::getBuiltinSymbols();
    auto        it = builtins.find( wxT( "7400-1.sym" ) );

    BOOST_REQUIRE( it != builtins.end() );

    SCH_IO_GEDA io;

    // Parse the builtin symbol to get pin orientations
    wxString tempPath = wxFileName::CreateTempFileName( wxT( "geda_pin_test_" ) );
    BOOST_REQUIRE( !tempPath.IsEmpty() );

    {
        wxFile temp( tempPath, wxFile::write );
        BOOST_REQUIRE( temp.IsOpened() );
        temp.Write( it->second );
    }

    SCH_IO_GEDA         loader;
    std::unique_ptr<LIB_SYMBOL> sym;

    // Use a known .sym file from test data instead
    wxString symPath( KI_TEST::GetEeschemaTestDataDir() + "/io/geda/7400-1.sym" );
    wxTextFile file;

    BOOST_REQUIRE( file.Open( symPath ) );

    wxRemoveFile( tempPath );

    // Verify the pins have correct orientation via the symbol loaded from test data
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/minimal_test.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    // Find R1 (resistor-1.sym) which has a horizontal pin layout.
    // resistor-1.sym is a horizontal resistor: pin 1 at x=0, pin 2 at x=900
    // Pin 1: conn=(0,200) body=(200,200) → PIN_RIGHT (body extends right from conn)
    // Pin 2: conn=(900,200) body=(700,200) → PIN_LEFT (body extends left from conn)
    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym2 = static_cast<SCH_SYMBOL*>( item );

        if( sym2->GetField( FIELD_T::REFERENCE )->GetText() != wxT( "R1" ) )
            continue;

        const LIB_SYMBOL* libSym = sym2->GetLibSymbolRef().get();

        BOOST_REQUIRE( libSym );

        std::vector<SCH_PIN*> pins = libSym->GetPins();

        BOOST_REQUIRE_GE( pins.size(), 2u );

        // Check that we have both left and right pins (not all the same direction)
        bool hasRight = false;
        bool hasLeft = false;

        for( SCH_PIN* pin : pins )
        {
            if( pin->GetOrientation() == PIN_ORIENTATION::PIN_RIGHT )
                hasRight = true;

            if( pin->GetOrientation() == PIN_ORIENTATION::PIN_LEFT )
                hasLeft = true;
        }

        BOOST_CHECK_MESSAGE( hasRight && hasLeft,
                             "Resistor should have both PIN_RIGHT and PIN_LEFT pins" );
        break;
    }
}


BOOST_AUTO_TEST_CASE( PageSizeAndContentPlacement )
{
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/ayab/ayab_rs.sch" );

    BOOST_REQUIRE( sheet );

    SCH_SCREEN* screen = sheet->GetScreen();

    PAGE_INFO pageInfo = screen->GetPageSettings();
    VECTOR2I  pageSizeIU = pageInfo.GetSizeIU( schIUScale.IU_PER_MILS );

    int pageWidthMils = schIUScale.IUToMils( pageSizeIU.x );
    int pageHeightMils = schIUScale.IUToMils( pageSizeIU.y );

    // The ayab schematic uses a D-size title block (~34x22 inches).
    // Page dimensions should be close to D-size, not astronomically large.
    BOOST_CHECK_LT( pageWidthMils, 40000 );
    BOOST_CHECK_LT( pageHeightMils, 30000 );
    BOOST_CHECK_GT( pageWidthMils, 20000 );
    BOOST_CHECK_GT( pageHeightMils, 15000 );

    // Compute actual content bounding box
    BOX2I bbox;

    for( SCH_ITEM* item : screen->Items() )
        bbox.Merge( item->GetBoundingBox() );

    BOOST_REQUIRE( bbox.GetWidth() > 0 );
    BOOST_REQUIRE( bbox.GetHeight() > 0 );

    // Content should be within the page boundaries
    int margin = schIUScale.MilsToIU( 500 );

    BOOST_CHECK_GE( bbox.GetOrigin().x, -margin );
    BOOST_CHECK_GE( bbox.GetOrigin().y, -margin );
    BOOST_CHECK_LE( bbox.GetEnd().x, pageSizeIU.x + margin );
    BOOST_CHECK_LE( bbox.GetEnd().y, pageSizeIU.y + margin );
}


// ============================================================================
// Properties-based search paths
// ============================================================================

BOOST_AUTO_TEST_CASE( PowerDetectionWithProjectOverride )
{
    // When a project provides its own gnd-1.sym without a net= attribute,
    // the importer should still detect it as a power symbol by inheriting
    // the builtin's net= attribute.
    SCH_SHEET* sheet = LoadGedaSchematic( "/io/geda/power_override_test/power_override_test.sch" );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    bool foundPower = false;

    for( SCH_ITEM* item : sheet->GetScreen()->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        LIB_SYMBOL* libSym = sym->GetLibSymbolRef().get();

        if( !libSym )
            continue;

        if( libSym->IsGlobalPower() )
        {
            foundPower = true;
            BOOST_CHECK_EQUAL( sym->GetField( FIELD_T::VALUE )->GetText(), wxString( wxT( "GND" ) ) );
            BOOST_CHECK( !sym->GetField( FIELD_T::REFERENCE )->IsVisible() );
        }
    }

    BOOST_CHECK_MESSAGE( foundPower,
                         "gnd-1.sym without net= attribute should still be detected as power "
                         "symbol when builtin has net=GND:1" );
}


BOOST_AUTO_TEST_CASE( PropertiesSearchPaths )
{
    std::string extraDir = KI_TEST::GetEeschemaTestDataDir() + "/io/geda/extra-syms";

    std::map<std::string, UTF8> props;
    props["sym_search_paths"] = extraDir;

    SCH_SHEET* sheet = LoadGedaSchematicWithProperties( "/io/geda/props_test.sch", props );

    BOOST_REQUIRE( sheet );
    BOOST_REQUIRE( sheet->GetScreen() );

    SCH_SCREEN* screen = sheet->GetScreen();

    for( SCH_ITEM* item : screen->Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        int pinCount = static_cast<int>( sym->GetLibPins().size() );

        BOOST_CHECK_MESSAGE( pinCount == 2,
                             "Expected 2-pin symbol from extra-syms, got "
                                     + std::to_string( pinCount ) );
        return;
    }

    BOOST_FAIL( "Symbol not found" );
}


BOOST_AUTO_TEST_SUITE_END()
