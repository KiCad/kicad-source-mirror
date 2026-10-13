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

#include <dialogs/git/panel_git_remotes.h>

#include <mutex>

#include <wx/datetime.h>

#include <confirm.h>
#include <dialogs/git/dialog_git_remote.h>
#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_ui_utils.h>
#include <eda_base_frame.h>
#include <git/kicad_git_common.h>
#include <project.h>


PANEL_GIT_REMOTES::PANEL_GIT_REMOTES( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame ) :
        PANEL_GIT_REMOTES_BASE( aParent ),
        m_dialog( aDialog ),
        m_frame( aFrame )
{
    m_remotesList->AppendColumn( _( "Name" ), wxLIST_FORMAT_LEFT, FromDIP( 110 ) );
    m_remotesList->AppendColumn( _( "URL" ), wxLIST_FORMAT_LEFT, FromDIP( 340 ) );

    KIGIT::MarkDestructive( m_removeRemoteButton );

    // The detail pane appears once a remote is selected.
    m_remoteSashPos = FromDIP( 460 );
    m_remotesSplitter->Unsplit( m_remoteDetail );

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

    m_remotesList->Bind( wxEVT_LIST_ITEM_SELECTED,
                         [this]( wxListEvent& aEvent )
                         {
                             if( !m_fillingRemotes )
                                 showRemoteDetail( m_remotesList->GetItemText( aEvent.GetIndex(), 0 ) );
                         } );

    m_editUrlButton->Bind( wxEVT_BUTTON,
                           [this]( wxCommandEvent& )
                           {
                               onEditRemote();
                           } );
    m_removeRemoteButton->Bind( wxEVT_BUTTON,
                                [this]( wxCommandEvent& )
                                {
                                    onRemoveRemote();
                                } );
    m_addRemoteButton->Bind( wxEVT_BUTTON,
                             [this]( wxCommandEvent& )
                             {
                                 onAddRemote();
                             } );
}


void PANEL_GIT_REMOTES::Populate()
{
    // The caller holds the git action lock.
    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    wxString                      selected = KIGIT::SelectedText( m_remotesList, 0 );
    long                          selectRow = wxNOT_FOUND;

    m_fillingRemotes = true;
    m_remotesList->DeleteAllItems();

    for( const auto& [name, url] : common->GetRemotes() )
    {
        long idx = m_remotesList->InsertItem( m_remotesList->GetItemCount(), name );
        m_remotesList->SetItem( idx, 1, url );

        // Keep the selection where it was, or start on the first remote.
        if( name == selected || ( selected.IsEmpty() && idx == 0 ) )
            selectRow = idx;
    }

    int shown = m_remotesList->GetItemCount();
    m_remoteCount->SetLabel( shown == 1 ? _( "1 remote" ) : wxString::Format( _( "%d remotes" ), shown ) );

    if( selectRow != wxNOT_FOUND )
    {
        m_remotesList->SetItemState( selectRow, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
        fillRemoteDetail( m_remotesList->GetItemText( selectRow, 0 ), common->GetLastFetchTime() );
    }
    else
    {
        hideRemoteDetail();
    }

    m_fillingRemotes = false;
}


void PANEL_GIT_REMOTES::showRemoteDetail( const wxString& aRemote )
{
    if( aRemote.IsEmpty() )
    {
        hideRemoteDetail();
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

    fillRemoteDetail( aRemote, common->GetLastFetchTime() );
}


void PANEL_GIT_REMOTES::fillRemoteDetail( const wxString& aRemote, const wxDateTime& aFetched )
{
    // The list already holds the address.
    wxString url;

    for( long row = 0; row < m_remotesList->GetItemCount(); ++row )
    {
        if( m_remotesList->GetItemText( row, 0 ) == aRemote )
            url = m_remotesList->GetItemText( row, 1 );
    }

    m_remoteTitle->SetLabel( wxString::Format( _( "Remote: %s" ), aRemote ) );
    m_remoteUrl->ChangeValue( url );

    // Git records fetches in FETCH_HEAD and nothing about pulls or pushes, so only that one
    // can be shown.
    m_remoteFetched->SetLabel( aFetched.IsValid()
                                       ? wxString::Format( _( "Last fetch: %s" ), KIGIT::RelativeDate( aFetched ) )
                                       : _( "Not fetched yet." ) );

    if( !m_remotesSplitter->IsSplit() )
        m_remotesSplitter->SplitVertically( m_remoteListPanel, m_remoteDetail, m_remoteSashPos );

    m_remoteDetail->Layout();
}


void PANEL_GIT_REMOTES::hideRemoteDetail()
{
    KIGIT::UnsplitKeepingSash( m_remotesSplitter, m_remoteDetail, m_remoteSashPos );
}


void PANEL_GIT_REMOTES::runRemoteAction( const std::function<bool( KIGIT_COMMON& )>& aMutation, const wxString& aError )
{
    m_dialog->RunGitAction(
            [&aMutation]( KIGIT_COMMON& aCommon )
            {
                if( !aMutation( aCommon ) )
                    return false;

                // Refresh the cached URL and credentials, so a later push does not reach the new
                // remote authenticating as the old one.
                aCommon.UpdateCurrentBranchInfo();
                return true;
            },
            aError );
}


void PANEL_GIT_REMOTES::onAddRemote()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    DIALOG_GIT_REMOTE dlg( this );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString name = dlg.GetName();
    wxString url = dlg.GetUrl();

    runRemoteAction(
            [name, url]( KIGIT_COMMON& aCommon )
            {
                return aCommon.AddRemote( name, url );
            },
            wxString::Format( _( "Could not add remote '%s'." ), name ) );
}


void PANEL_GIT_REMOTES::onEditRemote()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    wxString name = KIGIT::SelectedText( m_remotesList, 0 );

    if( name.IsEmpty() )
        return;

    wxString          current = KIGIT::SelectedText( m_remotesList, 1 );
    DIALOG_GIT_REMOTE dlg( this, name, current );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString url = dlg.GetUrl();

    // The list shows the address with any user:password removed, so writing it back unchanged
    // would delete credentials the config still holds.
    if( url == current )
        return;

    runRemoteAction(
            [name, url]( KIGIT_COMMON& aCommon )
            {
                return aCommon.SetRemoteUrl( name, url );
            },
            wxString::Format( _( "Could not update remote '%s'." ), name ) );
}


void PANEL_GIT_REMOTES::onRemoveRemote()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    wxString name = KIGIT::SelectedText( m_remotesList, 0 );

    if( name.IsEmpty() )
        return;

    if( !IsOK( this, wxString::Format( _( "Remove remote '%s'?" ), name ) ) )
        return;

    runRemoteAction(
            [name]( KIGIT_COMMON& aCommon )
            {
                return aCommon.RemoveRemote( name );
            },
            wxString::Format( _( "Could not remove remote '%s'." ), name ) );
}
