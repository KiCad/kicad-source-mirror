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

#include <widgets/git_commit_pane.h>
#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_status_entries.h>
#include <frame_type.h>
#include <kiway.h>
#include <mail_type.h>

#include <algorithm>
#include <memory>

#include <wx/app.h>
#include <wx/button.h>
#include <wx/dataview.h>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <wx/choicdlg.h>

#include <confirm.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <diff_merge/merge_dispatch.h>
#include <kiplatform/ui.h>
#include <diff_merge/diff_scene.h>
#include <reporter.h>
#include <git/kicad_git_common.h>
#include <git/git_branch_handler.h>
#include <git/git_commit_handler.h>
#include <git/git_config_handler.h>
#include <git/git_pull_handler.h>
#include <git/git_push_handler.h>
#include <eda_base_frame.h>
#include <kiplatform/environment.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/common_settings.h>
#include <thread>
#include <wildcards_and_files_ext.h>
#include <widgets/file_checklist_ctrl.h>


// Make aName at HEAD and move onto it, so the next commit lands there. The caller holds the git
// action lock. A failure leaves nothing behind and says why in aError and aDetail.
static bool moveToNewBranch( KIGIT_COMMON& aCommon, const wxString& aName, wxString& aError, wxString& aDetail )
{
    GIT_BRANCH_HANDLER branchHandler( &aCommon );

    // With no commits there is nothing to branch from, so the first commit makes the branch.
    if( aCommon.IsHeadUnborn() )
    {
        if( !aCommon.StartUnbornBranch( aName ) )
        {
            aError = wxString::Format( _( "Could not start branch '%s'." ), aName );
            aDetail = KIGIT_COMMON::GetLastGitError();
            return false;
        }

        return true;
    }

    if( !aCommon.CreateBranch( aName ) )
    {
        aError = wxString::Format( _( "Could not create branch '%s'." ), aName );
        aDetail = KIGIT_COMMON::GetLastGitError();
        return false;
    }

    if( branchHandler.SwitchToBranch( aName ) != BranchResult::Success )
    {
        aError = wxString::Format( _( "Could not switch to the new branch '%s'." ), aName );
        aDetail = branchHandler.GetErrorString();
        aCommon.DeleteBranch( aName );
        return false;
    }

    return true;
}


// Go back to aPrevious and drop aName, after the commit meant for it failed. The caller holds
// the git action lock.
static void dropNewBranch( KIGIT_COMMON& aCommon, const wxString& aName, const wxString& aPrevious )
{
    // Before the first commit there was no branch to come back to, and the name is only where
    // HEAD points.
    if( aPrevious.IsEmpty() )
        return;

    GIT_BRANCH_HANDLER branchHandler( &aCommon );
    branchHandler.SwitchToBranch( aPrevious );
    aCommon.DeleteBranch( aName );
}


// Claim a name for the version a conflict is about to replace. An earlier copy beside the same
// file may be the only remaining source of its work, so it is never overwritten.
static bool reserveRecoveryFile( const wxString& aPreferredPath, wxString& aPathOut, wxString& aCreatedDir )
{
    aPathOut.Clear();
    aCreatedDir.Clear();

    wxFileName parent( aPreferredPath );

    if( !parent.DirExists() )
    {
        if( !parent.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
            return false;

        aCreatedDir = parent.GetPath();
    }

    for( int attempt = 0; attempt < 100; attempt++ )
    {
        wxString candidate =
                attempt == 0 ? aPreferredPath : wxString::Format( wxS( "%s.%d" ), aPreferredPath, attempt );

        // Exclusive create, so two names are never handed out for one file.
        wxFile out;

        if( out.Create( candidate, false ) )
        {
            aPathOut = candidate;
            return true;
        }

        // Free and still not creatable means the filesystem is refusing, and counting up
        // will not change that.
        if( !wxFileName::FileExists( candidate ) )
            return false;
    }

    return false;
}


GIT_COMMIT_PANE::GIT_COMMIT_PANE( EDA_BASE_FRAME* aParent ) :
        GIT_COMMIT_PANE( aParent, aParent )
{
}


GIT_COMMIT_PANE::GIT_COMMIT_PANE( wxWindow* aParent, EDA_BASE_FRAME* aFrame ) :
        GIT_COMMIT_PANE_BASE( aParent ),
        m_frame( aFrame ),
        m_hasUnpushedCommits( false ),
        m_hasRemote( false ),
        m_headDetached( false ),
        m_remoteAhead( false ),
        m_operationInProgress( false ),
        m_operation( GitOperation::None ),
        m_editorsDirty( false ),
        m_networkBusy( false )
{
    m_summary->SetHint( _( "Summarize your changes..." ) );
    m_newBranchName->SetHint( _( "Branch name" ) );
    updateSummaryCount();

    // Only a platform without a native hint for a multi line box gets the label laid over it.
    if( !m_description->SetHint( _( "Extended Description (optional)" ) ) )
    {
        m_descriptionHint = new wxStaticText( this, wxID_ANY, _( "Extended Description (optional)" ) );
        m_descriptionHint->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
        m_descriptionHint->Bind( wxEVT_LEFT_DOWN,
                                 [this]( wxMouseEvent& )
                                 {
                                     m_description->SetFocus();
                                 } );
    }

    m_advancedButton->Bind( wxEVT_BUTTON, &GIT_COMMIT_PANE::onOpenAdvanced, this );
    m_commitButton->Bind( wxEVT_BUTTON, &GIT_COMMIT_PANE::onCommit, this );
    m_commitOnlyButton->Bind( wxEVT_BUTTON, &GIT_COMMIT_PANE::onCommitOnly, this );
    m_pullButton->Bind( wxEVT_BUTTON, &GIT_COMMIT_PANE::onPull, this );
    m_amendCheckbox->Bind( wxEVT_CHECKBOX, &GIT_COMMIT_PANE::onAmendToggled, this );
    m_summary->Bind( wxEVT_TEXT, &GIT_COMMIT_PANE::onMessageChanged, this );
    m_newBranchName->Bind( wxEVT_TEXT,
                           [this]( wxCommandEvent& )
                           {
                               updateCommitButton();
                           } );
    m_newBranchCheckbox->Bind( wxEVT_CHECKBOX, &GIT_COMMIT_PANE::onNewBranchToggled, this );
    m_branchChoice->Bind( wxEVT_CHOICE, &GIT_COMMIT_PANE::onBranchChosen, this );
    m_showDiffButton->Bind( wxEVT_BUTTON, &GIT_COMMIT_PANE::onShowDiff, this );
    m_description->Bind( wxEVT_TEXT,
                         [this]( wxCommandEvent& )
                         {
                             updateDescriptionHint();

                             if( m_messageChangedHandler )
                                 m_messageChangedHandler();
                         } );
    m_checklist->Bind( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &GIT_COMMIT_PANE::onChecklistChanged, this );
    m_checklist->SetToolTip( _( "Double click a file to see what changed in it. Right click for more." ) );
    m_checklist->Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &GIT_COMMIT_PANE::onFileActivated, this );
    m_checklist->Bind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &GIT_COMMIT_PANE::onChecklistContextMenu, this );
    m_checklist->Bind( wxEVT_DATAVIEW_SELECTION_CHANGED,
                       [this]( wxDataViewEvent& )
                       {
                           m_showDiffButton->Enable( m_checklist->GetSelection().IsOk() );

                           if( m_fileSelectedHandler )
                               m_fileSelectedHandler( m_checklist->GetItemPath( m_checklist->GetSelection() ) );
                       } );
    Bind( wxEVT_SIZE, &GIT_COMMIT_PANE::onSize, this );
    Bind( wxEVT_IDLE, &GIT_COMMIT_PANE::onIdle, this );

    m_refreshRetryTimer.SetOwner( this );
    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                RefreshFileList();
            },
            m_refreshRetryTimer.GetId() );

    // Inside the advanced window the button would only raise the window it is in, and the
    // page is wide enough for the list and the message side by side.
    if( aParent != aFrame )
    {
        m_advancedButton->Hide();
        layoutWide();
    }

    // Sized by hand, or a long branch name would set the width of the whole row. Wide enough
    // for an ordinary name, with the tooltip carrying the rest when one is longer.
    m_branchChoice->SetMinSize( FromDIP( wxSize( 170, -1 ) ) );
    KIPLATFORM::UI::EllipsizeChoiceBox( m_branchChoice );

    // Measured while every control is still shown.
    m_minDockWidth = GetSizer()->CalcMin().x;

    RefreshFileList();
}


