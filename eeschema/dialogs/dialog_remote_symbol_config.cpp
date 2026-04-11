/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dialog_remote_symbol_config.h"

#include <common.h>
#include <oauth/secure_token_store.h>
#include <kiplatform/environment.h>
#include <pgm_base.h>
#include <remote_provider_client.h>
#include <settings/settings_manager.h>
#include <widgets/ui_common.h>

#include <wx/button.h>
#include <wx/dirdlg.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>


DIALOG_REMOTE_SYMBOL_CONFIG::DIALOG_REMOTE_SYMBOL_CONFIG( wxWindow* aParent ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Remote Symbol Settings" ), wxDefaultPosition,
                     wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_providerList( nullptr ),
        m_providerUrlCtrl( nullptr ),
        m_providerNameCtrl( nullptr ),
        m_providerAccountLabel( nullptr ),
        m_providerAuthLabel( nullptr ),
        m_addProviderButton( nullptr ),
        m_removeProviderButton( nullptr ),
        m_refreshProviderButton( nullptr ),
        m_signOutProviderButton( nullptr ),
        m_destinationCtrl( nullptr ),
        m_prefixCtrl( nullptr ),
        m_prefixHint( nullptr ),
        m_projectRadio( nullptr ),
        m_globalRadio( nullptr ),
        m_resetButton( nullptr ),
        m_browseButton( nullptr ),
        m_settings( GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
{
    wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );

    wxStaticText* intro = new wxStaticText( this, wxID_ANY,
            _( "Configure remote symbol providers and where downloaded libraries are stored." ) );
    intro->Wrap( FromDIP( 460 ) );
    topSizer->Add( intro, 0, wxALL | wxEXPAND, FromDIP( 10 ) );

    wxStaticBoxSizer* providerSizer = new wxStaticBoxSizer( wxVERTICAL, this, _( "Providers" ) );

    wxBoxSizer* providerTopSizer = new wxBoxSizer( wxHORIZONTAL );
    m_providerList = new wxListBox( this, wxID_ANY );
    m_providerList->SetMinSize( FromDIP( wxSize( 280, 150 ) ) );
    providerTopSizer->Add( m_providerList, 1, wxRIGHT | wxEXPAND, FromDIP( 8 ) );

    wxBoxSizer* providerButtonSizer = new wxBoxSizer( wxVERTICAL );
    m_addProviderButton = new wxButton( this, wxID_ANY, _( "Add / Update" ) );
    m_removeProviderButton = new wxButton( this, wxID_ANY, _( "Remove" ) );
    m_refreshProviderButton = new wxButton( this, wxID_ANY, _( "Refresh Metadata" ) );
    m_signOutProviderButton = new wxButton( this, wxID_ANY, _( "Sign Out" ) );
    providerButtonSizer->Add( m_addProviderButton, 0, wxBOTTOM | wxEXPAND, FromDIP( 4 ) );
    providerButtonSizer->Add( m_removeProviderButton, 0, wxBOTTOM | wxEXPAND, FromDIP( 4 ) );
    providerButtonSizer->Add( m_refreshProviderButton, 0, wxBOTTOM | wxEXPAND, FromDIP( 4 ) );
    providerButtonSizer->Add( m_signOutProviderButton, 0, wxEXPAND );
    providerTopSizer->Add( providerButtonSizer, 0, wxEXPAND );
    providerSizer->Add( providerTopSizer, 0, wxALL | wxEXPAND, FromDIP( 8 ) );

    wxFlexGridSizer* providerGrid = new wxFlexGridSizer( 2, FromDIP( 6 ), FromDIP( 6 ) );
    providerGrid->AddGrowableCol( 1, 1 );
    providerGrid->Add( new wxStaticText( this, wxID_ANY, _( "Metadata URL:" ) ),
                       0, wxALIGN_CENTER_VERTICAL );
    m_providerUrlCtrl = new wxTextCtrl( this, wxID_ANY );
    providerGrid->Add( m_providerUrlCtrl, 1, wxEXPAND );
    providerGrid->Add( new wxStaticText( this, wxID_ANY, _( "Display name:" ) ),
                       0, wxALIGN_CENTER_VERTICAL );
    m_providerNameCtrl = new wxTextCtrl( this, wxID_ANY );
    providerGrid->Add( m_providerNameCtrl, 1, wxEXPAND );
    providerGrid->Add( new wxStaticText( this, wxID_ANY, _( "Last account:" ) ),
                       0, wxALIGN_CENTER_VERTICAL );
    m_providerAccountLabel = new wxStaticText( this, wxID_ANY, _( "Not signed in" ) );
    providerGrid->Add( m_providerAccountLabel, 1, wxEXPAND );
    providerGrid->Add( new wxStaticText( this, wxID_ANY, _( "Auth status:" ) ),
                       0, wxALIGN_CENTER_VERTICAL );
    m_providerAuthLabel = new wxStaticText( this, wxID_ANY, _( "Not configured" ) );
    providerGrid->Add( m_providerAuthLabel, 1, wxEXPAND );
    providerSizer->Add( providerGrid, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, FromDIP( 8 ) );

    topSizer->Add( providerSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, FromDIP( 10 ) );

    wxFlexGridSizer* gridSizer = new wxFlexGridSizer( 2, FromDIP( 6 ), FromDIP( 6 ) );
    gridSizer->AddGrowableCol( 1, 1 );

    gridSizer->Add( new wxStaticText( this, wxID_ANY, _( "Destination directory:" ) ),
                    0, wxALIGN_CENTER_VERTICAL );

    wxBoxSizer* destSizer = new wxBoxSizer( wxHORIZONTAL );
    m_destinationCtrl = new wxTextCtrl( this, wxID_ANY );
    m_destinationCtrl->SetMinSize( FromDIP( wxSize( 320, -1 ) ) );
    m_destinationCtrl->SetToolTip( _(
            "Directory where downloaded symbol, footprint, and 3D model data will be written." ) );
    destSizer->Add( m_destinationCtrl, 1, wxALIGN_CENTER_VERTICAL );

    m_browseButton = new wxButton( this, wxID_ANY, _( "Browse..." ) );
    destSizer->Add( m_browseButton, 0, wxLEFT, FromDIP( 4 ) );
    gridSizer->Add( destSizer, 1, wxEXPAND );

    gridSizer->Add( new wxStaticText( this, wxID_ANY, _( "Library prefix:" ) ),
                    0, wxALIGN_CENTER_VERTICAL );
    m_prefixCtrl = new wxTextCtrl( this, wxID_ANY );
    m_prefixCtrl->SetToolTip( _( "Prefix that will be applied to the generated libraries." ) );
    gridSizer->Add( m_prefixCtrl, 0, wxEXPAND );

    gridSizer->AddSpacer( 0 );
    m_prefixHint = new wxStaticText( this, wxID_ANY, wxString() );
    m_prefixHint->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );
    gridSizer->Add( m_prefixHint, 0, wxEXPAND );

    gridSizer->Add( new wxStaticText( this, wxID_ANY, _( "Add libraries to:" ) ),
                    0, wxALIGN_CENTER_VERTICAL );

    wxBoxSizer* radioSizer = new wxBoxSizer( wxHORIZONTAL );
    m_projectRadio = new wxRadioButton( this, wxID_ANY, _( "Project library table" ),
                                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_projectRadio->SetToolTip(
            _( "Adds the generated libraries to the project's library tables." ) );
    radioSizer->Add( m_projectRadio, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, FromDIP( 12 ) );

    m_globalRadio = new wxRadioButton( this, wxID_ANY, _( "Global library table" ) );
    m_globalRadio->SetToolTip(
            _( "Adds the generated libraries to the global library tables." ) );
    radioSizer->Add( m_globalRadio, 0, wxALIGN_CENTER_VERTICAL );

    gridSizer->Add( radioSizer, 0, wxEXPAND );

    topSizer->Add( gridSizer, 0, wxALL | wxEXPAND, FromDIP( 10 ) );

    m_resetButton = new wxButton( this, wxID_ANY, _( "Reset to Defaults" ) );
    m_resetButton->SetToolTip( _( "Restore the default destination and prefix." ) );
    topSizer->Add( m_resetButton, 0, wxLEFT | wxBOTTOM, FromDIP( 10 ) );

    wxStdDialogButtonSizer* buttonSizer = CreateStdDialogButtonSizer( wxOK | wxCANCEL );
    topSizer->Add( buttonSizer, 0, wxALL | wxEXPAND, FromDIP( 10 ) );

    SetSizer( topSizer );
    topSizer->Fit( this );

    SetupStandardButtons();
    finishDialogSettings();

    SetInitialFocus( m_providerUrlCtrl );

    m_browseButton->Bind( wxEVT_BUTTON, &DIALOG_REMOTE_SYMBOL_CONFIG::onBrowseDestination, this );
    m_resetButton->Bind( wxEVT_BUTTON, &DIALOG_REMOTE_SYMBOL_CONFIG::onResetDefaults, this );
    m_prefixCtrl->Bind( wxEVT_TEXT, &DIALOG_REMOTE_SYMBOL_CONFIG::onPrefixChanged, this );
    m_providerList->Bind( wxEVT_LISTBOX, &DIALOG_REMOTE_SYMBOL_CONFIG::onProviderSelected, this );
    m_addProviderButton->Bind( wxEVT_BUTTON, &DIALOG_REMOTE_SYMBOL_CONFIG::onAddProvider, this );
    m_removeProviderButton->Bind( wxEVT_BUTTON, &DIALOG_REMOTE_SYMBOL_CONFIG::onRemoveProvider,
                                  this );
    m_refreshProviderButton->Bind( wxEVT_BUTTON,
                                   &DIALOG_REMOTE_SYMBOL_CONFIG::onRefreshProvider, this );
    m_signOutProviderButton->Bind( wxEVT_BUTTON,
                                   &DIALOG_REMOTE_SYMBOL_CONFIG::onSignOutProvider, this );
}


bool DIALOG_REMOTE_SYMBOL_CONFIG::TransferDataToWindow()
{
    if( m_settings )
        applyRemoteSettings( m_settings->m_RemoteSymbol );
    else
        applyRemoteSettings( REMOTE_PROVIDER_SETTINGS() );

    return true;
}


bool DIALOG_REMOTE_SYMBOL_CONFIG::TransferDataFromWindow()
{
    wxString destination = m_destinationCtrl->GetValue();
    wxString prefix = m_prefixCtrl->GetValue();

    destination.Trim( true ).Trim( false );
    prefix.Trim( true ).Trim( false );

    if( destination.IsEmpty() )
    {
        wxMessageBox( _( "Please choose a destination directory." ), GetTitle(),
                      wxOK | wxICON_WARNING, this );
        m_destinationCtrl->SetFocus();
        return false;
    }

    if( prefix.IsEmpty() )
    {
        wxMessageBox( _( "Please enter a library prefix." ), GetTitle(), wxOK | wxICON_WARNING,
                      this );
        m_prefixCtrl->SetFocus();
        return false;
    }

    if( !m_settings )
        m_settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    if( !m_settings )
        return true;

    commitProviderEdits();

    m_remoteSettings.destination_dir = destination;
    m_remoteSettings.library_prefix = prefix;
    m_remoteSettings.add_to_global_table = m_globalRadio->GetValue();
    m_settings->m_RemoteSymbol = m_remoteSettings;
    return true;
}


void DIALOG_REMOTE_SYMBOL_CONFIG::onBrowseDestination( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );

    wxString initialPath = ExpandEnvVarSubstitutions( m_destinationCtrl->GetValue(), &Prj() );

    if( initialPath.IsEmpty() )
    {
        if( Prj().IsNullProject() )
            initialPath = KIPLATFORM::ENV::GetDocumentsPath();
        else
            initialPath = Prj().GetProjectPath();
    }

    wxDirDialog dlg( this, _( "Select Destination Directory" ), initialPath );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString path = dlg.GetPath();

    if( !Prj().IsNullProject() )
    {
        wxString projectRoot = Prj().GetProjectPath();
        wxFileName selectedDir = wxFileName::DirName( path );

        if( selectedDir.MakeRelativeTo( projectRoot ) )
        {
            wxString relative = selectedDir.GetFullPath();

            if( relative.IsEmpty() || relative == wxS( "." ) )
                path = wxS( "${KIPRJMOD}" );
            else
                path = wxString::Format( wxS( "${KIPRJMOD}/%s" ), relative );
        }
    }

    path.Replace( wxS( "\\" ), wxS( "/" ) );
    m_destinationCtrl->ChangeValue( path );
}


