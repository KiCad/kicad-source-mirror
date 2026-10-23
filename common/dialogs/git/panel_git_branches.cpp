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

#include <memory>
#include <vector>

#include <wx/richmsgdlg.h>
#include <wx/textdlg.h>

#include <confirm.h>
#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <dialogs/git/git_transfer.h>
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

    m_branchesList->Bind( wxEVT_LIST_ITEM_DESELECTED,
                          [this]( wxListEvent& )
                          {
                              if( !m_fillingBranches
                                  && m_branchesList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ) == -1 )
                              {
                                  hideBranchDetail();
                              }
                          } );

    // Double-clicking a branch switches the working tree to it.
    m_branchesList->SetToolTip( _( "Double click a local branch to switch the project to it." ) );
    m_branchesList->Bind( wxEVT_LIST_ITEM_ACTIVATED,
                          [this]( wxListEvent& aEvent )
                          {
                              int row = aEvent.GetIndex();

                              // A remote branch is someone else's work to start from, not
                              // somewhere to stand, so New Branch is the way to take it up.
                              if( row < static_cast<int>( m_branchIsRemote.size() ) && m_branchIsRemote[row] )
                              {
                                  DisplayInfoMessage( this, _( "This branch is on the remote. Select it and "
                                                               "press New Branch to start your own branch "
                                                               "from it." ) );
                                  return;
                              }

                              m_dialog->SwitchToBranch( m_branchesList->GetItemText( row, 0 ) );
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


void PANEL_GIT_BRANCHES::StopTransfer()
{
    m_push.Cancel();
}


void PANEL_GIT_BRANCHES::Populate()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    m_detailQuery.Cancel();

    auto branches = std::make_shared<std::vector<KIGIT_BRANCH_INFO>>();

    m_listQuery.Run(
            m_frame->Prj().GetGitCommon(),
            [branches]( KIGIT_COMMON& aCommon )
            {
                *branches = aCommon.GetBranches();
                KIGIT::SortBranchesForDisplay( *branches );
            },
            [this, branches]()
            {
                if( !m_dialog->SessionStillCurrent() )
                    return;

                // Preserve selections and search edits made while the repository was being read.
                wxString search = m_branchSearch->GetValue().Lower();
                wxString selected = KIGIT::SelectedText( m_branchesList, 0 );
                long     selectRow = wxNOT_FOUND;

                m_detailQuery.Cancel();
                m_fillingBranches = true;
                m_branchesList->DeleteAllItems();
                m_branchIsRemote.clear();

                for( const KIGIT_BRANCH_INFO& branch : *branches )
                {
                    if( !search.IsEmpty() && !branch.name.Lower().Contains( search ) )
                        continue;

                    long idx = m_branchesList->InsertItem( m_branchesList->GetItemCount(), branch.name );

                    if( branch.isCurrent )
                        m_branchesList->SetItem( idx, 1, _( "current" ) );
                    else if( branch.isRemote )
                        m_branchesList->SetItem( idx, 1, _( "remote" ) );

                    m_branchesList->SetItem( idx, 2, branch.tipHash );
                    m_branchesList->SetItem( idx, 3, KIGIT::RelativeDate( branch.when ) );
                    m_branchIsRemote.push_back( branch.isRemote );

                    // Keep the selection where it was, or start on the current branch.
                    if( branch.name == selected || ( selected.IsEmpty() && branch.isCurrent ) )
                        selectRow = idx;
                }

                int shown = m_branchesList->GetItemCount();
                m_branchCount->SetLabel( wxString::Format( wxPLURAL( "%d branch", "%d branches", shown ), shown ) );

                if( selectRow != wxNOT_FOUND )
                {
                    m_branchesList->SetItemState( selectRow, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
                    showBranchDetail( m_branchesList->GetItemText( selectRow, 0 ) );
                }
                else
                {
                    hideBranchDetail();
                }

                m_fillingBranches = false;
            } );
}


void PANEL_GIT_BRANCHES::reloadBranches()
{
    Populate();
}


void PANEL_GIT_BRANCHES::showBranchDetail( const wxString& aBranch )
{
    hideBranchDetail();

    if( aBranch.IsEmpty() || !m_dialog->SessionStillCurrent() )
        return;

    auto detail = std::make_shared<KIGIT_BRANCH_DETAIL>();

    m_detailQuery.Run(
            m_frame->Prj().GetGitCommon(),
            [detail, aBranch]( KIGIT_COMMON& aCommon )
            {
                *detail = aCommon.GetBranchDetail( aBranch );
            },
            [this, detail, aBranch]()
            {
                if( m_dialog->SessionStillCurrent() && KIGIT::SelectedText( m_branchesList, 0 ) == aBranch )
                {
                    fillBranchDetail( *detail );
                }
            } );
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
        sync = _( "Not tracking another branch." );
    else if( !aDetail.divergence )
        sync = wxString::Format( _( "Could not compare with %s." ), aDetail.upstream );
    else
    {
        const KIGIT_AHEAD_BEHIND& counts = *aDetail.divergence;

        if( counts.ahead == 0 && counts.behind == 0 )
            sync = wxString::Format( _( "Up to date with %s." ), aDetail.upstream );
        else if( counts.behind == 0 )
            sync = wxString::Format( wxPLURAL( "%zu commit ahead of %s.", "%zu commits ahead of %s.", counts.ahead ),
                                     counts.ahead, aDetail.upstream );
        else if( counts.ahead == 0 )
            sync = wxString::Format( wxPLURAL( "%zu commit behind %s.", "%zu commits behind %s.", counts.behind ),
                                     counts.behind, aDetail.upstream );
        else
            sync = wxString::Format( wxPLURAL( "%zu commit ahead and %zu behind %s.",
                                               "%zu commits ahead and %zu behind %s.", counts.ahead ),
                                     counts.ahead, counts.behind, aDetail.upstream );
    }

    m_branchSync->SetLabel( sync );

    if( !m_branchesSplitter->IsSplit() )
        m_branchesSplitter->SplitVertically( m_branchListPanel, m_branchDetail, m_branchSashPos );

    m_branchDetail->Layout();
}


void PANEL_GIT_BRANCHES::hideBranchDetail()
{
    m_detailQuery.Cancel();
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
    wxString name;
    wxString nameError;

    do
    {
        if( !nameError.IsEmpty() )
            DisplayErrorMessage( this, nameError );

        name = wxGetTextFromUser( prompt, _( "New Branch" ), name, this );

        if( name.IsEmpty() || !m_dialog->SessionStillCurrent() )
            return;

        nameError = KIGIT::BranchNameError( name );
    } while( !nameError.IsEmpty() );

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

    struct OUTCOME
    {
        wxString              current;
        std::optional<size_t> unmerged;
        bool                  checked = false;
    };

    auto outcome = std::make_shared<OUTCOME>();

    m_actionQuery.Run(
            m_frame->Prj().GetGitCommon(),
            [outcome, name]( KIGIT_COMMON& aCommon )
            {
                outcome->current = aCommon.GetCurrentBranchName();
                outcome->unmerged = aCommon.GetUnmergedCommitCount( name );
                outcome->checked = true;
            },
            [this, outcome, name]()
            {
                if( !m_dialog->SessionStillCurrent() || !outcome->checked )
                    return;

                if( name == outcome->current )
                {
                    DisplayErrorMessage( this, _( "You cannot delete the branch you are on." ) );
                    return;
                }

                if( !outcome->unmerged )
                {
                    DisplayErrorMessage( this, _( "Could not compare this branch with the current version." ) );
                    return;
                }

                wxString question;

                if( *outcome->unmerged == 1 )
                {
                    question = wxString::Format( _( "Branch '%s' has one commit that is not in the current version."
                                                    "\n\nDelete it anyway?" ),
                                                 name );
                }
                else if( *outcome->unmerged > 1 )
                {
                    question = wxString::Format( _( "Branch '%s' has %zu commits that are not in the current version."
                                                    "\n\nDelete it anyway?" ),
                                                 name, *outcome->unmerged );
                }
                else
                {
                    question = wxString::Format( _( "Delete branch '%s'?" ), name );
                }

                wxRichMessageDialog confirm( this, question, _( "Delete Branch" ),
                                             wxYES_NO | wxNO_DEFAULT | wxICON_WARNING | wxCENTER );
                confirm.SetYesNoLabels( _( "&Delete" ), _( "Cancel" ) );

                if( confirm.ShowModal() != wxID_YES || !m_dialog->SessionStillCurrent() )
                    return;

                m_dialog->RunGitAction(
                        [name]( KIGIT_COMMON& aCommon )
                        {
                            return aCommon.DeleteBranch( name );
                        },
                        wxString::Format( _( "Could not delete branch '%s'." ), name ) );
            } );
}


void PANEL_GIT_BRANCHES::onForcePush( bool aRetriedAfterTrust )
{
    if( m_push.IsBusy() )
    {
        KIGIT::ShowBusyMessage( this );
        return;
    }

    if( !m_dialog->SessionStillCurrent() )
        return;

    struct OUTCOME
    {
        wxString branch;
        wxString headOid;
        wxString upstream;
        bool     hasRemote = false;
        bool     detached = false;
        bool     checked = false;
    };

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    auto                          outcome = std::make_shared<OUTCOME>();

    m_actionQuery.Run(
            common,
            [outcome]( KIGIT_COMMON& aCommon )
            {
                outcome->hasRemote = aCommon.HasPushAndPullRemote();
                outcome->detached = aCommon.IsHeadDetached();
                outcome->branch = aCommon.GetCurrentBranchName();
                outcome->headOid = aCommon.GetHeadCommitId();

                // Push targets the same-named remote ref, even with a differently named upstream.
                outcome->upstream = aCommon.GetRemoteNameOrDefault() + wxS( "/" ) + outcome->branch;
                outcome->checked = true;
            },
            [this, common, outcome, aRetriedAfterTrust]()
            {
                if( !m_dialog->SessionStillCurrent() || !outcome->checked || m_push.IsBusy() )
                    return;

                if( outcome->detached )
                {
                    DisplayInfoMessage( this,
                                        _( "This project is showing an old snapshot rather than a branch, so there "
                                           "is no branch to push. Double click a branch to go back to it first." ) );
                    return;
                }

                if( !outcome->hasRemote )
                {
                    DisplayInfoMessage( this, _( "No remote is configured to push to." ) );
                    return;
                }

                wxRichMessageDialog confirm(
                        this,
                        wxString::Format( _( "Force push replaces '%s' with your local '%s'.\n\n"
                                             "Commits that exist only on the remote will be lost." ),
                                          outcome->upstream, outcome->branch ),
                        _( "Force Push" ), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING | wxCENTER );
                confirm.SetYesNoLabels( _( "Force &Push" ), _( "Cancel" ) );

                if( confirm.ShowModal() == wxID_YES && m_dialog->SessionStillCurrent() )
                    startForcePush( common, outcome->branch, outcome->headOid, aRetriedAfterTrust );
            } );
}


void PANEL_GIT_BRANCHES::startForcePush( std::shared_ptr<KIGIT_COMMON> aCommon, const wxString& aExpectedBranch,
                                         const wxString& aExpectedHeadOid, bool aRetriedAfterTrust )
{
    // Carried rather than captured by reference, because Start returns before the work runs.
    struct OUTCOME
    {
        PushResult result = PushResult::Error;
        wxString   error;
        wxString   details;
        bool       authFailure = false;
        bool       cancelled = false;

        std::optional<KIGIT::HOST_KEY_FAILURE> hostKey;
    };

    auto outcome = std::make_shared<OUTCOME>();

    m_dialog->SetPaneTransferBusy( true, _( "Pushing..." ) );

    bool started = m_push.Start(
            "git force push",
            [aCommon, outcome, aExpectedBranch, aExpectedHeadOid, alive = m_push.Alive(),
             dialog = m_dialog]( const KIGIT_COMMON::CANCEL_TOKEN& aToken )
            {
                GIT_PUSH_HANDLER handler( aCommon.get() );
                handler.SetProgressReporter( std::make_unique<KIGIT_STATUS_REPORTER>(
                        alive,
                        [dialog]( const wxString& aText )
                        {
                            dialog->ShowTransferProgress( aText );
                        } ) );
                handler.SetCancelToken( aToken );
                handler.SetWaitForActionLock( true );
                handler.SetExpectedTarget( aExpectedBranch, aExpectedHeadOid );
                outcome->result = handler.PerformPush( true );
                KIGIT_ERROR_MESSAGE error = handler.PopError();
                outcome->error = error.summary;
                outcome->details = error.details;
                outcome->authFailure = handler.WasAuthFailure();
                outcome->hostKey = handler.GetHostKeyFailure();

                // This operation's own token, so a force push that never became active cannot
                // consume a cancel belonging to the transfer that is running.
                outcome->cancelled = handler.IsThisOperationCancelled();

                // The other surfaces need this even when the page that started it has gone.
                if( outcome->result == PushResult::Success && !outcome->cancelled )
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
                if( !m_dialog->SessionStillCurrent() )
                    return;

                m_dialog->SetPaneTransferBusy( false );

                if( !outcome->cancelled && outcome->result == PushResult::Success )
                    KIGIT::ClearSyncWarnings( aCommon.get() );

                if( outcome->cancelled || outcome->result == PushResult::Success )
                    return;

                if( outcome->result == PushResult::Busy )
                {
                    KIGIT::ShowBusyMessage( this );
                    return;
                }

                if( outcome->hostKey )
                {
                    if( KIGIT::PromptToTrustHost( this, *outcome->hostKey, aRetriedAfterTrust ) )
                        onForcePush( true );

                    return;
                }

                if( outcome->authFailure
                    && KIGIT::PromptForCredentials( this, aCommon.get(), &m_frame->Prj(), outcome->error ) )
                {
                    onForcePush( aRetriedAfterTrust );
                    return;
                }

                wxString error = outcome->error;

                if( error.IsEmpty() )
                    error = KIGIT::UnexplainedFailure( KIGIT_OPERATION::PUSH, aCommon->GetRemote() );

                DisplayErrorMessage( this, error, outcome->details );
            } );

    if( !started )
    {
        m_dialog->SetPaneTransferBusy( false );
        DisplayInfoMessage( this, _( "Could not start the force push." ) );
    }
}
