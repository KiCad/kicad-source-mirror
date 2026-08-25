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

#include <dialogs/dialog_library_source_setup.h>

#include <confirm.h>
#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/merge_conflict_prompt.h>
#include <dialogs/git/mdw_connection_prompt.h>
#include <gestfich.h>
#include <kidialog.h>
#include <kiway.h>
#include <lib_id.h>
#include <kiway_mail.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <mail_type.h>
#include <mdw/client/mdw_overwritten_parts.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <scoped_set_reset.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <widgets/mdw_editing.h>
#include <widgets/mdw_part_controller.h>
#include <widgets/ui_common.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <optional>
#include <set>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/frame.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/weakref.h>
#include <wx/intl.h>


DIALOG_LIBRARY_SOURCE_SETUP::DIALOG_LIBRARY_SOURCE_SETUP( wxWindow*                                     aParent,
                                                          const std::optional<MDW_LIBRARY_REQUIREMENT>& aRequirement,
                                                          const std::vector<MDW_LIBRARY_REQUIREMENT>&   aProjectNeeds,
                                                          SET_UP                                        aSetUp ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Set Up Team Library" ), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_requirement( aRequirement ),
        m_projectNeeds( aProjectNeeds ),
        m_setUp( std::move( aSetUp ) )
{
    const int margin = FromDIP( KIUI::GetStdMargin() );
    const int dlgMargin = 2 * margin;

    wxBoxSizer*      topSizer = new wxBoxSizer( wxVERTICAL );
    wxFlexGridSizer* grid = new wxFlexGridSizer( 0, 2, margin, dlgMargin );
    grid->AddGrowableCol( 1 );
    grid->SetFlexibleDirection( wxHORIZONTAL );

    auto addRow = [&]( const wxString& aLabel, wxWindow* aControl )
    {
        grid->Add( new wxStaticText( this, wxID_ANY, aLabel ), 0, wxALIGN_CENTER_VERTICAL );
        grid->Add( aControl, 1, wxEXPAND );
    };

    m_backendChoice = new wxChoice( this, wxID_ANY );
    addRow( _( "Backend:" ), m_backendChoice );

    m_locatorCtrl = new wxTextCtrl( this, wxID_ANY );
    m_locatorCtrl->SetMinSize( FromDIP( wxSize( 360, -1 ) ) );
    addRow( _( "Location:" ), m_locatorCtrl );

    m_nameCtrl = new wxTextCtrl( this, wxID_ANY );
    addRow( _( "Name:" ), m_nameCtrl );

    m_prefixCtrl = new wxTextCtrl( this, wxID_ANY );
    addRow( _( "Nickname prefix:" ), m_prefixCtrl );

    m_usernameCtrl = new wxTextCtrl( this, wxID_ANY );
    addRow( _( "Username:" ), m_usernameCtrl );

    m_passwordCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    addRow( _( "Password:" ), m_passwordCtrl );

    topSizer->Add( grid, 0, wxEXPAND | wxALL, dlgMargin );

    wxButton* testButton = new wxButton( this, wxID_ANY, _( "Test Connection" ) );
    topSizer->Add( testButton, 0, wxLEFT | wxRIGHT | wxBOTTOM, dlgMargin );

    // A row of its own, so a long result wraps instead of running off the dialog
    m_statusText = new wxStaticText( this, wxID_ANY, wxEmptyString );
    topSizer->Add( m_statusText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, dlgMargin );

    topSizer->Add( new wxStaticLine( this ), 0, wxEXPAND | wxLEFT | wxRIGHT, dlgMargin );

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    buttons->AddButton( new wxButton( this, wxID_OK ) );
    buttons->AddButton( new wxButton( this, wxID_CANCEL ) );
    buttons->Realize();
    topSizer->Add( buttons, 0, wxEXPAND | wxALL, dlgMargin );

    SetSizer( topSizer );

    testButton->Bind( wxEVT_BUTTON, &DIALOG_LIBRARY_SOURCE_SETUP::onTestConnection, this );

    m_locatorCtrl->Bind( wxEVT_KILL_FOCUS,
                         [this]( wxFocusEvent& aEvent )
                         {
                             if( m_nameCtrl->IsEmpty() )
                                 m_nameCtrl->SetValue( MDW_LIBRARY_SOURCES::DefaultName( collectSource().locator ) );

                             PrefillCredentials( MDW_SETUP::Realm( wxEmptyString, collectSource().locator ),
                                                 wxEmptyString, m_usernameCtrl, m_passwordCtrl );
                             aEvent.Skip();
                         } );

    // Saved control state would keep the typed password in the common settings
    OptOut( this );

    SetupStandardButtons( { { wxID_OK, _( "Set Up" ) } } );
    finishDialogSettings();

    // Sized to its content and centred on whichever window opened it, never where it was last left
    resetSize();
    Centre( wxBOTH );
}


void DIALOG_LIBRARY_SOURCE_SETUP::ShowStatus( const wxString& aText )
{
    if( !aText.IsEmpty() )
        SetWrappedStatus( this, m_statusText, aText );
}


bool DIALOG_LIBRARY_SOURCE_SETUP::TransferDataToWindow()
{
    m_backendChoice->Clear();
    m_backendIds.clear();

    for( const MDW_BACKEND_CHOICE& backend : MDW_SETUP::Backends( MDW_CAPABILITY_GROUP::SHARED_LIBRARIES ) )
    {
        m_backendChoice->Append( backend.display_name );
        m_backendIds.push_back( backend.backend_id );
    }

    const wxString preferred =
            m_requirement ? m_requirement->backend_id : Pgm().GetCommonSettings()->m_Workspace.default_backend_id;

    // Another backend cannot serve what a project asked for, so a missing one stays unselected
    int selected = m_requirement ? wxNOT_FOUND : 0;

    for( size_t ii = 0; ii < m_backendIds.size(); ++ii )
    {
        if( m_backendIds[ii] == preferred )
            selected = static_cast<int>( ii );
    }

    if( !m_backendIds.empty() )
        m_backendChoice->SetSelection( selected );

    if( m_requirement && selected == wxNOT_FOUND )
    {
        SetWrappedStatus( this, m_statusText,
                          wxString::Format( _( "The backend '%s' this project uses is not installed." ),
                                            m_requirement->backend_id ) );
    }

    if( m_requirement )
    {
        m_locatorCtrl->SetValue( m_requirement->locator );
        m_nameCtrl->SetValue( MDW_LIBRARY_SOURCES::NameFor( *m_requirement ) );
        m_prefixCtrl->SetValue( m_requirement->prefix );

        // Any other source would not satisfy what the project names
        m_backendChoice->Enable( false );
        m_locatorCtrl->Enable( false );
        m_prefixCtrl->Enable( false );

        PrefillCredentials( MDW_SETUP::Realm( wxEmptyString, m_requirement->locator ), wxEmptyString, m_usernameCtrl,
                            m_passwordCtrl );
    }

    return true;
}


MDW_LIBRARY_SOURCE_CONFIG DIALOG_LIBRARY_SOURCE_SETUP::collectSource() const
{
    MDW_LIBRARY_SOURCE_CONFIG source;
    int                       sel = m_backendChoice->GetSelection();

    if( sel >= 0 && sel < static_cast<int>( m_backendIds.size() ) )
        source.backend_id = m_backendIds[sel];

    source.locator = m_locatorCtrl->GetValue().Strip( wxString::both );
    source.name = m_nameCtrl->GetValue().Strip( wxString::both );
    source.prefix = m_prefixCtrl->GetValue();
    source.account = m_usernameCtrl->GetValue().Strip( wxString::both );
    return source;
}


bool DIALOG_LIBRARY_SOURCE_SETUP::servesProject( const MDW_LIBRARY_SOURCE_CONFIG& aSource )
{
    for( const MDW_LIBRARY_REQUIREMENT& needed : m_projectNeeds )
    {
        if( !MDW_LIBRARY_SOURCES::SameLocator( needed.locator, aSource.locator )
            || MDW_LIBRARY_SOURCES::Satisfies( aSource, needed ) )
            continue;

        SetWrappedStatus( this, m_statusText,
                          wxString::Format( _( "This project uses '%s' with the nickname prefix '%s'." ),
                                            needed.locator, needed.prefix ) );
        return false;
    }

    return true;
}


bool DIALOG_LIBRARY_SOURCE_SETUP::resolveSource( MDW_LIBRARY_SOURCE_CONFIG& aSource )
{
    MDW_LIBRARY_SOURCES&  sources = MDW_LIBRARY_SOURCES::Get();
    MDW_REQUIREMENT_STATE state = sources.Match( { MDW_LIBRARY_SOURCES::RequirementFor( aSource ) } ).front();

    // Another project already set this source up, and its copy serves this one too when the nicknames match
    if( state.status != MDW_REQUIREMENT_STATUS::MISSING )
    {
        if( std::optional<MDW_LIBRARY_SOURCE_CONFIG> existing = sources.Find( state.source_id ) )
        {
            if( state.status == MDW_REQUIREMENT_STATUS::PREFIX_DIFFERS )
            {
                SetWrappedStatus( this, m_statusText,
                                  wxString::Format( _( "'%s' is already set up with the nickname prefix "
                                                       "'%s'." ),
                                                    existing->name, existing->prefix ) );
                return false;
            }

            aSource = *existing;
            return servesProject( aSource );
        }
    }

    // The name keys the source's copy and session, so a taken one would serve another source
    for( const MDW_LIBRARY_SOURCE_CONFIG& existing : sources.Sources() )
    {
        if( existing.name.IsSameAs( aSource.name, false ) )
        {
            SetWrappedStatus( this, m_statusText,
                              wxString::Format( _( "A source named '%s' already exists." ), existing.name ) );
            return false;
        }
    }

    return servesProject( aSource );
}


bool DIALOG_LIBRARY_SOURCE_SETUP::TransferDataFromWindow()
{
    MDW_LIBRARY_SOURCE_CONFIG source = collectSource();

    if( source.backend_id.IsEmpty() || source.locator.IsEmpty() )
    {
        SetWrappedStatus( this, m_statusText, _( "Choose a backend and enter the library location." ) );
        return false;
    }

    if( !MDW_LIBRARY_SOURCES::IsSafeName( source.name ) )
    {
        SetWrappedStatus( this, m_statusText, _( "Enter a name that can be used as a folder name." ) );
        return false;
    }

    if( LIB_ID::FindIllegalLibraryNameChar( UTF8( source.prefix ) ) )
    {
        SetWrappedStatus( this, m_statusText,
                          _( "The nickname prefix contains a character a library nickname cannot use." ) );
        return false;
    }

    if( !resolveSource( source )
        || !VerifyTypedLogin( this, m_statusText, source.backend_id, source.name, source.locator, wxEmptyString,
                              m_usernameCtrl, m_passwordCtrl ) )
    {
        return false;
    }

    const bool     added = source.id.IsEmpty();
    const wxString username = m_usernameCtrl->GetValue().Strip( wxString::both );
    MDW_RESULT     result = MDW_RESULT::Ok();

    if( added )
    {
        result = MDW_LIBRARY_SOURCES::Get().Add( source );
    }
    else if( !username.IsEmpty()
             && username != MDW_SETUP::Username( MDW_SETUP::Realm( wxEmptyString, source.locator ), source.account ) )
    {
        result = MDW_SETUP::RememberSourceAccount( source, username );
    }

    if( !result.IsOk() )
    {
        SetWrappedStatus( this, m_statusText, MdwErrorText( result ) );
        return false;
    }

    // A source the set-up could not use is gone again, so the next try adds it anew
    if( wxString failure = m_setUp ? m_setUp( this, source, added ) : wxString(); !failure.IsEmpty() )
    {
        SetWrappedStatus( this, m_statusText, failure );
        return false;
    }

    m_source = source;
    return true;
}


void DIALOG_LIBRARY_SOURCE_SETUP::onTestConnection( wxCommandEvent& aEvent )
{
    MDW_LIBRARY_SOURCE_CONFIG source = collectSource();

    if( source.backend_id.IsEmpty() || source.locator.IsEmpty() || source.name.IsEmpty() )
    {
        SetWrappedStatus( this, m_statusText, _( "Choose a backend and enter the location and name first." ) );
        return;
    }

    if( !resolveSource( source ) )
        return;

    ShowConnectionTest( this, m_statusText, source.backend_id, source.name, source.locator, wxEmptyString,
                        m_usernameCtrl, m_passwordCtrl );
}


MDW_RESULT ShowConnectionTest( wxWindow* aDialog, wxStaticText* aStatus, const wxString& aBackendId,
                               const wxString& aWorkspaceId, const wxString& aLocator, const wxString& aProjectPath,
                               wxTextCtrl* aUsername, wxTextCtrl* aPassword, std::shared_ptr<MDW_SESSION>* aKeep,
                               const wxString& aKeyFile )
{
    using TESTED = std::pair<MDW_RESULT, MDW_SOURCE_INFO>;

    MDW_TYPED_CREDENTIALS typed;
    typed.fields[wxS( "username" )] = aUsername->GetValue();
    typed.fields[wxS( "password" )] = aPassword->GetValue();

    auto test = [&]( const std::optional<MDW_HOST_KEY>& aTrust )
    {
        // The test stops itself at its own deadline, and at once when the user cancels
        return RunWithProgress<TESTED>( aDialog, _( "Connecting..." ),
                                        [=]()
                                        {
                                            return MDW_SETUP::TestConnection( aBackendId, aWorkspaceId, aLocator,
                                                                              aProjectPath, typed, aTrust, aKeep,
                                                                              aKeyFile );
                                        } );
    };

    TESTED              tested = test( std::nullopt );
    wxWeakRef<wxWindow> alive( aDialog );

    // The dialog's own fields hold the credentials, so only a host key is asked about here
    if( tested.second.connection.phase == MDW_CONNECTION::TRUST_NEEDED )
    {
        MdwResolveConnection( aDialog, tested.second.connection,
                              [&]( const MDW_CONNECTION_ANSWER& aAnswer )
                              {
                                  tested = test( aAnswer.trust );
                              } );
    }

    if( !alive )
        return MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED );

    if( tested.first.error == MDW_ERROR_CODE::CANCELLED )
    {
        SetWrappedStatus( aDialog, aStatus, _( "Connection test cancelled." ) );
    }
    else if( tested.first.IsOk() )
    {
        const MDW_ACCOUNT& account = tested.second.connection.account;
        wxString           connected = wxString::Format( _( "Connected to %s." ), tested.second.display_name );

        if( !account.Name().IsEmpty() )
        {
            connected = wxString::Format( _( "Connected to %s as %s." ), tested.second.display_name,
                                          account.NameAndLogin() );
        }

        SetWrappedStatus( aDialog, aStatus, connected );

        // The sign-in kept it, so OK need not send it again
        aPassword->Clear();
        PrefillCredentials( tested.second.connection.realm, wxEmptyString, aUsername, aPassword );
    }
    else
    {
        SetWrappedStatus( aDialog, aStatus,
                          wxString::Format( _( "Connection failed. %s" ), MdwErrorText( tested.first ) ) );
    }

    return tested.first;
}


