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

#include <dialogs/git/panel_git_stashes.h>

#include <memory>

#include <wx/textdlg.h>

#include <confirm.h>
#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/git_ui_utils.h>
#include <eda_base_frame.h>
#include <git/kicad_git_common.h>
#include <git/project_git_utils.h>
#include <kiplatform/ui.h>
#include <project.h>


namespace
{

// A stash index is only valid for the listing it came from. Another program adding or dropping
// a stash shifts every index below it, so act on the stash itself.
static bool findStashByIdUnlocked( KIGIT_COMMON& aCommon, const wxString& aId, size_t& aIndexOut )
{
    if( aId.IsEmpty() )
        return false;

    for( const KIGIT_STASH_INFO& s : aCommon.GetStashes() )
    {
        if( s.id == aId )
        {
            aIndexOut = s.index;
            return true;
        }
    }

    return false;
}

} // namespace


PANEL_GIT_STASHES::PANEL_GIT_STASHES( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame ) :
        PANEL_GIT_STASHES_BASE( aParent ),
        m_dialog( aDialog ),
        m_frame( aFrame )
{
    m_stashesList->AppendColumn( _( "Description" ), wxLIST_FORMAT_LEFT, FromDIP( 360 ) );
    m_stashesList->AppendColumn( _( "Date" ), wxLIST_FORMAT_LEFT, FromDIP( 100 ) );
    m_stashFiles->AppendColumn( _( "File" ), wxLIST_FORMAT_LEFT, FromDIP( 300 ) );
    m_stashFiles->SetToolTip( _( "Double click a file to see what the stash changes in it." ) );

    KIGIT::MarkDestructive( m_dropStashButton );

    // The detail pane appears once a stash is selected.
    m_stashSashPos = FromDIP( 460 );
    m_stashesSplitter->Unsplit( m_stashDetail );

    m_stashesList->Bind( wxEVT_LIST_ITEM_SELECTED,
                         [this]( wxListEvent& aEvent )
                         {
                             if( !m_fillingStashes )
                                 showStashDetail( stashIdForRow( aEvent.GetIndex() ) );
                         } );

    m_stashesList->Bind( wxEVT_LIST_ITEM_DESELECTED,
                         [this]( wxListEvent& )
                         {
                             if( !m_fillingStashes
                                 && m_stashesList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ) == -1 )
                             {
                                 hideStashDetail();
                             }
                         } );

    // A stash is a commit, so comparing it with the commit it was taken from is what it changes.
    m_stashFiles->Bind( wxEVT_LIST_ITEM_ACTIVATED,
                        [this]( wxListEvent& aEvent )
                        {
                            long sel = m_stashesList->GetNextItem( -1, wxLIST_NEXT_ALL,
                                                                   wxLIST_STATE_SELECTED );
                            wxString id = stashIdForRow( sel );

                            if( !id.IsEmpty() )
                            {
                                ShowGitCommitDiff( m_frame, id, m_dialog,
                                                   m_stashFiles->GetItemText( aEvent.GetIndex() ) );
                            }
                        } );

    m_applyStashButton->Bind( wxEVT_BUTTON,
                              [this]( wxCommandEvent& )
                              {
                                  onStashApply();
                              } );
    m_popStashButton->Bind( wxEVT_BUTTON,
                            [this]( wxCommandEvent& )
                            {
                                onStashPop();
                            } );
    m_dropStashButton->Bind( wxEVT_BUTTON,
                             [this]( wxCommandEvent& )
                             {
                                 onStashDrop();
                             } );
    m_stashSaveButton->Bind( wxEVT_BUTTON,
                             [this]( wxCommandEvent& )
                             {
                                 onStashSave();
                             } );
}


void PANEL_GIT_STASHES::Populate()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    m_detailQuery.Cancel();

    auto stashes = std::make_shared<std::vector<KIGIT_STASH_INFO>>();

    m_listQuery.Run(
            m_frame->Prj().GetGitCommon(),
            [stashes]( KIGIT_COMMON& aCommon )
            {
                *stashes = aCommon.GetStashes();
            },
            [this, stashes]()
            {
                if( !m_dialog->SessionStillCurrent() )
                    return;

                long     selectedRow = m_stashesList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
                wxString selected = stashIdForRow( selectedRow );
                long     selectRow = wxNOT_FOUND;

                m_detailQuery.Cancel();
                m_fillingStashes = true;
                m_stashesList->DeleteAllItems();
                m_stashIds.clear();

                for( const KIGIT_STASH_INFO& s : *stashes )
                {
                    long idx = m_stashesList->InsertItem( m_stashesList->GetItemCount(), s.message );
                    m_stashesList->SetItem( idx, 1, KIGIT::RelativeDate( s.when ) );
                    m_stashesList->SetItemData( idx, static_cast<long>( s.index ) );
                    m_stashIds.push_back( s.id );

                    // Keep the selection on the same stash, or start on the newest.
                    if( s.id == selected || ( selected.IsEmpty() && idx == 0 ) )
                        selectRow = idx;
                }

                int shown = m_stashesList->GetItemCount();
                m_stashCount->SetLabel( wxString::Format( wxPLURAL( "%d stash", "%d stashes", shown ), shown ) );

                if( selectRow != wxNOT_FOUND )
                {
                    m_stashesList->SetItemState( selectRow, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
                    showStashDetail( stashIdForRow( selectRow ) );
                }
                else
                {
                    hideStashDetail();
                }

                m_fillingStashes = false;
            } );
}


