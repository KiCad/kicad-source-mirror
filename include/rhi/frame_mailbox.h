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

#ifndef RHI_FRAME_MAILBOX_H
#define RHI_FRAME_MAILBOX_H

#include <rhi/frame_packet.h>

#include <atomic>
#include <memory>


namespace KIGFX
{

/**
 * Lock-free single-slot mailbox carrying frame state from the main thread to the render thread.
 *
 * Only the latest packet is kept, so the render thread never draws a stale frame. One producer and
 * one consumer may call Submit() and Load() concurrently.
 */
class FRAME_MAILBOX
{
public:
    FRAME_MAILBOX() = default;

    ~FRAME_MAILBOX()
    {
        delete m_pending.load( std::memory_order_acquire );
    }

    FRAME_MAILBOX( const FRAME_MAILBOX& ) = delete;
    FRAME_MAILBOX& operator=( const FRAME_MAILBOX& ) = delete;

    /**
     * @return the displaced unread packet, or nullptr if the consumer already took it.
     */
    std::unique_ptr<FRAME_PACKET> Submit( std::unique_ptr<FRAME_PACKET> aPacket )
    {
        if( aPacket )
        {
            aPacket->bufferGeneration = std::min( aPacket->staticDrawDesc.bufferGeneration,
                                                  aPacket->dynamicDrawDesc.bufferGeneration );
        }

        if( aPacket )
            m_pendingInputSerial.store( aPacket->inputSerial, std::memory_order_release );

        FRAME_PACKET* old = m_pending.exchange( aPacket.release(), std::memory_order_acq_rel );
        return std::unique_ptr<FRAME_PACKET>( old );
    }

    /**
     * @return the pending packet, or nullptr if nothing was submitted since the last Load().
     */
    std::unique_ptr<FRAME_PACKET> Load()
    {
        std::unique_ptr<FRAME_PACKET> packet( m_pending.exchange( nullptr, std::memory_order_acq_rel ) );

        /** Older packets are never drawn again, so their retired buffers can be freed. */
        if( packet )
            m_consumedGeneration.store( packet->bufferGeneration, std::memory_order_release );

        return packet;
    }

    /**
     * @return the buffer generation of the packet the consumer loaded last. The producer may
     *         free whatever it retired at or before that generation.
     */
    uint64_t ConsumedBufferGeneration() const
    {
        return m_consumedGeneration.load( std::memory_order_acquire );
    }

    /**
     * @return the input serial of the newest packet submitted.
     */
    uint64_t PendingInputSerial() const
    {
        return m_pendingInputSerial.load( std::memory_order_acquire );
    }

private:
    std::atomic<FRAME_PACKET*> m_pending{ nullptr };
    std::atomic<uint64_t>      m_consumedGeneration{ 0 };
    std::atomic<uint64_t>      m_pendingInputSerial{ 0 };
};


} // namespace KIGFX


#endif // RHI_FRAME_MAILBOX_H
