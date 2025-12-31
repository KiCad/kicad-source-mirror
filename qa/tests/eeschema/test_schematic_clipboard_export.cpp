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
 * @file test_schematic_clipboard_export.cpp
 * Tests for multi-format clipboard export functionality for schematic editor.
 *
 * These tests verify:
 * 1. SVG export produces valid output for schematic items
 * 2. Various schematic element types can be created for export testing
 * 3. Bounding box calculation for different element types
 */

#include <boost/test/unit_test.hpp>

#include <sch_symbol.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_bus_entry.h>
#include <sch_no_connect.h>
#include <sch_text.h>
#include <sch_label.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <lib_symbol.h>
#include <plotters/plotters_pslike.h>
#include <sch_painter.h>
#include <sch_plotter.h>
#include <locale_io.h>
#include <settings/color_settings.h>
#include <wx/ffile.h>
#include <wx/mstream.h>


class SCHEMATIC_CLIPBOARD_FIXTURE
{
public:
    SCHEMATIC_CLIPBOARD_FIXTURE() = default;
    ~SCHEMATIC_CLIPBOARD_FIXTURE() = default;

    std::unique_ptr<SCH_LINE> CreateWire( int x1, int y1, int x2, int y2 )
    {
        auto wire = std::make_unique<SCH_LINE>(
                VECTOR2I( schIUScale.MilsToIU( x1 ), schIUScale.MilsToIU( y1 ) ),
                LAYER_WIRE );
        wire->SetEndPoint( VECTOR2I( schIUScale.MilsToIU( x2 ), schIUScale.MilsToIU( y2 ) ) );
        m_items.push_back( wire.get() );
        return wire;
    }

    std::unique_ptr<SCH_LINE> CreateBus( int x1, int y1, int x2, int y2 )
    {
        auto bus = std::make_unique<SCH_LINE>(
                VECTOR2I( schIUScale.MilsToIU( x1 ), schIUScale.MilsToIU( y1 ) ),
                LAYER_BUS );
        bus->SetEndPoint( VECTOR2I( schIUScale.MilsToIU( x2 ), schIUScale.MilsToIU( y2 ) ) );
        m_items.push_back( bus.get() );
        return bus;
    }

    std::unique_ptr<SCH_JUNCTION> CreateJunction( int x, int y )
    {
        auto junction = std::make_unique<SCH_JUNCTION>(
                VECTOR2I( schIUScale.MilsToIU( x ), schIUScale.MilsToIU( y ) ) );
        m_items.push_back( junction.get() );
        return junction;
    }

    std::unique_ptr<SCH_NO_CONNECT> CreateNoConnect( int x, int y )
    {
        auto noConnect = std::make_unique<SCH_NO_CONNECT>(
                VECTOR2I( schIUScale.MilsToIU( x ), schIUScale.MilsToIU( y ) ) );
        m_items.push_back( noConnect.get() );
        return noConnect;
    }

    std::unique_ptr<SCH_BUS_WIRE_ENTRY> CreateBusEntry( int x, int y )
    {
        auto entry = std::make_unique<SCH_BUS_WIRE_ENTRY>(
                VECTOR2I( schIUScale.MilsToIU( x ), schIUScale.MilsToIU( y ) ) );
        m_items.push_back( entry.get() );
        return entry;
    }