void GIT_COMMIT_PANE::layoutWide()
{
    wxSizer* column = GetSizer();
    column->Detach( m_changesRow );
    column->Detach( m_emptyLabel );
    column->Detach( m_checklist );
    column->GetItem( m_description )->SetProportion( 1 );
    column->SetMinSize( FromDIP( wxSize( 280, -1 ) ) );

    wxStaticText* heading = new wxStaticText( this, wxID_ANY, _( "Commit Message" ) );
    heading->SetFont( heading->GetFont().Bold() );
    column->Insert( 0, heading, 0, wxLEFT | wxRIGHT | wxTOP, 8 );

    // The summary takes the diff's colours for its line counts, lifted a little on a dark
    // background.
    wxStaticBoxSizer* summary = new wxStaticBoxSizer( wxVERTICAL, this, _( "Summary" ) );
    wxWindow*         box = summary->GetStaticBox();

    m_summaryFiles = new wxStaticText( box, wxID_ANY, wxEmptyString );
    m_summaryAdded = new wxStaticText( box, wxID_ANY, wxEmptyString );
    m_summaryRemoved = new wxStaticText( box, wxID_ANY, wxEmptyString );

    KICAD_DIFF::DIFF_COLOR_THEME theme;
    bool                         dark = KIPLATFORM::UI::IsDarkTheme();
    m_summaryAdded->SetForegroundColour( ( dark ? theme.added.Brightened( 0.4 ) : theme.added ).ToColour() );
    m_summaryRemoved->SetForegroundColour( ( dark ? theme.removed.Brightened( 0.4 ) : theme.removed ).ToColour() );

    wxBoxSizer* lines = new wxBoxSizer( wxHORIZONTAL );
    lines->Add( m_summaryAdded, 0, wxRIGHT, 8 );
    lines->Add( m_summaryRemoved, 0 );
    summary->Add( m_summaryFiles, 0, wxLEFT | wxRIGHT | wxTOP, 4 );
    summary->Add( lines, 0, wxALL, 4 );

    // Under the commit options box.
    size_t afterOptions = column->GetItemCount();

    for( size_t ii = 0; ii < column->GetItemCount(); ++ii )
    {
        if( column->GetItem( ii )->GetSizer() == m_optionsSizer )
            afterOptions = ii + 1;
    }

    column->Insert( afterOptions, summary, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8 );

    wxBoxSizer* files = new wxBoxSizer( wxVERTICAL );
    files->Add( m_changesRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8 );
    files->Add( m_emptyLabel, 0, wxALL, 8 );
    files->Add( m_checklist, 1, wxALL | wxEXPAND, 4 );

    wxBoxSizer* row = new wxBoxSizer( wxHORIZONTAL );
    row->Add( files, 1, wxEXPAND );
    row->Add( column, 0, wxEXPAND );

    // The old sizer lives on as the right column.
    SetSizer( row, false );
    Layout();
}


void GIT_COMMIT_PANE::SetFileSelectedHandler( std::function<void( const wxString& )> aHandler )
{
    m_fileSelectedHandler = std::move( aHandler );
}


GIT_COMMIT_PANE::~GIT_COMMIT_PANE()
{
    // The window's Pull and Push run through this pane, so it goes when the pane goes.
    GIT_ADVANCED_DIALOG::CloseForPane( this );

    // A transfer still running holds this pane, so stop it and wait. The wait ends at the
    // transfer's next callback, or at the network timeouts while it is still connecting.
    bool transferInFlight = m_networkBusy;

    if( m_networkCancel )
        m_networkCancel->store( true );

    if( m_networkWorker.joinable() )
        m_networkWorker.join();

    if( std::shared_ptr<KIGIT_COMMON> common = m_listenerSession.lock() )
    {
        common->RemoveRefreshListener( this );

        // The completion queued on this pane goes with it, so the other windows are told here.
        if( transferInFlight )
            common->NotifyRefreshListeners();
    }
}


void GIT_COMMIT_PANE::syncListener()
{
    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    if( m_listenerSession.lock() == common )
        return;

    // Loading a project replaces the git session, so move the listener to the new one.
    if( std::shared_ptr<KIGIT_COMMON> previous = m_listenerSession.lock() )
        previous->RemoveRefreshListener( this );

    // The window still lists the previous project, so it closes with the project that filled it.
    // A first attach replaced nothing, and must not take the window from whoever opened it.
    if( m_listenerAttached )
        GIT_ADVANCED_DIALOG::CloseActive();

    m_listenerAttached = true;

    // The remembered paths describe the previous project's file list, so a path that repeats in
    // the new project would inherit the old project's tick state.
    m_shownPaths.clear();

    common->AddRefreshListener( this,
                                [this]()
                                {
                                    RefreshFileList();
                                } );
    m_listenerSession = common;
}


void GIT_COMMIT_PANE::RefreshFileList()
{
    syncListener();

    // A session attached before the preference was turned off is still bound.
    if( !Pgm().GetCommonSettings()->m_Git.enableGit )
    {
        showDisabledState();
        return;
    }

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    // Retry shortly rather than wait, since a background fetch can hold the lock for a whole
    // network round trip.
    std::unique_lock<std::mutex> lock = common->TryLockAction();

    if( !lock.owns_lock() )
    {
        m_refreshRetryTimer.Start( 1000, wxTIMER_ONE_SHOT );
        return;
    }

    std::vector<FILE_CHECKLIST_ENTRY> entries = BuildGitChangeEntries( m_frame->Prj() );
    bool                              hasLocalCommits = common->GetRepo() && common->HasLocalCommits();
    m_hasRemote = common->GetRepo() && common->HasPushAndPullRemote();
    m_headDetached = common->GetRepo() && common->IsHeadDetached();
    m_operationInProgress = common->GetRepo() && common->IsOperationInProgress();
    m_operation = common->GetRepo() ? common->OperationInProgress() : GitOperation::None;
    m_remoteAhead = common->GetRepo() && common->HasUpstreamCommits();

    // Without a remote there is nowhere to push to, so local commits are not waiting for it.
    m_hasUnpushedCommits = hasLocalCommits && m_hasRemote;

    m_lineStats = { 0, 0 };

    if( common->GetRepo() && m_summaryFiles )
    {
        std::set<wxString> changed;

        for( const FILE_CHECKLIST_ENTRY& entry : entries )
            changed.insert( entry.path );

        m_lineStats = common->GetWorkingTreeLineStats( changed );
    }

    populateBranches( *common );

    lock.unlock();

    // Edits still only in an editor's memory are not in the git status yet, but the commit flow
    // saves the editors first, so they are committable and the controls must allow it.
    m_editorsDirty = common->GetRepo() && anyEditorDirty();

    // Amending is offered only while the last commit has not been pushed, so an already
    // published commit can never be rewritten from here.
    m_canAmend = hasLocalCommits && !m_headDetached && !m_operationInProgress;

    if( !m_canAmend )
        m_amendCheckbox->SetValue( false );

    // A file the user unticked must stay unticked across a refresh, or returning to the project
    // manager would re-include it. Newly appeared files stay ticked.
    std::set<wxString> stillChecked;

    for( const wxString& path : m_checklist->GetCheckedPaths() )
        stillChecked.insert( path );

    for( FILE_CHECKLIST_ENTRY& entry : entries )
    {
        if( m_shownPaths.count( entry.path ) && !stillChecked.count( entry.path ) )
            entry.checked = false;
    }

    m_shownPaths.clear();

    for( const FILE_CHECKLIST_ENTRY& entry : entries )
        m_shownPaths.insert( entry.path );

    m_checklist->SetEntries( entries );
    updateSummary();

    bool hasChanges = !entries.empty();

    m_checklist->Show( hasChanges );
    m_changesRow->ShowItems( hasChanges );

    // The message field also has to be there for a message-only amend and for committing
    // unsaved editor changes, when the list itself is empty.
    updateMessageVisibility( hasChanges );

    if( m_operationInProgress )
    {
        setStatusText( wxString::Format( _( "This project is in the middle of a %s that was started outside "
                                            "KiCad or interrupted. Abandon it to go back to the last commit, "
                                            "or finish it with the program that started it." ),
                                         operationNoun() ) );
        m_emptyLabel->Show( true );
    }
    else if( m_headDetached )
    {
        setStatusText( _( "This project is showing an old snapshot rather than a branch, so it cannot be "
                          "committed or pushed from here. Open Advanced, choose Branches and double click a "
                          "branch to go back to it." ) );
        m_emptyLabel->Show( true );
    }
    else if( hasChanges )
    {
        m_emptyLabel->Show( false );
    }
    else
    {
        if( !Pgm().GetCommonSettings()->m_Git.enableGit )
            setStatusText( _( "Version control is turned off in Preferences." ) );
        else if( !common->GetRepo() && !common->GetAttachError().IsEmpty() )
            setStatusText( common->GetAttachError() );
        else if( !common->GetRepo() )
            setStatusText( _( "This project is not under version control. To set it up, right click "
                              "the project in the project manager and choose Version Control." ) );
        else if( m_editorsDirty )
            setStatusText( _( "The open editors have unsaved changes. Committing saves them first." ) );
        else if( m_remoteAhead )
            setStatusText( _( "The remote has changes this project does not. Get Changes brings them in." ) );
        else if( m_hasUnpushedCommits )
            setStatusText( _( "Committed changes are waiting to be pushed." ) );
        else
            setStatusText( _( "No changes" ) );

        m_emptyLabel->Show( true );
    }

    updateCommitButton();
    Layout();
    updateDescriptionHint();
}


