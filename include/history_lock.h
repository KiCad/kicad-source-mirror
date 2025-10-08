/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef HISTORY_LOCK_H
#define HISTORY_LOCK_H

#include <memory>
#include <wx/string.h>

struct git_repository;
struct git_index;
class LOCKFILE;

/**
 * Hybrid locking mechanism for local history git repositories.
 *
 * Implements a two-layer locking strategy:
 * - Layer 1: File-based lock (prevents cross-instance conflicts)
 * - Layer 2: Git index lock (prevents concurrent git operations)
 *
 * This provides defense-in-depth protection against:
 * - Multiple KiCad instances accessing same project
 * - Multi-threaded operations within same instance
 * - Repository corruption from concurrent writes
 *
 * Usage:
 * @code
 *   HISTORY_LOCK_MANAGER lock( projectPath );
 *   if( !lock.IsLocked() )
 *   {
 *       wxLogError( "Cannot acquire lock: %s", lock.GetLockError() );
 *       return false;
 *   }
 *   git_repository* repo = lock.GetRepository();
 *   git_index* index = lock.GetIndex();
 *   // ... perform git operations ...
 *   // Lock automatically released when object goes out of scope
 * @endcode
 */
class HISTORY_LOCK_MANAGER
{
public:
    /**
     * Construct a lock manager and attempt to acquire locks.
     *
     * @param aProjectPath Path to the KiCad project directory
     * @param aStaleTimeoutSec Timeout in seconds after which a lock is considered stale
     *                         and can be forcibly removed. If <= 0, uses the value from
     *                         ADVANCED_CFG::m_HistoryLockStaleTimeout (default: 0 = use config)
     */
    HISTORY_LOCK_MANAGER( const wxString& aProjectPath, int aStaleTimeoutSec = 0 );

    /**
     * Destructor releases all locks and closes git repository.
     */
    ~HISTORY_LOCK_MANAGER();

    // Non-copyable
    HISTORY_LOCK_MANAGER( const HISTORY_LOCK_MANAGER& ) = delete;
    HISTORY_LOCK_MANAGER& operator=( const HISTORY_LOCK_MANAGER& ) = delete;

    /**
     * Check if locks were successfully acquired.
     *
     * @return true if both file lock and git repository are accessible
     */
    bool IsLocked() const;

    /**
     * Get the git repository handle (only valid if IsLocked() returns true).
     *
     * @return Pointer to git_repository, or nullptr if not locked
     */
    git_repository* GetRepository() { return m_repo; }

    /**
     * Get the git index handle (only valid if IsLocked() returns true).
     *
     * @return Pointer to git_index, or nullptr if not locked
     */
    git_index* GetIndex() { return m_index; }

    /**
     * Get error message describing why lock could not be acquired.
     *
     * @return Human-readable error message with details about lock holder
     */
    wxString GetLockError() const;

    /**
     * Get information about who currently holds the lock.
     *
     * @return String in format "username@hostname" or empty if lock is held by current process
     */
    wxString GetLockHolder() const;

    /**
     * Check if a lock file exists and is stale (older than timeout).
     * This does not acquire the lock, just checks its status.
     *
     * @param aProjectPath Path to project directory
     * @param aStaleTimeoutSec Timeout in seconds. If <= 0, uses the value from
     *                         ADVANCED_CFG::m_HistoryLockStaleTimeout (default: 0 = use config)
     * @return true if lock exists and is stale
     */
    static bool IsLockStale( const wxString& aProjectPath, int aStaleTimeoutSec = 0 );

    /**
     * Forcibly remove a stale lock file.
     * Should only be called after confirming with user or if IsLockStale() returns true.
     *
     * @param aProjectPath Path to project directory
     * @return true if lock was removed successfully
     */
    static bool BreakStaleLock( const wxString& aProjectPath );

private:
    wxString                      m_projectPath;
    wxString                      m_historyPath;
    std::unique_ptr<LOCKFILE>     m_fileLock;
    git_repository*               m_repo;
    git_index*                    m_index;
    bool                          m_repoOwned;
    bool                          m_indexOwned;
    wxString                      m_lockError;
    int                           m_staleTimeoutSec;

    bool acquireFileLock();
    bool openRepository();
    bool acquireIndexLock();
};

#endif // HISTORY_LOCK_H