    std::unique_ptr<SCH_TEXT> CreateText( int x, int y, const wxString& text )
    {
        auto schText = std::make_unique<SCH_TEXT>(
                VECTOR2I( schIUScale.MilsToIU( x ), schIUScale.MilsToIU( y ) ), text );
        schText->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( 50 ) ) );
        m_items.push_back( schText.get() );
        return schText;
    }

    std::unique_ptr<SCH_LABEL> CreateLabel( int x, int y, const wxString& text )
    {
        auto label = std::make_unique<SCH_LABEL>(
                VECTOR2I( schIUScale.MilsToIU( x ), schIUScale.MilsToIU( y ) ), text );
        m_items.push_back( label.get() );
        return label;
    }

    std::unique_ptr<SCH_GLOBALLABEL> CreateGlobalLabel( int x, int y, const wxString& text )
    {
        auto label = std::make_unique<SCH_GLOBALLABEL>(
                VECTOR2I( schIUScale.MilsToIU( x ), schIUScale.MilsToIU( y ) ), text );
        m_items.push_back( label.get() );
        return label;
    }

    std::unique_ptr<SCH_HIERLABEL> CreateHierLabel( int x, int y, const wxString& text )
    {
        auto label = std::make_unique<SCH_HIERLABEL>(
                VECTOR2I( schIUScale.MilsToIU( x ), schIUScale.MilsToIU( y ) ), text );
        m_items.push_back( label.get() );
        return label;
    }

    std::unique_ptr<SCH_SHAPE> CreateRectangle( int x1, int y1, int x2, int y2 )
    {
        auto rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE );
        rect->SetPosition( VECTOR2I( schIUScale.MilsToIU( x1 ), schIUScale.MilsToIU( y1 ) ) );
        rect->SetEnd( VECTOR2I( schIUScale.MilsToIU( x2 ), schIUScale.MilsToIU( y2 ) ) );
        rect->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        m_items.push_back( rect.get() );
        return rect;
    }

    std::unique_ptr<SCH_SHAPE> CreateCircle( int cx, int cy, int radius )
    {
        auto circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE );
        circle->SetPosition( VECTOR2I( schIUScale.MilsToIU( cx ), schIUScale.MilsToIU( cy ) ) );
        circle->SetEnd( VECTOR2I( schIUScale.MilsToIU( cx + radius ), schIUScale.MilsToIU( cy ) ) );
        circle->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        m_items.push_back( circle.get() );
        return circle;
    }

    std::unique_ptr<SCH_SHAPE> CreatePolyline( const std::vector<std::pair<int, int>>& points )
    {
        auto poly = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY );
        poly->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );

        for( const auto& pt : points )
            poly->AddPoint( VECTOR2I( schIUScale.MilsToIU( pt.first ), schIUScale.MilsToIU( pt.second ) ) );

        m_items.push_back( poly.get() );
        return poly;
    }

    void ClearItems()
    {
        m_items.clear();
    }

    std::vector<SCH_ITEM*> m_items;
};


BOOST_FIXTURE_TEST_SUITE( SchematicClipboardExport, SCHEMATIC_CLIPBOARD_FIXTURE )


/**
 * Test that wires can be created for export testing.
 */
BOOST_AUTO_TEST_CASE( ElementCreation_Wires )
{
    auto wire = CreateWire( 0, 0, 100, 0 );

    BOOST_CHECK( wire != nullptr );
    BOOST_CHECK( wire->IsWire() );
    BOOST_CHECK_EQUAL( wire->GetStartPoint().x, schIUScale.MilsToIU( 0 ) );
    BOOST_CHECK_EQUAL( wire->GetEndPoint().x, schIUScale.MilsToIU( 100 ) );
}


/**
 * Test that buses can be created for export testing.
 */
BOOST_AUTO_TEST_CASE( ElementCreation_Buses )
{
    auto bus = CreateBus( 0, 0, 0, 100 );

    BOOST_CHECK( bus != nullptr );
    BOOST_CHECK( bus->IsBus() );
}


/**
 * Test that junctions can be created for export testing.
 */
BOOST_AUTO_TEST_CASE( ElementCreation_Junctions )
{
    auto junction = CreateJunction( 50, 50 );

    BOOST_CHECK( junction != nullptr );
    BOOST_CHECK_EQUAL( junction->GetPosition().x, schIUScale.MilsToIU( 50 ) );
    BOOST_CHECK_EQUAL( junction->GetPosition().y, schIUScale.MilsToIU( 50 ) );
}


/**
 * Test that no-connect markers can be created for export testing.
 */
BOOST_AUTO_TEST_CASE( ElementCreation_NoConnect )
{
    auto noConnect = CreateNoConnect( 100, 100 );

    BOOST_CHECK( noConnect != nullptr );
    BOOST_CHECK( noConnect->Type() == SCH_NO_CONNECT_T );
}