void PANEL_GIT_STASHES::showStashDetail( const wxString& aId )
{
    // The old file list must not be used with the newly selected stash while its read is pending.
    hideStashDetail();

    if( aId.IsEmpty() || !m_dialog->SessionStillCurrent() )
        return;

    auto detail = std::make_shared<KIGIT_STASH_DETAIL>();

    m_detailQuery.Run(
            m_frame->Prj().GetGitCommon(),
            [detail, aId]( KIGIT_COMMON& aCommon )
            {
                *detail = aCommon.GetStashDetail( aId );
            },
            [this, detail, aId]()
            {
                if( !m_dialog->SessionStillCurrent() )
                    return;

                long selectedRow = m_stashesList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

                if( stashIdForRow( selectedRow ) == aId )
                    fillStashDetail( *detail );
            } );
}


void PANEL_GIT_STASHES::fillStashDetail( const KIGIT_STASH_DETAIL& aDetail )
{
    if( !aDetail.found )
    {
        hideStashDetail();
        return;
    }

    m_stashWhen->SetLabel( wxString::Format( _( "Stashed: %s" ), KIGIT::RelativeDate( aDetail.when ) ) );
    m_stashBranch->SetLabel( wxString::Format( _( "Branch: %s" ), aDetail.branch ) );
    m_stashCommit->SetLabel( wxString::Format( _( "Commit: %s" ), aDetail.baseHash ) );
    m_stashMessage->ChangeValue( aDetail.message );

    m_stashFiles->DeleteAllItems();

    for( const wxString& file : aDetail.files )
        m_stashFiles->InsertItem( m_stashFiles->GetItemCount(), file );

    if( !m_stashesSplitter->IsSplit() )
        m_stashesSplitter->SplitVertically( m_stashListPanel, m_stashDetail, m_stashSashPos );

    m_stashDetail->Layout();
}


void PANEL_GIT_STASHES::hideStashDetail()
{
    m_detailQuery.Cancel();
    KIGIT::UnsplitKeepingSash( m_stashesSplitter, m_stashDetail, m_stashSashPos );
}


wxString PANEL_GIT_STASHES::stashIdForRow( long aRow ) const
{
    if( aRow < 0 || static_cast<size_t>( aRow ) >= m_stashIds.size() )
        return wxEmptyString;

    return m_stashIds[aRow];
}


void PANEL_GIT_STASHES::restoreSelectedStash( bool aRemoveAfterApply )
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    // Resolved before the dialogs below. Each runs a nested event loop in which a refresh can
    // rebuild the list, and a row number would then stand for a different stash.
    wxString id = stashIdForRow( m_stashesList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ) );

    if( id.IsEmpty() )
        return;

    // Applying needs a clean tree, so report unsaved editors before asking to rewrite files.
    m_dialog->RefuseOverUncommittedWork(
            [this, id, aRemoveAfterApply]( const GIT_WORKTREE_STATUS& aStatus )
            {
                wxString question = aRemoveAfterApply ? _( "Apply this stash and remove it from the list?" )
                                                      : _( "Apply this stash to the working tree?" );

                if( !IsOK( this, question ) )
                    return;

                if( !m_dialog->ConfirmWorkingTreeChange( aStatus ) )
                    return;

                m_dialog->RunGitAction(
                        [id, aRemoveAfterApply]( KIGIT_COMMON& aCommon )
                        {
                            size_t index = 0;

                            if( !findStashByIdUnlocked( aCommon, id, index ) )
                                return false;

                            return aRemoveAfterApply ? aCommon.StashPop( index ) : aCommon.StashApply( index );
                        },
                        aRemoveAfterApply ? _( "Could not pop the stash. Another program may have changed the stash "
                                               "list, so nothing was applied." )
                                          : _( "Could not apply the stash. Another program may have changed the stash "
                                               "list, so nothing was applied." ),
                        /* rewrites working tree */ true );
            } );
}


