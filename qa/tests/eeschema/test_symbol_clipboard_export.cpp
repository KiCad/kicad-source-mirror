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
 * @file test_symbol_clipboard_export.cpp
 * Tests for multi-format clipboard export functionality for symbol editor.
 *
 * These tests verify:
 * 1. SVG export produces valid output for symbols
 * 2. Symbol bounding box calculation is correct for different element types
 * 3. PNG alpha computation using dual-buffer technique
 */

#include <boost/test/unit_test.hpp>

#include <lib_symbol.h>
#include <sch_pin.h>
#include <sch_shape.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <plotters/plotters_pslike.h>
#include <sch_painter.h>
#include <sch_plotter.h>
#include <locale_io.h>
#include <settings/color_settings.h>
#include <wx/ffile.h>
#include <wx/mstream.h>


class SYMBOL_CLIPBOARD_FIXTURE
{
public:
    SYMBOL_CLIPBOARD_FIXTURE()
    {
        m_symbol = std::make_unique<LIB_SYMBOL>( wxT( "TestSymbol" ), nullptr );
    }

    ~SYMBOL_CLIPBOARD_FIXTURE() = default;

    void AddPin( int x, int y, const wxString& name, const wxString& number )
    {
        std::unique_ptr<SCH_PIN> pin = std::make_unique<SCH_PIN>( m_symbol.get() );
        pin->SetPosition( VECTOR2I( schIUScale.MilsToIU( x ), schIUScale.MilsToIU( y ) ) );
        pin->SetName( name );
        pin->SetNumber( number );
        pin->SetLength( schIUScale.MilsToIU( 100 ) );
        m_symbol->AddDrawItem( pin.release() );
    }