bool VerifyTypedLogin( wxWindow* aDialog, wxStaticText* aStatus, const wxString& aBackendId,
                       const wxString& aWorkspaceId, const wxString& aLocator, const wxString& aProjectPath,
                       wxTextCtrl* aUsername, wxTextCtrl* aPassword, const wxString& aKeyFile )
{
    if( aPassword->IsEmpty() )
        return true;

    MDW_ERROR_CODE error = ShowConnectionTest( aDialog, aStatus, aBackendId, aWorkspaceId, aLocator, aProjectPath,
                                               aUsername, aPassword, nullptr, aKeyFile )
                                   .error;

    return error != MDW_ERROR_CODE::CANCELLED && error != MDW_ERROR_CODE::AUTH_FAILED
           && error != MDW_ERROR_CODE::NOT_AUTHENTICATED;
}


void PrefillCredentials( const wxString& aRealm, const wxString& aBindingAccount, wxTextCtrl* aUsername,
                         wxTextCtrl* aPassword )
{
    if( aRealm.IsEmpty() )
        return;

    if( aUsername->IsEmpty() )
        aUsername->SetValue( MDW_SETUP::Username( aRealm, aBindingAccount ) );

    if( MDW_SETUP::HasCredential( aRealm, aUsername->GetValue() ) )
        aPassword->SetHint( wxString::Format( _( "Saved for %s" ), aRealm ) );
}