void PANEL_GIT_STASHES::onStashApply()
{
    restoreSelectedStash( false );
}


void PANEL_GIT_STASHES::onStashPop()
{
    restoreSelectedStash( true );
}


void PANEL_GIT_STASHES::onStashDrop()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    // Resolved before the confirmation, which runs a nested event loop in which a refresh can
    // rebuild the list and renumber the rows.
    wxString id = stashIdForRow( m_stashesList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ) );

    if( id.IsEmpty() )
        return;

    if( !IsOK( this, _( "Drop this stash? This cannot be undone." ) ) )
        return;

    m_dialog->RunGitAction(
            [id]( KIGIT_COMMON& aCommon )
            {
                size_t index = 0;

                if( !findStashByIdUnlocked( aCommon, id, index ) )
                    return false;

                return aCommon.StashDrop( index );
            },
            _( "Could not drop the stash. Another program may have changed the stash list, so nothing "
               "was dropped." ) );
}


void PANEL_GIT_STASHES::onStashSave()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    struct PREFLIGHT
    {
        wxString            lfsMessage;
        KIGIT::GIT_IDENTITY identity;
    };

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    auto                          preflight = std::make_shared<PREFLIGHT>();

    m_actionQuery.Run(
            common,
            [preflight]( KIGIT_COMMON& aCommon )
            {
                preflight->lfsMessage = KIGIT::PROJECT_GIT_UTILS::LfsRefusal( &aCommon );
                preflight->identity = KIGIT::ReadGitIdentity( aCommon );
            },
            [this, common, preflight]()
            {
                if( !m_dialog->SessionStillCurrent() || !common->GetRepo() )
                    return;

                if( !preflight->lfsMessage.IsEmpty() )
                {
                    DisplayErrorMessage( this, preflight->lfsMessage );
                    return;
                }

                // Stashing writes a commit object, so it needs an author identity like a commit does.
                if( !KIGIT::EnsureGitIdentity( this, m_frame, common, preflight->identity ) )
                    return;

                // Unsaved editor changes are written to disk here, so they are part of what gets
                // stashed instead of surviving in memory.
                if( !m_dialog->ConfirmWorkingTreeChange() )
                    return;

                checkStashChanges();
            } );
}


void PANEL_GIT_STASHES::checkStashChanges()
{
    struct OUTCOME
    {
        GIT_WORKTREE_STATUS worktree;
        bool                hasNewFilesOnly = false;
    };

    wxString projectPath = m_frame->Prj().GetProjectPath();
    bool     dark = KIPLATFORM::UI::IsDarkTheme();
    auto     outcome = std::make_shared<OUTCOME>();

    m_actionQuery.Run(
            m_frame->Prj().GetGitCommon(),
            [outcome, projectPath, dark]( KIGIT_COMMON& aCommon )
            {
                outcome->worktree = ReadGitWorktreeStatus( aCommon, projectPath );
                outcome->hasNewFilesOnly = !outcome->worktree.hasTrackedChanges
                                           && !BuildGitChangeEntries( aCommon, projectPath, dark ).empty();
            },
            [this, outcome]()
            {
                if( !m_dialog->SessionStillCurrent() )
                    return;

                if( !ProjectSafeToRewrite( m_frame, this, outcome->worktree, /* allow tracked changes */ true ) )
                    return;

                if( !outcome->worktree.hasTrackedChanges )
                {
                    // A stash only includes files that have been committed before. When the only changes
                    // are brand new files, saying "no changes" would contradict what the panel shows.
                    if( outcome->hasNewFilesOnly )
                    {
                        DisplayInfoMessage( this, _( "The changed files have never been committed, and a stash only "
                                                     "includes files that have been. Commit them from the Version "
                                                     "Control panel first, or discard them there." ) );
                    }
                    else
                    {
                        DisplayInfoMessage( this, _( "There are no changes to stash." ) );
                    }

                    return;
                }

                wxTextEntryDialog dlg( this, _( "Optional description for this stash:" ), _( "Stash Changes" ) );

                if( dlg.ShowModal() != wxID_OK || !m_dialog->SessionStillCurrent() )
                    return;

                // Editing can resume while either the status read or the description dialog runs.
                // Do not rewrite a document whose editor has acquired fresh unsaved changes.
                if( !ProjectSafeToRewrite( m_frame, this, outcome->worktree, /* allow tracked changes */ true ) )
                    return;

                wxString message = dlg.GetValue().Trim( true ).Trim( false );

                m_dialog->RunGitAction(
                        [message]( KIGIT_COMMON& aCommon )
                        {
                            return aCommon.StashSave( message );
                        },
                        _( "Could not stash the changes." ), /* rewrites working tree */ true );
            } );
}
