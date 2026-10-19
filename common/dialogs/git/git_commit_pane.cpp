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
#include <kiway_player.h>
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

#include <bitmaps.h>
#include <confirm.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <dialogs/git/git_transfer.h>
#include <dialogs/git/git_ui_utils.h>
#include <diff_merge/merge_dispatch.h>
#include <kiplatform/ui.h>
#include <diff_merge/diff_scene.h>
#include <reporter.h>
#include <git/kicad_git_common.h>
#include <git/project_git_utils.h>
#include <git/git_branch_handler.h>
#include <git/git_backend.h>
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
#include <widgets/std_bitmap_button.h>


namespace
{
enum
{
    ID_OPEN_ADVANCED = wxID_HIGHEST + 1,
    ID_SET_AUTHOR
};
}


// Pin both the branch (including an unborn one) and its tip while a commit's status reads run.
// The caller holds the action lock, just as it does when validating this before the write.
static std::pair<wxString, wxString> commitHead( git_repository* aRepo )
{
    std::pair<wxString, wxString> state;
    git_reference*                head = nullptr;

    if( git_reference_lookup( &head, aRepo, "HEAD" ) == GIT_OK )
    {
        if( const char* target = git_reference_symbolic_target( head ) )
            state.first = wxString::FromUTF8( target );

        git_reference_free( head );
    }

    git_oid oid;

    if( git_reference_name_to_id( &oid, aRepo, "HEAD" ) == GIT_OK )
        state.second = wxString::FromUTF8( git_oid_tostr_s( &oid ) );

    return state;
}


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
        m_editorsDirty( false )
{
    m_summary->SetHint( _( "Summarize your changes..." ) );
    m_newBranchName->SetHint( _( "Branch name" ) );

    // Sized by hand, or the longest message would set the width of the whole row.
    m_recentMessages->SetMinSize( FromDIP( wxSize( 170, -1 ) ) );
    KIPLATFORM::UI::EllipsizeChoiceBox( m_recentMessages );

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

    m_settingsButton->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
    m_settingsButton->Bind( wxEVT_BUTTON, &GIT_COMMIT_PANE::onSettings, this );
    m_commitButton->Bind( wxEVT_BUTTON, &GIT_COMMIT_PANE::onCommit, this );

    // Commit from the keyboard, since the description box takes a plain Return as a new line.
    auto commitOnEnter = [this]( wxKeyEvent& aEvent )
    {
        bool enter = aEvent.GetKeyCode() == WXK_RETURN || aEvent.GetKeyCode() == WXK_NUMPAD_ENTER;

        if( enter && ( aEvent.CmdDown() || aEvent.ControlDown() ) && m_commitButton->IsEnabled() )
            doCommit( m_commitButtonPushes );
        else
            aEvent.Skip();
    };

    m_summary->Bind( wxEVT_CHAR_HOOK, commitOnEnter );
    m_description->Bind( wxEVT_CHAR_HOOK, commitOnEnter );
    m_commitAndPushItem = m_commitButton->GetSplitButtonMenu()->Append( wxID_ANY, _( "Commit and Push" ) );
    m_commitButton->GetSplitButtonMenu()->Bind( wxEVT_MENU, &GIT_COMMIT_PANE::onCommitAndPush, this,
                                                m_commitAndPushItem->GetId() );
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
    m_recentMessages->Bind( wxEVT_CHOICE, &GIT_COMMIT_PANE::onRecentMessage, this );
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

    // Inside the advanced window the page is wide enough for the list and the message side
    // by side.
    m_inAdvancedWindow = aParent != aFrame;

    if( m_inAdvancedWindow )
        layoutWide();

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
    m_refresh.Cancel();
    m_messageQuery.Cancel();
    m_actionQuery.Cancel();

    // The window's Pull and Push run through this pane, so it goes when the pane goes.
    GIT_ADVANCED_DIALOG::CloseForPane( this );

    // The transfer outlives this pane, and its completion refreshes the other windows, so
    // nothing has to be reported here. Asking it to stop is the destructor's own job.
    if( std::shared_ptr<KIGIT_COMMON> common = m_listenerSession.lock() )
        common->RemoveRefreshListener( this );
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

    m_refresh.Cancel();
    m_messageQuery.Cancel();
    m_actionQuery.Cancel();

    if( m_actionPending )
    {
        m_actionPending = false;
        setBusy( false, wxEmptyString );
    }
    m_hasRepo = false;

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


struct GIT_COMMIT_PANE::STATUS
{
    bool                                   hasRepo = false;
    bool                                   hasLocalCommits = false;
    bool                                   hasRemote = false;
    bool                                   headDetached = false;
    bool                                   operationInProgress = false;
    bool                                   remoteAhead = false;
    GitOperation                           operation = GitOperation::None;
    wxString                               attachError;
    wxString                               currentBranch;
    std::vector<FILE_CHECKLIST_ENTRY>      entries;
    std::vector<std::pair<wxString, bool>> branches;
    std::vector<wxString>                  recentMessages;
    std::pair<int, int>                    lineStats{ 0, 0 };
};


void GIT_COMMIT_PANE::RefreshFileList()
{
    syncListener();

    // A session attached before the preference was turned off is still bound.
    if( !Pgm().GetCommonSettings()->m_Git.enableGit )
    {
        m_refresh.Cancel();
        showDisabledState();
        return;
    }

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    wxString                      projectPath = m_frame->Prj().GetProjectPath();
    wxString                      projectName = m_frame->Prj().GetProjectName();
    bool                          dark = KIPLATFORM::UI::IsDarkTheme();
    bool                          countLines = m_summaryFiles != nullptr;
    auto                          status = std::make_shared<STATUS>();
    status->attachError = common->GetAttachError();

    m_refresh.Run(
            common,
            [status, projectPath, projectName, dark, countLines]( KIGIT_COMMON& aCommon )
            {
                status->hasRepo = true;
                status->entries = BuildGitChangeEntries( aCommon, projectPath, projectName, dark );
                status->hasLocalCommits = aCommon.HasLocalCommits();
                status->hasRemote = aCommon.HasPushAndPullRemote();
                status->headDetached = aCommon.IsHeadDetached();
                status->operationInProgress = aCommon.IsOperationInProgress();
                status->operation = aCommon.OperationInProgress();
                status->remoteAhead = aCommon.HasUpstreamCommits();
                status->branches = aCommon.GetAllBranches();
                status->currentBranch = aCommon.GetCurrentBranchName();
                status->recentMessages = aCommon.GetRecentCommitMessages( 10 );

                if( countLines )
                {
                    std::set<wxString> changed;

                    for( const FILE_CHECKLIST_ENTRY& entry : status->entries )
                        changed.insert( entry.path );

                    status->lineStats = aCommon.GetWorkingTreeLineStats( changed );
                }
            },
            [this, common, status]()
            {
                if( m_frame->Prj().GetGitCommon() == common && !common->IsShuttingDown()
                    && Pgm().GetCommonSettings()->m_Git.enableGit )
                {
                    applyStatus( *status );
                }
            } );
}


void GIT_COMMIT_PANE::applyStatus( const STATUS& aStatus )
{
    std::vector<FILE_CHECKLIST_ENTRY> entries = aStatus.entries;
    m_hasRepo = aStatus.hasRepo;
    m_hasRemote = aStatus.hasRemote;
    m_headDetached = aStatus.headDetached;
    m_operationInProgress = aStatus.operationInProgress;
    m_operation = aStatus.operation;
    m_remoteAhead = aStatus.remoteAhead;

    // Without a remote there is nowhere to push to, so local commits are not waiting for it.
    m_hasUnpushedCommits = aStatus.hasLocalCommits && m_hasRemote;
    m_lineStats = aStatus.lineStats;
    populateBranches( aStatus );
    populateRecentMessages( aStatus.recentMessages );

    // Edits still only in an editor's memory are not in the git status yet, but the commit flow
    // saves the editors first, so they are committable and the controls must allow it.
    m_editorsDirty = aStatus.hasRepo && anyEditorDirty();

    // Amending is offered only while the last commit has not been pushed, so an already
    // published commit can never be rewritten from here.
    m_canAmend = aStatus.hasLocalCommits && !m_headDetached && !m_operationInProgress;

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
                          "committed or pushed from here. In the Advanced Git window, choose Branches "
                          "and double click a branch to go back to it." ) );
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
        else if( !aStatus.hasRepo && !aStatus.attachError.IsEmpty() )
            setStatusText( aStatus.attachError );
        else if( !aStatus.hasRepo )
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
    m_messageQuery.Cancel();
    m_actionQuery.Cancel();

    if( m_actionPending )
    {
        m_actionPending = false;
        setBusy( false, wxEmptyString );
    }
    m_hasRepo = false;
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
    m_settingsButton->Show();

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
    if( !Pgm().GetCommonSettings()->m_Git.enableGit || m_transfer.IsBusy() || m_actionPending || m_refresh.IsBusy()
        || !IsShownOnScreen() )
        return;

    bool dirty = m_hasRepo && anyEditorDirty();

    if( dirty != m_editorsDirty )
        RefreshFileList();
}


