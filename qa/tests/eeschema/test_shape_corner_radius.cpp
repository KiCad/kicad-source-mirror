/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <boost/test/unit_test.hpp>

#include <sch_shape.h>

BOOST_AUTO_TEST_CASE( SCHShapeCornerRadius )
{
    SCH_SHAPE shape( SHAPE_T::RECTANGLE, LAYER_NOTES );
    shape.SetPosition( VECTOR2I( 0, 0 ) );
    shape.SetEnd( VECTOR2I( 100, 100 ) );
    shape.SetCornerRadius( 20 );
    BOOST_CHECK_EQUAL( shape.GetCornerRadius(), 20 );
}
