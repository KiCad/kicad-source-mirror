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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <vector>

constexpr uint64_t RHI_FRAMES_IN_FLIGHT = 2;

enum class RHI_DEFERRED_RESOURCE_TYPE : uint8_t
{
    BUFFER,
    TEXTURE,
    SHADER,
    PIPELINE,
    BIND_GROUP_LAYOUT,
    BIND_GROUP,
    RENDER_PASS,
    FRAMEBUFFER,
};


struct RHI_DEFERRED_RELEASE_ENTRY
{
    RHI_DEFERRED_RESOURCE_TYPE type;
    uint32_t                   handleValue;  /**< generation and index */
    uint64_t                   retireFrame;
};


/**
 * Holds destroyed GPU resource handles until no in-flight command buffer can still reference them.
 *
 * The UI thread enqueues while the render thread frees. The free callback runs outside the lock so
 * slow GPU destruction does not block producers and a callback that re-enters the queue cannot deadlock.
 */
class RHI_DEFERRED_RELEASE_QUEUE
{
public:
    void Enqueue( RHI_DEFERRED_RESOURCE_TYPE aType, uint32_t aHandleValue,
                  uint64_t aCurrentFrame )
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_entries.push_back( { aType, aHandleValue, aCurrentFrame + RHI_FRAMES_IN_FLIGHT } );
    }

    template <typename Func>
    void ProcessRetired( uint64_t aCompletedFrame, Func&& aFreeFunc )
    {
        std::vector<RHI_DEFERRED_RELEASE_ENTRY> retired;

        {
            std::lock_guard<std::mutex> lock( m_mutex );

            auto firstRetired = std::stable_partition( m_entries.begin(), m_entries.end(),
                                                       [&]( const RHI_DEFERRED_RELEASE_ENTRY& aEntry )
                                                       {
                                                           return aEntry.retireFrame > aCompletedFrame;
                                                       } );

            retired.assign( firstRetired, m_entries.end() );
            m_entries.erase( firstRetired, m_entries.end() );
        }

        for( auto& entry : retired )
            aFreeFunc( entry );
    }

    /** @return true when ProcessRetired( aCompletedFrame ) would free at least one entry. */
    bool HasRetired( uint64_t aCompletedFrame ) const
    {
        std::lock_guard<std::mutex> lock( m_mutex );

        return std::ranges::any_of( m_entries,
                                    [&]( const RHI_DEFERRED_RELEASE_ENTRY& aEntry )
                                    {
                                        return aEntry.retireFrame <= aCompletedFrame;
                                    } );
    }

    size_t PendingCount() const
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        return m_entries.size();
    }

    template <typename Func>
    void FlushAll( Func&& aFreeFunc )
    {
        std::vector<RHI_DEFERRED_RELEASE_ENTRY> retired;

        {
            std::lock_guard<std::mutex> lock( m_mutex );
            retired.swap( m_entries );
        }

        for( auto& entry : retired )
            aFreeFunc( entry );
    }

private:
    mutable std::mutex                      m_mutex;
    std::vector<RHI_DEFERRED_RELEASE_ENTRY> m_entries;
};
