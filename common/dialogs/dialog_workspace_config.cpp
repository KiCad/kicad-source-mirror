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

#include <dialogs/dialog_workspace_config.h>

#include <dialogs/dialog_library_source_setup.h>
#include <dialogs/git/git_ui_utils.h>
#include <git/kicad_git_common.h>
#include <git/kigit_remote_url.h>
#include <git/kigit_ssh_keys.h>
#include <i18n_utility.h>
#include <mdw/client/mdw_library_sources.h>
#include <mdw/client/mdw_project_file.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <thread_pool.h>
#include <widgets/ui_common.h>

#include <algorithm>

#include <mdw/mdw_backend_supervisor.h>
#include <mdw/mdw_session.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choicdlg.h>
#include <wx/choice.h>
#include <wx/filepicker.h>
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/textdlg.h>


// Texts of the stock backends' manifests, listed for translation since only their keys are read from the manifest
[[maybe_unused]] static const wxString s_stockManifestTexts[] = {
    _HKI( "Name" ), _HKI( "A name for this workspace" ), _HKI( "Server address" ),
    _HKI( "https://odoo.example.com/?db=NAME&project=CODE" ) };


DIALOG_WORKSPACE_CONFIG::DIALOG_WORKSPACE_CONFIG( wxWindow* aParent, const wxString& aProjectPath,
                                                  const MDW_PROJECT_BINDING& aBinding, const wxString& aUsername,
                                                  const wxString& aSSHKey ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Configure Design Workspace" ), wxDefaultPosition,
                     wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_projectPath( aProjectPath ),
        m_binding( aBinding ),
        m_username( aUsername )
{
    const int margin = FromDIP( KIUI::GetStdMargin() );
    const int dlgMargin = 2 * margin;

    wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );

    wxFlexGridSizer* grid = nullptr;

    auto newGrid =
            [&]()
            {
                grid = new wxFlexGridSizer( 0, 2, margin, dlgMargin );
                grid->AddGrowableCol( 1 );
                grid->SetFlexibleDirection( wxHORIZONTAL );
                return grid;
            };

    auto addRow =
            [&]( const wxString& aLabel, wxWindow* aControl ) -> wxStaticText*
            {
                wxStaticText* label = new wxStaticText( this, wxID_ANY, aLabel );

                grid->Add( label, 0, wxALIGN_CENTER_VERTICAL );
                grid->Add( aControl, 1, wxEXPAND );
                return label;
            };

    topSizer->Add( newGrid(), 0, wxEXPAND | wxALL, dlgMargin );

    // The one row every backend shows, so the width holds when the binding rows hide
    m_backendChoice = new wxChoice( this, wxID_ANY );
    m_backendChoice->SetMinSize( FromDIP( wxSize( 360, -1 ) ) );
    addRow( _( "Backend:" ), m_backendChoice );

    m_workspaceIdCtrl = new wxTextCtrl( this, wxID_ANY );
    m_workspaceIdLabel = addRow( _( "Workspace ID:" ), m_workspaceIdCtrl );

    m_bindingCtrl = new wxTextCtrl( this, wxID_ANY );
    m_bindingLabel = addRow( _( "Binding:" ), m_bindingCtrl );

    m_noFieldsNote = new wxStaticText( this, wxID_ANY, wxEmptyString );
    topSizer->Add( m_noFieldsNote, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dlgMargin );

    topSizer->Add( new wxStaticText( this, wxID_ANY, _( "Authentication" ) ), 0, wxLEFT | wxRIGHT, dlgMargin );
    topSizer->AddSpacer( margin );
    topSizer->Add( new wxStaticLine( this ), 0, wxEXPAND | wxLEFT | wxRIGHT, dlgMargin );
    topSizer->Add( newGrid(), 0, wxEXPAND | wxALL, dlgMargin );

    // Unticked leaves the choice to the SSH agent and the default keys, as the git remote dialog does
    m_sshKeyCheck = new wxCheckBox( this, wxID_ANY, _( "SSH private key:" ) );
    m_sshKeyCheck->SetValue( !aSSHKey.IsEmpty() );
    m_sshKeyPicker = new wxFilePickerCtrl( this, wxID_ANY, aSSHKey, _( "Select SSH private key file" ),
                                           wxFileSelectorDefaultWildcardStr, wxDefaultPosition, wxDefaultSize,
                                           wxFLP_DEFAULT_STYLE | wxFLP_FILE_MUST_EXIST | wxFLP_OPEN );
    m_sshKeyPicker->SetInitialDirectory( KIGIT::SSHKeyFolder() );
    grid->Add( m_sshKeyCheck, 0, wxALIGN_CENTER_VERTICAL );
    grid->Add( m_sshKeyPicker, 1, wxEXPAND );

    m_usernameCtrl = new wxTextCtrl( this, wxID_ANY );
    addRow( _( "Username:" ), m_usernameCtrl );

    m_passwordCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                     wxDefaultSize, wxTE_PASSWORD );
    m_passwordLabel = addRow( _( "Password or token:" ), m_passwordCtrl );

    wxBoxSizer* testRow = new wxBoxSizer( wxHORIZONTAL );
    wxButton*   testButton = new wxButton( this, wxID_ANY, _( "Test Connection" ) );

    m_findProduct = new wxButton( this, wxID_ANY, _( "Find Product..." ) );
    m_findProduct->SetToolTip( _( "List the products this server offers, once the connection test passes" ) );
    testRow->Add( testButton, 0, wxRIGHT, margin );
    testRow->Add( m_findProduct, 0 );
    topSizer->Add( testRow, 0, wxLEFT | wxRIGHT | wxBOTTOM, dlgMargin );

    // A row of its own, so a long result wraps instead of running off the dialog
    m_statusText = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_statusText->Hide();
    topSizer->Add( m_statusText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dlgMargin );

    topSizer->Add( new wxStaticLine( this ), 0, wxEXPAND | wxLEFT | wxRIGHT, dlgMargin );

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    buttons->AddButton( new wxButton( this, wxID_OK ) );
    buttons->AddButton( new wxButton( this, wxID_CANCEL ) );
    buttons->Realize();
    topSizer->Add( buttons, 0, wxEXPAND | wxALL, dlgMargin );

    SetSizer( topSizer );

    testButton->Bind( wxEVT_BUTTON, &DIALOG_WORKSPACE_CONFIG::onTestConnection, this );
    m_sshKeyCheck->Bind( wxEVT_CHECKBOX,
                         [this]( wxCommandEvent& )
                         {
                             updateAuthControls();
                         } );
    m_findProduct->Bind( wxEVT_BUTTON, &DIALOG_WORKSPACE_CONFIG::onFindProduct, this );
    m_backendChoice->Bind( wxEVT_CHOICE,
                           [this]( wxCommandEvent& )
                           {
                               dropTested();
                               updateFields();
                           } );

    // The tested server is whatever these said, so a passing test no longer stands once they change
    for( wxTextCtrl* ctrl : { m_workspaceIdCtrl, m_bindingCtrl } )
    {
        ctrl->Bind( wxEVT_TEXT,
                    [this]( wxCommandEvent& aEvent )
                    {
                        dropTested();
                        aEvent.Skip();
                    } );
    }

    // Saved control state would keep the typed password in the common settings
    OptOut( this );
    SetupStandardButtons();

    finishDialogSettings();

    // Sized to its content and centred on whichever window opened it, never where it was last left
    resetSize();
    Centre( wxBOTH );
}


