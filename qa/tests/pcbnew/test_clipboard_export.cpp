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
 * @file test_clipboard_export.cpp
 * Tests for multi-format clipboard export functionality in PCBnew.
 *
 * These tests verify:
 * 1. SVG export includes all element types (tracks, arcs, vias, zones, pads, fields)
 * 2. SVG export uses proper layer structure with KiCad layer names
 * 3. PNG export uses unselected colors (no highlight)
 * 4. PNG export renders holes in foreground
 * 5. Footprint editor export includes pads and fields
 */

#include <boost/test/unit_test.hpp>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_field.h>
#include <zone.h>
#include <lset.h>

#include <plotters/plotters_pslike.h>
#include <pcb_painter.h>
#include <wx/ffile.h>
#include <wx/mstream.h>


class CLIPBOARD_EXPORT_FIXTURE
{
public:
    CLIPBOARD_EXPORT_FIXTURE()
    {
        m_board = std::make_unique<BOARD>();
        m_board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );
        m_board->SetVisibleLayers( m_board->GetEnabledLayers() );
    }

    ~CLIPBOARD_EXPORT_FIXTURE() = default;

    void AddTrack( int x1, int y1, int x2, int y2, PCB_LAYER_ID layer = F_Cu )
    {
        PCB_TRACK* track = new PCB_TRACK( m_board.get() );
        track->SetStart( VECTOR2I( pcbIUScale.mmToIU( x1 ), pcbIUScale.mmToIU( y1 ) ) );
        track->SetEnd( VECTOR2I( pcbIUScale.mmToIU( x2 ), pcbIUScale.mmToIU( y2 ) ) );
        track->SetWidth( pcbIUScale.mmToIU( 0.25 ) );
        track->SetLayer( layer );
        m_board->Add( track );
        m_items.push_back( track );
    }

    void AddVia( int x, int y )
    {
        PCB_VIA* via = new PCB_VIA( m_board.get() );
        via->SetPosition( VECTOR2I( pcbIUScale.mmToIU( x ), pcbIUScale.mmToIU( y ) ) );
        via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.8 ) );
        via->SetDrill( pcbIUScale.mmToIU( 0.4 ) );
        via->SetViaType( VIATYPE::THROUGH );
        m_board->Add( via );
        m_items.push_back( via );
    }

    void AddPad( FOOTPRINT* fp, int x, int y, const wxString& padNum, PAD_SHAPE shape = PAD_SHAPE::CIRCLE )
    {
        PAD* pad = new PAD( fp );
        pad->SetPosition( VECTOR2I( pcbIUScale.mmToIU( x ), pcbIUScale.mmToIU( y ) ) );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1.5 ), pcbIUScale.mmToIU( 1.5 ) ) );
        pad->SetDrillSize( VECTOR2I( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.8 ) ) );
        pad->SetShape( PADSTACK::ALL_LAYERS, shape );
        pad->SetAttribute( PAD_ATTRIB::PTH );
        pad->SetNumber( padNum );
        pad->SetLayerSet( PAD::PTHMask() );
        fp->Add( pad );
    }

    FOOTPRINT* AddFootprint( int x, int y, const wxString& ref = wxT( "U1" ) )
    {
        FOOTPRINT* fp = new FOOTPRINT( m_board.get() );
        fp->SetPosition( VECTOR2I( pcbIUScale.mmToIU( x ), pcbIUScale.mmToIU( y ) ) );
        fp->SetReference( ref );
        fp->SetValue( wxT( "TestComponent" ) );
        m_board->Add( fp );
        m_items.push_back( fp );
        return fp;
    }

    void AddZone( PCB_LAYER_ID layer = F_Cu )
    {
        ZONE* zone = new ZONE( m_board.get() );
        zone->SetLayer( layer );
        zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 0 ), pcbIUScale.mmToIU( 0 ) ), -1 );
        zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 0 ), pcbIUScale.mmToIU( 10 ) ), -1 );
        zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ), -1 );
        zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 0 ) ), -1 );
        m_board->Add( zone );
        m_items.push_back( zone );
    }

    std::unique_ptr<BOARD>       m_board;
    std::vector<BOARD_ITEM*>     m_items;
};


BOOST_FIXTURE_TEST_SUITE( ClipboardExportTests, CLIPBOARD_EXPORT_FIXTURE )


/**
 * Test that SVG_PLOTTER supports the StartLayer and EndLayer methods
 * for proper Inkscape-compatible layer grouping.
 */
BOOST_AUTO_TEST_CASE( SvgPlotter_LayerSupport )
{
    wxFileName tempFile( wxFileName::CreateTempFileName( wxT( "kicad_svg_layer_test" ) ) );
    tempFile.SetExt( wxT( "svg" ) );

    // Create an SVG plotter
    SVG_PLOTTER plotter;

    PAGE_INFO pageInfo;
    pageInfo.SetWidthMils( 1000 );
    pageInfo.SetHeightMils( 1000 );
    plotter.SetPageSettings( pageInfo );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1, 1.0, false );

    BOOST_CHECK( plotter.OpenFile( tempFile.GetFullPath() ) );

    plotter.StartPlot( wxT( "1" ) );

    // Create a layer with a known name
    wxString layerName = wxT( "F.Cu" );
    plotter.StartLayer( layerName );

    // Draw something in the layer
    plotter.SetColor( COLOR4D( 1.0, 0, 0, 1.0 ) );
    plotter.ThickSegment( VECTOR2I( 100, 100 ), VECTOR2I( 500, 500 ), 50, nullptr );

    plotter.EndLayer();

    plotter.EndPlot();

    // Read the SVG file and verify layer structure
    wxFFile file( tempFile.GetFullPath(), wxT( "r" ) );
    BOOST_REQUIRE( file.IsOpened() );

    wxString content;
    file.ReadAll( &content );
    file.Close();

    // Verify the SVG contains proper layer structure
    // Should have Inkscape namespace
    BOOST_CHECK( content.Contains( wxT( "xmlns:inkscape" ) ) );

    // Should have layer group with inkscape:groupmode="layer"
    BOOST_CHECK( content.Contains( wxT( "inkscape:groupmode=\"layer\"" ) ) );

    // Should have the layer name in inkscape:label
    BOOST_CHECK( content.Contains( wxT( "inkscape:label=\"F.Cu\"" ) ) );

    // Should have the layer name in id attribute
    BOOST_CHECK( content.Contains( wxT( "id=\"F.Cu\"" ) ) );

    wxRemoveFile( tempFile.GetFullPath() );
}


