/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/git/mdw_connection_prompt.h>

#include <dialog_shim.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <git/kigit_known_hosts.h>
#include <git/kigit_platform.h>
#include <i18n_utility.h>
#include <mdw/client/mdw_library_sources.h>

#include <wx/button.h>
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/weakref.h>

namespace
{

// Backends name their fields in English, so the names this form knows are extracted for translation
[[maybe_unused]] const wxChar* const c_fieldNames[] = { _HKI( "Username" ), _HKI( "Password" ), _HKI( "Passphrase" ),
                                                        _HKI( "Login" ), _HKI( "API key" ) };


/// The username and password the user typed into the form aStatus names, or none when they cancelled
std::optional<MDW_TYPED_CREDENTIALS> askCredentials( wxWindow* aParent, const MDW_CONNECTION_STATUS& aStatus )
{
    const std::optional<MDW_AUTH_CHALLENGE>& challenge = aStatus.reason.auth_challenge;

    // The backend may name the fields, as Odoo calls its password an API key
    auto label = [&]( const wxString& aFieldId, const wxString& aDefault )
    {
        if( challenge )
        {
            for( const MDW_AUTH_FIELD& field : challenge->fields )
            {
                if( field.field_id == aFieldId && !field.display_name.IsEmpty() )
                {
                    const wxString name = wxGetTranslation( field.display_name );
                    return name.EndsWith( wxS( ":" ) ) ? name : name + wxS( ":" );
                }
            }
        }

        return aDefault;
    };

    DIALOG_SHIM dlg( aParent, wxID_ANY, _( "Sign In" ) );
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    wxString    intro = aStatus.realm.IsEmpty() ? _( "Sign in to the workspace server." )
                                                : wxString::Format( _( "Sign in to %s." ), aStatus.realm );

    sizer->Add( new wxStaticText( &dlg, wxID_ANY, intro ), 0, wxALL, 10 );

    if( !aStatus.reason.IsOk() )
    {
        wxStaticText* reason = new wxStaticText( &dlg, wxID_ANY, MdwErrorMessage( aStatus.reason ) );

        reason->SetToolTip( MdwErrorText( aStatus.reason ) );
        sizer->Add( reason, 0, wxLEFT | wxRIGHT, 10 );
    }

    wxFlexGridSizer* grid = new wxFlexGridSizer( 2, 5, 5 );
    grid->AddGrowableCol( 1 );

    wxString hint = challenge ? challenge->username_hint : wxString();

    if( hint.IsEmpty() )
        hint = MDW_SETUP::Username( aStatus.realm, aStatus.username );

    if( hint.IsEmpty() )
        hint = aStatus.account.user_id;

    wxTextCtrl* username = new wxTextCtrl( &dlg, wxID_ANY, hint, wxDefaultPosition, wxSize( dlg.FromDIP( 260 ), -1 ) );
    wxTextCtrl* password =
            new wxTextCtrl( &dlg, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );

    grid->Add( new wxStaticText( &dlg, wxID_ANY, label( wxS( "username" ), _( "Username:" ) ) ), 0,
               wxALIGN_CENTER_VERTICAL );
    grid->Add( username, 1, wxEXPAND );
    grid->Add( new wxStaticText( &dlg, wxID_ANY, label( wxS( "password" ), _( "Password:" ) ) ), 0,
               wxALIGN_CENTER_VERTICAL );
    grid->Add( password, 1, wxEXPAND );
    sizer->Add( grid, 0, wxALL | wxEXPAND, 10 );

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    wxButton*               ok = new wxButton( &dlg, wxID_OK, _( "Sign In" ) );
    ok->SetDefault();
    buttons->AddButton( ok );
    buttons->AddButton( new wxButton( &dlg, wxID_CANCEL ) );
    buttons->Realize();
    sizer->Add( buttons, 0, wxALL | wxEXPAND, 10 );

    // The title-keyed dialog state is shared across realms and would store the typed password
    dlg.OptOut( &dlg );
    dlg.SetSizerAndFit( sizer );
    dlg.SetInitialFocus( username->IsEmpty() ? username : password );

    wxWeakRef<wxWindow> parent( aParent );

    // A parent closed while the prompt was up has nobody left to sign in for
    if( dlg.ShowModal() != wxID_OK || ( aParent && !parent ) )
        return std::nullopt;

    MDW_TYPED_CREDENTIALS credentials;
    credentials.fields[wxS( "username" )] = username->GetValue();
    credentials.fields[wxS( "password" )] = password->GetValue();
    return credentials;
}


void signIn( wxWindow* aParent, const MDW_CONNECTION_STATUS& aStatus,
             const std::function<void( const MDW_CONNECTION_ANSWER& )>& aRetry )
{
    MDW_CONNECTION_ANSWER answer;
    answer.credentials = askCredentials( aParent, aStatus );

    if( answer.credentials )
        aRetry( answer );
}

} // namespace


void MdwResolveConnection( wxWindow* aParent, const MDW_CONNECTION_STATUS& aStatus,
                           std::function<void( const MDW_CONNECTION_ANSWER& )> aRetry )
{
    MDW_CONNECTION_ANSWER answer;

    if( aStatus.phase == MDW_CONNECTION::TRUST_NEEDED && aStatus.reason.host_key )
    {
        const MDW_HOST_KEY&     key = *aStatus.reason.host_key;
        KIGIT::HOST_KEY_FAILURE failure;
        failure.host = key.host;
        failure.port = static_cast<int>( key.port );
        failure.keyType = key.key_type;
        failure.fingerprint = key.sha256_fingerprint;

        if( KIGIT::AskToTrustHost( aParent, failure, false ) != KIGIT::PROMPT_ANSWER::ACCEPTED )
            return;

        answer.trust = key;
    }
    else if( aStatus.phase == MDW_CONNECTION::SIGN_IN_NEEDED )
    {
        const wxString username = MDW_SETUP::Username( aStatus.realm, aStatus.username );
        const auto&    getSecret = KIGIT::PlatformServices().getSecretAsync;

        // A login stored since the session last tried is sent as it stands, and only a refused one opens the form
        if( !username.IsEmpty() && !aStatus.storedLoginRefused && MDW_SETUP::HasCredential( aStatus.realm, username ) )
        {
            aRetry( MDW_CONNECTION_ANSWER() );
            return;
        }

        // A locked keychain reads as empty without asking, so it is asked to unlock first
        // A refused stored login is never resent, so it goes straight to the form
        if( !username.IsEmpty() && getSecret && !aStatus.storedLoginRefused )
        {
            getSecret( aStatus.realm, username,
                       [parent = wxWeakRef<wxWindow>( aParent ), hadParent = aParent != nullptr, aStatus, aRetry,
                        username]( bool aFound, const wxString& aSecret )
                       {
                           if( hadParent && !parent )
                               return;

                           if( !aFound )
                           {
                               signIn( parent, aStatus, aRetry );
                               return;
                           }

                           // A keychain item that distrusts this build answers only this prompt, never a read
                           MDW_CONNECTION_ANSWER unlocked;
                           unlocked.stored.emplace().fields = { { wxS( "username" ), username },
                                                                { wxS( "password" ), aSecret } };
                           aRetry( unlocked );
                       } );
            return;
        }

        signIn( aParent, aStatus, aRetry );
        return;
    }
    else
    {
        return;
    }

    aRetry( answer );
}