DIALOG_WORKSPACE_CONFIG::~DIALOG_WORKSPACE_CONFIG()
{
    dropTested();
}


wxString DIALOG_WORKSPACE_CONFIG::GetUsername() const
{
    return m_usernameCtrl->GetValue().Strip( wxString::both );
}


std::optional<wxString> DIALOG_WORKSPACE_CONFIG::GetSSHKey() const
{
    if( m_remoteType != KIGIT::GIT_CONN_TYPE::GIT_CONN_SSH )
        return std::nullopt;

    return sshKeyFile();
}


wxString DIALOG_WORKSPACE_CONFIG::sshKeyFile() const
{
    if( m_remoteType != KIGIT::GIT_CONN_TYPE::GIT_CONN_SSH || !m_sshKeyCheck->IsChecked() )
        return wxEmptyString;

    return KIGIT::PrivateKeyPath( m_sshKeyPicker->GetPath() );
}


bool DIALOG_WORKSPACE_CONFIG::reportUnusableKey()
{
    const wxString key = sshKeyFile();
    wxString       error;
    wxString       details;
    bool           encrypted = false;

    if( key.IsEmpty() || KIGIT::CheckSSHKey( key, error, details, encrypted ) )
        return false;

    SetWrappedStatus( this, m_statusText, error + wxS( ". " ) + details );
    return true;
}