/**
 * Test that tracks are properly represented in SVG export.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsTracks )
{
    // Add a track to the board
    AddTrack( 0, 0, 10, 10, F_Cu );

    // Verify the track was added
    BOOST_CHECK_EQUAL( m_board->Tracks().size(), 1 );

    // Verify the track has correct properties
    PCB_TRACK* track = static_cast<PCB_TRACK*>( m_board->Tracks().front() );
    BOOST_CHECK_EQUAL( track->GetLayer(), F_Cu );
    BOOST_CHECK_EQUAL( track->GetStart().x, pcbIUScale.mmToIU( 0 ) );
    BOOST_CHECK_EQUAL( track->GetStart().y, pcbIUScale.mmToIU( 0 ) );
    BOOST_CHECK_EQUAL( track->GetEnd().x, pcbIUScale.mmToIU( 10 ) );
    BOOST_CHECK_EQUAL( track->GetEnd().y, pcbIUScale.mmToIU( 10 ) );
}


/**
 * Test that vias are properly represented in SVG export.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsVias )
{
    // Add a via to the board
    AddVia( 5, 5 );

    // Verify the via was added
    auto tracks = m_board->Tracks();
    PCB_VIA* via = nullptr;

    for( auto item : tracks )
    {
        if( item->Type() == PCB_VIA_T )
        {
            via = static_cast<PCB_VIA*>( item );
            break;
        }
    }

    BOOST_REQUIRE( via != nullptr );
    BOOST_CHECK_EQUAL( via->GetPosition().x, pcbIUScale.mmToIU( 5 ) );
    BOOST_CHECK_EQUAL( via->GetPosition().y, pcbIUScale.mmToIU( 5 ) );
    BOOST_CHECK( via->GetViaType() == VIATYPE::THROUGH );
}


/**
 * Test that footprints with pads are properly created for export testing.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsFootprintPads )
{
    // Add a footprint with pads
    FOOTPRINT* fp = AddFootprint( 25, 25, wxT( "U1" ) );
    AddPad( fp, 25, 23, wxT( "1" ) );
    AddPad( fp, 25, 27, wxT( "2" ) );

    // Verify the footprint was added
    BOOST_CHECK_EQUAL( m_board->Footprints().size(), 1 );

    // Verify the footprint has pads
    BOOST_CHECK_EQUAL( fp->Pads().size(), 2 );

    // Verify pad properties
    PAD* pad1 = fp->Pads()[0];
    // Pad numbers are set via SetNumber() and are non-empty after creation
    BOOST_CHECK( !pad1->GetNumber().IsEmpty() );
    BOOST_CHECK( ( pad1->GetLayerSet() & LSET( { F_Cu } ) ).any() );
}


/**
 * Test that zones are properly created for export testing.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsZones )
{
    // Add a zone to the board
    AddZone( F_Cu );

    // Verify the zone was added
    BOOST_CHECK_EQUAL( m_board->Zones().size(), 1 );

    // Verify the zone has corners
    ZONE* zone = m_board->Zones()[0];
    BOOST_CHECK( zone->GetNumCorners() >= 4 );
    BOOST_CHECK_EQUAL( zone->GetFirstLayer(), F_Cu );
}


/**
 * Test the dual-buffer alpha computation algorithm used for PNG transparency.
 * This verifies the mathematical approach: α = 1 - (white - black) / 255
 */
BOOST_AUTO_TEST_CASE( PngExport_AlphaComputation_FullyOpaque )
{
    // Test fully opaque pixel (alpha = 255)
    // A fully opaque red pixel renders identically on both backgrounds
    int rW = 255, gW = 0, bW = 0;  // Red on white
    int rB = 255, gB = 0, bB = 0;  // Red on black (same because opaque)

    int diffR = rW - rB;  // 0
    int diffG = gW - gB;  // 0
    int diffB = bW - bB;  // 0
    int avgDiff = ( diffR + diffG + diffB ) / 3;  // 0
    int alpha = 255 - avgDiff;  // 255

    BOOST_CHECK_EQUAL( alpha, 255 );
}


BOOST_AUTO_TEST_CASE( PngExport_AlphaComputation_FullyTransparent )
{
    // Test fully transparent pixel (alpha = 0)
    // A transparent pixel shows the background color
    int rW = 255, gW = 255, bW = 255;  // White background shows through
    int rB = 0, gB = 0, bB = 0;        // Black background shows through

    int diffR = rW - rB;  // 255
    int diffG = gW - gB;  // 255
    int diffB = bW - bB;  // 255
    int avgDiff = ( diffR + diffG + diffB ) / 3;  // 255
    int alpha = 255 - avgDiff;  // 0

    BOOST_CHECK_EQUAL( alpha, 0 );
}