bool SyncLibraryTable( LIBRARY_TABLE_TYPE aType, LIBRARY_TABLE_SCOPE aScope, bool& aFailed,
                       const std::function<bool( LIBRARY_TABLE&, const std::set<wxString>& )>& aSync )
{
    LIBRARY_MANAGER&              manager = Pgm().GetLibraryManager();
    std::optional<LIBRARY_TABLE*> live = manager.Table( aType, aScope );

    if( !live )
        return false;

    // Edited apart from the live table, whose rows background loads may be reading
    LIBRARY_TABLE table( wxFileName( ( *live )->Path() ), aScope, aType );

    // A project without its own table yet starts an empty one
    if( !table.IsOk() && aScope == LIBRARY_TABLE_SCOPE::PROJECT && !wxFileName::FileExists( ( *live )->Path() ) )
    {
        table.SetType( aType );
        table.SetOk();
    }

    if( !table.IsOk() )
    {
        aFailed = true;
        return false;
    }

    // A nested table's library is found by nickname as surely as a row of this one
    std::set<wxString> nested;

    for( const LIBRARY_TABLE_ROW* row : manager.Rows( aType, aScope, true ) )
    {
        if( !table.HasRow( row->Nickname() ) )
            nested.insert( row->Nickname() );
    }

    if( !aSync( table, nested ) )
        return false;

    aFailed |= !table.Save();
    return true;
}


void ReloadLibraryEditors( KIWAY& aKiway )
{
    for( KIWAY::FACE_T face : { KIWAY::FACE_SCH, KIWAY::FACE_PCB } )
    {
        if( KIFACE* kiface = aKiway.KiFACE( face ) )
            kiface->PreloadLibraries( &aKiway );
    }

    // The symbol editor and CvPcb block until every library loads, so they wait for the preload's own reload mail
    std::string payload;

    for( FRAME_T frame : { FRAME_FOOTPRINT_EDITOR, FRAME_FOOTPRINT_VIEWER } )
        aKiway.ExpressMail( frame, MAIL_RELOAD_LIB, payload );
}


MDW_ROWS_REGISTERED RegisterLibrarySourceRows( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject,
                                               const std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources,
                                               bool                                          aFilesChanged )
{
    LIBRARY_MANAGER&    manager = Pgm().GetLibraryManager();
    MDW_ROWS_REGISTERED registered;
    bool                changed = false;
    bool                projectChanged = false;
    bool                failed = false;
    bool                projectFailed = false;
    std::set<wxString>  ids;
    std::set<wxString>  markers;

    const std::vector<MDW_LIBRARY_REQUIREMENT> needs =
            aProject ? aProject->GetProjectFile().m_WorkspaceLibrarySources : std::vector<MDW_LIBRARY_REQUIREMENT>();

    for( const MDW_LIBRARY_SOURCE_CONFIG& source : aSources )
    {
        ids.insert( source.id );
        markers.insert( MDW_LIBRARY_SOURCES::Marker( source ) );

        for( const MDW_LIBRARY_REQUIREMENT& needed : needs )
        {
            if( !needed.pin.token.IsEmpty() && MDW_LIBRARY_SOURCES::Satisfies( source, needed ) )
                markers.insert( MDW_LIBRARY_SOURCES::Marker( source, needed.pin ) );
        }
    }

    auto countRows = [&]( const LIBRARY_TABLE& aTable )
    {
        for( const LIBRARY_TABLE_ROW& row : aTable.Rows() )
            registered.rows += markers.contains( MDW_LIBRARY_SOURCES::MarkerOf( row.Options() ) );
    };

    for( auto [tableType, libraryType] : { std::pair( LIBRARY_TABLE_TYPE::SYMBOL, MDW_LIBRARY_TYPE::SYMBOL ),
                                           std::pair( LIBRARY_TABLE_TYPE::FOOTPRINT, MDW_LIBRARY_TYPE::FOOTPRINT ) } )
    {
        changed |= SyncLibraryTable( tableType, LIBRARY_TABLE_SCOPE::GLOBAL, failed,
                                     [&]( LIBRARY_TABLE& aTable, const std::set<wxString>& aNested )
                                     {
                                         bool synced = MDW_SETUP::SyncLibraryRows( aTable, libraryType, aSources,
                                                                                   registered.conflicts, aNested );
                                         countRows( aTable );
                                         return synced;
                                     } );

        if( !aProject )
            continue;

        projectChanged |=
                SyncLibraryTable( tableType, LIBRARY_TABLE_SCOPE::PROJECT, projectFailed,
                                  [&]( LIBRARY_TABLE& aTable, const std::set<wxString>& aNested )
                                  {
                                      bool synced = MDW_SETUP::SyncPinnedRows( aTable, libraryType, needs, ids,
                                                                               registered.projectConflicts, aNested );
                                      countRows( aTable );
                                      return synced;
                                  } );
    }

    if( projectChanged )
        manager.ProjectChanged();

    if( changed || projectChanged || aFilesChanged )
    {
        manager.LoadGlobalTables( { LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_TYPE::FOOTPRINT } );
        ReloadLibraryEditors( aKiway );
    }

    if( failed )
        DisplayErrorMessage( aParent, _( "The global library tables could not be updated." ) );

    if( projectFailed )
        DisplayErrorMessage( aParent, _( "The project library tables could not be updated." ) );

    return registered;
}


namespace
{

/// A few of aNames, one per line, so a long list cannot push the message box off the screen
wxString nameList( const std::vector<wxString>& aNames )
{
    const size_t shown = std::min<size_t>( aNames.size(), 10 );
    wxString     text;

    for( size_t ii = 0; ii < shown; ++ii )
        text << ( ii ? wxS( "\n" ) : wxS( "" ) ) << aNames[ii];

    if( aNames.size() > shown )
        text << wxS( "\n" ) << wxString::Format( _( "and %zu more" ), aNames.size() - shown );

    return text;
}


/// Put aText in the status bar of the frame aParent belongs to, since the top window may be the manager's
void showFrameStatus( wxWindow* aParent, const wxString& aText )
{
    wxWindow* frame = aParent;

    while( frame && !wxDynamicCast( frame, wxFrame ) )
        frame = frame->GetParent();

    wxLogStatus( static_cast<wxFrame*>( frame ), wxS( "%s" ), aText );
}


/// Whether a library sync started from aParent must wait for the one running, saying so in aParent's frame
bool librarySyncRefused( wxWindow* aParent )
{
    if( !MDW_LIBRARY_SOURCES::Get().LibrarySyncRunning() )
        return false;

    // Progress dialogs and the merge tool leave other frames live, and a sync started there would nest its modal
    // loops inside the running one's, which could then not end in order
    wxBell();
    showFrameStatus( aParent, _( "Team libraries are already being updated. Try again once that finishes." ) );
    return true;
}


/// The nicknames the copies of aSource at aPins already held, whose collisions the user was told of then
std::set<wxString> heldNicknames( const MDW_LIBRARY_SOURCE_CONFIG& aSource, const std::vector<MDW_SNAPSHOT>& aPins )
{
    std::set<wxString> nicknames;

    for( const MDW_SNAPSHOT& pin : aPins )
    {
        if( std::optional<MDW_LIBRARY_COPY> copy =
                    MDW_LIBRARY_SOURCES::Get().Copy( MDW_LIBRARY_SOURCES::Marker( aSource, pin ) ) )
        {
            for( const MDW_COPY_LIBRARY& library : copy->libraries )
                nicknames.insert( library.nickname );
        }
    }

    return nicknames;
}


/// Tell the user which newly fetched libraries were not added because their names are taken
void reportConflicts( wxWindow* aParent, const MDW_ROWS_REGISTERED& aRegistered, const std::set<wxString>& aTold )
{
    auto fresh = [&]( const std::vector<wxString>& aNames )
    {
        std::vector<wxString> names;

        std::copy_if( aNames.begin(), aNames.end(), std::back_inserter( names ),
                      [&]( const wxString& aName )
                      {
                          return !aTold.contains( aName );
                      } );

        return names;
    };

    if( std::vector<wxString> names = fresh( aRegistered.conflicts ); !names.empty() )
    {
        DisplayInfoMessage( aParent,
                            _( "These team libraries were not added because the global library tables already "
                               "have libraries with the same names:" ),
                            nameList( names ) );
    }

    if( std::vector<wxString> names = fresh( aRegistered.projectConflicts ); !names.empty() )
    {
        DisplayInfoMessage( aParent,
                            _( "These pinned team libraries were not added because the project library tables "
                               "already have libraries with the same names:" ),
                            nameList( names ) );
    }
}

} // namespace


