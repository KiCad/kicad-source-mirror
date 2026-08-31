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

#include <widgets/mdw_workspace_strip.h>

#include <confirm.h>
#include <dialogs/dialog_library_source_setup.h>
#include <dialogs/dialog_workspace_config.h>
#include <dialogs/git/dialog_mdw_design_release.h>
#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/mdw_connection_prompt.h>
#include <dialogs/git/workspace_item_files.h>
#include <eda_base_frame.h>
#include <git/kicad_git_common.h>
#include <kidialog.h>
#include <kiway.h>
#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_library_sources.h>
#include <mdw/client/mdw_paged_read.h>
#include <mdw/client/mdw_project_file.h>
#include <mdw/client/mdw_source.h>
#include <mdw/mdw_framework.h>
#include <mdw/mdw_session.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>
#include <widgets/git_commit_pane.h>
#include <widgets/mdw_editing.h>
#include <wildcards_and_files_ext.h>

#include <wx/app.h>
#include <wx/button.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/weakref.h>
#include <wx/intl.h>

#include <algorithm>
#include <optional>
#include <set>
#include <tuple>


namespace
{
enum
{
    ID_SETTINGS = wxID_HIGHEST + 1,
    ID_CONNECT_LIBRARY,
    ID_UPDATE_LIBRARIES,
    ID_PREFERENCES,
    ID_RECONNECT,
    ID_DISCONNECT,
    ID_UNBIND,
    ID_RELEASE_DESIGN,
    ID_ADD_TO_WORKSPACE,
    ID_OPEN_FROM_WORKSPACE,
    ID_CHANGE_ORDERS
};


/// Files a backend listed, wrote or took, with what it said about any it would not
struct WORKSPACE_FILES
{
    MDW_RESULT                       status = MDW_RESULT::Ok();
    std::vector<MDW_ITEM_INFO>       listed;
    std::vector<MDW_CHECKOUT_RESULT> done;
    wxArrayString                    failed;
};


// aSnapshot is the project file from before the backend wrote anything
void recordFiles( wxWindow* aParent, const wxString& aProjectPath, const std::string& aSnapshot,
                  const WORKSPACE_FILES& aFiles, const wxString& aDone )
{
    PROJECT*      project = Pgm().GetSettingsManager().GetProject( aProjectPath );
    bool          recorded = true;
    wxArrayString where;

    for( const MDW_CHECKOUT_RESULT& file : aFiles.done )
    {
        where.Add( file.item.path_hint.IsEmpty() ? file.local_path : file.item.path_hint );

        if( !project
            || !MDW_SAVE_GUARD::RecordCheckout( Pgm().GetSettingsManager(), *project, file.local_path,
                                                file.item.item_id, { file.item.revision_id, file.item.revision_label },
                                                aSnapshot ) )
        {
            recorded = false;
        }
    }

    if( !aFiles.status.IsOk() || !aFiles.failed.IsEmpty() )
    {
        if( aFiles.status.IsOk() )
            MdwReportFailure( aParent, aDone, wxJoin( aFiles.failed, '\n', '\0' ) );
        else
            MdwReportFailure( aParent, aDone, aFiles.status );
    }
    else if( !recorded )
    {
        MdwReportFailure( aParent, _( "The workspace has the files, but the project's settings could not be "
                                      "saved." ) );
    }
    else
    {
        DisplayInfoMessage( aParent, aDone, wxJoin( where, '\n', '\0' ) );
    }
}
}


