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

#include "db_worker_thread.h"

#include <database/database_lib_settings.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <algorithm>
#include <filesystem>
#include <map>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif


namespace
{
struct SOURCE_COUNTS
{
    size_t slots = 0;     ///< jobs accepted and not yet returned, abandoned ones included
    size_t abandoned = 0; ///< jobs given up on whose thread has not returned
};

struct REGISTRY
{
    std::mutex                           m;
    std::condition_variable              changed; ///< signalled when a slot is returned or a job is abandoned
    std::map<std::string, SOURCE_COUNTS> sources;
};

// Never destroyed, since an abandoned thread may return after static destruction has begun
REGISTRY& registry()
{
    static REGISTRY* instance = new REGISTRY;

    return *instance;
}

thread_local bool t_onWorker = false;


void eraseIfIdle( REGISTRY& aReg, const std::string& aKey )
{
    auto it = aReg.sources.find( aKey );

    if( it != aReg.sources.end() && it->second.slots == 0 && it->second.abandoned == 0 )
        aReg.sources.erase( it );
}


// Waits until aDeadline for a slot while healthy jobs hold them, and refuses at once when abandoned jobs do
bool acquireSlot( const std::string& aKey, std::chrono::steady_clock::time_point aDeadline,
                  const std::atomic<bool>& aStopping )
{
    REGISTRY&        reg = registry();
    std::unique_lock lock( reg.m );
    bool             timedOut = false;

    for( ;; )
    {
        if( aStopping )
        {
            eraseIfIdle( reg, aKey );
            return false;
        }

        SOURCE_COUNTS& counts = reg.sources[aKey];

        if( counts.abandoned >= DB_WORKER_THREAD::MAX_ABANDONED )
        {
            eraseIfIdle( reg, aKey );
            return false;
        }

        if( counts.slots < DB_WORKER_THREAD::MAX_ABANDONED )
        {
            ++counts.slots;
            return true;
        }

        if( timedOut )
        {
            eraseIfIdle( reg, aKey );
            return false;
        }

        timedOut = reg.changed.wait_until( lock, aDeadline ) == std::cv_status::timeout;
    }
}


void returnSlot( const std::string& aKey )
{
    std::lock_guard lock( registry().m );

    --registry().sources[aKey].slots;
    eraseIfIdle( registry(), aKey );
    registry().changed.notify_all();
}
} // namespace


/// State shared by the owner and the worker thread; the thread keeps it alive after the owner is gone
struct DB_WORKER_CORE
{
    typedef std::vector<std::vector<DB_VALUE>> BATCH;

    std::string             key;
    std::mutex              m;
    std::condition_variable cv;

    std::function<void( DB_JOB_CONTEXT& )> job;
    bool                                   hasJob = false;
    bool                                   busy = false;
    bool                                   done = false;
    bool                                   stop = false;
    std::atomic<bool>                      initFailed{ false };
    std::atomic<bool>                      abandoned{ false };
    std::atomic<bool>                      rejected{ false };
    bool                                   exited = false; ///< Set once the exit hook has returned

    bool               slotHeld = false;
    std::deque<BATCH>  queue;
    size_t             pushed = 0;
    size_t             consumed = 0;
    std::exception_ptr error;

    bool pumpMessages = false; ///< Windows only, the worker dispatches window messages whenever it waits

#ifdef _WIN32
    HANDLE wakeEvent = CreateEventW( nullptr, FALSE, FALSE, nullptr );

    ~DB_WORKER_CORE()
    {
        if( wakeEvent )
            CloseHandle( wakeEvent );
    }
#endif

    /// Wakes the owner and the worker, which may be waiting in either kind of wait
    void wake()
    {
        cv.notify_all();

#ifdef _WIN32
        if( pumpMessages && wakeEvent )
            SetEvent( wakeEvent );
#endif
    }