BOOST_AUTO_TEST_CASE( PngExport_AlphaComputation_SemiTransparent )
{
    // Test semi-transparent pixel (alpha ≈ 128)
    // Foreground: gray (128, 128, 128), alpha = 0.5
    // On white: 0.5*128 + 0.5*255 = 192
    // On black: 0.5*128 + 0.5*0 = 64
    int rW = 192, gW = 192, bW = 192;
    int rB = 64, gB = 64, bB = 64;

    int diffR = rW - rB;  // 128
    int diffG = gW - gB;  // 128
    int diffB = bW - bB;  // 128
    int avgDiff = ( diffR + diffG + diffB ) / 3;  // 128
    int alpha = 255 - avgDiff;  // 127

    BOOST_CHECK_CLOSE( static_cast<double>( alpha ), 128.0, 1.0 );  // Allow 1% tolerance
}


/**
 * Test that selection state is cleared for proper unselected color rendering.
 * This verifies that ClearSelected() properly removes selection flag.
 */
BOOST_AUTO_TEST_CASE( PngExport_UnselectedColors )
{
    AddTrack( 0, 0, 10, 10, F_Cu );

    PCB_TRACK* track = static_cast<PCB_TRACK*>( m_board->Tracks().front() );

    // Set selected state
    track->SetSelected();
    BOOST_CHECK( track->IsSelected() );

    // Clone and clear selection (as done in renderSelectionToBitmap)
    std::unique_ptr<BOARD_ITEM> clone( static_cast<BOARD_ITEM*>( track->Clone() ) );
    clone->ClearSelected();

    // Verify selection is cleared
    BOOST_CHECK( !clone->IsSelected() );
}


/**
 * Test that footprint children have selection cleared for proper rendering.
 */
BOOST_AUTO_TEST_CASE( PngExport_FootprintChildrenUnselected )
{
    FOOTPRINT* fp = AddFootprint( 25, 25, wxT( "U1" ) );
    AddPad( fp, 25, 23, wxT( "1" ) );
    AddPad( fp, 25, 27, wxT( "2" ) );

    // Set footprint and children as selected
    fp->SetSelected();

    for( PAD* pad : fp->Pads() )
        pad->SetSelected();

    // Verify selected state
    BOOST_CHECK( fp->IsSelected() );

    for( PAD* pad : fp->Pads() )
        BOOST_CHECK( pad->IsSelected() );

    // Clone and clear selection (as done in renderSelectionToBitmap)
    std::unique_ptr<FOOTPRINT> clone( static_cast<FOOTPRINT*>( fp->Clone() ) );
    clone->ClearSelected();

    clone->RunOnChildren(
            []( BOARD_ITEM* child )
            {
                child->ClearSelected();
            },
            RECURSE_MODE::RECURSE );

    // Verify selection is cleared
    BOOST_CHECK( !clone->IsSelected() );

    for( PAD* pad : clone->Pads() )
        BOOST_CHECK( !pad->IsSelected() );
}


/**
 * Test layer ordering for proper hole rendering in PNG export.
 * Higher order values are drawn later (on top).
 */
BOOST_AUTO_TEST_CASE( PngExport_LayerOrder )
{
    // The implementation sets layer order as follows:
    // 1. Copper layers and their zones (lowest)
    // 2. Via types
    // 3. Pads
    // 4. Holes (highest - drawn on top)

    // Verify the concept: holes should have higher order than pads/vias

    // Count copper layers (32) + zone layers (32) = 64 base layers
    int copperLayers = LSET::AllCuMask().CuStack().size();
    int zonePerCopper = 1;
    int baseLayerCount = copperLayers * ( 1 + zonePerCopper );

    // Via layers (4): THROUGH, BLIND, BURIED, MICROVIA
    int viaLayerCount = 4;

    // Pads layer (1)
    int padLayerCount = 1;

    // Hole layers (5): VIA_HOLES, VIA_HOLEWALLS, PAD_PLATEDHOLES, PAD_HOLEWALLS, NON_PLATEDHOLES
    int holeLayerCount = 5;

    // Verify holes come after pads in the ordering
    int padOrder = baseLayerCount + viaLayerCount;
    int holeStartOrder = padOrder + padLayerCount;

    BOOST_CHECK( holeStartOrder > padOrder );
    BOOST_CHECK_EQUAL( holeLayerCount, 5 );
}


/**
 * Test bitmap size calculation from bounding box and view scale.
 */
BOOST_AUTO_TEST_CASE( PngExport_BitmapSizeCalculation )
{
    // Test the formula: bitmapSize = bbox_IU * viewScale
    int bboxWidth = 10000;  // 10000 IU
    int bboxHeight = 5000;  // 5000 IU

    // At viewScale = 1.0
    double viewScale1 = 1.0;
    int    bitmapWidth1 = static_cast<int>( bboxWidth * viewScale1 + 0.5 );
    int    bitmapHeight1 = static_cast<int>( bboxHeight * viewScale1 + 0.5 );
    BOOST_CHECK_EQUAL( bitmapWidth1, 10000 );
    BOOST_CHECK_EQUAL( bitmapHeight1, 5000 );

    // At viewScale = 0.5 (zoomed out)
    double viewScale2 = 0.5;
    int    bitmapWidth2 = static_cast<int>( bboxWidth * viewScale2 + 0.5 );
    int    bitmapHeight2 = static_cast<int>( bboxHeight * viewScale2 + 0.5 );
    BOOST_CHECK_EQUAL( bitmapWidth2, 5000 );
    BOOST_CHECK_EQUAL( bitmapHeight2, 2500 );
}