void GIT_COMMIT_PANE::onSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();

    if( m_emptyLabel && m_emptyLabel->IsShown() && !m_statusText.IsEmpty() )
        setStatusText( m_statusText );
}


void GIT_COMMIT_PANE::onSettings( wxCommandEvent& aEvent )
{
    wxMenu menu;

    // Inside the advanced window the entry would only raise the window it is in.
    if( !m_inAdvancedWindow )
    {
        menu.Append( ID_OPEN_ADVANCED, _( "Open Advanced Git Window..." ) );
        menu.Bind( wxEVT_MENU,
                   [this]( wxCommandEvent& )
                   {
                       // The window belongs to the session, not to this panel, so all three
                       // frames raise the same one.
                       GIT_ADVANCED_DIALOG::ShowForFrame( m_frame, this );
                   },
                   ID_OPEN_ADVANCED );
    }

    menu.Append( ID_SET_AUTHOR, _( "Set Author..." ) );
    menu.Bind( wxEVT_MENU,
               [this]( wxCommandEvent& )
               {
                   setAuthor();
               },
               ID_SET_AUTHOR );

    PopupMenu( &menu );
}


void GIT_COMMIT_PANE::setAuthor()
{
    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    auto                          identity = std::make_shared<GitUserConfig>();
    identity->authorName = Pgm().GetCommonSettings()->m_Git.authorName;
    identity->authorEmail = Pgm().GetCommonSettings()->m_Git.authorEmail;

    runActionQuery(
            common,
            [identity]( KIGIT_COMMON& aSession )
            {
                GIT_CONFIG_HANDLER handler( &aSession );
                wxString           value;

                if( GetGitBackend()->GetConfigString( &handler, "user.name", value ) )
                    identity->authorName = value;

                if( GetGitBackend()->GetConfigString( &handler, "user.email", value ) )
                    identity->authorEmail = value;
            },
            [this, common, identity]()
            {
                if( !KIGIT::PromptGitIdentity( this, _( "Set Author" ), identity->authorName, identity->authorEmail ) )
                {
                    return;
                }

                if( common != m_frame->Prj().GetGitCommon() || common->IsShuttingDown() )
                    return;

                std::unique_lock<std::mutex> lock = KIGIT::TryLockAction( *common, this );

                if( !lock.owns_lock() )
                    return;

                bool saved = common->SetUserConfig( identity->authorName, identity->authorEmail );
                lock.unlock();

                if( !saved )
                {
                    DisplayErrorMessage( this, _( "Could not save the name and email to the repository." ) );
                    return;
                }

                // The Repository page shows the same name and email.
                common->NotifyRefreshListeners();
            },
            _( "Reading author..." ) );
}


