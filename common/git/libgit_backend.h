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

#ifndef LIBGIT_BACKEND_H_
#define LIBGIT_BACKEND_H_

#include "git_backend.h"
#include <import_export.h>

// Forward declarations to avoid exposing libgit2 headers
struct git_annotated_commit;

class APIEXPORT LIBGIT_BACKEND : public GIT_BACKEND
{
public:
    void Init() override;
    void Shutdown() override;
    bool IsLibraryAvailable() override;

    bool Clone( GIT_CLONE_HANDLER* aHandler ) override;

    CommitResult Commit( GIT_COMMIT_HANDLER* aHandler,
                         const std::vector<wxString>& aFiles,
                         const wxString& aMessage,
                         const wxString& aAuthorName,
                         const wxString& aAuthorEmail ) override;

    CommitResult Amend( GIT_COMMIT_HANDLER* aHandler, const std::vector<wxString>& aFiles, const wxString& aMessage,
                        const wxString& aAuthorName, const wxString& aAuthorEmail ) override;

    UndoCommitResult UndoLastCommit( GIT_COMMIT_HANDLER* aHandler, const wxString& aExpectedBranch,
                                     const git_oid& aExpectedHead, wxString& aMessage ) override;

    PushResult Push( GIT_PUSH_HANDLER* aHandler, bool aForce = false ) override;

    std::map<wxString, FileStatus> GetFileStatus( GIT_STATUS_HANDLER* aHandler, const wxString& aPathspec,
                                                  bool aChangedOnly, bool* aReadOk = nullptr ) override;

    wxString GetCurrentBranchName( GIT_STATUS_HANDLER* aHandler ) override;

    void UpdateRemoteStatus( GIT_STATUS_HANDLER* aHandler,
                             const std::set<wxString>& aLocalChanges,
                             const std::set<wxString>& aRemoteChanges,
                             std::map<wxString, FileStatus>& aFileStatus ) override;

    wxString GetWorkingDirectory( GIT_STATUS_HANDLER* aHandler ) override;

    bool GetConfigString( git_repository* aRepo, const wxString& aKey, wxString& aValue ) override;
    bool SetConfigString( git_repository* aRepo, const wxString& aKey, const wxString& aValue ) override;

    bool IsRepository( GIT_INIT_HANDLER* aHandler, const wxString& aPath ) override;
    InitResult InitializeRepository( GIT_INIT_HANDLER* aHandler, const wxString& aPath ) override;
    bool SetupRemote( GIT_INIT_HANDLER* aHandler, const RemoteConfig& aConfig ) override;

    BranchResult SwitchToBranch( GIT_BRANCH_HANDLER* aHandler, const wxString& aBranchName ) override;

    bool PerformFetch( GIT_PULL_HANDLER* aHandler, bool aSkipLock ) override;

    PullResult PerformPull( GIT_PULL_HANDLER* aHandler ) override;

    git_repository* GetRepositoryForFile( const char* aFilename, wxString* aError, wxString* aTrustCommand ) override;
    int             CreateBranch( git_repository* aRepo, const wxString& aBranchName, const wxString& aFrom ) override;
    bool RemoveVCS( git_repository*& aRepo, const wxString& aProjectPath,
                    bool aRemoveGitDir, wxString* aErrors ) override;

    bool DeleteBranch( git_repository* aRepo, const wxString& aBranchName ) override;

    bool StashSave( git_repository* aRepo, const wxString& aMessage ) override;
    bool StashApply( git_repository* aRepo, size_t aIndex ) override;
    bool StashPop( git_repository* aRepo, size_t aIndex ) override;
    bool StashDrop( git_repository* aRepo, size_t aIndex ) override;

    bool CreateTag( git_repository* aRepo, const wxString& aName, const wxString& aMessage ) override;
    bool DeleteTag( git_repository* aRepo, const wxString& aName ) override;
    bool CheckoutTag( git_repository* aRepo, const wxString& aName ) override;

    bool AddRemote( git_repository* aRepo, const wxString& aName, const wxString& aUrl ) override;
    bool SetRemoteUrl( git_repository* aRepo, const wxString& aName, const wxString& aUrl ) override;
    bool RemoveRemote( git_repository* aRepo, const wxString& aName ) override;

    bool SetUserConfig( git_repository* aRepo, const wxString& aName, const wxString& aEmail ) override;

    RestoreResult RestoreVersion( KIGIT_COMMON& aCommon, const wxString& aCommitHash,
                                  KIGIT_ERROR_MESSAGE& aError ) override;

    bool DiscardChanges( git_repository* aRepo, const std::vector<wxString>& aPaths, wxString& aError ) override;

    bool IsPathInHead( git_repository* aRepo, const wxString& aPath ) override;

    bool AbandonOperation( git_repository* aRepo, wxString& aError ) override;

    bool GetMergeConflicts( git_repository* aRepo, const wxString& aStageDir, wxString& aHeadOid, wxString& aTheirsOid,
                            std::vector<KIGIT_CONFLICT_SIDES>& aConflicts, wxString& aError ) override;

