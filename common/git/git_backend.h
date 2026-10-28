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

#ifndef GIT_BACKEND_H_
#define GIT_BACKEND_H_

#include <map>
#include <import_export.h>
#include <set>
#include <utility>
#include <vector>
#include <wx/string.h>

#include "kigit_orphan_registry.h"

class GIT_CLONE_HANDLER;
class GIT_COMMIT_HANDLER;
class GIT_PUSH_HANDLER;
class GIT_STATUS_HANDLER;
class GIT_CONFIG_HANDLER;
class GIT_INIT_HANDLER;
class GIT_BRANCH_HANDLER;
class GIT_PULL_HANDLER;
class KIGIT_COMMON;
struct RemoteConfig;
struct git_repository;
struct git_oid;
enum class InitResult;
enum class BranchResult;
enum class PullResult;
struct FileStatus;
struct KIGIT_CONFLICT_SIDES;
struct KIGIT_ERROR_MESSAGE;
enum class RestoreResult;
enum class PushResult;

// Commit result shared across backend and handlers
enum class CommitResult
{
    Success,

    /// Committed, but the staged list was left as another program had it. Never retry on this.
    SuccessIndexStale,

    Error,
    Cancelled
};

// Why undoing the last commit did or did not move the branch
enum class UndoCommitResult
{
    Success,

    /// A remote-tracking branch already holds the commit, so taking it back would rewrite shared history.
    Published,

    /// The branch or its last commit is no longer the one the caller saw.
    TargetChanged,

    NoCommit,
    Error
};

// What finishing a merge did to the working files. A partial write leaves the recovery copies
// as the user's only route back, so the caller must keep them.
enum class MergeCommitResult
{
    Success,
    FailedCleanly,
    FailedPartially
};


class APIEXPORT GIT_BACKEND
{
public:
    virtual ~GIT_BACKEND() = default;

    virtual void Init() = 0;
    virtual void Shutdown() = 0;

    // Clone, Push, PerformFetch and PerformPull take the action lock themselves. Every other
    // method here expects the caller to hold it.

    /**
     * Return the process-wide orphan thread registry owned by this backend.
     *
     * Callers that abandon a long-running git operation must route their
     * cleanup thread through this registry so the shutdown path can wait for
     * libgit2 workers to finish before calling git_libgit2_shutdown().
     */
    KIGIT_ORPHAN_REGISTRY& OrphanRegistry() { return m_orphanRegistry; }

    // Whether the libgit2 library is available/initialized enough for use
    virtual bool IsLibraryAvailable() = 0;

    virtual bool Clone( GIT_CLONE_HANDLER* aHandler ) = 0;

    virtual CommitResult Commit( GIT_COMMIT_HANDLER* aHandler,
                                 const std::vector<wxString>& aFiles,
                                 const wxString& aMessage,
                                 const wxString& aAuthorName,
                                 const wxString& aAuthorEmail ) = 0;

    virtual CommitResult Amend( GIT_COMMIT_HANDLER* aHandler, const std::vector<wxString>& aFiles,
                                const wxString& aMessage, const wxString& aAuthorName,
                                const wxString& aAuthorEmail ) = 0;

    /// Move aExpectedBranch back to the parent of aExpectedHead, leaving the index and working files as they
    /// are. aMessage receives the undone commit's full message, or on Published the remote-tracking branch
    /// holding it.
    virtual UndoCommitResult UndoLastCommit( GIT_COMMIT_HANDLER* aHandler, const wxString& aExpectedBranch,
                                             const git_oid& aExpectedHead, wxString& aMessage ) = 0;

    virtual PushResult Push( GIT_PUSH_HANDLER* aHandler, bool aForce = false ) = 0;

    virtual std::map<wxString, FileStatus> GetFileStatus( GIT_STATUS_HANDLER* aHandler, const wxString& aPathspec,
                                                          bool aChangedOnly ) = 0;

    virtual wxString GetCurrentBranchName( GIT_STATUS_HANDLER* aHandler ) = 0;

    virtual void UpdateRemoteStatus( GIT_STATUS_HANDLER* aHandler,
                                     const std::set<wxString>& aLocalChanges,
                                     const std::set<wxString>& aRemoteChanges,
                                     std::map<wxString, FileStatus>& aFileStatus ) = 0;

