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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <core/throttle.h>
#include <thread>


BOOST_AUTO_TEST_SUITE( Throttle )


BOOST_AUTO_TEST_CASE( FirstCallAlwaysReady )
{
    THROTTLE t( std::chrono::milliseconds( 500 ) );
    BOOST_CHECK( t.Ready() );
}


BOOST_AUTO_TEST_CASE( SecondCallRejectsWithinInterval )
{
    THROTTLE t( std::chrono::milliseconds( 200 ) );
    BOOST_CHECK( t.Ready() );
    BOOST_CHECK( !t.Ready() );
}


BOOST_AUTO_TEST_CASE( ReadyAgainAfterInterval )
{
    THROTTLE t( std::chrono::milliseconds( 50 ) );
    BOOST_CHECK( t.Ready() );

    std::this_thread::sleep_for( std::chrono::milliseconds( 60 ) );

    BOOST_CHECK( t.Ready() );
}


BOOST_AUTO_TEST_SUITE_END()