MDW_WORKSPACE_STRIP::MDW_WORKSPACE_STRIP( wxWindow* aParent, EDA_BASE_FRAME* aFrame ) :
        wxPanel( aParent ),
        m_frame( aFrame )
{
    m_title = new wxStaticText( this, wxID_ANY, _( "Design Workspace" ) );
    m_title->SetFont( m_title->GetFont().Bold() );
    m_state = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                wxST_ELLIPSIZE_END | wxST_NO_AUTORESIZE );

    // The state shrinks first so the action and menu buttons stay inside a narrow pane
    m_state->SetMinSize( wxSize( 0, -1 ) );
    m_actionButton = new wxButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    m_menuButton = new wxButton( this, wxID_ANY, wxS( "\u22EF" ), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );

    // An exact fit around one glyph leaves Windows at 100% room for a single dot
    m_menuButton->SetMinSize( wxSize( m_menuButton->GetTextExtent( m_menuButton->GetLabel() ).x + FromDIP( 16 ), -1 ) );
    m_menuButton->SetToolTip( _( "Design Workspace actions" ) );

    wxBoxSizer* row = new wxBoxSizer( wxHORIZONTAL );
    row->Add( m_title, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6 );
    row->Add( m_state, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6 );
    row->Add( m_actionButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2 );
    row->Add( m_menuButton, 0, wxALIGN_CENTER_VERTICAL );
    SetSizer( row );

    m_actionButton->Bind( wxEVT_BUTTON, &MDW_WORKSPACE_STRIP::onAction, this );
    m_menuButton->Bind( wxEVT_BUTTON, &MDW_WORKSPACE_STRIP::onMenu, this );

    // Opening, replacing or closing the session changes what the strip shows
    m_connections = MDW_CLIENT::Get().SubscribeConnections( this,
                                                            [this]( const wxString& aProjectPath )
                                                            {
                                                                if( aProjectPath == m_projectPath )
                                                                    Sync();
                                                            } );

    setAction( ACTION::NONE );
    Hide();
}


MDW_WORKSPACE_STRIP::~MDW_WORKSPACE_STRIP() = default;


bool MDW_WORKSPACE_STRIP::available() const
{
    return MDW_CLIENT::Get().ProjectConnection( m_frame->Prj() ).status.phase != MDW_CONNECTION::UNAVAILABLE;
}


void MDW_WORKSPACE_STRIP::setAction( ACTION aAction )
{
    m_action = aAction;

    if( aAction == ACTION::CONNECT )
        m_actionButton->SetLabel( _( "Connect Project..." ) );
    else if( aAction == ACTION::RESTART )
        m_actionButton->SetLabel( _( "Restart" ) );
    else if( aAction == ACTION::SIGN_IN )
        m_actionButton->SetLabel( _( "Sign In..." ) );
    else if( aAction == ACTION::VERIFY )
        m_actionButton->SetLabel( _( "Verify Server..." ) );

    m_actionButton->Show( aAction != ACTION::NONE );
}


void MDW_WORKSPACE_STRIP::showLibraries()
{
    const MDW_LIBRARY_STATUS libraries = MDW_SETUP::LibraryStatus(
            m_frame->Prj().GetProjectFile().m_WorkspaceLibrarySources, MDW_LIBRARY_SOURCES::TeamLibrariesEnabled() );
    const int count = static_cast<int>( libraries.states.size() );

    if( libraries.notice == MDW_LIBRARY_NOTICE::NOT_SET_UP )
    {
        m_state->SetLabel(
                m_state->GetLabel() + wxS( " \u00B7 " )
                + wxString::Format( wxPLURAL( "%d team library not set up", "%d team libraries not set up", count ),
                                    count ) );

        if( m_action == ACTION::NONE )
        {
            setAction( ACTION::LIBRARIES );
            m_actionButton->SetLabel( _( "Set Up Libraries..." ) );
        }
    }
    else if( libraries.notice == MDW_LIBRARY_NOTICE::NOT_FETCHED )
    {
        m_state->SetLabel(
                m_state->GetLabel() + wxS( " \u00B7 " )
                + wxString::Format( wxPLURAL( "%d team library not fetched", "%d team libraries not fetched", count ),
                                    count ) );

        if( m_action == ACTION::NONE )
        {
            setAction( ACTION::LIBRARIES );
            m_actionButton->SetLabel( _( "Get Library Updates" ) );
        }
    }
}


