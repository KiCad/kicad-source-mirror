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

/**
 * @file test_pcad_sch_import.cpp
 * P-CAD 2006 ACCEL_ASCII schematic import tests.
 */

#include "test_pcad_sch_import_fixture.h"

#include <lib_id.h>
#include <wx/arrstr.h>


BOOST_FIXTURE_TEST_SUITE( PcadSchImport, PCAD_SCH_IMPORT_FIXTURE )


// P-CAD saves are extension-agnostic and boards share the ACCEL_ASCII
// container, so detection is by content.
BOOST_AUTO_TEST_CASE( CanReadSchematic )
{
    BOOST_CHECK( m_plugin.CanReadSchematicFile( GetTestDataDir() + "pcad_feature_test.sch" ) );

    // library-only ACCEL file without a schematicDesign section
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( GetTestDataDir() + "pcad_library_test.lia" ) );

    // ACCEL board file
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( GetTestDataDir() + "pcad_board_reject.pcb" ) );

    // pre-ACCEL vintage P-CAD format
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( GetTestDataDir() + "pcad_vintage_reject.sch" ) );
}


BOOST_AUTO_TEST_CASE( FeatureTestCounts )
{
    LoadSchematic( "pcad_feature_test.sch" );

    // "Main" lands on the content root and "IO" becomes a subsheet
    BOOST_CHECK_EQUAL( CountImportedScreens(), 2 );
    BOOST_CHECK_EQUAL( CountItemsOfType( SCH_SHEET_T ), 1 );

    // U1 unit 1, U1 unit 2, GND1 and U2
    BOOST_CHECK_EQUAL( CountItemsOfType( SCH_SYMBOL_T ), 4 );

    // Main has DATA0 (1 seg + 2-seg polyline), SENSE_NET (2) and DB0 (1); IO has MM_NET (1)
    BOOST_CHECK_EQUAL( CountWires(), 7 );

    BOOST_CHECK_EQUAL( CountBusSegments(), 2 );
    BOOST_CHECK_EQUAL( CountItemsOfType( SCH_BUS_WIRE_ENTRY_T ), 2 );

    // each entry's slant must be chosen so its far end lands on its bus; the
    // Right entry disambiguates the vertical slant, the Up entry the horizontal
    forEachItemOfType( SCH_BUS_WIRE_ENTRY_T,
            []( SCH_ITEM* item, const SCH_SHEET_PATH& )
            {
                auto* entry = static_cast<SCH_BUS_WIRE_ENTRY*>( item );

                if( entry->GetPosition().x == schIUScale.MilsToIU( 6300 ) )
                {
                    BOOST_CHECK_EQUAL( entry->GetEnd().x, schIUScale.MilsToIU( 6400 ) );
                    BOOST_CHECK_EQUAL( entry->GetEnd().y, schIUScale.MilsToIU( 3900 ) );
                }
                else
                {
                    BOOST_CHECK_EQUAL( entry->GetPosition().x, schIUScale.MilsToIU( 8600 ) );
                    BOOST_CHECK_EQUAL( entry->GetEnd().x, schIUScale.MilsToIU( 8500 ) );
                    BOOST_CHECK_EQUAL( entry->GetEnd().y, schIUScale.MilsToIU( 2000 ) );
                }
            } );
    BOOST_CHECK_EQUAL( CountItemsOfType( SCH_JUNCTION_T ), 3 );

    // displayed net names DATA0 and MM_NET, undisplayed named net SENSE_NET,
    // bus names DBUS and HBUS
    BOOST_CHECK_EQUAL( CountItemsOfType( SCH_LABEL_T ), 5 );
    BOOST_CHECK( HasLabel( wxT( "DATA0" ) ) );
    BOOST_CHECK( HasLabel( wxT( "SENSE_NET" ) ) );
    BOOST_CHECK( HasLabel( wxT( "DBUS" ) ) );

    // ports become global labels
    BOOST_CHECK_EQUAL( CountItemsOfType( SCH_GLOBAL_LABEL_T ), 3 );
    BOOST_CHECK( HasLabel( wxT( "VCC" ), SCH_GLOBAL_LABEL_T ) );
    BOOST_CHECK( HasLabel( wxT( "DATA0" ), SCH_GLOBAL_LABEL_T ) );
    BOOST_CHECK( HasLabel( wxT( "MM_NET" ), SCH_GLOBAL_LABEL_T ) );

    // two free texts and the placed Title field on Main
    BOOST_CHECK_EQUAL( CountItemsOfType( SCH_TEXT_T ), 3 );

    // dotted polyline, two arcs, filled triangle and the IEEE Generator polyline
    BOOST_CHECK_EQUAL( CountItemsOfType( SCH_SHAPE_T ), 5 );
    BOOST_CHECK_EQUAL( CountShapesWithStyle( LINE_STYLE::DOT ), 1 );
}


