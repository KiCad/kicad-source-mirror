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

/**
 * @file kigit_transport.h
 * @brief The one place a fetch, push, clone or connection check reaches a remote.
 *
 * Every call offers the session's credentials and checks the server's host key the same way,
 * reports progress through its handler and stops when the handler's operation is cancelled.
 */

#ifndef KIGIT_TRANSPORT_H
#define KIGIT_TRANSPORT_H

#include <import_export.h>

#include <git2.h>
#include <wx/string.h>

#include <functional>
#include <memory>
#include <vector>

class KIGIT_REPO_MIXIN;
struct KIGIT_ERROR_INFO;

namespace KIGIT
{

class APIEXPORT REMOTE_TRANSPORT
{
public:
    /**
     * aHandler supplies the session and records progress, the credentials offered and why a call
     * failed. aRemote is the caller's and must outlive the transport. A transport for a clone takes none.
     * Without aFollowRedirects a push the server redirects fails, so it reaches only the address the
     * caller checked. Older libgit2 versions cannot enforce this and use their default redirect behavior.
     */
    explicit REMOTE_TRANSPORT( KIGIT_REPO_MIXIN* aHandler, git_remote* aRemote = nullptr,
                               bool aFollowRedirects = true );
    ~REMOTE_TRANSPORT();

    REMOTE_TRANSPORT( const REMOTE_TRANSPORT& ) = delete;
    REMOTE_TRANSPORT& operator=( const REMOTE_TRANSPORT& ) = delete;

    /// Fetch as `git fetch` would. aBranches, when given, receives the branches the remote advertised
    int Fetch( std::vector<wxString>* aBranches = nullptr );

    /**
     * Connect, then push the refspec aConnected answers, so the caller can check what it pushes after a
     * connection that may have taken long. An empty answer disconnects and returns GIT_EUSER.
     */
    int Push( const std::function<wxString()>& aConnected );

    /// Connect, which proves the credentials and the host key without transferring anything
    int Check();

    /// Clone aUrl into aPath. The new repository is returned in aRepo for the caller to free
    int Clone( const wxString& aUrl, const wxString& aPath, git_repository** aRepo );

    /// libgit2's error for the last call that failed, read before anything else could replace it
    const KIGIT_ERROR_INFO& Error() const;

    struct STATE;

private:
    int connect( git_direction aDirection, const git_remote_callbacks& aCallbacks );

    std::unique_ptr<STATE> m_state;
    git_remote*            m_remote;
    bool                   m_followRedirects;
};

} // namespace KIGIT

#endif // KIGIT_TRANSPORT_H
