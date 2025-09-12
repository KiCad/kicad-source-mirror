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

#ifndef GIT_BACKEND_H_
#define GIT_BACKEND_H_

#include <map>
#include <import_export.h>
#include <set>
#include <vector>
#include <wx/string.h>

class GIT_CLONE_HANDLER;
class GIT_COMMIT_HANDLER;
class GIT_PUSH_HANDLER;
class GIT_STATUS_HANDLER;
class GIT_ADD_TO_INDEX_HANDLER;
class GIT_REMOVE_FROM_INDEX_HANDLER;
class GIT_CONFIG_HANDLER;
class GIT_INIT_HANDLER;
class GIT_BRANCH_HANDLER;
class GIT_PULL_HANDLER;
class GIT_REVERT_HANDLER;
struct RemoteConfig;
struct git_repository;
enum class InitResult;
enum class BranchResult;
enum class PullResult;
struct FileStatus;
enum class PushResult;

// Commit result shared across backend and handlers
enum class CommitResult
{
    Success,
    Error,
    Cancelled
};

class APIEXPORT GIT_BACKEND
{
public:
    virtual ~GIT_BACKEND() = default;

    virtual void Init() = 0;
    virtual void Shutdown() = 0;

    // Whether the libgit2 library is available/initialized enough for use
    virtual bool IsLibraryAvailable() = 0;

    virtual bool Clone( GIT_CLONE_HANDLER* aHandler ) = 0;

    virtual CommitResult Commit( GIT_COMMIT_HANDLER* aHandler,
                                 const std::vector<wxString>& aFiles,
                                 const wxString& aMessage,
                                 const wxString& aAuthorName,
                                 const wxString& aAuthorEmail ) = 0;

    virtual PushResult Push( GIT_PUSH_HANDLER* aHandler ) = 0;

    virtual bool HasChangedFiles( GIT_STATUS_HANDLER* aHandler ) = 0;

    virtual std::map<wxString, FileStatus> GetFileStatus( GIT_STATUS_HANDLER* aHandler,
                                                          const wxString& aPathspec ) = 0;

    virtual wxString GetCurrentBranchName( GIT_STATUS_HANDLER* aHandler ) = 0;

    virtual void UpdateRemoteStatus( GIT_STATUS_HANDLER* aHandler,
                                     const std::set<wxString>& aLocalChanges,
                                     const std::set<wxString>& aRemoteChanges,
                                     std::map<wxString, FileStatus>& aFileStatus ) = 0;

    virtual wxString GetWorkingDirectory( GIT_STATUS_HANDLER* aHandler ) = 0;

    virtual wxString GetWorkingDirectory( GIT_CONFIG_HANDLER* aHandler ) = 0;
    virtual bool GetConfigString( GIT_CONFIG_HANDLER* aHandler, const wxString& aKey,
                                  wxString& aValue ) = 0;

    virtual bool IsRepository( GIT_INIT_HANDLER* aHandler, const wxString& aPath ) = 0;
    virtual InitResult InitializeRepository( GIT_INIT_HANDLER* aHandler, const wxString& aPath ) = 0;
    virtual bool SetupRemote( GIT_INIT_HANDLER* aHandler, const RemoteConfig& aConfig ) = 0;

    virtual BranchResult SwitchToBranch( GIT_BRANCH_HANDLER* aHandler, const wxString& aBranchName ) = 0;
    virtual bool BranchExists( GIT_BRANCH_HANDLER* aHandler, const wxString& aBranchName ) = 0;

    virtual bool PerformFetch( GIT_PULL_HANDLER* aHandler, bool aSkipLock ) = 0;
    virtual PullResult PerformPull( GIT_PULL_HANDLER* aHandler ) = 0;

    virtual void PerformRevert( GIT_REVERT_HANDLER* aHandler ) = 0;

    virtual git_repository* GetRepositoryForFile( const char* aFilename ) = 0;
    virtual int CreateBranch( git_repository* aRepo, const wxString& aBranchName ) = 0;
    virtual bool RemoveVCS( git_repository*& aRepo, const wxString& aProjectPath,
                            bool aRemoveGitDir, wxString* aErrors ) = 0;

    virtual bool AddToIndex( GIT_ADD_TO_INDEX_HANDLER* aHandler, const wxString& aFilePath ) = 0;

    virtual bool PerformAddToIndex( GIT_ADD_TO_INDEX_HANDLER* aHandler ) = 0;

    virtual bool RemoveFromIndex( GIT_REMOVE_FROM_INDEX_HANDLER* aHandler, const wxString& aFilePath ) = 0;

    virtual void PerformRemoveFromIndex( GIT_REMOVE_FROM_INDEX_HANDLER* aHandler ) = 0;
};

APIEXPORT GIT_BACKEND* GetGitBackend();
APIEXPORT void SetGitBackend( GIT_BACKEND* aBackend );

#endif
