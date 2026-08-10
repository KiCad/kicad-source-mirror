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

#include <git/kigit_platform.h>
#include <git/kigit_remote_url.h>

#include <wx/app.h>
#include <wx/thread.h>


KIGIT::PLATFORM_SERVICES& KIGIT::PlatformServices()
{
    static PLATFORM_SERVICES services;

    return services;
}


void KIGIT::SaveCredentialAsync( const wxString& aUrl, const wxString& aUsername, const wxString& aPassword,
                                 std::function<void( bool aStored )> aDone )
{
    // A transfer reports the credential that worked from its own thread
    if( wxAppConsole* app = wxAppConsole::GetInstance(); app && !wxIsMainThread() )
    {
        app->CallAfter(
                [=]()
                {
                    SaveCredentialAsync( aUrl, aUsername, aPassword, aDone );
                } );
        return;
    }

    const PLATFORM_SERVICES& services = PlatformServices();
    const wxString           realm = CredentialRealm( aUrl );
    const wxString           url = ParseRemoteURL( aUrl ).url;

    if( !services.storeSecretAsync )
        return;

    if( !realm.IsEmpty() && services.setAccount )
        services.setAccount( realm, aUsername );

    services.storeSecretAsync( realm.IsEmpty() ? url : realm, aUsername, aPassword,
                               [realm, url, aUsername, aDone]( bool aStored )
                               {
                                   const PLATFORM_SERVICES& vault = PlatformServices();

                                   // The repository's own item is read first, so it would shadow the server's
                                   if( aStored && !realm.IsEmpty() && vault.deleteSecret )
                                       vault.deleteSecret( url, aUsername );

                                   if( aStored && !realm.IsEmpty() && vault.forgetRealm )
                                       vault.forgetRealm( realm );

                                   if( aDone )
                                       aDone( aStored );
                               } );
}