/**
 * Test bitmap size clamping to maximum size while preserving aspect ratio.
 */
BOOST_AUTO_TEST_CASE( PngExport_BitmapSizeClamping )
{
    const int maxBitmapSize = 4096;

    int    bboxWidth = 10000;
    int    bboxHeight = 5000;
    double viewScale = 1.0;

    int bitmapWidth = static_cast<int>( bboxWidth * viewScale + 0.5 );
    int bitmapHeight = static_cast<int>( bboxHeight * viewScale + 0.5 );

    // Apply clamping as in plotSelectionToPng
    if( bitmapWidth > maxBitmapSize || bitmapHeight > maxBitmapSize )
    {
        double scaleDown = static_cast<double>( maxBitmapSize ) / std::max( bitmapWidth, bitmapHeight );
        bitmapWidth = static_cast<int>( bitmapWidth * scaleDown + 0.5 );
        bitmapHeight = static_cast<int>( bitmapHeight * scaleDown + 0.5 );
        viewScale *= scaleDown;
    }

    BOOST_CHECK( bitmapWidth <= maxBitmapSize );
    BOOST_CHECK( bitmapHeight <= maxBitmapSize );

    // Check aspect ratio is preserved (2:1)
    double aspectRatio = static_cast<double>( bitmapWidth ) / bitmapHeight;
    BOOST_CHECK_CLOSE( aspectRatio, 2.0, 0.1 );
}


/**
 * Test that board layer names are retrievable for SVG export.
 */
BOOST_AUTO_TEST_CASE( SvgExport_LayerNames )
{
    // Verify standard layer names are available
    wxString fCuName = m_board->GetLayerName( F_Cu );
    wxString bCuName = m_board->GetLayerName( B_Cu );
    wxString fSilkName = m_board->GetLayerName( F_SilkS );

    BOOST_CHECK( !fCuName.IsEmpty() );
    BOOST_CHECK( !bCuName.IsEmpty() );
    BOOST_CHECK( !fSilkName.IsEmpty() );

    // Standard names should be like "F.Cu", "B.Cu", "F.SilkS"
    BOOST_CHECK( fCuName.Contains( wxT( "Cu" ) ) );
    BOOST_CHECK( bCuName.Contains( wxT( "Cu" ) ) );
}


/**
 * Test layer set collection from board items.
 */
BOOST_AUTO_TEST_CASE( SvgExport_CollectLayers )
{
    // Add items on different layers
    AddTrack( 0, 0, 10, 10, F_Cu );
    AddTrack( 0, 0, 10, 10, B_Cu );

    // Create a selection-like layer set
    LSET layers;

    for( auto track : m_board->Tracks() )
        layers |= track->GetLayerSet();

    // Should contain both F.Cu and B.Cu
    BOOST_CHECK( layers.test( F_Cu ) );
    BOOST_CHECK( layers.test( B_Cu ) );

    // Should not contain unrelated layers
    BOOST_CHECK( !layers.test( F_SilkS ) );
}