    /// Waits on the worker thread for aReady with m held, dispatching window messages meanwhile if asked to
    template <typename PRED>
    void workerWait( std::unique_lock<std::mutex>& aLock, PRED aReady )
    {
#ifdef _WIN32
        if( pumpMessages && wakeEvent )
        {
            while( !aReady() )
            {
                aLock.unlock();

                const DWORD waited =
                        MsgWaitForMultipleObjectsEx( 1, &wakeEvent, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE );

                // A failed wait would spin, so fall back to the plain condition wait
                if( waited == WAIT_FAILED )
                {
                    aLock.lock();
                    cv.wait( aLock, aReady );
                    return;
                }

                MSG message;

                while( PeekMessageW( &message, nullptr, 0, 0, PM_REMOVE ) )
                {
                    TranslateMessage( &message );
                    DispatchMessageW( &message );
                }

                aLock.lock();
            }

            return;
        }
#endif

        cv.wait( aLock, aReady );
    }

    /// Marks the running job as given up on, with m held by the caller
    void abandon()
    {
        if( !abandoned.exchange( true ) )
        {
            std::lock_guard registryLock( registry().m );
            ++registry().sources[key].abandoned;
            registry().changed.notify_all();
        }

        wake();
    }

    /// Returns the job's admission slot, with m held by the caller
    void releaseSlot()
    {
        if( !slotHeld )
            return;

        slotHeld = false;
        returnSlot( key );
    }

    /// The thread body; owns aCore until the thread ends
    static void Main( std::shared_ptr<DB_WORKER_CORE> aCore, std::function<bool()> aInit,
                      std::function<void()> aExit );
};


/// The part of a worker that outlives its owner while a caller is still inside Run
struct DB_WORKER_STATE
{
    std::mutex            m;
    std::atomic<bool>     stopping{ false };
    std::function<bool()> init;
    std::function<void()> exit;
    std::function<void()> onAbandon;

    std::shared_ptr<DB_WORKER_CORE> core;
    std::thread                     thread;
};


bool DB_JOB_CONTEXT::Cancelled() const
{
    return m_core.abandoned.load() || m_core.rejected.load();
}


bool DB_JOB_CONTEXT::pushBatch()
{
    std::unique_lock lock( m_core.m );

    m_core.workerWait( lock,
                       [&]()
                       {
                           return m_core.queue.size() < MAX_QUEUED_BATCHES || Cancelled();
                       } );

    if( Cancelled() )
    {
        m_pending.clear();
        return false;
    }

    m_core.queue.push_back( std::move( m_pending ) );
    m_pending.clear();
    m_pending.reserve( BATCH_ROWS );
    ++m_core.pushed;
    m_core.wake();

    return true;
}


bool DB_JOB_CONTEXT::Emit( std::vector<DB_VALUE>&& aRow )
{
    if( Cancelled() )
        return false;

    m_pending.emplace_back( std::move( aRow ) );

    return m_pending.size() < BATCH_ROWS || pushBatch();
}


bool DB_JOB_CONTEXT::Flush()
{
    if( !m_pending.empty() && !pushBatch() )
        return false;

    std::unique_lock lock( m_core.m );

    m_core.workerWait( lock,
                       [&]()
                       {
                           return m_core.consumed == m_core.pushed || Cancelled();
                       } );

    return !Cancelled();
}


