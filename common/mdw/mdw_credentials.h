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
 * @file mdw_credentials.h
 * @brief The credential a workspace signs in with, one per server and user.
 *
 * A backend never touches the keychain. Every project, library source and git remote signing in to one server as
 * one user shares the same credential.
 */

#ifndef MDW_CREDENTIALS_H
#define MDW_CREDENTIALS_H

#include <kicommon.h>
#include <mdw/client/mdw_auth.h>
#include <mdw/client/mdw_types.h>

#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <utility>

#include <wx/string.h>

class MDW_SESSION;

/**
 * The password lives in the platform keychain under (realm, username), the item stock git reads.
 * COMMON_SETTINGS::m_Workspace.accounts holds the server's default account for a binding that names no username.
 * Save() sets it only while it is empty. A credential the server accepted but the keychain refused is kept for
 * this process only.
 */
class KICOMMON_API MDW_CREDENTIALS
{
public:
    /**
     * The password saved for aUsername at aRealm, or for the server's default account when aUsername is empty,
     * never prompting; a locked keychain reads as none
     */
    std::optional<MDW_TYPED_CREDENTIALS> Find( const wxString& aRealm, const wxString& aUsername ) const;

    /**
     * Keep a credential the server accepted for aRealm. False when the keychain refused it, which still leaves
     * it serving this process. It may wait on an unlock prompt, so never call it on the UI thread.
     */
    bool Save( const wxString& aRealm, const MDW_TYPED_CREDENTIALS& aCredentials );

    /**
     * MDW_SESSION::Authenticate(), saving aCredentials once the server accepts them, after which the session
     * signs in again as their username. Never on the UI thread
     */
    MDW_RESULT SignIn( MDW_SESSION& aSession, const MDW_TYPED_CREDENTIALS& aCredentials, int aTimeoutMs );

    /// Drop what this run remembers for aRealm, once the keychain holds a password saved elsewhere
    void Forget( const wxString& aRealm );

    /// Tells one stored login from another without keeping its secret
    static std::size_t Fingerprint( const MDW_TYPED_CREDENTIALS& aCredentials );

    /// Record whether the server refused aStored as the stored login of its user at aRealm, for this run only
    void JudgeStored( const wxString& aRealm, const MDW_TYPED_CREDENTIALS& aStored, bool aRefused );

    /// Whether the server refused aStored as its user's stored login at aRealm
    bool Refused( const wxString& aRealm, const MDW_TYPED_CREDENTIALS& aStored ) const;

    /// Whether the last stored login of aUsername at aRealm, or of the server's account when empty, was refused
    bool LoginRefused( const wxString& aRealm, const wxString& aUsername ) const;

    /// Run after every Save() or Forget() with the realm and the username saved, empty after Forget()
    void SetSaveListener( std::function<void( const wxString& aRealm, const wxString& aUsername )> aListener );

    /// The realm a credential for the server aLocator or a backend's challenge names is kept under. Empty unless
    /// the server is HTTP(S), as only stock git keeps an SSH passphrase
    static wxString Realm( const wxString& aLocator );

    /**
     * The realm a backend bound to aLocator, or with none to the git repository at aLocalRoot, may sign in with
     * when it names aRealm. Empty when aRealm is not HTTP(S) or is on another server, so no stored credential
     * reaches it.
     */
    static wxString BoundRealm( const wxString& aRealm, const wxString& aLocator, const wxString& aLocalRoot );

private:
    mutable std::mutex m_mutex;

    /// The fingerprint of the stored login the server last refused, by realm and username
    std::map<std::pair<wxString, wxString>, std::size_t> m_refused;

    std::function<void( const wxString& aRealm, const wxString& aUsername )> m_onSave;

    /// Accepted but refused by the keychain, by realm and username
    std::map<std::pair<wxString, wxString>, MDW_TYPED_CREDENTIALS> m_unsaved;
};

#endif // MDW_CREDENTIALS_H