    virtual wxString GetWorkingDirectory( GIT_STATUS_HANDLER* aHandler ) = 0;

    virtual bool GetConfigString( GIT_CONFIG_HANDLER* aHandler, const wxString& aKey,
                                  wxString& aValue ) = 0;

    virtual bool IsRepository( GIT_INIT_HANDLER* aHandler, const wxString& aPath ) = 0;
    virtual InitResult InitializeRepository( GIT_INIT_HANDLER* aHandler, const wxString& aPath ) = 0;
    virtual bool SetupRemote( GIT_INIT_HANDLER* aHandler, const RemoteConfig& aConfig ) = 0;

    virtual BranchResult SwitchToBranch( GIT_BRANCH_HANDLER* aHandler, const wxString& aBranchName ) = 0;

    virtual bool PerformFetch( GIT_PULL_HANDLER* aHandler, bool aSkipLock ) = 0;
    virtual PullResult PerformPull( GIT_PULL_HANDLER* aHandler ) = 0;

    /// aError receives why git refused to open the repository, and aTrustCommand the command that trusts it when
    /// another user owns it. Both are untouched otherwise.
    virtual git_repository* GetRepositoryForFile( const char* aFilename, wxString* aError,
                                                  wxString* aTrustCommand ) = 0;
    virtual int  CreateBranch( git_repository* aRepo, const wxString& aBranchName, const wxString& aFrom ) = 0;
    virtual bool RemoveVCS( git_repository*& aRepo, const wxString& aProjectPath,
                            bool aRemoveGitDir, wxString* aErrors ) = 0;

    virtual bool DeleteBranch( git_repository* aRepo, const wxString& aBranchName ) = 0;

    virtual bool StashSave( git_repository* aRepo, const wxString& aMessage ) = 0;
    virtual bool StashApply( git_repository* aRepo, size_t aIndex ) = 0;
    virtual bool StashPop( git_repository* aRepo, size_t aIndex ) = 0;
    virtual bool StashDrop( git_repository* aRepo, size_t aIndex ) = 0;

    virtual bool CreateTag( git_repository* aRepo, const wxString& aName, const wxString& aMessage ) = 0;
    virtual bool DeleteTag( git_repository* aRepo, const wxString& aName ) = 0;
    virtual bool CheckoutTag( git_repository* aRepo, const wxString& aName ) = 0;

    virtual bool AddRemote( git_repository* aRepo, const wxString& aName, const wxString& aUrl ) = 0;
    virtual bool SetRemoteUrl( git_repository* aRepo, const wxString& aName, const wxString& aUrl ) = 0;
    virtual bool RemoveRemote( git_repository* aRepo, const wxString& aName ) = 0;

    virtual bool SetUserConfig( git_repository* aRepo, const wxString& aName, const wxString& aEmail ) = 0;

    virtual RestoreResult RestoreVersion( KIGIT_COMMON& aCommon, const wxString& aCommitHash,
                                          KIGIT_ERROR_MESSAGE& aError ) = 0;

    virtual bool DiscardChanges( git_repository* aRepo, const std::vector<wxString>& aPaths, wxString& aError ) = 0;

    virtual bool IsPathInHead( git_repository* aRepo, const wxString& aPath ) = 0;

    /// Drop a merge, rebase, cherry-pick or revert left in progress and return to the last commit.
    virtual bool AbandonOperation( git_repository* aRepo, wxString& aError ) = 0;

    virtual bool GetMergeConflicts( git_repository* aRepo, const wxString& aStageDir, wxString& aHeadOid,
                                    wxString& aTheirsOid, std::vector<KIGIT_CONFLICT_SIDES>& aConflicts,
                                    wxString& aError ) = 0;

    virtual MergeCommitResult CreateMergedCommit( git_repository* aRepo, const wxString& aExpectedHeadOid,
                                                  const wxString&                                   aTheirsOid,
                                                  const std::vector<std::pair<wxString, wxString>>& aResolvedFiles,
                                                  KIGIT_ERROR_MESSAGE&                              aError ) = 0;

protected:
    KIGIT_ORPHAN_REGISTRY m_orphanRegistry;
};

APIEXPORT GIT_BACKEND* GetGitBackend();
APIEXPORT void SetGitBackend( GIT_BACKEND* aBackend );

#endif