void DB_WORKER_CORE::Main( std::shared_ptr<DB_WORKER_CORE> aCore, std::function<bool()> aInit,
                           std::function<void()> aExit )
{
    DB_WORKER_CORE& core = *aCore;
    bool            ready = true;

    t_onWorker = true;

    try
    {
        ready = !aInit || aInit();
    }
    catch( ... )
    {
        ready = false;
    }

    {
        std::lock_guard lock( core.m );

        core.initFailed = !ready;
        core.wake();
    }

    while( ready )
    {
        std::function<void( DB_JOB_CONTEXT& )> job;

        {
            std::unique_lock lock( core.m );

            core.workerWait( lock,
                             [&]()
                             {
                                 return core.hasJob || core.stop || core.abandoned;
                             } );

            // A job given up on before it began has no one waiting for it
            if( !core.hasJob || core.abandoned )
                break;

            job = std::move( core.job );
            core.hasJob = false;
        }

        std::exception_ptr error;

        {
            DB_JOB_CONTEXT context( core );

            try
            {
                job( context );

                // Rows a careless job left unflushed are still the caller's
                if( !context.m_pending.empty() )
                    context.pushBatch();
            }
            catch( ... )
            {
                error = std::current_exception();
            }
        }

        // Whatever the job owns is freed here, before the caller can see the job as finished
        job = nullptr;

        std::lock_guard lock( core.m );

        core.error = error;
        core.done = true;
        core.busy = false;

        // An abandoned job keeps its slot until the thread has returned
        if( !core.abandoned )
            core.releaseSlot();

        core.wake();

        if( core.abandoned )
            break;
    }

    try
    {
        if( ready && aExit )
            aExit();
    }
    catch( ... )
    {
    }

    std::lock_guard lock( core.m );

    core.releaseSlot();

    if( core.abandoned )
    {
        std::lock_guard registryLock( registry().m );

        --registry().sources[core.key].abandoned;
        eraseIfIdle( registry(), core.key );
        registry().changed.notify_all();
    }

    core.exited = true;
    core.wake();
}


DB_WORKER_THREAD::DB_WORKER_THREAD( std::string aSourceKey, std::function<bool()> aThreadInit,
                                    std::function<void()> aThreadExit, bool aPumpMessages,
                                    std::function<void()> aOnAbandon ) :
        m_key( std::move( aSourceKey ) ),
        m_pumpMessages( aPumpMessages ),
        m_state( std::make_shared<DB_WORKER_STATE>() )
{
    m_state->onAbandon = std::move( aOnAbandon );
    m_state->init = std::move( aThreadInit );
    m_state->exit = std::move( aThreadExit );
}


DB_WORKER_THREAD::~DB_WORKER_THREAD()
{
    std::shared_ptr<DB_WORKER_CORE> core;
    std::thread                     thread;

    {
        std::lock_guard lock( m_state->m );

        m_state->stopping = true;
        core = std::move( m_state->core );
        thread = std::move( m_state->thread );
    }

    // Callers waiting for a slot re-check the flag under the registry lock, so this wake cannot be lost
    {
        std::lock_guard registryLock( registry().m );
    }

    registry().changed.notify_all();

    if( !core )
        return;

    bool abandoned = false;

    // One deadline covers the job and the exit hook, which can block on a lock held elsewhere
    const auto deadline = std::chrono::steady_clock::now() + m_shutdownBound;

    {
        std::unique_lock lock( core->m );

        if( core->busy && !core->abandoned )
        {
            core->cv.wait_until( lock, deadline,
                                 [&]()
                                 {
                                     return !core->busy;
                                 } );

            if( core->busy )
                core->abandon();
        }

        core->stop = true;
        core->wake();

        if( !core->abandoned
            && !core->cv.wait_until( lock, deadline,
                                     [&]()
                                     {
                                         return core->exited;
                                     } ) )
        {
            core->abandon();
        }

        abandoned = core->abandoned;
    }

    if( !thread.joinable() )
        return;

    if( abandoned )
    {
        if( m_state->onAbandon )
            m_state->onAbandon();

        thread.detach();
    }
    else
    {
        thread.join();
    }
}


bool DB_WORKER_THREAD::IsHealthy() const
{
    std::lock_guard lock( m_state->m );

    return !m_state->core || ( !m_state->core->abandoned && !m_state->core->initFailed );
}


bool DB_WORKER_THREAD::HasLiveThread() const
{
    std::lock_guard lock( m_state->m );

    return m_state->core && !m_state->core->abandoned && !m_state->core->initFailed;
}


std::chrono::milliseconds DB_WORKER_THREAD::CallerBound( int aTimeoutSeconds )
{
    return std::chrono::seconds( DatabaseFileWaitSeconds( aTimeoutSeconds ) );
}