void SetWrappedStatus( wxWindow* aDialog, wxStaticText* aStatus, const wxString& aText )
{
    // An empty status is hidden, so it leaves no blank line in the dialog
    aStatus->Show( !aText.IsEmpty() );

    // Measure before SetLabel, which resizes the text to fit an unwrapped label
    aDialog->Layout();
    int width = std::max( aStatus->GetSize().x, aDialog->FromDIP( 200 ) );

    aStatus->SetLabel( aText );
    aStatus->Wrap( width );
    aDialog->Layout();

    wxSize best = aDialog->GetBestSize();
    wxSize size = aDialog->GetSize();
    aDialog->SetSize( std::max( size.x, best.x ), std::max( size.y, best.y ) );
}


namespace
{

std::optional<std::string> readFile( const wxString& aFile )
{
    std::ifstream in( ToFsPath( aFile ), std::ios::binary );

    if( !in.is_open() )
        return std::nullopt;

    return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
}


/// Settle aSides of aCopy with the user as a git merge does, adding the answers to aResolutions, the backup to
/// aBackups, and to aOutcomes a line per answer saying which version it kept and which it saved
MDW_RESULT resolveConflicts( wxWindow* aParent, KIWAY& aKiway, const MDW_LIBRARY_COPY& aCopy,
                             const std::vector<MDW_CONFLICT_SIDES>& aSides, const wxString& aFolder,
                             std::vector<MDW_CONFLICT_RESOLUTION>& aResolutions, std::vector<wxString>& aBackups,
                             std::vector<wxString>& aOutcomes )
{
    std::vector<MERGE_CONFLICT> conflicts;

    for( const MDW_CONFLICT_SIDES& side : aSides )
    {
        const MDW_UPDATE_CONFLICT& conflict = side.conflict;
        const wxFileName           file( conflict.relative_path );
        const bool                 symbol = file.GetExt() == wxS( "kicad_sym" );
        wxString libraryPath = symbol ? conflict.relative_path : conflict.relative_path.BeforeLast( '/' );

        // A symbol folder is the library, whichever of its files holds the member
        if( symbol && libraryPath.BeforeLast( '/' ).EndsWith( wxS( ".kicad_symdir" ) ) )
            libraryPath = libraryPath.BeforeLast( '/' );

        const wxString comparable = MDW_LIBRARY_SOURCES::ComparablePath( libraryPath );

        auto copyLibrary =
                std::find_if( aCopy.libraries.begin(), aCopy.libraries.end(),
                              [&]( const MDW_COPY_LIBRARY& aLibrary )
                              {
                                  return MDW_LIBRARY_SOURCES::ComparablePath( aLibrary.relative_path ) == comparable;
                              } );

        // Name the library by its nickname, as the tables do, not by its file
        const wxString library =
                copyLibrary != aCopy.libraries.end() ? copyLibrary->nickname : wxFileName( libraryPath ).GetName();

        // Members of one symbol library each need a file of their own, unique even where names differ only in case
        const wxString path = symbol ? libraryPath.BeforeLast( '.' ) + wxS( "/" )
                                               + wxString::Format( wxS( "%zu_" ), conflicts.size() )
                                               + EscapeString( conflict.name, CTX_FILENAME ) + wxS( ".kicad_sym" )
                                     : conflict.relative_path;

        conflicts.push_back( { path, library + wxS( ":" ) + conflict.name, side.ancestorFile, side.oursFile,
                               side.theirsFile, !conflict.base_revision_id.IsEmpty(), side.hasOurs,
                               !conflict.theirs_revision_id.IsEmpty() } );
    }

    MERGE_CONFLICT_TEXT text;
    text.onlyTheirs = _( "'%s' exists only in the team library (%zu of %zu).\n\n"
                         "How should it be resolved?" );
    text.deletedHere = _( "'%s' was deleted here but changed in the team library (%zu of %zu).\n\n"
                          "How should it be resolved?" );
    text.onlyOurs = _( "'%s' exists only here (%zu of %zu).\n\n"
                       "How should it be resolved?" );
    text.deletedThere = _( "'%s' was changed here but deleted in the team library (%zu of %zu).\n\n"
                           "How should it be resolved?" );
    text.changedBoth = _( "'%s' was changed both here and in the team library (%zu of %zu).\n\n"
                          "Using the team library's version replaces the whole part and drops every change made "
                          "here. How should it be resolved?" );
    text.useTheirs = _( "Use the team library's version" );
    text.deleteAsTheirs = _( "Delete it, as in the team library" );
    text.leaveOutAsTheirs = _( "Leave it out, as in the team library" );

    std::vector<MERGE_ANSWER>      answers;
    std::vector<MERGE_LOSING_COPY> losingCopies;

    if( PromptMergeConflicts( aParent, aKiway, conflicts, text, aFolder, answers, losingCopies )
        != MERGE_PROMPT_RESULT::RESOLVED )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED );
    }

    for( size_t ii = 0; ii < answers.size(); ++ii )
    {
        const MDW_UPDATE_CONFLICT& conflict = aSides[ii].conflict;
        MDW_CONFLICT_RESOLUTION    resolution{ conflict.item.item_id, conflict.theirs_revision_id, std::nullopt,
                                            conflict.ours_token, wxEmptyString };

        // Ours is left to the copy, which holds it whole even where the extract could not
        if( answers[ii].kept != MERGE_ANSWER::KEPT::OURS )
        {
            resolution.merged = answers[ii].file.IsEmpty() ? std::string() : readFile( answers[ii].file );

            if( !resolution.merged )
            {
                return MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL,
                                         wxString::Format( _( "Could not read '%s'." ), answers[ii].file ) );
            }
        }

        std::erase_if( aResolutions,
                       [&]( const MDW_CONFLICT_RESOLUTION& aOld )
                       {
                           return aOld.item_id == resolution.item_id;
                       } );

        aResolutions.push_back( resolution );
    }

    wxString backup;
    wxString failed;
    wxString root = MDW_LIBRARY_SOURCES::Get().BackupRoot( aCopy );

    if( !SaveLosingCopies( root, losingCopies, backup, failed ) )
    {
        return MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL,
                                 wxString::Format( _( "Could not save the version you are replacing "
                                                      "as '%s', so the merge was stopped." ),
                                                   failed ) );
    }

    if( !backup.IsEmpty() )
        aBackups.push_back( backup );

    for( size_t ii = 0; ii < answers.size(); ++ii )
    {
        const MERGE_CONFLICT& conflict = conflicts[ii];

        auto saved = [&]( const wxString& aSuffix )
        {
            return std::any_of( losingCopies.begin(), losingCopies.end(),
                                [&]( const MERGE_LOSING_COPY& aLosing )
                                {
                                    return aLosing.relativeDest == conflict.path + aSuffix;
                                } );
        };

        const bool deleted = answers[ii].file.IsEmpty();
        const bool savedMine = saved( wxS( ".mine" ) );
        const bool savedTheirs = saved( wxS( ".theirs" ) );
        wxString   outcome;

        switch( answers[ii].kept )
        {
        case MERGE_ANSWER::KEPT::OURS:
            outcome = !deleted               ? _( "Kept your version of %s." )
                      : conflict.hasAncestor ? _( "Kept %s deleted." )
                                             : _( "Left %s out." );
            break;

        case MERGE_ANSWER::KEPT::THEIRS:
            outcome = !deleted               ? _( "Took the team library's version of %s." )
                      : conflict.hasAncestor ? _( "Deleted %s, as in the team library." )
                                             : _( "Left %s out, as in the team library." );
            break;

        case MERGE_ANSWER::KEPT::MERGED:
            outcome = savedMine || savedTheirs ? _( "Merged %s by hand." )
                                               : _( "Merged the changes made to %s on both sides." );
            break;
        }

        outcome = wxString::Format( outcome, conflict.name );

        if( savedMine && savedTheirs )
            outcome += wxS( " " ) + _( "Both earlier versions are saved." );
        else if( savedTheirs )
            outcome += wxS( " " ) + _( "The team library's version is saved." );
        else if( savedMine )
            outcome += wxS( " " ) + _( "Your version is saved." );

        aOutcomes.push_back( outcome );
    }

    return MDW_RESULT::Ok();
}


