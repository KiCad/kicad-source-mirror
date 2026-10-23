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

#include <dialogs/git/git_advanced_dialog.h>

#include <bitmaps.h>
#include <dialogs/git/git_file_diff_area.h>
#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/git_transfer.h>
#include <dialogs/git/git_ui_utils.h>
#include <dialogs/git/panel_git_branches.h>
#include <dialogs/git/panel_git_history.h>
#include <dialogs/git/panel_git_remotes.h>
#include <dialogs/git/panel_git_repository.h>
#include <dialogs/git/panel_git_stashes.h>
#include <dialogs/git/panel_git_tags.h>

#include <memory>

#include <wx/bitmap.h>
#include <wx/bmpbndl.h>
#include <wx/filename.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>

#include <confirm.h>
#include <eda_base_frame.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <git/git_pull_handler.h>
#include <git/kicad_git_common.h>
#include <kiplatform/ui.h>
#include <kiway.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_local_settings.h>
#include <settings/common_settings.h>
#include <thread>
#include <widgets/git_commit_pane.h>
#include <widgets/ui_common.h>
#include <widgets/wx_treebook.h>


void GIT_ADVANCED_DIALOG::optOutOfTextPersistence( wxWindow* aWindow )
{
    for( wxWindow* child : aWindow->GetChildren() )
    {
        if( dynamic_cast<wxTextEntry*>( child ) )
            OptOut( child );

        optOutOfTextPersistence( child );
    }
}


GIT_ADVANCED_DIALOG::GIT_ADVANCED_DIALOG( EDA_BASE_FRAME* aParent, GIT_COMMIT_PANE* aPane ) :
        DIALOG_GIT_ADVANCED_BASE( aParent ),
        m_parent( aParent ),
        m_pane( aPane ),
        m_historyPage( nullptr ),
        m_branchesPage( nullptr ),
        m_remotesPage( nullptr ),
        m_stashesPage( nullptr ),
        m_tagsPage( nullptr ),
        m_repositoryPage( nullptr ),
        m_changesSplitter( nullptr ),
        m_changesPane( nullptr ),
        m_workingDiff( nullptr )
{
    m_hash_key = "GitAdvancedDialog";

    m_book->SetFont( KIUI::GetControlFont( this ) );

    m_repoName->SetFont( KIUI::GetControlFont( this ).Bold() );
    m_book->SetFitToCurrentPage( true );

    // One icon per page, in page order, as the About dialog does for its tabs.
    wxVector<wxBitmapBundle> pageIcons;
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_changes, 16 ) );
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_history, 16 ) );
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_branches, 16 ) );
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_remotes, 16 ) );
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_stashes, 16 ) );
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_tags, 16 ) );
    pageIcons.push_back( KiBitmapBundleDef( BITMAPS::git_repository, 16 ) );
    m_book->SetImages( pageIcons );

    m_book->AddPage( buildChangesPage( m_book ), _( "Changes" ), true, 0 );
    m_historyPage = new PANEL_GIT_HISTORY( m_book, this, m_parent );
    m_book->AddPage( m_historyPage, _( "History" ), false, 1 );

    m_branchesPage = new PANEL_GIT_BRANCHES( m_book, this, m_parent );
    m_book->AddPage( m_branchesPage, _( "Branches" ), false, 2 );

    m_remotesPage = new PANEL_GIT_REMOTES( m_book, this, m_parent );
    m_book->AddPage( m_remotesPage, _( "Remotes" ), false, 3 );

    m_stashesPage = new PANEL_GIT_STASHES( m_book, this, m_parent );
    m_book->AddPage( m_stashesPage, _( "Stashes" ), false, 4 );

    m_tagsPage = new PANEL_GIT_TAGS( m_book, this, m_parent );
    m_book->AddPage( m_tagsPage, _( "Tags" ), false, 5 );

    m_repositoryPage = new PANEL_GIT_REPOSITORY( m_book, this, m_parent );
    m_book->AddPage( m_repositoryPage, _( "Repository" ), false, 6 );

    // Pull and Push run through the docked panel, which owns the merge and its conflict handling.
    m_fetchButton->SetBitmap( KiBitmapBundle( BITMAPS::git_fetch ) );
    m_pullButton->SetBitmap( KiBitmapBundle( BITMAPS::git_pull ) );
    m_pushButton->SetBitmap( KiBitmapBundle( BITMAPS::git_push ) );

    m_fetchButton->Bind( wxEVT_BUTTON,
                         [this]( wxCommandEvent& )
                         {
                             onFetch();
                         } );
    // The choice is empty until the repository is read, so without a floor the top bar lays
    // it out at the width of nothing and the branch name arrives too late to widen it.
    m_branchChoice->SetMinSize( FromDIP( wxSize( 110, -1 ) ) );
    KIPLATFORM::UI::EllipsizeChoiceBox( m_branchChoice );

    m_pane->SetBusyChangedHandler(
            [this]( bool aBusy )
            {
                m_pullButton->Enable( !aBusy );
                m_pushButton->Enable( !aBusy );
            } );

    m_pullButton->Bind( wxEVT_BUTTON,
                        [this]( wxCommandEvent& )
                        {
                            m_pane->Pull();
                        } );
    m_pushButton->Bind( wxEVT_BUTTON,
                        [this]( wxCommandEvent& )
                        {
                            m_pane->Push();
                        } );

    m_branchChoice->Bind( wxEVT_CHOICE,
                          [this]( wxCommandEvent& )
                          {
                              SwitchToBranch( m_branchChoice->GetStringSelection() );

                              // A refused switch leaves the choice on a branch the tree is not
                              // on, so put it back on the real one.
                              ReloadPages();
                          } );

    SetupStandardButtons( { { wxID_CANCEL, _( "Close" ) } } );

    // Every box here is filled from the repository. DIALOG_SHIM would otherwise put back the
    // text a previous session left, keyed by where the control sits in the window, so a value
    // saved from one page reappears in whichever page now has a box in the same place.
    optOutOfTextPersistence( this );

    // The sidebar tree does not invalidate its best size when pages are added, so lay it
    // out and pin its width, matching how PAGED_DIALOG builds its navigation.
    m_book->GetTreeCtrl()->InvalidateBestSize();

    for( size_t i = 0; i < m_book->GetPageCount(); ++i )
        m_book->GetPage( i )->Layout();

    m_book->Layout();
    m_book->Fit();

    wxSize treeSize = m_book->GetTreeCtrl()->GetSize();
    treeSize.x += 20;
    m_book->GetTreeCtrl()->SetMinSize( treeSize );

    // A page is a bare list whose natural size is a few rows, so without a floor the first
    // open is a stamp. A saved size still wins once there is one.
    m_book->SetMinSize( FromDIP( wxSize( 800, 600 ) ) );
    m_book->Layout();

    Bind(
            wxEVT_BUTTON,
            [this]( wxCommandEvent& )
            {
                Close();
            },
            wxID_CANCEL );

    ReloadPages();

    // A commit, push or pull from any surface broadcasts a change, so the window never goes stale.
    syncListener();

    finishDialogSettings();
}