std::string DB_WORKER_THREAD::NormalizePath( const std::string& aPath, bool aWindowsPath )
{
    if( !aWindowsPath )
        return aPath;

    std::string path = aPath;

    std::replace( path.begin(), path.end(), '\\', '/' );

    auto startsWith = [&]( const char* aPrefix )
                      {
                          return path.rfind( aPrefix, 0 ) == 0;
                      };

    if( startsWith( "//?/UNC/" ) || startsWith( "//./UNC/" ) )
        return "//" + path.substr( 8 );

    if( startsWith( "//?/" ) || startsWith( "//./" ) )
        return path.substr( 4 );

    return path;
}


std::string DB_WORKER_THREAD::SourceKeyFor( const std::string& aPath, bool aWindowsPath )
{
    const std::string path = NormalizePath( aPath, aWindowsPath );
    std::string       root;
    size_t            start = 0;

    if( aWindowsPath && path.size() >= 2 && path[1] == ':' )
    {
        root = path.substr( 0, 2 );
        start = 2;

        if( path.size() > 2 && path[2] == '/' )
        {
            root += '/';
            start = 3;
        }
    }
    else if( !path.empty() && path[0] == '/' )
    {
        // A leading double slash is a UNC root on Windows and the same root as one slash elsewhere
        root = aWindowsPath && path.rfind( "//", 0 ) == 0 && path.size() > 2 && path[2] != '/' ? "//" : "/";
        start = path.find_first_not_of( '/' );

        if( start == std::string::npos )
            start = path.size();
    }

    std::vector<std::string> parts;

    for( size_t pos = start; pos <= path.size(); )
    {
        size_t end = path.find( '/', pos );

        if( end == std::string::npos )
            end = path.size();

        const std::string part = path.substr( pos, end - pos );

        if( part == ".." )
        {
            if( !parts.empty() && parts.back() != ".." )
                parts.pop_back();
            else if( root.empty() )
                parts.push_back( part );
        }
        else if( !part.empty() && part != "." )
        {
            parts.push_back( part );
        }

        pos = end + 1;
    }

    std::string key = root;

    for( size_t i = 0; i < parts.size(); ++i )
        key += ( i ? "/" : "" ) + parts[i];

    if( aWindowsPath )
    {
        for( char& c : key )
        {
            if( c >= 'A' && c <= 'Z' )
                c = static_cast<char>( c - 'A' + 'a' );
        }
    }

    return key;
}


std::string DB_WORKER_THREAD::SourceKeyFor( const std::string& aPath )
{
    namespace fs = std::filesystem;

#ifdef _WIN32
    constexpr bool windows = true;
#else
    constexpr bool windows = false;
#endif

    std::error_code ec;
    fs::path        path = fs::absolute( fs::path( std::u8string( aPath.begin(), aPath.end() ) ), ec );

    if( ec )
        path = fs::path( std::u8string( aPath.begin(), aPath.end() ) );

    const std::u8string text = path.generic_u8string();

    return SourceKeyFor( std::string( text.begin(), text.end() ), windows );
}


bool DB_WORKER_THREAD::OnWorkerThread()
{
    return t_onWorker;
}


bool DB_WORKER_THREAD::RunCleanup( std::function<void()> aCleanup, std::chrono::milliseconds aBound )
{
    return runJob(
            [cleanup = std::move( aCleanup )]( DB_JOB_CONTEXT& )
            {
                cleanup();
            },
            aBound, nullptr, true );
}