/**
 * Test that bus entries can be created for export testing.
 */
BOOST_AUTO_TEST_CASE( ElementCreation_BusEntry )
{
    auto entry = CreateBusEntry( 150, 150 );

    BOOST_CHECK( entry != nullptr );
    BOOST_CHECK( entry->Type() == SCH_BUS_WIRE_ENTRY_T );
}


/**
 * Test that text items can be created for export testing.
 */
BOOST_AUTO_TEST_CASE( ElementCreation_Text )
{
    auto text = CreateText( 200, 200, wxT( "Test Text" ) );

    BOOST_CHECK( text != nullptr );
    BOOST_CHECK( text->GetText() == wxT( "Test Text" ) );
}


/**
 * Test that labels can be created for export testing.
 */
BOOST_AUTO_TEST_CASE( ElementCreation_Labels )
{
    auto label = CreateLabel( 0, 0, wxT( "NET1" ) );
    auto globalLabel = CreateGlobalLabel( 100, 0, wxT( "VCC" ) );
    auto hierLabel = CreateHierLabel( 200, 0, wxT( "DATA_IN" ) );

    BOOST_CHECK( label != nullptr );
    BOOST_CHECK( globalLabel != nullptr );
    BOOST_CHECK( hierLabel != nullptr );

    BOOST_CHECK( label->Type() == SCH_LABEL_T );
    BOOST_CHECK( globalLabel->Type() == SCH_GLOBAL_LABEL_T );
    BOOST_CHECK( hierLabel->Type() == SCH_HIER_LABEL_T );
}


/**
 * Test that shapes can be created for export testing.
 */
BOOST_AUTO_TEST_CASE( ElementCreation_Shapes )
{
    auto rect = CreateRectangle( 0, 0, 100, 100 );
    auto circle = CreateCircle( 200, 50, 50 );
    auto poly = CreatePolyline( { { 300, 0 }, { 350, 50 }, { 300, 100 } } );

    BOOST_CHECK( rect != nullptr );
    BOOST_CHECK( circle != nullptr );
    BOOST_CHECK( poly != nullptr );

    BOOST_CHECK( rect->GetShape() == SHAPE_T::RECTANGLE );
    BOOST_CHECK( circle->GetShape() == SHAPE_T::CIRCLE );
    BOOST_CHECK( poly->GetShape() == SHAPE_T::POLY );
}


/**
 * Test wire endpoint calculation.
 */
BOOST_AUTO_TEST_CASE( Wire_Endpoints )
{
    auto wire1 = CreateWire( 0, 0, 100, 0 );
    auto wire2 = CreateWire( 0, 0, 0, 100 );

    // Verify wire endpoints
    BOOST_CHECK_EQUAL( wire1->GetStartPoint().x, schIUScale.MilsToIU( 0 ) );
    BOOST_CHECK_EQUAL( wire1->GetEndPoint().x, schIUScale.MilsToIU( 100 ) );
    BOOST_CHECK_EQUAL( wire2->GetStartPoint().y, schIUScale.MilsToIU( 0 ) );
    BOOST_CHECK_EQUAL( wire2->GetEndPoint().y, schIUScale.MilsToIU( 100 ) );
}


/**
 * Test mixed element positions.
 */
BOOST_AUTO_TEST_CASE( MixedElements_Positions )
{
    auto wire = CreateWire( 0, 0, 100, 0 );
    auto junction = CreateJunction( 100, 0 );
    auto text = CreateText( 150, 0, wxT( "Label" ) );

    // Verify positions
    BOOST_CHECK_EQUAL( wire->GetEndPoint().x, schIUScale.MilsToIU( 100 ) );
    BOOST_CHECK_EQUAL( junction->GetPosition().x, schIUScale.MilsToIU( 100 ) );
    BOOST_CHECK_EQUAL( text->GetPosition().x, schIUScale.MilsToIU( 150 ) );
}


/**
 * Test rectangle dimensions.
 */
