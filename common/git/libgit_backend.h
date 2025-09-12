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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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

    PushResult Push( GIT_PUSH_HANDLER* aHandler ) override;

    bool HasChangedFiles( GIT_STATUS_HANDLER* aHandler ) override;

    std::map<wxString, FileStatus> GetFileStatus( GIT_STATUS_HANDLER* aHandler,
                                                  const wxString& aPathspec ) override;

    wxString GetCurrentBranchName( GIT_STATUS_HANDLER* aHandler ) override;

    void UpdateRemoteStatus( GIT_STATUS_HANDLER* aHandler,
                             const std::set<wxString>& aLocalChanges,
                             const std::set<wxString>& aRemoteChanges,
                             std::map<wxString, FileStatus>& aFileStatus ) override;

    wxString GetWorkingDirectory( GIT_STATUS_HANDLER* aHandler ) override;

    wxString GetWorkingDirectory( GIT_CONFIG_HANDLER* aHandler ) override;
    bool GetConfigString( GIT_CONFIG_HANDLER* aHandler, const wxString& aKey,
                          wxString& aValue ) override;

    bool IsRepository( GIT_INIT_HANDLER* aHandler, const wxString& aPath ) override;
    InitResult InitializeRepository( GIT_INIT_HANDLER* aHandler, const wxString& aPath ) override;
    bool SetupRemote( GIT_INIT_HANDLER* aHandler, const RemoteConfig& aConfig ) override;

    BranchResult SwitchToBranch( GIT_BRANCH_HANDLER* aHandler, const wxString& aBranchName ) override;
    bool BranchExists( GIT_BRANCH_HANDLER* aHandler, const wxString& aBranchName ) override;

    bool PerformFetch( GIT_PULL_HANDLER* aHandler, bool aSkipLock ) override;
    PullResult PerformPull( GIT_PULL_HANDLER* aHandler ) override;

    void PerformRevert( GIT_REVERT_HANDLER* aHandler ) override;

    git_repository* GetRepositoryForFile( const char* aFilename ) override;
    int CreateBranch( git_repository* aRepo, const wxString& aBranchName ) override;
    bool RemoveVCS( git_repository*& aRepo, const wxString& aProjectPath,
                    bool aRemoveGitDir, wxString* aErrors ) override;

    bool AddToIndex( GIT_ADD_TO_INDEX_HANDLER* aHandler, const wxString& aFilePath ) override;

    bool PerformAddToIndex( GIT_ADD_TO_INDEX_HANDLER* aHandler ) override;

    bool RemoveFromIndex( GIT_REMOVE_FROM_INDEX_HANDLER* aHandler, const wxString& aFilePath ) override;

    void PerformRemoveFromIndex( GIT_REMOVE_FROM_INDEX_HANDLER* aHandler ) override;

private:
    PullResult handleFastForward( GIT_PULL_HANDLER* aHandler );
    PullResult handleMerge( GIT_PULL_HANDLER* aHandler, const git_annotated_commit** aMergeHeads,
                            size_t aMergeHeadsCount );
    PullResult handleRebase( GIT_PULL_HANDLER* aHandler, const git_annotated_commit** aMergeHeads,
                             size_t aMergeHeadsCount );
};

#endif
