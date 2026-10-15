/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/git/panel_git_branches.h>

#include <mutex>
#include <vector>

#include <wx/richmsgdlg.h>
#include <wx/textdlg.h>

#include <confirm.h>
#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <dialogs/git/git_ui_utils.h>
#include <eda_base_frame.h>
#include <git/git_push_handler.h>
#include <git/kicad_git_common.h>
#include <project.h>


PANEL_GIT_BRANCHES::PANEL_GIT_BRANCHES( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame ) :
        PANEL_GIT_BRANCHES_BASE( aParent ),
        m_dialog( aDialog ),
        m_frame( aFrame )
{
    m_branchSearch->SetDescriptiveText( _( "Search branches" ) );

    m_branchesList->AppendColumn( _( "Branch" ), wxLIST_FORMAT_LEFT, FromDIP( 220 ) );
    m_branchesList->AppendColumn( _( "Status" ), wxLIST_FORMAT_LEFT, FromDIP( 70 ) );
    m_branchesList->AppendColumn( _( "Commit" ), wxLIST_FORMAT_LEFT, FromDIP( 100 ) );
    m_branchesList->AppendColumn( _( "Date" ), wxLIST_FORMAT_LEFT, FromDIP( 90 ) );

    KIGIT::MarkDestructive( m_deleteBranchButton );
    KIGIT::MarkDestructive( m_forcePushButton );

    // The detail pane appears once a branch is selected.
    m_branchSashPos = FromDIP( 460 );
    m_branchesSplitter->Unsplit( m_branchDetail );

    m_retryTimer.SetOwner( this );
    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                reloadBranches();
            },
            m_retryTimer.GetId() );

    m_branchSearch->Bind( wxEVT_TEXT,
                          [this]( wxCommandEvent& )
                          {
                              reloadBranches();
                          } );

    m_branchesList->Bind( wxEVT_LIST_ITEM_SELECTED,
                          [this]( wxListEvent& aEvent )
                          {
                              if( !m_fillingBranches )
                                  showBranchDetail( m_branchesList->GetItemText( aEvent.GetIndex(), 0 ) );
                          } );

    // Double-clicking a branch switches the working tree to it.
    m_branchesList->SetToolTip( _( "Double click a branch to switch the project to it." ) );
    m_branchesList->Bind( wxEVT_LIST_ITEM_ACTIVATED,
                          [this]( wxListEvent& aEvent )
                          {
                              m_dialog->SwitchToBranch( m_branchesList->GetItemText( aEvent.GetIndex(), 0 ) );
                          } );

    m_newBranchButton->Bind( wxEVT_BUTTON,
                             [this]( wxCommandEvent& )
                             {
                                 onNewBranch();
                             } );
    m_deleteBranchButton->Bind( wxEVT_BUTTON,
                                [this]( wxCommandEvent& )
                                {
                                    onDeleteBranch();
                                } );
    m_forcePushButton->Bind( wxEVT_BUTTON,
                             [this]( wxCommandEvent& )
                             {
                                 onForcePush();
                             } );
}


PANEL_GIT_BRANCHES::~PANEL_GIT_BRANCHES()
{
    StopTransfer();
}


bool PANEL_GIT_BRANCHES::StopTransfer()
{
    bool started = m_pushWorker.joinable();

    if( m_pushCancel )
        m_pushCancel->store( true );

    if( m_pushWorker.joinable() )
        m_pushWorker.join();

    return started;
}