bool GIT_COMMIT_PANE::anyEditorDirty() const
{
    return DocumentEditorsDirty( m_frame->Kiway() );
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
    if( m_transfer.IsBusy() || m_actionPending )
        return;

    if( m_operationInProgress )
    {
        m_commitButton->Enable( false );
        m_pullButton->SetLabel( abandonLabel() );
        m_pullButton->Enable( true );
        return;
    }

    if( m_headDetached )
    {
        m_commitButton->Enable( false );
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

    wxString mainLabel = _( "Commit" );
    wxString menuLabel = _( "Commit and Push" );
    bool     mainEnabled = false;
    bool     menuEnabled = false;
    m_commitButtonPushes = false;

    // A disabled button with nothing said about it reads as broken, so each state the user can
    // act on names what it is waiting for.
    wxString blocked;

    if( amend )
    {
        // A message-only amend needs no checked files, just a message.
        mainLabel = _( "Amend" );
        menuLabel = _( "Amend and Push" );
        mainEnabled = hasMessage;
        menuEnabled = hasMessage;

        if( !hasMessage )
            blocked = _( "Write a summary to amend the last commit." );
    }
    else if( hasCheckedChanges || m_editorsDirty )
    {
        // Unsaved editor changes count because the commit flow saves them first, then commits
        // what the save produced.
        mainEnabled = hasMessage && hasBranchName;
        menuEnabled = hasMessage && hasBranchName;

        if( !hasMessage )
            blocked = _( "Write a summary to commit." );
        else if( !hasBranchName )
            blocked = _( "Name the new branch, or untick Create a new branch." );
    }
    else if( m_hasUnpushedCommits )
    {
        mainLabel = _( "Push" );
        mainEnabled = true;
        m_commitButtonPushes = true;
    }
    else if( !m_shownPaths.empty() )
    {
        blocked = _( "Tick a file to commit it." );
    }

    m_commitButton->SetLabel( mainLabel );
    m_commitButton->Enable( mainEnabled );
    m_commitAndPushItem->SetItemLabel( menuLabel );
    m_commitAndPushItem->Enable( menuEnabled );

    if( !blocked.IsEmpty() )
        setStatusText( blocked );

    // With nothing listed the empty state owns the line and has already set its own text.
    bool showReason = !blocked.IsEmpty() || m_shownPaths.empty();

    if( m_emptyLabel->IsShown() != showReason )
    {
        m_emptyLabel->Show( showReason );
        Layout();
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
    if( m_actionPending || m_transfer.IsBusy() )
    {
        KIGIT::ShowBusyMessage( this );
        return;
    }

    wxString branch = m_branchChoice->GetStringSelection();
    m_actionPending = true;
    setBusy( true, _( "Checking branch..." ) );

    SwitchProjectToBranch( m_frame, this, branch, m_actionQuery,
                           [this]()
                           {
                               m_actionPending = false;
                               setBusy( false, wxEmptyString );

                               // A refused switch leaves the choice on a branch the project is not on.
                               RefreshFileList();
                           } );
}


void GIT_COMMIT_PANE::onShowDiff( wxCommandEvent& aEvent )
{
    wxString path = m_checklist->GetItemPath( m_checklist->GetSelection() );

    if( !path.IsEmpty() )
        ShowGitFileDiff( m_frame, path );
}


void GIT_COMMIT_PANE::populateBranches( const STATUS& aStatus )
{
    m_branchChoice->Clear();
    m_branchRow->ShowItems( aStatus.hasRepo );

    // The advanced window is reachable without a repository, as the old button was.
    m_settingsButton->Show();

    if( !aStatus.hasRepo )
        return;

    // Only a local branch can take a commit.
    for( const auto& [name, isRemote] : aStatus.branches )
    {
        if( !isRemote )
            m_branchChoice->Append( name );
    }

    wxString current = aStatus.currentBranch;

    m_branchChoice->SetStringSelection( current );
    m_branchChoice->Enable( !m_transfer.IsBusy() && !m_actionPending && !m_headDetached && !m_operationInProgress );

    // The box ellipsizes, so the whole name is only readable here.
    m_branchChoice->SetToolTip( current );
}


void GIT_COMMIT_PANE::populateRecentMessages( const std::vector<wxString>& aMessages )
{
    m_recentMessageTexts = aMessages;

    // A dropdown always shows its selection, so the first entry is the caption.
    m_recentMessages->Clear();
    m_recentMessages->Append( _( "Recent messages" ) );

    for( const wxString& message : m_recentMessageTexts )
    {
        wxString line = message.BeforeFirst( '\n' );

        if( line.length() > 40 )
            line = line.Left( 37 ) + wxS( "..." );

        m_recentMessages->Append( line );
    }

    m_recentMessages->SetSelection( 0 );
    m_recentMessages->Show( !m_recentMessageTexts.empty() );
}


void GIT_COMMIT_PANE::onRecentMessage( wxCommandEvent& aEvent )
{
    int      picked = m_recentMessages->GetSelection() - 1;
    wxString summary, description;

    // The caption entry starts the message over.
    if( picked >= 0 && picked < static_cast<int>( m_recentMessageTexts.size() ) )
        KIGIT::SplitCommitMessage( m_recentMessageTexts[picked], summary, description );

    m_summary->SetValue( summary );
    setDescription( description );
    m_recentMessages->SetSelection( 0 );
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
    if( m_transfer.IsBusy() || m_actionPending )
        return;

    wxString path = m_checklist->GetItemPath( aEvent.GetItem() );

    if( path.IsEmpty() )
        return;

    wxMenu menu;
    menu.Append( 1, _( "Show History for This File" ) );
    menu.AppendSeparator();
    menu.Append( 2, _( "Discard Changes" ) );

    switch( GetPopupMenuSelectionFromUser( menu ) )
    {
    case 1:
        // From the Changes page the window is already open, so it only turns to History.
        if( GIT_ADVANCED_DIALOG* dialog = m_inAdvancedWindow ? GIT_ADVANCED_DIALOG::Active() : nullptr )
            dialog->ShowHistoryForFile( path );
        else
            GIT_ADVANCED_DIALOG::ShowHistoryForFile( m_frame, this, path );

        break;

    case 2:
        discardFile( m_frame->Prj().GetGitCommon(), path );
        break;

    default:
        break;
    }
}


void GIT_COMMIT_PANE::discardFile( std::shared_ptr<KIGIT_COMMON> aCommon, const wxString& aPath )
{
    struct FILE_STATE
    {
        bool     tracked = false;
        bool     conflicted = false;
        wxString root;
    };

    auto state = std::make_shared<FILE_STATE>();

    runActionQuery(
            aCommon,
            [state, aPath]( KIGIT_COMMON& aSession )
            {
                state->tracked = aSession.IsPathInHead( aPath );
                state->conflicted = aSession.IsPathConflicted( aPath );
                state->root = aSession.GetGitRootDirectory();
            },
            [this, aCommon, aPath, state]()
            {
                confirmDiscardFile( aCommon, aPath, state->tracked, state->conflicted, state->root );
            },
            _( "Checking changes..." ) );
}


void GIT_COMMIT_PANE::confirmDiscardFile( std::shared_ptr<KIGIT_COMMON> aCommon, const wxString& aPath, bool aTracked,
                                          bool aConflicted, wxString aRoot )
{
    // An unsaved editor's next save would undo the discard, so settle that first.
    if( DocumentEditorsDirty( m_frame->Kiway() ) || !LibraryEditorsWithUnsavedChanges( m_frame->Kiway() ).empty() )
    {
        DisplayInfoMessage( this, _( "An editor has unsaved changes. Save or revert them before discarding "
                                     "files, so the discard is not undone by the next save." ) );
        return;
    }

    wxString name = wxFileName( aPath ).GetFullName();
    wxString question;

    if( aConflicted )
    {
        // Putting back what HEAD holds also clears the conflict, so the file is settled rather
        // than only reverted.
        question = wxString::Format( _( "Resolve the conflict in '%s' by going back to the version this "
                                        "project was on?\n\n"
                                        "The version that arrived is dropped, and the file no longer needs "
                                        "resolving." ),
                                     name );
    }
    else if( aTracked )
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
    dlg.SetYesNoLabels( aConflicted ? _( "Resolve" )
                        : aTracked  ? _( "Discard" )
                                    : _( "Move to Trash" ),
                        _( "Cancel" ) );

    if( dlg.ShowModal() != wxID_YES )
        return;

    if( aCommon != m_frame->Prj().GetGitCommon() || aCommon->IsShuttingDown() )
        return;

    if( DocumentEditorsDirty( m_frame->Kiway() ) || !LibraryEditorsWithUnsavedChanges( m_frame->Kiway() ).empty() )
        return;

    // Snapshot first, so an editor showing the file reloads the committed version afterwards.
    EDITOR_SNAPSHOT docsBefore = CaptureOpenEditorDocs( m_frame->Kiway() );

    // Dropping the staged entry before touching the file means a contended lock or an index
    // failure leaves everything unchanged.
    std::unique_lock<std::mutex> lock = KIGIT::TryLockAction( *aCommon, this );

    if( !lock.owns_lock() )
        return;

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

    if( !aTracked )
    {
        if( !aRoot.EndsWith( wxS( "/" ) ) )
            aRoot += wxS( "/" );

        wxFileName fullPath( aRoot + aPath );

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
    doCommit( m_commitButtonPushes );
}


void GIT_COMMIT_PANE::onCommitAndPush( wxCommandEvent& aEvent )
{
    doCommit( true );
}


void GIT_COMMIT_PANE::onPull( wxCommandEvent& aEvent )
{
    syncListener();

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    if( !common->GetRepo() )
        return;

    if( m_actionPending )
    {
        m_actionQuery.Cancel();
        m_actionPending = false;
        setBusy( false, _( "Cancelled." ) );
        return;
    }

    // While a transfer runs this button reads Cancel. The transfer callbacks poll the flag and
    // abort on their next tick, and the completion then reports the cancellation.
    if( m_transfer.IsBusy() )
    {
        m_transfer.Cancel();

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
    // The Advanced window also calls this, with the panel's status out of sight.
    if( m_transfer.IsBusy() || m_actionPending )
    {
        KIGIT::ShowBusyMessage( this );
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
        DisplayInfoMessage( this, _( "This project is showing an old snapshot rather than a branch, so it cannot be "
                                     "committed or pushed from here. In the Advanced Git window, choose Branches "
                                     "and double click a branch to go back to it." ) );
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

    submitPush( common );
}


void GIT_COMMIT_PANE::abandonOperation( std::shared_ptr<KIGIT_COMMON> aCommon )
{
    struct PREPARATION
    {
        std::vector<wxString> outside;
        bool                  ready = false;
    };

    auto           preparation = std::make_shared<PREPARATION>();
    const wxString projectPath = m_frame->Prj().GetProjectPath();

    runActionQuery(
            aCommon,
            [preparation, projectPath]( KIGIT_COMMON& aSession )
            {
                // Going back to the last commit covers the whole repository, so work in a
                // sibling project has to be named before the user agrees.
                preparation->outside = ChangedTrackedFilesOutsideProject( aSession, projectPath );
                preparation->ready = true;
            },
            [this, aCommon, preparation]()
            {
                if( preparation->ready )
                    finishAbandonOperation( aCommon, preparation->outside );
            },
            _( "Checking working files..." ) );
}


void GIT_COMMIT_PANE::finishAbandonOperation( std::shared_ptr<KIGIT_COMMON> aCommon,
                                              const std::vector<wxString>&  aOutside )
{
    // Saving would only turn unsaved edits into changes that are then thrown away, so ask for
    // a decision in the editors first rather than offer a save.
    auto canAbandon = [this, aCommon]()
    {
        if( aCommon != m_frame->Prj().GetGitCommon() || aCommon->IsShuttingDown()
            || !Pgm().GetCommonSettings()->m_Git.enableGit )
        {
            return false;
        }

        if( DocumentEditorsDirty( m_frame->Kiway() ) || !LibraryEditorsWithUnsavedChanges( m_frame->Kiway() ).empty() )
        {
            DisplayInfoMessage( this, _( "The open editors have unsaved changes. Save or revert them first, so it "
                                         "is clear what is being abandoned." ) );
            return false;
        }

        return true;
    };

    if( !canAbandon() )
        return;

    wxString question = wxString::Format( _( "Abandon the %s and go back to the last commit?\n\n"
                                             "Changes made to the files since that commit are discarded." ),
                                          operationNoun() );

    if( !aOutside.empty() )
    {
        wxString listed;

        for( const wxString& path : aOutside )
            listed += wxS( "\n    " ) + path;

        question += wxString::Format( _( "\n\nThis also discards uncommitted changes elsewhere in this "
                                         "repository:%s" ),
                                      listed );
    }

    if( !IsOK( this, question ) || !canAbandon() )
        return;

    EDITOR_SNAPSHOT docsBefore = CaptureOpenEditorDocs( m_frame->Kiway() );
    wxString        error;
    bool            ok = false;

    {
        std::unique_lock<std::mutex> lock = KIGIT::TryLockAction( *aCommon, this );

        if( !lock.owns_lock() )
            return;

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
    m_messageQuery.Cancel();

    // Ticking the box offers the last commit's message as the starting point, since an amend
    // usually keeps or touches up the existing text.
    if( m_amendCheckbox->IsChecked() && m_summary->GetValue().Trim().IsEmpty()
        && m_description->GetValue().Trim().IsEmpty() )
    {
        std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
        auto                          last = std::make_shared<wxString>();

        m_messageQuery.Run(
                common,
                [last]( KIGIT_COMMON& aCommon )
                {
                    *last = aCommon.GetLastCommitMessage();
                },
                [this, common, last]()
                {
                    if( common != m_frame->Prj().GetGitCommon() || common->IsShuttingDown()
                        || !m_amendCheckbox->IsChecked() || last->IsEmpty() || !m_summary->GetValue().Trim().IsEmpty()
                        || !m_description->GetValue().Trim().IsEmpty() )
                    {
                        return;
                    }

                    wxString summary, description;
                    KIGIT::SplitCommitMessage( *last, summary, description );
                    m_summary->SetValue( summary );
                    setDescription( description );
                    updateCommitButton();
                } );
    }

    updateMessageVisibility( m_checklist->IsShown() );
    Layout();

    updateCommitButton();
}


struct GIT_COMMIT_PANE::COMMIT_REQUEST
{
    bool                          hasIdentity = false;
    bool                          amend = false;
    bool                          newBranch = false;
    bool                          andPush = false;
    bool                          signOff = false;
    bool                          darkTheme = false;
    wxString                      displayedBranch;
    wxString                      branchName;
    wxString                      summary;
    wxString                      description;
    wxString                      projectPath;
    wxString                      projectName;
    wxString                      lfsMessage;
    std::vector<wxString>         files;
    std::set<wxString>            beforeSave;
    std::pair<wxString, wxString> head;
};


void GIT_COMMIT_PANE::doCommit( bool aAndPush )
{
    syncListener();

    if( m_actionPending || m_transfer.IsBusy() )
    {
        KIGIT::ShowBusyMessage( this );
        return;
    }

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

        submitPush( common );
        return;
    }

    auto request = std::make_shared<COMMIT_REQUEST>();
    request->amend = amend;
    request->newBranch = newBranch;
    request->andPush = aAndPush;
    request->signOff = m_signOffCheckbox->IsChecked();
    request->branchName = branchName;
    request->displayedBranch = m_branchChoice->GetStringSelection();
    request->summary = m_summary->GetValue();
    request->description = m_description->GetValue();
    request->files = std::move( files );
    request->projectPath = m_frame->Prj().GetProjectPath();
    request->projectName = m_frame->Prj().GetProjectName();
    request->darkTheme = KIPLATFORM::UI::IsDarkTheme();

    // Snapshot immediately before saving. Using the last displayed status could sweep files
    // the user never saw into the commit when the editors save.
    runActionQuery(
            common,
            [request]( KIGIT_COMMON& aCommon )
            {
                request->head = commitHead( aCommon.GetRepo() );
                request->hasIdentity = KIGIT::HasGitIdentity( aCommon );
                request->lfsMessage = KIGIT::PROJECT_GIT_UTILS::LfsRefusal( &aCommon );

                for( const FILE_CHECKLIST_ENTRY& entry :
                     BuildGitChangeEntries( aCommon, request->projectPath, request->projectName, request->darkTheme ) )
                {
                    request->beforeSave.insert( entry.path );
                }
            },
            [this, common, request]()
            {
                saveForCommit( common, request );
            },
            _( "Checking changes..." ) );
}


void GIT_COMMIT_PANE::saveForCommit( std::shared_ptr<KIGIT_COMMON> aCommon, std::shared_ptr<COMMIT_REQUEST> aRequest )
{
    if( !aRequest->displayedBranch.IsEmpty()
        && aRequest->head.first != wxS( "refs/heads/" ) + aRequest->displayedBranch )
    {
        DisplayInfoMessage( this, _( "The branch changed while preparing the commit. "
                                     "Review the changes and commit again." ) );
        aCommon->NotifyRefreshListeners();
        return;
    }

    if( !aRequest->lfsMessage.IsEmpty() )
    {
        DisplayErrorMessage( this, aRequest->lfsMessage );
        return;
    }

    bool saved = false;

    if( !PrepareEditorsForGitOperation( m_frame, GIT_EDITOR_PREP::COMMIT, &saved ) )
        return;

    if( aCommon != m_frame->Prj().GetGitCommon() || aCommon->IsShuttingDown() )
        return;

    // Commit Anyway approves only the selected files. Nothing was saved to discover here.
    if( !saved )
    {
        finishCommit( aCommon, *aRequest );
        return;
    }

    // Saving can create design changes that were only in editor memory. Local settings are
    // absent because saving itself dirties them, without an edit by the user.
    runActionQuery(
            aCommon,
            [aRequest]( KIGIT_COMMON& aSession )
            {
                std::set<wxString> already( aRequest->files.begin(), aRequest->files.end() );

                for( const FILE_CHECKLIST_ENTRY& entry : BuildGitChangeEntries(
                             aSession, aRequest->projectPath, aRequest->projectName, aRequest->darkTheme ) )
                {
                    wxString ext = wxFileName( entry.path ).GetExt();
                    bool     document = ext.CmpNoCase( FILEEXT::KiCadSchematicFileExtension ) == 0
                                        || ext.CmpNoCase( FILEEXT::KiCadPcbFileExtension ) == 0
                                        || ext.CmpNoCase( FILEEXT::ProjectFileExtension ) == 0;

                    if( document && !aRequest->beforeSave.count( entry.path ) && already.insert( entry.path ).second )
                        aRequest->files.push_back( entry.path );
                }
            },
            [this, aCommon, aRequest]()
            {
                finishCommit( aCommon, *aRequest );
            },
            _( "Checking saved changes..." ) );
}


void GIT_COMMIT_PANE::finishCommit( std::shared_ptr<KIGIT_COMMON> common, const COMMIT_REQUEST& aRequest )
{
    const std::vector<wxString>& files = aRequest.files;
    const bool                   amend = aRequest.amend;
    const bool                   newBranch = aRequest.newBranch;
    const wxString&              branchName = aRequest.branchName;

    // Entered on the strength of unsaved editor changes alone, but the user declined the save,
    // so nothing reached the working tree and there is nothing to commit.
    if( files.empty() && !amend )
    {
        setBusy( false, _( "Nothing was saved, so there is nothing to commit." ) );
        return;
    }

    // A fresh machine has no git identity yet, and the commit cannot be recorded without one.
    if( !KIGIT::EnsureGitIdentity( this, common.get(), aRequest.hasIdentity ) )
        return;

    if( common != m_frame->Prj().GetGitCommon() || common->IsShuttingDown() )
        return;

    CommitResult result = CommitResult::Error;
    wxString     error;
    wxString     branchError;
    wxString     branchDetail;
    wxString     previousBranch;

    {
        // Keep the lock through branch creation and the commit.
        std::unique_lock<std::mutex> lock = KIGIT::TryLockAction( *common, this );

        if( !lock.owns_lock() )
            return;

        if( aRequest.head != commitHead( common->GetRepo() ) )
        {
            lock.unlock();
            DisplayInfoMessage( this, _( "The branch or its last commit changed while preparing the commit. "
                                         "Review the changes and commit again." ) );
            common->NotifyRefreshListeners();
            return;
        }

        GIT_CONFIG_HANDLER configHandler( common.get() );
        GitUserConfig      userConfig = configHandler.GetUserConfig();
        wxString           signOff;

        if( aRequest.signOff )
            signOff = wxString::Format( wxS( "%s <%s>" ), userConfig.authorName, userConfig.authorEmail );

        wxString message = KIGIT::ComposeCommitMessage( aRequest.summary, aRequest.description, signOff );

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

    // The other commit pane can still edit the shared message while this one is preparing.
    if( m_summary->GetValue() == aRequest.summary && m_description->GetValue() == aRequest.description )
    {
        m_summary->Clear();
        setDescription( wxEmptyString );
    }
    m_amendCheckbox->SetValue( false );
    m_newBranchCheckbox->SetValue( false );
    m_newBranchName->Clear();
    common->NotifyRefreshListeners();

    if( aRequest.andPush )
        submitPush( common );
    else
        setBusy( false, _( "Committed." ) );
}


void GIT_COMMIT_PANE::runActionQuery( std::shared_ptr<KIGIT_COMMON> aCommon, std::function<void( KIGIT_COMMON& )> aWork,
                                      std::function<void()> aDone, const wxString& aStatus )
{
    if( m_actionPending || m_transfer.IsBusy() )
    {
        KIGIT::ShowBusyMessage( this );
        return;
    }

    m_actionPending = true;
    setBusy( true, aStatus );
    auto read = std::make_shared<bool>( false );

    m_actionQuery.Run(
            aCommon,
            [read, work = std::move( aWork )]( KIGIT_COMMON& aSession )
            {
                work( aSession );
                *read = true;
            },
            [this, aCommon, read, done = std::move( aDone )]()
            {
                m_actionPending = false;
                setBusy( false, wxEmptyString );

                if( *read && aCommon == m_frame->Prj().GetGitCommon() && !aCommon->IsShuttingDown()
                    && aCommon->GetRepo() && Pgm().GetCommonSettings()->m_Git.enableGit )
                {
                    done();
                }
            } );
}


void GIT_COMMIT_PANE::setBusy( bool aBusy, const wxString& aStatus )
{
    m_checklist->Enable( !aBusy );
    m_commitButton->Enable( !aBusy );
    m_amendCheckbox->Enable( !aBusy );
    m_signOffCheckbox->Enable( !aBusy );
    m_newBranchCheckbox->Enable( !aBusy );
    m_newBranchName->Enable( !aBusy && m_newBranchCheckbox->IsChecked() );
    m_branchChoice->Enable( !aBusy && !m_headDetached && !m_operationInProgress );
    m_recentMessages->Enable( !aBusy );
    m_summary->Enable( !aBusy );
    m_description->Enable( !aBusy );
    m_frame->SetStatusText( aStatus );

    // The frame status bar is overwritten within seconds by the editors, so an outcome shown
    // with the empty label also goes to the panel's own label, where it stays.
    if( !aBusy && !aStatus.IsEmpty() && m_emptyLabel->IsShown() && !m_headDetached && !m_operationInProgress )
        setStatusText( aStatus );

    if( m_busyChangedHandler )
        m_busyChangedHandler( aBusy );

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
    struct TARGET
    {
        bool     hasRemote = false;
        wxString branch;
        wxString head;
    };

    auto target = std::make_shared<TARGET>();

    runActionQuery(
            aCommon,
            [target]( KIGIT_COMMON& aSession )
            {
                target->hasRemote = aSession.HasPushAndPullRemote();
                target->branch = aSession.GetCurrentBranchName();
                target->head = aSession.GetHeadCommitId();
            },
            [this, aCommon, target]()
            {
                if( !target->hasRemote )
                {
                    setBusy( false, _( "Committed. No remote is configured to push to." ) );
                    aCommon->NotifyRefreshListeners();
                    return;
                }

                startPush( aCommon, target->branch, target->head );
            },
            _( "Checking remote..." ) );
}


void GIT_COMMIT_PANE::startPush( std::shared_ptr<KIGIT_COMMON> aCommon, const wxString& aBranch, const wxString& aHead )
{
    setBusy( true, _( "Pushing..." ) );

    // Carried rather than captured by reference, because Start returns before the work runs.
    struct OUTCOME
    {
        PushResult result = PushResult::Error;
        wxString   error;
        bool       authFailure = false;
        bool       cancelled = false;
    };

    auto outcome = std::make_shared<OUTCOME>();

    bool started = m_transfer.Start(
            "git push",
            [aCommon, outcome, aBranch, aHead]( const KIGIT_COMMON::CANCEL_TOKEN& aToken )
            {
                aCommon->ClearAuthFailure();

                GIT_PUSH_HANDLER handler( aCommon.get() );
                handler.SetWaitForActionLock( true );
                handler.SetExpectedTarget( aBranch, aHead );
                handler.SetCancelToken( aToken );
                outcome->result = handler.PerformPush();
                outcome->error = handler.GetErrorString();
                outcome->authFailure = aCommon->WasAuthFailure();

                // This operation's own token, so a cancel aimed at something else is not
                // reported here and this one's is not left behind for the next operation.
                outcome->cancelled = handler.IsThisOperationCancelled();

                // The other surfaces need this even when the pane that started it has gone.
                if( outcome->result == PushResult::Success )
                {
                    KIGIT::CallOnUi(
                            [aCommon]()
                            {
                                aCommon->NotifyRefreshListeners();
                            } );
                }
            },
            [this, aCommon, outcome]()
            {
                onPushDone( aCommon, static_cast<int>( outcome->result ), outcome->error, outcome->authFailure,
                            outcome->cancelled );
            } );

    if( !started )
        setBusy( false, _( "Could not start the push." ) );
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

    if( result == PushResult::Busy )
    {
        setBusy( false, wxEmptyString );
        KIGIT::ShowBusyMessage( this );
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
        // Getting their changes needs a clean tree. Files left unticked at the commit, or edits
        // still in an editor, block it, and offering the click anyway would only fail later.
        auto     worktree = std::make_shared<GIT_WORKTREE_STATUS>();
        wxString projectPath = m_frame->Prj().GetProjectPath();

        runActionQuery(
                aCommon,
                [worktree, projectPath]( KIGIT_COMMON& aSession )
                {
                    *worktree = ReadGitWorktreeStatus( aSession, projectPath );
                },
                [this, aCommon, worktree]()
                {
                    if( !ProjectSafeToRewrite( m_frame, this, *worktree ) )
                        return;

                    if( IsOK( this, _( "Someone else pushed changes first.\n\n"
                                       "Get their changes and push again?" ) )
                        && aCommon == m_frame->Prj().GetGitCommon() && !aCommon->IsShuttingDown()
                        && Pgm().GetCommonSettings()->m_Git.enableGit )
                    {
                        submitPull( aCommon, true );
                    }
                },
                _( "Checking working files..." ) );

        return;
    }

    setBusy( false, _( "Push failed." ) );
    DisplayErrorMessage( this, wxString::Format( _( "Could not push: %s" ), aError ) );
}


// The worker owns the materialized conflict files until the UI finishes resolving them. A
// cancelled query drops the same object, so scratch files cannot outlive an abandoned flow.
struct GIT_COMMIT_PANE::MERGE_PREPARATION
{
    std::unique_ptr<GIT_DIFF_WORKSPACE> workspace;
    wxString                            headOid;
    wxString                            theirsOid;
    wxString                            rootDir;
    wxString                            error;
    std::vector<KIGIT_CONFLICT_SIDES>   conflicts;
    bool                                hasIdentity = false;
    bool                                rebaseConfigured = false;
    bool                                prepared = false;
};


void GIT_COMMIT_PANE::resolvePullConflicts( std::shared_ptr<KIGIT_COMMON> aCommon, bool aThenPush )
{
    auto preparation = std::make_shared<MERGE_PREPARATION>();

    runActionQuery(
            aCommon,
            [preparation]( KIGIT_COMMON& aSession )
            {
                preparation->workspace = std::make_unique<GIT_DIFF_WORKSPACE>();

                if( !preparation->workspace->IsOk() )
                    return;

                preparation->rebaseConfigured = aSession.IsPullRebaseConfigured();
                preparation->hasIdentity = KIGIT::HasGitIdentity( aSession );
                preparation->rootDir = aSession.GetGitRootDirectory();
                preparation->prepared = aSession.GetMergeConflicts(
                        preparation->workspace->Root() + wxFileName::GetPathSeparator() + wxS( "stages" ),
                        preparation->headOid, preparation->theirsOid, preparation->conflicts, preparation->error );
            },
            [this, aCommon, aThenPush, preparation]()
            {
                finishPullConflictPreparation( aCommon, aThenPush, preparation );
            },
            _( "Preparing the merge..." ) );
}


void GIT_COMMIT_PANE::finishPullConflictPreparation( std::shared_ptr<KIGIT_COMMON> aCommon, bool aThenPush,
                                                     std::shared_ptr<MERGE_PREPARATION> aPreparation )
{
    if( !aPreparation->workspace || !aPreparation->workspace->IsOk() )
    {
        DisplayErrorMessage( this, _( "Could not create a temporary directory for the merge." ) );
        return;
    }

    if( !aPreparation->prepared )
    {
        DisplayErrorMessage( this, _( "Could not prepare the merge." ), aPreparation->error );
        return;
    }

    // Editors remain usable while the repository is read and while another editor's merge
    // tool is open. Refuse to reload over anything typed during either interval.
    auto canResolve = [this, aCommon]()
    {
        if( aCommon != m_frame->Prj().GetGitCommon() || aCommon->IsShuttingDown()
            || !Pgm().GetCommonSettings()->m_Git.enableGit )
        {
            return false;
        }

        if( DocumentEditorsDirty( m_frame->Kiway() ) || !LibraryEditorsWithUnsavedChanges( m_frame->Kiway() ).empty() )
        {
            DisplayInfoMessage( this, _( "The open editors have unsaved changes. Save or revert them before "
                                         "resolving the merge." ) );
            return false;
        }

        return true;
    };

    if( !canResolve() )
        return;

    // A conflicted rebase was already abandoned by the time it got here, and settling it from
    // this window can only produce a merge commit. Someone who asked for rebases is told that
    // before one is made.
    if( aPreparation->rebaseConfigured
        && !IsOK( this, _( "This project is set to rebase when getting changes, but a conflict can only be "
                           "settled here by merging. Carry on and make a merge commit, or cancel and rebase "
                           "from a git command line?" ) ) )
    {
        setBusy( false, _( "Getting changes was cancelled. Your files are unchanged." ) );
        return;
    }

    if( !canResolve() )
        return;

    // The merge commit needs an identity whatever the user decides about the files, so it is
    // asked for before any merging. Cancelling it then costs no work.
    if( !KIGIT::EnsureGitIdentity( this, aCommon.get(), aPreparation->hasIdentity ) )
    {
        setBusy( false, _( "Getting changes was cancelled. Your files are unchanged." ) );
        return;
    }

    if( !canResolve() )
        return;

    GIT_DIFF_WORKSPACE&                      workspace = *aPreparation->workspace;
    const wxString                           sep = wxFileName::GetPathSeparator();
    const wxString&                          headOid = aPreparation->headOid;
    const wxString&                          theirsOid = aPreparation->theirsOid;
    const std::vector<KIGIT_CONFLICT_SIDES>& conflicts = aPreparation->conflicts;
    wxString                                 error;

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
        if( !canResolve() )
            return;

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

    if( !canResolve() )
        return;

    // Written before the merge is published, so a copy that cannot be made stops the merge
    // rather than losing the version it was meant to protect.
    wxString rootDir = aPreparation->rootDir;

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

    MergeCommitResult result = MergeCommitResult::FailedCleanly;

    // The lock was released while the user resolved, and the background fetch may hold it now.
    // Throwing the resolutions away would mean doing every file again, so let the user retry.
    while( true )
    {
        if( !canResolve() )
        {
            discardSavedCopies();
            return;
        }

        std::unique_lock<std::mutex> lock = aCommon->TryLockAction();

        if( lock.owns_lock() )
        {
            result = aCommon->CreateMergedCommit( headOid, theirsOid, resolved, error );
            break;
        }

        if( !IsOK( this, _( "Another git operation is in progress. Try again?" ) ) )
        {
            discardSavedCopies();
            setBusy( false, _( "Getting changes was cancelled. Your files are unchanged." ) );
            return;
        }
    }

    if( result == MergeCommitResult::FailedCleanly )
    {
        // Nothing reached the files, so the copies that would have explained it go too.
        discardSavedCopies();
    }
    else
    {
        // Whatever was written has to reach the editors and panels, and a partial write leaves
        // the saved copies as the only way back to the old version.
        ReloadOpenEditors( m_frame->Kiway(), m_docsBeforePull );
        aCommon->NotifyRefreshListeners();
    }

    if( result != MergeCommitResult::Success )
    {
        DisplayErrorMessage( this, _( "Could not finish the merge." ), error );
        return;
    }

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

    struct CHECK
    {
        bool                hasIdentity = false;
        GIT_WORKTREE_STATUS worktree;
    };

    auto     result = std::make_shared<CHECK>();
    wxString projectPath = m_frame->Prj().GetProjectPath();

    runActionQuery(
            aCommon,
            [result, projectPath]( KIGIT_COMMON& aSession )
            {
                result->hasIdentity = KIGIT::HasGitIdentity( aSession );
                result->worktree = ReadGitWorktreeStatus( aSession, projectPath );
            },
            [this, aCommon, aThenPush, result]()
            {
                if( !ProjectSafeToRewrite( m_frame, this, result->worktree ) )
                    return;

                // A non-fast-forward pull creates a commit, so obtain its identity before
                // the worker can change files. The repository check was in the query above.
                if( !KIGIT::EnsureGitIdentity( this, aCommon.get(), result->hasIdentity ) )
                    return;

                if( aCommon != m_frame->Prj().GetGitCommon() || aCommon->IsShuttingDown() )
                    return;

                // The identity prompt can allow new editor changes while it is open.
                if( !ProjectSafeToRewrite( m_frame, this, result->worktree ) )
                    return;

                startPull( aCommon, aThenPush );
            },
            _( "Checking changes..." ) );
}


void GIT_COMMIT_PANE::startPull( std::shared_ptr<KIGIT_COMMON> aCommon, bool aThenPush )
{
    setBusy( true, _( "Getting changes..." ) );

    // Record what the open documents look like on disk now, so only the ones the merge actually
    // rewrites are reloaded afterwards.
    m_docsBeforePull = CaptureOpenEditorDocs( m_frame->Kiway() );

    EDITOR_SNAPSHOT docsBefore = m_docsBeforePull;

    // The KIWAY outlives this pane, so the editors are still reached when the pane has gone.
    KIWAY* kiway = &m_frame->Kiway();

    // Carried rather than captured by reference, because Start returns before the work runs.
    struct OUTCOME
    {
        PullResult result = PullResult::Error;
        wxString   error;
        bool       authFailure = false;
        bool       cancelled = false;
    };

    auto outcome = std::make_shared<OUTCOME>();

    bool started = m_transfer.Start(
            "git pull",
            [aCommon, docsBefore, kiway, outcome]( const KIGIT_COMMON::CANCEL_TOKEN& aToken )
            {
                aCommon->ClearAuthFailure();

                GIT_PULL_HANDLER handler( aCommon.get() );
                handler.SetWaitForActionLock( true );
                handler.SetCancelToken( aToken );
                outcome->result = handler.PerformPull();
                outcome->error = handler.GetErrorStrings().empty() ? wxString() : handler.GetErrorString();
                outcome->authFailure = aCommon->WasAuthFailure();

                // This operation's own token, so a cancel aimed at something else is not
                // reported here and this one's is not left behind for the next operation.
                outcome->cancelled = handler.IsThisOperationCancelled();

                PullResult result = outcome->result;

                KIGIT::CallOnUi(
                        [aCommon, docsBefore, result, kiway]()
                        {
                            // A merge that failed partway can still have written files, so the
                            // editors are brought up to date whatever the outcome. Nothing
                            // reloads when nothing changed on disk.
                            ReloadOpenEditors( *kiway, docsBefore );

                            if( result == PullResult::Success || result == PullResult::FastForward )
                                aCommon->NotifyRefreshListeners();
                        } );
            },
            [this, aCommon, aThenPush, outcome]()
            {
                onPullDone( aCommon, static_cast<int>( outcome->result ), outcome->error, outcome->authFailure,
                            aThenPush, outcome->cancelled );
            } );

    if( !started )
        setBusy( false, _( "Could not start getting changes." ) );
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
