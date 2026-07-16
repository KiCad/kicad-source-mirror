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

#include <boost/test/unit_test.hpp>

#include <mdw_runtime/mdw_backend_server.h>
#include <mdw_runtime/mdw_poller.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>


namespace
{

/// Counts passes, each answering outcome, and lets a test wait for a number of them
struct PASSES
{
    MDW_POLL Count()
    {
        std::lock_guard<std::mutex> lock( mutex );
        ++count;
        passed.notify_all();
        return outcome;
    }

    bool WaitFor( int aCount )
    {
        std::unique_lock<std::mutex> lock( mutex );

        return passed.wait_for( lock, std::chrono::seconds( 10 ),
                                [&]()
                                {
                                    return count >= aCount;
                                } );
    }

    int Seen()
    {
        std::lock_guard<std::mutex> lock( mutex );
        return count;
    }

    std::mutex              mutex;
    std::condition_variable passed;
    int                     count = 0;
    MDW_POLL                outcome = MDW_POLL::OK;
};

} // namespace


BOOST_AUTO_TEST_SUITE( MdwPoller )


// A reconnect looks at once rather than waiting out the interval
BOOST_AUTO_TEST_CASE( ResumeRunsAPassAtOnce )
{
    PASSES     passes;
    MDW_POLLER poller( std::chrono::seconds( 30 ),
                       [&]()
                       {
                           return passes.Count();
                       } );

    poller.Resume();
    BOOST_CHECK( passes.WaitFor( 1 ) );
}


// A failing pass must not end the backend, and passes keep their interval
BOOST_AUTO_TEST_CASE( PassesRepeatPastAThrow )
{
    PASSES     passes;
    MDW_POLLER poller( std::chrono::milliseconds( 10 ),
                       [&]()
                       {
                           passes.Count();

                           if( passes.Seen() == 1 )
                               throw std::runtime_error( "malformed reply" );

                           return MDW_POLL::OK;
                       } );

    BOOST_CHECK( passes.WaitFor( 3 ) );
}


// Sending refused credentials again and again locks the account, so only a sign-in lets the poller go on
BOOST_AUTO_TEST_CASE( RefusalHoldsUntilResumed )
{
    PASSES passes;
    passes.outcome = MDW_POLL::REFUSED;

    MDW_POLLER poller( std::chrono::milliseconds( 10 ),
                       [&]()
                       {
                           return passes.Count();
                       } );

    BOOST_REQUIRE( passes.WaitFor( 1 ) );

    // Thirty intervals, each of which would have sent the refused login again
    std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
    BOOST_CHECK_EQUAL( passes.Seen(), 1 );

    poller.Resume();
    BOOST_CHECK( passes.WaitFor( 2 ) );
}


// A sign-in refused between passes must stop the next one, which would send the refused login again
BOOST_AUTO_TEST_CASE( HoldStopsTheNextPass )
{
    PASSES     passes;
    MDW_POLLER poller( std::chrono::milliseconds( 50 ),
                       [&]()
                       {
                           return passes.Count();
                       } );

    BOOST_REQUIRE( passes.WaitFor( 1 ) );
    poller.Hold();

    const int held = passes.Seen();

    std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );

    // A pass already under way may end, but none starts
    BOOST_CHECK_LE( passes.Seen(), held + 1 );
}


// A sign-in that passes and a heartbeat refused before the poller looks again must leave it held, not spinning
BOOST_AUTO_TEST_CASE( ResumeThenHoldStaysHeld )
{
    PASSES                       passes;
    std::atomic<bool>            entered = false;
    std::mutex                   gate;
    std::unique_lock<std::mutex> closed( gate );

    // Left undestroyed if it spins, since its destructor would then never return
    MDW_POLLER* poller = new MDW_POLLER( std::chrono::milliseconds( 10 ),
                                         [&]()
                                         {
                                             entered = true;
                                             std::lock_guard<std::mutex> open( gate );
                                             return passes.Count();
                                         } );

    while( !entered )
        std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );

    poller->Resume();
    poller->Hold();
    closed.unlock();

    BOOST_REQUIRE( passes.WaitFor( 1 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
    BOOST_CHECK_EQUAL( passes.Seen(), 1 );

    auto answered = std::make_shared<std::atomic<bool>>( false );

    std::thread(
            [poller, answered]()
            {
                poller->Hold();
                *answered = true;
            } )
            .detach();

    for( int ii = 0; ii < 200 && !*answered; ++ii )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    BOOST_REQUIRE( *answered );
    delete poller;
}


// An unreachable server is asked less and less often rather than every interval
BOOST_AUTO_TEST_CASE( UnreachableServerBacksOff )
{
    PASSES passes;
    passes.outcome = MDW_POLL::UNREACHABLE;

    MDW_POLLER poller( std::chrono::milliseconds( 10 ),
                       [&]()
                       {
                           return passes.Count();
                       } );

    BOOST_REQUIRE( passes.WaitFor( 1 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

    // Doubling from 10 ms leaves room for at most six more passes in 500 ms, where a steady interval runs fifty
    BOOST_CHECK_LE( passes.Seen(), 7 );
}


// A pass that always throws must back off rather than log every interval
BOOST_AUTO_TEST_CASE( ThrowingPassBacksOff )
{
    PASSES     passes;
    MDW_POLLER poller( std::chrono::milliseconds( 10 ),
                       [&]() -> MDW_POLL
                       {
                           passes.Count();
                           throw std::runtime_error( "malformed reply" );
                       } );

    BOOST_REQUIRE( passes.WaitFor( 1 ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

    BOOST_CHECK_LE( passes.Seen(), 7 );
}


// A backend that exits mid-pass must not wait out a slow server, or the supervisor kills it mid-fetch
BOOST_AUTO_TEST_CASE( StopCancelsAPassUnderWay )
{
    PASSES                                passes;
    std::chrono::steady_clock::time_point stopped;
    std::chrono::steady_clock::time_point ended;

    {
        MDW_POLLER poller( std::chrono::milliseconds( 10 ),
                           [&]()
                           {
                               passes.Count();
                               const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds( 30 );

                               while( !MdwCurrentRequestCancelled() && std::chrono::steady_clock::now() < limit )
                                   std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );

                               return MDW_POLL::OK;
                           } );

        BOOST_REQUIRE( passes.WaitFor( 1 ) );
        stopped = std::chrono::steady_clock::now();
    }

    ended = std::chrono::steady_clock::now();
    BOOST_CHECK( ended - stopped < std::chrono::seconds( 5 ) );
}


BOOST_AUTO_TEST_SUITE_END()
