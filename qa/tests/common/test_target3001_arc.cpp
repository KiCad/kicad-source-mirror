/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <io/target3001/target3001_parser.h>


static double sweep( float aStart, float aEnd )
{
    TARGET3001::ARC arc;
    arc.startAngle = aStart;
    arc.endAngle = aEnd;
    return arc.Sweep();
}


BOOST_AUTO_TEST_SUITE( Target3001Arc )


BOOST_AUTO_TEST_CASE( SweepStaysWithinOneTurn )
{
    BOOST_CHECK_CLOSE( sweep( 10.0f, 400.0f ), 30.0, 1e-9 );

    double huge = sweep( 0.0f, 1e30f );

    BOOST_CHECK( huge > 0.0 && huge <= 360.0 );
}


BOOST_AUTO_TEST_SUITE_END()
