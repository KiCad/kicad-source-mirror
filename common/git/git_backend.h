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

#include <cstdint>
#include <functional>
#include <map>
#include <import_export.h>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <wx/string.h>

#include "kigit_orphan_registry.h"

class GIT_CLONE_HANDLER;
class GIT_COMMIT_HANDLER;
class GIT_PUSH_HANDLER;
class GIT_STATUS_HANDLER;
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
    Cancelled,

    /// Nothing differs from the last commit, so none was made. Only when the handler asks for it.
    Unchanged,

    /// Another writer kept the branch or index locked past the handler's retry budget.
    Busy
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


/// Polled by long history walks; true once their caller gave up on them
using KIGIT_CANCEL = std::function<bool()>;


/// One commit as a path history reports it. Ids are full hex.
struct KIGIT_PATH_COMMIT
{
    std::string              oid;
    std::vector<std::string> parents;
    std::string              author;
    std::int64_t             time = 0;
    std::string              summary;
};


/// One commit read whole. Ids are full hex.
struct KIGIT_COMMIT_RECORD
{
    std::string              oid;
    std::vector<std::string> parents;
    std::string              authorName;
    std::string              authorEmail;
    std::int64_t             time = 0;
    std::string              message;
};


/// Where one repository-relative path stands
struct KIGIT_PATH_PLACE
{
    bool inHead = false;  ///< A file in HEAD's tree
    bool inIndex = false; ///< Staged at stage 0
};


/// One entry of a commit's tree. Paths are repository-relative and '/'-separated.
struct KIGIT_TREE_ENTRY
{
    std::string   path;
    std::string   oid;
    std::uint32_t mode = 0;
    bool          isTree = false;
};


/// A reference and the commit or tag it resolves to
struct KIGIT_REFERENCE
{
    std::string name;
    std::string oid;
};


struct KIGIT_TAG_RECORD
{
    std::string  oid;
    std::string  target; ///< The object the tag points at
    std::string  message;
    std::string  taggerName;
    std::string  taggerEmail;
    std::int64_t time = 0;
};


