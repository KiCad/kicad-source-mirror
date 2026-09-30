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

#include <algorithm>
#include <memory>

#include <wx/button.h>
#include <wx/dataview.h>
#include <wx/textctrl.h>

#include <git/kicad_git_common.h>
#include <eda_base_frame.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/common_settings.h>
#include <widgets/file_checklist_ctrl.h>


GIT_COMMIT_PANE::GIT_COMMIT_PANE( EDA_BASE_FRAME* aParent ) :
        GIT_COMMIT_PANE_BASE( aParent ),
        m_frame( aParent ),
        m_hasUnpushedCommits( false ),
        m_hasRemote( false ),
        m_headDetached( false ),
        m_operationInProgress( false ),
        m_editorsDirty( false ),
        m_networkBusy( false )
{
    m_message->SetHint( _( "Summarize your changes" ) );

    m_advancedButton->Bind( wxEVT_BUTTON, &GIT_COMMIT_PANE::onOpenAdvanced, this );
    m_message->Bind( wxEVT_TEXT, &GIT_COMMIT_PANE::onMessageChanged, this );
    m_checklist->Bind( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &GIT_COMMIT_PANE::onChecklistChanged, this );
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

    RefreshFileList();
}


GIT_COMMIT_PANE::~GIT_COMMIT_PANE()
{
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
    // network round trip without broadcasting a refresh.
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

    // Without a remote there is nowhere to push to, so local commits are not waiting for it.
    m_hasUnpushedCommits = hasLocalCommits && m_hasRemote;

    lock.unlock();

    // Edits still only in an editor's memory are not in the git status yet, but the commit flow
    // saves the editors first, so they are committable and the controls must allow it.
    m_editorsDirty = common->GetRepo() && anyEditorDirty();

    // Amending is offered only while the last commit has not been pushed, so an already
    // published commit can never be rewritten from here.
    bool canAmend = hasLocalCommits && !m_headDetached && !m_operationInProgress;

    m_amendCheckbox->Show( canAmend );

    if( !canAmend )
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

    bool hasChanges = !entries.empty();

    m_checklist->Show( hasChanges );

    // The message field also has to be there for a message-only amend and for committing
    // unsaved editor changes, when the list itself is empty.
    updateMessageVisibility( hasChanges );

    if( m_operationInProgress )
    {
        setStatusText( _( "This project is in the middle of a merge or rebase that was started outside "
                          "KiCad or interrupted. Abandon it to go back to the last commit, or finish it "
                          "with the program that started it." ) );
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
        else if( m_hasUnpushedCommits )
            setStatusText( _( "Committed changes are waiting to be pushed." ) );
        else
            setStatusText( _( "No changes" ) );

        m_emptyLabel->Show( true );
    }

    updateCommitButton();
    Layout();
}


void GIT_COMMIT_PANE::showDisabledState()
{
    m_hasUnpushedCommits = false;
    m_hasRemote = false;
    m_headDetached = false;
    m_operationInProgress = false;
    m_editorsDirty = false;

    m_shownPaths.clear();
    m_checklist->SetEntries( {} );
    m_checklist->Show( false );

    m_amendCheckbox->SetValue( false );
    m_amendCheckbox->Show( false );
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
    GIT_ADVANCED_DIALOG::ShowForFrame( m_frame );
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
    // share this, so the field cannot disappear while the buttons still ask for a message.
    m_message->Show( aHasChanges || m_editorsDirty || ( m_amendCheckbox->IsShown() && m_amendCheckbox->IsChecked() ) );
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
        m_pullButton->SetLabel( _( "Abandon Merge" ) );
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

    bool hasCheckedChanges = !m_checklist->GetCheckedPaths().empty();
    bool amend = m_amendCheckbox->IsShown() && m_amendCheckbox->IsChecked();
    bool hasMessage = !m_message->GetValue().Trim().IsEmpty();

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
        m_commitButton->Enable( hasMessage );
        m_commitOnlyButton->SetLabel( _( "Commit" ) );
        m_commitOnlyButton->Enable( hasMessage );
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


void GIT_COMMIT_PANE::onMessageChanged( wxCommandEvent& aEvent )
{
    updateCommitButton();
}


void GIT_COMMIT_PANE::onChecklistChanged( wxDataViewEvent& aEvent )
{
    updateCommitButton();
}


void GIT_COMMIT_PANE::setBusy( bool aBusy, const wxString& aStatus )
{
    m_networkBusy = aBusy;

    m_commitButton->Enable( !aBusy );
    m_commitOnlyButton->Enable( !aBusy );
    m_amendCheckbox->Enable( !aBusy );
    m_message->Enable( !aBusy );
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