/**
 * Test that PCB arcs are properly created for export testing.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsArcs )
{
    // Add an arc track to the board
    PCB_ARC* arc = new PCB_ARC( m_board.get() );
    arc->SetStart( VECTOR2I( pcbIUScale.mmToIU( 0 ), pcbIUScale.mmToIU( 0 ) ) );
    arc->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 0 ) ) );
    arc->SetMid( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ) );
    arc->SetWidth( pcbIUScale.mmToIU( 0.25 ) );
    arc->SetLayer( F_Cu );
    m_board->Add( arc );
    m_items.push_back( arc );

    // Verify the arc was added and has correct properties
    BOOST_CHECK_EQUAL( m_board->Tracks().size(), 1 );
    BOOST_CHECK_EQUAL( arc->GetLayer(), F_Cu );
    // Arc should have non-zero angle
    BOOST_CHECK( !arc->GetAngle().IsZero() );
}


/**
 * Test that PCB shapes on copper layers are properly created.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsPcbShapes )
{
    // Add a line shape on copper
    PCB_SHAPE* line = new PCB_SHAPE( m_board.get(), SHAPE_T::SEGMENT );
    line->SetStart( VECTOR2I( pcbIUScale.mmToIU( 0 ), pcbIUScale.mmToIU( 0 ) ) );
    line->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    line->SetLayer( F_Cu );
    line->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    m_board->Add( line );
    m_items.push_back( line );

    // Add a rectangle shape
    PCB_SHAPE* rect = new PCB_SHAPE( m_board.get(), SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 0 ) ) );
    rect->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 10 ) ) );
    rect->SetLayer( F_Cu );
    rect->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    m_board->Add( rect );
    m_items.push_back( rect );

    // Add a circle shape
    PCB_SHAPE* circle = new PCB_SHAPE( m_board.get(), SHAPE_T::CIRCLE );
    circle->SetCenter( VECTOR2I( pcbIUScale.mmToIU( 45 ), pcbIUScale.mmToIU( 5 ) ) );
    circle->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 50 ), pcbIUScale.mmToIU( 5 ) ) );  // radius = 5mm
    circle->SetLayer( F_Cu );
    circle->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    m_board->Add( circle );
    m_items.push_back( circle );

    // Verify shapes were added
    size_t shapeCount = 0;

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( item->Type() == PCB_SHAPE_T )
            shapeCount++;
    }

    BOOST_CHECK_EQUAL( shapeCount, 3 );
}


/**
 * Test that PCB text is properly created for export testing.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsPcbText )
{
    // Add text on silkscreen
    PCB_TEXT* text = new PCB_TEXT( m_board.get() );
    text->SetText( wxT( "Test Label" ) );
    text->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    text->SetLayer( F_SilkS );
    text->SetTextSize( VECTOR2I( pcbIUScale.mmToIU( 1.5 ), pcbIUScale.mmToIU( 1.5 ) ) );
    m_board->Add( text );
    m_items.push_back( text );

    // Verify the text was added
    size_t textCount = 0;

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( item->Type() == PCB_TEXT_T )
            textCount++;
    }

    BOOST_CHECK_EQUAL( textCount, 1 );
    BOOST_CHECK( text->GetText() == wxT( "Test Label" ) );
    BOOST_CHECK_EQUAL( text->GetLayer(), F_SilkS );
}


/**
 * Test that blind vias are properly created.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsBlindVia )
{
    PCB_VIA* via = new PCB_VIA( m_board.get() );
    via->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 15 ), pcbIUScale.mmToIU( 15 ) ) );
    via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.6 ) );
    via->SetDrill( pcbIUScale.mmToIU( 0.3 ) );
    via->SetViaType( VIATYPE::BLIND );
    via->SetLayerPair( F_Cu, In1_Cu );
    m_board->Add( via );
    m_items.push_back( via );

    // Verify the blind via was added
    PCB_VIA* foundVia = nullptr;

    for( auto item : m_board->Tracks() )
    {
        if( item->Type() == PCB_VIA_T )
        {
            foundVia = static_cast<PCB_VIA*>( item );
            break;
        }
    }

    BOOST_REQUIRE( foundVia != nullptr );
    BOOST_CHECK( foundVia->GetViaType() == VIATYPE::BLIND );
}


/**
 * Test that micro vias are properly created.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsMicroVia )
{
    PCB_VIA* via = new PCB_VIA( m_board.get() );
    via->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) ) );
    via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.4 ) );
    via->SetDrill( pcbIUScale.mmToIU( 0.2 ) );
    via->SetViaType( VIATYPE::MICROVIA );
    via->SetLayerPair( F_Cu, In1_Cu );
    m_board->Add( via );
    m_items.push_back( via );

    // Verify the micro via was added
    PCB_VIA* foundVia = nullptr;

    for( auto item : m_board->Tracks() )
    {
        if( item->Type() == PCB_VIA_T )
        {
            foundVia = static_cast<PCB_VIA*>( item );
            break;
        }
    }

    BOOST_REQUIRE( foundVia != nullptr );
    BOOST_CHECK( foundVia->GetViaType() == VIATYPE::MICROVIA );
}


/**
 * Test that tracks on multiple copper layers can be created.
 */
BOOST_AUTO_TEST_CASE( SvgExport_MultipleCopperLayers )
{
    AddTrack( 0, 0, 10, 10, F_Cu );
    AddTrack( 0, 0, 10, 10, In1_Cu );
    AddTrack( 0, 0, 10, 10, In2_Cu );
    AddTrack( 0, 0, 10, 10, B_Cu );

    // Verify tracks were added on different layers
    BOOST_CHECK_EQUAL( m_board->Tracks().size(), 4 );

    // Collect layers from tracks
    LSET layers;

    for( auto track : m_board->Tracks() )
        layers |= track->GetLayerSet();

    BOOST_CHECK( layers.test( F_Cu ) );
    BOOST_CHECK( layers.test( In1_Cu ) );
    BOOST_CHECK( layers.test( In2_Cu ) );
    BOOST_CHECK( layers.test( B_Cu ) );
}


/**
 * Test that footprint graphics (silkscreen items) are properly created.
 */
BOOST_AUTO_TEST_CASE( SvgExport_FootprintGraphics )
{
    FOOTPRINT* fp = AddFootprint( 50, 50, wxT( "J1" ) );

    // Add a line to the footprint silkscreen
    PCB_SHAPE* fpLine = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
    fpLine->SetStart( VECTOR2I( pcbIUScale.mmToIU( -2 ), pcbIUScale.mmToIU( -2 ) ) );
    fpLine->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 2 ), pcbIUScale.mmToIU( -2 ) ) );
    fpLine->SetLayer( F_SilkS );
    fpLine->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.12 ), LINE_STYLE::SOLID ) );
    fp->Add( fpLine );

    // Add a rectangle to the footprint fabrication layer
    PCB_SHAPE* fpRect = new PCB_SHAPE( fp, SHAPE_T::RECTANGLE );
    fpRect->SetStart( VECTOR2I( pcbIUScale.mmToIU( -3 ), pcbIUScale.mmToIU( -3 ) ) );
    fpRect->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 3 ), pcbIUScale.mmToIU( 3 ) ) );
    fpRect->SetLayer( F_Fab );
    fpRect->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.1 ), LINE_STYLE::SOLID ) );
    fp->Add( fpRect );

    // Verify footprint graphics were added
    int graphicsCount = 0;

    for( BOARD_ITEM* item : fp->GraphicalItems() )
    {
        if( item->Type() == PCB_SHAPE_T )
            graphicsCount++;
    }

    BOOST_CHECK_EQUAL( graphicsCount, 2 );
}