void MDW_WORKSPACE_STRIP::Sync()
{
    PROJECT& project = m_frame->Prj();

    m_projectPath = project.GetProjectFullName();

    MDW_PROJECT_CONNECTION connection = MDW_CLIENT::Get().ProjectConnection( project );
    bool show = !m_projectPath.IsEmpty() && connection.status.phase != MDW_CONNECTION::UNAVAILABLE;

    if( show && connection.status.phase == MDW_CONNECTION::UNBOUND )
    {
        const bool here = project.GetLocalSettings().m_WorkspaceDisabledHere;

        m_state->SetLabel( here ? _( "Disconnected on this computer" ) : _( "Not connected" ) );
        m_state->SetToolTip( here ? _( "The project file still holds the team's binding. Choose Connect on This "
                                       "Computer in the menu to use it again." )
                                  : wxString() );
        setAction( ACTION::CONNECT );
        showLibraries();
    }
    else if( show )
    {
        // The title names the feature on every line, so the backend leads the state instead
        wxString source = connection.source_name;

        if( !connection.workspace_id.IsEmpty() )
            source << wxS( " \u00B7 " ) << connection.workspace_id;

        // Only an idle project connects on its own; a stopped one waits for Restart
        if( connection.status.phase == MDW_CONNECTION::IDLE )
        {
            MDW_CLIENT::Get().ConnectProject( project );
            connection = MDW_CLIENT::Get().ProjectConnection( project );
        }

        const MDW_CONNECTION_STATUS& status = connection.status;
        wxString                     label = MdwConnectionName( status.phase );

        // A backend too old to check its server stops again however often it restarts
        const bool tooOld =
                status.phase == MDW_CONNECTION::BACKEND_STOPPED && status.reason.error == MDW_ERROR_CODE::PROTOCOL;

        if( status.phase == MDW_CONNECTION::ONLINE && !status.account.Name().IsEmpty() )
        {
            label = wxString::Format( _( "Connected as %s" ), status.account.NameAndLogin() );
        }
        else if( status.phase == MDW_CONNECTION::OFFLINE && status.lastOnline.IsValid() )
        {
            label << wxS( " \u00B7 " )
                  << wxString::Format( _( "last connected %s" ), status.lastOnline.Format( wxS( "%H:%M" ) ) );
        }
        else if( tooOld )
        {
            label << wxS( " \u00B7 " ) << MdwErrorMessage( status.reason );
        }

        m_state->SetLabel( source + wxS( " \u00B7 " ) + label );
        m_state->SetToolTip( status.reason.IsOk() ? wxString() : MdwErrorText( status.reason ) );

        switch( status.phase )
        {
        case MDW_CONNECTION::SIGN_IN_NEEDED: setAction( ACTION::SIGN_IN ); break;
        case MDW_CONNECTION::TRUST_NEEDED: setAction( ACTION::VERIFY ); break;
        case MDW_CONNECTION::BACKEND_STOPPED: setAction( tooOld ? ACTION::NONE : ACTION::RESTART ); break;
        default: setAction( ACTION::NONE ); break;
        }

        showLibraries();
    }

    if( m_state->GetToolTipText().IsEmpty() )
        m_state->SetToolTip( m_state->GetLabel() );

    if( show != IsShown() )
        Show( show );

    if( GetParent() )
        GetParent()->Layout();
}


void MDW_WORKSPACE_STRIP::onAction( wxCommandEvent& aEvent )
{
    if( m_action == ACTION::CONNECT )
    {
        ShowSettings();
    }
    else if( m_action == ACTION::RESTART )
    {
        // Closing resets the connect state, so the sync it triggers opens a fresh session
        MDW_CLIENT::Get().CloseProject( m_projectPath );
    }
    else if( m_action == ACTION::LIBRARIES )
    {
        PROJECT&                 project = m_frame->Prj();
        const MDW_LIBRARY_STATUS libraries = MDW_SETUP::LibraryStatus(
                project.GetProjectFile().m_WorkspaceLibrarySources, MDW_LIBRARY_SOURCES::TeamLibrariesEnabled() );

        if( libraries.notice == MDW_LIBRARY_NOTICE::NOT_SET_UP )
        {
            std::vector<MDW_LIBRARY_REQUIREMENT> requirements;

            for( const MDW_REQUIREMENT_STATE& state : libraries.states )
                requirements.push_back( state.requirement );

            SetUpLibrarySource( m_frame, m_frame->Kiway(), &project, requirements );
        }
        else
        {
            UpdateLibrarySources( m_frame, m_frame->Kiway(), &project, ProjectLibrarySources( project ) );
        }

        Sync();
    }
    else if( m_action == ACTION::SIGN_IN || m_action == ACTION::VERIFY )
    {
        // The answer can come after a keychain unlock, by when another project may be open
        MdwResolveConnection( m_frame, MDW_CLIENT::Get().ProjectConnection( m_frame->Prj() ).status,
                              [frame = m_frame, path = m_projectPath]( const MDW_CONNECTION_ANSWER& aAnswer )
                              {
                                  if( frame->Prj().GetProjectFullName() != path )
                                      return;

                                  // The next open signs in as whoever signed in here
                                  if( aAnswer.credentials )
                                  {
                                      auto user = aAnswer.credentials->fields.find( wxS( "username" ) );

                                      if( user != aAnswer.credentials->fields.end() )
                                      {
                                          MDW_SETUP::RememberProjectAccount( frame->Prj(),
                                                                             MDW_CLIENT::ProjectBinding( frame->Prj() ),
                                                                             user->second );
                                      }
                                  }

                                  MDW_CLIENT::Get().ReconnectProject( frame->Prj(), aAnswer );
                              } );
    }
}