void DIALOG_WORKSPACE_CONFIG::dropTested()
{
    if( m_tested )
    {
        // Shutdown waits on the backend, so it must not hold up the dialog
        GetKiCadThreadPool().detach_task(
                [session = std::move( m_tested )]()
                {
                    session->Supervisor()->Shutdown();
                } );
    }

    m_findProduct->Enable( false );
}


bool DIALOG_WORKSPACE_CONFIG::TransferDataToWindow()
{
    m_backendChoice->Clear();
    m_backends = MDW_SETUP::Backends();

    for( const MDW_BACKEND_CHOICE& backend : m_backends )
        m_backendChoice->Append( backend.display_name );

    // An unbound project has no backend to preselect, so it falls back to the configured default
    const wxString preferred = m_binding.backend_id.IsEmpty()
                                       ? Pgm().GetCommonSettings()->m_Workspace.default_backend_id
                                       : m_binding.backend_id;

    // A missing backend stays unselected, since another one cannot serve this binding
    int selected = m_binding.IsBound() ? wxNOT_FOUND : 0;

    for( size_t ii = 0; ii < m_backends.size(); ++ii )
    {
        if( m_backends[ii].backend_id == preferred )
            selected = static_cast<int>( ii );
    }

    if( !m_backends.empty() )
        m_backendChoice->SetSelection( selected );

    m_workspaceIdCtrl->SetValue( m_binding.workspace_id );
    m_bindingCtrl->SetValue( m_binding.binding );

    updateFields();

    PrefillCredentials( MDW_SETUP::Realm( m_projectPath, m_binding.binding ), m_username, m_usernameCtrl,
                        m_passwordCtrl );

    return true;
}


MDW_PROJECT_BINDING DIALOG_WORKSPACE_CONFIG::collectBinding() const
{
    MDW_PROJECT_BINDING binding;

    int sel = m_backendChoice->GetSelection();

    if( sel >= 0 && sel < static_cast<int>( m_backends.size() ) )
        binding.backend_id = m_backends[sel].backend_id;

    // A field the backend does not use keeps what the project file already holds for it, else nothing
    const bool same = binding.backend_id == m_binding.backend_id;

    auto valueOf = [&]( wxTextCtrl* aCtrl, const wxString& aHeld ) -> wxString
    {
        if( aCtrl->IsShown() )
            return aCtrl->GetValue();

        return same ? aHeld : wxString();
    };

    binding.workspace_id = valueOf( m_workspaceIdCtrl, m_binding.workspace_id );
    binding.binding = valueOf( m_bindingCtrl, m_binding.binding );

    return binding;
}


