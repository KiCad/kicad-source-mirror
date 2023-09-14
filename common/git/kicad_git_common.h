/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef _GIT_COMMON_H_
#define _GIT_COMMON_H_

#include <git/kicad_git_errors.h>

#include <git2.h>
#include <set>

#include <wx/string.h>

class KIGIT_COMMON : public KIGIT_ERRORS
{

public:
    KIGIT_COMMON( git_repository* aRepo );
    ~KIGIT_COMMON();

    git_repository* GetRepo() const;

    void SetRepo( git_repository* aRepo )
    {
        m_repo = aRepo;
    }

    wxString GetCurrentBranchName() const;

    std::vector<wxString> GetBranchNames() const;

    virtual void UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage ) {};

    /**
     * Return a vector of project files in the repository.  Sorted by the depth of
     * the project file in the directory tree
     *
     * @return std::vector<wxString> of project files
     */
    std::vector<wxString> GetProjectDirs();

    /**
     * Return a pair of sets of files that differ locally from the remote repository
     * The first set is files that have been committed locally but not pushed
     * The second set is files that have been committed remotely but not pulled
    */
    std::pair<std::set<wxString>,std::set<wxString>> GetDifferentFiles() const;

    enum class GIT_STATUS
    {
        GIT_STATUS_UNTRACKED,
        GIT_STATUS_CURRENT,
        GIT_STATUS_MODIFIED,        // File changed but not committed to local repository
        GIT_STATUS_ADDED,
        GIT_STATUS_DELETED,
        GIT_STATUS_BEHIND,          // File changed in remote repository but not in local
        GIT_STATUS_AHEAD,           // File changed in local repository but not in remote
        GIT_STATUS_CONFLICTED,
        GIT_STATUS_LAST
    };

    enum class GIT_CONN_TYPE
    {
        GIT_CONN_HTTPS = 0,
        GIT_CONN_SSH,
        GIT_CONN_LOCAL,
        GIT_CONN_LAST
    };

    wxString GetUsername() const { return m_username; }
    wxString GetPassword() const { return m_password; }
    wxString GetSSHKey() const { return m_sshKey; }
    GIT_CONN_TYPE GetConnType() const { return m_connType; }

    void SetUsername( const wxString& aUsername ) { m_username = aUsername; }
    void SetPassword( const wxString& aPassword ) { m_password = aPassword; }
    void SetSSHKey( const wxString& aSSHKey ) { m_sshKey = aSSHKey; }

    void SetConnType( GIT_CONN_TYPE aConnType ) { m_connType = aConnType; }
    void SetConnType( unsigned aConnType )
    {
        if( aConnType < static_cast<unsigned>( GIT_CONN_TYPE::GIT_CONN_LAST ) )
            m_connType = static_cast<GIT_CONN_TYPE>( aConnType );
    }

    // Holds a temporary variable that can be used by the authentication callback
    // to remember which types of authentication have been tested so that we
    // don't loop forever.
    unsigned& TestedTypes() { return m_testedTypes; }

    // Returns true if the repository has local commits that have not been pushed
    bool HasLocalCommits() const;

    // Returns true if the repository has a remote that can be pushed to pulled from
    bool HasPushAndPullRemote() const;

protected:
    git_repository* m_repo;

    GIT_CONN_TYPE m_connType;
    wxString m_username;
    wxString m_password;
    wxString m_sshKey;

    unsigned m_testedTypes;

};


extern "C" int progress_cb( const char* str, int len, void* data );
extern "C" void clone_progress_cb( const char* str, size_t len, size_t total, void* data );
extern "C" int transfer_progress_cb( const git_transfer_progress* aStats, void* aPayload );
extern "C" int update_cb( const char* aRefname, const git_oid* aFirst, const git_oid* aSecond,
                          void* aPayload );
extern "C" int push_transfer_progress_cb( unsigned int aCurrent, unsigned int aTotal,
                                          size_t aBytes, void* aPayload );
extern "C" int push_update_reference_cb( const char* aRefname, const char* aStatus,
                                         void* aPayload );

extern "C" int fetchhead_foreach_cb( const char*, const char*,
                                     const git_oid* aOID, unsigned int aIsMerge, void* aPayload );
extern "C" int credentials_cb( git_cred** aOut, const char* aUrl, const char* aUsername,
                                unsigned int aAllowedTypes, void* aPayload );

#endif // _GIT_COMMON_H_