void GIT_COMMIT_PANE::ReloadAdvancedWindow()
{
    if( GIT_ADVANCED_DIALOG* dialog = GIT_ADVANCED_DIALOG::Active() )
        dialog->ReloadPages();
}


void GIT_COMMIT_PANE::showDisabledState()
{
    m_hasUnpushedCommits = false;
    m_hasRemote = false;
    m_headDetached = false;
    m_operationInProgress = false;
    m_operation = GitOperation::None;
    m_remoteAhead = false;
    m_editorsDirty = false;

    m_shownPaths.clear();
    m_checklist->SetEntries( {} );
    m_checklist->Show( false );
    m_changesRow->ShowItems( false );
    m_branchChoice->Clear();
    m_branchRow->ShowItems( false );

    m_canAmend = false;
    m_amendCheckbox->SetValue( false );
    updateMessageVisibility( false );

    setStatusText( _( "Version control is turned off in Preferences." ) );
    m_emptyLabel->Show( true );

    updateCommitButton();
    Layout();
}


void GIT_COMMIT_PANE::setStatusText( const wxString& aText )
{
    m_statusText = aText;

    // Wrap to the panel width so a long line is not clipped when the pane is narrow.
    m_emptyLabel->SetLabel( m_statusText );
    m_emptyLabel->Wrap( std::max( 40, GetClientSize().GetWidth() - 16 ) );
}


void GIT_COMMIT_PANE::onIdle( wxIdleEvent& aEvent )
{
    aEvent.Skip();

    // Every layout path moves the box, so the hint follows it from here.
    updateDescriptionHint();

    // Editing does not announce itself to the pane, so watch for the unsaved editor state
    // changing while the pane is visible. The refresh runs only on the transition.
    if( m_networkBusy || m_refreshRetryTimer.IsRunning() || !IsShownOnScreen() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    bool                          dirty = common->GetRepo() && anyEditorDirty();

    if( dirty != m_editorsDirty )
        RefreshFileList();
}


void GIT_COMMIT_PANE::onSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();

    if( m_emptyLabel && m_emptyLabel->IsShown() && !m_statusText.IsEmpty() )
        setStatusText( m_statusText );
}


void GIT_COMMIT_PANE::onOpenAdvanced( wxCommandEvent& aEvent )
{
    // The window belongs to the session, not to this panel, so all three frames raise the
    // same one.
    GIT_ADVANCED_DIALOG::ShowForFrame( m_frame, this );
}


bool GIT_COMMIT_PANE::anyEditorDirty() const
{
    for( FRAME_T type : { FRAME_SCH, FRAME_PCB_EDITOR } )
    {
        KIWAY_PLAYER* player = m_frame->Kiway().Player( type, false );

        if( player && player->IsContentModified() )
            return true;
    }

    return false;
}


void GIT_COMMIT_PANE::updateMessageVisibility( bool aHasChanges )
{
    // A commit is possible with listed changes, unsaved editor changes, or an amend. Both callers
    // share this, so the fields cannot disappear while the buttons still ask for a message.
    bool show = aHasChanges || m_editorsDirty || ( m_canAmend && m_amendCheckbox->IsChecked() );

    m_summary->Show( show );
    m_summaryCount->Show( show );
    m_description->Show( show );

    // The options box stays for a message only amend, with just the amend tick in it.
    m_optionsSizer->ShowItems( show || m_canAmend );
    m_amendCheckbox->Show( m_canAmend );
    m_signOffCheckbox->Show( show );
    m_newBranchCheckbox->Show( show );
    m_newBranchName->Show( show );
}


wxString GIT_COMMIT_PANE::operationNoun() const
{
    switch( m_operation )
    {
    case GitOperation::Merge:      return _( "merge" );
    case GitOperation::Rebase:     return _( "rebase" );
    case GitOperation::CherryPick: return _( "cherry-pick" );
    case GitOperation::Revert:     return _( "revert" );
    default: return _( "git operation" );
    }
}


wxString GIT_COMMIT_PANE::abandonLabel() const
{
    switch( m_operation )
    {
    case GitOperation::Merge:      return _( "Abandon Merge" );
    case GitOperation::Rebase:     return _( "Abandon Rebase" );
    case GitOperation::CherryPick: return _( "Abandon Cherry-pick" );
    case GitOperation::Revert:     return _( "Abandon Revert" );
    default: return _( "Abandon Operation" );
    }
}


void GIT_COMMIT_PANE::updateCommitButton()
{
    // While a transfer runs the busy state owns the controls. Checklist or message edits
    // must not re-enable the buttons under it.
    if( m_networkBusy )
        return;

    if( m_operationInProgress )
    {
        m_commitButton->Enable( false );
        m_commitOnlyButton->Enable( false );
        m_pullButton->SetLabel( abandonLabel() );
        m_pullButton->Enable( true );
        return;
    }

    if( m_headDetached )
    {
        m_commitButton->Enable( false );
        m_commitOnlyButton->Enable( false );
        m_pullButton->Enable( false );
        return;
    }

    m_pullButton->SetLabel( _( "Get Changes" ) );

    // The empty state label only shows when there is nothing to commit, so the button carries
    // the notice the rest of the time.
    if( m_remoteAhead )
        m_pullButton->SetToolTip( _( "The remote has commits this project does not have." ) );
    else
        m_pullButton->UnsetToolTip();

    bool hasCheckedChanges = !m_checklist->GetCheckedPaths().empty();
    bool amend = m_canAmend && m_amendCheckbox->IsChecked();
    bool newBranch = !amend && m_newBranchCheckbox->IsChecked();
    bool hasMessage = !m_summary->GetValue().Trim().IsEmpty();
    bool hasBranchName = !newBranch || !m_newBranchName->GetValue().Trim().IsEmpty();

    // An amend stays on the branch its commit is on.
    m_newBranchCheckbox->Enable( !amend );
    m_newBranchName->Enable( newBranch );

    if( amend )
    {
        // A message-only amend needs no checked files, just a message.
        m_commitButton->SetLabel( _( "Amend and Push" ) );
        m_commitButton->Enable( hasMessage );
        m_commitOnlyButton->SetLabel( _( "Amend" ) );
        m_commitOnlyButton->Enable( hasMessage );
    }
    else if( hasCheckedChanges || m_editorsDirty )
    {
        // Unsaved editor changes count because the commit flow saves them first, then commits
        // what the save produced.
        m_commitButton->SetLabel( _( "Commit and Push" ) );
        m_commitButton->Enable( hasMessage && hasBranchName );
        m_commitOnlyButton->SetLabel( _( "Commit" ) );
        m_commitOnlyButton->Enable( hasMessage && hasBranchName );
    }
    else if( m_hasUnpushedCommits )
    {
        m_commitButton->SetLabel( _( "Push" ) );
        m_commitButton->Enable( true );
        m_commitOnlyButton->SetLabel( _( "Commit" ) );
        m_commitOnlyButton->Enable( false );
    }
    else
    {
        m_commitButton->SetLabel( _( "Commit and Push" ) );
        m_commitButton->Enable( false );
        m_commitOnlyButton->SetLabel( _( "Commit" ) );
        m_commitOnlyButton->Enable( false );
    }

    // Pushing and pulling need a remote. Committing does not.
    m_pullButton->Enable( m_hasRemote );
}


void GIT_COMMIT_PANE::updateSummaryCount()
{
    // Git tools show the first line whole up to 72 columns.
    int left = 72 - static_cast<int>( m_summary->GetValue().length() );
    m_summaryCount->SetLabel( wxString::Format( wxS( "%d" ), left ) );
}


void GIT_COMMIT_PANE::onMessageChanged( wxCommandEvent& aEvent )
{
    updateSummaryCount();
    updateCommitButton();

    if( m_messageChangedHandler )
        m_messageChangedHandler();
}


wxString GIT_COMMIT_PANE::GetCommitMessage() const
{
    return KIGIT::ComposeCommitMessage( m_summary->GetValue(), m_description->GetValue(), wxEmptyString );
}