// One window per process. KiCad has one project open at a time and every frame shares it.
static GIT_ADVANCED_DIALOG* activeDialog()
{
    return static_cast<GIT_ADVANCED_DIALOG*>( KIGIT::ActiveAdvancedGitWindow() );
}


static void setActiveDialog( GIT_ADVANCED_DIALOG* aDialog )
{
    KIGIT::ActiveAdvancedGitWindow() = aDialog;
}


GIT_ADVANCED_DIALOG* GIT_ADVANCED_DIALOG::Active()
{
    return activeDialog();
}


void GIT_ADVANCED_DIALOG::ShowForFrame( EDA_BASE_FRAME* aParent, GIT_COMMIT_PANE* aPane )
{
    if( !activeDialog() )
    {
        GIT_ADVANCED_DIALOG* dialog = new GIT_ADVANCED_DIALOG( aParent, aPane );

        dialog->Bind( wxEVT_CLOSE_WINDOW,
                      [dialog]( wxCloseEvent& )
                      {
                          if( activeDialog() == dialog )
                              setActiveDialog( nullptr );

                          // Hiding through DIALOG_SHIM records the size for the next open. This
                          // handler takes the close event, so the base class's own save never
                          // runs.
                          dialog->Show( false );
                          dialog->Destroy();
                      } );

        setActiveDialog( dialog );
    }

    activeDialog()->Show( true );
    activeDialog()->Raise();
}