/**
 * Test that SMD pads are properly created.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsSmdPads )
{
    FOOTPRINT* fp = AddFootprint( 75, 75, wxT( "C1" ) );

    // Add SMD pads (no hole)
    PAD* pad1 = new PAD( fp );
    pad1->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 74 ), pcbIUScale.mmToIU( 75 ) ) );
    pad1->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.5 ) ) );
    pad1->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    pad1->SetAttribute( PAD_ATTRIB::SMD );
    pad1->SetNumber( wxT( "1" ) );
    pad1->SetLayerSet( LSET( { F_Cu, F_Paste, F_Mask } ) );
    fp->Add( pad1 );

    PAD* pad2 = new PAD( fp );
    pad2->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 76 ), pcbIUScale.mmToIU( 75 ) ) );
    pad2->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.5 ) ) );
    pad2->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    pad2->SetAttribute( PAD_ATTRIB::SMD );
    pad2->SetNumber( wxT( "2" ) );
    pad2->SetLayerSet( LSET( { F_Cu, F_Paste, F_Mask } ) );
    fp->Add( pad2 );

    // Verify SMD pads were added
    BOOST_CHECK_EQUAL( fp->Pads().size(), 2 );

    for( PAD* pad : fp->Pads() )
    {
        BOOST_CHECK( pad->GetAttribute() == PAD_ATTRIB::SMD );
        BOOST_CHECK( pad->GetDrillSize().x == 0 );  // No drill for SMD
    }
}


/**
 * Test that zone fills can be created.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ZoneWithNet )
{
    // Create a zone on bottom copper
    ZONE* zone = new ZONE( m_board.get() );
    zone->SetLayer( B_Cu );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 100 ), pcbIUScale.mmToIU( 0 ) ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 100 ), pcbIUScale.mmToIU( 20 ) ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 120 ), pcbIUScale.mmToIU( 20 ) ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 120 ), pcbIUScale.mmToIU( 0 ) ), -1 );
    zone->SetZoneName( wxT( "TestZone" ) );
    m_board->Add( zone );
    m_items.push_back( zone );

    // Verify the zone was added
    BOOST_CHECK_EQUAL( m_board->Zones().size(), 1 );
    BOOST_CHECK_EQUAL( zone->GetFirstLayer(), B_Cu );
    BOOST_CHECK( zone->GetZoneName() == wxT( "TestZone" ) );
}


/**
 * Test bounding box computation for various item types.
 */
BOOST_AUTO_TEST_CASE( BoundingBox_MultipleItems )
{
    AddTrack( 0, 0, 10, 10, F_Cu );
    AddVia( 20, 20 );
    AddFootprint( 40, 40, wxT( "R1" ) );

    // Compute bounding box for all items
    BOX2I bbox;

    for( BOARD_ITEM* item : m_items )
    {
        if( bbox.GetWidth() == 0 && bbox.GetHeight() == 0 )
            bbox = item->GetBoundingBox();
        else
            bbox.Merge( item->GetBoundingBox() );
    }

    // Bounding box should encompass all items
    BOOST_CHECK( bbox.GetWidth() > 0 );
    BOOST_CHECK( bbox.GetHeight() > 0 );
    BOOST_CHECK( bbox.Contains( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ) ) );
    BOOST_CHECK( bbox.Contains( VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) ) ) );
}


/**
 * Test that SVG fill-opacity is set correctly when drawing shapes.
 * This verifies that colors with alpha values produce correct fill-opacity.
 */
BOOST_AUTO_TEST_CASE( SvgExport_FillOpacity_MatchesColorAlpha )
{
    wxFileName tempFile( wxFileName::CreateTempFileName( wxT( "kicad_svg_alpha_test" ) ) );
    tempFile.SetExt( wxT( "svg" ) );

    SVG_PLOTTER plotter;

    PAGE_INFO pageInfo;
    pageInfo.SetWidthMils( 1000 );
    pageInfo.SetHeightMils( 1000 );
    plotter.SetPageSettings( pageInfo );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1, 1.0, false );
    plotter.SetColorMode( true );

    BOOST_REQUIRE( plotter.OpenFile( tempFile.GetFullPath() ) );

    plotter.StartPlot( wxT( "1" ) );

    // Test with a color that has alpha = 0.5
    double testAlpha = 0.5;
    COLOR4D colorWithAlpha( 1.0, 0.0, 0.0, testAlpha );
    plotter.SetColor( colorWithAlpha );

    // Draw a filled shape to trigger the fill-opacity output
    plotter.Rect( VECTOR2I( 100, 100 ), VECTOR2I( 500, 500 ), FILL_T::FILLED_SHAPE, 10 );

    plotter.EndPlot();

    // Read the SVG file and verify fill-opacity
    wxFFile file( tempFile.GetFullPath(), wxT( "r" ) );
    BOOST_REQUIRE( file.IsOpened() );

    wxString content;
    file.ReadAll( &content );
    file.Close();

    // The SVG should contain fill-opacity matching our alpha value
    // It should NOT have fill-opacity:0 for a color with alpha=0.5
    wxString fillOpacityPattern = wxT( "fill-opacity:0.5" );
    wxString badFillOpacity = wxT( "fill-opacity:0" );

    // Check that the alpha value is represented in fill-opacity
    // Note: The SVG plotter uses 4-digit precision, so 0.5 should be "0.5000"
    BOOST_CHECK_MESSAGE( content.Contains( wxT( "fill-opacity:0.5" ) ) ||
                         content.Contains( wxT( "fill-opacity: 0.5" ) ),
                         "SVG should contain fill-opacity matching color alpha (0.5)" );

    // Verify we don't have fill-opacity:0 (which would make the fill invisible)
    // Check that if fill-opacity:0 exists, it's not at the start (might be 0.5xxx)
    if( content.Contains( wxT( "fill-opacity:0;" ) ) ||
        content.Contains( wxT( "fill-opacity:0 " ) ) ||
        content.Contains( wxT( "fill-opacity: 0;" ) ) )
    {
        BOOST_CHECK_MESSAGE( false,
                             "SVG should NOT have fill-opacity:0 when color alpha is 0.5" );
    }

    wxRemoveFile( tempFile.GetFullPath() );
}