void DIALOG_REMOTE_SYMBOL_CONFIG::onResetDefaults( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );
    applyRemoteSettings( REMOTE_PROVIDER_SETTINGS() );
}


void DIALOG_REMOTE_SYMBOL_CONFIG::onPrefixChanged( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );
    updatePrefixHint();
}


void DIALOG_REMOTE_SYMBOL_CONFIG::onProviderSelected( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );
    updateProviderEditor();
    updateProviderButtons();
}


void DIALOG_REMOTE_SYMBOL_CONFIG::onAddProvider( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );

    wxString url = m_providerUrlCtrl->GetValue();
    wxString name = m_providerNameCtrl->GetValue();
    url.Trim( true ).Trim( false );
    name.Trim( true ).Trim( false );

    if( url.IsEmpty() )
    {
        wxMessageBox( _( "Enter a provider metadata URL first." ), GetTitle(),
                      wxOK | wxICON_WARNING, this );
        m_providerUrlCtrl->SetFocus();
        return;
    }

    REMOTE_PROVIDER_ENTRY& provider = m_remoteSettings.UpsertProvider( url );
    provider.metadata_url = url;
    provider.display_name_override = name;

    if( provider.last_auth_status.IsEmpty() )
        provider.last_auth_status = wxS( "signed_out" );

    m_remoteSettings.last_used_provider_id = provider.provider_id;
    reloadProviderList();

    for( size_t ii = 0; ii < m_remoteSettings.providers.size(); ++ii )
    {
        if( m_remoteSettings.providers[ii].provider_id == provider.provider_id )
        {
            m_providerList->SetSelection( static_cast<int>( ii ) );
            break;
        }
    }

    updateProviderEditor();
    updateProviderButtons();
}