BOOST_AUTO_TEST_CASE( MultiUnitComponent )
{
    LoadSchematic( "pcad_feature_test.sch" );

    SCH_SYMBOL* unit1 = SymbolForRefdesUnit( wxT( "U1" ), 1 );
    SCH_SYMBOL* unit2 = SymbolForRefdesUnit( wxT( "U1" ), 2 );

    BOOST_REQUIRE( unit1 && unit2 );

    LIB_SYMBOL* libSym = LibSymbolForRefdes( wxT( "U1" ) );
    BOOST_REQUIRE( libSym );
    BOOST_CHECK_EQUAL( libSym->GetUnitCount(), 2 );

    // symbol pin ordinals resolve to pad designators through the compPin mapping
    SCH_PIN* pin1 = LibPin( libSym, wxT( "1" ) );
    SCH_PIN* pin2 = LibPin( libSym, wxT( "2" ) );
    SCH_PIN* pin3 = LibPin( libSym, wxT( "3" ) );
    SCH_PIN* pin4 = LibPin( libSym, wxT( "4" ) );

    BOOST_REQUIRE( pin1 && pin2 && pin3 && pin4 );

    BOOST_CHECK( pin1->GetType() == ELECTRICAL_PINTYPE::PT_INPUT );
    BOOST_CHECK( pin2->GetType() == ELECTRICAL_PINTYPE::PT_OUTPUT );
    BOOST_CHECK( pin3->GetType() == ELECTRICAL_PINTYPE::PT_PASSIVE );
    BOOST_CHECK( pin4->GetType() == ELECTRICAL_PINTYPE::PT_TRISTATE );

    BOOST_CHECK_EQUAL( pin1->GetUnit(), 1 );
    BOOST_CHECK_EQUAL( pin3->GetUnit(), 2 );

    // outsideEdgeStyle Dot and insideEdgeStyle Clock
    BOOST_CHECK( pin1->GetShape() == GRAPHIC_PINSHAPE::INVERTED );
    BOOST_CHECK( pin2->GetShape() == GRAPHIC_PINSHAPE::CLOCK );

    BOOST_CHECK_EQUAL( libSym->GetFootprintField().GetText(), wxT( "DIP4" ) );
    BOOST_CHECK_EQUAL( libSym->GetDescription(), wxT( "Dual QA test gate" ) );
}


BOOST_AUTO_TEST_CASE( ComponentAlias )
{
    LoadSchematic( "pcad_feature_test.sch" );

    // U2 references compAlias GATE_ALIAS, which resolves to compDef GATE
    SCH_SYMBOL* u2 = SymbolForRefdesUnit( wxT( "U2" ), 1 );

    BOOST_REQUIRE( u2 );
    BOOST_CHECK_EQUAL( u2->GetField( FIELD_T::VALUE )->GetText(), wxT( "74HC02" ) );
    BOOST_CHECK_EQUAL( u2->GetLibId().GetLibItemName().wx_str(), wxT( "GATE" ) );
}


BOOST_AUTO_TEST_CASE( PowerSymbol )
{
    LoadSchematic( "pcad_feature_test.sch" );

    LIB_SYMBOL* gnd = LibSymbolForRefdes( wxT( "#PWR" ) );

    // compType Power maps to a KiCad power symbol with an invisible reference
    if( !gnd )
        gnd = LibSymbolForRefdes( wxT( "GND1" ) );

    BOOST_REQUIRE( gnd );
    BOOST_CHECK( gnd->IsPower() );

    SCH_PIN* pin = LibPin( gnd, wxT( "1" ) );
    BOOST_REQUIRE( pin );
    BOOST_CHECK( pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN );
    BOOST_CHECK( !pin->IsVisible() );
}