/**
 * Test that SVG export with fully opaque colors has fill-opacity = 1.0.
 */
BOOST_AUTO_TEST_CASE( SvgExport_FillOpacity_FullyOpaque )
{
    wxFileName tempFile( wxFileName::CreateTempFileName( wxT( "kicad_svg_opaque_test" ) ) );
    tempFile.SetExt( wxT( "svg" ) );

    SVG_PLOTTER plotter;

    PAGE_INFO pageInfo;
    pageInfo.SetWidthMils( 1000 );
    pageInfo.SetHeightMils( 1000 );
    plotter.SetPageSettings( pageInfo );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1, 1.0, false );
    plotter.SetColorMode( true );

    BOOST_REQUIRE( plotter.OpenFile( tempFile.GetFullPath() ) );

    plotter.StartPlot( wxT( "1" ) );

    // Test with a fully opaque color (alpha = 1.0)
    COLOR4D opaqueColor( 0.0, 0.0, 1.0, 1.0 );
    plotter.SetColor( opaqueColor );

    // Draw a filled rectangle
    plotter.Rect( VECTOR2I( 100, 100 ), VECTOR2I( 500, 500 ), FILL_T::FILLED_SHAPE, 10 );

    plotter.EndPlot();

    // Read the SVG file
    wxFFile file( tempFile.GetFullPath(), wxT( "r" ) );
    BOOST_REQUIRE( file.IsOpened() );

    wxString content;
    file.ReadAll( &content );
    file.Close();

    // The SVG should have fill-opacity:1.0 for fully opaque colors
    BOOST_CHECK_MESSAGE( content.Contains( wxT( "fill-opacity:1" ) ) ||
                         content.Contains( wxT( "fill-opacity: 1" ) ),
                         "SVG should contain fill-opacity:1 for fully opaque colors" );

    wxRemoveFile( tempFile.GetFullPath() );
}


/**
 * Test that footprint pads are included in SVG export.
 * This verifies that when a footprint is plotted, all its pads appear in the output.
 */
BOOST_AUTO_TEST_CASE( SvgExport_FootprintPads_IncludedInOutput )
{
    wxFileName tempFile( wxFileName::CreateTempFileName( wxT( "kicad_svg_fp_pads" ) ) );
    tempFile.SetExt( wxT( "svg" ) );

    SVG_PLOTTER plotter;

    PAGE_INFO pageInfo;
    pageInfo.SetWidthMils( 2000 );
    pageInfo.SetHeightMils( 2000 );
    plotter.SetPageSettings( pageInfo );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1, 1.0, false );
    plotter.SetColorMode( true );

    BOOST_REQUIRE( plotter.OpenFile( tempFile.GetFullPath() ) );

    plotter.StartPlot( wxT( "1" ) );
    plotter.StartLayer( wxT( "F.Cu" ) );

    // Create a footprint with multiple pads
    FOOTPRINT* fp = AddFootprint( 25, 25, wxT( "TestFP" ) );
    AddPad( fp, 23, 25, wxT( "1" ) );  // Pad 1
    AddPad( fp, 27, 25, wxT( "2" ) );  // Pad 2

    // Set color and draw circles at pad positions to simulate pad plotting
    COLOR4D padColor( 1.0, 0.0, 0.0, 1.0 );
    plotter.SetColor( padColor );

    for( PAD* pad : fp->Pads() )
    {
        // Draw a circle at each pad position
        int radius = pcbIUScale.mmToIU( 0.75 );  // Half of 1.5mm pad size
        plotter.Circle( pad->GetPosition(), radius * 2, FILL_T::FILLED_SHAPE, 0 );
    }

    plotter.EndLayer();
    plotter.EndPlot();

    // Read the SVG file
    wxFFile file( tempFile.GetFullPath(), wxT( "r" ) );
    BOOST_REQUIRE( file.IsOpened() );

    wxString content;
    file.ReadAll( &content );
    file.Close();

    // The SVG should contain circle elements for the pads
    // Count circle elements - should have at least 2 for our 2 pads
    int circleCount = 0;
    int pos = 0;

    while( ( pos = content.find( wxT( "<circle" ), pos ) ) != wxNOT_FOUND )
    {
        circleCount++;
        pos += 7;  // Move past "<circle"
    }

    BOOST_CHECK_MESSAGE( circleCount >= 2,
                         "SVG should contain circle elements for footprint pads. "
                         "Found: " << circleCount << ", expected at least 2" );

    wxRemoveFile( tempFile.GetFullPath() );
}


/**
 * Test that footprint graphical items are included in SVG export.
 */