    MergeCommitResult CreateMergedCommit( git_repository* aRepo, const wxString& aExpectedHeadOid,
                                          const wxString&                                   aTheirsOid,
                                          const std::vector<std::pair<wxString, wxString>>& aResolvedFiles,
                                          KIGIT_ERROR_MESSAGE&                              aError ) override;

    std::optional<std::vector<std::string>> IndexedPaths( git_repository* aRepo ) override;

    std::vector<KIGIT_PATH_PLACE> LocatePaths( git_repository* aRepo, const std::vector<std::string>& aPaths ) override;

    std::map<std::string, KIGIT_PATH_COMMIT> LastChanges( git_repository* aRepo, const std::vector<std::string>& aPaths,
                                                          const KIGIT_CANCEL& aCancelled ) override;

    std::vector<KIGIT_PATH_COMMIT> PathHistory( git_repository* aRepo, const std::string& aPath,
                                                const KIGIT_CANCEL& aCancelled ) override;

    std::optional<KIGIT_TREE_ENTRY> TreeEntryAt( git_repository* aRepo, const std::string& aCommit,
                                                 const std::string& aPath ) override;

    std::optional<std::string> ReadBlob( git_repository* aRepo, const std::string& aOid ) override;

    int WalkTree( git_repository* aRepo, const std::string& aCommit, const std::string& aFolder,
                  const std::function<KIGIT_WALK( const KIGIT_TREE_ENTRY& )>& aVisit ) override;

    std::optional<std::string> FilterText( git_repository* aRepo, const std::string& aPath, const std::string& aText,
                                           bool aToRepository ) override;

    std::map<std::string, std::optional<std::string>> FilterTextsAt( git_repository* aRepo, const std::string& aCommit,
                                                                     const std::map<std::string, std::string>& aTexts,
                                                                     bool aToRepository ) override;

    std::optional<std::string> MergeBase( git_repository* aRepo, const std::string& aOne,
                                          const std::string& aTwo ) override;

    std::optional<std::vector<std::string>> ChangedPaths( git_repository* aRepo, const std::string& aFrom,
                                                          const std::string& aTo ) override;

    std::string BlobId( const std::string& aContent ) override;

    std::optional<bool> IsDescendant( git_repository* aRepo, const std::string& aCommit,
                                      const std::string& aAncestor ) override;

    std::optional<KIGIT_COMMIT_RECORD> ReadCommit( git_repository* aRepo, const std::string& aOid ) override;

    int WalkCommits( git_repository* aRepo, const std::string& aRef, const std::string& aHide,
                     const std::function<bool( const KIGIT_COMMIT_RECORD& )>& aVisit ) override;

    std::optional<std::string> ReadReference( git_repository* aRepo, const std::string& aName ) override;

    std::string HeadTarget( git_repository* aRepo ) override;

    int SetReference( git_repository* aRepo, const std::string& aName, const std::string& aOid,
                      const std::optional<std::string>& aExpected, const std::string& aLogMessage ) override;

    int DeleteReference( git_repository* aRepo, const std::string& aName,
                         const std::optional<std::string>& aExpected ) override;

    std::vector<KIGIT_REFERENCE> ListReferences( git_repository* aRepo, const std::string& aGlob ) override;

    std::optional<std::string> CreateRecordCommit( git_repository* aRepo, const std::string& aMessage,
                                                   const std::string& aAuthorName,
                                                   const std::string& aAuthorEmail ) override;

    int CreateAnnotatedTag( git_repository* aRepo, const std::string& aName, const std::string& aTarget,
                            const std::string& aMessage, const std::string& aTaggerName,
                            const std::string& aTaggerEmail, std::string& aTagOid ) override;

    std::optional<KIGIT_TAG_RECORD> ReadAnnotatedTag( git_repository* aRepo, const std::string& aRefName ) override;

    git_repository* OpenRepositoryAt( const std::string& aPath ) override;

    int FindRepository( const std::string& aPath, git_repository*& aRepo ) override;

    int CheckoutCommit( git_repository* aRepo, const std::string& aCommit, const std::string& aBranch ) override;

    int ResetIndex( git_repository* aRepo, const std::string& aCommit ) override;

    int CheckoutFromIndex( git_repository* aRepo, const std::vector<std::string>& aPaths ) override;

    int MoveStaging( git_repository* aRepo, const std::string& aFrom, const std::string& aTo,
                     const std::vector<std::string>& aPaths ) override;

    int CommitFiles( git_repository* aRepo, const std::string& aBranch, const std::string& aParent,
                     const std::map<std::string, std::optional<std::string>>& aFiles, const std::string& aMessage,
                     const std::string& aAuthorName, const std::string& aAuthorEmail, std::string& aCommit ) override;

private:
    PullResult handleFastForward( GIT_PULL_HANDLER* aHandler );
    PullResult handleMerge( GIT_PULL_HANDLER* aHandler, const git_annotated_commit** aMergeHeads,
                            size_t aMergeHeadsCount );
    PullResult handleRebase( GIT_PULL_HANDLER* aHandler, const git_annotated_commit** aMergeHeads,
                             size_t aMergeHeadsCount );
};

#endif