/// What a tree walk does after visiting one entry
enum class KIGIT_WALK
{
    CONTINUE,
    SKIP, ///< Do not descend into this folder
    STOP
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
                                                          bool aChangedOnly, bool* aReadOk = nullptr ) = 0;

    virtual wxString GetCurrentBranchName( GIT_STATUS_HANDLER* aHandler ) = 0;

    virtual void UpdateRemoteStatus( GIT_STATUS_HANDLER* aHandler,
                                     const std::set<wxString>& aLocalChanges,
                                     const std::set<wxString>& aRemoteChanges,
                                     std::map<wxString, FileStatus>& aFileStatus ) = 0;

    virtual wxString GetWorkingDirectory( GIT_STATUS_HANDLER* aHandler ) = 0;

    virtual bool GetConfigString( git_repository* aRepo, const wxString& aKey, wxString& aValue ) = 0;
    /// Set aKey in aRepo's own config, never in a level shared with other repositories
    virtual bool SetConfigString( git_repository* aRepo, const wxString& aKey, const wxString& aValue ) = 0;

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

    // Paths below are repository-relative, '/'-separated UTF-8, and ids are hex. A commit id may be
    // abbreviated but is never revision syntax, so a caller cannot steer a lookup through refs.

    /// Every path the index holds at stage 0, or nothing when it cannot be read.
    virtual std::optional<std::vector<std::string>> IndexedPaths( git_repository* aRepo ) = 0;

    /// Where each of aPaths stands, reading HEAD's tree and the index once for all of them.
    virtual std::vector<KIGIT_PATH_PLACE> LocatePaths( git_repository*                 aRepo,
                                                       const std::vector<std::string>& aPaths ) = 0;

    /**
     * The newest commit on HEAD that changed each path, as `git log -1 -- <path>` picks it. A path may
     * name a folder, and the empty path names the whole tree. A path missing from the result has no
     * such commit, or the walk was cancelled first.
     */
    virtual std::map<std::string, KIGIT_PATH_COMMIT>
    LastChanges( git_repository* aRepo, const std::vector<std::string>& aPaths, const KIGIT_CANCEL& aCancelled ) = 0;

    /// The commits on HEAD that changed aPath, a file, a folder or the whole tree when empty, newest first, as
    /// `git log -- <path>` lists them.
    virtual std::vector<KIGIT_PATH_COMMIT> PathHistory( git_repository* aRepo, const std::string& aPath,
                                                        const KIGIT_CANCEL& aCancelled ) = 0;

    /// The entry at aPath in commit aCommit's tree, the tree itself for an empty path, or nothing when either
    /// is missing.
    virtual std::optional<KIGIT_TREE_ENTRY> TreeEntryAt( git_repository* aRepo, const std::string& aCommit,
                                                         const std::string& aPath ) = 0;

    /// The raw content of blob aOid, or nothing when it cannot be read.
    virtual std::optional<std::string> ReadBlob( git_repository* aRepo, const std::string& aOid ) = 0;

    /**
     * Visit every entry below aFolder (the root when empty) of commit aCommit's tree, each folder before
     * its contents. 0 once all were visited, GIT_EUSER when aVisit stopped it, another libgit2 error when
     * the commit or a folder could not be read.
     */
    virtual int WalkTree( git_repository* aRepo, const std::string& aCommit, const std::string& aFolder,
                          const std::function<KIGIT_WALK( const KIGIT_TREE_ENTRY& )>& aVisit ) = 0;

    /// aText for the file at aPath through the repository's line-ending filters, toward the object store
    /// when aToRepository and toward the working tree otherwise. Nothing when a filter fails.
    virtual std::optional<std::string> FilterText( git_repository* aRepo, const std::string& aPath,
                                                   const std::string& aText, bool aToRepository ) = 0;

    /// As FilterText for each path and text of aTexts, with the attributes commit aCommit holds rather than
    /// those in the working tree. A path whose filter fails maps to nothing.
    virtual std::map<std::string, std::optional<std::string>>
    FilterTextsAt( git_repository* aRepo, const std::string& aCommit, const std::map<std::string, std::string>& aTexts,
                   bool aToRepository ) = 0;

    /// The best common ancestor of commits aOne and aTwo, or nothing when they share no history.
    virtual std::optional<std::string> MergeBase( git_repository* aRepo, const std::string& aOne,
                                                  const std::string& aTwo ) = 0;

    /// Every path whose entry differs between commit aFrom's tree and commit aTo's, or the index when aTo
    /// is empty. A rename lists both names. Nothing when a tree or the index cannot be read.
    virtual std::optional<std::vector<std::string>> ChangedPaths( git_repository* aRepo, const std::string& aFrom,
                                                                  const std::string& aTo ) = 0;

    /// The id git gives a blob holding aContent, without writing it.
    virtual std::string BlobId( const std::string& aContent ) = 0;

    /// Whether aAncestor is a strict ancestor of aCommit; nothing when either cannot be read.
    virtual std::optional<bool> IsDescendant( git_repository* aRepo, const std::string& aCommit,
                                              const std::string& aAncestor ) = 0;

    /// Commit aOid read whole, or nothing when it is not a commit.
    virtual std::optional<KIGIT_COMMIT_RECORD> ReadCommit( git_repository* aRepo, const std::string& aOid ) = 0;

    /**
     * Visit the commits reachable from reference aRef and not from commit aHide (none when empty), newest
     * first and children before parents, until aVisit answers false. 0 once all were visited, GIT_EUSER
     * when aVisit stopped it, another libgit2 error when aRef or aHide cannot be read.
     */
    virtual int WalkCommits( git_repository* aRepo, const std::string& aRef, const std::string& aHide,
                             const std::function<bool( const KIGIT_COMMIT_RECORD& )>& aVisit ) = 0;

    /// The id a reference resolves to, following symbolic ones, or nothing when it does not.
    virtual std::optional<std::string> ReadReference( git_repository* aRepo, const std::string& aName ) = 0;

    /// The ref HEAD names, an unborn branch included, "HEAD" itself when detached, or empty when it cannot be read
    virtual std::string HeadTarget( git_repository* aRepo ) = 0;

    /**
     * Point aName at aOid. With aExpected the move happens under the ref lock only while aName still names
     * that id, or is absent when aExpected is empty. 0 on success, GIT_EMODIFIED when aExpected no longer
     * holds, another libgit2 error otherwise.
     */
    virtual int SetReference( git_repository* aRepo, const std::string& aName, const std::string& aOid,
                              const std::optional<std::string>& aExpected, const std::string& aLogMessage ) = 0;

    /// Remove aName, and with aExpected only while it still names that id. 0 when it is gone or was never
    /// there, GIT_EMODIFIED when it names another id, another libgit2 error otherwise.
    virtual int DeleteReference( git_repository* aRepo, const std::string& aName,
                                 const std::optional<std::string>& aExpected ) = 0;

    /// Every reference whose full name matches aGlob, such as "refs/tags/v*".
    virtual std::vector<KIGIT_REFERENCE> ListReferences( git_repository* aRepo, const std::string& aGlob ) = 0;

    /// A parentless commit of the empty tree holding aMessage, on no branch. Its id, or nothing on failure.
    virtual std::optional<std::string> CreateRecordCommit( git_repository* aRepo, const std::string& aMessage,
                                                           const std::string& aAuthorName,
                                                           const std::string& aAuthorEmail ) = 0;

    /**
     * Tag commit aTarget as refs/tags/<aName> with an annotated tag carrying aMessage. An existing tag is
     * never replaced. 0 with aTagOid set on success, GIT_EEXISTS when the name is taken, another libgit2
     * error otherwise.
     */
    virtual int CreateAnnotatedTag( git_repository* aRepo, const std::string& aName, const std::string& aTarget,
                                    const std::string& aMessage, const std::string& aTaggerName,
                                    const std::string& aTaggerEmail, std::string& aTagOid ) = 0;

    /// The annotated tag reference aRefName points at, or nothing for a lightweight or missing one.
    virtual std::optional<KIGIT_TAG_RECORD> ReadAnnotatedTag( git_repository* aRepo, const std::string& aRefName ) = 0;

    /// The repository whose working folder, or git folder when bare, is exactly aPath, never one above it.
    /// The caller frees it. Nothing when there is none.
    virtual git_repository* OpenRepositoryAt( const std::string& aPath ) = 0;

    /// The repository whose working tree or git folder holds aPath, searching up and across file systems.
    /// 0 with aRepo set, which the caller frees, GIT_ENOTFOUND when there is none, another libgit2 error otherwise.
    virtual int FindRepository( const std::string& aPath, git_repository*& aRepo ) = 0;

    /**
     * Check commit aCommit out safely, then point refs/heads/<aBranch> at it and HEAD at that branch, or
     * detach HEAD there when aBranch is empty. The checkout overwrites no edited, untracked or ignored file.
     * 0 on success, GIT_ECONFLICT naming the first such file in the libgit2 error when one is in the way,
     * another libgit2 error otherwise. When HEAD cannot move after the checkout, the index is read back from
     * HEAD so nothing is left staged.
     */
    virtual int CheckoutCommit( git_repository* aRepo, const std::string& aCommit, const std::string& aBranch ) = 0;

    /// Make the index hold commit aCommit's tree and write it. 0 or a libgit2 error.
    virtual int ResetIndex( git_repository* aRepo, const std::string& aCommit ) = 0;

    /// Write aPaths to the working tree as the index on disk holds them, as `git checkout -- <path>` does,
    /// leaving the index alone. Each path is literal, never a pattern. 0, GIT_ENOTFOUND when a path has
    /// nothing staged, which leaves every file alone, or another libgit2 error.
    virtual int CheckoutFromIndex( git_repository* aRepo, const std::vector<std::string>& aPaths ) = 0;

    /**
     * Under the index lock, stage each of aPaths as commit aTo holds it, or unstage it where aTo lacks it. A path
     * moves only while it is staged as commit aFrom holds it, mode included, or is not staged where aFrom lacks it.
     * 0, GIT_ELOCKED while another writer holds the index, or another libgit2 error, which leaves the index alone.
     */
    virtual int MoveStaging( git_repository* aRepo, const std::string& aFrom, const std::string& aTo,
                             const std::vector<std::string>& aPaths ) = 0;

    /**
     * Commit aFiles on top of commit aParent's tree, each path holding its text or removed when it has none,
     * and move refs/heads/<aBranch> to it only while the branch still names aParent. A path whose index entry
     * still matches aParent, mode included, then follows the commit there, under the index lock. 0 with aCommit
     * set on success, GIT_EMODIFIED when the branch moved, another libgit2 error otherwise.
     */
    virtual int CommitFiles( git_repository* aRepo, const std::string& aBranch, const std::string& aParent,
                             const std::map<std::string, std::optional<std::string>>& aFiles,
                             const std::string& aMessage, const std::string& aAuthorName,
                             const std::string& aAuthorEmail, std::string& aCommit ) = 0;

protected:
    KIGIT_ORPHAN_REGISTRY m_orphanRegistry;
};

APIEXPORT GIT_BACKEND* GetGitBackend();
APIEXPORT void SetGitBackend( GIT_BACKEND* aBackend );

#endif