void PANEL_GIT_BRANCHES::Populate()
{
    // The caller holds the git action lock.
    std::shared_ptr<KIGIT_COMMON>  common = m_frame->Prj().GetGitCommon();
    std::vector<KIGIT_BRANCH_INFO> branches = common->GetBranches();
    wxString                       search = m_branchSearch->GetValue().Lower();
    wxString                       selected = KIGIT::SelectedText( m_branchesList, 0 );
    wxString                       current;
    long                           selectRow = wxNOT_FOUND;

    m_fillingBranches = true;
    m_branchesList->DeleteAllItems();

    for( const KIGIT_BRANCH_INFO& branch : branches )
    {
        if( branch.isCurrent )
            current = branch.name;

        if( !search.IsEmpty() && !branch.name.Lower().Contains( search ) )
            continue;

        long idx = m_branchesList->InsertItem( m_branchesList->GetItemCount(), branch.name );

        if( branch.isCurrent )
            m_branchesList->SetItem( idx, 1, _( "current" ) );
        else if( branch.isRemote )
            m_branchesList->SetItem( idx, 1, _( "remote" ) );

        m_branchesList->SetItem( idx, 2, branch.tipHash );
        m_branchesList->SetItem( idx, 3, KIGIT::RelativeDate( branch.when ) );

        // Keep the selection where it was, or start on the current branch.
        if( branch.name == selected || ( selected.IsEmpty() && branch.isCurrent ) )
            selectRow = idx;
    }

    int shown = m_branchesList->GetItemCount();
    m_branchCount->SetLabel( shown == 1 ? _( "1 branch" ) : wxString::Format( _( "%d branches" ), shown ) );

    if( selectRow != wxNOT_FOUND )
    {
        m_branchesList->SetItemState( selectRow, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
        fillBranchDetail( common->GetBranchDetail( m_branchesList->GetItemText( selectRow, 0 ) ) );
    }
    else
    {
        hideBranchDetail();
    }

    m_fillingBranches = false;
}


void PANEL_GIT_BRANCHES::reloadBranches()
{
    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    std::unique_lock<std::mutex>  lock = common->TryLockAction();

    if( !lock.owns_lock() )
    {
        m_retryTimer.Start( 1000, wxTIMER_ONE_SHOT );
        return;
    }

    Populate();
}


void PANEL_GIT_BRANCHES::showBranchDetail( const wxString& aBranch )
{
    if( aBranch.IsEmpty() )
    {
        hideBranchDetail();
        return;
    }

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    std::unique_lock<std::mutex>  lock = common->TryLockAction();

    // The reload keeps the selection and fills the pane then.
    if( !lock.owns_lock() )
    {
        m_retryTimer.Start( 1000, wxTIMER_ONE_SHOT );
        return;
    }

    fillBranchDetail( common->GetBranchDetail( aBranch ) );
}


void PANEL_GIT_BRANCHES::fillBranchDetail( const KIGIT_BRANCH_DETAIL& aDetail )
{
    if( !aDetail.found )
    {
        hideBranchDetail();
        return;
    }

    m_branchTitle->SetLabel( wxString::Format( _( "Branch: %s" ), aDetail.name ) );
    m_branchCommit->SetLabel(
            wxString::Format( _( "Commit: %s, %s" ), aDetail.tipHash, KIGIT::RelativeDate( aDetail.when ) ) );
    m_branchAuthor->SetLabel( wxString::Format( _( "Author: %s <%s>" ), aDetail.author, aDetail.email ) );
    m_branchMessage->ChangeValue( aDetail.message );

    wxString sync;

    if( aDetail.upstream.IsEmpty() )
        sync = _( "Not tracking a remote branch." );
    else if( aDetail.ahead == 0 && aDetail.behind == 0 )
        sync = wxString::Format( _( "Up to date with %s." ), aDetail.upstream );
    else if( aDetail.behind == 0 )
        sync = wxString::Format( _( "%zu commits ahead of %s." ), aDetail.ahead, aDetail.upstream );
    else if( aDetail.ahead == 0 )
        sync = wxString::Format( _( "%zu commits behind %s." ), aDetail.behind, aDetail.upstream );
    else
        sync = wxString::Format( _( "%zu commits ahead and %zu behind %s." ), aDetail.ahead, aDetail.behind,
                                 aDetail.upstream );

    m_branchSync->SetLabel( sync );

    if( !m_branchesSplitter->IsSplit() )
        m_branchesSplitter->SplitVertically( m_branchListPanel, m_branchDetail, m_branchSashPos );

    m_branchDetail->Layout();
}


void PANEL_GIT_BRANCHES::hideBranchDetail()
{
    KIGIT::UnsplitKeepingSash( m_branchesSplitter, m_branchDetail, m_branchSashPos );
}


void PANEL_GIT_BRANCHES::onNewBranch()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    // The new branch starts where the selected one is, so a branch can be made off any other.
    wxString from = KIGIT::SelectedText( m_branchesList, 0 );
    wxString prompt =
            from.IsEmpty() ? _( "New branch name:" ) : wxString::Format( _( "New branch from '%s', name:" ), from );
    wxString name = wxGetTextFromUser( prompt, _( "New Branch" ), wxEmptyString, this );

    if( name.IsEmpty() )
        return;

    m_dialog->RunGitAction(
            [name, from]( KIGIT_COMMON& aCommon )
            {
                return aCommon.CreateBranch( name, from );
            },
            wxString::Format( _( "Could not create branch '%s'." ), name ) );
}