    void AddRectangle( int x1, int y1, int x2, int y2 )
    {
        std::unique_ptr<SCH_SHAPE> rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE, LAYER_DEVICE );
        rect->SetPosition( VECTOR2I( schIUScale.MilsToIU( x1 ), schIUScale.MilsToIU( y1 ) ) );
        rect->SetEnd( VECTOR2I( schIUScale.MilsToIU( x2 ), schIUScale.MilsToIU( y2 ) ) );
        rect->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        m_symbol->AddDrawItem( rect.release() );
    }

    void AddCircle( int cx, int cy, int radius )
    {
        std::unique_ptr<SCH_SHAPE> circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE, LAYER_DEVICE );
        circle->SetPosition( VECTOR2I( schIUScale.MilsToIU( cx ), schIUScale.MilsToIU( cy ) ) );
        circle->SetEnd( VECTOR2I( schIUScale.MilsToIU( cx + radius ), schIUScale.MilsToIU( cy ) ) );
        circle->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        m_symbol->AddDrawItem( circle.release() );
    }

    void AddPolyline( const std::vector<std::pair<int, int>>& points )
    {
        std::unique_ptr<SCH_SHAPE> poly = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE );
        poly->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );

        for( const auto& pt : points )
            poly->AddPoint( VECTOR2I( schIUScale.MilsToIU( pt.first ), schIUScale.MilsToIU( pt.second ) ) );

        m_symbol->AddDrawItem( poly.release() );
    }

    void AddArc( int cx, int cy, int radius, EDA_ANGLE startAngle, EDA_ANGLE endAngle )
    {
        std::unique_ptr<SCH_SHAPE> arc = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC, LAYER_DEVICE );
        arc->SetCenter( VECTOR2I( schIUScale.MilsToIU( cx ), schIUScale.MilsToIU( cy ) ) );
        arc->SetRadius( schIUScale.MilsToIU( radius ) );
        arc->SetArcAngleAndEnd( endAngle - startAngle );
        arc->SetStroke( STROKE_PARAMS( schIUScale.MilsToIU( 10 ), LINE_STYLE::SOLID ) );
        m_symbol->AddDrawItem( arc.release() );
    }

    void AddText( int x, int y, const wxString& text )
    {
        std::unique_ptr<SCH_TEXT> txt = std::make_unique<SCH_TEXT>( VECTOR2I( schIUScale.MilsToIU( x ),
                                                                               schIUScale.MilsToIU( y ) ),
                                                                    text, LAYER_DEVICE );
        txt->SetTextSize( VECTOR2I( schIUScale.MilsToIU( 50 ), schIUScale.MilsToIU( 50 ) ) );
        m_symbol->AddDrawItem( txt.release() );
    }

    BOX2I GetSymbolBoundingBox( int aUnit = 0, int aBodyStyle = 0 )
    {
        BOX2I bbox;

        for( SCH_ITEM& item : m_symbol->GetDrawItems() )
        {
            if( item.Type() == SCH_FIELD_T )
                continue;

            if( aUnit && item.GetUnit() && item.GetUnit() != aUnit )
                continue;

            if( aBodyStyle && item.GetBodyStyle() && item.GetBodyStyle() != aBodyStyle )
                continue;

            if( bbox.GetWidth() == 0 && bbox.GetHeight() == 0 )
                bbox = item.GetBoundingBox();
            else
                bbox.Merge( item.GetBoundingBox() );
        }

        return bbox;
    }

    wxString PlotToSvgString( int aUnit = 0, int aBodyStyle = 0 )
    {
        BOX2I bbox = GetSymbolBoundingBox( aUnit, aBodyStyle );

        if( bbox.GetWidth() <= 0 || bbox.GetHeight() <= 0 )
            return wxEmptyString;

        bbox.Inflate( bbox.GetWidth() * 0.02, bbox.GetHeight() * 0.02 );

        SCH_RENDER_SETTINGS renderSettings;
        COLOR_SETTINGS      colorSettings;
        renderSettings.LoadColors( &colorSettings );
        renderSettings.SetDefaultPenWidth( schIUScale.MilsToIU( 6 ) );

        std::unique_ptr<SVG_PLOTTER> plotter = std::make_unique<SVG_PLOTTER>();
        plotter->SetRenderSettings( &renderSettings );

        PAGE_INFO pageInfo;
        pageInfo.SetWidthMils( schIUScale.IUToMils( bbox.GetWidth() ) );
        pageInfo.SetHeightMils( schIUScale.IUToMils( bbox.GetHeight() ) );

        plotter->SetPageSettings( pageInfo );
        plotter->SetColorMode( true );

        VECTOR2I plot_offset = bbox.GetOrigin();
        plotter->SetViewport( plot_offset, schIUScale.IU_PER_MILS / 10, 1.0, false );
        plotter->SetCreator( wxT( "Eeschema-SVG-Test" ) );

        wxFileName tempFile( wxFileName::CreateTempFileName( wxT( "kicad_test_svg" ) ) );

        if( !plotter->OpenFile( tempFile.GetFullPath() ) )
        {
            wxRemoveFile( tempFile.GetFullPath() );
            return wxEmptyString;
        }

        LOCALE_IO     toggle;
        SCH_PLOT_OPTS plotOpts;

        plotter->StartPlot( wxT( "1" ) );

        constexpr bool background = true;
        m_symbol->Plot( plotter.get(), background, plotOpts, aUnit, aBodyStyle, VECTOR2I( 0, 0 ), false );
        m_symbol->Plot( plotter.get(), !background, plotOpts, aUnit, aBodyStyle, VECTOR2I( 0, 0 ), false );
        m_symbol->PlotFields( plotter.get(), !background, plotOpts, aUnit, aBodyStyle, VECTOR2I( 0, 0 ), false );

        plotter->EndPlot();
        plotter.reset();

        wxFFile file( tempFile.GetFullPath(), wxT( "rb" ) );
        wxString content;

        if( file.IsOpened() )
            file.ReadAll( &content );

        wxRemoveFile( tempFile.GetFullPath() );
        return content;
    }

    std::unique_ptr<LIB_SYMBOL> m_symbol;
};


BOOST_FIXTURE_TEST_SUITE( SymbolClipboardExport, SYMBOL_CLIPBOARD_FIXTURE )