void GIT_COMMIT_PANE::SetCommitMessage( const wxString& aMessage )
{
    wxString summary;
    wxString description;

    KIGIT::SplitCommitMessage( aMessage, summary, description );

    // ChangeValue rather than SetValue, so filling one form does not echo back into the other.
    m_summary->ChangeValue( summary );
    setDescription( description );
    updateSummaryCount();
    updateCommitButton();
}


void GIT_COMMIT_PANE::onNewBranchToggled( wxCommandEvent& aEvent )
{
    updateCommitButton();

    if( m_newBranchCheckbox->IsChecked() )
        m_newBranchName->SetFocus();
}


void GIT_COMMIT_PANE::onBranchChosen( wxCommandEvent& aEvent )
{
    SwitchProjectToBranch( m_frame, this, m_branchChoice->GetStringSelection() );

    // A refused switch leaves the choice on a branch the project is not on.
    RefreshFileList();
}


void GIT_COMMIT_PANE::onShowDiff( wxCommandEvent& aEvent )
{
    wxString path = m_checklist->GetItemPath( m_checklist->GetSelection() );

    if( !path.IsEmpty() )
        ShowGitFileDiff( m_frame, path );
}


void GIT_COMMIT_PANE::populateBranches( KIGIT_COMMON& aCommon )
{
    m_branchChoice->Clear();
    m_branchRow->ShowItems( aCommon.GetRepo() != nullptr );

    if( !aCommon.GetRepo() )
        return;

    // Only a local branch can take a commit.
    for( const auto& [name, isRemote] : aCommon.GetAllBranches() )
    {
        if( !isRemote )
            m_branchChoice->Append( name );
    }

    wxString current = aCommon.GetCurrentBranchName();

    m_branchChoice->SetStringSelection( current );
    m_branchChoice->Enable( !m_networkBusy && !m_headDetached && !m_operationInProgress );

    // The box ellipsizes, so the whole name is only readable here.
    m_branchChoice->SetToolTip( current );
}


void GIT_COMMIT_PANE::updateDescriptionHint()
{
    if( !m_descriptionHint )
        return;

    // Over the box's top left corner, where typed text would start.
    bool    show = m_description->IsShown() && m_description->IsEmpty();
    wxPoint at = m_description->GetPosition() + wxPoint( FromDIP( 6 ), FromDIP( 4 ) );

    if( m_descriptionHint->IsShown() != show )
        m_descriptionHint->Show( show );

    if( m_descriptionHint->GetPosition() != at )
        m_descriptionHint->SetPosition( at );
}


void GIT_COMMIT_PANE::setDescription( const wxString& aText )
{
    m_description->ChangeValue( aText );
    updateDescriptionHint();
}


void GIT_COMMIT_PANE::onChecklistChanged( wxDataViewEvent& aEvent )
{
    updateCommitButton();
    updateSummary();
}


void GIT_COMMIT_PANE::updateSummary()
{
    // A ticked file is staged for the commit, an unticked one is left out.
    int staged = static_cast<int>( m_checklist->GetCheckedPaths().size() );
    int unstaged = static_cast<int>( m_shownPaths.size() ) - staged;

    m_changesLabel->SetLabel( wxString::Format( _( "Changes to Commit (%d of %d)" ), staged,
                                                static_cast<int>( m_shownPaths.size() ) ) );
    m_showDiffButton->Enable( m_checklist->GetSelection().IsOk() );

    if( m_summaryFiles )
    {
        wxString stagedText = staged == 1 ? _( "1 file will be committed" )
                                          : wxString::Format( _( "%d files will be committed" ), staged );
        wxString unstagedText = unstaged == 1 ? _( "1 left out" )
                                              : wxString::Format( _( "%d left out" ), unstaged );

        m_summaryFiles->SetLabel( wxString::Format( wxS( "%s, %s" ), stagedText, unstagedText ) );
        m_summaryAdded->SetLabel( wxString::Format( wxS( "+%d" ), m_lineStats.first ) );
        m_summaryRemoved->SetLabel( wxString::Format( wxS( "-%d" ), m_lineStats.second ) );
    }

    Layout();
}


void GIT_COMMIT_PANE::onChecklistContextMenu( wxDataViewEvent& aEvent )
{
    if( m_networkBusy )
        return;

    wxString path = m_checklist->GetItemPath( aEvent.GetItem() );

    if( path.IsEmpty() )
        return;

    wxMenu menu;
    menu.Append( 1, _( "Discard Changes" ) );

    if( GetPopupMenuSelectionFromUser( menu ) != 1 )
        return;

    discardFile( m_frame->Prj().GetGitCommon(), path );
}


void GIT_COMMIT_PANE::discardFile( std::shared_ptr<KIGIT_COMMON> aCommon, const wxString& aPath )
{
    std::unique_lock<std::mutex> lock = aCommon->TryLockAction();

    if( !lock.owns_lock() )
    {
        setBusy( false, _( "Another git operation is in progress. Try again in a moment." ) );
        return;
    }

    if( !aCommon->GetRepo() )
        return;

    bool     tracked = aCommon->IsPathInHead( aPath );
    bool     conflicted = aCommon->IsPathConflicted( aPath );
    wxString rootDir = aCommon->GetGitRootDirectory();

    // The confirmation is modal, so it must not run while the action lock is held.
    lock.unlock();

    // An unsaved editor's next save would undo the discard, so settle that first.
    if( DocumentEditorsDirty( m_frame->Kiway() ) || !LibraryEditorsWithUnsavedChanges( m_frame->Kiway() ).empty() )
    {
        DisplayInfoMessage( this, _( "An editor has unsaved changes. Save or revert them before discarding "
                                     "files, so the discard is not undone by the next save." ) );
        return;
    }

    wxString name = wxFileName( aPath ).GetFullName();
    wxString question;

    if( conflicted )
    {
        // Putting back what HEAD holds also clears the conflict, so the file is settled rather
        // than only reverted.
        question = wxString::Format( _( "Resolve the conflict in '%s' by going back to the version this "
                                        "project was on?\n\n"
                                        "The version that arrived is dropped, and the file no longer needs "
                                        "resolving." ),
                                     name );
    }
    else if( tracked )
    {
        question = wxString::Format( _( "Throw away the changes to '%s'?\n\n"
                                        "The file goes back to the last committed version." ),
                                     name );
    }
    else
    {
        question = wxString::Format( _( "Move '%s' to the trash?\n\n"
                                        "It has never been committed, so there is no version to bring back." ),
                                     name );
    }

    wxMessageDialog dlg( this, question, _( "Discard Changes" ), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING );
    dlg.SetYesNoLabels( conflicted  ? _( "Resolve" )
                        : tracked ? _( "Discard" )
                                  : _( "Move to Trash" ),
                        _( "Cancel" ) );

    if( dlg.ShowModal() != wxID_YES )
        return;

    // Snapshot first, so an editor showing the file reloads the committed version afterwards.
    EDITOR_SNAPSHOT docsBefore = CaptureOpenEditorDocs( m_frame->Kiway() );

    // Dropping the staged entry before touching the file means a contended lock or an index
    // failure leaves everything unchanged.
    lock = aCommon->TryLockAction();

    if( !lock.owns_lock() )
    {
        setBusy( false, _( "Another git operation is in progress. Try again in a moment." ) );
        return;
    }

    // The question above ran without the lock, so the project may have closed meanwhile.
    if( !aCommon->GetRepo() )
        return;

    wxString error;

    if( !aCommon->DiscardChanges( { aPath }, error ) )
    {
        lock.unlock();
        DisplayErrorMessage( this, wxString::Format( _( "Could not discard the changes to '%s'." ), name ),
                             error );
        return;
    }

    lock.unlock();

    if( !tracked )
    {
        if( !rootDir.EndsWith( wxS( "/" ) ) )
            rootDir += wxS( "/" );

        wxFileName fullPath( rootDir + aPath );

        // A previous discard can have removed the file already, so a missing file is done,
        // not an error.
        if( fullPath.FileExists() && !KIPLATFORM::ENV::MoveToTrash( fullPath.GetFullPath(), error ) )
        {
            // The staged entry is already gone, so the panels still refresh.
            aCommon->NotifyRefreshListeners();
            DisplayErrorMessage( this,
                                 wxString::Format( _( "Could not move '%s' to the trash. The file is "
                                                      "still on disk, so discard it again to retry." ),
                                                   name ),
                                 error );
            return;
        }
    }

    ReloadOpenEditors( m_frame->Kiway(), docsBefore );
    aCommon->NotifyRefreshListeners();
}


void GIT_COMMIT_PANE::onFileActivated( wxDataViewEvent& aEvent )
{
    wxString path = m_checklist->GetItemPath( aEvent.GetItem() );

    if( !path.IsEmpty() )
        ShowGitFileDiff( m_frame, path );
}