void DIALOG_REMOTE_SYMBOL_CONFIG::onRemoveProvider( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );

    const int selection = m_providerList->GetSelection();

    if( selection == wxNOT_FOUND )
        return;

    const wxString removedId = m_remoteSettings.providers[selection].provider_id;
    m_remoteSettings.providers.erase( m_remoteSettings.providers.begin() + selection );

    if( m_remoteSettings.last_used_provider_id == removedId )
        m_remoteSettings.last_used_provider_id.clear();

    reloadProviderList();
}


void DIALOG_REMOTE_SYMBOL_CONFIG::onRefreshProvider( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );

    const int selection = m_providerList->GetSelection();

    if( selection == wxNOT_FOUND )
        return;

    REMOTE_PROVIDER_CLIENT  client;
    REMOTE_PROVIDER_METADATA metadata;
    REMOTE_PROVIDER_ERROR    error;

    if( !client.DiscoverProvider( m_remoteSettings.providers[selection].metadata_url, metadata, error ) )
    {
        wxMessageBox( error.message, GetTitle(), wxOK | wxICON_ERROR, this );
        return;
    }

    if( m_remoteSettings.providers[selection].display_name_override.IsEmpty() )
        m_providerNameCtrl->ChangeValue( metadata.provider_name );

    wxMessageBox( _( "Provider metadata refreshed." ), GetTitle(), wxOK | wxICON_INFORMATION, this );
}