void GIT_ADVANCED_DIALOG::CloseActive()
{
    // Destroy() only schedules the delete, so the pointer is dropped here. Reopening before the
    // event loop gets round to it then builds a new window instead of raising a dying one.
    if( GIT_ADVANCED_DIALOG* dialog = activeDialog() )
    {
        setActiveDialog( nullptr );
        dialog->Destroy();
    }
}


void GIT_ADVANCED_DIALOG::CloseForPane( GIT_COMMIT_PANE* aPane )
{
    if( activeDialog() && activeDialog()->m_pane == aPane )
    {
        aPane->SetBusyChangedHandler( nullptr );
        aPane->SetMessageChangedHandler( nullptr );
        activeDialog()->m_pane = nullptr;
        CloseActive();
    }
}


GIT_ADVANCED_DIALOG::~GIT_ADVANCED_DIALOG()
{
    m_reloadQuery.Cancel();
    m_actionQuery.Cancel();
    m_fetchQuery.Cancel();

    // The panel usually outlives this window, so it must not be left calling into it. When the
    // panel went first it cleared this and dropped the pointer.
    if( m_pane )
    {
        m_pane->SetBusyChangedHandler( nullptr );
        m_pane->SetMessageChangedHandler( nullptr );
    }

    if( activeDialog() == this )
        setActiveDialog( nullptr );

    // A push or fetch outlives this window, and its completion refreshes the other windows, so
    // nothing has to be reported here. The transfers stop themselves as they go.
    m_branchesPage->StopTransfer();

    if( std::shared_ptr<KIGIT_COMMON> session = m_listenerSession.lock() )
        session->RemoveRefreshListener( this );
}


void GIT_ADVANCED_DIALOG::syncListener()
{
    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();

    if( m_listenerAttached )
    {
        // Pending reads belong to the original session just as much as the displayed rows do.
        // Never rebind this window and accept an old result into a new project's controls.
        if( m_listenerSession.lock() != common )
            SessionStillCurrent();

        return;
    }

    common->AddRefreshListener( this,
                                [this]()
                                {
                                    ReloadPages();
                                } );
    m_listenerSession = common;
    m_listenerAttached = true;
}


void GIT_ADVANCED_DIALOG::SetPaneTransferBusy( bool aBusy, const wxString& aStatus )
{
    // Two forms on one project, so both follow the transfer.
    if( m_changesPane )
        m_changesPane->SetTransferBusy( aBusy, aStatus );

    if( m_pane )
        m_pane->SetTransferBusy( aBusy, aStatus );
}


void GIT_ADVANCED_DIALOG::ShowTransferProgress( const wxString& aText )
{
    if( m_changesPane )
        m_changesPane->ShowTransferProgress( aText );
    else if( m_pane )
        m_pane->ShowTransferProgress( aText );
}


void GIT_ADVANCED_DIALOG::ReloadPages()
{
    syncListener();

    if( !SessionStillCurrent() )
        return;

    populateTopBar();
    m_historyPage->Populate();
    m_branchesPage->Populate();
    m_remotesPage->Populate();
    m_stashesPage->Populate();
    m_tagsPage->Populate();
    m_repositoryPage->Populate();
}


void GIT_ADVANCED_DIALOG::ShowHistoryForFile( EDA_BASE_FRAME* aParent, GIT_COMMIT_PANE* aPane, const wxString& aPath )
{
    ShowForFrame( aParent, aPane );
    activeDialog()->ShowHistoryForFile( aPath );
}


void GIT_ADVANCED_DIALOG::ShowHistoryForFile( const wxString& aPath )
{
    m_book->SetSelection( 1 );
    m_historyPage->ShowHistoryForFile( aPath );
}


void GIT_ADVANCED_DIALOG::ShowAmend( const wxString& aCommitHash )
{
    if( !SessionStillCurrent() )
        return;

    m_book->SetSelection( 0 );
    m_changesPane->BeginAmend( aCommitHash );
}


