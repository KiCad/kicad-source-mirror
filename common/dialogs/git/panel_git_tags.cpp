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

#include <memory>

#include <confirm.h>
#include <dialogs/git/dialog_git_new_tag.h>
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

    m_tagsList->Bind( wxEVT_LIST_ITEM_SELECTED,
                      [this]( wxListEvent& aEvent )
                      {
                          if( !m_fillingTags )
                              showTagDetail( m_tagsList->GetItemText( aEvent.GetIndex(), 0 ) );
                      } );

    m_tagsList->Bind( wxEVT_LIST_ITEM_DESELECTED,
                      [this]( wxListEvent& )
                      {
                          if( !m_fillingTags
                              && m_tagsList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ) == -1 )
                          {
                              hideTagDetail();
                          }
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
    if( !m_dialog->SessionStillCurrent() )
        return;

    m_detailQuery.Cancel();

    auto tags = std::make_shared<std::vector<KIGIT_TAG_INFO>>();

    m_listQuery.Run(
            m_frame->Prj().GetGitCommon(),
            [tags]( KIGIT_COMMON& aCommon )
            {
                *tags = aCommon.GetTags();
            },
            [this, tags]()
            {
                if( !m_dialog->SessionStillCurrent() )
                    return;

                wxString selected = KIGIT::SelectedText( m_tagsList, 0 );
                long     selectRow = wxNOT_FOUND;

                m_detailQuery.Cancel();
                m_fillingTags = true;
                m_tagsList->DeleteAllItems();

                for( const KIGIT_TAG_INFO& t : *tags )
                {
                    long idx = m_tagsList->InsertItem( m_tagsList->GetItemCount(), t.name );
                    m_tagsList->SetItem( idx, 1, t.hash );
                    m_tagsList->SetItem( idx, 2, KIGIT::RelativeDate( t.when ) );

                    if( t.name == selected || ( selected.IsEmpty() && idx == 0 ) )
                        selectRow = idx;
                }

                int shown = m_tagsList->GetItemCount();
                m_tagCount->SetLabel( wxString::Format( wxPLURAL( "%d tag", "%d tags", shown ), shown ) );

                if( selectRow != wxNOT_FOUND )
                {
                    m_tagsList->SetItemState( selectRow, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
                    showTagDetail( m_tagsList->GetItemText( selectRow, 0 ) );
                }
                else
                {
                    hideTagDetail();
                }

                m_fillingTags = false;
            } );
}


void PANEL_GIT_TAGS::showTagDetail( const wxString& aTag )
{
    hideTagDetail();

    if( aTag.IsEmpty() || !m_dialog->SessionStillCurrent() )
        return;

    auto detail = std::make_shared<KIGIT_TAG_DETAIL>();

    m_detailQuery.Run(
            m_frame->Prj().GetGitCommon(),
            [detail, aTag]( KIGIT_COMMON& aCommon )
            {
                *detail = aCommon.GetTagDetail( aTag );
            },
            [this, detail, aTag]()
            {
                if( m_dialog->SessionStillCurrent() && KIGIT::SelectedText( m_tagsList, 0 ) == aTag )
                {
                    fillTagDetail( *detail );
                }
            } );
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
    m_detailQuery.Cancel();
    KIGIT::UnsplitKeepingSash( m_tagsSplitter, m_tagDetail, m_tagSashPos );
}


void PANEL_GIT_TAGS::onNewTag()
{
    if( !m_dialog->SessionStillCurrent() )
        return;

    DIALOG_GIT_NEW_TAG dlg( this );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString name = dlg.GetTagName();
    wxString message = dlg.GetMessage();

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

    m_dialog->RefuseOverUncommittedWork(
            [this, name]( const GIT_WORKTREE_STATUS& aStatus )
            {
                if( !IsOK( this, wxString::Format( _( "Check out tag '%s'? This shows an old snapshot of the "
                                                      "project. To keep any commits you make here, create a new "
                                                      "branch before committing. Switching away without one "
                                                      "leaves those commits outside every branch." ),
                                                   name ) ) )
                {
                    return;
                }

                if( !m_dialog->ConfirmWorkingTreeChange( aStatus ) )
                    return;

                m_dialog->RunGitAction(
                        [name]( KIGIT_COMMON& aCommon )
                        {
                            return aCommon.CheckoutTag( name );
                        },
                        wxString::Format( _( "Could not check out tag '%s'." ), name ),
                        /* rewrites working tree */ true );
            } );
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