void MDW_WORKSPACE_STRIP::ShowSettings()
{
    if( !available() )
        return;

    PROJECT&                  project = m_frame->Prj();
    PROJECT_LOCAL_SETTINGS&   local = project.GetLocalSettings();
    const MDW_PROJECT_BINDING binding = project.GetProjectFile().WorkspaceBinding();
    const wxString            username = MDW_CLIENT::BindsGitRemote( binding ) ? local.m_GitRepoUsername : wxString();
    DIALOG_WORKSPACE_CONFIG   dlg( m_frame, m_projectPath, binding, username, local.m_GitSSHKey );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // The session reopens on apply and signs in as whoever was typed here
    MDW_SETUP::RememberProjectAccount( project, dlg.GetBinding(), dlg.GetUsername() );

    // The key is the git remote's, so the project's own git operations take it too
    if( std::optional<wxString> key = dlg.GetSSHKey(); key && MDW_CLIENT::BindsGitRemote( dlg.GetBinding() ) )
    {
        local.m_GitSSHKey = *key;

        if( !key->IsEmpty() && project.GetGitCommon() )
            project.GetGitCommon()->SetSSHKey( *key );
    }

    if( !ApplyWorkspaceBinding( project, m_projectPath, dlg.GetBinding() ) )
        DisplayErrorMessage( m_frame, _( "The project's workspace settings could not be saved." ) );
}


