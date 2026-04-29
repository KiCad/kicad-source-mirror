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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <git/kigit_orphan_registry.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>


BOOST_AUTO_TEST_SUITE( KiGitOrphanRegistry )


BOOST_AUTO_TEST_CASE( JoinAllCompletesFastWork )
{
    KIGIT_ORPHAN_REGISTRY registry;
    std::atomic<int>      counter{ 0 };

    for( int i = 0; i < 4; ++i )
    {
        registry.Register( "fast-" + std::to_string( i ),
                           [&counter]()
                           {
                               std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
                               counter.fetch_add( 1, std::memory_order_relaxed );
                           } );
    }

    size_t stuck = registry.JoinAll( std::chrono::seconds( 2 ) );

    BOOST_CHECK_EQUAL( stuck, 0u );
    BOOST_CHECK_EQUAL( counter.load(), 4 );
    BOOST_CHECK_EQUAL( registry.TrackedCount(), 0u );
}


BOOST_AUTO_TEST_CASE( JoinAllTimesOutOnStuckWork )
{
    KIGIT_ORPHAN_REGISTRY   registry;
    std::mutex              releaseMutex;
    std::condition_variable releaseCv;
    bool                    release = false;
    std::atomic<bool>       workerEntered{ false };
    std::atomic<bool>       workerFinished{ false };

    registry.Register(
            "stuck",
            [&]()
            {
                workerEntered.store( true, std::memory_order_release );

                std::unique_lock<std::mutex> lock( releaseMutex );
                releaseCv.wait( lock, [&]() { return release; } );
                lock.unlock();

                workerFinished.store( true, std::memory_order_release );
            } );

    // Wait until the worker is actually running so we're exercising the
    // timeout path, not the trivial "thread never started" path.

    while( !workerEntered.load( std::memory_order_acquire ) )
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

    auto   start = std::chrono::steady_clock::now();
    size_t stuck = registry.JoinAll( std::chrono::milliseconds( 100 ) );
    auto   elapsed = std::chrono::steady_clock::now() - start;

    BOOST_CHECK_EQUAL( stuck, 1u );
    BOOST_CHECK( elapsed < std::chrono::seconds( 2 ) );

    // After JoinAll the registry is empty even though the thread kept
    // running.  Release the worker now so it can exit cleanly; it was
    // detached when JoinAll gave up on it.

    {
        std::lock_guard<std::mutex> lock( releaseMutex );
        release = true;
    }

    releaseCv.notify_all();

    // Wait for the detached worker to actually return before this test
    // function exits.  Otherwise the worker is still touching releaseMutex,
    // releaseCv and release on the stack while the next test reuses that
    // memory.

    for( int i = 0; i < 1000 && !workerFinished.load( std::memory_order_acquire ); ++i )
        std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );

    BOOST_CHECK( workerFinished.load() );
    BOOST_CHECK_EQUAL( registry.TrackedCount(), 0u );
}


BOOST_AUTO_TEST_CASE( DestructorDetachesOutstandingThreads )
{
    std::mutex              releaseMutex;
    std::condition_variable releaseCv;
    bool                    release = false;
    std::atomic<bool>       workerEntered{ false };
    std::atomic<bool>       workerFinished{ false };

    {
        KIGIT_ORPHAN_REGISTRY registry;

        registry.Register(
                "outlive-registry",
                [&]()
                {
                    workerEntered.store( true, std::memory_order_release );

                    {
                        std::unique_lock<std::mutex> lock( releaseMutex );
                        releaseCv.wait( lock, [&]() { return release; } );
                    }

                    // Set workerFinished after the unique_lock has destructed so the
                    // test waiting on workerFinished cannot return while we're still
                    // touching releaseMutex on the test's stack.
                    workerFinished.store( true, std::memory_order_release );
                } );

        while( !workerEntered.load( std::memory_order_acquire ) )
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

        // Registry destructor runs here.  It must detach the still-running
        // worker instead of terminating the process by destroying a joinable
        // std::thread.
    }

    {
        std::lock_guard<std::mutex> lock( releaseMutex );
        release = true;
    }

    releaseCv.notify_all();

    // Wait for the detached worker to actually finish.  Under sanitizer slowdown
    // this can take a noticeable fraction of a second; the loop is generous
    // enough to keep the test from leaking the worker into the next test.

    for( int i = 0; i < 1000 && !workerFinished.load( std::memory_order_acquire ); ++i )
        std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );

    BOOST_CHECK( workerFinished.load() );
}


BOOST_AUTO_TEST_CASE( RegisterReapsFinishedEntries )
{
    KIGIT_ORPHAN_REGISTRY registry;

    BOOST_CHECK( registry.Register( "quick-1", []() {} ) );

    // Give the first worker a moment to finish before we register the
    // second.  The second Register() should reap the first.

    std::this_thread::sleep_for( std::chrono::milliseconds( 25 ) );

    BOOST_CHECK( registry.Register( "quick-2", []() {} ) );

    size_t stuck = registry.JoinAll( std::chrono::seconds( 1 ) );
    BOOST_CHECK_EQUAL( stuck, 0u );
}


BOOST_AUTO_TEST_CASE( RegisterRejectsAfterJoinAll )
{
    KIGIT_ORPHAN_REGISTRY registry;

    // Close the registry; no outstanding work, so JoinAll returns 0.

    BOOST_CHECK_EQUAL( registry.JoinAll( std::chrono::milliseconds( 1 ) ), 0u );

    std::atomic<bool> workRan{ false };

    bool accepted = registry.Register( "late",
                                       [&workRan]() { workRan.store( true ); } );

    BOOST_CHECK( !accepted );

    // Work must not have been scheduled.

    std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
    BOOST_CHECK( !workRan.load() );
}


BOOST_AUTO_TEST_CASE( MoveOnlyWorkIsAccepted )
{
    KIGIT_ORPHAN_REGISTRY registry;

    auto             payload = std::make_unique<int>( 42 );
    std::atomic<int> observed{ 0 };

    bool accepted = registry.Register(
            "move-only",
            [payload = std::move( payload ), &observed]() mutable
            {
                observed.store( *payload, std::memory_order_release );
            } );

    BOOST_CHECK( accepted );
    BOOST_CHECK_EQUAL( registry.JoinAll( std::chrono::seconds( 1 ) ), 0u );
    BOOST_CHECK_EQUAL( observed.load(), 42 );
}


BOOST_AUTO_TEST_SUITE_END()
