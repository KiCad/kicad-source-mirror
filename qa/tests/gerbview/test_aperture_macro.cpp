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

/**
 * Regression tests for aperture macro shape generation.
 */

#include <boost/test/unit_test.hpp>

#include <aperture_macro.h>
#include <dcode.h>
#include <gerber_draw_item.h>
#include <gerber_file_image.h>


BOOST_AUTO_TEST_SUITE( GerbviewApertureMacro )


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
