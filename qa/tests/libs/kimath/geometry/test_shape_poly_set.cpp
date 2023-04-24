/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <geometry/shape_poly_set.h>
#include <trigo.h>

#include <qa_utils/geometry/geometry.h>
#include <qa_utils/numeric.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include "geom_test_utils.h"


BOOST_AUTO_TEST_SUITE( ShapePolySet )

BOOST_AUTO_TEST_CASE( RemoveNullSegments )
{
    SHAPE_POLY_SET base_set;

    base_set.NewOutline();
    base_set.Append( 0, 0, -1, -1, true );
    base_set.Append( 0, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 0, -1, -1, true );

    int removed = base_set.RemoveNullSegments();

    BOOST_CHECK_EQUAL( removed, 3 );
    BOOST_CHECK_EQUAL( base_set.VertexCount(), 4 );

    base_set.DeletePolygon( 0 );

    base_set.NewOutline();
    base_set.Append( 0, 0, -1, -1, true );
    base_set.Append( 0, 10, -1, -1, true );
    base_set.Append( 0, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 10, -1, -1, true );
    base_set.Append( 10, 0, -1, -1, true );
    base_set.Append( 10, 0, -1, -1, true );
    base_set.Append( 0, 0, -1, -1, true );

    removed = base_set.RemoveNullSegments();

    BOOST_CHECK_EQUAL( removed, 4 );
    BOOST_CHECK_EQUAL( base_set.VertexCount(), 4 );

}

BOOST_AUTO_TEST_SUITE_END()
