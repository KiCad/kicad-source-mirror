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

#include <mdw/mdw_serial_worker.h>

#include <map>

#include <wx/app.h>
#include <wx/event.h>
#include <wx/thread.h>
#include <wx/weakref.h>


MDW_SERIAL_WORKER::MDW_SERIAL_WORKER() :
        m_thread( [state = m_state]() { run( state ); } )
{
}


MDW_SERIAL_WORKER::~MDW_SERIAL_WORKER()
{
    {
        std::lock_guard<std::mutex> lock( m_state->mutex );
        m_state->stop = true;
        m_state->tasks.clear();
    }

    m_state->cv.notify_all();

    if( !m_thread.joinable() )
        return;

    // The last owner of an object that holds this worker can release it from a task
    if( m_thread.get_id() == std::this_thread::get_id() )
        m_thread.detach();
    else
        m_thread.join();
}


void MDW_SERIAL_WORKER::Post( std::function<void()> aTask )
{
    {
        std::lock_guard<std::mutex> lock( m_state->mutex );

        if( m_state->stop )
            return;

        m_state->tasks.push_back( std::move( aTask ) );
    }

    m_state->cv.notify_one();
}


bool MDW_SERIAL_WORKER::Stop( std::chrono::milliseconds aTimeout )
{
    std::unique_lock<std::mutex> lock( m_state->mutex );

    m_state->stop = true;
    m_state->tasks.clear();
    m_state->cv.notify_all();

    if( !m_state->cv.wait_for( lock, aTimeout,
                               [this]()
                               {
                                   return m_state->done;
                               } ) )
    {
        return false;
    }

    lock.unlock();
    m_thread.join();
    return true;
}


void MDW_SERIAL_WORKER::run( const std::shared_ptr<STATE>& aState )
{
    while( true )
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock( aState->mutex );
            aState->cv.wait( lock, [&]() { return aState->stop || !aState->tasks.empty(); } );

            if( aState->stop )
            {
                aState->done = true;
                aState->cv.notify_all();
                return;
            }

            task = std::move( aState->tasks.front() );
            aState->tasks.pop_front();
        }

        task();
    }
}


namespace
{
// Touched only on the UI thread, so it needs no lock
std::map<std::uint64_t, wxWeakRef<wxEvtHandler>>& owners()
{
    static std::map<std::uint64_t, wxWeakRef<wxEvtHandler>> s_owners;
    return s_owners;
}
}


MDW_UI_COMPLETION::TICKET MDW_UI_COMPLETION::Register( wxEvtHandler* aOwner )
{
    wxASSERT( wxIsMainThread() );

    static std::uint64_t s_next = 0;
    std::uint64_t        ticket = ++s_next;

    owners().emplace( ticket, wxWeakRef<wxEvtHandler>( aOwner ) );

    // Queued behind every completion posted with the ticket, so each still finds its owner
    return TICKET( new std::uint64_t( ticket ),
                   []( const std::uint64_t* aTicket )
                   {
                       std::uint64_t id = *aTicket;
                       delete aTicket;

                       if( wxTheApp )
                           wxTheApp->CallAfter( [id]() { owners().erase( id ); } );
                   } );
}


void MDW_UI_COMPLETION::Post( const TICKET& aTicket, std::function<void()> aDone )
{
    if( !wxTheApp || !aTicket )
        return;

    wxTheApp->CallAfter(
            [aTicket = *aTicket, done = std::move( aDone )]()
            {
                auto it = owners().find( aTicket );

                if( it != owners().end() && it->second.get() && done )
                    done();
            } );
}