bool DB_WORKER_THREAD::runJob( std::function<void( DB_JOB_CONTEXT& )> aJob, std::chrono::milliseconds aBound,
                               const DB_ROW_SINK* aSink, bool aCleanup )
{
    // The destructor can end the admission wait, so after it only these local copies are used
    const std::shared_ptr<DB_WORKER_STATE> state = m_state;
    const std::shared_ptr<OUTCOME>         outcome = m_outcome;
    const std::string                      key = m_key;

    *outcome = OUTCOME::OK;

    // One deadline covers admission and the job, and only a delivered batch moves it
    auto deadline = std::chrono::steady_clock::now() + aBound;

    const bool slot = !aCleanup && acquireSlot( key, deadline, state->stopping );

    if( !aCleanup && !slot )
    {
        *outcome = state->stopping ? OUTCOME::UNAVAILABLE : OUTCOME::REFUSED;
        return false;
    }

    auto giveUpSlot = [&]()
                      {
                          if( slot )
                              returnSlot( key );
                      };

    // Waiting for a slot spent the bound, so a job started now would be abandoned almost at once
    if( slot && deadline - std::chrono::steady_clock::now() < aBound / 2 )
    {
        giveUpSlot();
        *outcome = OUTCOME::TOO_LATE;
        return false;
    }

    std::shared_ptr<DB_WORKER_CORE> core;

    {
        std::lock_guard stateLock( state->m );

        if( state->stopping )
        {
            giveUpSlot();
            *outcome = OUTCOME::UNAVAILABLE;
            return false;
        }

        if( !state->core || state->core->abandoned || state->core->initFailed )
        {
            if( state->thread.joinable() )
            {
                if( state->onAbandon && state->core && state->core->abandoned )
                    state->onAbandon();

                state->thread.detach();
            }

            state->core = std::make_shared<DB_WORKER_CORE>();
            state->core->key = key;
            state->core->pumpMessages = m_pumpMessages;

            try
            {
                state->thread = std::thread( &DB_WORKER_CORE::Main, state->core, state->init, state->exit );
            }
            catch( const std::system_error& )
            {
                state->core->initFailed = true;
                giveUpSlot();
                *outcome = OUTCOME::UNAVAILABLE;
                return false;
            }
        }

        core = state->core;
    }

    std::unique_lock lock( core->m );

    // The owner can have stopped the worker since the snapshot, and no thread would take the job
    if( core->stop )
    {
        lock.unlock();
        giveUpSlot();
        *outcome = OUTCOME::UNAVAILABLE;
        return false;
    }

    core->queue.clear();
    core->pushed = 0;
    core->consumed = 0;
    core->rejected = false;
    core->done = false;
    core->error = nullptr;
    core->job = std::move( aJob );
    core->slotHeld = slot;
    core->hasJob = true;
    core->busy = true;
    core->wake();

    for( ;; )
    {
        // Giving up wins over everything the job may have finished meanwhile
        if( core->abandoned )
        {
            *outcome = OUTCOME::TIMED_OUT;
            return false;
        }

        if( !core->queue.empty() )
        {
            if( std::chrono::steady_clock::now() >= deadline )
            {
                core->abandon();
                *outcome = OUTCOME::TIMED_OUT;
                return false;
            }

            DB_WORKER_CORE::BATCH batch = std::move( core->queue.front() );
            core->queue.pop_front();
            core->wake();

            lock.unlock();

            bool rejected = core->rejected;

            for( std::vector<DB_VALUE>& row : batch )
            {
                if( rejected || core->abandoned || !aSink || !*aSink )
                    break;

                try
                {
                    ( *aSink )( std::move( row ) );
                }
                catch( ... )
                {
                    rejected = true;
                }
            }

            lock.lock();

            if( core->abandoned )
            {
                *outcome = OUTCOME::TIMED_OUT;
                return false;
            }

            // Each delivered batch is progress, so the bound runs from here and excludes the sink
            deadline = std::chrono::steady_clock::now() + aBound;
            ++core->consumed;

            if( rejected )
                core->rejected = true;

            core->wake();
            continue;
        }

        if( core->done )
            break;

        if( core->initFailed )
        {
            core->hasJob = false;
            core->busy = false;
            core->job = nullptr;
            core->releaseSlot();
            *outcome = OUTCOME::UNAVAILABLE;
            return false;
        }

        if( core->cv.wait_until( lock, deadline ) == std::cv_status::timeout && core->queue.empty()
            && !core->done && !core->initFailed )
        {
            core->abandon();
            *outcome = OUTCOME::TIMED_OUT;
            return false;
        }
    }

    std::exception_ptr error = core->error;

    lock.unlock();

    if( error )
        std::rethrow_exception( error );

    return true;
}