wxWindow* GIT_ADVANCED_DIALOG::buildChangesPage( wxWindow* aBook )
{
    wxPanel*    panel = new wxPanel( aBook );
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    // The commit controls above, and once a file is picked, its changes below.
    m_changesSplitter = new wxSplitterWindow( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                              wxSP_LIVE_UPDATE | wxSP_3DSASH );
    m_changesSplitter->SetMinimumPaneSize( panel->FromDIP( 120 ) );
    m_changesSplitter->SetSashGravity( 0.5 );
    m_changesSashPos = panel->FromDIP( 260 );

    m_changesPane = new GIT_COMMIT_PANE( m_changesSplitter, m_parent );
    m_workingDiff = new GIT_FILE_DIFF_AREA( m_changesSplitter, m_parent );

    m_changesSplitter->Initialize( m_changesPane );
    m_workingDiff->Hide();

    sizer->Add( m_changesSplitter, 1, wxEXPAND | wxALL, 4 );
    panel->SetSizer( sizer );

    // The docked panel and this page are two forms on one project, so the message starts from
    // what the panel already holds and goes back to it as the user types. This window closes
    // with the panel, so the panel is always there to write to.
    if( m_pane )
    {
        m_changesPane->SetCommitMessage( m_pane->GetCommitMessage() );

        m_changesPane->SetMessageChangedHandler(
                [this]()
                {
                    m_pane->SetCommitMessage( m_changesPane->GetCommitMessage() );
                } );

        // SetCommitMessage writes with ChangeValue, so neither form echoes back into the other.
        m_pane->SetMessageChangedHandler(
                [this]()
                {
                    m_changesPane->SetCommitMessage( m_pane->GetCommitMessage() );
                } );
    }

    m_changesPane->SetFileSelectedHandler(
            [this]( const wxString& aPath )
            {
                showWorkingDiff( aPath );
            } );

    return panel;
}


void GIT_ADVANCED_DIALOG::hideWorkingDiff()
{
    m_workingDiff->Clear();
    KIGIT::UnsplitKeepingSash( m_changesSplitter, m_workingDiff, m_changesSashPos );
}


void GIT_ADVANCED_DIALOG::showWorkingDiff( const wxString& aPath )
{
    if( aPath.IsEmpty() || !m_parent->Prj().GetGitCommon()->GetRepo() )
    {
        hideWorkingDiff();
        return;
    }

    // The top bar already names the branch the working tree is on.
    wxString branch = m_branchChoice->GetStringSelection();
    wxString baseLabel = branch.IsEmpty() ? _( "BASE (HEAD)" ) : wxString::Format( _( "BASE (%s)" ), branch );

    m_workingDiff->ShowFile(
            m_parent->Prj().GetGitCommon(),
            [path = aPath]( KIGIT_COMMON& aCommon, GIT_DIFF_WORKSPACE& aWorkspace, GIT_FILE_SIDES& aSides )
            {
                return MaterializeWorkingFile( aCommon, aWorkspace, path, aSides );
            },
            baseLabel, _( "WORKING (changes)" ) );

    if( !m_changesSplitter->IsSplit() )
        m_changesSplitter->SplitHorizontally( m_changesPane, m_workingDiff, m_changesSashPos );
}


void GIT_ADVANCED_DIALOG::SwitchToBranch( const wxString& aBranch )
{
    if( !SessionStillCurrent() )
        return;

    SwitchProjectToBranch( m_parent, this, aBranch, m_actionQuery );
}


void GIT_ADVANCED_DIALOG::populateTopBar()
{
    struct RESULT
    {
        wxString                       current;
        wxString                       root;
        std::vector<KIGIT_BRANCH_INFO> branches;
    };

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    auto                          result = std::make_shared<RESULT>();

    m_reloadQuery.Run(
            common,
            [result]( KIGIT_COMMON& aCommon )
            {
                result->current = aCommon.GetCurrentBranchName();
                result->root = aCommon.GetGitRootDirectory();
                result->branches = aCommon.GetBranches();
                KIGIT::SortBranchesForDisplay( result->branches );
            },
            [this, common, result]()
            {
                if( common != m_parent->Prj().GetGitCommon() || common->IsShuttingDown() )
                    return;

                wxFileName root = wxFileName::DirName( result->root );
                m_repoName->SetLabel( root.GetDirs().empty() ? root.GetFullPath() : root.GetDirs().Last() );
                m_branchChoice->Clear();

                // Only a local branch can be checked out here. Remote ones are on the Branches page.
                for( const KIGIT_BRANCH_INFO& branch : result->branches )
                {
                    if( !branch.isRemote )
                        m_branchChoice->Append( branch.name );
                }

                m_branchChoice->SetStringSelection( result->current );
                m_historyPage->SetBranches( result->branches );
                m_branchChoice->InvalidateBestSize();
                Layout();
            } );
}


