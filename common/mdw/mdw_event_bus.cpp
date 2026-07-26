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

#include <mdw/mdw_event_bus.h>
#include <trace_helpers.h>

#include <wx/log.h>

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <exception>
#include <map>
#include <mutex>
#include <thread>
#include <vector>


namespace
{
// Echoes a backend never sends are forgotten after this many newer operations
constexpr size_t MAX_PENDING_ECHOES = 256;
}


struct MDW_SUBSCRIPTION::ENTRY
{
    MDW_EVENT_BUS::HANDLER handler; ///< Guarded by STATE::delivering
};


struct MDW_SUBSCRIPTION::STATE
{
    enum class ECHO
    {
        LOCAL_DELIVERED,  ///< Local copy delivered, backend copy will be dropped
        BACKEND_DELIVERED ///< Backend copy delivered, local copy will be dropped
    };

    // Makes acceptance, state application and queueing one step, so the queue holds events
    // in the order their state was applied. Recursive so an apply step may publish
    std::recursive_mutex                            gate;

    // Held for each delivery and by Reset(), so a reset returns only after any call of its
    // handler. Recursive so a handler may reset its own subscription
    std::recursive_mutex delivering;

    std::mutex                                      mutex;
    std::deque<MDW_EVENT>                           queue; ///< Accepted, not yet delivered
    bool                                            stop = false;
    std::condition_variable                         wake;
    std::thread                                     dispatcher;
    std::vector<std::shared_ptr<ENTRY>>             entries;
    // Keyed by operation, then item and type, since one operation such as a library check-in publishes per
    // member and may announce several changes to one item
    std::map<wxString, std::map<std::pair<wxString, MDW_EVENT_TYPE>, ECHO>> echoes;
    std::deque<wxString>                            echoOrder;
    std::map<wxString, std::uint64_t>               lastSeq;
    MDW_EVENT_BUS::ACCEPT_HOOK                      acceptHook;
};


MDW_SUBSCRIPTION::MDW_SUBSCRIPTION( std::weak_ptr<STATE> aState, std::shared_ptr<ENTRY> aEntry ) :
        m_state( std::move( aState ) ),
        m_entry( std::move( aEntry ) )
{
}


MDW_SUBSCRIPTION::~MDW_SUBSCRIPTION()
{
    Reset();
}


MDW_SUBSCRIPTION::MDW_SUBSCRIPTION( MDW_SUBSCRIPTION&& aOther ) noexcept :
        m_state( std::move( aOther.m_state ) ),
        m_entry( std::move( aOther.m_entry ) )
{
    aOther.m_entry.reset();
}


MDW_SUBSCRIPTION& MDW_SUBSCRIPTION::operator=( MDW_SUBSCRIPTION&& aOther ) noexcept
{
    if( this != &aOther )
    {
        Reset();
        m_state = std::move( aOther.m_state );
        m_entry = std::move( aOther.m_entry );
        aOther.m_entry.reset();
    }

    return *this;
}


void MDW_SUBSCRIPTION::Reset()
{
    if( !m_entry )
        return;

    // With the bus gone nothing can be dispatching to the handler any more
    if( std::shared_ptr<STATE> state = m_state.lock() )
    {
        std::lock_guard<std::recursive_mutex> delivering( state->delivering );
        m_entry->handler = nullptr;

        std::lock_guard<std::mutex> lock( state->mutex );
        std::erase( state->entries, m_entry );
    }

    m_entry.reset();
    m_state.reset();
}


MDW_EVENT_BUS::MDW_EVENT_BUS() :
        m_state( std::make_shared<STATE>() )
{
}


MDW_EVENT_BUS::~MDW_EVENT_BUS()
{
    std::thread dispatcher;

    {
        std::lock_guard<std::mutex> lock( m_state->mutex );
        m_state->stop = true;
        dispatcher = std::move( m_state->dispatcher );
    }

    m_state->wake.notify_all();

    // Events already accepted are still delivered before the thread ends
    if( !dispatcher.joinable() )
        return;

    if( dispatcher.get_id() == std::this_thread::get_id() )
        dispatcher.detach();
    else
        dispatcher.join();
}


void MDW_EVENT_BUS::deliverQueued( STATE& aState )
{
    std::unique_lock<std::mutex> lock( aState.mutex );

    while( true )
    {
        aState.wake.wait( lock,
                          [&]()
                          {
                              return aState.stop || !aState.queue.empty();
                          } );

        if( aState.queue.empty() )
            break;

        MDW_EVENT event = std::move( aState.queue.front() );
        aState.queue.pop_front();

        std::vector<std::shared_ptr<MDW_SUBSCRIPTION::ENTRY>> entries = aState.entries;

        lock.unlock();

        {
            std::lock_guard<std::recursive_mutex> delivering( aState.delivering );

            for( const std::shared_ptr<MDW_SUBSCRIPTION::ENTRY>& entry : entries )
            {
                // A copy, because the handler may reset its own subscription mid-call
                HANDLER handler = entry->handler;

                if( !handler )
                    continue;

                try
                {
                    handler( event );
                }
                catch( const std::exception& e )
                {
                    wxLogTrace( traceMdw, wxS( "Event handler threw: %s" ), e.what() );
                }
                catch( ... )
                {
                    wxLogTrace( traceMdw, wxS( "Event handler threw" ) );
                }
            }
        }

        lock.lock();
    }
}


