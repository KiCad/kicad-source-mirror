/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


/**
 * @file mdw_git_repo.h
 * @brief The repository reads and the check-in commit the git backend is built on.
 *
 * Paths are repository-relative, '/'-separated UTF-8, as they appear in MDW item ids. Every read and
 * write goes through the application's git backend. The object owns its own libgit2 handle and is
 * used by one thread at a time.
 */

#ifndef MDW_GIT_REPO_H
#define MDW_GIT_REPO_H

#include <git/git_backend.h>

#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class KIGIT_COMMON;
class wxString;
struct git_repository;

namespace kiapi::workspace
{
class Account;
}

/// Polled by long walks; true once the request they serve was cancelled
using MDW_GIT_CANCEL = KIGIT_CANCEL;

using MDW_GIT_VERSION = KIGIT_PATH_COMMIT;


/// The author a git backend records on its commits, tags and claims
struct GIT_IDENTITY
{
    std::string name;  ///< Empty when no author or login names anyone
    std::string email; ///< Empty when no author names one

    /// What a git signature records. libgit2 refuses an empty name or email, so a stand-in fills either gap
    std::string SignedName() const { return name.empty() ? "KiCad" : name; }
    std::string SignedEmail() const { return email.empty() ? "local" : email; }
};


/**
 * The author of new commits in aRepo, which may be null. The repository's own config wins, then the Hello
 * author, then the global git config, then aSignedInUser, as KIGIT::ResolveAuthor orders them.
 */
GIT_IDENTITY MdwGitIdentity( git_repository* aRepo, const kiapi::workspace::Account& aHello,
                             const wxString& aSignedInUser );


/**
 * The id a claim or lock records for the account signed in as aUsername to the server aRemoteUrl names, as
 * username@realm. The server verified that login on the push, where an author is only a name anyone can type.
 * With no login, as in a local-path workspace, it is aEmail.
 */
std::string MdwGitOwnerId( const std::string& aUsername, const std::string& aRemoteUrl, const std::string& aEmail );


/**
 * The message of a record the backend keeps in git, such as a claim's commit or a release's tag. Any git viewer
 * shows the one-line aSubject and aBody, which may be empty, and aJson follows as the last line for
 * MdwGitRecordJson to read back.
 */
std::string MdwGitRecordMessage( const std::string& aSubject, const std::string& aBody, const nlohmann::json& aJson );

/// The JSON MdwGitRecordMessage wrote into aMessage, or null when aMessage holds no record that parses
nlohmann::json MdwGitRecordJson( const std::string& aMessage );

/// The last component of the '/'-separated aPath
std::string MdwGitBaseName( const std::string& aPath );


/// Never created, so naming it as core.attributesFile leaves only the repository's own rules. Relative, as libgit2
/// reads it from the working tree, so a staged clone renamed into place still finds it
inline constexpr char MDW_GIT_NO_USER_ATTRIBUTES[] = ".git/kicad-no-attributes";

/**
 * Have aRepo check out and stage files as their blobs hold them whatever the user's config asks. Only system
 * attributes still apply, as checkout cannot skip them and filtering must agree with checkout. The settings go in
 * aRepo's own config, as a level above it may be shared with other repositories. 0 or a libgit2 error.
 */
int MdwGitCommittedBytes( git_repository* aRepo );


/// One path under a directory whose working-tree file differs from HEAD, as git status sees it.
struct MDW_GIT_CHANGE
{
    enum class KIND
    {
        ADDED,
        REMOVED,
        MODIFIED
    };

    std::string path; ///< Repository-relative
    KIND        kind = KIND::MODIFIED;
    bool        tracked = true; ///< False for an untracked file, which no item names yet
};


class MDW_GIT_REPO
{
public:
    MDW_GIT_REPO();
    ~MDW_GIT_REPO();

    /// Open the repository whose working tree contains aPath. False when there is none.
    bool Open( const std::filesystem::path& aPath );

    git_repository* Handle() const { return m_repo.get(); }

    KIGIT_COMMON* Common() const { return m_common.get(); }

    const std::filesystem::path& Root() const { return m_root; }

