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

#include <mutex>

#include <wx/textdlg.h>

#include <confirm.h>
#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_credentials_prompt.h>
#include <dialogs/git/git_status_entries.h>
#include <dialogs/git/git_ui_utils.h>
#include <eda_base_frame.h>
#include <git/kicad_git_common.h>
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

    KIGIT::MarkDestructive( m_dropStashButton );

    // The detail pane appears once a stash is selected.
    m_stashSashPos = FromDIP( 460 );
    m_stashesSplitter->Unsplit( m_stashDetail );

    m_retryTimer.SetOwner( this );
    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
                std::unique_lock<std::mutex>  lock = common->TryLockAction();

                if( lock.owns_lock() )
                    Populate();
                else
                    m_retryTimer.Start( 1000, wxTIMER_ONE_SHOT );
            },
            m_retryTimer.GetId() );

    m_stashesList->Bind( wxEVT_LIST_ITEM_SELECTED,
                         [this]( wxListEvent& aEvent )
                         {
                             if( !m_fillingStashes )
                                 showStashDetail( stashIdForRow( aEvent.GetIndex() ) );
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
    // The caller holds the git action lock.
    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    long     selectedRow = m_stashesList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
    wxString selected = stashIdForRow( selectedRow );
    long     selectRow = wxNOT_FOUND;

    m_fillingStashes = true;
    m_stashesList->DeleteAllItems();
    m_stashIds.clear();

    for( const KIGIT_STASH_INFO& s : common->GetStashes() )
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
    m_stashCount->SetLabel( shown == 1 ? _( "1 stash" ) : wxString::Format( _( "%d stashes" ), shown ) );

    if( selectRow != wxNOT_FOUND )
    {
        m_stashesList->SetItemState( selectRow, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
        fillStashDetail( common->GetStashDetail( stashIdForRow( selectRow ) ) );
    }
    else
    {
        hideStashDetail();
    }

    m_fillingStashes = false;
}


void PANEL_GIT_STASHES::showStashDetail( const wxString& aId )
{
    if( aId.IsEmpty() )
    {
        hideStashDetail();
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

    fillStashDetail( common->GetStashDetail( aId ) );
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

    long sel = m_stashesList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( sel == -1 )
        return;

    // Applying needs a clean tree, so saving the editors first would only turn unsaved edits
    // into the very changes that block it. Ask for a commit up front instead.
    if( !m_dialog->RefuseOverUncommittedWork() )
        return;

    wxString question = aRemoveAfterApply ? _( "Apply this stash and remove it from the list?" )
                                          : _( "Apply this stash to the working tree?" );

    if( !IsOK( this, question ) )
        return;

    if( !m_dialog->ConfirmWorkingTreeChange() )
        return;

    wxString id = stashIdForRow( sel );

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

    long sel = m_stashesList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( sel == -1 )
        return;

    if( !IsOK( this, _( "Drop this stash? This cannot be undone." ) ) )
        return;

    wxString id = stashIdForRow( sel );

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

    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();

    // Stashing writes a commit object, so it needs an author identity like a commit does.
    if( !KIGIT::EnsureGitIdentity( this, common.get() ) )
        return;

    // Unsaved editor changes are written to disk here, so they are part of what gets
    // stashed instead of surviving in memory.
    if( !m_dialog->ConfirmWorkingTreeChange() )
        return;

    bool hasTrackedChanges = false;
    bool hasNewFilesOnly = false;
    bool busy = false;

    {
        // Reading the status touches the repository, so it runs under the same lock the
        // background fetch and the transfer workers take. Only plain answers leave this scope,
        // because the messages below open modal dialogs.
        std::unique_lock<std::mutex> lock = common->TryLockAction();

        if( !lock.owns_lock() )
        {
            busy = true;
        }
        else
        {
            hasTrackedChanges = common->HasUncommittedTrackedChanges();
            hasNewFilesOnly = !hasTrackedChanges && !BuildGitChangeEntries( m_frame->Prj() ).empty();
        }
    }

    if( busy )
    {
        DisplayInfoMessage( this, _( "Another git operation is in progress. Try again in a moment." ) );
        return;
    }

    if( !hasTrackedChanges )
    {
        // A stash only includes files that have been committed before. When the only changes
        // are brand new files, saying "no changes" would contradict what the panel shows.
        if( hasNewFilesOnly )
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

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString message = dlg.GetValue().Trim( true ).Trim( false );

    m_dialog->RunGitAction(
            [message]( KIGIT_COMMON& aCommon )
            {
                return aCommon.StashSave( message );
            },
            _( "Could not stash the changes." ), /* rewrites working tree */ true );
}
