// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

#ifndef GIT_REPO_MIXIN_H
#define GIT_REPO_MIXIN_H

#include "kicad_git_common.h"
#include "kicad_git_errors.h"
#include "git_progress.h"
#include <import_export.h>

class APIEXPORT KIGIT_REPO_MIXIN: public KIGIT_ERRORS, public GIT_PROGRESS
{
public:
    KIGIT_REPO_MIXIN( KIGIT_COMMON* aCommon ) : m_common( aCommon )
    {
        // Ensure m_common is initialized
        wxASSERT( aCommon != nullptr );
    }

    virtual ~KIGIT_REPO_MIXIN()
    {
        // Non-owning, don't delete
    }

    /**
     * @brief Get the current branch name
     * @return The current branch name
     */
    wxString GetCurrentBranchName() const
    {
        return m_common->GetCurrentBranchName();
    }

    /**
     * @brief Get a list of branch names
     * @return A vector of branch names
     */
    std::vector<wxString> GetBranchNames() const
    {
        return m_common->GetBranchNames();
    }

    /**
     * @brief Get a list of project directories
     * @return A vector of project directories
     */
    std::vector<wxString> GetProjectDirs()
    {
        return m_common->GetProjectDirs();
    }

    /**
     * @brief Get a pointer to the git repository
     * @return A pointer to the git repository
     */
    git_repository* GetRepo() const
    {
        return m_common->GetRepo();
    }

    /**
     * @brief Get a pair of sets of files that differ locally from the remote repository
     * @return A pair of sets of files that differ locally from the remote repository
     */
    std::pair<std::set<wxString>, std::set<wxString>> GetDifferentFiles() const
    {
        return m_common->GetDifferentFiles();
    }

    /**
     * @brief Get the common object
     * @return The common object
     */

    KIGIT_COMMON* GetCommon() const
    {
        return m_common;
    }

    /**
     * @brief Reset the next public key to test
     */
    void ResetNextKey() { m_common->ResetNextKey(); }

    /**
     * @brief Get the next public key
     * @return The next public key
     */
    wxString GetNextPublicKey()
    {
        return m_common->GetNextPublicKey();
    }

    /**
     * @brief Get the connection type
     * @return The connection type
     */
    KIGIT_COMMON::GIT_CONN_TYPE GetConnType() const
    {
        return m_common->GetConnType();
    }

    /**
     * @brief Get the username
     * @return The username
     */
    wxString GetUsername() const
    {
        return m_common->GetUsername();
    }

    /**
     * @brief Get the password
     * @return The password
     */
    wxString GetPassword() const
    {
        return m_common->GetPassword();
    }

    /**
     * @brief Set the username
     * @param aUsername The username
     */
    void SetUsername( const wxString& aUsername )
    {
        m_common->SetUsername( aUsername );
    }

    /**
     * @brief Set the password
     * @param aPassword The password
     */
    void SetPassword( const wxString& aPassword )
    {
        m_common->SetPassword( aPassword );
    }

    /**
     * @brief Set the SSH key
     * @param aSSHKey The SSH key
     */
    void SetSSHKey( const wxString& aSSHKey )
    {
        m_common->SetSSHKey( aSSHKey );
    }

    /**
     * @brief Get the remote name
     * @return The remote name
     */
    wxString GetRemotename() const
    {
        return m_common->GetRemotename();
    }

    /**
     * @brief Get the git root directory
     * @return The git root directory
     */
    wxString GetGitRootDirectory() const
    {
        return m_common->GetGitRootDirectory();
    }

    /**
     * @brief Get the project directory path, preserving symlinks if set
     * @return The project directory path
     */
    wxString GetProjectDir() const
    {
        return m_common->GetProjectDir();
    }

    /**
     * @brief Return the connection types that have been tested for authentication
     * @return The connection types that have been tested for authentication
     */
    unsigned& TestedTypes() { return m_common->TestedTypes(); }


private:
    KIGIT_COMMON* m_common;
};


#endif // GIT_REPO_MIXIN_H