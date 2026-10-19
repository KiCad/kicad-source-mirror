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
#include <memory>
#include <string>

#include <git/kicad_git_common.h>

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
class KIGIT_TRANSFER
{
public:
    KIGIT_TRANSFER() = default;
    ~KIGIT_TRANSFER();

    KIGIT_TRANSFER( const KIGIT_TRANSFER& ) = delete;
    KIGIT_TRANSFER& operator=( const KIGIT_TRANSFER& ) = delete;

    /// True while a transfer started here is still running.
    bool IsBusy() const { return m_busy; }

    /// Ask the running transfer to stop. It finishes on its own time.
    void Cancel();

    /**
     * Run aWork on a background thread, then aDone on the UI thread.
     *
     * aWork is given the cancellation to hand its handler. aDone runs once aWork has returned,
     * and only while this object is still alive, so it may touch the owning window freely.
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

    bool m_busy = false;
};

#endif // GIT_TRANSFER_H