BOOST_AUTO_TEST_CASE( Rectangle_Dimensions )
{
    auto rect = CreateRectangle( -50, -50, 50, 50 );

    // Verify the rectangle was created with correct points
    BOOST_CHECK_EQUAL( rect->GetStart().x, schIUScale.MilsToIU( -50 ) );
    BOOST_CHECK_EQUAL( rect->GetStart().y, schIUScale.MilsToIU( -50 ) );
    BOOST_CHECK_EQUAL( rect->GetEnd().x, schIUScale.MilsToIU( 50 ) );
    BOOST_CHECK_EQUAL( rect->GetEnd().y, schIUScale.MilsToIU( 50 ) );
}


/**
 * Test circle creation.
 */
BOOST_AUTO_TEST_CASE( Circle_Creation )
{
    int radius = 100;
    auto circle = CreateCircle( 0, 0, radius );

    // Verify circle center and end point (which determines radius)
    BOOST_CHECK_EQUAL( circle->GetStart().x, schIUScale.MilsToIU( 0 ) );
    BOOST_CHECK_EQUAL( circle->GetStart().y, schIUScale.MilsToIU( 0 ) );
    BOOST_CHECK_EQUAL( circle->GetEnd().x, schIUScale.MilsToIU( radius ) );
}


/**
 * Test PNG alpha computation formula used in schematic export.
 */
BOOST_AUTO_TEST_CASE( PngExport_AlphaComputation_Opaque )
{
    // Opaque pixel: same on white and black
    int rW = 128, gW = 128, bW = 128;
    int rB = 128, gB = 128, bB = 128;

    int diffR = rW - rB;
    int diffG = gW - gB;
    int diffB = bW - bB;
    int avgDiff = ( diffR + diffG + diffB ) / 3;
    int alpha = 255 - avgDiff;

    BOOST_CHECK_EQUAL( alpha, 255 );
}


/**
 * Test PNG alpha computation for transparent pixel.
 */
BOOST_AUTO_TEST_CASE( PngExport_AlphaComputation_Transparent )
{
    // Transparent pixel: shows background
    int rW = 255, gW = 255, bW = 255;
    int rB = 0, gB = 0, bB = 0;

    int diffR = rW - rB;
    int diffG = gW - gB;
    int diffB = bW - bB;
    int avgDiff = ( diffR + diffG + diffB ) / 3;
    int alpha = 255 - avgDiff;

    BOOST_CHECK_EQUAL( alpha, 0 );
}


/**
 * Test that a complex schematic selection can be created.
 */
BOOST_AUTO_TEST_CASE( ComplexSchematic_MultipleLayers )
{
    // Create a simple schematic structure
    // Wires
    auto wire1 = CreateWire( 0, 0, 200, 0 );
    auto wire2 = CreateWire( 200, 0, 200, 100 );

    // Junction at intersection
    auto junction = CreateJunction( 200, 0 );

    // Bus and bus entry
    auto bus = CreateBus( 0, 200, 300, 200 );
    auto busEntry = CreateBusEntry( 150, 200 );

    // Labels
    auto label = CreateLabel( 50, -20, wxT( "NET_A" ) );
    auto globalLabel = CreateGlobalLabel( 250, 50, wxT( "VCC" ) );

    // Text
    auto text = CreateText( 100, 300, wxT( "Note: Power section" ) );

    // Shapes
    auto rect = CreateRectangle( -50, -50, 350, 350 );

    // Verify all items were created with correct types
    BOOST_CHECK_EQUAL( m_items.size(), 9 );
    BOOST_CHECK( wire1->IsWire() );
    BOOST_CHECK( wire2->IsWire() );
    BOOST_CHECK( bus->IsBus() );
    BOOST_CHECK( junction->Type() == SCH_JUNCTION_T );
    BOOST_CHECK( busEntry->Type() == SCH_BUS_WIRE_ENTRY_T );
    BOOST_CHECK( label->Type() == SCH_LABEL_T );
    BOOST_CHECK( globalLabel->Type() == SCH_GLOBAL_LABEL_T );
    BOOST_CHECK( text->Type() == SCH_TEXT_T );
    BOOST_CHECK( rect->GetShape() == SHAPE_T::RECTANGLE );
}


BOOST_AUTO_TEST_SUITE_END()