/// aOutcomes, one per line, then the backup folders holding the versions they saved
wxString outcomeMessage( const std::vector<wxString>& aOutcomes, const std::vector<wxString>& aBackups )
{
    wxString text;
    wxString folders;

    for( const wxString& outcome : aOutcomes )
        text << outcome << wxS( "\n" );

    for( const wxString& backup : aBackups )
        folders << ( folders.IsEmpty() ? wxS( "" ) : wxS( ", " ) ) << wxS( "'" ) << backup << wxS( "'" );

    // A path too wide for the wx 3.3 generic message box stops its wrapping, dropping every later line
    return text << wxS( "\n" ) << wxString::Format( _( "The saved versions are in %s." ), folders );
}


/// The snapshot each of aNeeds that aSource serves pins, the latest when none pins one
std::vector<MDW_SNAPSHOT> pinsFor( const MDW_LIBRARY_SOURCE_CONFIG&            aSource,
                                   const std::vector<MDW_LIBRARY_REQUIREMENT>& aNeeds )
{
    std::vector<MDW_SNAPSHOT> pins;

    for( const MDW_LIBRARY_REQUIREMENT& needed : aNeeds )
    {
        if( MDW_LIBRARY_SOURCES::Satisfies( aSource, needed ) )
            pins.push_back( needed.pin );
    }

    if( pins.empty() )
        pins.emplace_back();

    return pins;
}


/// Placed parts keep their own copies, so an update reaches a design only through Update Symbols or Footprints
void offerPlacedPartReview( wxWindow* aParent, KIWAY& aKiway )
{
    std::vector<FRAME_T> designs;

    for( FRAME_T frame : { FRAME_SCH, FRAME_PCB_EDITOR } )
    {
        if( aKiway.Player( frame, false ) )
            designs.push_back( frame );
    }

    if( designs.empty() )
        return;

    KIDIALOG dlg( aParent,
                  _( "The team library was updated. Parts placed from it keep their old copies until "
                     "they are updated from the library." ),
                  _( "Library Updated" ), wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxICON_INFORMATION );
    dlg.SetOKCancelLabels( _( "Review Placed Part Updates..." ), _( "Not Now" ) );
    dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

    if( dlg.ShowModal() != wxID_OK )
        return;

    for( FRAME_T frame : designs )
    {
        std::string payload;
        aKiway.ExpressMail( frame, MAIL_REVIEW_LIBRARY_UPDATES, payload );
    }
}


/// The one line a sync leaves in the status bar
wxString syncOutcome( const MDW_LIBRARY_SOURCE_CONFIG& aSource, const MDW_LIBRARY_SYNC_RESULT& aSync )
{
    if( aSync.result.error == MDW_ERROR_CODE::CANCELLED )
        return wxString::Format( _( "%s: stopped before it finished." ), aSource.name );

    if( !aSync.result.IsOk() )
        return wxString::Format( _( "%s: not updated. %s" ), aSource.name, MdwErrorText( aSync.result ) );

    if( !aSync.conflicts.empty() )
        return wxString::Format( _( "%s: not updated, changes not checked in conflict with the latest version." ),
                                 aSource.name );

    auto count = [&]( MDW_LIBRARY_TYPE aType )
    {
        return std::count_if( aSync.copy.libraries.begin(), aSync.copy.libraries.end(),
                              [&]( const MDW_COPY_LIBRARY& aLibrary )
                              {
                                  return aLibrary.type == aType;
                              } );
    };

    return wxString::Format( _( "%s updated. Symbol libraries: %ld, footprint libraries: %ld, parts changed: %zu" ),
                             aSource.name,
                             static_cast<long>( count( MDW_LIBRARY_TYPE::SYMBOL ) ),
                             static_cast<long>( count( MDW_LIBRARY_TYPE::FOOTPRINT ) ), aSync.changes.size() );
}