void GIT_ADVANCED_DIALOG::onFetch( bool aRetriedAfterTrust )
{
    if( !SessionStillCurrent() )
        return;

    if( m_fetch.IsBusy() || m_fetchQuery.IsBusy() )
    {
        KIGIT::ShowBusyMessage( this );
        return;
    }

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    auto                          hasRemote = std::make_shared<bool>( false );

    m_fetchButton->SetLabel( _( "Fetching..." ) );
    m_fetchButton->Disable();
    SetPaneTransferBusy( true, _( "Fetching..." ) );
    Layout();

    m_fetchQuery.Run(
            common,
            [hasRemote]( KIGIT_COMMON& aCommon )
            {
                *hasRemote = aCommon.HasPushAndPullRemote();
            },
            [this, common, hasRemote, aRetriedAfterTrust]()
            {
                if( !SessionStillCurrent() )
                    return;

                if( !*hasRemote )
                {
                    m_fetchButton->SetLabel( _( "Fetch" ) );
                    m_fetchButton->Enable();
                    Layout();
                    DisplayInfoMessage( this, _( "No remote is configured to get changes from." ) );
                    return;
                }

                startFetch( common, aRetriedAfterTrust );
            } );
}


void GIT_ADVANCED_DIALOG::startFetch( std::shared_ptr<KIGIT_COMMON> aCommon, bool aRetriedAfterTrust )
{
    m_fetchButton->SetLabel( _( "Fetching..." ) );
    m_fetchButton->Disable();
    Layout();

    // Carried rather than captured by reference, because Start returns before the work runs.
    struct OUTCOME
    {
        bool     fetched = false;
        wxString error;
        wxString details;
        bool     authFailure = false;
        bool     cancelled = false;

        std::optional<KIGIT::HOST_KEY_FAILURE> hostKey;
    };

    auto outcome = std::make_shared<OUTCOME>();

    bool started = m_fetch.Start(
            "git fetch",
            [aCommon, outcome, alive = m_fetch.Alive(), dialog = this]( const KIGIT_COMMON::CANCEL_TOKEN& aToken )
            {
                GIT_PULL_HANDLER handler( aCommon.get() );
                handler.SetProgressReporter( std::make_unique<KIGIT_STATUS_REPORTER>(
                        alive,
                        [dialog]( const wxString& aText )
                        {
                            dialog->ShowTransferProgress( aText );
                        } ) );
                handler.SetCancelToken( aToken );
                handler.SetWaitForActionLock( true );
                outcome->fetched = handler.PerformFetch();
                KIGIT_ERROR_MESSAGE error = handler.PopError();
                outcome->error = error.summary;
                outcome->details = error.details;
                outcome->authFailure = handler.WasAuthFailure();
                outcome->hostKey = handler.GetHostKeyFailure();

                // This operation's own token, so a fetch that never became active cannot
                // consume a cancel belonging to the transfer that is running.
                outcome->cancelled = handler.IsThisOperationCancelled();

                // The other surfaces need this even when the window that started it has gone.
                if( outcome->fetched && !outcome->cancelled )
                {
                    KIGIT::CallOnUi(
                            [aCommon]()
                            {
                                aCommon->NotifyRefreshListeners();
                            } );
                }
            },
            [this, aCommon, outcome, aRetriedAfterTrust]()
            {
                if( !SessionStillCurrent() )
                    return;

                m_fetchButton->SetLabel( _( "Fetch" ) );
                m_fetchButton->Enable();
                SetPaneTransferBusy( false );
                Layout();

                if( outcome->fetched && !outcome->cancelled )
                    KIGIT::ClearSyncWarnings( aCommon.get() );

                if( outcome->cancelled || outcome->fetched )
                    return;

                if( outcome->hostKey )
                {
                    if( KIGIT::PromptToTrustHost( this, *outcome->hostKey, aRetriedAfterTrust ) )
                        onFetch( true );

                    return;
                }

                if( outcome->authFailure
                    && KIGIT::PromptForCredentials( this, aCommon.get(), &m_parent->Prj(), outcome->error ) )
                {
                    onFetch( aRetriedAfterTrust );
                    return;
                }

                wxString error = outcome->error;

                if( error.IsEmpty() )
                    error = KIGIT::UnexplainedFailure( KIGIT_OPERATION::FETCH, aCommon->GetRemote() );

                DisplayErrorMessage( this, error, outcome->details );
            } );

    if( !started )
    {
        m_fetchButton->SetLabel( _( "Fetch" ) );
        m_fetchButton->Enable();
        SetPaneTransferBusy( false );
        Layout();
        DisplayInfoMessage( this, _( "Could not start the fetch." ) );
    }
}


