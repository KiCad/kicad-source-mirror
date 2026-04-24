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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <import_gfx/svg_import_plugin.h>
#include <import_gfx/graphics_importer_buffer.h>
#include <eda_item.h>
#include <base_units.h>


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


BOOST_AUTO_TEST_SUITE_END()
