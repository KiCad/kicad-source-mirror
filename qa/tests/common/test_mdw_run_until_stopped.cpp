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

#include <mdw/client/mdw_op.h>

#include <atomic>
#include <memory>
#include <thread>


BOOST_AUTO_TEST_SUITE( MdwStoppableRun )


namespace
{

using namespace std::chrono_literals;

/// A task that returns only once aStop is set, as a backend call does when its transport closes
struct STUCK
{
    std::shared_ptr<std::atomic<bool>> stop = std::make_shared<std::atomic<bool>>( false );

    std::function<MDW_RESULT()> Task() const
    {
        return [flag = stop]()
        {
            while( !flag->load() )
                std::this_thread::sleep_for( 5ms );

            return MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );
        };
    }
};

} // namespace


BOOST_AUTO_TEST_CASE( StuckTaskTimesOutAndIsStoppedOnce )
{
    STUCK stuck;
    int   stops = 0;

    MDW_RESULT result = MdwRunUntilStopped(
            stuck.Task(), 100ms, 1s,
            []()
            {
                return false;
            },
            [&]()
            {
                ++stops;
                stuck.stop->store( true );
            } );

    BOOST_CHECK( result.error == MDW_ERROR_CODE::TIMEOUT );
    BOOST_CHECK_EQUAL( stops, 1 );
}


BOOST_AUTO_TEST_CASE( RaisedCancelFlagCancelsAndStopsOnce )
{
    STUCK             stuck;
    std::atomic<bool> cancel( false );
    int               stops = 0;

    std::thread raise(
            [&]()
            {
                std::this_thread::sleep_for( 100ms );
                cancel.store( true );
            } );

    MDW_RESULT result = MdwRunUntilStopped(
            stuck.Task(), 30s, 1s,
            [&]()
            {
                return cancel.load();
            },
            [&]()
            {
                ++stops;
                stuck.stop->store( true );
            } );

    raise.join();

    BOOST_CHECK( result.error == MDW_ERROR_CODE::CANCELLED );
    BOOST_CHECK_EQUAL( stops, 1 );
}


// A stop that fails to end the task still releases the caller within the grace
BOOST_AUTO_TEST_CASE( IgnoredStopStillReturnsWithinGrace )
{
    STUCK stuck;
    auto  started = std::chrono::steady_clock::now();

    MDW_RESULT result = MdwRunUntilStopped(
            stuck.Task(), 100ms, 200ms,
            []()
            {
                return false;
            },
            []()
            {
            } );

    BOOST_CHECK( result.error == MDW_ERROR_CODE::TIMEOUT );
    BOOST_CHECK_LT( std::chrono::steady_clock::now() - started, 2s );

    // Lets the detached task end
    stuck.stop->store( true );
}


BOOST_AUTO_TEST_CASE( FinishedTaskAnswersItsOwnResultWithoutStopping )
{
    int stops = 0;

    MDW_RESULT result = MdwRunUntilStopped(
            []()
            {
                return MDW_RESULT::Ok();
            },
            5s, 1s,
            []()
            {
                return false;
            },
            [&]()
            {
                ++stops;
            } );

    BOOST_CHECK( result.IsOk() );
    BOOST_CHECK_EQUAL( stops, 0 );
}


BOOST_AUTO_TEST_SUITE_END()