void GIT_ADVANCED_DIALOG::RefuseOverUncommittedWork( std::function<void( const GIT_WORKTREE_STATUS& )> aReady )
{
    if( !SessionStillCurrent() )
        return;

    ProjectClearOfUncommittedWork( m_parent, this, m_actionQuery,
                                   [this, aReady]( const GIT_WORKTREE_STATUS& aStatus )
                                   {
                                       if( SessionStillCurrent() )
                                           aReady( aStatus );
                                   } );
}


bool GIT_ADVANCED_DIALOG::ConfirmWorkingTreeChange()
{
    if( !SessionStillCurrent() )
        return false;

    // Save rather than close: this window is parented to the frame hosting the panel, so a close
    // would destroy that frame and this dialog with it, mid-call.
    if( !PrepareEditorsForGitOperation( m_parent, GIT_EDITOR_PREP::WORKING_TREE ) )
        return false;

    if( !SessionStillCurrent() )
        return false;

    m_docsBeforeChange = CaptureOpenEditorDocs( m_parent->Kiway() );
    return true;
}


bool GIT_ADVANCED_DIALOG::ConfirmWorkingTreeChange( const GIT_WORKTREE_STATUS& aStatus )
{
    if( !SessionStillCurrent() || !ProjectSafeToRewrite( m_parent, this, aStatus ) )
        return false;

    m_docsBeforeChange = CaptureOpenEditorDocs( m_parent->Kiway() );
    return true;
}


bool GIT_ADVANCED_DIALOG::SessionStillCurrent()
{
    if( IsBeingDeleted() || m_parent->IsBeingDeleted() )
        return false;

    std::shared_ptr<KIGIT_COMMON> session = m_listenerSession.lock();

    if( m_listenerAttached && session && !session->IsShuttingDown() && session == m_parent->Prj().GetGitCommon()
        && Pgm().GetCommonSettings()->m_Git.enableGit && !m_parent->Prj().GetLocalSettings().m_GitIntegrationDisabled )
    {
        return true;
    }

    // A window and its pending reads belong to the session it opened with. Once it changes,
    // close rather than act on identically named branches or files in another project.
    m_reloadQuery.Cancel();
    m_actionQuery.Cancel();
    m_fetchQuery.Cancel();
    Close();
    return false;
}


void GIT_ADVANCED_DIALOG::RunGitAction( const std::function<bool( KIGIT_COMMON& )>& aFn, const wxString& aError,
                                        bool aRewritesWorkingTree )
{
    RunGitAction(
            [&aFn]( KIGIT_COMMON& aCommon, wxString& )
            {
                return aFn( aCommon );
            },
            aError, aRewritesWorkingTree );
}


void GIT_ADVANCED_DIALOG::RunGitAction( const std::function<bool( KIGIT_COMMON&, wxString& )>& aFn,
                                        const wxString& aError, bool aRewritesWorkingTree )
{
    if( !SessionStillCurrent() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_parent->Prj().GetGitCommon();
    bool                          ok = false;
    wxString                      detail;

    {
        // Release the lock before reloading editors and refreshing other Git views.
        std::unique_lock<std::mutex> lock = KIGIT::WaitForActionLock( *common, this );

        if( !lock.owns_lock() )
            return;

        ok = aFn( *common, detail );
    }

    if( !ok )
    {
        // A failed action can still have written files, so the editors and the panels are
        // brought up to date first. Nothing reloads when nothing changed on disk.
        if( aRewritesWorkingTree )
        {
            ReloadOpenEditors( m_parent->Kiway(), m_docsBeforeChange );
            common->NotifyRefreshListeners();
        }

        DisplayErrorMessage( this, aError, detail.IsEmpty() ? KIGIT_COMMON::GetLastGitError() : detail );
        return;
    }

    if( aRewritesWorkingTree )
    {
        // The files the editors have open were just rewritten, so pull them back in from disk.
        ReloadOpenEditors( m_parent->Kiway(), m_docsBeforeChange );
    }

    common->NotifyRefreshListeners();
}


