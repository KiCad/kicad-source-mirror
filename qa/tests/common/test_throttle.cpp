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
#include <widgets/wx_progress_reporters.h>
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


BOOST_AUTO_TEST_CASE( ProgressReporterGatingThrottlesBulkCalls )
{
    THROTTLE throttle( std::chrono::milliseconds( 100 ) );

    int refreshes = 0;
    int maxProgress = 1000;

    // Only the priming refresh should run inside the throttle window.
    for( int progress = 0; progress < maxProgress; ++progress )
    {
        if( WX_PROGRESS_REPORTER::shouldRefresh( progress, maxProgress, 0, 1, throttle ) )
            refreshes++;
    }

    BOOST_CHECK_EQUAL( refreshes, 1 );
}


BOOST_AUTO_TEST_CASE( ProgressReporterGatingAlwaysRefreshesFinalTick )
{
    THROTTLE throttle( std::chrono::milliseconds( 100 ) );

    int maxProgress = 1000;

    // Consume the priming refresh.
    BOOST_CHECK( WX_PROGRESS_REPORTER::shouldRefresh( 0, maxProgress, 0, 1, throttle ) );
    BOOST_CHECK( !WX_PROGRESS_REPORTER::shouldRefresh( 500, maxProgress, 0, 1, throttle ) );

    // The final tick must refresh regardless of throttle state.
    BOOST_CHECK( WX_PROGRESS_REPORTER::shouldRefresh( maxProgress, maxProgress, 0, 1, throttle ) );
}


BOOST_AUTO_TEST_CASE( ProgressReporterGatingThrottlesPhaseBoundaries )
{
    THROTTLE throttle( std::chrono::milliseconds( 100 ) );

    int maxProgress = 1000;

    // Consume the priming refresh.
    BOOST_CHECK( WX_PROGRESS_REPORTER::shouldRefresh( 0, maxProgress, 0, 2, throttle ) );

    // Intermediate phase completion must stay throttled.
    BOOST_CHECK( !WX_PROGRESS_REPORTER::shouldRefresh( maxProgress, maxProgress, 0, 2, throttle ) );

    // The last phase is the terminal tick.
    BOOST_CHECK( WX_PROGRESS_REPORTER::shouldRefresh( maxProgress, maxProgress, 1, 2, throttle ) );
}


BOOST_AUTO_TEST_SUITE_END()
