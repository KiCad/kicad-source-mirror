/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <import_gfx/svg_import_plugin.h>
#include <import_gfx/graphics_importer_buffer.h>
#include <eda_item.h>
#include <base_units.h>
#include <nanosvg.h>
#include <nanosvgrast.h>
#include <memory>
#include <fontconfig/fontconfig.h>
#include <paths.h>


class SVG_IMPORT_TEST_IMPORTER : public GRAPHICS_IMPORTER
{
public:
    SVG_IMPORT_TEST_IMPORTER() : GRAPHICS_IMPORTER()
    {
        m_millimeterToIu = PCB_IU_PER_MM;
    }

    void AddLine( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                  const IMPORTED_STROKE& aStroke ) override
    {
        m_lines.push_back( { aStart, aEnd } );
    }

    void AddCircle( const VECTOR2D& aCenter, double aRadius, const IMPORTED_STROKE& aStroke,
                    bool aFilled, const COLOR4D& aFillColor ) override
    {
    }

    void AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart, const EDA_ANGLE& aAngle,
                 const IMPORTED_STROKE& aStroke ) override
    {
    }

    void AddEllipse( const VECTOR2D& aCenter, double aMajorRadius, double aMinorRadius, const EDA_ANGLE& aRotation,
                     const IMPORTED_STROKE& aStroke, bool aFilled, const COLOR4D& aFillColor ) override
    {
    }

    void AddEllipseArc( const VECTOR2D& aCenter, double aMajorRadius, double aMinorRadius, const EDA_ANGLE& aRotation,
                        const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aEndAngle,
                        const IMPORTED_STROKE& aStroke ) override
    {
    }

    void AddPolygon( const std::vector<VECTOR2D>& aVertices, const IMPORTED_STROKE& aStroke,
                     bool aFilled, const COLOR4D& aFillColor ) override
    {
        m_polygons.push_back( aVertices );
    }

    void AddText( const VECTOR2D& aOrigin, const wxString& aText, double aHeight, double aWidth,
                  double aThickness, double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                  GR_TEXT_V_ALIGN_T aVJustify, const COLOR4D& aColor ) override
    {
    }

    void AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                    const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd,
                    const IMPORTED_STROKE& aStroke ) override
    {
        m_splines.push_back( { aStart, aBezierControl1, aBezierControl2, aEnd } );
    }

    std::vector<std::pair<VECTOR2D, VECTOR2D>>                        m_lines;
    std::vector<std::vector<VECTOR2D>>                                m_polygons;
    std::vector<std::tuple<VECTOR2D, VECTOR2D, VECTOR2D, VECTOR2D>>   m_splines;
};


BOOST_AUTO_TEST_SUITE( SvgImport )


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/11445
 *
 * A closed SVG path with a single anchor node (one cubic bezier from a point back to itself)
 * was not imported because the adaptive bezier subdivision failed when start == end, producing
 * only degenerate zero-area polygons.
 */