BOOST_AUTO_TEST_CASE( TitleBlock )
{
    LoadSchematic( "pcad_feature_test.sch" );

    const TITLE_BLOCK& tb = m_loadedRoot->GetScreen()->GetTitleBlock();

    BOOST_CHECK_EQUAL( tb.GetTitle(), wxT( "P-CAD import feature fixture" ) );
    BOOST_CHECK_EQUAL( tb.GetRevision(), wxT( "A" ) );
    BOOST_CHECK_EQUAL( tb.GetDate(), wxT( "2026-07-03" ) );
    BOOST_CHECK_EQUAL( tb.GetCompany(), wxT( "KiCad Project QA" ) );
}


BOOST_AUTO_TEST_CASE( TextFormatting )
{
    LoadSchematic( "pcad_feature_test.sch" );

    // TrueType style with fontWeight 700 and fontItalic, rotated 90 degrees
    SCH_TEXT* bold = FindText( wxT( "Bold TTF note" ) );

    BOOST_REQUIRE( bold );
    BOOST_CHECK( bold->IsBold() );
    BOOST_CHECK( bold->IsItalic() );
    BOOST_CHECK( bold->GetTextAngle() == EDA_ANGLE( 90.0, DEGREES_T ) );

    SCH_TEXT* stroke = FindText( wxT( "Stroke note" ) );

    BOOST_REQUIRE( stroke );
    BOOST_CHECK( !stroke->IsBold() );

    // placed title-block field renders with the fieldSet value
    BOOST_CHECK( FindText( wxT( "P-CAD import feature fixture" ) ) );
}


BOOST_AUTO_TEST_CASE( MetricUnits )
{
    LoadSchematic( "pcad_feature_test.sch" );

    // The IO sheet junction sits at (25.4mm, 127mm) in a mil-unit file; with
    // the 11000 mil page the KiCad position is (1000, 6000) mils.
    bool found = false;

    forEachItemOfType( SCH_JUNCTION_T,
            [&]( SCH_ITEM* item, const SCH_SHEET_PATH& )
            {
                if( item->GetPosition() == VECTOR2I( schIUScale.MilsToIU( 1000 ),
                                                     schIUScale.MilsToIU( 6000 ) ) )
                    found = true;
            } );

    BOOST_CHECK( found );
}


BOOST_AUTO_TEST_CASE( EnumerateLibrary )
{
    wxArrayString names;

    m_plugin.EnumerateSymbolLib( names, GetTestDataDir() + "pcad_library_test.lia" );

    BOOST_REQUIRE_EQUAL( names.size(), 1 );
    BOOST_CHECK_EQUAL( names[0], wxT( "RES_QA" ) );

    // LoadSymbol returns a plugin-cache-owned pointer
    LIB_SYMBOL* sym = m_plugin.LoadSymbol( GetTestDataDir() + "pcad_library_test.lia",
                                           wxT( "RES_QA" ) );

    BOOST_REQUIRE( sym );

    // repeated loads must serve the identical cached object
    BOOST_CHECK_EQUAL( sym, m_plugin.LoadSymbol( GetTestDataDir() + "pcad_library_test.lia",
                                                 wxT( "RES_QA" ) ) );
    BOOST_CHECK_EQUAL( sym->GetPins().size(), 2 );
    BOOST_CHECK_EQUAL( sym->GetReferenceField().GetText(), wxT( "R" ) );
    BOOST_CHECK_EQUAL( sym->GetFootprintField().GetText(), wxT( "RES0805" ) );

    // in the metric file the 15.24mm wide body spans 600 mils
    SCH_PIN* pin1 = LibPin( sym, wxT( "1" ) );
    SCH_PIN* pin2 = LibPin( sym, wxT( "2" ) );

    BOOST_REQUIRE( pin1 && pin2 );

    int span = std::abs( pin2->GetPosition().x - pin1->GetPosition().x );

    // pins extend 5.08mm (200 mil) past each body end, so 600 + 2*200 mils
    BOOST_CHECK_EQUAL( span, schIUScale.MilsToIU( 1000 ) );
}

BOOST_AUTO_TEST_SUITE_END()
