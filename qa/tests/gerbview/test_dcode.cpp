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
 * Tests for aperture-to-polygon tessellation in D_CODE.
 */

#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include <dcode.h>
#include <gerber_draw_item.h>
#include <gerber_file_image.h>
#include <base_units.h>
#include <geometry/shape_poly_set.h>


namespace
{

SHAPE_POLY_SET convertOvalAperture( double aWidthMm, double aHeightMm )
{
    GERBER_FILE_IMAGE image( 0 );
    GERBER_DRAW_ITEM  item( &image );
    D_CODE            dcode( 10 );

    dcode.m_ApertType = APT_OVAL;
    dcode.m_Size = VECTOR2I( gerbIUScale.mmToIU( aWidthMm ), gerbIUScale.mmToIU( aHeightMm ) );
    dcode.m_DrillShape = APT_DEF_NO_HOLE;
    dcode.ConvertShapeToPolygon( &item );

    return dcode.m_Polygon;
}

} // namespace


BOOST_AUTO_TEST_SUITE( GerbviewDcode )


// Guards against regressing to a segment count independent of the aperture size.
BOOST_AUTO_TEST_CASE( OvalTessellationScalesWithSize )
{
    int smallCount = convertOvalAperture( 2.0, 1.0 ).VertexCount();
    int largeCount = convertOvalAperture( 40.0, 20.0 ).VertexCount();

    BOOST_CHECK_GT( largeCount, smallCount );
}


// Exercises the true oval path (unequal axes) in both orientations.
BOOST_AUTO_TEST_CASE( OvalTessellationKeepsRequestedSize )
{
    const int tolerance = 2 * gerbIUScale.mmToIU( 0.005 );

    BOX2I horizontal = convertOvalAperture( 2.0, 1.0 ).BBox();

    BOOST_CHECK_LE( std::abs( horizontal.GetWidth() - gerbIUScale.mmToIU( 2.0 ) ), tolerance );
    BOOST_CHECK_LE( std::abs( horizontal.GetHeight() - gerbIUScale.mmToIU( 1.0 ) ), tolerance );

    BOX2I vertical = convertOvalAperture( 1.0, 2.0 ).BBox();

    BOOST_CHECK_LE( std::abs( vertical.GetWidth() - gerbIUScale.mmToIU( 1.0 ) ), tolerance );
    BOOST_CHECK_LE( std::abs( vertical.GetHeight() - gerbIUScale.mmToIU( 2.0 ) ), tolerance );
}


BOOST_AUTO_TEST_SUITE_END()