void DIALOG_REMOTE_SYMBOL_CONFIG::onSignOutProvider( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );

    const int selection = m_providerList->GetSelection();

    if( selection == wxNOT_FOUND )
        return;

    SECURE_TOKEN_STORE tokenStore;
    REMOTE_PROVIDER_ENTRY& provider = m_remoteSettings.providers[selection];
    tokenStore.DeleteTokens( provider.provider_id, wxS( "default" ) );
    provider.last_account_label.clear();
    provider.last_auth_status = wxS( "signed_out" );
    updateProviderEditor();
}


void DIALOG_REMOTE_SYMBOL_CONFIG::applyRemoteSettings( const REMOTE_PROVIDER_SETTINGS& aConfig )
{
    m_remoteSettings = aConfig;
    m_destinationCtrl->ChangeValue( aConfig.destination_dir );
    m_prefixCtrl->ChangeValue( aConfig.library_prefix );

    if( aConfig.add_to_global_table )
        m_globalRadio->SetValue( true );
    else
        m_projectRadio->SetValue( true );

    reloadProviderList();
    updatePrefixHint();
}


void DIALOG_REMOTE_SYMBOL_CONFIG::reloadProviderList()
{
    m_providerList->Clear();

    for( const REMOTE_PROVIDER_ENTRY& provider : m_remoteSettings.providers )
    {
        wxString label = provider.display_name_override.IsEmpty()
                                 ? provider.metadata_url
                                 : wxString::Format( wxS( "%s  [%s]" ),
                                                     provider.display_name_override,
                                                     provider.metadata_url );

        m_providerList->Append( label );
    }

    if( !m_remoteSettings.last_used_provider_id.IsEmpty() )
    {
        for( size_t ii = 0; ii < m_remoteSettings.providers.size(); ++ii )
        {
            if( m_remoteSettings.providers[ii].provider_id == m_remoteSettings.last_used_provider_id )
            {
                m_providerList->SetSelection( static_cast<int>( ii ) );
                break;
            }
        }
    }

    updateProviderEditor();
    updateProviderButtons();
}