void DIALOG_WORKSPACE_CONFIG::updateFields()
{
    int sel = m_backendChoice->GetSelection();

    const MDW_BACKEND_CHOICE* backend =
            sel >= 0 && sel < static_cast<int>( m_backends.size() ) ? &m_backends[sel] : nullptr;

    // A manifest that does not say what it needs keeps both fields under their generic labels
    auto update = [&]( const wxString& aId, const wxString& aDefault, wxStaticText* aLabel, wxTextCtrl* aCtrl )
    {
        const MDW_BINDING_FIELD* declared = nullptr;
        bool                     shown = true;

        if( backend && backend->binding_fields )
        {
            auto it = std::find_if( backend->binding_fields->begin(), backend->binding_fields->end(),
                                    [&]( const MDW_BINDING_FIELD& aField )
                                    {
                                        return aField.id == aId;
                                    } );

            shown = it != backend->binding_fields->end();
            declared = shown ? &*it : nullptr;
        }

        aLabel->SetLabel( declared && !declared->label.IsEmpty() ? wxGetTranslation( declared->label ) + wxS( ":" )
                                                                 : aDefault );
        aCtrl->SetHint( declared ? wxGetTranslation( declared->hint ) : wxString() );
        aLabel->Show( shown );
        aCtrl->Show( shown );
    };

    update( wxS( "workspace_id" ), _( "Workspace ID:" ), m_workspaceIdLabel, m_workspaceIdCtrl );
    update( wxS( "binding" ), _( "Binding:" ), m_bindingLabel, m_bindingCtrl );

    m_findProduct->Show( backend && backend->binding_search );

    const bool      gitRemote = backend && backend->binding_source == MDW_BINDING_SOURCE::GIT_REMOTE;
    PROJECT*        project = gitRemote ? Pgm().GetSettingsManager().GetProject( m_projectPath ) : nullptr;
    git_repository* repo = nullptr;
    wxString        name;
    wxString        url;

    if( project && project->AttachGitRepository() )
    {
        std::shared_ptr<KIGIT_COMMON> git = project->GetGitCommon();

        repo = git->GetRepo();
        name = git->GetRemoteNameOrDefault();

        for( const auto& [remoteName, remoteUrl] : git->GetRemotes() )
        {
            if( remoteName == name )
                url = remoteUrl;
        }
    }

    wxString note;

    if( backend && backend->binding_fields && backend->binding_fields->empty() )
    {
        note = _( "This backend needs no more details." );

        if( gitRemote )
        {
            note = url.IsEmpty() ? _( "Uses this project's git remote, which is not set yet." )
                                 : wxString::Format( _( "Uses this project's git remote: %s (%s)" ), name, url );
        }
    }

    SetWrappedStatus( this, m_noFieldsNote, note );

    const KIGIT::REMOTE_URL parsed = KIGIT::ParseRemoteURL( url );

    m_remoteType.reset();
    m_remoteUsername = parsed.username;

    if( gitRemote )
        m_remoteType = url.IsEmpty() ? KIGIT::GIT_CONN_TYPE::GIT_CONN_LOCAL : parsed.connType;

    // As the git remote dialog does, an SSH remote starts from the key ssh itself would pick
    if( m_remoteType == KIGIT::GIT_CONN_TYPE::GIT_CONN_SSH && m_sshKeyPicker->GetPath().IsEmpty() )
    {
        std::vector<wxString> keys = KIGIT::DefaultSSHKeyCandidates( KIGIT::HomeDir(), parsed.hostname,
                                                                     KIGIT::ConfiguredSSHCommand( repo ) );

        if( !keys.empty() )
        {
            m_sshKeyCheck->SetValue( true );
            m_sshKeyPicker->SetPath( keys.front() );
        }
    }

    if( !parsed.username.IsEmpty() )
        m_usernameCtrl->ChangeValue( parsed.username );

    m_sshKeyCheck->Show( gitRemote );
    m_sshKeyPicker->Show( gitRemote );
    updateAuthControls();

    // The size the constructor fitted had every row shown, and hidden rows would leave a gap below the buttons
    GetSizer()->SetSizeHints( this );
}


void DIALOG_WORKSPACE_CONFIG::updateAuthControls()
{
    const bool ssh = m_remoteType == KIGIT::GIT_CONN_TYPE::GIT_CONN_SSH;
    const bool custom = ssh && m_sshKeyCheck->IsChecked();

    // A git remote with no transport yet has nothing to sign in to
    const bool local = m_remoteType == KIGIT::GIT_CONN_TYPE::GIT_CONN_LOCAL;

    m_sshKeyCheck->Enable( ssh );
    m_sshKeyPicker->Enable( custom );

    // An SSH username written into the address is the one that signs in, so the field cannot disagree with it
    m_usernameCtrl->Enable( ssh ? custom && m_remoteUsername.IsEmpty() : !local );
    m_passwordCtrl->Enable( ssh ? custom : !local );
    m_passwordLabel->SetLabel( ssh ? _( "SSH key password:" ) : _( "Password or token:" ) );
}


