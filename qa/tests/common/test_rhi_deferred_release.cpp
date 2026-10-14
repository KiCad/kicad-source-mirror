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
#include <rhi/rhi_deferred_release.h>

#include <vector>


namespace
{
std::vector<uint32_t> retire( RHI_DEFERRED_RELEASE_QUEUE& aQueue, uint64_t aCompletedFrame )
{
    std::vector<uint32_t> freed;

    aQueue.ProcessRetired( aCompletedFrame,
                           [&]( const RHI_DEFERRED_RELEASE_ENTRY& aEntry )
                           {
                               freed.push_back( aEntry.handleValue );
                           } );

    return freed;
}
} // namespace


BOOST_AUTO_TEST_SUITE( RhiDeferredRelease )


BOOST_AUTO_TEST_CASE( HeldForFramesInFlight )
{
    RHI_DEFERRED_RELEASE_QUEUE queue;
    queue.Enqueue( RHI_DEFERRED_RESOURCE_TYPE::BUFFER, 1, 10 );

    BOOST_CHECK( !queue.HasRetired( 10 + RHI_FRAMES_IN_FLIGHT - 1 ) );
    BOOST_CHECK( retire( queue, 10 + RHI_FRAMES_IN_FLIGHT - 1 ).empty() );

    BOOST_CHECK( queue.HasRetired( 10 + RHI_FRAMES_IN_FLIGHT ) );
    BOOST_CHECK( retire( queue, 10 + RHI_FRAMES_IN_FLIGHT ) == std::vector<uint32_t>{ 1 } );
    BOOST_CHECK( !queue.HasRetired( 1000 ) );
}


BOOST_AUTO_TEST_CASE( RetiresInOrderAndKeepsTheRest )
{
    RHI_DEFERRED_RELEASE_QUEUE queue;

    // Interleaved so that retiring has to pick entries out of the middle
    queue.Enqueue( RHI_DEFERRED_RESOURCE_TYPE::BUFFER, 1, 0 );
    queue.Enqueue( RHI_DEFERRED_RESOURCE_TYPE::TEXTURE, 2, 5 );
    queue.Enqueue( RHI_DEFERRED_RESOURCE_TYPE::BUFFER, 3, 1 );
    queue.Enqueue( RHI_DEFERRED_RESOURCE_TYPE::TEXTURE, 4, 6 );
    queue.Enqueue( RHI_DEFERRED_RESOURCE_TYPE::BUFFER, 5, 2 );

    BOOST_CHECK( ( retire( queue, 2 + RHI_FRAMES_IN_FLIGHT ) == std::vector<uint32_t>{ 1, 3, 5 } ) );

    std::vector<uint32_t> flushed;

    queue.FlushAll(
            [&]( const RHI_DEFERRED_RELEASE_ENTRY& aEntry )
            {
                flushed.push_back( aEntry.handleValue );
            } );

    BOOST_CHECK( ( flushed == std::vector<uint32_t>{ 2, 4 } ) );
    BOOST_CHECK( retire( queue, 1000 ).empty() );
}


BOOST_AUTO_TEST_SUITE_END()