BOOST_AUTO_TEST_CASE( SingleNodeClosedPath )
{
    const char* svg =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\""
        " width=\"100mm\" height=\"100mm\">"
        "<path d=\"M 50,80 C 80,20 20,20 50,80 Z\" fill=\"black\" stroke=\"none\" />"
        "</svg>";

    wxMemoryBuffer buf;
    buf.AppendData( svg, strlen( svg ) + 1 );

    SVG_IMPORT_PLUGIN plugin;
    SVG_IMPORT_TEST_IMPORTER importer;
    plugin.SetImporter( &importer );

    BOOST_REQUIRE( plugin.LoadFromMemory( buf ) );
    BOOST_REQUIRE( plugin.Import() );

    // The single-node closed path should produce at least one polygon with non-degenerate area
    BOOST_REQUIRE_MESSAGE( !importer.m_polygons.empty(),
                           "Single-node closed SVG path should produce a polygon" );

    const auto& poly = importer.m_polygons[0];

    BOOST_CHECK_MESSAGE( poly.size() > 3,
                         "Polygon should have more than 3 vertices (got " << poly.size() << ")" );

    // Verify the polygon has non-zero area by checking that not all vertices are identical
    bool hasDistinctVertices = false;

    for( size_t i = 1; i < poly.size(); ++i )
    {
        if( poly[i] != poly[0] )
        {
            hasDistinctVertices = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( hasDistinctVertices,
                         "Polygon vertices should not all be at the same point" );

    // Compute approximate area using the shoelace formula to verify non-degenerate polygon
    double area = 0.0;

    for( size_t i = 0; i < poly.size(); ++i )
    {
        size_t j = ( i + 1 ) % poly.size();
        area += poly[i].x * poly[j].y;
        area -= poly[j].x * poly[i].y;
    }

    area = std::fabs( area ) / 2.0;

    BOOST_CHECK_MESSAGE( area > 0.01,
                         "Polygon should have non-zero area (got " << area << ")" );
}


/**
 * Verify that a normal multi-node closed path still imports correctly after the single-node fix.
 */
BOOST_AUTO_TEST_CASE( MultiNodeClosedPath )
{
    const char* svg =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\""
        " width=\"100mm\" height=\"100mm\">"
        "<path d=\"M 10,10 C 40,0 60,0 90,10 C 100,40 100,60 90,90"
        " C 60,100 40,100 10,90 C 0,60 0,40 10,10 Z\" fill=\"black\" stroke=\"none\" />"
        "</svg>";

    wxMemoryBuffer buf;
    buf.AppendData( svg, strlen( svg ) + 1 );

    SVG_IMPORT_PLUGIN plugin;
    SVG_IMPORT_TEST_IMPORTER importer;
    plugin.SetImporter( &importer );

    BOOST_REQUIRE( plugin.LoadFromMemory( buf ) );
    BOOST_REQUIRE( plugin.Import() );

    BOOST_REQUIRE_MESSAGE( !importer.m_polygons.empty(),
                           "Multi-node closed SVG path should produce a polygon" );

    BOOST_CHECK_MESSAGE( importer.m_polygons[0].size() > 4,
                         "Multi-node polygon should have many vertices" );
}


namespace
{
using SVG_IMAGE = std::unique_ptr<NSVGimage, decltype( &nsvgDelete )>;

SVG_IMAGE parseSvgText( const std::string& aBody )
{
    using FONT_CONFIG = std::unique_ptr<FcConfig, decltype( &FcConfigDestroy )>;
    static FONT_CONFIG fonts = []
    {
        FONT_CONFIG config( FcConfigCreate(), FcConfigDestroy );
        BOOST_REQUIRE( config );

        for( const char* file : { "NimbusSans-Regular.t1", "NimbusSans-Bold.t1", "NimbusSans-Italic.t1" } )
        {
            wxCharBuffer path = ( PATHS::GetStockDataPath() + wxS( "/libwmf/fonts/" )
                                  + wxString::FromUTF8( file ) ).utf8_str();
            BOOST_REQUIRE( FcConfigAppFontAddFile( config.get(), reinterpret_cast<const FcChar8*>( path.data() ) ) );
        }

        return config;
    }();

    std::string svg = "<svg width=\"300\" height=\"150\" xmlns=\"http://www.w3.org/2000/svg\">"
                      + aBody + "</svg>";
    return SVG_IMAGE( nsvgParseWithFontConfig( svg.data(), "px", 96, fonts.get() ), nsvgDelete );
}

std::vector<NSVGshape*> svgShapes( const SVG_IMAGE& aImage )
{
    std::vector<NSVGshape*> result;

    for( NSVGshape* shape = aImage->shapes; shape; shape = shape->next )
        result.push_back( shape );

    return result;
}
}


BOOST_AUTO_TEST_CASE( TextOutlinesRasterize )
{
    auto image = parseSvgText( "<text x=\"10\" y=\"60\" font-family=\"sans-serif\" font-size=\"40\">"
                               "<![CDATA[UMC]]></text>" );
    BOOST_REQUIRE( image );
    BOOST_REQUIRE_EQUAL( svgShapes( image ).size(), 3 );
    std::unique_ptr<NSVGrasterizer, decltype( &nsvgDeleteRasterizer )> rasterizer(
            nsvgCreateRasterizer(), nsvgDeleteRasterizer );
    BOOST_REQUIRE( rasterizer );
    std::vector<unsigned char> pixels( 300 * 150 * 4 );
    nsvgRasterize( rasterizer.get(), image.get(), 0, 0, 1, pixels.data(), 300, 150, 300 * 4 );
    size_t ink = 0;

    for( size_t i = 3; i < pixels.size(); i += 4 )
        ink += pixels[i] != 0;

    BOOST_CHECK_GT( ink, 200 );
}


BOOST_AUTO_TEST_CASE( TextEntitiesAndCdata )
{
    auto encoded = parseSvgText( "<text y=\"50\">&lt;&amp;&#x3a9;&#233;</text>" );
    auto literal = parseSvgText( "<text y=\"50\"><![CDATA[<&Ωé]]></text>" );
    auto unexpanded = parseSvgText( "<text y=\"50\"><![CDATA[&lt;]]></text>" );
    BOOST_REQUIRE( encoded );
    BOOST_REQUIRE( literal );
    BOOST_REQUIRE( unexpanded );
    auto a = svgShapes( encoded );
    auto b = svgShapes( literal );
    BOOST_REQUIRE_EQUAL( a.size(), 4 );
    BOOST_REQUIRE_EQUAL( b.size(), a.size() );
    BOOST_CHECK_EQUAL( svgShapes( unexpanded ).size(), 4 );

    for( size_t i = 0; i < a.size(); ++i )
    {
        for( int axis = 0; axis < 4; ++axis )
            BOOST_CHECK_SMALL( a[i]->bounds[axis] - b[i]->bounds[axis], 0.001f );
    }
}


BOOST_AUTO_TEST_CASE( TextPositionListsAndSpans )
{
    auto image = parseSvgText( "<text x=\"10 50 100\" y=\"40 60 80\" font-size=\"20\">"
                              "H<tspan dx=\"3\" dy=\"4\">H</tspan>H</text>" );
    BOOST_REQUIRE( image );
    auto shapes = svgShapes( image );
    BOOST_REQUIRE_EQUAL( shapes.size(), 3 );
    BOOST_CHECK_SMALL( shapes[1]->bounds[0] - shapes[0]->bounds[0] - 43, 0.001f );
    BOOST_CHECK_SMALL( shapes[1]->bounds[1] - shapes[0]->bounds[1] - 24, 0.001f );
    BOOST_CHECK_SMALL( shapes[2]->bounds[0] - shapes[0]->bounds[0] - 90, 0.001f );
    BOOST_CHECK_SMALL( shapes[2]->bounds[1] - shapes[0]->bounds[1] - 40, 0.001f );
}


BOOST_AUTO_TEST_CASE( TextAnchorCoversSpans )
{
    auto start = parseSvgText( "<text x=\"150\" y=\"60\" font-size=\"30\">H<tspan>H</tspan>H</text>" );
    auto middle = parseSvgText( "<text x=\"150\" y=\"60\" font-size=\"30\" text-anchor=\"middle\">"
                                "H<tspan>H</tspan>H</text>" );
    auto end = parseSvgText( "<text x=\"150\" y=\"60\" font-size=\"30\" text-anchor=\"end\">"
                             "H<tspan>H</tspan>H</text>" );
    BOOST_REQUIRE( start );
    BOOST_REQUIRE( middle );
    BOOST_REQUIRE( end );
    auto a = svgShapes( start );
    auto b = svgShapes( middle );
    auto c = svgShapes( end );
    BOOST_REQUIRE_EQUAL( a.size(), 3 );
    BOOST_REQUIRE_EQUAL( b.size(), 3 );
    BOOST_REQUIRE_EQUAL( c.size(), 3 );
    float shift = a[0]->bounds[0] - b[0]->bounds[0];
    BOOST_CHECK_GT( shift, 20 );

    for( size_t i = 0; i < a.size(); ++i )
    {
        BOOST_CHECK_SMALL( a[i]->bounds[0] - b[i]->bounds[0] - shift, 0.001f );
        BOOST_CHECK_SMALL( a[i]->bounds[0] - c[i]->bounds[0] - 2 * shift, 0.001f );
    }
}


BOOST_AUTO_TEST_CASE( TextTransformsAndInheritedStyle )
{
    auto image = parseSvgText( "<g font-family=\"sans-serif\" font-size=\"20\" fill=\"#123456\">"
                              "<text x=\"10\" y=\"40\">H</text>"
                              "<text x=\"10\" y=\"40\" transform=\"translate(100,0) rotate(90)\">H</text>"
                              "<text x=\"10\" y=\"40\" font-size=\"40\">H</text></g>" );
    BOOST_REQUIRE( image );
    auto shapes = svgShapes( image );
    BOOST_REQUIRE_EQUAL( shapes.size(), 3 );
    BOOST_CHECK_EQUAL( shapes[0]->fill.color, 0xff563412u );
    BOOST_CHECK_SMALL( shapes[1]->bounds[0] - ( 100 - shapes[0]->bounds[3] ), 0.001f );
    BOOST_CHECK_SMALL( shapes[1]->bounds[1] - shapes[0]->bounds[0], 0.001f );
    float height = shapes[0]->bounds[3] - shapes[0]->bounds[1];
    BOOST_CHECK_SMALL( shapes[2]->bounds[3] - shapes[2]->bounds[1] - 2 * height, 0.05f );
}


BOOST_AUTO_TEST_CASE( UpdatedParserPreservesLocalDefaults )
{
    auto image = parseSvgText( "<style>.s { fill: #ff0000; }</style>"
                              "<path style=\"fill:#00ff00\" class=\"s\" d=\"M0 0 L10 0 L10 10 Z\"/>" );
    BOOST_REQUIRE( image );
    BOOST_REQUIRE( image->shapes );
    BOOST_CHECK_EQUAL( image->shapes->fill.color, 0xff00ff00u );
    BOOST_CHECK_EQUAL( image->shapes->strokeWidth, 0 );
    std::string svg = "<svg width=\"200\" viewBox=\"0 0 100 100\"><rect width=\"100\" height=\"100\"/></svg>";
    SVG_IMAGE inferred( nsvgParse( svg.data(), "px", 96 ), nsvgDelete );
    BOOST_REQUIRE( inferred );
    BOOST_CHECK_EQUAL( inferred->width, 200 );
    BOOST_CHECK_EQUAL( inferred->height, 100 );
    BOOST_REQUIRE( inferred->shapes );
    BOOST_CHECK_EQUAL( inferred->shapes->bounds[0], 50 );
}

BOOST_AUTO_TEST_CASE( TextWhitespaceAndFontVariants )
{
    auto regular = parseSvgText( "<text y=\"50\" font-family=\"sans-serif\" font-size=\"30\"> H  H </text>" );
    auto spans = parseSvgText( "<text y=\"50\" font-family=\"sans-serif\" font-size=\"30\">"
                               "H <tspan> H</tspan></text>" );
    auto bold = parseSvgText( "<text y=\"50\" font-family=\"sans-serif\" font-size=\"30\" font-weight=\"700\">H</text>" );
    auto italic = parseSvgText( "<text y=\"50\" font-family=\"sans-serif\" font-size=\"30\" font-style=\"italic\">H</text>" );
    BOOST_REQUIRE( regular );
    BOOST_REQUIRE( spans );
    BOOST_REQUIRE( bold );
    BOOST_REQUIRE( italic );
    auto a = svgShapes( regular );
    auto b = svgShapes( spans );
    BOOST_REQUIRE_EQUAL( a.size(), 2 );
    BOOST_REQUIRE_EQUAL( b.size(), 2 );
    BOOST_REQUIRE( bold->shapes );
    BOOST_REQUIRE( italic->shapes );
    BOOST_CHECK_SMALL( a[1]->bounds[0] - b[1]->bounds[0], 0.001f );
    float width = a[0]->bounds[2] - a[0]->bounds[0];
    BOOST_CHECK_GT( bold->shapes->bounds[2] - bold->shapes->bounds[0], width );
    BOOST_CHECK_GT( italic->shapes->bounds[2] - italic->shapes->bounds[0], width );
}


BOOST_AUTO_TEST_CASE( DefinitionsDoNotConsumeTextState )
{
    auto reference = parseSvgText( "<text x=\"150\" y=\"60\" text-anchor=\"middle\" fill=\"#123456\">HH</text>"
                                   "<text x=\"20\" y=\"100\">H</text>" );
    auto definitions = parseSvgText( "<text x=\"150\" y=\"60\" text-anchor=\"middle\" fill=\"#123456\">H"
                                     "<defs><g><tspan>ignored</tspan></g><defs><tspan>nested</tspan></defs>"
                                     "<tspan>also ignored</tspan></defs>H</text><text x=\"20\" y=\"100\">H</text>" );
    BOOST_REQUIRE( reference );
    BOOST_REQUIRE( definitions );
    auto a = svgShapes( reference );
    auto b = svgShapes( definitions );
    BOOST_REQUIRE_EQUAL( a.size(), 3 );
    BOOST_REQUIRE_EQUAL( b.size(), a.size() );

    for( size_t i = 0; i < a.size(); ++i )
    {
        BOOST_CHECK_EQUAL( a[i]->fill.color, b[i]->fill.color );

        for( int axis = 0; axis < 4; ++axis )
            BOOST_CHECK_SMALL( a[i]->bounds[axis] - b[i]->bounds[axis], 0.001f );
    }
}


BOOST_AUTO_TEST_CASE( InvalidNumericEntitiesStayLiteral )
{
    auto encoded = parseSvgText( "<text y=\"50\">&#zz;&#xzz;&#12z;</text>" );
    auto literal = parseSvgText( "<text y=\"50\"><![CDATA[&#zz;&#xzz;&#12z;]]></text>" );
    BOOST_REQUIRE( encoded );
    BOOST_REQUIRE( literal );
    auto a = svgShapes( encoded );
    auto b = svgShapes( literal );
    BOOST_REQUIRE_EQUAL( a.size(), 17 );
    BOOST_REQUIRE_EQUAL( b.size(), a.size() );

    for( size_t i = 0; i < a.size(); ++i )
    {
        for( int axis = 0; axis < 4; ++axis )
            BOOST_CHECK_SMALL( a[i]->bounds[axis] - b[i]->bounds[axis], 0.001f );
    }
}


BOOST_AUTO_TEST_SUITE_END()