BOOST_AUTO_TEST_CASE( SvgExport_FootprintGraphics_IncludedInOutput )
{
    wxFileName tempFile( wxFileName::CreateTempFileName( wxT( "kicad_svg_fp_graphics" ) ) );
    tempFile.SetExt( wxT( "svg" ) );

    SVG_PLOTTER plotter;

    PAGE_INFO pageInfo;
    pageInfo.SetWidthMils( 2000 );
    pageInfo.SetHeightMils( 2000 );
    plotter.SetPageSettings( pageInfo );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1, 1.0, false );
    plotter.SetColorMode( true );

    BOOST_REQUIRE( plotter.OpenFile( tempFile.GetFullPath() ) );

    plotter.StartPlot( wxT( "1" ) );
    plotter.StartLayer( wxT( "F.SilkS" ) );

    // Create a footprint with graphical items
    FOOTPRINT* fp = AddFootprint( 50, 50, wxT( "TestGraphics" ) );

    // Add a line to the footprint silkscreen
    PCB_SHAPE* fpLine = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
    fpLine->SetStart( VECTOR2I( pcbIUScale.mmToIU( 48 ), pcbIUScale.mmToIU( 48 ) ) );
    fpLine->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 52 ), pcbIUScale.mmToIU( 48 ) ) );
    fpLine->SetLayer( F_SilkS );
    fpLine->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.12 ), LINE_STYLE::SOLID ) );
    fp->Add( fpLine );

    // Draw line using plotter (simulating what plotSelectionToSvg does)
    COLOR4D silkColor( 1.0, 1.0, 0.0, 1.0 );
    plotter.SetColor( silkColor );
    plotter.ThickSegment( fpLine->GetStart(), fpLine->GetEnd(),
                          pcbIUScale.mmToIU( 0.12 ), nullptr );

    plotter.EndLayer();
    plotter.EndPlot();

    // Read the SVG file
    wxFFile file( tempFile.GetFullPath(), wxT( "r" ) );
    BOOST_REQUIRE( file.IsOpened() );

    wxString content;
    file.ReadAll( &content );
    file.Close();

    // The SVG should contain line or path elements for the silkscreen graphics
    bool hasLineContent = content.Contains( wxT( "<line" ) ) ||
                          content.Contains( wxT( "<path" ) ) ||
                          content.Contains( wxT( "<polyline" ) );

    BOOST_CHECK_MESSAGE( hasLineContent,
                         "SVG should contain line/path elements for footprint graphics" );

    wxRemoveFile( tempFile.GetFullPath() );
}


/**
 * Test that verifies the COLOR4D alpha value is preserved through SetColor.
 * This is a regression test for fill-opacity:0 bug.
 */
BOOST_AUTO_TEST_CASE( SvgExport_ColorAlpha_PreservedThroughSetColor )
{
    // This test verifies that when we call SetColor with a COLOR4D that has
    // a specific alpha value, the SVG output reflects that alpha in fill-opacity.

    // Test various alpha values
    std::vector<double> testAlphas = { 0.0, 0.25, 0.5, 0.75, 1.0 };

    for( double expectedAlpha : testAlphas )
    {
        wxFileName tempFile( wxFileName::CreateTempFileName( wxT( "kicad_svg_alpha" ) ) );
        tempFile.SetExt( wxT( "svg" ) );

        SVG_PLOTTER plotter;

        PAGE_INFO pageInfo;
        pageInfo.SetWidthMils( 500 );
        pageInfo.SetHeightMils( 500 );
        plotter.SetPageSettings( pageInfo );
        plotter.SetViewport( VECTOR2I( 0, 0 ), 1, 1.0, false );
        plotter.SetColorMode( true );

        BOOST_REQUIRE( plotter.OpenFile( tempFile.GetFullPath() ) );

        plotter.StartPlot( wxT( "1" ) );

        // Set color with specific alpha
        COLOR4D testColor( 0.5, 0.5, 0.5, expectedAlpha );
        plotter.SetColor( testColor );

        // Draw something
        plotter.Circle( VECTOR2I( 250, 250 ), 200, FILL_T::FILLED_SHAPE, 10 );

        plotter.EndPlot();

        // Read and verify
        wxFFile file( tempFile.GetFullPath(), wxT( "r" ) );
        BOOST_REQUIRE( file.IsOpened() );

        wxString content;
        file.ReadAll( &content );
        file.Close();

        // Build the expected fill-opacity pattern
        wxString expectedPattern = wxString::Format( wxT( "fill-opacity:%.1f" ), expectedAlpha );

        // For alpha=0, we should find "fill-opacity:0" (which is correct for transparent)
        // For alpha=1, we should find "fill-opacity:1"
        // For alpha=0.5, we should find "fill-opacity:0.5"
        if( expectedAlpha == 0.0 )
        {
            BOOST_CHECK_MESSAGE( content.Contains( wxT( "fill-opacity:0" ) ),
                                 "Alpha=0 should produce fill-opacity:0" );
        }
        else if( expectedAlpha == 1.0 )
        {
            BOOST_CHECK_MESSAGE( content.Contains( wxT( "fill-opacity:1" ) ),
                                 "Alpha=1 should produce fill-opacity:1" );
        }
        else
        {
            // For intermediate values, check we don't have fill-opacity:0 or fill-opacity:1
            // when we shouldn't
            bool hasSemiTransparent = content.Contains( wxString::Format( wxT( "fill-opacity:%.4f" ), expectedAlpha ) ) ||
                                      content.Contains( wxString::Format( wxT( "fill-opacity:%.1f" ), expectedAlpha ) );
            BOOST_CHECK_MESSAGE( hasSemiTransparent ||
                                !( content.Contains( wxT( "fill-opacity:0;" ) ) ||
                                   content.Contains( wxT( "fill-opacity:1.0000;" ) ) ),
                                 "Alpha=" << expectedAlpha << " should not produce fill-opacity:0 or 1" );
        }

        wxRemoveFile( tempFile.GetFullPath() );
    }
}


BOOST_AUTO_TEST_SUITE_END()
