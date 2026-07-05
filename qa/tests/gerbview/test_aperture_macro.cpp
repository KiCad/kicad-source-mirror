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

/**
 * Regression tests for aperture macro shape generation.
 */

#include <boost/test/unit_test.hpp>

#include <aperture_macro.h>
#include <am_primitive.h>
#include <am_param.h>
#include <dcode.h>
#include <gerber_draw_item.h>
#include <gerber_file_image.h>
#include <geometry/shape_poly_set.h>


BOOST_AUTO_TEST_SUITE( GerbviewApertureMacro )


namespace
{

/**
 * Build a single AMP_CIRCLE primitive of the given diameter (mm) and return the segment count
 * of its tessellated outline. Drives the real conversion path used when flashing an
 * aperture-macro circle.
 */
int circlePrimitiveSegmentCount( double aDiameterMm )
{
    APERTURE_MACRO macro;
    AM_PRIMITIVE   prim( true, AMP_CIRCLE );

    auto pushValue =
            []( AM_PRIMITIVE& aPrim, double aValue )
            {
                AM_PARAM param;
                param.PushOperator( PUSHVALUE, aValue );
                aPrim.m_Params.push_back( param );
            };

    // exposure ON, diameter, center x, center y
    pushValue( prim, 1.0 );
    pushValue( prim, aDiameterMm );
    pushValue( prim, 0.0 );
    pushValue( prim, 0.0 );

    SHAPE_POLY_SET buffer;
    prim.ConvertBasicShapeToPolygon( &macro, buffer );

    BOOST_REQUIRE_GT( buffer.OutlineCount(), 0 );

    // The conversion closes the outline by repeating the first point.
    return buffer.Outline( 0 ).PointCount() - 1;
}

} // namespace


// Guards against regressing to a fixed segment count independent of the circle size.
BOOST_AUTO_TEST_CASE( CirclePrimitiveTessellationScalesWithSize )
{
    int smallCount = circlePrimitiveSegmentCount( 0.2 );
    int largeCount = circlePrimitiveSegmentCount( 20.0 );

    // A larger circle needs more segments to hold the same maximum deviation.
    BOOST_CHECK_GT( largeCount, smallCount );

    // Both counts follow the TransformCircleToPolygon convention of a multiple of 8 segments
    // with an 8-segment floor.
    BOOST_CHECK_GE( smallCount, 8 );
    BOOST_CHECK_EQUAL( smallCount % 8, 0 );
    BOOST_CHECK_EQUAL( largeCount % 8, 0 );
}


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23816
 *
 * When an aperture macro has invalid or missing parameters (as reported during
 * Gerber file load), GetApertureMacroShape() returns an empty SHAPE_POLY_SET.
 * GBR_TO_PCB_EXPORTER previously crashed by calling Outline(0) on the empty
 * set without checking OutlineCount() first.
 *
 * Verifies that GetApertureMacroShape() and ConvertShapeToPolygon() handle
 * macros that produce no geometry without crashing.
 */
BOOST_AUTO_TEST_CASE( EmptyMacroProducesNoOutlines )
{
    GERBER_FILE_IMAGE image( 0 );

    D_CODE* dcode = image.GetDCODEOrCreate( 10, true );
    BOOST_REQUIRE( dcode != nullptr );
    dcode->m_ApertType = APT_MACRO;

    // A macro with no primitives simulates a macro with invalid template parameters:
    // it produces no shapes, so GetApertureMacroShape returns an empty SHAPE_POLY_SET.
    APERTURE_MACRO emptyMacro;
    emptyMacro.m_AmName = wxT( "EMPTY_TEST_MACRO" );
    dcode->SetMacro( &emptyMacro );

    GERBER_DRAW_ITEM item( &image );
    item.m_ShapeType = GBR_SPOT_MACRO;
    item.m_DCode = 10;

    SHAPE_POLY_SET* shape = emptyMacro.GetApertureMacroShape( &item, VECTOR2I( 0, 0 ) );

    BOOST_REQUIRE( shape != nullptr );
    BOOST_CHECK_EQUAL( shape->OutlineCount(), 0 );

    // ConvertShapeToPolygon must also leave m_Polygon with 0 outlines for an empty macro.
    dcode->ConvertShapeToPolygon( &item );
    BOOST_CHECK_EQUAL( dcode->m_Polygon.OutlineCount(), 0 );
}


BOOST_AUTO_TEST_SUITE_END()