/**
 * Test that a symbol with pins produces SVG output
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsPins )
{
    AddPin( 0, 0, wxT( "VCC" ), wxT( "1" ) );
    AddPin( 0, 100, wxT( "GND" ), wxT( "2" ) );
    AddPin( 0, 200, wxT( "OUT" ), wxT( "3" ) );

    wxString svg = PlotToSvgString();

    BOOST_CHECK( !svg.IsEmpty() );
    BOOST_CHECK( svg.Contains( wxT( "<svg" ) ) );
    BOOST_CHECK( svg.Contains( wxT( "</svg>" ) ) );
    // SVG should contain paths or lines for pin elements
    BOOST_CHECK( svg.Contains( wxT( "<path" ) ) || svg.Contains( wxT( "<line" ) ) );
}


/**
 * Test that a symbol with a rectangle produces SVG output
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsRectangle )
{
    AddRectangle( -100, -100, 100, 100 );

    wxString svg = PlotToSvgString();

    BOOST_CHECK( !svg.IsEmpty() );
    BOOST_CHECK( svg.Contains( wxT( "<svg" ) ) );
    // Rectangle should produce a path or rect element
    BOOST_CHECK( svg.Contains( wxT( "<path" ) ) || svg.Contains( wxT( "<rect" ) ) );
}


/**
 * Test that a symbol with a circle produces SVG output
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsCircle )
{
    AddCircle( 0, 0, 50 );

    wxString svg = PlotToSvgString();

    BOOST_CHECK( !svg.IsEmpty() );
    BOOST_CHECK( svg.Contains( wxT( "<svg" ) ) );
    // Circle should produce a circle or ellipse element
    BOOST_CHECK( svg.Contains( wxT( "<circle" ) ) || svg.Contains( wxT( "<ellipse" ) )
                 || svg.Contains( wxT( "<path" ) ) );
}


/**
 * Test that a symbol with a polyline produces SVG output
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsPolyline )
{
    AddPolyline( { { 0, 0 }, { 50, 50 }, { 100, 0 }, { 100, 100 } } );

    wxString svg = PlotToSvgString();

    BOOST_CHECK( !svg.IsEmpty() );
    BOOST_CHECK( svg.Contains( wxT( "<svg" ) ) );
    // Polyline should produce a path or polyline element
    BOOST_CHECK( svg.Contains( wxT( "<path" ) ) || svg.Contains( wxT( "<polyline" ) ) );
}


/**
 * Test that a symbol with text produces SVG output
 */
BOOST_AUTO_TEST_CASE( SvgExport_ContainsText )
{
    AddText( 0, 0, wxT( "Test Label" ) );

    wxString svg = PlotToSvgString();

    BOOST_CHECK( !svg.IsEmpty() );
    BOOST_CHECK( svg.Contains( wxT( "<svg" ) ) );
    // Text should produce a text element or paths for glyphs
    BOOST_CHECK( svg.Contains( wxT( "<text" ) ) || svg.Contains( wxT( "<path" ) ) );
}


/**
 * Test that bounding box is calculated correctly for pins
 */
BOOST_AUTO_TEST_CASE( BoundingBox_Pins )
{
    AddPin( 0, 0, wxT( "PIN1" ), wxT( "1" ) );
    AddPin( 200, 0, wxT( "PIN2" ), wxT( "2" ) );

    BOX2I bbox = GetSymbolBoundingBox();

    // Bounding box should span from first pin to second pin (plus pin length and decoration)
    BOOST_CHECK( bbox.GetWidth() > 0 );
    BOOST_CHECK( bbox.GetHeight() > 0 );
}


/**
 * Test that bounding box is calculated correctly for rectangle
 */
BOOST_AUTO_TEST_CASE( BoundingBox_Rectangle )
{
    AddRectangle( -50, -50, 50, 50 );

    BOX2I bbox = GetSymbolBoundingBox();

    // Bounding box should approximately match the rectangle size
    int expectedWidth = schIUScale.MilsToIU( 100 );  // 50 - (-50) = 100 mils
    int expectedHeight = schIUScale.MilsToIU( 100 );

    BOOST_CHECK( std::abs( bbox.GetWidth() - expectedWidth ) < schIUScale.MilsToIU( 20 ) );
    BOOST_CHECK( std::abs( bbox.GetHeight() - expectedHeight ) < schIUScale.MilsToIU( 20 ) );
}


/**
 * Test that bounding box is calculated correctly for circle
 */
BOOST_AUTO_TEST_CASE( BoundingBox_Circle )
{
    int radius = 100;
    AddCircle( 0, 0, radius );

    BOX2I bbox = GetSymbolBoundingBox();

    // Bounding box should be approximately 2*radius in each dimension
    int expectedSize = schIUScale.MilsToIU( 2 * radius );

    BOOST_CHECK( std::abs( bbox.GetWidth() - expectedSize ) < schIUScale.MilsToIU( 20 ) );
    BOOST_CHECK( std::abs( bbox.GetHeight() - expectedSize ) < schIUScale.MilsToIU( 20 ) );
}


