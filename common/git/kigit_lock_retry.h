/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file kigit_lock_retry.h
 * @brief Retrying short git writes that lost a race with another process.
 *
 * libgit2 fails at once on a held lock where the git CLI waits. An operation is rerun whole, so it must
 * reread whatever the lock holder may have changed.
 */

#ifndef KIGIT_LOCK_RETRY_H
#define KIGIT_LOCK_RETRY_H

#include <import_export.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

struct git_index;
struct git_repository;

namespace KIGIT
{

/// Budget for a backend call, which never runs on the UI thread
constexpr std::chrono::milliseconds BACKEND_LOCK_BUDGET{ 5000 };

/**
 * True when aGitError says another writer held a lock or moved a ref first. On Windows a lock
 * commit can also fail with a sharing violation from a reader holding the file open, which
 * libgit2 reports as a plain OS error.
 */
bool IsLockContention( int aGitError );

/**
 * Run aOp, which returns 0 or a libgit2 error code, again after each lock contention until it succeeds,
 * fails otherwise, or aBudget is spent. Returns the last result.
 */
APIEXPORT int WithLockRetry( std::chrono::milliseconds aBudget, const std::function<int()>& aOp );

/**
 * A process's lock on $GIT_COMMON_DIR/kicad-action, held while it rewrites the files, the index or HEAD in
 * more than one step, so no other process sees a branch switch half done. The OS releases it with the
 * process. A network filesystem may not honour it.
 */
class APIEXPORT REPO_ACTION_LOCK
{
public:
    /// aCommonDir is as git_repository_commondir gives it. aName picks an independent lock in that directory.
    explicit REPO_ACTION_LOCK( const std::filesystem::path& aCommonDir, const char* aName = "kicad-action" );
    ~REPO_ACTION_LOCK();

    REPO_ACTION_LOCK( const REPO_ACTION_LOCK& ) = delete;
    REPO_ACTION_LOCK& operator=( const REPO_ACTION_LOCK& ) = delete;

    /// Try once without waiting. 0 once held, GIT_ELOCKED while another holder has it, GIT_ERROR otherwise
    int TryAcquire();

    /// TryAcquire again after each contention until it is held, fails otherwise, or aBudget is spent
    int Acquire( std::chrono::milliseconds aBudget = BACKEND_LOCK_BUDGET );

    bool Held() const { return m_held; }

    /// Why the last TryAcquire could not use the lock file, or empty when it could
    const std::string& Error() const { return m_error; }

    void Release();

private:
    std::filesystem::path m_path;
    int                   m_fd = -1;
    bool                  m_held = false;
    std::string           m_error;
};

/**
 * Rewrite aRepo's index the way git does, holding index.lock from a fresh read of the file to the rename that
 * publishes it, so staging by another writer is never overwritten. aEdit changes the fresh copy it is handed, and
 * any result but 0 leaves the index alone and is returned. GIT_ELOCKED while another writer holds the lock. The
 * repository's own index is reread afterwards.
 */
int EditIndexLocked( git_repository* aRepo, const std::function<int( git_index* aIndex )>& aEdit );

/// The lock aName in aRepo's common git directory, which every worktree of the repository shares. Null when
/// aRepo has no git directory.
APIEXPORT std::unique_ptr<REPO_ACTION_LOCK> RepoActionLock( git_repository* aRepo, const char* aName = "kicad-action" );

} // namespace KIGIT

#endif // KIGIT_LOCK_RETRY_H
