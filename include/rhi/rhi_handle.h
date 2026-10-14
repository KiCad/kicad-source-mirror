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

#include <cstdint>
#include <mutex>
#include <vector>

/**
 * Opaque GPU resource handle, typed by a tag struct so handles of different kinds cannot be mixed.
 *
 * The low 20 bits index the backend's resource table and the high 12 bits hold a generation that
 * changes each time the slot is reused, so a stale handle is detected instead of aliasing a new resource.
 */
template <typename Tag>
struct RHI_HANDLE
{
    uint32_t value = 0;

    bool operator==( const RHI_HANDLE& other ) const = default;

    explicit operator bool() const { return value != 0; }
};


constexpr uint32_t RHI_HANDLE_INDEX_BITS = 20;
constexpr uint32_t RHI_HANDLE_GEN_BITS   = 12;
constexpr uint32_t RHI_HANDLE_INDEX_MASK = ( 1u << RHI_HANDLE_INDEX_BITS ) - 1;
constexpr uint32_t RHI_HANDLE_GEN_MASK   = ( 1u << RHI_HANDLE_GEN_BITS ) - 1;


template <typename Tag>
uint32_t RhiHandleIndex( RHI_HANDLE<Tag> h )
{
    return h.value & RHI_HANDLE_INDEX_MASK;
}


template <typename Tag>
uint32_t RhiHandleGeneration( RHI_HANDLE<Tag> h )
{
    return ( h.value >> RHI_HANDLE_INDEX_BITS ) & RHI_HANDLE_GEN_MASK;
}


template <typename Tag>
RHI_HANDLE<Tag> RhiMakeHandle( uint32_t index, uint32_t generation )
{
    return { ( index & RHI_HANDLE_INDEX_MASK )
             | ( ( generation & RHI_HANDLE_GEN_MASK ) << RHI_HANDLE_INDEX_BITS ) };
}


struct RHI_TAG_BUFFER {};
struct RHI_TAG_TEXTURE {};
struct RHI_TAG_SHADER {};
struct RHI_TAG_PIPELINE {};
struct RHI_TAG_RENDER_PASS {};
struct RHI_TAG_BIND_GROUP_LAYOUT {};
struct RHI_TAG_BIND_GROUP {};
struct RHI_TAG_SURFACE {};
struct RHI_TAG_SWAPCHAIN {};
struct RHI_TAG_FRAMEBUFFER {};

using RHI_BufferHandle          = RHI_HANDLE<RHI_TAG_BUFFER>;
using RHI_TextureHandle         = RHI_HANDLE<RHI_TAG_TEXTURE>;
using RHI_ShaderHandle          = RHI_HANDLE<RHI_TAG_SHADER>;
using RHI_PipelineHandle        = RHI_HANDLE<RHI_TAG_PIPELINE>;
using RHI_RenderPassHandle      = RHI_HANDLE<RHI_TAG_RENDER_PASS>;
using RHI_BindGroupLayoutHandle = RHI_HANDLE<RHI_TAG_BIND_GROUP_LAYOUT>;
using RHI_BindGroupHandle       = RHI_HANDLE<RHI_TAG_BIND_GROUP>;
using RHI_SurfaceHandle         = RHI_HANDLE<RHI_TAG_SURFACE>;
using RHI_SwapchainHandle       = RHI_HANDLE<RHI_TAG_SWAPCHAIN>;
using RHI_FramebufferHandle     = RHI_HANDLE<RHI_TAG_FRAMEBUFFER>;


/**
 * Thread-safe pool that allocates and recycles generation-tracked handles for device implementations.
 */
template <typename Tag>
class RHI_HANDLE_POOL
{
public:
    RHI_HANDLE_POOL() { m_entries.reserve( 64 ); }

    RHI_HANDLE<Tag> Allocate()
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        uint32_t                    index;

        if( !m_freeList.empty() )
        {
            index = m_freeList.back();
            m_freeList.pop_back();
        }
        else
        {
            index = static_cast<uint32_t>( m_entries.size() );

            if( index > RHI_HANDLE_INDEX_MASK )
                return {};

            m_entries.push_back( { 1, true } );
        }

        m_entries[index].alive = true;
        return RhiMakeHandle<Tag>( index, m_entries[index].generation );
    }

    /**
     * Make a handle invalid while keeping its slot out of the free list until Release().
     *
     * Deferred release needs this because the GPU resource outlives the handle, and a recycled slot
     * would let the pending release destroy an unrelated resource.
     *
     * @return true only for the call that claimed the slot. Callers enqueue the release only then, so
     *         concurrent destroys of one handle never free it twice.
     */
    bool MarkPendingDestroy( RHI_HANDLE<Tag> handle )
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        Entry*                      entry = findLive( *this, handle );

        if( !entry || entry->pendingDestroy )
            return false;

        entry->pendingDestroy = true;
        return true;
    }

    void Release( RHI_HANDLE<Tag> handle )
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        Entry*                      entry = findLive( *this, handle );

        if( !entry )
            return;

        entry->alive = false;
        entry->pendingDestroy = false;

        uint32_t nextGen = ( entry->generation + 1 ) & RHI_HANDLE_GEN_MASK;

        /**
         * A wrapped generation would let a very old handle match a new resource, so retire the slot.
         * The 20-bit index space absorbs the rare leak.
         */
        if( nextGen == 0 )
            return;

        entry->generation = nextGen;
        m_freeList.push_back( RhiHandleIndex( handle ) );
    }

    bool IsValid( RHI_HANDLE<Tag> handle ) const
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        const Entry*                entry = findLive( *this, handle );

        return entry && !entry->pendingDestroy;
    }

private:

    struct Entry
    {
        uint32_t generation     = 0;
        bool     alive          = false;
        bool     pendingDestroy = false;
    };

    /**
     * Handles are allocated and released from both the UI and render threads, and Allocate() can
     * reallocate m_entries while IsValid() reads it.
     */
    mutable std::mutex    m_mutex;
    std::vector<Entry>    m_entries;
    std::vector<uint32_t> m_freeList;

    /**
     * Return the entry of a handle that is alive at its generation, or nullptr. The caller holds m_mutex.
     */
    template <typename POOL>
    static auto findLive( POOL& aPool, RHI_HANDLE<Tag> handle ) -> decltype( &aPool.m_entries[0] )
    {
        const uint32_t index = RhiHandleIndex( handle );

        if( !handle || index >= aPool.m_entries.size() )
            return nullptr;

        auto& entry = aPool.m_entries[index];

        return entry.alive && RhiHandleGeneration( handle ) == entry.generation ? &entry : nullptr;
    }
};


/**
 * Release a deferred handle whose only teardown is dropping its table entry and recycling the
 * pool slot, as for a resource that holds no native object or one that reference counting frees.
 */
template <typename MAP, typename Tag>
void RhiReleasePooledResource( MAP& aMap, RHI_HANDLE_POOL<Tag>& aPool, uint32_t aHandleValue )
{
    aMap.erase( aHandleValue & RHI_HANDLE_INDEX_MASK );
    aPool.Release( RHI_HANDLE<Tag>{ aHandleValue } );
}
