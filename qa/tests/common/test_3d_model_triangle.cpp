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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <plugins/3dapi/c3dmodel.h>


BOOST_AUTO_TEST_SUITE( C3dModelTriangle )


BOOST_AUTO_TEST_CASE( InRangeTriangleAccepted )
{
    const unsigned int faces[] = { 0, 1, 2 };

    BOOST_CHECK( IsTriangleInRange( faces, 0, 3 ) );
}


BOOST_AUTO_TEST_CASE( OutOfRangeIndexRejected )
{
    const unsigned int faces[] = { 0, 1, 5 };

    BOOST_CHECK( !IsTriangleInRange( faces, 0, 3 ) );
}


BOOST_AUTO_TEST_CASE( BoundaryIndexEqualsCountRejected )
{
    const unsigned int faces[] = { 0, 3, 1 };

    BOOST_CHECK( !IsTriangleInRange( faces, 0, 3 ) );
}


BOOST_AUTO_TEST_CASE( OffsetTriangleValidated )
{
    const unsigned int faces[] = { 0, 1, 2, 2, 9, 0 };

    BOOST_CHECK( IsTriangleInRange( faces, 0, 3 ) );
    BOOST_CHECK( !IsTriangleInRange( faces, 3, 3 ) );
}


BOOST_AUTO_TEST_SUITE_END()
