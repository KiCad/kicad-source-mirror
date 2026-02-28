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

/**
 * @file test_pads_sch_parser.cpp
 * Test suite for PADS_SCH::PADS_SCH_PARSER
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa_utils/wx_utils/wx_assert.h>

#include <sch_io/pads/pads_sch_parser.h>
#include <sch_io/pads/pads_sch_symbol_builder.h>
#include <lib_symbol.h>
#include <sch_shape.h>
#include <sch_pin.h>


BOOST_AUTO_TEST_SUITE( PadsSchParser )


BOOST_AUTO_TEST_CASE( CheckFileHeader_ValidLogicFile )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    BOOST_CHECK( PADS_SCH::PADS_SCH_PARSER::CheckFileHeader( testFile ) );
}


BOOST_AUTO_TEST_CASE( CheckFileHeader_ValidPowerLogicFile )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/powerlogic_schematic.txt";

    BOOST_CHECK( PADS_SCH::PADS_SCH_PARSER::CheckFileHeader( testFile ) );
}


BOOST_AUTO_TEST_CASE( CheckFileHeader_InvalidFile )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/nonexistent.txt";

    BOOST_CHECK( !PADS_SCH::PADS_SCH_PARSER::CheckFileHeader( testFile ) );
}


BOOST_AUTO_TEST_CASE( ParseHeader_LogicFormat )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );
    BOOST_CHECK( parser.IsValid() );

    const auto& header = parser.GetHeader();

    BOOST_CHECK_EQUAL( header.product, "PADS-LOGIC" );
    BOOST_CHECK_EQUAL( header.version, "V9.0" );
    BOOST_CHECK( header.description.find( "DESIGN EXPORT FILE" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( ParseHeader_PowerLogicFormat )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/powerlogic_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );
    BOOST_CHECK( parser.IsValid() );

    const auto& header = parser.GetHeader();

    BOOST_CHECK_EQUAL( header.product, "PADS-POWERLOGIC" );
    BOOST_CHECK_EQUAL( header.version, "V9.5" );
}


BOOST_AUTO_TEST_CASE( ParseParameters_Units_Mils )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& params = parser.GetParameters();

    BOOST_CHECK( params.units == PADS_SCH::UNIT_TYPE::MILS );
    BOOST_CHECK_EQUAL( params.grid_x, 100.0 );
    BOOST_CHECK_EQUAL( params.grid_y, 100.0 );
    BOOST_CHECK_EQUAL( params.border_template, "Default_A" );
}


BOOST_AUTO_TEST_CASE( ParseParameters_Units_Metric )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/powerlogic_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& params = parser.GetParameters();

    BOOST_CHECK( params.units == PADS_SCH::UNIT_TYPE::METRIC );
}


BOOST_AUTO_TEST_CASE( GetVersion )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );
    BOOST_CHECK_EQUAL( parser.GetVersion(), "V9.0" );
}


BOOST_AUTO_TEST_CASE( ParseParameters_JobName )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& params = parser.GetParameters();

    BOOST_CHECK_EQUAL( params.job_name, "Test Design" );
}


BOOST_AUTO_TEST_CASE( ParseParameters_SheetSize )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& params = parser.GetParameters();

    BOOST_CHECK_EQUAL( params.sheet_size.width, 11000.0 );
    BOOST_CHECK_EQUAL( params.sheet_size.height, 8500.0 );
    BOOST_CHECK_EQUAL( params.sheet_size.name, "A" );
}


BOOST_AUTO_TEST_CASE( ParseParameters_TextAndLineDefaults )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& params = parser.GetParameters();

    BOOST_CHECK_EQUAL( params.text_size, 60.0 );
    BOOST_CHECK_EQUAL( params.line_width, 2.0 );
}


BOOST_AUTO_TEST_CASE( ParseSymbols_Count )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& symbols = parser.GetSymbolDefs();

    BOOST_CHECK_EQUAL( symbols.size(), 3 );
}


BOOST_AUTO_TEST_CASE( ParseSymbols_V52_DecalWithoutFontLines )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/v52_decals.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );
    BOOST_CHECK( parser.IsValid() );

    const auto& header = parser.GetHeader();

    BOOST_CHECK_EQUAL( header.product, "PADS-POWERLOGIC" );
    BOOST_CHECK_EQUAL( header.version, "V5.2" );

    const auto& symbols = parser.GetSymbolDefs();

    BOOST_REQUIRE_EQUAL( symbols.size(), 1 );

    const PADS_SCH::SYMBOL_DEF* pinb = parser.GetSymbolDef( "PINB" );
    BOOST_REQUIRE( pinb != nullptr );

    BOOST_CHECK_EQUAL( pinb->num_attrs, 4 );
    BOOST_CHECK_EQUAL( pinb->num_pieces, 2 );
    BOOST_CHECK_EQUAL( pinb->num_pins, 0 );
    BOOST_CHECK_EQUAL( pinb->is_pin_decal, 1 );

    // Verify attribute names were parsed correctly (not misaligned)
    BOOST_REQUIRE_EQUAL( pinb->attrs.size(), 4 );
    BOOST_CHECK_EQUAL( pinb->attrs[0].attr_name, "REF-DES" );
    BOOST_CHECK_EQUAL( pinb->attrs[1].attr_name, "PART-TYPE" );
    BOOST_CHECK_EQUAL( pinb->attrs[2].attr_name, "*" );
    BOOST_CHECK_EQUAL( pinb->attrs[3].attr_name, "*" );

    // Verify graphics were parsed correctly
    BOOST_REQUIRE_EQUAL( pinb->graphics.size(), 2 );

    const auto& openLine = pinb->graphics[0];
    BOOST_CHECK( openLine.type == PADS_SCH::GRAPHIC_TYPE::POLYLINE );
    BOOST_CHECK_EQUAL( openLine.line_width, 10.0 );
    BOOST_REQUIRE_EQUAL( openLine.points.size(), 2 );
    BOOST_CHECK_EQUAL( openLine.points[0].coord.x, 0.0 );
    BOOST_CHECK_EQUAL( openLine.points[0].coord.y, 0.0 );
    BOOST_CHECK_EQUAL( openLine.points[1].coord.x, 140.0 );
    BOOST_CHECK_EQUAL( openLine.points[1].coord.y, 0.0 );

    const auto& circle = pinb->graphics[1];
    BOOST_CHECK( circle.type == PADS_SCH::GRAPHIC_TYPE::CIRCLE );
    BOOST_CHECK_EQUAL( circle.line_width, 10.0 );
    BOOST_CHECK_EQUAL( circle.center.x, 165.0 );
    BOOST_CHECK_EQUAL( circle.center.y, 0.0 );
    BOOST_CHECK_EQUAL( circle.radius, 25.0 );

    // Font names should be empty since V5.2 doesn't have them
    BOOST_CHECK( pinb->font1.empty() );
    BOOST_CHECK( pinb->font2.empty() );
}


BOOST_AUTO_TEST_CASE( ParseSymbols_Resistor )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SYMBOL_DEF* res = parser.GetSymbolDef( "RES_0805" );
    BOOST_REQUIRE( res != nullptr );

    BOOST_CHECK_EQUAL( res->name, "RES_0805" );
    BOOST_CHECK_EQUAL( res->gate_count, 1 );
    BOOST_CHECK_EQUAL( res->graphics.size(), 4 );
    BOOST_CHECK_EQUAL( res->pins.size(), 2 );
}


BOOST_AUTO_TEST_CASE( ParseSymbols_ResistorPins )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SYMBOL_DEF* res = parser.GetSymbolDef( "RES_0805" );
    BOOST_REQUIRE( res != nullptr );
    BOOST_REQUIRE( res->pins.size() >= 2 );

    // CAEDECAL pins have placeholder numbers and empty names.
    // Actual pin data comes from PARTTYPE GATE_DEF at build time.
    const auto& pin1 = res->pins[0];
    BOOST_CHECK_EQUAL( pin1.number, "1" );
    BOOST_CHECK_EQUAL( pin1.position.x, -200.0 );
    BOOST_CHECK_EQUAL( pin1.position.y, 0.0 );
    BOOST_CHECK( pin1.type == PADS_SCH::PIN_TYPE::UNSPECIFIED );

    const auto& pin2 = res->pins[1];
    BOOST_CHECK_EQUAL( pin2.number, "2" );
    BOOST_CHECK_EQUAL( pin2.position.x, 200.0 );
    BOOST_CHECK_EQUAL( pin2.position.y, 0.0 );
    BOOST_CHECK( pin2.type == PADS_SCH::PIN_TYPE::UNSPECIFIED );

    // Verify PARTTYPE provides the actual pin numbers/names
    auto ptIt = parser.GetPartTypes().find( "RES_0805" );
    BOOST_REQUIRE( ptIt != parser.GetPartTypes().end() );
    BOOST_REQUIRE( !ptIt->second.gates.empty() );

    const auto& gate = ptIt->second.gates[0];
    BOOST_REQUIRE_GE( gate.pins.size(), 2u );
    BOOST_CHECK_EQUAL( gate.pins[0].pin_id, "1" );
    BOOST_CHECK_EQUAL( gate.pins[0].pin_name, "1" );
    BOOST_CHECK_EQUAL( gate.pins[1].pin_id, "2" );
    BOOST_CHECK_EQUAL( gate.pins[1].pin_name, "2" );
}


BOOST_AUTO_TEST_CASE( ParseSymbols_Capacitor )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SYMBOL_DEF* cap = parser.GetSymbolDef( "CAP_0603" );
    BOOST_REQUIRE( cap != nullptr );

    BOOST_CHECK_EQUAL( cap->name, "CAP_0603" );
    BOOST_CHECK_EQUAL( cap->gate_count, 1 );
    BOOST_CHECK_EQUAL( cap->graphics.size(), 2 );
    BOOST_CHECK_EQUAL( cap->pins.size(), 2 );
}


BOOST_AUTO_TEST_CASE( ParseSymbols_IC_MultiGate )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SYMBOL_DEF* ic = parser.GetSymbolDef( "IC_QUAD_NAND" );
    BOOST_REQUIRE( ic != nullptr );

    BOOST_CHECK_EQUAL( ic->name, "IC_QUAD_NAND" );
    BOOST_CHECK_EQUAL( ic->graphics.size(), 9 );
    BOOST_CHECK_EQUAL( ic->pins.size(), 14 );

    // gate_count on SYMBOL_DEF stays at default (1). Multi-gate info lives in PARTTYPE.
    auto ptIt = parser.GetPartTypes().find( "IC_QUAD_NAND" );
    BOOST_REQUIRE( ptIt != parser.GetPartTypes().end() );
    BOOST_CHECK_EQUAL( ptIt->second.num_physical, 4 );
}


BOOST_AUTO_TEST_CASE( ParseSymbols_IC_PinTypes )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    // Pin names/types on SYMBOL_DEF are raw CAEDECAL data (empty/unspecified).
    // Actual pin data lives in PARTTYPE GATE_DEF and is applied at build time.
    auto ptIt = parser.GetPartTypes().find( "IC_QUAD_NAND" );
    BOOST_REQUIRE( ptIt != parser.GetPartTypes().end() );
    BOOST_REQUIRE( !ptIt->second.gates.empty() );

    const auto& pins = ptIt->second.gates[0].pins;
    BOOST_REQUIRE_GE( pins.size(), 14u );

    // Check input pins
    BOOST_CHECK_EQUAL( pins[0].pin_name, "1A" );
    BOOST_CHECK_EQUAL( pins[0].pin_type, 'L' );

    // Check output pins
    BOOST_CHECK_EQUAL( pins[2].pin_name, "1Y" );
    BOOST_CHECK_EQUAL( pins[2].pin_type, 'S' );

    // Check power pins
    BOOST_CHECK_EQUAL( pins[6].pin_name, "GND" );
    BOOST_CHECK_EQUAL( pins[6].pin_type, 'G' );
}


BOOST_AUTO_TEST_CASE( ParseSymbols_Graphics )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SYMBOL_DEF* res = parser.GetSymbolDef( "RES_0805" );
    BOOST_REQUIRE( res != nullptr );
    BOOST_REQUIRE( res->graphics.size() >= 1 );

    // First graphic should be a rectangle (CLOSED)
    const auto& rect = res->graphics[0];
    BOOST_CHECK( rect.type == PADS_SCH::GRAPHIC_TYPE::RECTANGLE );
    BOOST_CHECK_EQUAL( rect.line_width, 10.0 );
    BOOST_REQUIRE( rect.points.size() >= 2 );
    BOOST_CHECK_EQUAL( rect.points[0].coord.x, -100.0 );
    BOOST_CHECK_EQUAL( rect.points[0].coord.y, -50.0 );
    BOOST_CHECK_EQUAL( rect.points[1].coord.x, 100.0 );
    BOOST_CHECK_EQUAL( rect.points[1].coord.y, 50.0 );
}


BOOST_AUTO_TEST_CASE( GetSymbolDef_NotFound )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SYMBOL_DEF* notFound = parser.GetSymbolDef( "NONEXISTENT_SYMBOL" );
    BOOST_CHECK( notFound == nullptr );
}


BOOST_AUTO_TEST_CASE( ParseSymbols_EmptySection )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& symbols = parser.GetSymbolDefs();
    BOOST_CHECK( symbols.empty() );
}


BOOST_AUTO_TEST_CASE( ParseParts_Count )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/parts_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& parts = parser.GetPartPlacements();

    BOOST_CHECK_EQUAL( parts.size(), 5 );
}


BOOST_AUTO_TEST_CASE( ParseParts_Resistor )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/parts_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::PART_PLACEMENT* r1 = parser.GetPartPlacement( "R1" );
    BOOST_REQUIRE( r1 != nullptr );

    BOOST_CHECK_EQUAL( r1->reference, "R1" );
    BOOST_CHECK_EQUAL( r1->symbol_name, "RES_0805" );
    BOOST_CHECK_EQUAL( r1->position.x, 1000.0 );
    BOOST_CHECK_EQUAL( r1->position.y, 2000.0 );
    BOOST_CHECK_EQUAL( r1->rotation, 0.0 );
    BOOST_CHECK_EQUAL( r1->mirror_flags, 0 );
    BOOST_CHECK_EQUAL( r1->sheet_number, 1 );
}


BOOST_AUTO_TEST_CASE( ParseParts_RotatedPart )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/parts_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::PART_PLACEMENT* r2 = parser.GetPartPlacement( "R2" );
    BOOST_REQUIRE( r2 != nullptr );

    BOOST_CHECK_EQUAL( r2->rotation, 90.0 );
    BOOST_CHECK_EQUAL( r2->mirror_flags, 0 );
}


BOOST_AUTO_TEST_CASE( ParseParts_MirroredPart )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/parts_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::PART_PLACEMENT* c1 = parser.GetPartPlacement( "C1" );
    BOOST_REQUIRE( c1 != nullptr );

    BOOST_CHECK_NE( c1->mirror_flags, 0 );
}


BOOST_AUTO_TEST_CASE( ParseParts_Attributes )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/parts_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::PART_PLACEMENT* r1 = parser.GetPartPlacement( "R1" );
    BOOST_REQUIRE( r1 != nullptr );
    BOOST_CHECK_EQUAL( r1->attributes.size(), 2 );

    // Check Ref.Des. attribute
    bool foundRefDes = false;
    bool foundValue = false;

    for( const auto& attr : r1->attributes )
    {
        if( attr.name == "Ref.Des." )
        {
            BOOST_CHECK_EQUAL( attr.value, "R1" );
            BOOST_CHECK( attr.visible );
            foundRefDes = true;
        }
        else if( attr.name == "Value" )
        {
            BOOST_CHECK_EQUAL( attr.value, "10K" );
            BOOST_CHECK( attr.visible );
            foundValue = true;
        }
    }

    BOOST_CHECK( foundRefDes );
    BOOST_CHECK( foundValue );
}


BOOST_AUTO_TEST_CASE( ParseParts_AttributeVisibility )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/parts_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::PART_PLACEMENT* r2 = parser.GetPartPlacement( "R2" );
    BOOST_REQUIRE( r2 != nullptr );

    // R2 has Value attribute with visibility N (hidden)
    for( const auto& attr : r2->attributes )
    {
        if( attr.name == "Value" )
        {
            BOOST_CHECK( !attr.visible );
            break;
        }
    }
}


BOOST_AUTO_TEST_CASE( ParseParts_IC_MultipleAttributes )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/parts_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::PART_PLACEMENT* u1 = parser.GetPartPlacement( "U1" );
    BOOST_REQUIRE( u1 != nullptr );

    BOOST_CHECK_EQUAL( u1->symbol_name, "IC_QUAD_NAND" );
    BOOST_CHECK_EQUAL( u1->attributes.size(), 4 );
}


BOOST_AUTO_TEST_CASE( ParseParts_MultiGatePart )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/parts_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::PART_PLACEMENT* u1a = parser.GetPartPlacement( "U1.A" );
    BOOST_REQUIRE( u1a != nullptr );

    BOOST_CHECK_EQUAL( u1a->symbol_name, "IC_QUAD_NAND" );
    BOOST_CHECK_EQUAL( u1a->gate_number, 2 );
}


BOOST_AUTO_TEST_CASE( GetPartPlacement_NotFound )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/parts_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::PART_PLACEMENT* notFound = parser.GetPartPlacement( "NONEXISTENT" );
    BOOST_CHECK( notFound == nullptr );
}


BOOST_AUTO_TEST_CASE( ParseParts_EmptySection )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& parts = parser.GetPartPlacements();
    BOOST_CHECK( parts.empty() );
}


BOOST_AUTO_TEST_CASE( ParseSignals_Count )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/signals_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& signals = parser.GetSignals();

    BOOST_CHECK_EQUAL( signals.size(), 4 );
}


BOOST_AUTO_TEST_CASE( ParseSignals_VCC )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/signals_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SCH_SIGNAL* vcc = parser.GetSignal( "VCC" );
    BOOST_REQUIRE( vcc != nullptr );

    BOOST_CHECK_EQUAL( vcc->name, "VCC" );
    BOOST_CHECK_EQUAL( vcc->connections.size(), 2 );
    BOOST_CHECK_EQUAL( vcc->wires.size(), 2 );
}


BOOST_AUTO_TEST_CASE( ParseSignals_PinConnections )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/signals_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SCH_SIGNAL* vcc = parser.GetSignal( "VCC" );
    BOOST_REQUIRE( vcc != nullptr );
    BOOST_REQUIRE( vcc->connections.size() >= 2 );

    // Check first connection: R1.1
    BOOST_CHECK_EQUAL( vcc->connections[0].reference, "R1" );
    BOOST_CHECK_EQUAL( vcc->connections[0].pin_number, "1" );

    // Check second connection: U1.14
    BOOST_CHECK_EQUAL( vcc->connections[1].reference, "U1" );
    BOOST_CHECK_EQUAL( vcc->connections[1].pin_number, "14" );
}


BOOST_AUTO_TEST_CASE( ParseSignals_WireSegments )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/signals_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SCH_SIGNAL* vcc = parser.GetSignal( "VCC" );
    BOOST_REQUIRE( vcc != nullptr );
    BOOST_REQUIRE( vcc->wires.size() >= 2 );

    // Check first wire segment
    const auto& wire1 = vcc->wires[0];
    BOOST_CHECK_EQUAL( wire1.start.x, 1000.0 );
    BOOST_CHECK_EQUAL( wire1.start.y, 2000.0 );
    BOOST_CHECK_EQUAL( wire1.end.x, 2000.0 );
    BOOST_CHECK_EQUAL( wire1.end.y, 2000.0 );
    BOOST_CHECK_EQUAL( wire1.sheet_number, 1 );

    // Check second wire segment
    const auto& wire2 = vcc->wires[1];
    BOOST_CHECK_EQUAL( wire2.start.x, 2000.0 );
    BOOST_CHECK_EQUAL( wire2.start.y, 2000.0 );
    BOOST_CHECK_EQUAL( wire2.end.x, 2000.0 );
    BOOST_CHECK_EQUAL( wire2.end.y, 3000.0 );
}


BOOST_AUTO_TEST_CASE( ParseSignals_MultipleConnections )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/signals_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SCH_SIGNAL* net1 = parser.GetSignal( "NET1" );
    BOOST_REQUIRE( net1 != nullptr );

    // NET1 connects R1.2, R2.1, U1.1, U1.2
    BOOST_CHECK_EQUAL( net1->connections.size(), 4 );
    BOOST_CHECK_EQUAL( net1->wires.size(), 4 );
}


BOOST_AUTO_TEST_CASE( ParseSignals_SingleConnection )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/signals_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SCH_SIGNAL* output = parser.GetSignal( "OUTPUT" );
    BOOST_REQUIRE( output != nullptr );

    BOOST_CHECK_EQUAL( output->connections.size(), 1 );
    BOOST_CHECK_EQUAL( output->connections[0].reference, "U1" );
    BOOST_CHECK_EQUAL( output->connections[0].pin_number, "3" );
}


BOOST_AUTO_TEST_CASE( GetSignal_NotFound )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/signals_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const PADS_SCH::SCH_SIGNAL* notFound = parser.GetSignal( "NONEXISTENT" );
    BOOST_CHECK( notFound == nullptr );
}


BOOST_AUTO_TEST_CASE( ParseSignals_EmptySection )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& signals = parser.GetSignals();
    BOOST_CHECK( signals.empty() );
}


BOOST_AUTO_TEST_CASE( SymbolBuilder_CreateSymbol )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PADS_SCH_SYMBOL_BUILDER builder( parser.GetParameters() );

    const PADS_SCH::SYMBOL_DEF* resDef = parser.GetSymbolDef( "RES_0805" );
    BOOST_REQUIRE( resDef != nullptr );

    LIB_SYMBOL* symbol = builder.BuildSymbol( *resDef );
    BOOST_REQUIRE( symbol != nullptr );

    BOOST_CHECK_EQUAL( symbol->GetName(), "RES_0805" );

    delete symbol;
}


BOOST_AUTO_TEST_CASE( SymbolBuilder_SymbolHasGraphics )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PADS_SCH_SYMBOL_BUILDER builder( parser.GetParameters() );

    const PADS_SCH::SYMBOL_DEF* resDef = parser.GetSymbolDef( "RES_0805" );
    BOOST_REQUIRE( resDef != nullptr );

    LIB_SYMBOL* symbol = builder.BuildSymbol( *resDef );
    BOOST_REQUIRE( symbol != nullptr );

    // Count graphics (shapes) in the symbol
    int shapeCount = 0;

    for( const SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( item.Type() == SCH_SHAPE_T )
            shapeCount++;
    }

    BOOST_CHECK_EQUAL( shapeCount, 4 );

    delete symbol;
}


BOOST_AUTO_TEST_CASE( SymbolBuilder_SymbolHasPins )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PADS_SCH_SYMBOL_BUILDER builder( parser.GetParameters() );

    const PADS_SCH::SYMBOL_DEF* resDef = parser.GetSymbolDef( "RES_0805" );
    BOOST_REQUIRE( resDef != nullptr );

    LIB_SYMBOL* symbol = builder.BuildSymbol( *resDef );
    BOOST_REQUIRE( symbol != nullptr );

    // Count pins in the symbol
    int pinCount = 0;

    for( const SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( item.Type() == SCH_PIN_T )
            pinCount++;
    }

    BOOST_CHECK_EQUAL( pinCount, 2 );

    delete symbol;
}


BOOST_AUTO_TEST_CASE( SymbolBuilder_PinProperties )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PADS_SCH_SYMBOL_BUILDER builder( parser.GetParameters() );

    const PADS_SCH::SYMBOL_DEF* resDef = parser.GetSymbolDef( "RES_0805" );
    BOOST_REQUIRE( resDef != nullptr );

    LIB_SYMBOL* symbol = builder.BuildSymbol( *resDef );
    BOOST_REQUIRE( symbol != nullptr );

    // Find a pin and verify its properties
    const SCH_PIN* pin1 = nullptr;

    for( const SCH_ITEM& item : symbol->GetDrawItems() )
    {
        if( item.Type() == SCH_PIN_T )
        {
            const SCH_PIN* pin = static_cast<const SCH_PIN*>( &item );

            if( pin->GetNumber() == "1" )
            {
                pin1 = pin;
                break;
            }
        }
    }

    BOOST_REQUIRE( pin1 != nullptr );
    BOOST_CHECK_EQUAL( pin1->GetNumber(), "1" );
    BOOST_CHECK( pin1->GetType() == ELECTRICAL_PINTYPE::PT_UNSPECIFIED );

    delete symbol;
}


BOOST_AUTO_TEST_CASE( SymbolBuilder_CacheSymbol )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/symbols_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PADS_SCH_SYMBOL_BUILDER builder( parser.GetParameters() );

    const PADS_SCH::SYMBOL_DEF* resDef = parser.GetSymbolDef( "RES_0805" );
    BOOST_REQUIRE( resDef != nullptr );

    // First call creates the symbol
    BOOST_CHECK( !builder.HasSymbol( "RES_0805" ) );

    LIB_SYMBOL* symbol1 = builder.GetOrCreateSymbol( *resDef );
    BOOST_REQUIRE( symbol1 != nullptr );
    BOOST_CHECK( builder.HasSymbol( "RES_0805" ) );

    // Second call returns the same symbol
    LIB_SYMBOL* symbol2 = builder.GetOrCreateSymbol( *resDef );
    BOOST_CHECK_EQUAL( symbol1, symbol2 );
}


BOOST_AUTO_TEST_CASE( PowerSymbol_GroundVariants )
{
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "GND" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "gnd" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "AGND" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "DGND" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "PGND" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "VSS" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "0V" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "EARTH" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "CHASSIS" ) );
}


BOOST_AUTO_TEST_CASE( PowerSymbol_SupplyVariants )
{
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "VCC" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "vcc" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "VDD" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "VEE" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "VPP" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "VBAT" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "VBUS" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "V+" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "V-" ) );
}


BOOST_AUTO_TEST_CASE( PowerSymbol_VoltagePatterns )
{
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "+5V" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "+3V3" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "+12V" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "-12V" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "+1V8" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "+24V" ) );
}


BOOST_AUTO_TEST_CASE( PowerSymbol_NonPower )
{
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "R1" ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "U1" ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "NET1" ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "DATA" ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "CLK" ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SYMBOL_BUILDER::IsPowerSymbol( "RESET" ) );
}


BOOST_AUTO_TEST_CASE( PowerSymbol_KiCadMapping_Ground )
{
    auto gnd = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "GND" );
    BOOST_REQUIRE( gnd.has_value() );
    BOOST_CHECK_EQUAL( gnd->GetLibNickname().wx_str(), "power" );
    BOOST_CHECK_EQUAL( gnd->GetLibItemName().wx_str(), "GND" );

    auto agnd = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "AGND" );
    BOOST_REQUIRE( agnd.has_value() );
    BOOST_CHECK_EQUAL( agnd->GetLibItemName().wx_str(), "GND" );

    auto dgnd = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "DGND" );
    BOOST_REQUIRE( dgnd.has_value() );
    BOOST_CHECK_EQUAL( dgnd->GetLibItemName().wx_str(), "GNDD" );
}


BOOST_AUTO_TEST_CASE( PowerSymbol_KiCadMapping_Supply )
{
    auto vcc = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "VCC" );
    BOOST_REQUIRE( vcc.has_value() );
    BOOST_CHECK_EQUAL( vcc->GetLibNickname().wx_str(), "power" );
    BOOST_CHECK_EQUAL( vcc->GetLibItemName().wx_str(), "VCC" );

    auto vdd = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "VDD" );
    BOOST_REQUIRE( vdd.has_value() );
    BOOST_CHECK_EQUAL( vdd->GetLibItemName().wx_str(), "VDD" );

    auto v5 = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "+5V" );
    BOOST_REQUIRE( v5.has_value() );
    BOOST_CHECK_EQUAL( v5->GetLibItemName().wx_str(), "+5V" );
}


BOOST_AUTO_TEST_CASE( PowerSymbol_KiCadMapping_NotFound )
{
    auto notPower = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "R1" );
    BOOST_CHECK( !notPower.has_value() );

    auto notPower2 = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "CLK" );
    BOOST_CHECK( !notPower2.has_value() );
}


BOOST_AUTO_TEST_CASE( PowerSymbol_KiCadMapping_GenericFallback )
{
    // +V* names not in the explicit table should fall back to VCC
    auto plusV = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "+V1" );
    BOOST_REQUIRE( plusV.has_value() );
    BOOST_CHECK_EQUAL( plusV->GetLibItemName().wx_str(), "VCC" );

    auto plusV7 = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "+7V5" );
    BOOST_REQUIRE( plusV7.has_value() );
    BOOST_CHECK_EQUAL( plusV7->GetLibItemName().wx_str(), "VCC" );

    // -V* names should fall back to VEE
    auto minusV = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "-V1" );
    BOOST_REQUIRE( minusV.has_value() );
    BOOST_CHECK_EQUAL( minusV->GetLibItemName().wx_str(), "VEE" );
}


BOOST_AUTO_TEST_CASE( PowerSymbol_CaseInsensitive )
{
    auto gndLower = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "gnd" );
    BOOST_REQUIRE( gndLower.has_value() );
    BOOST_CHECK_EQUAL( gndLower->GetLibItemName().wx_str(), "GND" );

    auto vccMixed = PADS_SCH::PADS_SCH_SYMBOL_BUILDER::GetKiCadPowerSymbolId( "Vcc" );
    BOOST_REQUIRE( vccMixed.has_value() );
    BOOST_CHECK_EQUAL( vccMixed->GetLibItemName().wx_str(), "VCC" );
}


BOOST_AUTO_TEST_CASE( BuildKiCadPowerSymbol_Styles )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;
    PADS_SCH::PADS_SCH_SYMBOL_BUILDER builder( params );

    // GND style: chevron ground, pin facing down
    std::unique_ptr<LIB_SYMBOL> gnd( builder.BuildKiCadPowerSymbol( "GND" ) );
    BOOST_REQUIRE( gnd != nullptr );
    BOOST_CHECK( gnd->IsPower() );
    BOOST_CHECK_EQUAL( gnd->GetName(), "GND" );

    // VCC style: arrow up, pin facing up
    std::unique_ptr<LIB_SYMBOL> vcc( builder.BuildKiCadPowerSymbol( "VCC" ) );
    BOOST_REQUIRE( vcc != nullptr );
    BOOST_CHECK( vcc->IsPower() );
    BOOST_CHECK_EQUAL( vcc->GetName(), "VCC" );

    // VEE style: inverted arrow, pin facing down
    std::unique_ptr<LIB_SYMBOL> vee( builder.BuildKiCadPowerSymbol( "VEE" ) );
    BOOST_REQUIRE( vee != nullptr );
    BOOST_CHECK( vee->IsPower() );

    // GNDD style: thick bar ground
    std::unique_ptr<LIB_SYMBOL> gndd( builder.BuildKiCadPowerSymbol( "GNDD" ) );
    BOOST_REQUIRE( gndd != nullptr );
    BOOST_CHECK( gndd->IsPower() );

    // Earth style: descending bars
    std::unique_ptr<LIB_SYMBOL> earth( builder.BuildKiCadPowerSymbol( "Earth" ) );
    BOOST_REQUIRE( earth != nullptr );
    BOOST_CHECK( earth->IsPower() );

    // PWR_BAR style: thick bar pointing up (for positive rail symbols like +V1)
    std::unique_ptr<LIB_SYMBOL> pwrBar( builder.BuildKiCadPowerSymbol( "PWR_BAR" ) );
    BOOST_REQUIRE( pwrBar != nullptr );
    BOOST_CHECK( pwrBar->IsPower() );

    // PWR_TRIANGLE style: filled triangle pointing up (for arrow symbols like +V2)
    std::unique_ptr<LIB_SYMBOL> pwrTri( builder.BuildKiCadPowerSymbol( "PWR_TRIANGLE" ) );
    BOOST_REQUIRE( pwrTri != nullptr );
    BOOST_CHECK( pwrTri->IsPower() );

    // Verify each symbol has exactly one pin
    BOOST_CHECK_EQUAL( gnd->GetPins().size(), 1u );
    BOOST_CHECK_EQUAL( vcc->GetPins().size(), 1u );
    BOOST_CHECK_EQUAL( vee->GetPins().size(), 1u );
    BOOST_CHECK_EQUAL( gndd->GetPins().size(), 1u );
    BOOST_CHECK_EQUAL( earth->GetPins().size(), 1u );
    BOOST_CHECK_EQUAL( pwrBar->GetPins().size(), 1u );
    BOOST_CHECK_EQUAL( pwrTri->GetPins().size(), 1u );
}


BOOST_AUTO_TEST_CASE( PowerStyleFromVariant )
{
    using PADS_SCH::PADS_SCH_SYMBOL_BUILDER;

    // Ground variants
    BOOST_CHECK_EQUAL( PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant( "GND", "G" ), "GND" );
    BOOST_CHECK_EQUAL( PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant( "AGND", "G" ), "GND" );
    BOOST_CHECK_EQUAL( PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant( "CHGND", "G" ), "Chassis" );

    // Rail variants (thick bar)
    BOOST_CHECK_EQUAL( PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant( "+RAIL", "P" ), "PWR_BAR" );
    BOOST_CHECK_EQUAL( PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant( "-RAIL", "P" ), "GNDD" );

    // Arrow variants (filled triangle)
    BOOST_CHECK_EQUAL( PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant( "+ARROW", "P" ), "PWR_TRIANGLE" );
    BOOST_CHECK_EQUAL( PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant( "-ARROW", "P" ), "VEE" );

    // Bubble variants (map to VCC/VEE)
    BOOST_CHECK_EQUAL( PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant( "+BUBBLE", "P" ), "VCC" );
    BOOST_CHECK_EQUAL( PADS_SCH_SYMBOL_BUILDER::GetPowerStyleFromVariant( "-BUBBLE", "P" ), "VEE" );
}


BOOST_AUTO_TEST_CASE( SheetCount_SingleSheet )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    // Simple schematic with no parts defaults to 1 sheet
    BOOST_CHECK_EQUAL( parser.GetSheetCount(), 1 );
}


BOOST_AUTO_TEST_CASE( SheetNumbers_FromParts )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/parts_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    std::set<int> sheets = parser.GetSheetNumbers();
    BOOST_CHECK( sheets.count( 1 ) > 0 );  // At least sheet 1 should exist
}


BOOST_AUTO_TEST_CASE( GetPartsOnSheet )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/parts_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    std::vector<PADS_SCH::PART_PLACEMENT> partsOnSheet1 = parser.GetPartsOnSheet( 1 );

    // All parts in test file should be on sheet 1
    BOOST_CHECK_EQUAL( partsOnSheet1.size(), parser.GetPartPlacements().size() );
}


BOOST_AUTO_TEST_CASE( GetSignalsOnSheet )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/signals_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    std::vector<PADS_SCH::SCH_SIGNAL> signalsOnSheet1 = parser.GetSignalsOnSheet( 1 );

    // All signals in test file should have wires on sheet 1
    BOOST_CHECK( signalsOnSheet1.size() > 0 );
}


BOOST_AUTO_TEST_CASE( ParsePartTypes_V52_RegularPart_MultipleDecals )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/v52_parttypes.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );
    BOOST_CHECK( parser.IsValid() );

    const auto& partTypes = parser.GetPartTypes();

    // RES0805 should be parsed with G: gate format
    auto it = partTypes.find( "RES0805" );
    BOOST_REQUIRE( it != partTypes.end() );

    const PADS_SCH::PARTTYPE_DEF& res = it->second;
    BOOST_CHECK_EQUAL( res.category, "RES" );
    BOOST_REQUIRE_EQUAL( res.gates.size(), 1 );

    const PADS_SCH::GATE_DEF& resGate = res.gates[0];
    BOOST_CHECK_EQUAL( resGate.num_decal_variants, 4 );
    BOOST_CHECK_EQUAL( resGate.num_pins, 2 );
    BOOST_CHECK_EQUAL( resGate.swap_flag, 0 );

    // Decal names parsed from colon-separated G: field
    BOOST_REQUIRE_EQUAL( resGate.decal_names.size(), 4 );
    BOOST_CHECK_EQUAL( resGate.decal_names[0], "RESZ-H" );
    BOOST_CHECK_EQUAL( resGate.decal_names[1], "RESZ-V" );
    BOOST_CHECK_EQUAL( resGate.decal_names[2], "RESB-H" );
    BOOST_CHECK_EQUAL( resGate.decal_names[3], "RESB-V" );

    // Pin definitions from dot-separated tokens
    BOOST_REQUIRE_EQUAL( resGate.pins.size(), 2 );
    BOOST_CHECK_EQUAL( resGate.pins[0].pin_id, "1" );
    BOOST_CHECK_EQUAL( resGate.pins[0].swap_group, 1 );
    BOOST_CHECK_EQUAL( resGate.pins[0].pin_type, 'U' );
    BOOST_CHECK_EQUAL( resGate.pins[1].pin_id, "2" );
    BOOST_CHECK_EQUAL( resGate.pins[1].swap_group, 1 );
}


BOOST_AUTO_TEST_CASE( ParsePartTypes_V52_MultiGatePart )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/v52_parttypes.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& partTypes = parser.GetPartTypes();

    auto it = partTypes.find( "PS2802-4-A" );
    BOOST_REQUIRE( it != partTypes.end() );

    const PADS_SCH::PARTTYPE_DEF& ps = it->second;
    BOOST_CHECK_EQUAL( ps.category, "SOP" );
    BOOST_REQUIRE_EQUAL( ps.gates.size(), 4 );

    // Each gate should have 4 pins and reference the PS2802 decal
    for( int g = 0; g < 4; g++ )
    {
        BOOST_CHECK_EQUAL( ps.gates[g].num_pins, 4 );
        BOOST_REQUIRE_GE( ps.gates[g].decal_names.size(), 1 );
        BOOST_CHECK_EQUAL( ps.gates[g].decal_names[0], "PS2802" );
    }

    // Verify specific pin names from first gate (AN, CATH, EMIT, COL)
    BOOST_REQUIRE_EQUAL( ps.gates[0].pins.size(), 4 );
    BOOST_CHECK_EQUAL( ps.gates[0].pins[0].pin_id, "1" );
    BOOST_CHECK_EQUAL( ps.gates[0].pins[0].pin_name, "AN" );
    BOOST_CHECK_EQUAL( ps.gates[0].pins[1].pin_id, "2" );
    BOOST_CHECK_EQUAL( ps.gates[0].pins[1].pin_name, "CATH" );
    BOOST_CHECK_EQUAL( ps.gates[0].pins[2].pin_id, "15" );
    BOOST_CHECK_EQUAL( ps.gates[0].pins[2].pin_name, "EMIT" );
    BOOST_CHECK_EQUAL( ps.gates[0].pins[3].pin_id, "16" );
    BOOST_CHECK_EQUAL( ps.gates[0].pins[3].pin_name, "COL" );
}


BOOST_AUTO_TEST_CASE( ParsePartTypes_V52_Connector )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/v52_parttypes.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& partTypes = parser.GetPartTypes();

    auto it = partTypes.find( "43650-0400" );
    BOOST_REQUIRE( it != partTypes.end() );

    const PADS_SCH::PARTTYPE_DEF& conn = it->second;
    BOOST_CHECK_EQUAL( conn.category, "CON" );
    BOOST_CHECK( conn.is_connector );
    BOOST_REQUIRE_EQUAL( conn.gates.size(), 1 );
    BOOST_CHECK_EQUAL( conn.gates[0].num_pins, 4 );
    BOOST_REQUIRE_EQUAL( conn.gates[0].pins.size(), 4 );

    // Connector pins should have S type
    for( int p = 0; p < 4; p++ )
    {
        BOOST_CHECK_EQUAL( conn.gates[0].pins[p].pin_type, 'S' );
    }
}


BOOST_AUTO_TEST_CASE( ParsePartTypes_V52_SpecialSymbols )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/v52_parttypes.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& partTypes = parser.GetPartTypes();

    // $GND_SYMS
    auto gndIt = partTypes.find( "$GND_SYMS" );
    BOOST_REQUIRE( gndIt != partTypes.end() );

    const PADS_SCH::PARTTYPE_DEF& gnd = gndIt->second;
    BOOST_CHECK_EQUAL( gnd.special_keyword, "GND" );
    BOOST_CHECK_EQUAL( gnd.gates.size(), 2 );
    BOOST_REQUIRE_EQUAL( gnd.special_variants.size(), 2 );
    BOOST_CHECK_EQUAL( gnd.special_variants[0].decal_name, "DGND" );
    BOOST_CHECK_EQUAL( gnd.special_variants[0].pin_type, "G" );
    BOOST_CHECK_EQUAL( gnd.special_variants[1].decal_name, "PWRGND" );

    // V5.2 SIGPIN entries
    BOOST_REQUIRE_EQUAL( gnd.sigpins.size(), 2 );
    BOOST_CHECK_EQUAL( gnd.sigpins[0].pin_number, "1" );
    BOOST_CHECK_EQUAL( gnd.sigpins[0].net_name, "DGND" );
    BOOST_CHECK_EQUAL( gnd.sigpins[1].pin_number, "2" );
    BOOST_CHECK_EQUAL( gnd.sigpins[1].net_name, "PWRGND" );

    // $PWR_SYMS
    auto pwrIt = partTypes.find( "$PWR_SYMS" );
    BOOST_REQUIRE( pwrIt != partTypes.end() );

    const PADS_SCH::PARTTYPE_DEF& pwr = pwrIt->second;
    BOOST_CHECK_EQUAL( pwr.special_keyword, "PWR" );
    BOOST_CHECK_EQUAL( pwr.gates.size(), 2 );
    BOOST_REQUIRE_EQUAL( pwr.special_variants.size(), 2 );
    BOOST_CHECK_EQUAL( pwr.special_variants[0].decal_name, "+5V" );
    BOOST_CHECK_EQUAL( pwr.special_variants[0].pin_type, "P" );

    BOOST_REQUIRE_EQUAL( pwr.sigpins.size(), 2 );
    BOOST_CHECK_EQUAL( pwr.sigpins[0].net_name, "+5V" );
}


BOOST_AUTO_TEST_CASE( ParsePartTypes_V52_SimplePart )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/v52_parttypes.txt";

    PADS_SCH::PADS_SCH_PARSER parser;

    BOOST_REQUIRE( parser.Parse( testFile ) );

    const auto& partTypes = parser.GetPartTypes();

    // MMSZ5260BT1 should map to ZENER decal
    auto it = partTypes.find( "MMSZ5260BT1" );
    BOOST_REQUIRE( it != partTypes.end() );

    const PADS_SCH::PARTTYPE_DEF& diode = it->second;
    BOOST_CHECK_EQUAL( diode.category, "DIO" );
    BOOST_REQUIRE_EQUAL( diode.gates.size(), 1 );
    BOOST_REQUIRE_EQUAL( diode.gates[0].decal_names.size(), 1 );
    BOOST_CHECK_EQUAL( diode.gates[0].decal_names[0], "ZENER" );
    BOOST_CHECK_EQUAL( diode.gates[0].num_pins, 2 );
    BOOST_REQUIRE_EQUAL( diode.gates[0].pins.size(), 2 );
}


BOOST_AUTO_TEST_CASE( SymbolBuilder_ConnectorPinSymbol )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;
    PADS_SCH::PADS_SCH_SYMBOL_BUILDER builder( params );

    // Create a minimal connector PARTTYPE and symbol def
    PADS_SCH::PARTTYPE_DEF connPt;
    connPt.name = "TEST_CONN";
    connPt.is_connector = true;
    connPt.category = "CON";

    PADS_SCH::GATE_DEF gate;
    gate.num_pins = 1;
    gate.decal_names.push_back( "EXTIN" );

    PADS_SCH::PARTTYPE_PIN pin;
    pin.pin_id = "1";
    pin.pin_type = 'S';
    gate.pins.push_back( pin );
    connPt.gates.push_back( gate );

    PADS_SCH::SYMBOL_DEF symDef;
    symDef.name = "EXTIN";
    symDef.gate_count = 1;

    PADS_SCH::SYMBOL_PIN symPin;
    symPin.number = "1";
    symPin.position.x = 0;
    symPin.position.y = 0;
    symPin.length = 100;
    symDef.pins.push_back( symPin );

    // Pin 15 variant should have pin number "15"
    LIB_SYMBOL* sym15 = builder.GetOrCreateConnectorPinSymbol( connPt, symDef, "15" );
    BOOST_REQUIRE( sym15 != nullptr );

    auto pins15 = sym15->GetPins();
    BOOST_REQUIRE_EQUAL( pins15.size(), 1u );
    BOOST_CHECK_EQUAL( pins15[0]->GetNumber(), "15" );

    // Pin 1 variant should have pin number "1"
    LIB_SYMBOL* sym1 = builder.GetOrCreateConnectorPinSymbol( connPt, symDef, "1" );
    BOOST_REQUIRE( sym1 != nullptr );

    auto pins1 = sym1->GetPins();
    BOOST_REQUIRE_EQUAL( pins1.size(), 1u );
    BOOST_CHECK_EQUAL( pins1[0]->GetNumber(), "1" );

    // Different pin numbers should produce different cached symbols
    BOOST_CHECK_NE( sym15, sym1 );

    // Same pin number should return cached symbol
    LIB_SYMBOL* sym15again = builder.GetOrCreateConnectorPinSymbol( connPt, symDef, "15" );
    BOOST_CHECK_EQUAL( sym15, sym15again );
}


BOOST_AUTO_TEST_SUITE_END()


// Schematic Builder tests
#include <sch_io/pads/pads_sch_schematic_builder.h>
#include <io/pads/pads_attribute_mapper.h>
#include <sch_line.h>
#include <sch_label.h>
#include <sch_junction.h>
#include <sch_symbol.h>
#include <sch_screen.h>
#include <schematic.h>
#include <template_fieldnames.h>
#include <title_block.h>

BOOST_AUTO_TEST_SUITE( PadsSchSchematicBuilder )


BOOST_AUTO_TEST_CASE( CreateWire_SingleSegment )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), nullptr );

    PADS_SCH::WIRE_SEGMENT wire;
    wire.start.x = 1000.0;
    wire.start.y = 2000.0;
    wire.end.x = 3000.0;
    wire.end.y = 2000.0;

    SCH_LINE* line = builder.CreateWire( wire );
    BOOST_REQUIRE( line != nullptr );

    BOOST_CHECK( line->GetLayer() == LAYER_WIRE );
    BOOST_CHECK( line->GetStartPoint().x != 0 || line->GetStartPoint().y != 0 );
    BOOST_CHECK( line->GetEndPoint().x != 0 || line->GetEndPoint().y != 0 );

    delete line;
}


BOOST_AUTO_TEST_CASE( CreateWire_FromSignals )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/signals_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), nullptr );

    // Count total expected wires
    int expectedWires = 0;

    for( const auto& signal : parser.GetSignals() )
        expectedWires += static_cast<int>( signal.wires.size() );

    BOOST_CHECK( expectedWires > 0 );
}


BOOST_AUTO_TEST_CASE( CreateNetLabel_FromSignal )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), nullptr );

    PADS_SCH::SCH_SIGNAL signal;
    signal.name = "VCC";

    PADS_SCH::WIRE_SEGMENT wire;
    wire.start.x = 1000.0;
    wire.start.y = 2000.0;
    wire.end.x = 3000.0;
    wire.end.y = 2000.0;
    signal.wires.push_back( wire );

    VECTOR2I pos( 1000, 2000 );
    SCH_GLOBALLABEL* label = builder.CreateNetLabel( signal, pos );
    BOOST_REQUIRE( label != nullptr );

    BOOST_CHECK_EQUAL( label->GetText(), "VCC" );

    delete label;
}


BOOST_AUTO_TEST_CASE( CreateNetLabel_PreservesSpecialChars )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), nullptr );

    PADS_SCH::SCH_SIGNAL signal;
    signal.name = "NET 1";

    VECTOR2I pos( 1000, 2000 );
    SCH_GLOBALLABEL* label = builder.CreateNetLabel( signal, pos );
    BOOST_REQUIRE( label != nullptr );

    // Net names pass through unchanged; KiCad handles arbitrary UTF-8 net names
    BOOST_CHECK_EQUAL( label->GetText(), "NET 1" );

    delete label;
}


BOOST_AUTO_TEST_CASE( IsBusSignal_BracketNotation )
{
    // Bus notation with brackets: NAME[n:m] or NAME[n..m]
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "DATA[7:0]" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "ADDR[15:0]" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "BUS[0..7]" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "D[31..0]" ) );
}


BOOST_AUTO_TEST_CASE( IsBusSignal_AngleNotation )
{
    // Bus notation with angle brackets: NAME<n:m>
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "DATA<7:0>" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "ADDR<15:0>" ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "BUS<0..7>" ) );
}


BOOST_AUTO_TEST_CASE( IsBusSignal_NotABus )
{
    // Regular signal names that should not be detected as buses
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "VCC" ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "GND" ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "CLK" ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "NET1" ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "DATA0" ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsBusSignal( "" ) );
}


BOOST_AUTO_TEST_CASE( CreateBusWire_SingleSegment )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), nullptr );

    PADS_SCH::WIRE_SEGMENT wire;
    wire.start.x = 1000.0;
    wire.start.y = 2000.0;
    wire.end.x = 3000.0;
    wire.end.y = 2000.0;

    SCH_LINE* line = builder.CreateBusWire( wire );
    BOOST_REQUIRE( line != nullptr );

    BOOST_CHECK( line->GetLayer() == LAYER_BUS );

    delete line;
}


BOOST_AUTO_TEST_CASE( ApplyPartAttributes_Reference )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    // Create a test part placement
    PADS_SCH::PART_PLACEMENT placement;
    placement.reference = "R1";
    placement.part_type = "10K";

    // Create a mock schematic for testing
    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), &schematic );

    // Create a minimal symbol for testing
    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxS( "TEST" ) );
    LIB_ID libId( wxS( "test" ), wxS( "TEST" ) );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &schematic.CurrentSheet(), 0 );

    builder.ApplyPartAttributes( symbol, placement );

    // Verify reference was set
    wxString ref = symbol->GetRef( &schematic.CurrentSheet() );
    BOOST_CHECK_EQUAL( ref.ToStdString(), "R1" );

    // Verify value was set from part_type
    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::VALUE )->GetText().ToStdString(), "10K" );

    delete symbol;
    delete libSymbol;
}


BOOST_AUTO_TEST_CASE( ApplyPartAttributes_Footprint )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PART_PLACEMENT placement;
    placement.reference = "C1";
    placement.part_type = "100nF";

    // Add PCB DECAL attribute for footprint
    PADS_SCH::PART_ATTRIBUTE footprintAttr;
    footprintAttr.name = "PCB DECAL";
    footprintAttr.value = "CAP_0805";
    footprintAttr.visible = false;
    placement.attributes.push_back( footprintAttr );

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), &schematic );

    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxS( "CAP" ) );
    LIB_ID libId( wxS( "test" ), wxS( "CAP" ) );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &schematic.CurrentSheet(), 0 );

    builder.ApplyPartAttributes( symbol, placement );

    // Verify footprint was set
    BOOST_CHECK_EQUAL( symbol->GetField( FIELD_T::FOOTPRINT )->GetText().ToStdString(), "CAP_0805" );

    delete symbol;
    delete libSymbol;
}


BOOST_AUTO_TEST_CASE( ApplyFieldSettings_Visibility )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PART_PLACEMENT placement;
    placement.reference = "U1";
    placement.part_type = "74HC00";

    // Add VALUE attribute with visibility=false
    PADS_SCH::PART_ATTRIBUTE valueAttr;
    valueAttr.name = "VALUE";
    valueAttr.value = "74HC00";
    valueAttr.visible = false;
    placement.attributes.push_back( valueAttr );

    // Add REFDES attribute with visibility=true
    PADS_SCH::PART_ATTRIBUTE refAttr;
    refAttr.name = "REFDES";
    refAttr.value = "U1";
    refAttr.visible = true;
    placement.attributes.push_back( refAttr );

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), &schematic );

    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxS( "74HC00" ) );
    LIB_ID libId( wxS( "test" ), wxS( "74HC00" ) );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &schematic.CurrentSheet(), 0 );

    builder.ApplyPartAttributes( symbol, placement );

    // Verify visibility settings
    BOOST_CHECK( !symbol->GetField( FIELD_T::VALUE )->IsVisible() );
    BOOST_CHECK( symbol->GetField( FIELD_T::REFERENCE )->IsVisible() );

    delete symbol;
    delete libSymbol;
}


BOOST_AUTO_TEST_CASE( ApplyPartAttributes_NullSymbol )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PART_PLACEMENT placement;
    placement.reference = "R1";

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), nullptr );

    // Should not crash when called with nullptr
    builder.ApplyPartAttributes( nullptr, placement );

    BOOST_CHECK( true );  // If we get here, we passed
}


BOOST_AUTO_TEST_CASE( CreateCustomFields_ManufacturerAndMPN )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PART_PLACEMENT placement;
    placement.reference = "U1";
    placement.part_type = "74HC00";

    // Add manufacturer attribute (not a standard field)
    PADS_SCH::PART_ATTRIBUTE mfrAttr;
    mfrAttr.name = "Manufacturer";
    mfrAttr.value = "Texas Instruments";
    mfrAttr.visible = false;
    placement.attributes.push_back( mfrAttr );

    // Add MPN attribute (not a standard field)
    PADS_SCH::PART_ATTRIBUTE mpnAttr;
    mpnAttr.name = "MPN";
    mpnAttr.value = "SN74HC00N";
    mpnAttr.visible = false;
    placement.attributes.push_back( mpnAttr );

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), &schematic );

    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxS( "74HC00" ) );
    LIB_ID libId( wxS( "test" ), wxS( "74HC00" ) );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &schematic.CurrentSheet(), 0 );

    int fieldsCreated = builder.CreateCustomFields( symbol, placement );

    // Should have created 2 custom fields
    BOOST_CHECK_EQUAL( fieldsCreated, 2 );

    // Find and verify the manufacturer field
    SCH_FIELD* mfrField = symbol->GetField( wxS( "Manufacturer" ) );
    BOOST_REQUIRE( mfrField != nullptr );
    BOOST_CHECK_EQUAL( mfrField->GetText().ToStdString(), "Texas Instruments" );
    BOOST_CHECK( !mfrField->IsVisible() );

    // Find and verify the MPN field
    SCH_FIELD* mpnField = symbol->GetField( wxS( "MPN" ) );
    BOOST_REQUIRE( mpnField != nullptr );
    BOOST_CHECK_EQUAL( mpnField->GetText().ToStdString(), "SN74HC00N" );

    delete symbol;
    delete libSymbol;
}


BOOST_AUTO_TEST_CASE( CreateCustomFields_SkipsStandardFields )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PART_PLACEMENT placement;
    placement.reference = "R1";

    // Add standard field attributes (should be skipped by CreateCustomFields)
    PADS_SCH::PART_ATTRIBUTE refAttr;
    refAttr.name = "Ref.Des.";  // Standard field
    refAttr.value = "R1";
    placement.attributes.push_back( refAttr );

    PADS_SCH::PART_ATTRIBUTE valAttr;
    valAttr.name = "Part Type";  // Standard field
    valAttr.value = "10K";
    placement.attributes.push_back( valAttr );

    // Add one non-standard field
    PADS_SCH::PART_ATTRIBUTE customAttr;
    customAttr.name = "Tolerance";
    customAttr.value = "5%";
    customAttr.visible = true;
    placement.attributes.push_back( customAttr );

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), &schematic );

    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxS( "RES" ) );
    LIB_ID libId( wxS( "test" ), wxS( "RES" ) );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &schematic.CurrentSheet(), 0 );

    int fieldsCreated = builder.CreateCustomFields( symbol, placement );

    // Should have created only 1 custom field (Tolerance)
    BOOST_CHECK_EQUAL( fieldsCreated, 1 );

    SCH_FIELD* tolField = symbol->GetField( wxS( "Tolerance" ) );
    BOOST_REQUIRE( tolField != nullptr );
    BOOST_CHECK_EQUAL( tolField->GetText().ToStdString(), "5%" );
    BOOST_CHECK( !tolField->IsVisible() );

    delete symbol;
    delete libSymbol;
}


BOOST_AUTO_TEST_CASE( CreateCustomFields_SkipsEmptyValues )
{
    std::string testFile = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/simple_schematic.txt";

    PADS_SCH::PADS_SCH_PARSER parser;
    BOOST_REQUIRE( parser.Parse( testFile ) );

    PADS_SCH::PART_PLACEMENT placement;
    placement.reference = "U1";

    // Add attribute with empty value
    PADS_SCH::PART_ATTRIBUTE emptyAttr;
    emptyAttr.name = "SerialNumber";
    emptyAttr.value = "";
    placement.attributes.push_back( emptyAttr );

    // Add attribute with actual value
    PADS_SCH::PART_ATTRIBUTE validAttr;
    validAttr.name = "Revision";
    validAttr.value = "A";
    validAttr.visible = true;
    placement.attributes.push_back( validAttr );

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( parser.GetParameters(), &schematic );

    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( wxS( "IC" ) );
    LIB_ID libId( wxS( "test" ), wxS( "IC" ) );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &schematic.CurrentSheet(), 0 );

    int fieldsCreated = builder.CreateCustomFields( symbol, placement );

    // Should have created only 1 field (Revision, not empty SerialNumber)
    BOOST_CHECK_EQUAL( fieldsCreated, 1 );

    // Verify Revision field exists
    SCH_FIELD* revField = symbol->GetField( wxS( "Revision" ) );
    BOOST_REQUIRE( revField != nullptr );
    BOOST_CHECK_EQUAL( revField->GetText().ToStdString(), "A" );

    // Verify SerialNumber field was NOT created
    SCH_FIELD* snField = symbol->GetField( wxS( "SerialNumber" ) );
    BOOST_CHECK( snField == nullptr );

    delete symbol;
    delete libSymbol;
}


BOOST_AUTO_TEST_CASE( CreateTitleBlock_AllFields )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;
    params.fields["Title"] = "Test Design";
    params.fields["DATE"] = "2025-01-12";
    params.fields["Revision"] = "A";
    params.fields["Company Name"] = "Test Company";

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, nullptr );

    SCH_SCREEN screen;

    builder.CreateTitleBlock( &screen );

    const TITLE_BLOCK& tb = screen.GetTitleBlock();

    BOOST_CHECK_EQUAL( tb.GetTitle().ToStdString(), "Test Design" );
    BOOST_CHECK_EQUAL( tb.GetDate().ToStdString(), "2025-01-12" );
    BOOST_CHECK_EQUAL( tb.GetRevision().ToStdString(), "A" );
    BOOST_CHECK_EQUAL( tb.GetCompany().ToStdString(), "Test Company" );
}


BOOST_AUTO_TEST_CASE( CreateTitleBlock_JobNameFallback )
{
    // Create parameters with job_name but no title
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;
    params.job_name = "My Project";

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, nullptr );

    SCH_SCREEN screen;

    builder.CreateTitleBlock( &screen );

    const TITLE_BLOCK& tb = screen.GetTitleBlock();

    // Title should fall back to job_name
    BOOST_CHECK_EQUAL( tb.GetTitle().ToStdString(), "My Project" );
}


BOOST_AUTO_TEST_CASE( CreateTitleBlock_EmptyFields )
{
    // Create empty parameters
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, nullptr );

    SCH_SCREEN screen;

    builder.CreateTitleBlock( &screen );

    const TITLE_BLOCK& tb = screen.GetTitleBlock();

    // All fields should be empty
    BOOST_CHECK( tb.GetTitle().IsEmpty() );
    BOOST_CHECK( tb.GetDate().IsEmpty() );
    BOOST_CHECK( tb.GetRevision().IsEmpty() );
    BOOST_CHECK( tb.GetCompany().IsEmpty() );
}


BOOST_AUTO_TEST_CASE( CreateTitleBlock_NullScreen )
{
    PADS_SCH::PARAMETERS params;
    params.fields["Title"] = "Test";

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, nullptr );

    // Should not crash with nullptr
    builder.CreateTitleBlock( nullptr );

    BOOST_CHECK( true );  // If we get here, we passed
}


BOOST_AUTO_TEST_SUITE_END()


// Hierarchical sheet tests
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>

BOOST_AUTO_TEST_SUITE( PadsSchHierarchicalSheets )


BOOST_AUTO_TEST_CASE( GetDefaultSheetSize_ReturnsValidSize )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, &schematic );

    VECTOR2I size = builder.GetDefaultSheetSize();

    BOOST_CHECK( size.x > 0 );
    BOOST_CHECK( size.y > 0 );
}


BOOST_AUTO_TEST_CASE( CalculateSheetPosition_FirstSheet )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, &schematic );

    VECTOR2I pos = builder.CalculateSheetPosition( 0, 4 );

    BOOST_CHECK( pos.x > 0 );
    BOOST_CHECK( pos.y > 0 );
}


BOOST_AUTO_TEST_CASE( CalculateSheetPosition_GridLayout )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, &schematic );

    VECTOR2I pos0 = builder.CalculateSheetPosition( 0, 4 );
    VECTOR2I pos1 = builder.CalculateSheetPosition( 1, 4 );
    VECTOR2I pos2 = builder.CalculateSheetPosition( 2, 4 );
    VECTOR2I pos3 = builder.CalculateSheetPosition( 3, 4 );

    // For 4 sheets, should be 2x2 grid
    // pos0 and pos1 should be on same row (same Y)
    // pos0 and pos2 should be in same column (same X)
    BOOST_CHECK_EQUAL( pos0.y, pos1.y );
    BOOST_CHECK_EQUAL( pos2.y, pos3.y );
    BOOST_CHECK_EQUAL( pos0.x, pos2.x );
    BOOST_CHECK_EQUAL( pos1.x, pos3.x );

    // Positions should increase going right and down
    BOOST_CHECK( pos1.x > pos0.x );
    BOOST_CHECK( pos2.y > pos0.y );
}


BOOST_AUTO_TEST_CASE( CreateHierarchicalSheet_ReturnsValidSheet )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    // Create root sheet
    SCH_SHEET* rootSheet = new SCH_SHEET( &schematic );
    SCH_SCREEN* rootScreen = new SCH_SCREEN( &schematic );
    rootSheet->SetScreen( rootScreen );

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, &schematic );

    SCH_SHEET* sheet = builder.CreateHierarchicalSheet( 1, 3, rootSheet, wxT( "test_design.txt" ) );
    BOOST_REQUIRE( sheet != nullptr );

    BOOST_CHECK( sheet->GetScreen() != nullptr );

    // Verify sheet has valid position
    VECTOR2I pos = sheet->GetPosition();
    BOOST_CHECK( pos.x >= 0 );
    BOOST_CHECK( pos.y >= 0 );

    // Verify sheet has valid size
    VECTOR2I size = sheet->GetSize();
    BOOST_CHECK( size.x > 0 );
    BOOST_CHECK( size.y > 0 );

    delete rootSheet;
}


BOOST_AUTO_TEST_CASE( CreateHierarchicalSheet_SetsFilename )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    SCH_SHEET* rootSheet = new SCH_SHEET( &schematic );
    SCH_SCREEN* rootScreen = new SCH_SCREEN( &schematic );
    rootSheet->SetScreen( rootScreen );

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, &schematic );

    SCH_SHEET* sheet = builder.CreateHierarchicalSheet( 2, 3, rootSheet, wxT( "my_design.asc" ) );
    BOOST_REQUIRE( sheet != nullptr );

    // Verify sheet filename was set
    wxString filename = sheet->GetField( FIELD_T::SHEET_FILENAME )->GetText();
    BOOST_CHECK( filename.Contains( wxT( "my_design" ) ) );
    BOOST_CHECK( filename.Contains( wxT( "sheet2" ) ) );
    BOOST_CHECK( filename.EndsWith( wxT( ".kicad_sch" ) ) );

    delete rootSheet;
}


BOOST_AUTO_TEST_CASE( CreateHierarchicalSheet_SetsSheetName )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    SCH_SHEET* rootSheet = new SCH_SHEET( &schematic );
    SCH_SCREEN* rootScreen = new SCH_SCREEN( &schematic );
    rootSheet->SetScreen( rootScreen );

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, &schematic );

    SCH_SHEET* sheet = builder.CreateHierarchicalSheet( 3, 5, rootSheet, wxT( "design.txt" ) );
    BOOST_REQUIRE( sheet != nullptr );

    // Verify sheet name was set
    wxString name = sheet->GetField( FIELD_T::SHEET_NAME )->GetText();
    BOOST_CHECK( name.Contains( wxT( "3" ) ) );

    delete rootSheet;
}


BOOST_AUTO_TEST_CASE( CreateHierarchicalSheet_NullParent )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, &schematic );

    // Should return nullptr when parent is null
    SCH_SHEET* sheet = builder.CreateHierarchicalSheet( 1, 3, nullptr, wxT( "test.txt" ) );
    BOOST_CHECK( sheet == nullptr );
}


BOOST_AUTO_TEST_CASE( CreateSheetPin_ValidPin )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    SCH_SHEET* sheet = new SCH_SHEET( &schematic );
    sheet->SetPosition( VECTOR2I( 1000, 2000 ) );
    sheet->SetSize( VECTOR2I( 3000, 2000 ) );

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, &schematic );

    SCH_SHEET_PIN* pin = builder.CreateSheetPin( sheet, "NET1", 0 );
    BOOST_REQUIRE( pin != nullptr );

    BOOST_CHECK_EQUAL( pin->GetText().ToStdString(), "NET1" );
    BOOST_CHECK( pin->GetSide() == SHEET_SIDE::LEFT );

    delete sheet;
}


BOOST_AUTO_TEST_CASE( CreateSheetPin_PreservesName )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    SCH_SHEET* sheet = new SCH_SHEET( &schematic );

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, &schematic );

    SCH_SHEET_PIN* pin = builder.CreateSheetPin( sheet, "NET 1", 0 );
    BOOST_REQUIRE( pin != nullptr );

    // Net names pass through unchanged; KiCad handles arbitrary UTF-8 net names
    BOOST_CHECK_EQUAL( pin->GetText().ToStdString(), "NET 1" );

    delete sheet;
}


BOOST_AUTO_TEST_CASE( CreateSheetPin_NullSheet )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, &schematic );

    SCH_SHEET_PIN* pin = builder.CreateSheetPin( nullptr, "NET1", 0 );
    BOOST_CHECK( pin == nullptr );
}


BOOST_AUTO_TEST_CASE( CreateHierLabel_ValidLabel )
{
    PADS_SCH::PARAMETERS params;
    params.units = PADS_SCH::UNIT_TYPE::MILS;

    SCHEMATIC schematic( nullptr );
    schematic.Reset();

    SCH_SCREEN screen;

    PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER builder( params, &schematic );

    VECTOR2I pos( 1000, 2000 );
    SCH_HIERLABEL* label = builder.CreateHierLabel( "DATA_OUT", pos, &screen );
    BOOST_REQUIRE( label != nullptr );

    BOOST_CHECK_EQUAL( label->GetText().ToStdString(), "DATA_OUT" );

    // Label should be added to screen
    BOOST_CHECK( screen.Items().size() > 0 );
}



BOOST_AUTO_TEST_CASE( IsGlobalSignal_PowerNets )
{
    std::set<int> singleSheet = { 1 };

    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "VCC", singleSheet ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "GND", singleSheet ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "vcc", singleSheet ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "AGND", singleSheet ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "+5V", singleSheet ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "+3V3", singleSheet ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "-12V", singleSheet ) );
}


BOOST_AUTO_TEST_CASE( IsGlobalSignal_MultiSheet )
{
    std::set<int> multiSheet = { 1, 2, 3 };

    // Any signal on multiple sheets should be global
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "DATA_BUS", multiSheet ) );
    BOOST_CHECK( PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "RANDOM_NET", multiSheet ) );
}


BOOST_AUTO_TEST_CASE( IsGlobalSignal_NotGlobal )
{
    std::set<int> singleSheet = { 1 };

    BOOST_CHECK( !PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "NET1", singleSheet ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "DATA", singleSheet ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "CLK", singleSheet ) );
    BOOST_CHECK( !PADS_SCH::PADS_SCH_SCHEMATIC_BUILDER::IsGlobalSignal( "", singleSheet ) );
}


BOOST_AUTO_TEST_SUITE_END()