void GIT_COMMIT_PANE::onCommit( wxCommandEvent& aEvent )
{
    doCommit( true );
}


void GIT_COMMIT_PANE::onCommitOnly( wxCommandEvent& aEvent )
{
    doCommit( false );
}


void GIT_COMMIT_PANE::onPull( wxCommandEvent& aEvent )
{
    syncListener();

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    if( !common->GetRepo() )
        return;

    // While a transfer runs this button reads Cancel. The transfer callbacks poll the flag and
    // abort on their next tick, and the completion then reports the cancellation.
    if( m_networkBusy )
    {
        if( m_networkCancel )
            m_networkCancel->store( true );

        m_frame->SetStatusText( _( "Cancelling..." ) );
        return;
    }

    if( m_operationInProgress )
    {
        abandonOperation( common );
        return;
    }

    Pull();
}


bool GIT_COMMIT_PANE::transferCanStart()
{
    // The advanced window reaches these too, with this panel's own labels out of sight, so
    // the refusal is a message box.
    if( m_networkBusy )
    {
        DisplayInfoMessage( this, _( "A transfer is already running." ) );
        return false;
    }

    if( m_operationInProgress )
    {
        DisplayInfoMessage( this, wxString::Format( _( "Finish or abandon the %s in progress first." ),
                                                    operationNoun() ) );
        return false;
    }

    // The panel disables its own buttons for this, but the advanced window's reach here too.
    if( m_headDetached )
    {
        DisplayInfoMessage( this, _( "This project is showing an old snapshot rather than a branch, so there "
                                     "is nothing to send or receive. In the Advanced Git window, choose "
                                     "Branches and double click a branch to go back to it." ) );
        return false;
    }

    return true;
}


void GIT_COMMIT_PANE::Pull()
{
    syncListener();

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    if( !common->GetRepo() || !transferCanStart() )
        return;

    if( !m_hasRemote )
    {
        DisplayInfoMessage( this, _( "No remote is configured to get changes from." ) );
        return;
    }

    {
        std::unique_lock<std::mutex> probe = common->TryLockAction();

        if( !probe.owns_lock() )
        {
            setBusy( false, _( "Another git operation is in progress." ) );
            return;
        }
    }

    // A pull that is not a fast forward creates a merge commit, which needs an author
    // identity. Asking now beats failing after the merge has already touched the files.
    if( !KIGIT::EnsureGitIdentity( this, common.get() ) )
        return;

    submitPull( common, false );
}


void GIT_COMMIT_PANE::Push()
{
    syncListener();

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    if( !common->GetRepo() || !transferCanStart() )
        return;

    if( !m_hasRemote )
    {
        DisplayInfoMessage( this, _( "No remote is configured to push to." ) );
        return;
    }

    if( !m_hasUnpushedCommits )
    {
        DisplayInfoMessage( this, _( "There are no commits to push." ) );
        return;
    }

    {
        std::unique_lock<std::mutex> probe = common->TryLockAction();

        if( !probe.owns_lock() )
        {
            setBusy( false, _( "Another git operation is in progress." ) );
            return;
        }
    }

    submitPush( common );
}