MDW_SUBSCRIPTION MDW_EVENT_BUS::Subscribe( HANDLER aHandler )
{
    auto entry = std::make_shared<MDW_SUBSCRIPTION::ENTRY>();
    entry->handler = std::move( aHandler );

    std::lock_guard<std::mutex> lock( m_state->mutex );
    m_state->entries.push_back( entry );

    return MDW_SUBSCRIPTION( m_state, entry );
}


// Publishers pass their own reference, since a handler may destroy the bus mid-publish
void MDW_EVENT_BUS::dispatch( const std::shared_ptr<STATE>& aState, const MDW_EVENT& aEvent )
{
    {
        std::lock_guard<std::mutex> lock( aState->mutex );

        // A destroyed bus has no owner left to join a new thread
        if( aState->stop )
            return;

        aState->queue.push_back( aEvent );

        // The thread keeps the state alive, since a handler may destroy the bus it runs on
        if( !aState->dispatcher.joinable() )
        {
            aState->dispatcher = std::thread(
                    [state = aState]()
                    {
                        deliverQueued( *state );
                    } );
        }
    }

    aState->wake.notify_one();
}


void MDW_EVENT_BUS::SetAcceptHook( ACCEPT_HOOK aHook )
{
    std::lock_guard<std::mutex> lock( m_state->mutex );
    m_state->acceptHook = std::move( aHook );
}


void MDW_EVENT_BUS::ExpectEcho( const wxString& aOperationId )
{
    if( aOperationId.IsEmpty() )
        return;

    std::lock_guard<std::mutex> lock( m_state->mutex );

    if( m_state->echoes.try_emplace( aOperationId ).second )
        m_state->echoOrder.push_back( aOperationId );

    while( m_state->echoOrder.size() > MAX_PENDING_ECHOES )
    {
        wxString oldest = m_state->echoOrder.front();
        m_state->echoOrder.pop_front();

        // A reused idempotency id is queued again and must survive its older copy
        if( std::find( m_state->echoOrder.begin(), m_state->echoOrder.end(), oldest )
            == m_state->echoOrder.end() )
        {
            m_state->echoes.erase( oldest );
        }
    }
}


void MDW_EVENT_BUS::CancelEcho( const wxString& aOperationId )
{
    std::lock_guard<std::mutex> lock( m_state->mutex );
    m_state->echoes.erase( aOperationId );
}


void MDW_EVENT_BUS::Publish( const MDW_EVENT& aEvent )
{
    std::shared_ptr<STATE> state = m_state;
    dispatch( state, aEvent );
}


void MDW_EVENT_BUS::PublishLocal( const MDW_EVENT& aEvent, const std::function<void()>& aApply )
{
    std::shared_ptr<STATE> state = m_state;

    // Acceptance, state application and queueing are one step against the backend path
    std::lock_guard<std::recursive_mutex> gate( state->gate );
    MDW_EVENT                             event = aEvent;
    event.from_this_client = true;

    if( !aEvent.operation_id.IsEmpty() )
    {
        std::lock_guard<std::mutex> lock( state->mutex );
        auto                        it = state->echoes.find( aEvent.operation_id );

        if( it != state->echoes.end() )
        {
            auto echo = it->second.find( { aEvent.item_id, aEvent.type } );

            if( echo != it->second.end() && echo->second == STATE::ECHO::BACKEND_DELIVERED )
            {
                it->second.erase( echo );
                return;
            }

            it->second[{ aEvent.item_id, aEvent.type }] = STATE::ECHO::LOCAL_DELIVERED;
        }
    }

    if( aApply )
        aApply();

    dispatch( state, event );
}


bool MDW_EVENT_BUS::PublishFromBackend( const MDW_EVENT& aEvent )
{
    std::shared_ptr<STATE>                state = m_state;
    std::lock_guard<std::recursive_mutex> gate( state->gate );
    MDW_EVENT   event = aEvent;
    bool        gap = false;
    bool        suppress = false;
    ACCEPT_HOOK hook;

    {
        std::lock_guard<std::mutex> lock( state->mutex );

        if( aEvent.seq != 0 )
        {
            std::uint64_t& last = state->lastSeq[aEvent.workspace_id];

            if( aEvent.seq <= last )
                return false;

            gap = last != 0 && aEvent.seq > last + 1;
            last = aEvent.seq;
        }

        if( !aEvent.operation_id.IsEmpty() )
        {
            auto it = state->echoes.find( aEvent.operation_id );

            if( it != state->echoes.end() )
            {
                auto echo = it->second.find( { aEvent.item_id, aEvent.type } );
                event.from_this_client = true;
                suppress = echo != it->second.end() && echo->second == STATE::ECHO::LOCAL_DELIVERED;

                // The operation stays announced, since its other items may still echo
                if( suppress )
                    it->second.erase( echo );
                else
                    it->second[{ aEvent.item_id, aEvent.type }] = STATE::ECHO::BACKEND_DELIVERED;
            }
        }

        hook = state->acceptHook;
    }

    // Authoritative state is applied even for a suppressed echo, before anyone reads it
    if( hook )
        hook( event );

    if( !suppress )
        dispatch( state, event );

    if( gap )
    {
        MDW_EVENT resync;
        resync.type = MDW_EVENT_TYPE::WORKSPACE_RECONNECTED;
        resync.workspace_id = aEvent.workspace_id;
        dispatch( state, resync );
    }

    return true;
}


void MDW_EVENT_BUS::ResetBackendState()
{
    std::lock_guard<std::mutex> lock( m_state->mutex );
    m_state->echoes.clear();
    m_state->echoOrder.clear();
    m_state->lastSeq.clear();
}
