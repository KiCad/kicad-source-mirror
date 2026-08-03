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


#include "mdw_credentials.h"
#include "mdw_session.h"

#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/kigit_platform.h>
#include <git/kigit_remote_url.h>
#include <trace_helpers.h>

#include <wx/log.h>

#include <string>


namespace
{

wxString field( const MDW_TYPED_CREDENTIALS& aCredentials, const wxString& aName )
{
    auto it = aCredentials.fields.find( aName );
    return it == aCredentials.fields.end() ? wxString() : it->second;
}


// The remote a git backend bound to the repository at aLocalRoot signs in to, read as that backend reads it
wxString repositoryRemote( const wxString& aLocalRoot )
{
    KIGIT::LIBGIT2_SCOPE libgit2;
    git_repository*      opened = nullptr;
    wxString             found;

    if( !aLocalRoot.IsEmpty() && git_repository_open_ext( &opened, aLocalRoot.utf8_str(), 0, nullptr ) == GIT_OK )
    {
        KIGIT::GitRepositoryPtr repo( opened );
        KIGIT_COMMON            common( opened );
        const wxString          name = common.GetRemoteNameOrDefault();

        for( const auto& [remote, url] : common.GetRemotes() )
        {
            if( remote == name )
                found = url;
        }
    }

    return found;
}


// Odoo's locator scheme stands for https
wxString serverUrl( const wxString& aLocator )
{
    return aLocator.Lower().StartsWith( wxS( "odoo://" ) ) ? wxS( "https" ) + aLocator.Mid( 4 ) : aLocator;
}

} // anonymous namespace


std::optional<MDW_TYPED_CREDENTIALS> MDW_CREDENTIALS::Find( const wxString& aRealm, const wxString& aUsername ) const
{
    const KIGIT::PLATFORM_SERVICES& services = KIGIT::PlatformServices();
    MDW_TYPED_CREDENTIALS           found;
    wxString                        username = aUsername;
    wxString                        password;

    if( aRealm.IsEmpty() )
        return std::nullopt;

    if( username.IsEmpty() && services.getAccount )
        username = services.getAccount( aRealm );

    if( username.IsEmpty() )
        return std::nullopt;

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        if( auto it = m_unsaved.find( { aRealm, username } ); it != m_unsaved.end() )
            return it->second;
    }

    if( !services.getSecretNoPrompt || !services.getSecretNoPrompt( aRealm, username, password ) )
        return std::nullopt;

    found.fields[wxS( "username" )] = username;
    found.fields[wxS( "password" )] = password;
    return found;
}


bool MDW_CREDENTIALS::Save( const wxString& aRealm, const MDW_TYPED_CREDENTIALS& aCredentials )
{
    const KIGIT::PLATFORM_SERVICES& services = KIGIT::PlatformServices();
    const wxString                  username = field( aCredentials, wxS( "username" ) );
    const wxString                  password = field( aCredentials, wxS( "password" ) );

    if( aRealm.IsEmpty() || username.IsEmpty() )
        return false;

    const bool stored =
            password.IsEmpty() || ( services.storeSecret && services.storeSecret( aRealm, username, password ) );

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        if( stored )
            m_unsaved.erase( { aRealm, username } );
        else
            m_unsaved[{ aRealm, username }] = aCredentials;
    }

    // A test of one binding as someone else must not re-point every other binding on the server
    if( services.setAccount && ( !services.getAccount || services.getAccount( aRealm ).IsEmpty() ) )
        services.setAccount( aRealm, username );

    if( m_onSave )
        m_onSave( aRealm, username );

    return stored;
}


MDW_RESULT MDW_CREDENTIALS::SignIn( MDW_SESSION& aSession, const MDW_TYPED_CREDENTIALS& aCredentials, int aTimeoutMs )
{
    MDW_RESULT result = aSession.Authenticate( aCredentials, aTimeoutMs );

    // A refused credential is never kept, and one the keychain cannot keep still serves this process
    if( result.IsOk() )
    {
        Save( aSession.Status().realm, aCredentials );
        aSession.SetUsername( field( aCredentials, wxS( "username" ) ) );
    }

    return result;
}


void MDW_CREDENTIALS::Forget( const wxString& aRealm )
{
    {
        std::lock_guard<std::mutex> lock( m_mutex );

        std::erase_if( m_unsaved,
                       [&]( const auto& aEntry )
                       {
                           return aEntry.first.first == aRealm;
                       } );
    }

    if( m_onSave )
        m_onSave( aRealm, wxEmptyString );
}


void MDW_CREDENTIALS::SetSaveListener(
        std::function<void( const wxString& aRealm, const wxString& aUsername )> aListener )
{
    m_onSave = std::move( aListener );
}


std::size_t MDW_CREDENTIALS::Fingerprint( const MDW_TYPED_CREDENTIALS& aCredentials )
{
    std::string joined;

    for( const auto& [name, value] : aCredentials.fields )
        joined += name.utf8_string() + '\0' + value.utf8_string() + '\0';

    return std::hash<std::string>()( joined );
}


void MDW_CREDENTIALS::JudgeStored( const wxString& aRealm, const MDW_TYPED_CREDENTIALS& aStored, bool aRefused )
{
    const std::pair<wxString, wxString> key( aRealm, field( aStored, wxS( "username" ) ) );
    std::lock_guard<std::mutex>         lock( m_mutex );

    if( aRefused )
        m_refused[key] = Fingerprint( aStored );
    else
        m_refused.erase( key );
}


bool MDW_CREDENTIALS::Refused( const wxString& aRealm, const MDW_TYPED_CREDENTIALS& aStored ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    auto                        it = m_refused.find( { aRealm, field( aStored, wxS( "username" ) ) } );

    return it != m_refused.end() && it->second == Fingerprint( aStored );
}


bool MDW_CREDENTIALS::LoginRefused( const wxString& aRealm, const wxString& aUsername ) const
{
    const KIGIT::PLATFORM_SERVICES& services = KIGIT::PlatformServices();
    wxString                        username = aUsername;

    if( username.IsEmpty() && services.getAccount )
        username = services.getAccount( aRealm );

    std::lock_guard<std::mutex> lock( m_mutex );
    return m_refused.contains( { aRealm, username } );
}


wxString MDW_CREDENTIALS::Realm( const wxString& aLocator )
{
    return KIGIT::CredentialRealm( serverUrl( aLocator ) );
}


wxString MDW_CREDENTIALS::BoundRealm( const wxString& aRealm, const wxString& aLocator, const wxString& aLocalRoot )
{
    const wxString realm = Realm( aRealm );
    const wxString bound = aLocator.IsEmpty() ? repositoryRemote( aLocalRoot ) : serverUrl( aLocator );

    if( realm.IsEmpty() || KIGIT::CredentialServer( realm ) == KIGIT::CredentialServer( bound ) )
        return realm;

    wxLogTrace( traceMdw, wxS( "A backend bound to %s named %s, which gets no stored credential" ),
                KIGIT::CredentialServer( bound ), KIGIT::CredentialServer( realm ) );
    return wxEmptyString;
}