/// Bring the copy of aSource at aPin up to date, settling each conflict with the user beyond those aAnswered
MDW_LIBRARY_SYNC_RESULT syncWithMerge( wxWindow* aParent, KIWAY& aKiway, const MDW_LIBRARY_SOURCE_CONFIG& aSource,
                                       const MDW_SNAPSHOT&                         aPin,
                                       const std::vector<MDW_CONFLICT_RESOLUTION>& aAnswered = {} )
{
    std::vector<MDW_CONFLICT_RESOLUTION>           resolutions = aAnswered;
    std::vector<MDW_UPDATE_CONFLICT>               asked;
    wxString                                       localRoot;
    std::vector<wxString>                          backups;
    std::vector<wxString>                          outcomes;
    std::map<wxString, std::optional<std::string>> answered;
    bool                                           trustAsked = false;

    auto finish = [&]( MDW_LIBRARY_SYNC_RESULT aSync )
    {
        showFrameStatus( aParent, syncOutcome( aSource, aSync ) );

        if( aSync.result.IsOk() && aSync.conflicts.empty() && !backups.empty() )
        {
            DisplayInfoMessage( aParent, outcomeMessage( outcomes, backups ) );
        }
        else if( std::all_of( answered.begin(), answered.end(),
                              []( const auto& aFile )
                              {
                                  return readFile( aFile.first ) == aFile.second;
                              } ) )
        {
            // A failure that left the answered files alone replaced nothing, so their saved versions go too
            for( const wxString& backup : backups )
                wxFileName::Rmdir( backup, wxPATH_RMDIR_RECURSIVE );
        }

        return aSync;
    };

    while( true )
    {
        // Placed on this thread, which alone may load the schematic kiface
        std::optional<std::vector<MDW_CONFLICT_RESOLUTION>> placed =
                MDW_SETUP::PlacedAnswers( localRoot, asked, resolutions, MDW_SETUP::SymbolUpgrade( aKiway ) );

        if( !placed )
        {
            MDW_LIBRARY_SYNC_RESULT failed;
            failed.result = MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL,
                                              _( "Could not put the chosen versions into the file format of their "
                                                 "libraries." ) );
            return finish( failed );
        }

        MDW_LIBRARY_SYNC_RESULT sync = RunWithProgress<MDW_LIBRARY_SYNC_RESULT>(
                aParent, wxString::Format( _( "Getting %s..." ), aSource.name ),
                [&]()
                {
                    return MDW_SETUP::SyncLibrarySource( aSource, aPin, *placed );
                } );

        // A new server is offered for trust here as Test Connection offers it, once per sync
        if( sync.result.error == MDW_ERROR_CODE::UNTRUSTED_HOST && !trustAsked )
        {
            wxWeakRef<wxWindow>   alive( aParent );
            MDW_CONNECTION_STATUS untrusted;
            bool                  trusted = false;

            trustAsked = true;
            untrusted.phase = MDW_CONNECTION::TRUST_NEEDED;
            untrusted.reason = sync.result;

            auto reconnect = [&]( const MDW_CONNECTION_ANSWER& aAnswer )
            {
                auto task = [&]()
                {
                    return MDW_SETUP::ReconnectLibrarySource( aSource, aAnswer );
                };

                trusted = RunWithProgress<MDW_RESULT>( aParent, _( "Connecting..." ), task, false ).IsOk();
            };

            MdwResolveConnection( aParent, untrusted, reconnect );

            if( !alive )
            {
                sync.result = MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED );
                return sync;
            }

            if( trusted )
                continue;
        }

        // An answered conflict that comes back unchanged was not applied, so asking again would never end
        bool repeated = std::any_of(
                sync.conflicts.begin(), sync.conflicts.end(),
                [&]( const MDW_UPDATE_CONFLICT& aConflict )
                {
                    return std::any_of( resolutions.begin(), resolutions.end(),
                                        [&]( const MDW_CONFLICT_RESOLUTION& aResolution )
                                        {
                                            return aResolution.item_id == aConflict.item.item_id
                                                   && aResolution.theirs_revision_id == aConflict.theirs_revision_id
                                                   && aResolution.ours_token == aConflict.ours_token;
                                        } );
                } );

        SCOPED_TEMP_PATH folder;

        if( !sync.result.IsOk() || sync.conflicts.empty() || repeated
            || !folder.MakeDirectory( wxS( "kicad_libmerge_" ), 0700 ) )
        {
            return finish( sync );
        }

        MDW_CONFLICT_SIDES_RESULT sides = RunWithProgress<MDW_CONFLICT_SIDES_RESULT>(
                aParent, _( "Getting the conflicting versions..." ),
                [&]()
                {
                    return MDW_SETUP::ConflictSides( aSource, sync.copy.local_root, sync.conflicts, folder.Path() );
                } );

        localRoot = sync.copy.local_root;

        for( const MDW_UPDATE_CONFLICT& conflict : sync.conflicts )
        {
            std::erase_if( asked,
                           [&]( const MDW_UPDATE_CONFLICT& aOld )
                           {
                               return aOld.item.item_id == conflict.item.item_id;
                           } );

            asked.push_back( conflict );
        }

        // The files as answered, so a failure that leaves them alone can drop the versions saved for them
        for( const MDW_UPDATE_CONFLICT& conflict : sync.conflicts )
        {
            const wxString file = sync.copy.local_root + wxFILE_SEP_PATH + conflict.relative_path;

            if( !answered.count( file ) )
                answered[file] = readFile( file );
        }

        if( sides.result.IsOk() )
        {
            sides.result = resolveConflicts( aParent, aKiway, sync.copy, sides.sides, folder.Path(), resolutions,
                                             backups, outcomes );
        }

        // The reason replaces the conflicts, which callers would otherwise report instead
        if( !sides.result.IsOk() )
        {
            sync.result = sides.result;
            sync.conflicts.clear();
            return finish( sync );
        }
    }
}

} // namespace


std::vector<MDW_LIBRARY_SOURCE_CONFIG> ProjectLibrarySources( const PROJECT& aProject )
{
    std::vector<MDW_LIBRARY_SOURCE_CONFIG> sources;

    if( !MDW_LIBRARY_SOURCES::TeamLibrariesEnabled() )
        return sources;

    MDW_LIBRARY_SOURCES& all = MDW_LIBRARY_SOURCES::Get();

    for( const MDW_REQUIREMENT_STATE& state : all.Match( aProject.GetProjectFile().m_WorkspaceLibrarySources ) )
    {
        std::optional<MDW_LIBRARY_SOURCE_CONFIG> source = all.Find( state.source_id );

        if( source && std::find( sources.begin(), sources.end(), *source ) == sources.end() )
            sources.push_back( *source );
    }

    return sources;
}


namespace
{

/// UpdateLibrarySources() for a caller already holding the library sync
MDW_RESULT updateLibrarySources( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject,
                                 const std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources,
                                 const std::vector<MDW_CONFLICT_RESOLUTION>&   aAnswered )
{
    if( aSources.empty() )
        return MDW_RESULT::Ok();

    // An editor's next save would put back the version the update replaced, and a check-in would push it
    if( std::vector<wxString> unsaved = LibraryEditorsWithUnsavedChanges( aKiway ); !unsaved.empty() )
    {
        wxString listed;

        for( const wxString& name : unsaved )
            listed += wxString::Format( wxS( "\n    - %s" ), name );

        DisplayInfoMessage( aParent, wxString::Format( _( "The following editors have unsaved changes:%s\n\n"
                                                          "Save or discard those changes before continuing." ),
                                                       listed ) );
        return MDW_RESULT::Fail( MDW_ERROR_CODE::BUSY );
    }

    const std::vector<MDW_LIBRARY_REQUIREMENT> needs =
            aProject ? aProject->GetProjectFile().m_WorkspaceLibrarySources : std::vector<MDW_LIBRARY_REQUIREMENT>();

    bool               filesChanged = false;
    bool               partsChanged = false;
    MDW_RESULT         updated = MDW_RESULT::Ok();
    std::set<wxString> told;

    for( const MDW_LIBRARY_SOURCE_CONFIG& source : aSources )
    {
        const std::vector<MDW_SNAPSHOT> pins = pinsFor( source, needs );

        told.merge( heldNicknames( source, pins ) );

        for( const MDW_SNAPSHOT& pin : pins )
        {
            MDW_LIBRARY_SYNC_RESULT sync = syncWithMerge( aParent, aKiway, source, pin, aAnswered );

            filesChanged |= sync.filesChanged;

            if( sync.result.IsOk() && sync.conflicts.empty() )
            {
                partsChanged |= !sync.changes.empty();
                continue;
            }

            updated = sync.result.IsOk() ? MDW_RESULT::Fail( MDW_ERROR_CODE::VERSION_CONFLICT ) : sync.result;

            if( sync.result.error == MDW_ERROR_CODE::CANCELLED )
                continue;

            wxString why = MdwErrorText( sync.result );

            if( !sync.conflicts.empty() )
                why = _( "Changes not checked in conflict with the latest version." );

            DisplayErrorMessage(
                    aParent, wxString::Format( _( "The libraries of '%s' could not be fetched." ), source.name ), why );
        }
    }

    // A failed update may already have rewritten the index, so every source tried has its rows reconciled
    reportConflicts( aParent, RegisterLibrarySourceRows( aParent, aKiway, aProject, aSources, filesChanged ), told );

    if( partsChanged )
        offerPlacedPartReview( aParent, aKiway );

    return updated;
}

} // namespace


MDW_RESULT UpdateLibrarySources( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject,
                                 const std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources,
                                 const std::vector<MDW_CONFLICT_RESOLUTION>&   aAnswered )
{
    if( librarySyncRefused( aParent ) )
        return MDW_RESULT::Fail( MDW_ERROR_CODE::BUSY );

    SCOPED_SET_RESET running( MDW_LIBRARY_SOURCES::Get().LibrarySyncRunning(), true );

    return updateLibrarySources( aParent, aKiway, aProject, aSources, aAnswered );
}


