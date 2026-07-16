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

#include <mdw_runtime/mdw_poller.h>

#include "mdw_runtime_internal.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>

namespace ws = kiapi::workspace;


MDW_POLL MdwPollOutcome( const ws::WorkspaceError& aError )
{
    switch( aError.code() )
    {
    case ws::WE_OK:
    case ws::WE_BUSY:
    case ws::WE_CANCELLED: return MDW_POLL::OK;

    case ws::WE_NOT_AUTHENTICATED:
    case ws::WE_AUTH_FAILED:
    case ws::WE_PERMISSION_DENIED:
    case ws::WE_UNTRUSTED_HOST: return MDW_POLL::REFUSED;

    default: return MDW_POLL::UNREACHABLE;
    }
}


std::chrono::milliseconds MdwPollInterval( std::chrono::milliseconds aDefault )
{
    const char* set = std::getenv( "KICAD_MDW_POLL_INTERVAL_MS" );
    const long  interval = set ? std::strtol( set, nullptr, 10 ) : 0;

    return interval > 0 ? std::chrono::milliseconds( interval ) : aDefault;
}


MDW_POLLER::MDW_POLLER( std::chrono::milliseconds aInterval, std::function<MDW_POLL()> aPass ) :
        m_interval( aInterval ),
        m_pass( std::move( aPass ) ),
        m_cancel( std::make_shared<std::atomic<bool>>( false ) ),
        m_thread(
                [this]()
                {
                    run();
                } )
{
}


MDW_POLLER::~MDW_POLLER()
{
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_stop = true;
    }

    m_cancel->store( true );
    m_wake.notify_all();
    m_thread.join();
}


void MDW_POLLER::Resume()
{
    {
        std::lock_guard<std::mutex> lock( m_mutex );

        if( m_held || m_failures )
            std::fprintf( stderr, "kicad-mdw: the server answered again, so polling resumes\n" );

        m_woken = !m_passed;
        m_held = false;
        m_failures = 0;
        ++m_rulings;
    }

    m_wake.notify_all();
}


void MDW_POLLER::Hold()
{
    std::lock_guard<std::mutex> lock( m_mutex );
    settle( MDW_POLL::REFUSED );

    // A wake left by a Resume() would end every wait of the hold at once, and the loop would spin under the lock
    m_woken = false;
    ++m_rulings;
}


void MDW_POLLER::settle( MDW_POLL aOutcome )
{
    if( aOutcome == MDW_POLL::REFUSED && !m_held )
        std::fprintf( stderr, "kicad-mdw: the server refused this session, so polling waits for a sign-in\n" );
    else if( aOutcome == MDW_POLL::UNREACHABLE && !m_failures )
        std::fprintf( stderr, "kicad-mdw: the server did not answer, so polling backs off\n" );
    else if( aOutcome == MDW_POLL::OK && m_failures )
        std::fprintf( stderr, "kicad-mdw: the server answered again, so polling resumes\n" );

    m_passed = aOutcome == MDW_POLL::OK;
    m_held = aOutcome == MDW_POLL::REFUSED;

    // Capped, so the back-off's shift cannot overflow however long the server stays away
    m_failures = aOutcome == MDW_POLL::UNREACHABLE ? std::min( m_failures + 1, 16 ) : 0;
}


void MDW_POLLER::run()
{
    std::unique_lock<std::mutex> lock( m_mutex );

    auto due = [this]()
    {
        return m_stop || m_woken;
    };

    while( true )
    {
        using ms = std::chrono::milliseconds;

        const ms longest = std::max<ms>( MAX_BACKOFF, m_interval );
        const ms delay = m_failures ? std::min<ms>( longest, m_interval * ( 1LL << m_failures ) ) : m_interval;

        if( m_held )
            m_wake.wait( lock, due );
        else
            m_wake.wait_for( lock, delay, due );

        if( m_stop )
            return;

        // Held while the interval ran
        if( m_held )
            continue;

        const unsigned rulings = m_rulings;

        m_woken = false;
        lock.unlock();

        MDW_POLL                   outcome = MDW_POLL::OK;
        std::optional<std::string> failure;

        try
        {
            MDW_REQUEST_CANCEL_SCOPE scope( m_cancel );
            outcome = m_pass();
        }
        catch( const std::exception& error )
        {
            failure = error.what();
        }
        catch( ... )
        {
            failure = "unknown error";
        }

        lock.lock();

        // A pass that throws every time backs off as an unanswered one does, and says so once
        if( failure )
        {
            if( !m_failures )
                std::fprintf( stderr, "kicad-mdw: a poll failed (%s)\n", failure->c_str() );

            outcome = MDW_POLL::UNREACHABLE;
        }

        // A pass that began before new credentials says nothing of them
        if( rulings == m_rulings )
            settle( outcome );
    }
}
