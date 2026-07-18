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

#include <mdw/mdw_serial_worker.h>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>


BOOST_AUTO_TEST_SUITE( MdwSerialWorker )


namespace
{
/// Owns a worker as a session does, and reports when it is destroyed
struct OWNER
{
    explicit OWNER( std::shared_ptr<std::promise<void>> aGone ) :
            gone( std::move( aGone ) )
    {
    }

    ~OWNER()
    {
        worker.reset();
        gone->set_value();
    }

    std::shared_ptr<std::promise<void>> gone;
    std::unique_ptr<MDW_SERIAL_WORKER>  worker = std::make_unique<MDW_SERIAL_WORKER>();
};
} // namespace


// A task that holds the last reference to its own worker's owner must not join its own thread
BOOST_AUTO_TEST_CASE( LastOwnerReleasedOnWorker )
{
    using namespace std::chrono_literals;

    auto                     gone = std::make_shared<std::promise<void>>();
    std::future<void>        destroyed = gone->get_future();
    std::promise<void>       released;
    std::shared_future<void> release = released.get_future().share();

    {
        std::shared_ptr<OWNER> owner = std::make_shared<OWNER>( gone );

        owner->worker->Post(
                [held = owner, release]()
                {
                    release.wait();
                } );
    }

    released.set_value();

    BOOST_CHECK( destroyed.wait_for( 10s ) == std::future_status::ready );
}


BOOST_AUTO_TEST_SUITE_END()