namespace
{

/// What a set-up leaves to tell the user once it is done
struct SET_UP_REPORT
{
    MDW_ROWS_REGISTERED registered;
    std::set<wxString>  told;
};


/// Why a source could not be set up, and whether a new login would help
struct SET_UP_FAILURE
{
    wxString text;
    bool     signIn = false;
};


/// Fetch and register the libraries of aSource, removing a new source that is unusable with its copy
/// Gives why the source is unusable, or nothing when it is set up
std::optional<SET_UP_FAILURE> fetchSource( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject,
                                           const MDW_LIBRARY_SOURCE_CONFIG& aSource, bool aAdded,
                                           SET_UP_REPORT& aReport )
{
    const std::vector<MDW_LIBRARY_REQUIREMENT> needs =
            aProject ? aProject->GetProjectFile().m_WorkspaceLibrarySources : std::vector<MDW_LIBRARY_REQUIREMENT>();
    const std::vector<MDW_SNAPSHOT> pins = pinsFor( aSource, needs );
    std::vector<MDW_CREATED_COPY>   written;
    std::optional<MDW_RESULT>       failure;
    bool                            fetched = false;
    bool                            filesChanged = false;

    aReport.told = heldNicknames( aSource, pins );

    for( const MDW_SNAPSHOT& pin : pins )
    {
        MDW_LIBRARY_SYNC_RESULT sync = syncWithMerge( aParent, aKiway, aSource, pin );

        filesChanged |= sync.filesChanged;

        // Only a copy this set-up wrote may be deleted, since one already here may hold work not yet checked in
        if( sync.created )
            written.push_back( *sync.created );

        if( sync.result.error == MDW_ERROR_CODE::CANCELLED )
            continue;

        // The copy kept its local edits, so the libraries on disk are not the source's latest
        if( sync.result.IsOk() && !sync.conflicts.empty() )
        {
            sync.result = MDW_RESULT::Fail( MDW_ERROR_CODE::VERSION_CONFLICT,
                                            _( "Changes not checked in conflict with the latest version." ) );
        }

        if( sync.result.IsOk() )
        {
            fetched = true;
            continue;
        }

        failure = sync.result;
    }

    auto discard = [&]()
    {
        MDW_RESULT discarded = MDW_SETUP::DiscardLibrarySource( aSource, written );

        if( !discarded.IsOk() )
        {
            MdwReportFailure( aParent, wxString::Format( _( "'%s' could not be removed." ), aSource.name ),
                              discarded );
        }
    };

    if( !fetched )
    {
        if( aAdded )
            discard();

        const MDW_RESULT why = failure.value_or( MDW_RESULT::Fail( MDW_ERROR_CODE::CANCELLED ) );

        // Nothing failed to fetch, so the cancel alone is the reason
        if( why.error == MDW_ERROR_CODE::CANCELLED )
        {
            return SET_UP_FAILURE{ wxString::Format( aAdded ? _( "'%s' was not set up because the set-up was "
                                                                 "cancelled." )
                                                            : _( "The libraries of '%s' were not fetched because "
                                                                 "the update was cancelled." ),
                                                     aSource.name ),
                                   false };
        }

        return SET_UP_FAILURE{ wxString::Format( aAdded ? _( "'%s' was not set up because its libraries could not "
                                                             "be fetched." )
                                                        : _( "The libraries of '%s' could not be fetched." ),
                                                 aSource.name )
                                       + wxS( " " ) + MdwErrorText( why ),
                               why.error == MDW_ERROR_CODE::AUTH_FAILED
                                       || why.error == MDW_ERROR_CODE::NOT_AUTHENTICATED };
    }

    if( failure )
    {
        MdwReportFailure( aParent, wxString::Format( _( "The libraries of '%s' could not be fetched." ), aSource.name ),
                          *failure );
    }

    aReport.registered = RegisterLibrarySourceRows( aParent, aKiway, aProject, { aSource }, filesChanged );

    const MDW_ROWS_REGISTERED& registered = aReport.registered;

    // Every library name was taken, so the source would add nothing but its copy and a backend
    if( aAdded && registered.rows == 0 && !( registered.conflicts.empty() && registered.projectConflicts.empty() ) )
    {
        discard();

        std::vector<wxString> taken = registered.conflicts;
        taken.insert( taken.end(), registered.projectConflicts.begin(), registered.projectConflicts.end() );

        return SET_UP_FAILURE{ wxString::Format( _( "'%s' was not set up because libraries with the same names are "
                                                    "already in the library tables. Set it up again with a "
                                                    "nickname prefix." ),
                                                 aSource.name )
                                       + wxS( "\n" ) + nameList( taken ),
                               false };
    }

    return std::nullopt;
}


/// The source aRequirement names, when it can be set up without asking the user anything
std::optional<MDW_LIBRARY_SOURCE_CONFIG> sourceWithoutDialog( const MDW_LIBRARY_REQUIREMENT& aRequirement )
{
    MDW_LIBRARY_SOURCE_CONFIG source;
    source.backend_id = aRequirement.backend_id;
    source.locator = aRequirement.locator;
    source.name = MDW_LIBRARY_SOURCES::NameFor( aRequirement );
    source.prefix = aRequirement.prefix;

    const wxString realm = MDW_SETUP::Realm( wxEmptyString, source.locator );
    const auto     backends = MDW_SETUP::Backends( MDW_CAPABILITY_GROUP::SHARED_LIBRARIES );

    const bool installed = std::any_of( backends.begin(), backends.end(),
                                        [&]( const MDW_BACKEND_CHOICE& aBackend )
                                        {
                                            return aBackend.backend_id == source.backend_id;
                                        } );

    if( !installed || source.name.IsEmpty()
        || !( realm.IsEmpty() || MDW_SETUP::HasCredential( realm, wxEmptyString ) ) )
    {
        return std::nullopt;
    }

    // The name keys the copy, so a taken one is for the user to change
    for( const MDW_LIBRARY_SOURCE_CONFIG& existing : MDW_LIBRARY_SOURCES::Get().Sources() )
    {
        if( existing.name.IsSameAs( source.name, false ) )
            return std::nullopt;
    }

    return source;
}

} // namespace


wxString SetUpLibrarySource( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject,
                             const std::vector<MDW_LIBRARY_REQUIREMENT>& aRequirements )
{
    if( librarySyncRefused( aParent ) )
        return wxEmptyString;

    SCOPED_SET_RESET running( MDW_LIBRARY_SOURCES::Get().LibrarySyncRunning(), true );

    std::vector<MDW_LIBRARY_REQUIREMENT> needs;

    if( aProject )
        needs = aProject->GetProjectFile().m_WorkspaceLibrarySources;

    struct ASKED
    {
        std::optional<MDW_LIBRARY_REQUIREMENT> requirement;
        wxString                               status;
    };

    std::vector<ASKED>    asked;
    std::vector<wxString> unasked;
    wxString              setUp;

    for( const MDW_LIBRARY_REQUIREMENT& requirement : aRequirements )
    {
        std::optional<MDW_LIBRARY_SOURCE_CONFIG> source = sourceWithoutDialog( requirement );

        // A source another one already serves is explained by the dialog
        if( !source || !MDW_LIBRARY_SOURCES::Get().Add( *source ).IsOk() )
        {
            asked.push_back( { requirement, wxEmptyString } );
            continue;
        }

        SET_UP_REPORT                 report;
        std::optional<SET_UP_FAILURE> failure = fetchSource( aParent, aKiway, aProject, *source, true, report );

        // A saved login the server no longer takes is replaced in the dialog, which shows what the server said
        if( failure && failure->signIn )
        {
            asked.push_back( { requirement, failure->text } );
            continue;
        }

        if( failure )
        {
            MdwReportFailure( aParent, failure->text );
            continue;
        }

        reportConflicts( aParent, report.registered, report.told );
        setUp = source->id;
        unasked.push_back( source->name );
    }

    // No dialog opened for these, and a fast fetch closes its progress before it is seen
    if( !unasked.empty() )
        DisplayInfoMessage( aParent, _( "Team libraries set up on this computer:" ), nameList( unasked ) );

    if( aRequirements.empty() )
        asked.emplace_back();

    for( const ASKED& request : asked )
    {
        SET_UP_REPORT             report;
        MDW_LIBRARY_SOURCE_CONFIG source;

        // The dialog is a stack object parented to aParent, so it must not outlive its own loop
        {
            DIALOG_LIBRARY_SOURCE_SETUP dlg(
                    aParent, request.requirement, needs,
                    [&]( wxWindow* aDialog, const MDW_LIBRARY_SOURCE_CONFIG& aSource, bool aAdded )
                    {
                        report = SET_UP_REPORT();
                        std::optional<SET_UP_FAILURE> failure =
                                fetchSource( aDialog, aKiway, aProject, aSource, aAdded, report );

                        return failure ? failure->text : wxString();
                    } );

            dlg.ShowStatus( request.status );

            if( dlg.ShowModal() != wxID_OK )
                break;

            source = dlg.GetSource();
        }

        reportConflicts( aParent, report.registered, report.told );
        setUp = source.id;
    }

    return setUp;
}