void MDW_WORKSPACE_STRIP::onMenu( wxCommandEvent& aEvent )
{
    if( !available() )
        return;

    bool                        bound = !m_frame->Prj().GetProjectFile().m_WorkspaceBackend.IsEmpty();
    bool                        here = m_frame->Prj().GetLocalSettings().m_WorkspaceDisabledHere;
    std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().ProjectSource();
    wxMenu                      menu;

    // An upload backend holds a design only once its files are added, and gives them back by checkout
    bool upload = source && source->Info().backend.checkin_model == MDW_CHECKIN_MODEL::UPLOAD;

    menu.Append( ID_CONNECT_LIBRARY, _( "Connect Team Library..." ) );
    menu.Append( ID_UPDATE_LIBRARIES, _( "Get Library Updates" ) )
            ->Enable( !ProjectLibrarySources( m_frame->Prj() ).empty() );
    menu.Append( ID_RELEASE_DESIGN, _( "Release Design..." ) )
            ->Enable( source && source->Has( MDW_CAPABILITY_GROUP::REVISIONS ) );

    if( MDW_FRAMEWORK* framework = Pgm().MdwFrameworkOrNull(); source && framework )
    {
        const MDW_BACKEND_MANIFEST* manifest = framework->Registry().Find( source->Info().backend.backend_id );

        // The backend can serve change orders, so their absence is this server's, not the backend's
        if( manifest
            && std::any_of( manifest->capability_hints.begin(), manifest->capability_hints.end(),
                            [&]( MDW_CAPABILITY_GROUP aGroup )
                            {
                                return ( aGroup == MDW_CAPABILITY_GROUP::ECO || aGroup == MDW_CAPABILITY_GROUP::REVIEW )
                                       && !source->Has( aGroup );
                            } ) )
        {
            menu.Append( ID_CHANGE_ORDERS,
                         wxString::Format( _( "Change orders (needs %s PLM)" ), source->Info().backend.display_name ) )
                    ->Enable( false );
        }
    }

    if( upload )
    {
        menu.AppendSeparator();
        menu.Append( ID_OPEN_FROM_WORKSPACE, _( "Open Project from Workspace..." ) );
        menu.Append( ID_ADD_TO_WORKSPACE, _( "Add to Workspace" ) );
    }

    menu.AppendSeparator();
    menu.Append( ID_SETTINGS, _( "Workspace Settings..." ) );
    menu.Append( ID_PREFERENCES, _( "Design Workspace Preferences..." ) );
    menu.Append( ID_RECONNECT, here ? _( "Connect on This Computer" ) : _( "Reconnect" ) )->Enable( bound );
    menu.Append( ID_DISCONNECT, _( "Disconnect on This Computer" ) )->Enable( bound && !here );
    menu.Append( ID_UNBIND, _( "Unbind Project for Everyone..." ) )->Enable( bound );

    int choice = GetPopupMenuSelectionFromUser( menu, m_menuButton->GetPosition() );

    if( choice == ID_CONNECT_LIBRARY )
    {
        SetUpLibrarySource( m_frame, m_frame->Kiway(), &m_frame->Prj() );
        Sync();
    }
    else if( choice == ID_UPDATE_LIBRARIES )
    {
        UpdateLibrarySources( m_frame, m_frame->Kiway(), &m_frame->Prj(), ProjectLibrarySources( m_frame->Prj() ) );
    }
    else if( choice == ID_RELEASE_DESIGN )
    {
        // The docked commit pane holds the strip, and commits and pushes for the release
        ShowDesignRelease( m_frame, dynamic_cast<GIT_COMMIT_PANE*>( GetParent() ) );
    }
    else if( choice == ID_ADD_TO_WORKSPACE )
    {
        addToWorkspace();
    }
    else if( choice == ID_OPEN_FROM_WORKSPACE )
    {
        openFromWorkspace();
    }
    else if( choice == ID_SETTINGS )
    {
        ShowSettings();
    }
    else if( choice == ID_PREFERENCES )
    {
        m_frame->ShowPreferences( _( "Design Workspace" ), wxEmptyString );
    }
    else if( choice == ID_RECONNECT && here )
    {
        if( !MDW_SETUP::ReconnectHere( m_frame->Prj() ) )
            DisplayErrorMessage( m_frame, _( "The project's workspace settings could not be saved." ) );

        Sync();
    }
    else if( choice == ID_RECONNECT )
    {
        MDW_CLIENT::Get().ReconnectProject( m_frame->Prj() );
    }
    else if( choice == ID_DISCONNECT )
    {
        if( !MDW_SETUP::DisconnectHere( m_frame->Prj() ) )
            DisplayErrorMessage( m_frame, _( "The project's workspace settings could not be saved." ) );

        Sync();
    }
    else if( choice == ID_UNBIND )
    {
        wxFileName file( m_projectPath );

        KIDIALOG dlg( m_frame,
                      wxString::Format( _( "Unbind %s from its Design Workspace for everyone?\n\nThis changes %s, "
                                           "which your teammates share." ),
                                        file.GetName(), file.GetFullName() ),
                      _( "Unbind Project" ), wxOK | wxCANCEL | wxICON_WARNING );
        dlg.SetOKCancelLabels( _( "Unbind" ), _( "Cancel" ) );

        if( dlg.ShowModal() == wxID_OK && !ApplyWorkspaceBinding( m_frame->Prj(), m_projectPath, {} ) )
            DisplayErrorMessage( m_frame, _( "The project's workspace settings could not be saved." ) );
    }
}


std::shared_ptr<MDW_SESSION> MDW_WORKSPACE_STRIP::session() const
{
    MDW_FRAMEWORK* framework = Pgm().MdwFrameworkOrNull();

    return framework ? framework->Session( m_projectPath ) : nullptr;
}