    /// Every path in the index at stage 0, or nothing when the index cannot be read.
    std::optional<std::vector<std::string>> TrackedFiles();

    bool IsTracked( const std::string& aPath );

    /**
     * The newest commit on HEAD that changed each path, as `git log -1 -- <path>` would pick it.
     * A path absent from the map has no such commit, or the walk was cancelled first.
     */
    std::map<std::string, MDW_GIT_VERSION> LastChanges( const std::vector<std::string>& aPaths,
                                                        const MDW_GIT_CANCEL&           aCancelled );

    /**
     * The newest commit on HEAD that changed anything under the folder aScope, the whole tree when empty, as
     * `git log -1 -- <scope>` picks it. Nothing when there is none, or the walk was cancelled first.
     */
    std::optional<MDW_GIT_VERSION> ScopeRevision( const std::string& aScope, const MDW_GIT_CANCEL& aCancelled );

    /// The commits on HEAD that changed aPath, newest first, as `git log -- <path>` lists them. aPath may name a
    /// folder, and the empty path names the whole tree.
    std::vector<MDW_GIT_VERSION> History( const std::string& aPath, const MDW_GIT_CANCEL& aCancelled );

    /// The content of aPath in commit aOid, or nothing when either does not exist.
    std::optional<std::string> ContentAt( const std::string& aOid, const std::string& aPath );

    enum class PLACE
    {
        UNTRACKED,
        TRACKED, ///< In the index but not yet committed
        IN_HEAD
    };

    /// Where each of aPaths stands, reading HEAD's tree and the index once for all of them.
    std::vector<PLACE> Locate( const std::vector<std::string>& aPaths );

    /**
     * Every file under the repository-relative directory aPrefix (the whole tree when empty) whose
     * working copy differs from HEAD, sorted by path. Untracked files count unless ignored or
     * transient, as the Version Control pane hides them. Nothing when the status cannot be read.
     */
    std::optional<std::vector<MDW_GIT_CHANGE>> LocalChanges( const std::string& aPrefix );

    /// True when the working-tree file differs from HEAD or is staged differently.
    bool HasChanges( const std::string& aPath );

    enum class COMMIT_RESULT
    {
        OK,
        NO_CHANGES,
        BUSY,                  ///< Another writer kept the branch or index locked past the retry budget
        CANCELLED,             ///< aCancelled answered true before the branch moved
        OPERATION_IN_PROGRESS, ///< A merge, rebase or similar is underway and must be finished in git first
        FAILED
    };

    /**
     * Commit the working-tree file aPath as aAuthor, filtered as git add would, on HEAD's tree and nothing
     * else, so whatever else is staged stays staged. The whole step reruns while another writer
     * holds a lock or moves the branch first. aCancelled is asked before each attempt and just
     * before the branch moves. aOid gets the new commit, aError says why it failed.
     */
    COMMIT_RESULT CommitFile( const std::string& aPath, const std::string& aMessage, const GIT_IDENTITY& aAuthor,
                              const MDW_GIT_CANCEL& aCancelled, std::string& aOid, std::string& aError );

    /// The filter driver .gitattributes gives aPath, such as lfs, which libgit2 cannot run. Empty for none.
    std::string FilterDriver( const std::string& aPath ) const;

    /// The commit HEAD points at, empty while the branch is unborn.
    std::string HeadOid() const;

    /// The remote claims travel through, as the Version Control pane picks it
    std::string RemoteName() const;

    /// True when RemoteName() is a configured remote, so claims have somewhere to travel
    bool HasRemote() const;

    /// The address RemoteName() fetches from after url.insteadOf, with any credentials it embeds. Empty for none
    std::string RemoteUrl() const;

private:
    // KIGIT::GitRepositoryPtr's lambda deleter has internal linkage, which an exported member must not
    struct REPO_FREE
    {
        void operator()( git_repository* aRepo ) const;
    };

    std::unique_ptr<git_repository, REPO_FREE> m_repo;
    std::unique_ptr<KIGIT_COMMON>              m_common;
    std::filesystem::path                      m_root;
};

#endif // MDW_GIT_REPO_H
