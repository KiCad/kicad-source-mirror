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

#ifndef PROJECT_GIT_UTILS_H
#define PROJECT_GIT_UTILS_H

#include <git2.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <wx/string.h>
#include <wx/filename.h>
#include <import_export.h>

class KIGIT_COMMON;

namespace KIGIT
{

/** Utility class with helper functions for project level git operations. */
class APIEXPORT PROJECT_GIT_UTILS
{
public:
    /**
     * Discover and open the repository that contains the given file.
     *
     * @param aFilename Absolute path to a file inside the repository.
     * @param aError Receives a reason worth showing when the repository cannot be opened.
     * @param aTrustCommand Receives the git command that trusts a repository another user owns.
     * @return Newly opened git_repository or nullptr on failure.
     */
    static git_repository* GetRepositoryForFile( const char* aFilename, wxString* aError = nullptr,
                                                 wxString* aTrustCommand = nullptr );

    /**
     * Create a new branch based on HEAD.
     *
     * @param aRepo       Repository in which to create the branch.
     * @param aBranchName Name of the new branch.
     * @return 0 on success, libgit2 error code on failure.
     */
    static int CreateBranch( git_repository* aRepo, const wxString& aBranchName );

    /**
     * Return the current HEAD commit hash for the repository containing aProjectFile.
     *
     * @param aProjectFile Absolute path to any file within the repository (typically the
     *                     project file path).
     * @param aShort       If true, return the short (8 char) hash, otherwise full hash.
     * @return wxString containing the hash or "no hash" if unavailable.
     */
    static wxString GetCurrentHash( const wxString& aProjectFile, bool aShort );

    /**
     * Return HEAD for commit-based text queries.
     *
     * An active text-evaluation frame captures the first result per repository Git directory;
     * later calls can return an older HEAD than the repository currently has. Without a frame,
     * this reads the live HEAD.
     *
     * @param aRepo Repository to read, or nullptr.
     * @return HEAD object ID, or the zero OID when unavailable (including read errors).
     */
    static git_oid GetCapturedHeadOid( git_repository* aRepo );

    /**
     * Remove version control from a directory by freeing the repository and
     * optionally removing the .git directory.
     *
     * @param aRepo Repository to free (will be set to nullptr)
     * @param aProjectPath Path to the project directory
     * @param aRemoveGitDir If true, also remove the .git directory from disk
     * @param aErrors Output parameter for any error messages
     * @return True on success, false on failure
     */
    static bool RemoveVCS( git_repository*& aRepo, const wxString& aProjectPath = wxEmptyString,
                           bool aRemoveGitDir = false, wxString* aErrors = nullptr );

    /**
     * Compute a working directory path that preserves symlinks from the user's project path.
     *
     * When a project is opened via a symlinked path, git_repository_workdir() returns the
     * canonical (symlink-resolved) path. This causes path mismatches between the tree cache
     * (which uses the user-provided symlinked path) and the git status results (which use
     * canonical paths). This function computes a working directory path that preserves
     * the symlinks from the user's original project path.
     *
     * @param aUserProjectPath The path the user used to open the project (may contain symlinks)
     * @param aCanonicalWorkDir The canonical workdir from git_repository_workdir()
     * @return Working directory path with symlinks preserved, or aCanonicalWorkDir on failure
     */
    static wxString ComputeSymlinkPreservingWorkDir( const wxString& aUserProjectPath,
                                                     const wxString& aCanonicalWorkDir );

    /**
     * Test whether an absolute file path lives inside the current project directory.
     *
     * The git repository may span more than the KiCad project (the project is often a
     * subdirectory of a larger repo).  Commit and amend checklists must only offer files
     * under the project directory, otherwise unrelated repository files leak into the
     * commit (issue #15910).  The project path is normalized to end in a separator before
     * the prefix comparison so that a sibling such as "proj-extra/" is not treated as being
     * inside "proj/".  An empty project path matches nothing.
     *
     * @param aAbsPath Absolute path of the file being tested.
     * @param aProjectPath Absolute path of the project directory.
     * @return True if aAbsPath is within the project directory.
     */
    static bool IsWithinProjectPath( const wxString& aAbsPath, const wxString& aProjectPath );

    /**
     * Free the session's repository handle without pulling it out from under a running
     * operation. With the action mutex free it goes at once, otherwise the orphan registry
     * frees it once the running operation releases the mutex.
     *
     * @param aLabel Names the orphan registry entry, for shutdown diagnostics.
     */
    static void SafeFreeRepository( std::shared_ptr<KIGIT_COMMON> aCommon, const std::string& aLabel );

    /// Shared patterns for new .gitignore files and the untracked-file filter.
    static const std::vector<wxString>& TransientGitIgnorePatterns();

    /// Match a project-relative path against the shared transient patterns.
    /// Callers must keep tracked and staged files visible even when their names match.
    static bool IsTransientGitFile( const wxFileName& aFile );

    /**
     * The project's folder inside the repository, ending in a slash.
     *
     * An empty string means the repository root. No value means either path is
     * missing or the project is outside the repository. Uses forward slashes for libgit2.
     */
    static std::optional<wxString> ProjectPrefix( const wxString& aWorkDir, const wxString& aProjectPath );

    /// Whether git routes @p aPath through the Git LFS filter.
    /// aCommit reads the attributes that commit carries rather than the ones in hand, for
    /// judging a version before it is checked out.
    static bool PathUsesGitLfs( git_repository* aRepo, const wxString& aPath, const git_oid* aCommit = nullptr );

    /**
     * Whether any committed file in the project's folder goes through Git LFS.
     *
     * libgit2 runs no filter program, so every operation that writes such a file writes the
     * placeholder text instead of the file. @p aExample receives the first one found.
     */
    static bool ProjectUsesGitLfs( const KIGIT_COMMON& aCommon, wxString* aExample = nullptr,
                                   const git_oid* aCommit = nullptr );

    /**
     * Return a refusal message for LFS files or an unknown project folder, otherwise empty.
     * The caller holds the session's action lock.
     */
    static wxString LfsRefusal( KIGIT_COMMON* aCommon );

    /**
     * Write a file as it exists at a git ref to a path on disk, so a committed version can
     * be diffed. False for an empty ref or path, or one that is not a blob.
     *
     * @param aRef Branch, tag or commit to read from, for example "HEAD".
     * @param aPath Path inside the repository, relative to its root.
     */
    static bool WriteBlobToFile( git_repository* aRepo, const wxString& aRef, const wxString& aPath,
                                 const wxString& aDestPath );

    /**
     * Check out a whole tree into a directory, leaving the repository's working copy and
     * index alone. A hierarchical schematic needs this rather than a lone blob, because its
     * root sheet references sub-sheet files.
     *
     * @param aDestDir Destination directory, created if needed.
     */
    static bool WriteTreeToDir( git_repository* aRepo, const wxString& aRef, const wxString& aDestDir );
};

} // namespace KIGIT

#endif // PROJECT_GIT_UTILS_H