void MDW_WORKSPACE_STRIP::addToWorkspace()
{
    std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().ProjectSource();
    PROJECT&                    project = m_frame->Prj();
    std::vector<wxString>       names;
    wxDir                       dir( project.GetProjectPath() );
    wxString                    name;

    if( !source || !session() )
        return;

    // Upload what is saved, never what an editor still holds
    if( DocumentEditorsDirty( m_frame->Kiway() ) )
    {
        DisplayErrorMessage( m_frame, _( "Save your changes before adding the project to the workspace." ) );
        return;
    }

    for( bool found = dir.IsOpened() && dir.GetFirst( &name, wxEmptyString, wxDIR_FILES ); found;
         found = dir.GetNext( &name ) )
    {
        wxFileName file( project.GetProjectPath(), name );
        wxString   ext = file.GetExt();

        // A checked out file is already an item, and adding it again would fork its history
        if( ( ext == FILEEXT::ProjectFileExtension || ext == FILEEXT::KiCadSchematicFileExtension
              || ext == FILEEXT::KiCadPcbFileExtension )
            && project.GetLocalSettings().GetWorkspaceItemId( file.GetFullPath() ).IsEmpty() )
        {
            names.push_back( name );
        }
    }

    if( names.empty() )
    {
        DisplayInfoMessage( m_frame, _( "Every design file of this project is already in the workspace." ) );
        return;
    }

    m_addLocate = source->Locate(
            names,
            [this, names, projectFile = project.GetProjectFullName(), projectDir = project.GetProjectPath()](
                    const MDW_RESULT& aResult, const std::vector<MDW_LOCATED_ITEM>& aFound )
            {
                std::vector<MDW_ADD_ITEM_REQUEST> requests;

                // The names are the old project's, and adding them would upload the new project's files
                if( m_frame->Prj().GetProjectFullName() != projectFile )
                    return;

                // A backend that cannot locate leaves every file to AddItem, which refuses one it holds
                for( size_t ii = 0; ii < names.size(); ++ii )
                {
                    if( !aResult.IsOk() || ii >= aFound.size() || aFound[ii].item.item_id.IsEmpty() )
                    {
                        requests.push_back( { names[ii], wxFileName( projectDir, names[ii] ).GetFullPath(),
                                              _( "Added from KiCad" ), wxEmptyString } );
                    }
                }

                if( requests.empty() )
                {
                    DisplayInfoMessage( m_frame,
                                        _( "The workspace already holds every design file of this project." ) );
                }
                else
                {
                    addItems( requests );
                }
            } );
}


void MDW_WORKSPACE_STRIP::addItems( const std::vector<MDW_ADD_ITEM_REQUEST>& aRequests )
{
    std::shared_ptr<MDW_SESSION> current = session();
    wxWeakRef<wxWindow>          parent( m_frame );
    std::string                  snapshot = MDW_SAVE_GUARD::ProjectFileSnapshot( m_frame->Prj() );

    if( !current )
        return;

    // The app owns the completion, so files the backend took are bound even if this window closes first
    current->RunAsync<WORKSPACE_FILES>(
            wxTheApp,
            [aRequests, source = MDW_CLIENT::Get().ProjectSource()]()
            {
                WORKSPACE_FILES files;

                for( const MDW_ADD_ITEM_REQUEST& request : aRequests )
                {
                    MDW_CHECKOUT_RESULT added;
                    MDW_RESULT          result = MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );

                    if( source )
                    {
                        std::tie( result, added ) = MdwSync<MDW_CHECKOUT_RESULT>(
                                [&]( MDW_DONE<MDW_CHECKOUT_RESULT> aDone )
                                {
                                    return source->AddItem( request, std::move( aDone ) );
                                } );
                    }

                    if( result.IsOk() )
                        files.done.push_back( added );
                    else
                        files.failed.Add( request.relative_path + wxS( ": " ) + MdwErrorText( result ) );
                }

                return files;
            },
            [parent, projectPath = m_projectPath, snapshot]( WORKSPACE_FILES aFiles )
            {
                int count = (int) aFiles.done.size();

                recordFiles( parent.get(), projectPath, snapshot, aFiles,
                             wxString::Format( wxPLURAL( "Added %d file to the workspace.",
                                                         "Added %d files to the workspace.", count ),
                                               count ) );
            } );
}


void MDW_WORKSPACE_STRIP::openFromWorkspace()
{
    std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().ProjectSource();

    if( !source || !session() )
        return;

    // A checkout writes the design's files, which an editor would then hold stale
    if( DocumentEditorsDirty( m_frame->Kiway() ) )
    {
        DisplayErrorMessage( m_frame, _( "Save your changes before checking out the design." ) );
        return;
    }

    wxString projectFile = wxFileName( m_frame->Prj().GetProjectFullName() ).GetFullName();

    // Only a binding that names one design can say which item a project file is
    m_openLocate = source->Locate( { projectFile },
                                   [this]( const MDW_RESULT& aResult, const std::vector<MDW_LOCATED_ITEM>& )
                                   {
                                       if( aResult.IsOk() )
                                       {
                                           listDesign();
                                           return;
                                       }

                                       MdwReportFailure( m_frame,
                                                         aResult.error == MDW_ERROR_CODE::NOT_SUPPORTED
                                                                 ? _( "The workspace binding names no design." )
                                                                 : _( "Could not check the workspace binding." ),
                                                         aResult );
                                   } );
}


