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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef GIT_TRANSFER_H
#define GIT_TRANSFER_H

#include <functional>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <git/kicad_git_common.h>
#include <widgets/progress_reporter_base.h>

namespace KIGIT
{

/**
 * Run aFn on the UI thread.
 *
 * For a transfer that outlives the window that started it. The work is queued on the
 * application rather than on that window, because the window may be part way through its own
 * destructor, and queueing onto a handler being destroyed is a race.
 */
void CallOnUi( std::function<void()> aFn );

/// As CallOnUi, for work that touches the window that started the transfer. aWatch is locked
/// on the UI thread, so a window that has gone is never touched.
void CallIfAlive( const std::weak_ptr<bool>& aWatch, std::function<void()> aFn );

/**
 * Run aWork on a thread the orphan registry owns.
 *
 * The registry joins with a bounded timeout at shutdown and detaches whatever has not
 * finished, so a window closing mid transfer never waits for the socket. False when no
 * thread could be started, which only happens once shutdown has begun.
 */
bool StartTransfer( const std::string& aLabel, std::function<void()> aWork );

} // namespace KIGIT


/**
 * One background git transfer owned by the window that started it: the cancellation, whether
 * one is running, and the owner's lifetime.
 *
 * Push, pull, fetch and force push each used to keep this in a shape of their own, and the
 * disagreement is how one of them came to read a finished thread as a running one. The
 * transfer itself runs on a thread the orphan registry owns, so closing the window does not
 * wait for the socket, and the completion is delivered only while this object still exists.
 */
/**
 * A progress reporter that writes a line of text instead of opening a dialog over the editor.
 * libgit2's callbacks drive it from a worker, so each line is handed to the UI thread and
 * dropped once the window that started the transfer has gone.
 *
 * aShow must be safe to copy and destroy on a worker, so it may hold a raw window pointer but
 * never a wxWeakRef. It runs only on the UI thread and only while aAlive holds.
 */
class KIGIT_STATUS_REPORTER : public PROGRESS_REPORTER_BASE
{
public:
    KIGIT_STATUS_REPORTER( const std::weak_ptr<bool>& aAlive, std::function<void( const wxString& )> aShow );

protected:
    bool updateUI() override;

private:
    std::weak_ptr<bool>                    m_alive;
    std::function<void( const wxString& )> m_show;
    wxString                               m_lastShown;
};


class KIGIT_TRANSFER
{
public:
    KIGIT_TRANSFER() = default;
    ~KIGIT_TRANSFER();

    KIGIT_TRANSFER( const KIGIT_TRANSFER& ) = delete;
    KIGIT_TRANSFER& operator=( const KIGIT_TRANSFER& ) = delete;

    /// True while a transfer started here is still running.
    bool IsBusy() const { return m_busy; }

    /// Dropped when this object goes. Hand it to a reporter so its updates stop with it.
    std::weak_ptr<bool> Alive() const { return m_alive; }

    /// Ask the running transfer to stop. It finishes on its own time.
    void Cancel();

    /**
     * Run aWork on a background thread, then aDone on the UI thread.
     *
     * aWork is given the cancellation to hand its handler. aDone runs once aWork has returned,
     * and only while this object is still alive, so it may touch the owning window freely.
     * aWork captures must be safe to copy and destroy on a worker. UI references belong only
     * in aDone, including when captures share a context object.
     * False when no thread could be started, which only happens once shutdown has begun.
     */
    bool Start( const std::string& aLabel, const std::function<void( const KIGIT_COMMON::CANCEL_TOKEN& )>& aWork,
                const std::function<void()>& aDone );

private:
    /// Dropped when this object goes, so a transfer that outlives it completes into nothing.
    std::shared_ptr<bool> m_alive = std::make_shared<bool>( true );

    /// The token of the transfer started last. Held so a cancel reaches it even while the task
    /// is still queued and has no handler yet.
    KIGIT_COMMON::CANCEL_TOKEN m_cancel;

    /// Created and cleared on the UI thread. The worker carries only this shared holder, so
    /// captured wxWeakRefs are never copied or destroyed on the worker, even during shutdown.
    std::shared_ptr<std::function<void()>> m_completion;

    bool m_busy = false;
};


/**
 * Read repository state without blocking the UI. All methods are called on the UI thread.
 *
 * One request runs at a time. A newer request replaces any pending request and suppresses the
 * running request's completion, so a slow read cannot overwrite a newer selection. Closing
 * the owner cancels pending work and never waits for the worker.
 */
class KIGIT_QUERY
{
public:
    KIGIT_QUERY() = default;
    ~KIGIT_QUERY();

    KIGIT_QUERY( const KIGIT_QUERY& ) = delete;
    KIGIT_QUERY& operator=( const KIGIT_QUERY& ) = delete;

    /**
     * Run aWork under the session's action lock on a worker, then aDone on the UI thread.
     *
     * aWork must capture its inputs and result storage by value and must not touch windows.
     * The session stays alive until the worker returns. If it has no repository or is shutting
     * down, aWork is skipped and aDone can display the result's initial, empty state.
     * Exceptions are reported on the UI thread and still deliver aDone. Build each result
     * before assigning it to shared storage so an exception leaves that result empty.
     */
    void Run( std::shared_ptr<KIGIT_COMMON> aSession, std::function<void( KIGIT_COMMON& )> aWork,
              std::function<void()> aDone );

    /// Suppress the running completion and discard pending work. Does not wait for a read.
    void Cancel();

    /// Remains true until the running worker returns, including after Cancel().
    bool IsBusy() const { return m_transfer.IsBusy(); }

private:
    struct REQUEST
    {
        std::shared_ptr<KIGIT_COMMON>        session;
        std::function<void( KIGIT_COMMON& )> work;
        std::function<void()>                done;
        uint64_t                             generation;
    };

    void start( REQUEST aRequest );

    KIGIT_TRANSFER         m_transfer;
    std::optional<REQUEST> m_pending;
    uint64_t               m_generation = 0;
};

#endif // GIT_TRANSFER_H