bool DIALOG_WORKSPACE_CONFIG::TransferDataFromWindow()
{
    m_result = collectBinding();

    if( !m_result.IsBound() )
    {
        SetWrappedStatus( this, m_statusText, _( "Select a backend to bind this project." ) );
        return false;
    }

    if( reportUnusableKey() )
        return false;

    return VerifyTypedLogin( this, m_statusText, m_result.backend_id, m_result.workspace_id, m_result.binding,
                             m_projectPath, m_usernameCtrl, m_passwordCtrl, sshKeyFile() );
}


void DIALOG_WORKSPACE_CONFIG::onTestConnection( wxCommandEvent& aEvent )
{
    MDW_PROJECT_BINDING binding = collectBinding();

    if( !binding.IsBound() )
    {
        SetWrappedStatus( this, m_statusText, _( "Select a backend first." ) );
        return;
    }

    if( reportUnusableKey() )
        return;

    dropTested();

    if( ShowConnectionTest( this, m_statusText, binding.backend_id, binding.workspace_id, binding.binding,
                            m_projectPath, m_usernameCtrl, m_passwordCtrl, &m_tested, sshKeyFile() )
                .IsOk() )
    {
        m_findProduct->Enable( m_findProduct->IsShown() );
    }
}


void DIALOG_WORKSPACE_CONFIG::onFindProduct( wxCommandEvent& aEvent )
{
    if( !m_tested )
        return;

    wxTextEntryDialog ask( this, _( "Product name or code. Leave empty to list them all." ), _( "Find Product" ) );

    if( ask.ShowModal() != wxID_OK )
        return;

    const wxString                     filter = ask.GetValue();
    const std::shared_ptr<MDW_SESSION> session = m_tested;

    auto [result, targets] = RunWithProgress<std::pair<MDW_RESULT, std::vector<MDW_BINDING_TARGET>>>(
            this, _( "Searching..." ),
            [=]()
            {
                return MDW_SETUP::FindBindingTargets( session, filter );
            } );

    if( result.error == MDW_ERROR_CODE::CANCELLED )
        return;

    if( !result.IsOk() )
    {
        const wxString why = result.error == MDW_ERROR_CODE::NOT_SUPPORTED ? _( "This server does not list them." )
                                                                           : MdwErrorText( result );

        SetWrappedStatus( this, m_statusText,
                          wxString::Format( _( "Could not list products. %s Enter the server address with the "
                                               "product in it instead." ),
                                            why ) );
        return;
    }

    if( targets.empty() )
    {
        SetWrappedStatus( this, m_statusText, _( "No product matches." ) );
        return;
    }

    wxArrayString names;

    for( const MDW_BINDING_TARGET& target : targets )
    {
        names.Add( target.display_name.IsEmpty() ? target.code
                                                 : target.code + wxS( "  " ) + target.display_name );
    }

    wxSingleChoiceDialog pick( this, _( "Choose the product this project belongs to." ), _( "Find Product" ), names );

    if( pick.ShowModal() != wxID_OK )
        return;

    // No event, since a changed locator would drop the session that listed it
    m_bindingCtrl->ChangeValue(
            MDW_SETUP::LocatorWithItem( m_bindingCtrl->GetValue(), targets[pick.GetSelection()].locator ) );
}


bool ApplyWorkspaceBinding( PROJECT& aProject, const wxString& aProjectPath, const MDW_PROJECT_BINDING& aBinding )
{
    bool saved = MDW_SAVE_GUARD::RecordBinding( Pgm().GetSettingsManager(), aProject, aBinding );

    // Listeners re-sync on the close, and a bound project then opens under the new binding or credentials
    MDW_SETUP::CloseProjectSession( aProjectPath );
    return saved;
}