void DIALOG_REMOTE_SYMBOL_CONFIG::updateProviderEditor()
{
    const int selection = m_providerList->GetSelection();

    if( selection == wxNOT_FOUND )
    {
        m_providerUrlCtrl->ChangeValue( wxEmptyString );
        m_providerNameCtrl->ChangeValue( wxEmptyString );
        m_providerAccountLabel->SetLabel( _( "Not signed in" ) );
        m_providerAuthLabel->SetLabel( _( "Not configured" ) );
        return;
    }

    const REMOTE_PROVIDER_ENTRY& provider = m_remoteSettings.providers[selection];
    m_providerUrlCtrl->ChangeValue( provider.metadata_url );
    m_providerNameCtrl->ChangeValue( provider.display_name_override );
    m_providerAccountLabel->SetLabel( provider.last_account_label.IsEmpty()
                                              ? _( "Not signed in" )
                                              : provider.last_account_label );
    m_providerAuthLabel->SetLabel( provider.last_auth_status.IsEmpty()
                                           ? _( "Unknown" )
                                           : provider.last_auth_status );
}


void DIALOG_REMOTE_SYMBOL_CONFIG::updateProviderButtons()
{
    const bool hasSelection = m_providerList->GetSelection() != wxNOT_FOUND;
    m_removeProviderButton->Enable( hasSelection );
    m_refreshProviderButton->Enable( hasSelection );
    m_signOutProviderButton->Enable( hasSelection );
}


void DIALOG_REMOTE_SYMBOL_CONFIG::commitProviderEdits()
{
    const int selection = m_providerList->GetSelection();

    if( selection == wxNOT_FOUND || selection >= static_cast<int>( m_remoteSettings.providers.size() ) )
        return;

    REMOTE_PROVIDER_ENTRY& provider = m_remoteSettings.providers[selection];
    wxString url = m_providerUrlCtrl->GetValue();
    wxString name = m_providerNameCtrl->GetValue();
    url.Trim( true ).Trim( false );
    name.Trim( true ).Trim( false );

    if( url.IsEmpty() )
        return;

    provider.metadata_url = url;
    provider.display_name_override = name;

    if( provider.provider_id.IsEmpty() )
        provider.provider_id = REMOTE_PROVIDER_SETTINGS::CreateProviderId( url );
}


void DIALOG_REMOTE_SYMBOL_CONFIG::updatePrefixHint()
{
    const wxString prefix = m_prefixCtrl->GetValue();

    if( prefix.IsEmpty() )
    {
        m_prefixHint->SetLabel(
                _( "Library names will be created with suffixes such as _symbols, _fp, and _3d." ) );
    }
    else
    {
        m_prefixHint->SetLabel( wxString::Format(
                _( "Will create libraries like %1$s_symbols, %1$s_fp, %1$s_3d, etc." ), prefix ) );
    }

    m_prefixHint->Wrap( FromDIP( 360 ) );
}