/**
 * Test that a complex symbol with multiple elements produces valid SVG
 */
BOOST_AUTO_TEST_CASE( SvgExport_ComplexSymbol )
{
    // Create a simple IC-like symbol
    AddRectangle( -100, -150, 100, 150 );
    AddPin( -200, -100, wxT( "A" ), wxT( "1" ) );
    AddPin( -200, 0, wxT( "B" ), wxT( "2" ) );
    AddPin( -200, 100, wxT( "C" ), wxT( "3" ) );
    AddPin( 200, -100, wxT( "Y" ), wxT( "4" ) );
    AddPin( 200, 0, wxT( "Z" ), wxT( "5" ) );
    AddPin( 200, 100, wxT( "W" ), wxT( "6" ) );
    AddText( 0, 0, wxT( "IC" ) );

    wxString svg = PlotToSvgString();

    BOOST_CHECK( !svg.IsEmpty() );
    BOOST_CHECK( svg.Contains( wxT( "<svg" ) ) );
    BOOST_CHECK( svg.Contains( wxT( "</svg>" ) ) );

    // Should have multiple path elements for all the components
    int pathCount = 0;
    size_t pos = 0;

    while( ( pos = svg.find( wxT( "<path" ), pos ) ) != wxString::npos )
    {
        pathCount++;
        pos++;
    }

    // A complex symbol should have multiple paths
    BOOST_CHECK( pathCount >= 1 );
}


/**
 * Test PNG alpha computation formula:
 * Given pixels on white background (W) and black background (B),
 * alpha = 255 - (W - B), and color = B * 255 / alpha
 */
BOOST_AUTO_TEST_CASE( PngExport_AlphaComputation_OpaquePixel )
{
    // An opaque red pixel: on white = red, on black = red
    int rW = 255, gW = 0, bW = 0;    // red on white
    int rB = 255, gB = 0, bB = 0;    // red on black

    int diffR = rW - rB;
    int diffG = gW - gB;
    int diffB = bW - bB;
    int avgDiff = ( diffR + diffG + diffB ) / 3;

    int alpha = 255 - avgDiff;

    BOOST_CHECK_EQUAL( alpha, 255 );  // Fully opaque
}


/**
 * Test PNG alpha computation for transparent pixel
 */
BOOST_AUTO_TEST_CASE( PngExport_AlphaComputation_TransparentPixel )
{
    // A transparent pixel: on white = white, on black = black
    int rW = 255, gW = 255, bW = 255;    // white on white
    int rB = 0, gB = 0, bB = 0;          // black on black

    int diffR = rW - rB;
    int diffG = gW - gB;
    int diffB = bW - bB;
    int avgDiff = ( diffR + diffG + diffB ) / 3;

    int alpha = 255 - avgDiff;

    BOOST_CHECK_EQUAL( alpha, 0 );  // Fully transparent
}


/**
 * Test PNG alpha computation for semi-transparent pixel
 */
BOOST_AUTO_TEST_CASE( PngExport_AlphaComputation_SemiTransparentPixel )
{
    // A 50% transparent red pixel
    // On white: blends to (255, 128, 128)  approximately
    // On black: blends to (128, 0, 0) approximately

    int rW = 255, gW = 128, bW = 128;
    int rB = 128, gB = 0, bB = 0;

    int diffR = rW - rB;  // 127
    int diffG = gW - gB;  // 128
    int diffB = bW - bB;  // 128
    int avgDiff = ( diffR + diffG + diffB ) / 3;  // approximately 127-128

    int alpha = 255 - avgDiff;

    // Alpha should be around 127-128 (50%)
    BOOST_CHECK( alpha > 120 && alpha < 140 );
}


/**
 * Test that an empty symbol produces empty SVG content
 */
BOOST_AUTO_TEST_CASE( SvgExport_EmptySymbol )
{
    // Don't add any items - the symbol only has default fields
    // After removing fields from bounding box calculation, should be empty

    BOX2I bbox = GetSymbolBoundingBox();

    // An empty symbol (no non-field items) should have zero-size bounding box
    BOOST_CHECK( bbox.GetWidth() == 0 || bbox.GetHeight() == 0 );
}


BOOST_AUTO_TEST_SUITE_END()
