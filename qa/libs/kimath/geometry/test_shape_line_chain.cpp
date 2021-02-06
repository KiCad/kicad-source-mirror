/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>

#include <qa_utils/geometry/geometry.h>
#include <qa_utils/numeric.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include "geom_test_utils.h"

BOOST_AUTO_TEST_SUITE( ShapeLineChain )

BOOST_AUTO_TEST_CASE( ArcToPolyline )
{
    SHAPE_LINE_CHAIN base_chain( { VECTOR2I( 0, 0 ), VECTOR2I( 0, 1000 ), VECTOR2I( 1000, 0 ) } );

    SHAPE_LINE_CHAIN chain_insert( {
            VECTOR2I( 0, 1500 ),
            VECTOR2I( 1500, 1500 ),
            VECTOR2I( 1500, 0 ),
    } );

    SHAPE_LINE_CHAIN arc_insert1( SHAPE_ARC( VECTOR2I( 0, -100 ), VECTOR2I( 0, -200 ), 1800 ) );

    SHAPE_LINE_CHAIN arc_insert2( SHAPE_ARC( VECTOR2I( 0, 500 ), VECTOR2I( 0, 400 ), 1800 ) );

    BOOST_CHECK_EQUAL( base_chain.CShapes().size(), base_chain.CPoints().size() );
    BOOST_CHECK_EQUAL( arc_insert1.CShapes().size(), arc_insert1.CPoints().size() );
    BOOST_CHECK_EQUAL( arc_insert2.CShapes().size(), arc_insert2.CPoints().size() );

    base_chain.Insert( 0, SHAPE_ARC( VECTOR2I( 0, -100 ), VECTOR2I( 0, -200 ), 1800 ) );

    BOOST_CHECK_EQUAL( base_chain.CShapes().size(), base_chain.CPoints().size() );

    base_chain.Replace( 0, 2, chain_insert );
    BOOST_CHECK_EQUAL( base_chain.CShapes().size(), base_chain.CPoints().size() );
}


BOOST_AUTO_TEST_SUITE_END()