void PANEL_GIT_BRANCHES::onDeleteBranch()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    wxString name = KIGIT::SelectedText( m_branchesList, 0 );

    if( name.IsEmpty() )
        return;

    // Deleting a remote-tracking branch would mean deleting it on the server, and the lookup
    // below could otherwise hit a local branch of the same name.
    if( KIGIT::SelectedText( m_branchesList, 1 ) == _( "remote" ) )
    {
        DisplayInfoMessage( this, _( "This branch lives on the remote, so it cannot be deleted from here." ) );
        return;
    }

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    wxString                      current;
    size_t                        unmerged = 0;

    // Reading the current branch touches the repository, so do it under the lock.
    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, KIGIT::BusyMessage() );
            return;
        }

        current = common->GetCurrentBranchName();
        unmerged = common->GetUnmergedCommitCount( name );
    }

    if( name == current )
    {
        DisplayErrorMessage( this, _( "You cannot delete the branch you are on." ) );
        return;
    }

    // Deleting drops any commit no other branch reaches, and git offers no way back a
    // beginner would find, so say what is at stake before asking.
    wxString question;

    if( unmerged == 1 )
    {
        question = wxString::Format( _( "Branch '%s' has one commit that is not on the current branch. "
                                        "Deleting it loses that commit.\n\nDelete it anyway?" ),
                                     name );
    }
    else if( unmerged > 1 )
    {
        question = wxString::Format( _( "Branch '%s' has %zu commits that are not on the current branch. "
                                        "Deleting it loses them.\n\nDelete it anyway?" ),
                                     name, unmerged );
    }
    else
    {
        question = wxString::Format( _( "Delete branch '%s'?" ), name );
    }

    if( !IsOK( this, question ) )
        return;

    m_dialog->RunGitAction(
            [name]( KIGIT_COMMON& aCommon )
            {
                return aCommon.DeleteBranch( name );
            },
            wxString::Format( _( "Could not delete branch '%s'." ), name ) );
}


void PANEL_GIT_BRANCHES::onForcePush()
{
    if( m_pushWorker.joinable() )
    {
        DisplayInfoMessage( this, _( "A transfer is already running." ) );
        return;
    }

    if( !m_dialog->SessionStillCurrent() )
        return;

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    if( !common->GetRepo() )
        return;

    wxString branch;
    wxString upstream;
    bool     hasRemote = false;
    bool     detached = false;

    {
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            DisplayInfoMessage( this, KIGIT::BusyMessage() );
            return;
        }

        hasRemote = common->HasPushAndPullRemote();
        detached = common->IsHeadDetached();
        branch = common->GetCurrentBranchName();

        // Push sends the branch to its same-named ref on the remote, regardless of any
        // differently named upstream, so the warning must name that actual target.
        upstream = common->GetRemoteNameOrDefault() + wxS( "/" ) + branch;
    }

    if( detached )
    {
        DisplayInfoMessage( this, _( "This project is showing an old snapshot rather than a branch, so there "
                                     "is no branch to push. Double click a branch to go back to it first." ) );
        return;
    }

    if( !hasRemote )
    {
        DisplayInfoMessage( this, _( "No remote is configured to push to." ) );
        return;
    }

    wxRichMessageDialog confirm( this,
                                 wxString::Format( _( "Force push replaces '%s' with your local '%s'.\n\n"
                                                      "Commits that exist only on the remote will be lost." ),
                                                   upstream, branch ),
                                 _( "Force Push" ), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING | wxCENTER );
    confirm.SetYesNoLabels( _( "Force &Push" ), _( "Cancel" ) );

    if( confirm.ShowModal() != wxID_YES )
        return;

    // Never the shared compute pool. A libgit2 network read parked there stalls DRC, zone
    // filling and library loading.
    KIGIT_COMMON::CANCEL_TOKEN token = KIGIT_COMMON::NewCancelToken();
    m_pushCancel = token;

    if( m_pushWorker.joinable() )
        m_pushWorker.join();

    m_pushWorker = std::thread(
            [this, common, token]()
            {
                common->ClearAuthFailure();

                GIT_PUSH_HANDLER handler( common.get() );
                handler.SetCancelToken( token );
                PushResult result = handler.PerformPush( true );
                wxString   error = handler.GetErrorString();
                bool       authFailure = common->WasAuthFailure();

                // This operation's own token, so a force push that never became active cannot
                // consume a cancel belonging to the transfer that is running.
                bool cancelled = handler.IsThisOperationCancelled();

                CallAfter(
                        [this, common, result, error, authFailure, cancelled]()
                        {
                            if( cancelled )
                                return;

                            if( result == PushResult::Success )
                            {
                                common->NotifyRefreshListeners();
                                return;
                            }

                            if( result == PushResult::Busy )
                            {
                                DisplayInfoMessage( this, KIGIT::BusyMessage() );
                                return;
                            }

                            if( authFailure && KIGIT::PromptForCredentials( this, common.get(), &m_frame->Prj() ) )
                            {
                                onForcePush();
                                return;
                            }

                            DisplayErrorMessage( this, _( "Force push failed." ), error );
                        } );
            } );
}
