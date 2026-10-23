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
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef GIT_REPO_MIXIN_H
#define GIT_REPO_MIXIN_H

#include "kicad_git_common.h"
#include "kicad_git_errors.h"
#include "git_progress.h"
#include <import_export.h>

class APIEXPORT KIGIT_REPO_MIXIN: public KIGIT_ERRORS, public GIT_PROGRESS
{
public:
    KIGIT_REPO_MIXIN( KIGIT_COMMON* aCommon ) :
            m_common( aCommon ),
            m_cancelToken( KIGIT_COMMON::NewCancelToken() )
    {
        // Ensure m_common is initialized
        wxASSERT( aCommon != nullptr );
    }

    virtual ~KIGIT_REPO_MIXIN()
    {
        // Non-owning, don't delete
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
    std::atomic<unsigned>& TestedTypes() { return m_common->TestedTypes(); }

    void UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage ) override
    {
        // The transfer callbacks poll this token and abort on their next tick.
        if( !ReportProgress( aCurrent, aTotal, aMessage ) )
            CancelThisOperation();
    }

    /// Callbacks read this token rather than the session, so one operation's cancel cannot
    /// reach another. A pull passes its own token to the fetch nested inside it.
    void SetCancelToken( const KIGIT_COMMON::CANCEL_TOKEN& aToken )
    {
        if( aToken )
            m_cancelToken = aToken;
    }

    const KIGIT_COMMON::CANCEL_TOKEN& CancelToken() const { return m_cancelToken; }

    /// Worker transfers may wait for a background read to finish. UI callers keep the default
    /// nonblocking behavior and report a busy repository instead.
    void SetWaitForActionLock( bool aWait ) { m_waitForActionLock = aWait; }

    /// A UI caller retrying confirmed input waits this long for a background read instead of failing at once.
    void SetActionLockWait( std::chrono::milliseconds aWait ) { m_actionLockWait = aWait; }

    std::unique_lock<std::mutex> GetActionLock() const
    {
        if( m_waitForActionLock )
            return m_common->LockAction();

        if( m_actionLockWait.count() > 0 )
            return m_common->TryLockActionFor( m_actionLockWait );

        return m_common->TryLockAction();
    }

    /// The host key this operation's transfer refused, for the UI to offer to trust or to report. Kept per
    /// operation so an overlapping transfer on the same session cannot clear or replace it. Only the
    /// operation's own thread writes it, and readers wait for the transfer to return.
    std::optional<KIGIT::HOST_KEY_FAILURE> GetHostKeyFailure() const { return m_hostKeyFailure; }
    void SetHostKeyFailure( const KIGIT::HOST_KEY_FAILURE& aFailure ) { m_hostKeyFailure = aFailure; }
    void ClearHostKeyFailure() { m_hostKeyFailure.reset(); }

    bool IsThisOperationCancelled() const { return m_cancelToken->load(); }

    void CancelThisOperation() { m_cancelToken->store( true ); }


private:
    KIGIT_COMMON*              m_common;
    KIGIT_COMMON::CANCEL_TOKEN m_cancelToken;
    bool                       m_waitForActionLock = false;
    std::chrono::milliseconds  m_actionLockWait{ 0 };

    std::optional<KIGIT::HOST_KEY_FAILURE> m_hostKeyFailure;
};


#endif // GIT_REPO_MIXIN_H