bool ConfirmSaveOverChangedParts( wxWindow* aParent, const std::vector<wxString>& aNames )
{
    wxString names;

    for( const wxString& name : aNames )
        AccumulateDescription( names, UnescapeString( name ) );

    KIDIALOG dlg( aParent,
                  wxString::Format( _( "The library file changed since you opened %s. Saving replaces those changes." ),
                                    names ),
                  _( "Library Changed on Disk" ), wxOK | wxCANCEL | wxICON_WARNING );
    dlg.SetOKLabel( _( "Save Anyway" ) );

    return dlg.ShowModal() == wxID_OK;
}


bool MarkOverwrittenParts( wxWindow* aParent, PROJECT& aProject, MDW_LIBRARY_TYPE aType, const LIBRARY_TABLE_ROW& aRow,
                           const std::vector<wxString>& aNames )
{
    const wxString marker = MDW_LIBRARY_SOURCES::MarkerOf( aRow.Options() );

    if( marker.IsEmpty() || aNames.empty() )
        return true;

    MDW_LIBRARY_SOURCES&  sources = MDW_LIBRARY_SOURCES::Get();
    const wxString        path = LIBRARY_MANAGER::GetFullURI( &aRow, true, &aProject );
    std::vector<wxString> items;
    wxString              names;

    for( const wxString& name : aNames )
        AccumulateDescription( names, UnescapeString( name ) );

    // The copy's own index, since the listing may not be loaded and its source may be down
    if( sources.ForRowAt( aType, aRow.Options(), path ) )
    {
        for( const wxString& name : aNames )
        {
            if( std::optional<MDW_ITEM_REF> member = sources.MemberForAt( aType, aRow.Options(), path, name ) )
                items.push_back( member->item_id );
        }

        if( items.empty() || MDW_OVERWRITTEN_PARTS::Get().Mark( marker, items ) )
            return true;
    }

    DisplayErrorMessage( aParent,
                         wxString::Format( _( "%s was not saved, since saving it over a newer version in the team "
                                              "library could not be recorded." ),
                                           names ) );
    return false;
}


MDW_OVERWRITE_ANSWER SettleOverwrittenPart( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject,
                                            const MDW_PART_TARGET& aTarget )
{
    MDW_OVERWRITTEN_PARTS& marks = MDW_OVERWRITTEN_PARTS::Get();

    if( !aTarget.listed || !marks.Has( aTarget.marker, aTarget.item.item_id ) )
        return MDW_OVERWRITE_ANSWER::RELEASE;

    const MDW_LIBRARY_MEMBER& member = aTarget.listed->member;

    // A member never published leaves nothing a release could replace
    if( member.version.IsEmpty() )
    {
        marks.Clear( aTarget.marker, aTarget.item.item_id );
        return MDW_OVERWRITE_ANSWER::RELEASE;
    }

    if( librarySyncRefused( aParent ) )
        return MDW_OVERWRITE_ANSWER::CANCELLED;

    SCOPED_SET_RESET running( MDW_LIBRARY_SOURCES::Get().LibrarySyncRunning(), true );

    MDW_LIBRARY_SOURCES&                     sources = MDW_LIBRARY_SOURCES::Get();
    std::shared_ptr<const MDW_LIBRARY_COPY>  copy = sources.SharedCopy( aTarget.marker );
    std::optional<MDW_LIBRARY_SOURCE_CONFIG> source = copy ? sources.Find( copy->source_id ) : std::nullopt;
    wxFileName                               file( aTarget.localPath );
    SCOPED_TEMP_PATH                         folder;

    auto fail = [&]( const wxString& aWhy )
    {
        DisplayErrorMessage( aParent,
                             wxString::Format( _( "%s was saved over a newer version in the team library and could "
                                                  "not be settled with it, so it was not released." ),
                                               member.name ),
                             aWhy );
        return MDW_OVERWRITE_ANSWER::CANCELLED;
    };

    if( !source || !file.MakeRelativeTo( copy->local_root ) || !folder.MakeDirectory( wxS( "kicad_libmerge_" ), 0700 ) )
        return fail( wxEmptyString );

    // As an update conflict with no base, since the version the save started from is not known
    MDW_UPDATE_CONFLICT conflict;
    conflict.item = aTarget.item;
    conflict.theirs_revision_id = member.version;
    conflict.relative_path = file.GetFullPath( wxPATH_UNIX );
    conflict.name = member.name;

    // Another editor may save the part while the user chooses, and that save must not be lost
    std::optional<std::string> compared = MDW_SETUP::ReadMember( aTarget.localPath, conflict );

    MDW_CONFLICT_SIDES_RESULT sides = RunWithProgress<MDW_CONFLICT_SIDES_RESULT>(
            aParent, _( "Getting the team library's version..." ),
            [&]()
            {
                return MDW_SETUP::ConflictSides( *source, copy->local_root, { conflict }, folder.Path() );
            } );

    if( !sides.result.IsOk() || sides.sides.empty() )
        return fail( MdwErrorText( sides.result ) );

    // Saved back to what the team library holds, there is nothing left to compare
    if( MDW_SETUP::SidesMatch( sides.sides.front() ) )
    {
        marks.Clear( aTarget.marker, aTarget.item.item_id );
        return MDW_OVERWRITE_ANSWER::RELEASE;
    }

    std::vector<MDW_CONFLICT_RESOLUTION> resolutions;
    std::vector<wxString>                backups;
    std::vector<wxString>                outcomes;
    MDW_RESULT                           answered =
            resolveConflicts( aParent, aKiway, *copy, sides.sides, folder.Path(), resolutions, backups, outcomes );

    if( answered.error == MDW_ERROR_CODE::CANCELLED )
        return MDW_OVERWRITE_ANSWER::CANCELLED;

    if( !answered.IsOk() || resolutions.empty() )
        return fail( MdwErrorText( answered ) );

    if( !compared || MDW_SETUP::ReadMember( aTarget.localPath, conflict ) != compared )
        return fail( _( "It changed while the two versions were compared. Release it again to compare them anew." ) );

    const std::optional<std::string>& merged = resolutions.front().merged;

    // Only a member deleted upstream answers with nothing, and a listed member never is
    if( merged
        && ( merged->empty()
             || !MDW_SETUP::WriteMember( aTarget.localPath, conflict, *merged, MDW_SETUP::SymbolUpgrade( aKiway ) ) ) )
        return fail( wxString::Format( _( "Could not write '%s'." ), aTarget.localPath ) );

    // Kept as an update's conflict answer, so the release takes the version settled with as seen
    const MDW_RESULT updated = updateLibrarySources( aParent, aKiway, aProject, { *source },
                                                     { { aTarget.item.item_id, member.version, std::nullopt,
                                                         wxEmptyString, wxEmptyString } } );

    if( updated.IsOk() )
    {
        marks.Clear( aTarget.marker, aTarget.item.item_id );
    }
    else if( merged && updated.error != MDW_ERROR_CODE::CANCELLED )
    {
        DisplayInfoMessage( aParent, wxString::Format( _( "%s now holds your merge, but it was not released because "
                                                          "the team library could not be updated. Release it again "
                                                          "once what stopped the update is resolved." ),
                                                       member.name ) );
    }

    if( !backups.empty() )
        DisplayInfoMessage( aParent, outcomeMessage( outcomes, backups ) );

    // A merge on disk must reach the editor even unsettled, or its next save would put the old version back
    if( merged )
        return MDW_OVERWRITE_ANSWER::REWRITTEN;

    return updated.IsOk() ? MDW_OVERWRITE_ANSWER::RELEASE : MDW_OVERWRITE_ANSWER::CANCELLED;
}
