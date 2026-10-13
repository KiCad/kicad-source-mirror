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

#include <dialogs/git/panel_git_tags.h>

#include <mutex>

#include <wx/textdlg.h>

#include <confirm.h>
#include <dialogs/git/git_advanced_dialog.h>
#include <dialogs/git/git_ui_utils.h>
#include <eda_base_frame.h>
#include <git/kicad_git_common.h>
#include <project.h>


PANEL_GIT_TAGS::PANEL_GIT_TAGS( wxWindow* aParent, GIT_ADVANCED_DIALOG* aDialog, EDA_BASE_FRAME* aFrame ) :
        PANEL_GIT_TAGS_BASE( aParent ),
        m_dialog( aDialog ),
        m_frame( aFrame )
{
    m_tagsList->AppendColumn( _( "Tag" ), wxLIST_FORMAT_LEFT, FromDIP( 220 ) );
    m_tagsList->AppendColumn( _( "Commit" ), wxLIST_FORMAT_LEFT, FromDIP( 100 ) );
    m_tagsList->AppendColumn( _( "Date" ), wxLIST_FORMAT_LEFT, FromDIP( 110 ) );

    KIGIT::MarkDestructive( m_deleteTagButton );

    // The detail pane appears once a tag is selected.
    m_tagSashPos = FromDIP( 460 );
    m_tagsSplitter->Unsplit( m_tagDetail );

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

    m_tagsList->Bind( wxEVT_LIST_ITEM_SELECTED,
                      [this]( wxListEvent& aEvent )
                      {
                          if( !m_fillingTags )
                              showTagDetail( m_tagsList->GetItemText( aEvent.GetIndex(), 0 ) );
                      } );

    m_checkoutTagButton->Bind( wxEVT_BUTTON,
                               [this]( wxCommandEvent& )
                               {
                                   onCheckoutTag();
                               } );
    m_deleteTagButton->Bind( wxEVT_BUTTON,
                             [this]( wxCommandEvent& )
                             {
                                 onDeleteTag();
                             } );
    m_createTagButton->Bind( wxEVT_BUTTON,
                             [this]( wxCommandEvent& )
                             {
                                 onNewTag();
                             } );
}


void PANEL_GIT_TAGS::Populate()
{
    // The caller holds the git action lock.
    std::shared_ptr<KIGIT_COMMON> common = m_frame->Prj().GetGitCommon();
    wxString                      selected = KIGIT::SelectedText( m_tagsList, 0 );
    long                          selectRow = wxNOT_FOUND;

    m_fillingTags = true;
    m_tagsList->DeleteAllItems();

    for( const KIGIT_TAG_INFO& t : common->GetTags() )
    {
        long idx = m_tagsList->InsertItem( m_tagsList->GetItemCount(), t.name );
        m_tagsList->SetItem( idx, 1, t.hash );
        m_tagsList->SetItem( idx, 2, KIGIT::RelativeDate( t.when ) );

        if( t.name == selected || ( selected.IsEmpty() && idx == 0 ) )
            selectRow = idx;
    }

    int shown = m_tagsList->GetItemCount();
    m_tagCount->SetLabel( shown == 1 ? _( "1 tag" ) : wxString::Format( _( "%d tags" ), shown ) );

    if( selectRow != wxNOT_FOUND )
    {
        m_tagsList->SetItemState( selectRow, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
        fillTagDetail( common->GetTagDetail( m_tagsList->GetItemText( selectRow, 0 ) ) );
    }
    else
    {
        hideTagDetail();
    }

    m_fillingTags = false;
}


void PANEL_GIT_TAGS::showTagDetail( const wxString& aTag )
{
    if( aTag.IsEmpty() )
    {
        hideTagDetail();
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

    fillTagDetail( common->GetTagDetail( aTag ) );
}


void PANEL_GIT_TAGS::fillTagDetail( const KIGIT_TAG_DETAIL& aDetail )
{
    if( !aDetail.found )
    {
        hideTagDetail();
        return;
    }

    m_tagTitle->SetLabel( wxString::Format( _( "Tag: %s" ), aDetail.name ) );
    m_tagCommit->SetLabel( wxString::Format( _( "Commit: %s" ), aDetail.hash ) );
    m_tagAuthor->SetLabel( wxString::Format( _( "Author: %s <%s>" ), aDetail.author, aDetail.email ) );
    m_tagWhen->SetLabel( wxString::Format( _( "Date: %s" ), KIGIT::RelativeDate( aDetail.when ) ) );
    m_tagMessage->ChangeValue( aDetail.message );

    if( !m_tagsSplitter->IsSplit() )
        m_tagsSplitter->SplitVertically( m_tagListPanel, m_tagDetail, m_tagSashPos );

    m_tagDetail->Layout();
}


void PANEL_GIT_TAGS::hideTagDetail()
{
    KIGIT::UnsplitKeepingSash( m_tagsSplitter, m_tagDetail, m_tagSashPos );
}


void PANEL_GIT_TAGS::onNewTag()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    wxString name = wxGetTextFromUser( _( "New tag name:" ), _( "Create Tag" ), wxEmptyString, this );

    if( name.IsEmpty() )
        return;

    // A message makes the tag annotated.
    wxTextEntryDialog messageDlg( this, _( "Tag message, or leave empty for a plain tag:" ), _( "Create Tag" ) );

    if( messageDlg.ShowModal() != wxID_OK )
        return;

    wxString message = messageDlg.GetValue();

    m_dialog->RunGitAction(
            [name, message]( KIGIT_COMMON& aCommon )
            {
                return aCommon.CreateTag( name, message );
            },
            wxString::Format( _( "Could not create tag '%s'." ), name ) );
}


void PANEL_GIT_TAGS::onCheckoutTag()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    wxString name = KIGIT::SelectedText( m_tagsList, 0 );

    if( name.IsEmpty() )
        return;

    if( !m_dialog->RefuseOverUncommittedWork() )
        return;

    if( !IsOK( this, wxString::Format( _( "Check out tag '%s'? This shows an old snapshot of the "
                                          "project. To keep any commits you make here, create a new "
                                          "branch before committing. Switching away without one "
                                          "leaves those commits outside every branch." ),
                                       name ) ) )
    {
        return;
    }

    if( !m_dialog->ConfirmWorkingTreeChange() )
        return;

    m_dialog->RunGitAction(
            [name]( KIGIT_COMMON& aCommon )
            {
                return aCommon.CheckoutTag( name );
            },
            wxString::Format( _( "Could not check out tag '%s'." ), name ), /* rewrites working tree */ true );
}


void PANEL_GIT_TAGS::onDeleteTag()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    wxString name = KIGIT::SelectedText( m_tagsList, 0 );

    if( name.IsEmpty() )
        return;

    if( !IsOK( this, wxString::Format( _( "Delete tag '%s'?" ), name ) ) )
        return;

    m_dialog->RunGitAction(
            [name]( KIGIT_COMMON& aCommon )
            {
                return aCommon.DeleteTag( name );
            },
            wxString::Format( _( "Could not delete tag '%s'." ), name ) );
}