void MDW_WORKSPACE_STRIP::listDesign()
{
    std::shared_ptr<MDW_SESSION>   current = session();
    wxWeakRef<MDW_WORKSPACE_STRIP> strip( this );

    if( !current )
        return;

    current->RunAsync<WORKSPACE_FILES>(
            this,
            [source = MDW_CLIENT::Get().ProjectSource()]()
            {
                WORKSPACE_FILES files;

                if( !source )
                {
                    files.status = MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );
                    return files;
                }

                // An empty folder is the root of what the binding names, which for a design is its files
                auto [status, items] = MdwSync<MDW_PAGED<MDW_ITEM_INFO>>(
                        [&]( MDW_DONE<MDW_PAGED<MDW_ITEM_INFO>> aDone )
                        {
                            return MdwReadAllPages<MDW_ITEM_INFO>(
                                    [&]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_ITEM_INFO>> aPageDone )
                                    {
                                        return source->Items( wxEmptyString, wxEmptyString, aPage,
                                                              std::move( aPageDone ) );
                                    },
                                    0, std::move( aDone ) );
                        } );

                files.status = status;
                files.listed = std::move( items.rows );

                return files;
            },
            [strip]( WORKSPACE_FILES aFiles )
            {
                if( strip )
                    strip->checkOutDesign( aFiles.status, aFiles.listed );
            } );
}


void MDW_WORKSPACE_STRIP::checkOutDesign( const MDW_RESULT& aStatus, const std::vector<MDW_ITEM_INFO>& aItems )
{
    std::shared_ptr<MDW_SESSION>      current = session();
    const wxString                    projectDir = m_frame->Prj().GetProjectPath();
    std::vector<MDW_CHECKOUT_REQUEST> requests;
    std::set<wxString>                destinations;
    wxArrayString                     names;

    if( !aStatus.IsOk() )
    {
        MdwReportFailure( m_frame, _( "Could not list the workspace's files." ), aStatus );
        return;
    }

    for( const MDW_ITEM_INFO& item : aItems )
    {
        wxString destination = WorkspaceCheckoutPath( projectDir, item );

        // A file already here may hold unsaved or unshared work, so it is never replaced
        if( !item.is_folder && !destination.IsEmpty() && !wxFileName::Exists( destination )
            && destinations.insert( destination ).second )
        {
            requests.push_back( { item.item_id, destination, false, wxEmptyString } );
            names.Add( item.relative_path );
        }
    }

    if( !current || requests.empty() )
    {
        DisplayInfoMessage( m_frame, _( "This project already has every file the workspace names for it." ) );
        return;
    }

    if( !IsOK( m_frame, wxString::Format( _( "Check out these files into %s?\n\n%s" ), projectDir,
                                          wxJoin( names, '\n', '\0' ) ) ) )
    {
        return;
    }

    wxWeakRef<wxWindow> parent( m_frame );
    std::string         snapshot = MDW_SAVE_GUARD::ProjectFileSnapshot( m_frame->Prj() );

    current->RunAsync<WORKSPACE_FILES>(
            wxTheApp,
            [requests, source = MDW_CLIENT::Get().ProjectSource()]()
            {
                WORKSPACE_FILES files;

                for( const MDW_CHECKOUT_REQUEST& request : requests )
                {
                    MDW_CHECKOUT_RESULT checkedOut;
                    MDW_RESULT          result = MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );

                    // A file may have appeared while the user read the question
                    if( wxFileName::Exists( request.local_path ) )
                        result = MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER, _( "A file is already there." ) );
                    else if( source )
                    {
                        std::tie( result, checkedOut ) = MdwSync<MDW_CHECKOUT_RESULT>(
                                [&]( MDW_DONE<MDW_CHECKOUT_RESULT> aDone )
                                {
                                    return source->Checkout( request, std::move( aDone ) );
                                } );
                    }

                    if( result.IsOk() )
                        files.done.push_back( checkedOut );
                    else
                        files.failed.Add( request.local_path + wxS( ": " ) + MdwErrorText( result ) );
                }

                return files;
            },
            [parent, projectPath = m_projectPath, snapshot]( WORKSPACE_FILES aFiles )
            {
                int count = (int) aFiles.done.size();

                recordFiles( parent.get(), projectPath, snapshot, aFiles,
                             wxString::Format( wxPLURAL( "Checked out %d file from the workspace.",
                                                         "Checked out %d files from the workspace.", count ),
                                               count ) );
            } );
}
