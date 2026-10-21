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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <3d_rendering/raytracing/shapes3D/bbox_3d.h>


BOOST_AUTO_TEST_SUITE( BBox3D )


BOOST_AUTO_TEST_CASE( MaxDimensionUsesLongestAxis )
{
    const BBOX_3D bbox( SFVEC3F( 0.0f, 0.0f, 0.0f ), SFVEC3F( 10.0f, 2.0f, 5.0f ) );

    BOOST_CHECK_EQUAL( bbox.MaxDimension(), 0u );
}


BOOST_AUTO_TEST_SUITE_END()