void GIT_COMMIT_PANE::abandonOperation( std::shared_ptr<KIGIT_COMMON> aCommon )
{
    // Saving would only turn unsaved edits into changes that are then thrown away, so ask for
    // a decision in the editors first rather than offer a save.
    if( DocumentEditorsDirty( m_frame->Kiway() ) || !LibraryEditorsWithUnsavedChanges( m_frame->Kiway() ).empty() )
    {
        DisplayInfoMessage( this, _( "The open editors have unsaved changes. Save or revert them first, so it "
                                     "is clear what is being abandoned." ) );
        return;
    }

    std::vector<wxString> outside;

    {
        std::unique_lock<std::mutex> probe = aCommon->TryLockAction();

        if( !probe.owns_lock() )
        {
            setBusy( false, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        // Going back to the last commit covers the whole repository, so work in a sibling
        // project goes too and has to be named before the user agrees.
        outside = ChangedTrackedFilesOutsideProject( m_frame->Prj() );
    }

    wxString question = wxString::Format( _( "Abandon the %s and go back to the last commit?\n\n"
                                             "Changes made to the files since that commit are discarded." ),
                                          operationNoun() );

    if( !outside.empty() )
    {
        wxString listed;

        for( const wxString& path : outside )
            listed += wxS( "\n    " ) + path;

        question += wxString::Format( _( "\n\nThis also discards uncommitted changes elsewhere in this "
                                         "repository:%s" ),
                                      listed );
    }

    if( !IsOK( this, question ) )
        return;

    EDITOR_SNAPSHOT docsBefore = CaptureOpenEditorDocs( m_frame->Kiway() );
    wxString        error;
    bool            ok = false;

    {
        std::unique_lock<std::mutex> lock = aCommon->TryLockAction();

        if( !lock.owns_lock() )
        {
            setBusy( false, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        ok = aCommon->AbandonOperation( error );
    }

    if( !ok )
    {
        DisplayErrorMessage( this, wxString::Format( _( "Could not abandon the %s." ), operationNoun() ), error );
        return;
    }

    ReloadOpenEditors( m_frame->Kiway(), docsBefore );
    aCommon->NotifyRefreshListeners();
    setBusy( false, _( "Back at the last commit." ) );
}


void GIT_COMMIT_PANE::onAmendToggled( wxCommandEvent& aEvent )
{
    // Ticking the box offers the last commit's message as the starting point, since an amend
    // usually keeps or touches up the existing text.
    if( m_amendCheckbox->IsChecked() && m_summary->GetValue().Trim().IsEmpty()
        && m_description->GetValue().Trim().IsEmpty() )
    {
        std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
        std::unique_lock<std::mutex>  lock = common->TryLockAction();

        if( lock.owns_lock() && common->GetRepo() )
        {
            wxString last = common->GetLastCommitMessage();

            if( !last.IsEmpty() )
            {
                wxString summary, description;
                KIGIT::SplitCommitMessage( last, summary, description );
                m_summary->SetValue( summary );
                setDescription( description );
            }
        }
    }

    updateMessageVisibility( m_checklist->IsShown() );
    Layout();

    updateCommitButton();
}


void GIT_COMMIT_PANE::doCommit( bool aAndPush )
{
    syncListener();

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    if( !common->GetRepo() )
        return;

    const bool amend = m_canAmend && m_amendCheckbox->IsChecked();
    const bool newBranch = !amend && m_newBranchCheckbox->IsChecked();
    wxString   branchName = m_newBranchName->GetValue();
    branchName.Trim( true ).Trim( false );

    std::vector<wxString> files = m_checklist->GetCheckedPaths();

    // Unsaved editor changes still count, because the save below produces files the rescan
    // picks up. With local commits waiting the button reads "Push".
    if( files.empty() && !amend && !anyEditorDirty() )
    {
        if( !aAndPush || !m_hasUnpushedCommits )
            return;

        {
            std::unique_lock<std::mutex> probe = common->TryLockAction();

            if( !probe.owns_lock() )
            {
                setBusy( false, _( "Another git operation is in progress." ) );
                return;
            }
        }

        submitPush( common );
        return;
    }

    // Snapshot before saving, so the rescan can tell files the save dirtied from files that changed
    // for another reason. Keying it on the last refresh would sweep in files the user never saw.
    bool               preSaveOk = false;
    std::set<wxString> preSavePaths;

    {
        std::unique_lock<std::mutex> preLock = common->TryLockAction();

        if( preLock.owns_lock() )
        {
            preSaveOk = true;

            for( const FILE_CHECKLIST_ENTRY& entry : BuildGitChangeEntries( m_frame->Prj() ) )
                preSavePaths.insert( entry.path );
        }
    }

    if( !PrepareEditorsForGitOperation( m_frame, GIT_EDITOR_PREP::COMMIT ) )
        return;

    // A save can dirty design files that were not in the list, for example a board edited only in
    // memory, and leaving those out would commit less than the button promises.
    if( preSaveOk )
    {
        auto isProjectDocument = []( const wxString& aPath )
        {
            wxString ext = wxFileName( aPath ).GetExt();

            return ext.CmpNoCase( FILEEXT::KiCadSchematicFileExtension ) == 0
                   || ext.CmpNoCase( FILEEXT::KiCadPcbFileExtension ) == 0
                   || ext.CmpNoCase( FILEEXT::ProjectFileExtension ) == 0
                   || ext.CmpNoCase( FILEEXT::ProjectLocalSettingsFileExtension ) == 0;
        };

        std::unique_lock<std::mutex> rescanLock = common->TryLockAction();

        if( rescanLock.owns_lock() )
        {
            std::set<wxString> already( files.begin(), files.end() );

            for( const FILE_CHECKLIST_ENTRY& entry : BuildGitChangeEntries( m_frame->Prj() ) )
            {
                if( !preSavePaths.count( entry.path ) && !already.count( entry.path )
                    && isProjectDocument( entry.path ) )
                {
                    files.push_back( entry.path );
                    already.insert( entry.path );
                }
            }
        }
    }

    // Entered on the strength of unsaved editor changes alone, but the user declined the save,
    // so nothing reached the working tree and there is nothing to commit.
    if( files.empty() && !amend )
    {
        setBusy( false, _( "Nothing was saved, so there is nothing to commit." ) );
        return;
    }

    // A fresh machine has no git identity yet, and the commit cannot be recorded without one.
    if( !KIGIT::EnsureGitIdentity( this, common.get() ) )
        return;

    CommitResult result = CommitResult::Error;
    wxString     error;
    wxString     branchError;
    wxString     branchDetail;
    wxString     previousBranch;

    {
        // Committing writes to the repository, so hold the shared lock across it. If another
        // panel is pushing or pulling, the lock is busy: tell the user instead of racing.
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            setBusy( false, _( "Another git operation is in progress." ) );
            return;
        }

        GIT_CONFIG_HANDLER configHandler( common.get() );
        GitUserConfig      userConfig = configHandler.GetUserConfig();
        wxString           signOff;

        if( m_signOffCheckbox->IsChecked() )
            signOff = wxString::Format( wxS( "%s <%s>" ), userConfig.authorName, userConfig.authorEmail );

        wxString message = KIGIT::ComposeCommitMessage( m_summary->GetValue(), m_description->GetValue(), signOff );

        // The new branch starts at HEAD, so switching to it leaves the working tree as it is.
        if( newBranch )
            previousBranch = common->GetCurrentBranchName();

        if( !newBranch || moveToNewBranch( *common, branchName, branchError, branchDetail ) )
        {
            GIT_COMMIT_HANDLER commitHandler( common->GetRepo() );

            if( amend )
                result = commitHandler.PerformAmend( files, message, userConfig.authorName, userConfig.authorEmail );
            else
                result = commitHandler.PerformCommit( files, message, userConfig.authorName, userConfig.authorEmail );

            error = commitHandler.GetErrorString();

            bool committed = result == CommitResult::Success || result == CommitResult::SuccessIndexStale;

            // A failed commit must not leave the project on an empty new branch.
            if( newBranch && !committed )
                dropNewBranch( *common, branchName, previousBranch );
        }
    }

    if( !branchError.IsEmpty() )
    {
        DisplayErrorMessage( this, branchError, branchDetail );
        return;
    }

    // The commit exists on this path, so saying it failed would invite a second one.
    if( result == CommitResult::SuccessIndexStale )
    {
        DisplayInfoMessage( this, error );
    }
    else if( result != CommitResult::Success )
    {
        DisplayErrorMessage( this, wxString::Format( _( "Could not create the commit: %s" ), error ) );
        return;
    }

    m_summary->Clear();
    setDescription( wxEmptyString );
    m_amendCheckbox->SetValue( false );
    m_newBranchCheckbox->SetValue( false );
    m_newBranchName->Clear();
    common->NotifyRefreshListeners();

    if( aAndPush )
        submitPush( common );
    else
        setBusy( false, _( "Committed." ) );
}


void GIT_COMMIT_PANE::setBusy( bool aBusy, const wxString& aStatus )
{
    m_networkBusy = aBusy;

    m_commitButton->Enable( !aBusy );
    m_commitOnlyButton->Enable( !aBusy );
    m_amendCheckbox->Enable( !aBusy );
    m_signOffCheckbox->Enable( !aBusy );
    m_newBranchCheckbox->Enable( !aBusy );
    m_newBranchName->Enable( !aBusy && m_newBranchCheckbox->IsChecked() );
    m_branchChoice->Enable( !aBusy && !m_headDetached && !m_operationInProgress );
    m_summary->Enable( !aBusy );
    m_description->Enable( !aBusy );
    m_frame->SetStatusText( aStatus );

    // The frame status bar is overwritten within seconds by the editors, so an outcome shown
    // with the empty label also goes to the panel's own label, where it stays.
    if( !aBusy && !aStatus.IsEmpty() && m_emptyLabel->IsShown() && !m_headDetached && !m_operationInProgress )
        setStatusText( aStatus );

    // While a transfer runs, the network button turns into its cancel control.
    if( aBusy )
    {
        m_pullButton->SetLabel( _( "Cancel" ) );
        m_pullButton->Enable( true );
    }
    else
    {
        m_pullButton->SetLabel( _( "Get Changes" ) );
        updateCommitButton();
    }
}


void GIT_COMMIT_PANE::submitPush( std::shared_ptr<KIGIT_COMMON> aCommon )
{
    // Version control can be removed while a transfer runs, and leaving the busy state behind
    // would disable the panel for good.
    if( !aCommon->GetRepo() )
    {
        setBusy( false, wxEmptyString );
        return;
    }

    // Another panel's push or pull worker shares this repository, so reading the remote
    // configuration takes the action lock too.
    {
        std::unique_lock<std::mutex> lock = aCommon->TryLockAction();

        if( !lock.owns_lock() )
        {
            setBusy( false, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        // A project with no remote is committed locally only. Report that instead of failing a push.
        if( !aCommon->HasPushAndPullRemote() )
        {
            lock.unlock();
            setBusy( false, _( "Committed. No remote is configured to push to." ) );
            aCommon->NotifyRefreshListeners();
            return;
        }
    }

    setBusy( true, _( "Pushing..." ) );

    // Made here rather than in the worker, so Cancel pressed while the task is still queued
    // reaches this operation instead of being discarded when it starts.
    KIGIT_COMMON::CANCEL_TOKEN token = KIGIT_COMMON::NewCancelToken();
    m_networkCancel = token;

    // Never the shared compute pool. A libgit2 network read parked there stalls DRC,
    // zone filling and library loading.
    if( m_networkWorker.joinable() )
        m_networkWorker.join();

    m_networkWorker = std::thread(
            [this, aCommon, token]()
            {
                aCommon->ClearAuthFailure();

                GIT_PUSH_HANDLER handler( aCommon.get() );
                handler.SetCancelToken( token );
                PushResult       result = handler.PerformPush();
                wxString         error = handler.GetErrorString();
                bool             authFailure = aCommon->WasAuthFailure();

                // This operation's own token, so a cancel aimed at something else is not
                // reported here and this one's is not left behind for the next operation.
                bool cancelled = handler.IsThisOperationCancelled();

                CallAfter(
                        [this, aCommon, result, error, authFailure, cancelled]()
                        {
                            if( result == PushResult::Success )
                                aCommon->NotifyRefreshListeners();

                            onPushDone( aCommon, static_cast<int>( result ), error, authFailure, cancelled );
                        } );
            } );
}


void GIT_COMMIT_PANE::onPushDone( std::shared_ptr<KIGIT_COMMON> aCommon, int aResult, const wxString& aError,
                                  bool aAuthFailure, bool aCancelled )
{
    PushResult result = static_cast<PushResult>( aResult );

    if( result == PushResult::Success )
    {
        // The shared refresh already ran in the completion.
        setBusy( false, _( "Pushed to remote." ) );
        return;
    }

    // Lock contention with another panel's operation is not a push failure, so do not report
    // it as one.
    if( result == PushResult::Busy )
    {
        setBusy( false, _( "Another git operation is in progress. Try again in a moment." ) );
        return;
    }

    if( aCancelled )
    {
        setBusy( false, _( "Cancelled." ) );
        return;
    }

    if( aAuthFailure )
    {
        if( KIGIT::PromptForCredentials( this, aCommon.get(), &m_frame->Prj() ) )
            submitPush( aCommon );
        else
            setBusy( false, _( "Cancelled." ) );

        return;
    }

    // The remote moved on since the last sync. Offer to bring those changes in and push
    // again, rather than force over them.
    if( result == PushResult::NonFastForward )
    {
        setBusy( false, wxEmptyString );

        // Getting their changes needs a clean tree. Files left unticked at the commit, or edits
        // still in an editor, block it, and offering the click anyway would only fail later.
        bool dirty = anyEditorDirty();

        if( !dirty )
        {
            std::unique_lock<std::mutex> lock = aCommon->TryLockAction();

            if( !lock.owns_lock() )
            {
                setBusy( false, _( "Another git operation is in progress. Try again in a moment." ) );
                return;
            }

            dirty = aCommon->HasUncommittedTrackedChanges();
        }

        if( dirty )
        {
            DisplayInfoMessage( this, _( "Someone else pushed changes first. Before their changes can be "
                                         "brought in, commit or discard the changes that were left out of "
                                         "this commit. Then use Get Changes and push again." ) );
            return;
        }

        if( IsOK( this, _( "Someone else pushed changes first.\n\n"
                           "Get their changes and push again?" ) ) )
        {
            submitPull( aCommon, true );
        }

        return;
    }

    setBusy( false, _( "Push failed." ) );
    DisplayErrorMessage( this, wxString::Format( _( "Could not push: %s" ), aError ) );
}


void GIT_COMMIT_PANE::resolvePullConflicts( std::shared_ptr<KIGIT_COMMON> aCommon, bool aThenPush )
{
    setBusy( false, wxEmptyString );

    // The merge commit needs an identity whatever the user decides about the files, so it is
    // asked for before any merging. Cancelling it then costs no work.
    if( !KIGIT::EnsureGitIdentity( this, aCommon.get() ) )
    {
        setBusy( false, _( "Getting changes was cancelled. Your files are unchanged." ) );
        return;
    }

    GIT_DIFF_WORKSPACE workspace;

    if( !workspace.IsOk() )
    {
        DisplayErrorMessage( this, _( "Could not create a temporary directory for the merge." ) );
        return;
    }

    const wxString sep = wxFileName::GetPathSeparator();

    wxString                          headOid;
    wxString                          theirsOid;
    wxString                          error;
    std::vector<KIGIT_CONFLICT_SIDES> conflicts;

    bool prepared = false;
    bool prepareBusy = false;

    {
        std::unique_lock<std::mutex> lock = aCommon->TryLockAction();

        if( !lock.owns_lock() )
        {
            prepareBusy = true;
        }
        else
        {
            prepared = aCommon->GetMergeConflicts( workspace.Root() + sep + wxS( "stages" ), headOid, theirsOid,
                                                   conflicts, error );
        }
    }

    // A modal dialog runs its own event loop, so reporting the outcome happens after the lock
    // is released. Holding it here would stall every other git surface until the box is closed.
    if( prepareBusy )
    {
        setBusy( false, _( "Another git operation is in progress. Try again in a moment." ) );
        return;
    }

    if( !prepared )
    {
        DisplayErrorMessage( this, _( "Could not prepare the merge." ), error );
        return;
    }

    std::vector<std::pair<wxString, wxString>> resolved;

    // The version the user does not keep is still somebody's work, so it is saved beside the
    // project file. Written only once every conflict is answered, so cancelling changes nothing.
    struct LOSING_COPY
    {
        wxString source;
        wxString relativeDest;
    };

    std::vector<LOSING_COPY> losingCopies;

    // Files the user merged by hand. The scratch directory holding them goes when this returns.
    int handMerged = 0;

    for( size_t ii = 0; ii < conflicts.size(); )
    {
        const KIGIT_CONFLICT_SIDES& conflict = conflicts[ii];
        KICAD_DIFF::DOC_KIND        kind = KICAD_DIFF::DocKindFromExtension( conflict.path );

        // The merge tool needs all three sides. A deletion is kept or undone whole, and two files
        // created independently have no base to merge from, so they are kept or taken whole too.
        const bool mergeable = kind != KICAD_DIFF::DOC_KIND::UNKNOWN && conflict.hasAncestor && conflict.hasOurs
                               && conflict.hasTheirs;

        wxArrayString choices;

        if( mergeable )
            choices.Add( _( "Open the merge tool" ) );

        // A side with no base was added, not deleted. A renamed file arrives as two such entries.
        choices.Add( conflict.hasOurs       ? _( "Keep my version" )
                     : conflict.hasAncestor ? _( "Keep it deleted" )
                                            : _( "Leave it out" ) );
        choices.Add( conflict.hasTheirs     ? _( "Use the remote version" )
                     : conflict.hasAncestor ? _( "Delete it, as on the remote" )
                                            : _( "Leave it out, as on the remote" ) );

        wxString situation;

        if( !conflict.hasOurs && !conflict.hasAncestor )
        {
            situation = _( "'%s' exists only on the remote (%zu of %zu).\n\n"
                           "How should it be resolved?" );
        }
        else if( !conflict.hasOurs )
        {
            situation = _( "'%s' was deleted here but changed on the remote (%zu of %zu).\n\n"
                           "How should it be resolved?" );
        }
        else if( !conflict.hasTheirs && !conflict.hasAncestor )
        {
            situation = _( "'%s' exists only here (%zu of %zu).\n\n"
                           "How should it be resolved?" );
        }
        else if( !conflict.hasTheirs )
        {
            situation = _( "'%s' was changed here but deleted on the remote (%zu of %zu).\n\n"
                           "How should it be resolved?" );
        }
        else
        {
            situation = _( "'%s' was changed both here and on the remote (%zu of %zu).\n\n"
                           "How should it be resolved?" );
        }

        int sel = wxGetSingleChoiceIndex( wxString::Format( situation, conflict.path, ii + 1, conflicts.size() ),
                                          _( "Resolve Conflict" ), choices, this );

        if( sel < 0 )
        {
            wxString warning = handMerged == 1
                                       ? _( "Stop resolving? The file you merged by hand is discarded "
                                            "with the rest." )
                                       : wxString::Format( _( "Stop resolving? The %d files you merged by "
                                                              "hand are discarded with the rest." ),
                                                           handMerged );

            if( handMerged > 0 && !IsOK( this, warning ) )
                continue;

            // Nothing was touched yet, so cancelling really does leave everything as it was.
            setBusy( false, _( "Getting changes was cancelled. Your files are unchanged." ) );
            return;
        }

        int action = mergeable ? sel : sel + 1;

        if( action == 0 )
        {
            wxString mergedOut = workspace.Root() + sep + wxString::Format( wxS( "m%zu_" ), ii )
                                 + wxFileName( conflict.path ).GetFullName();

            WX_STRING_REPORTER reporter;
            int                rc = KICAD_DIFF::DispatchMerge(
                    m_frame->Kiway(), kind, conflict.ancestorFile, conflict.oursFile, conflict.theirsFile, mergedOut,
                    /* interactive */ true, kind == KICAD_DIFF::DOC_KIND::FOOTPRINT, &reporter );

            if( rc != 0 )
            {
                // Cancelled or failed. Offer the choices for this file again.
                if( reporter.HasMessage() )
                    DisplayErrorMessage( this, _( "The merge tool did not produce a result." ),
                                         reporter.GetMessages() );

                continue;
            }

            resolved.emplace_back( conflict.path, mergedOut );
            ++handMerged;

            // A merged result replaces both originals, so both are kept.
            if( conflict.hasOurs )
                losingCopies.push_back( { conflict.oursFile, conflict.path + wxS( ".mine" ) } );

            if( conflict.hasTheirs )
                losingCopies.push_back( { conflict.theirsFile, conflict.path + wxS( ".theirs" ) } );
        }
        else if( action == 1 )
        {
            // An empty path records the resolution "the file is deleted".
            resolved.emplace_back( conflict.path, conflict.hasOurs ? conflict.oursFile : wxString() );

            if( conflict.hasTheirs )
                losingCopies.push_back( { conflict.theirsFile, conflict.path + wxS( ".theirs" ) } );
        }
        else
        {
            resolved.emplace_back( conflict.path, conflict.hasTheirs ? conflict.theirsFile : wxString() );

            if( conflict.hasOurs )
                losingCopies.push_back( { conflict.oursFile, conflict.path + wxS( ".mine" ) } );
        }

        ++ii;
    }

    // Written before the merge is published, so a copy that cannot be made stops the merge
    // rather than losing the version it was meant to protect.
    wxString rootDir = aCommon->GetGitRootDirectory();

    if( !rootDir.EndsWith( wxS( "/" ) ) )
        rootDir += wxS( "/" );

    std::vector<wxString> savedCopies;
    std::vector<wxString> createdDirs;

    // Undoing the copies keeps every later abort honest when it says the files are unchanged.
    // Only what this attempt made is removed, so an older recovery file survives.
    auto discardSavedCopies = [&savedCopies, &createdDirs]()
    {
        for( const wxString& path : savedCopies )
            wxRemoveFile( path );

        // Deepest first, and only while empty, so a directory holding anything else stays.
        for( auto dir = createdDirs.rbegin(); dir != createdDirs.rend(); ++dir )
            wxFileName::Rmdir( *dir );

        savedCopies.clear();
        createdDirs.clear();
    };

    for( const LOSING_COPY& copy : losingCopies )
    {
        wxString saved;
        wxString createdDir;

        bool reserved = reserveRecoveryFile( rootDir + copy.relativeDest, saved, createdDir );

        if( !createdDir.IsEmpty() )
            createdDirs.push_back( createdDir );

        if( !reserved || !wxCopyFile( copy.source, saved, true ) )
        {
            if( !saved.IsEmpty() )
                wxRemoveFile( saved );

            discardSavedCopies();
            setBusy( false, _( "Getting changes was cancelled. Your files are unchanged." ) );
            DisplayErrorMessage( this, wxString::Format( _( "Could not save the version you are replacing "
                                                            "as '%s', so the merge was stopped." ),
                                                         copy.relativeDest ) );
            return;
        }

        savedCopies.push_back( saved );
    }

    bool ok = false;

    // The lock was released while the user resolved, and the background fetch may hold it now.
    // Throwing the resolutions away would mean doing every file again, so let the user retry.
    while( true )
    {
        std::unique_lock<std::mutex> lock = aCommon->TryLockAction();

        if( lock.owns_lock() )
        {
            ok = aCommon->CreateMergedCommit( headOid, theirsOid, resolved, error );
            break;
        }

        if( !IsOK( this, _( "Another git operation is in progress. Try again?" ) ) )
        {
            discardSavedCopies();
            setBusy( false, _( "Getting changes was cancelled. Your files are unchanged." ) );
            return;
        }
    }

    if( !ok )
    {
        // The merge wrote nothing, so the copies it would have explained are removed with it.
        discardSavedCopies();
        DisplayErrorMessage( this, _( "Could not finish the merge." ), error );
        return;
    }

    // The merge rewrote working files, so bring the editors and panels up to date.
    ReloadOpenEditors( m_frame->Kiway(), m_docsBeforePull );
    aCommon->NotifyRefreshListeners();

    if( aThenPush )
        submitPush( aCommon );
    else
        setBusy( false, _( "Changes merged." ) );
}


void GIT_COMMIT_PANE::submitPull( std::shared_ptr<KIGIT_COMMON> aCommon, bool aThenPush )
{
    // Version control can be removed while a transfer runs, and leaving the busy state behind
    // would disable the panel for good.
    if( !aCommon->GetRepo() )
    {
        setBusy( false, wxEmptyString );
        return;
    }

    // The merge and the conflict resolution both need a clean state, and committing first is the
    // one path that cannot lose the work. Saving here would only manufacture dirty files.
    std::vector<wxString> libraryEditors = LibraryEditorsWithUnsavedChanges( m_frame->Kiway() );

    if( !libraryEditors.empty() )
    {
        wxString listed;

        for( const wxString& name : libraryEditors )
            listed += wxString::Format( wxS( "\n    - %s" ), name );

        setBusy( false, _( "Save your library changes first." ) );
        DisplayInfoMessage( this, wxString::Format( _( "The following editors have unsaved changes, and getting "
                                                       "changes could rewrite the libraries they are editing:%s"
                                                       "\n\nSave or discard those changes first." ),
                                                    listed ) );
        return;
    }

    bool                  dirty = anyEditorDirty();
    std::vector<wxString> outside;

    if( !dirty )
    {
        std::unique_lock<std::mutex> lock = aCommon->TryLockAction();

        if( !lock.owns_lock() )
        {
            setBusy( false, _( "Another git operation is in progress. Try again in a moment." ) );
            return;
        }

        dirty = aCommon->HasUncommittedTrackedChanges();

        // The list shows this project only. A change elsewhere in the repository blocks the
        // merge just the same, and has to be named or the user is asked to commit nothing.
        if( dirty )
            outside = ChangedTrackedFilesOutsideProject( m_frame->Prj() );
    }

    if( dirty )
    {
        setBusy( false, _( "Commit your changes first." ) );

        if( !outside.empty() )
        {
            wxString listed;

            for( const wxString& path : outside )
                listed += wxS( "\n    " ) + path;

            DisplayInfoMessage( this, wxString::Format( _( "There are uncommitted changes elsewhere in this "
                                                           "repository:%s\n\nCommit or discard them from that "
                                                           "project first, then get changes again." ),
                                                        listed ) );
        }
        else
        {
            DisplayInfoMessage( this, _( "There are changes that are not committed yet, and getting changes "
                                         "would run over them. Commit them first, then get changes again. To "
                                         "set them aside instead, use Stash Changes in the Advanced Git "
                                         "window." ) );
        }

        return;
    }

    setBusy( true, _( "Getting changes..." ) );

    // Made here rather than in the worker, so Cancel pressed while the task is still queued
    // reaches this operation instead of being discarded when it starts.
    KIGIT_COMMON::CANCEL_TOKEN token = KIGIT_COMMON::NewCancelToken();
    m_networkCancel = token;

    // Record what the open documents look like on disk now, so only the ones the merge actually
    // rewrites are reloaded afterwards.
    m_docsBeforePull = CaptureOpenEditorDocs( m_frame->Kiway() );

    EDITOR_SNAPSHOT docsBefore = m_docsBeforePull;

    // Never the shared compute pool. A libgit2 network read parked there stalls DRC,
    // zone filling and library loading.
    if( m_networkWorker.joinable() )
        m_networkWorker.join();

    m_networkWorker = std::thread(
            [this, aCommon, aThenPush, docsBefore, token]()
            {
                aCommon->ClearAuthFailure();

                GIT_PULL_HANDLER handler( aCommon.get() );
                handler.SetCancelToken( token );
                PullResult       result = handler.PerformPull();
                wxString         error = handler.GetErrorStrings().empty() ? wxString() : handler.GetErrorString();
                bool             authFailure = aCommon->WasAuthFailure();

                // This operation's own token, so a cancel aimed at something else is not
                // reported here and this one's is not left behind for the next operation.
                bool cancelled = handler.IsThisOperationCancelled();

                CallAfter(
                        [this, aCommon, result, error, authFailure, aThenPush, docsBefore, cancelled]()
                        {
                            // A merge that failed partway can still have written files, so the
                            // editors are brought up to date whatever the outcome. Nothing
                            // reloads when nothing changed on disk.
                            ReloadOpenEditors( m_frame->Kiway(), docsBefore );

                            if( result == PullResult::Success || result == PullResult::FastForward )
                                aCommon->NotifyRefreshListeners();

                            onPullDone( aCommon, static_cast<int>( result ), error, authFailure, aThenPush, cancelled );
                        } );
            } );
}


void GIT_COMMIT_PANE::onPullDone( std::shared_ptr<KIGIT_COMMON> aCommon, int aResult, const wxString& aError,
                                  bool aAuthFailure, bool aThenPush, bool aCancelled )
{
    PullResult result = static_cast<PullResult>( aResult );

    if( result == PullResult::Success || result == PullResult::FastForward || result == PullResult::UpToDate )
    {
        // The editor reloads and the panel refresh already ran in the completion, before the
        // window check, so they happen even when the initiating window is gone.
        if( result == PullResult::UpToDate )
            aCommon->NotifyRefreshListeners();

        // Pushing afterwards belongs only to the push-was-rejected recovery. A plain Get
        // Changes must not publish local commits the user chose to keep local.
        if( aThenPush )
            submitPush( aCommon );
        else
            setBusy( false, result == PullResult::UpToDate ? _( "Already up to date." ) : _( "Changes received." ) );

        return;
    }

    // A rejected login is not a merge problem, so offer the credentials prompt and retry,
    // matching the push path.
    if( aCancelled )
    {
        setBusy( false, _( "Cancelled." ) );
        return;
    }

    if( aAuthFailure )
    {
        if( KIGIT::PromptForCredentials( this, aCommon.get(), &m_frame->Prj() ) )
            submitPull( aCommon, aThenPush );
        else
            setBusy( false, _( "Cancelled." ) );

        return;
    }

    // Conflicting changes are resolved file by file, with the visual merge for design files.
    // The failed merge already restored the pre-pull state, so cancelling costs nothing.
    if( result == PullResult::Conflict )
    {
        resolvePullConflicts( aCommon, aThenPush );
        return;
    }

    // Something uncommitted appeared after the pre-pull check, or an untracked file collides
    // with an incoming one. This is the panel's own job to fix, not the command line's.
    if( result == PullResult::DirtyWorkdir )
    {
        setBusy( false, _( "Commit your changes first." ) );
        aCommon->NotifyRefreshListeners();

        // The backend names the files when it can, which the generic text cannot.
        if( !aError.IsEmpty() )
        {
            DisplayInfoMessage( this, aError );
        }
        else
        {
            DisplayInfoMessage( this, _( "There are changes that are not committed yet, and getting changes "
                                         "would run over them. Commit them first, then get changes again. To "
                                         "set them aside instead, use Stash Changes in the Advanced Git "
                                         "window." ) );
        }

        return;
    }

    // The failure may or may not have touched the working tree, so refresh the panels and
    // report the backend's actual error rather than guessing at a cause.
    setBusy( false, _( "Could not get changes." ) );
    aCommon->NotifyRefreshListeners();
    DisplayErrorMessage( this, _( "Could not get changes." ), aError );
}
