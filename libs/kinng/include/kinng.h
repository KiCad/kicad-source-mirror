/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_KINNG_H
#define KICAD_KINNG_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>


class KINNG_REQUEST_SERVER
{
public:
    /// A nonzero aMaxMessageBytes drops any larger request unread; zero keeps nng's default
    KINNG_REQUEST_SERVER( const std::string& aSocketUrl, std::size_t aMaxMessageBytes = 0 );

    ~KINNG_REQUEST_SERVER();

    bool Start();

    /// Joins the listener, so it must not be called, nor the server destroyed, from the callback
    void Stop();

    bool Running() const;

    void SetCallback( std::function<void(std::string*)> aFunc ) { m_callback = aFunc; }

    void Reply( const std::string& aReply );

    const std::string& SocketPath() const { return m_socketUrl; }

private:
    void listenThread();

    std::thread m_thread;

    std::atomic<bool> m_shutdown;

    std::string m_socketUrl;
    std::size_t m_maxMessageBytes;

    std::function<void(std::string*)> m_callback;

    std::string m_sharedMessage;

    std::string m_pendingReply;

    /// Set by Reply(); an empty reply is valid, so emptiness cannot signal readiness
    bool m_hasReply = false;

    std::condition_variable m_replyReady;

    std::mutex m_mutex;
};


/**
 * An asynchronous nng REQ client. Each request runs on its own nng_ctx so a slow request never blocks a fast
 * one. Completions run on a private reaper thread, and each request has its own timeout and can be cancelled.
 */
class KINNG_REQUEST_CLIENT
{
public:
    enum class STATUS
    {
        RECEIVED,
        TIMED_OUT,
        CANCELLED,  ///< Cancel() aborted the request
        CLOSED,     ///< Stop() aborted the request before any Cancel()
        FAILED      ///< Any other nng failure
    };

    struct RESULT
    {
        STATUS      status = STATUS::FAILED;
        std::string reply;  ///< Reply bytes when status is RECEIVED
    };

    using ON_COMPLETE = std::function<void( const RESULT& )>;

    /// Runs under the send lock just before a request goes out, so its callers' sends keep that order
    using ON_SEND = std::function<void()>;

    /// A nonzero aMaxMessageBytes drops any larger reply unread; zero keeps nng's default
    KINNG_REQUEST_CLIENT( const std::string& aSocketUrl, std::size_t aMaxMessageBytes = 0 );

    ~KINNG_REQUEST_CLIENT();

    /**
     * Redial a peer that is not listening yet, or was lost, after aMinMs and then twice as long each time
     * up to aMaxMs. Takes effect at Start(). Without it nng waits a random time of up to a second.
     */
    void SetReconnect( int aMinMs, int aMaxMs );

    bool Start();

    /// Cancel every in-flight request, drain completions, and close the socket.
    /// Joins the reaper, so it must not be called, nor the client destroyed, from an ON_COMPLETE.
    void Stop();

    /**
     * Send aPayload asynchronously. aTimeoutMs of 0 or less disables the timeout, and aOnComplete runs exactly
     * once on the reaper thread. aOnSend, when given, runs only if the request is sent.
     *
     * @return the id to pass to Cancel, or 0 if the client is not running.
     */
    std::uint64_t Request( const std::string& aPayload, int aTimeoutMs, ON_COMPLETE aOnComplete,
                           const ON_SEND& aOnSend = nullptr );

    /// Best-effort abort of an in-flight request.
    void Cancel( std::uint64_t aRequestId );

    /// Block until the reply, timeout or cancellation, so never call it from the UI thread
    RESULT RequestSync( const std::string& aPayload, int aTimeoutMs, const ON_SEND& aOnSend = nullptr );

private:
    struct REQUEST;
    struct IMPL;

    static void aioCallback( void* aArg );

    void finishRequest( REQUEST* aRequest );

    void reaperThread();

    std::string           m_socketUrl;
    std::size_t           m_maxMessageBytes;
    int                   m_reconnectMinMs = 0;
    int                   m_reconnectMaxMs = 0;
    std::unique_ptr<IMPL> m_impl;
};

#endif //KICAD_KINNG_H
