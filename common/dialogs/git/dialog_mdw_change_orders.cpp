/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialog_mdw_change_orders.h"

#include <algorithm>

#include <dialogs/html_message_box.h>
#include <grid_tricks.h>
#include <mdw/client/mdw_baseline_design.h>
#include <mdw/client/mdw_paged_read.h>
#include <mdw/client/mdw_source.h>
#include <string_utils.h>
#include <widgets/mdw_change_orders.h>
#include <widgets/wx_grid.h>

#include <wx/display.h>
#include <wx/settings.h>
#include <wx/intl.h>


namespace
{

enum ECO_COLUMN
{
    ECO_COL_ID,
    ECO_COL_TITLE,
    ECO_COL_STATE,
    ECO_COL_TYPE,
    ECO_COL_AUTHOR
};

enum REVIEW_COLUMN
{
    REVIEW_COL_ID,
    REVIEW_COL_TITLE,
    REVIEW_COL_STATE,
    REVIEW_COL_REVISION,
    REVIEW_COL_REVIEWERS
};


wxString verdictText( kiapi::mdw::ReviewState aVerdict )
{
    switch( aVerdict )
    {
    case kiapi::mdw::RS_APPROVED: return _( "approved" );
    case kiapi::mdw::RS_CHANGES_REQUESTED: return _( "changes requested" );
    case kiapi::mdw::RS_REJECTED: return _( "rejected" );
    case kiapi::mdw::RS_CANCELLED: return _( "withdrawn" );
    default: return _( "waiting" );
    }
}


bool isOffline( MDW_ERROR_CODE aCode )
{
    return aCode == MDW_ERROR_CODE::NETWORK || aCode == MDW_ERROR_CODE::TIMEOUT
           || aCode == MDW_ERROR_CODE::BACKEND_LOST;
}

} // namespace


DIALOG_MDW_CHANGE_ORDERS::DIALOG_MDW_CHANGE_ORDERS( wxWindow* aParent, std::shared_ptr<MDW_SOURCE> aSource,
                                                    const MDW_CHANGE_ORDERS_SUBJECT& aSubject ) :
        DIALOG_MDW_CHANGE_ORDERS_BASE( aParent ),
        m_source( std::move( aSource ) ),
        m_subject( aSubject )
{
    if( !m_subject.name.IsEmpty() )
        SetTitle( wxString::Format( _( "Change Orders and Reviews: %s" ), m_subject.name ) );

    // A visited link's theme colour can be unreadable in dark mode, and these links are actions
    for( wxHyperlinkCtrl* link : { m_ecoRetry, m_reviewRetry } )
        link->SetVisitedColour( link->GetNormalColour() );

    // fitColumns caps wide columns, so their cells ellipsize and show in full as a tooltip
    for( WX_GRID* grid : { m_ecoGrid, m_reviewGrid } )
    {
        GRID_TRICKS* tricks = new GRID_TRICKS( grid );

        for( int col = 0; col < grid->GetNumberCols(); ++col )
            tricks->SetTooltipEnable( col );

        grid->SetSelectionMode( wxGrid::wxGridSelectRows );
        grid->SetDefaultCellFitMode( wxGridFitMode::Ellipsize() );
        grid->PushEventHandler( tricks );
    }

    bool hasEcos = m_source->Has( MDW_CAPABILITY_GROUP::ECO ) && !m_subject.eco_items.empty();
    bool hasReviews = m_source->Has( MDW_CAPABILITY_GROUP::REVIEW ) && m_subject.review_item.has_value();

    // Unsupported groups get no page at all rather than a disabled one
    if( !hasReviews )
        m_notebook->RemovePage( m_notebook->FindPage( m_reviewPanel ) );

    if( !hasEcos )
        m_notebook->RemovePage( m_notebook->FindPage( m_ecoPanel ) );

    // Without a subject revision there is nothing to compare a review's opaque revision id with
    if( m_subject.revision_id.IsEmpty() )
    {
        m_thisRevisionOnly->Hide();
        m_reviewGrid->HideCol( REVIEW_COL_REVISION );
    }

    // finishDialogSettings takes the dialog's minimum from its sizer, so the floor goes on the content
    m_notebook->SetMinSize( FromDIP( wxSize( 780, 360 ) ) );
    SetupStandardButtons();
    finishDialogSettings();
}


DIALOG_MDW_CHANGE_ORDERS::~DIALOG_MDW_CHANGE_ORDERS()
{
    m_ecoOp.Cancel();
    m_reviewOp.Cancel();
    m_detailOp.Cancel();

    m_ecoGrid->PopEventHandler( true );
    m_reviewGrid->PopEventHandler( true );
}


bool DIALOG_MDW_CHANGE_ORDERS::TransferDataToWindow()
{
    if( !DIALOG_MDW_CHANGE_ORDERS_BASE::TransferDataToWindow() )
        return false;

    wxWindow* page = m_subject.page == MDW_CHANGE_ORDERS_PAGE::REVIEWS ? m_reviewPanel : m_ecoPanel;
    int       index = m_notebook->FindPage( page );

    // Runs after the saved control state loads, so the caller's link wins over the remembered page
    if( index != wxNOT_FOUND )
        m_notebook->ChangeSelection( index );

    // Read only now, so the lists follow the restored checkboxes rather than their defaults
    if( m_notebook->FindPage( m_ecoPanel ) != wxNOT_FOUND )
        loadEcos();

    if( m_notebook->FindPage( m_reviewPanel ) != wxNOT_FOUND )
        loadReviews();

    return true;
}


void DIALOG_MDW_CHANGE_ORDERS::Open( wxWindow* aParent, std::shared_ptr<MDW_SOURCE> aSource,
                                     const MDW_CHANGE_ORDERS_SUBJECT& aSubject )
{
    if( !aSource || ( !aSource->Has( MDW_CAPABILITY_GROUP::ECO ) && !aSource->Has( MDW_CAPABILITY_GROUP::REVIEW ) ) )
    {
        return;
    }

    DIALOG_MDW_CHANGE_ORDERS dlg( aParent, std::move( aSource ), aSubject );
    dlg.ShowModal();
}


wxString DIALOG_MDW_CHANGE_ORDERS::failureText( const MDW_RESULT& aResult, const wxString& aThing ) const
{
    const wxString source = m_source->Info().Name();

    if( isOffline( aResult.error ) )
        return wxString::Format( _( "%s is offline." ), source );

    if( aResult.error == MDW_ERROR_CODE::PERMISSION_DENIED )
    {
        wxString text = wxString::Format( _( "You do not have access to %s in %s." ), aThing, source );

        if( !aResult.message.IsEmpty() )
            text << wxS( " " ) << aResult.message;

        return text;
    }

    return wxString::Format( _( "Could not read %s." ), aThing ) + wxS( " " ) + MdwErrorText( aResult );
}


void DIALOG_MDW_CHANGE_ORDERS::loadEcos()
{
    m_ecos.clear();
    m_ecoMessage->SetLabel( _( "Checking change orders…" ) );
    m_ecoRetry->Hide();
    m_ecoGrid->ClearRows();
    m_ecoDetails->Disable();

    m_ecoOp = MdwReadAllPages<kiapi::mdw::Eco>(
            [this, closed = m_showClosed->GetValue()]( const MDW_PAGE&                      aPage,
                                                       MDW_DONE<MDW_PAGED<kiapi::mdw::Eco>> aDone )
            {
                return m_source->Ecos().List( m_subject.eco_items, closed, aPage, std::move( aDone ) );
            },
            0,
            [this]( const MDW_RESULT& aResult, const MDW_PAGED<kiapi::mdw::Eco>& aRead )
            {
                if( !aResult.IsOk() )
                {
                    m_ecoMessage->SetLabel( failureText( aResult, _( "change orders" ) ) );
                    m_ecoRetry->Show( aResult.error != MDW_ERROR_CODE::PERMISSION_DENIED );
                    m_ecoPanel->Layout();
                    return;
                }

                m_ecos = aRead.rows;
                m_ecosPartial = !aRead.next_page_token.IsEmpty();
                fillEcos();
            },
            MDW_READ_CAP::PARTIAL );
}


void DIALOG_MDW_CHANGE_ORDERS::fillEcos()
{
    // A backend may ignore include_closed, so the category decides what an unticked box hides
    m_shownEcos = m_showClosed->GetValue() ? m_ecos : MdwOpenEcos( m_ecos );

    // Backends list in their own order, which shifts as orders close
    std::stable_sort( m_shownEcos.begin(), m_shownEcos.end(),
                      []( const kiapi::mdw::Eco& aLhs, const kiapi::mdw::Eco& aRhs )
                      {
                          wxString lhs = wxString::FromUTF8( aLhs.display_id() );
                          return StrNumCmp( lhs, wxString::FromUTF8( aRhs.display_id() ) ) < 0;
                      } );

    m_ecoGrid->ClearRows();

    if( !m_shownEcos.empty() )
        m_ecoGrid->AppendRows( static_cast<int>( m_shownEcos.size() ) );

    for( size_t ii = 0; ii < m_shownEcos.size(); ++ii )
    {
        const kiapi::mdw::Eco& eco = m_shownEcos[ii];
        int                    row = static_cast<int>( ii );

        m_ecoGrid->SetCellValue( row, ECO_COL_ID, wxString::FromUTF8( eco.display_id() ) );
        m_ecoGrid->SetCellValue( row, ECO_COL_TITLE, wxString::FromUTF8( eco.title() ) );
        m_ecoGrid->SetCellValue( row, ECO_COL_STATE, wxString::FromUTF8( eco.state() ) );
        m_ecoGrid->SetCellValue( row, ECO_COL_TYPE, wxString::FromUTF8( eco.eco_type() ) );
        m_ecoGrid->SetCellValue( row, ECO_COL_AUTHOR, wxString::FromUTF8( eco.author() ) );
    }

    if( m_ecosPartial )
    {
        m_ecoMessage->SetLabel(
                wxString::Format( _( "Showing the first %d change orders." ), static_cast<int>( m_ecos.size() ) ) );
    }
    else if( m_shownEcos.empty() )
    {
        m_ecoMessage->SetLabel( m_showClosed->GetValue() ? _( "No change orders." ) : _( "No open change orders." ) );
    }
    else
    {
        m_ecoMessage->SetLabel( wxEmptyString );
    }

    m_ecoDetails->Enable( !m_shownEcos.empty() );
    fitColumns( m_ecoGrid );
    m_ecoPanel->Layout();
}


void DIALOG_MDW_CHANGE_ORDERS::fitColumns( WX_GRID* aGrid )
{
    aGrid->AutoSizeColumns( false );

    // One long title must not push the other columns off the display
    const int maxColumn = FromDIP( 320 );

    for( int col = 0; col < aGrid->GetNumberCols(); ++col )
    {
        if( aGrid->GetColSize( col ) > maxColumn )
            aGrid->SetColSize( col, maxColumn );
    }

    int needed = aGrid->GetRowLabelSize() + wxSystemSettings::GetMetric( wxSYS_VSCROLL_X, this );

    for( int col = 0; col < aGrid->GetNumberCols(); ++col )
        needed += aGrid->GetColSize( col );

    int extra = needed - aGrid->GetClientSize().x;

    if( extra <= 0 )
        return;

    int    displayIndex = wxDisplay::GetFromWindow( this );
    wxRect area = wxDisplay( displayIndex == wxNOT_FOUND ? 0 : displayIndex ).GetClientArea();
    int    width = std::min( GetSize().x + extra, area.GetWidth() );

    // Growing rightwards could push the dialog off the display
    int x = std::max( area.GetLeft(), std::min( GetPosition().x, area.GetRight() + 1 - width ) );

    SetSize( x, GetPosition().y, width, GetSize().y );
}


void DIALOG_MDW_CHANGE_ORDERS::loadReviews()
{
    m_reviews.clear();
    m_reviewStatus->SetLabel( _( "Review: checking…" ) );
    m_reviewMessage->SetLabel( wxEmptyString );
    m_reviewRetry->Hide();
    m_reviewGrid->ClearRows();
    m_reviewDetails->Disable();

    wxString revision = m_thisRevisionOnly->GetValue() ? m_subject.revision_id : wxString();

    m_reviewOp = MdwReadReviews( m_source, *m_subject.review_item, revision,
                         [this]( const MDW_RESULT& aResult, const MDW_REVIEW_READ& aRead )
                         {
                             if( aRead.unattributed )
                             {
                                 m_thisRevisionOnly->SetValue( false );
                                 m_thisRevisionOnly->Disable();
                                 m_thisRevisionOnly->SetToolTip( wxString::Format(
                                         _( "%s cannot tell revisions apart." ), m_source->Info().Name() ) );
                             }

                             if( !aResult.IsOk() )
                             {
                                 m_reviewStatus->SetLabel( wxEmptyString );
                                 m_reviewMessage->SetLabel( failureText( aResult, _( "reviews" ) ) );
                                 m_reviewRetry->Show( aResult.error != MDW_ERROR_CODE::PERMISSION_DENIED );
                                 m_reviewPanel->Layout();
                                 return;
                             }

                             m_reviews = aRead.reviews;
                             m_approvalRequired = aRead.approval_required;
                             m_reviewsPartial = aRead.partial;
                             fillReviews();
                         },
                         MDW_READ_CAP::PARTIAL );
}


void DIALOG_MDW_CHANGE_ORDERS::fillReviews()
{
    wxString revision = m_thisRevisionOnly->GetValue() ? m_subject.revision_id : wxString();
    wxString status;
    wxString message;

    m_reviews = MdwReviewsOfRevision( m_reviews, revision );

    // The review that decides the state may be among the rows not read, and the box filters only the rows
    if( m_reviewsPartial )
        message = wxString::Format( _( "Showing the first %d reviews." ), static_cast<int>( m_reviews.size() ) );
    else
        status = MdwReviewStatusLine( m_reviews, m_subject.revision_id, m_source->Info().Name() );

    // A tooltip needs a hover, and other revisions' reviews show without one
    if( !m_thisRevisionOnly->IsEnabled() )
        message << ( message.IsEmpty() ? wxString() : wxString( wxS( " " ) ) ) << m_thisRevisionOnly->GetToolTipText();

    // Its own line, since a long review status would otherwise push it past the dialog edge
    if( m_approvalRequired )
        status << ( status.IsEmpty() ? wxString() : wxString( wxS( "\n" ) ) )
               << _( "Release needs an approved review." );

    m_reviewStatus->SetLabel( status );
    m_reviewMessage->SetLabel( message );
    m_reviewGrid->ClearRows();

    if( !m_reviews.empty() )
        m_reviewGrid->AppendRows( static_cast<int>( m_reviews.size() ) );

    for( size_t ii = 0; ii < m_reviews.size(); ++ii )
    {
        const kiapi::mdw::Review& review = m_reviews[ii];
        int                       row = static_cast<int>( ii );
        wxString                  reviewers;

        for( const kiapi::mdw::Reviewer& reviewer : review.reviewers() )
        {
            AccumulateDescription( reviewers, MdwReviewerName( reviewer ) + wxS( " (" )
                                                      + verdictText( reviewer.verdict() ) + wxS( ")" ) );
        }

        m_reviewGrid->SetCellValue( row, REVIEW_COL_ID, wxString::FromUTF8( review.display_id() ) );
        m_reviewGrid->SetCellValue( row, REVIEW_COL_TITLE, wxString::FromUTF8( review.title() ) );
        m_reviewGrid->SetCellValue( row, REVIEW_COL_STATE, wxString::FromUTF8( review.state() ) );
        m_reviewGrid->SetCellValue( row, REVIEW_COL_REVIEWERS, reviewers );

        // Revision ids are opaque, so a review only says whether it covers the subject's revision
        if( !m_subject.revision_id.IsEmpty() )
        {
            wxString coverage = _( "Another revision" );

            if( review.revision_id().empty() )
                coverage = _( "Not stated" );
            else if( review.revision_id() == m_subject.revision_id.utf8_string() )
                coverage = _( "This revision" );

            m_reviewGrid->SetCellValue( row, REVIEW_COL_REVISION, coverage );
        }
    }

    m_reviewDetails->Enable( !m_reviews.empty() );
    fitColumns( m_reviewGrid );
    m_reviewPanel->Layout();
}


void DIALOG_MDW_CHANGE_ORDERS::showEco( int aRow )
{
    if( aRow < 0 || aRow >= static_cast<int>( m_shownEcos.size() ) )
        return;

    kiapi::mdw::Eco listed = m_shownEcos[aRow];

    // The list may be stale, so the box shows a fresh read and falls back to the listed row
    auto present = [this, listed]()
    {
        m_detailOp = m_source->Ecos().Get(
                listed.eco_id(),
                [this, listed]( const MDW_RESULT& aResult, const kiapi::mdw::Eco& aEco )
                {
                    const kiapi::mdw::Eco& eco = aResult.IsOk() ? aEco : listed;
                    HTML_MESSAGE_BOX       box( this, wxString::FromUTF8( eco.display_id() ) );

                    box.SetDialogSizeInDU( 320, 240 );
                    box.AddHTML_Text( MdwEcoDetailsHtml( eco, m_ecoSchema.value_or( kiapi::workspace::Schema() ) ) );
                    box.ShowModal();
                } );
    };

    if( m_ecoSchema )
    {
        present();
        return;
    }

    m_detailOp = m_source->Ecos().Schema(
            [this, present]( const MDW_RESULT& aResult, const kiapi::workspace::Schema& aSchema )
            {
                if( aResult.IsOk() )
                    m_ecoSchema = aSchema;

                present();
            } );
}


void DIALOG_MDW_CHANGE_ORDERS::showReview( int aRow )
{
    if( aRow < 0 || aRow >= static_cast<int>( m_reviews.size() ) )
        return;

    kiapi::mdw::Review listed = m_reviews[aRow];

    m_detailOp = m_source->Reviews().Get( listed.review_id(),
                                          [this, listed]( const MDW_RESULT& aResult, const kiapi::mdw::Review& aReview )
                                          {
                                              const kiapi::mdw::Review& review = aResult.IsOk() ? aReview : listed;
                                              HTML_MESSAGE_BOX box( this, wxString::FromUTF8( review.display_id() ) );

                                              box.SetDialogSizeInDU( 320, 240 );
                                              box.AddHTML_Text( MdwReviewDetailsHtml( review ) );
                                              box.ShowModal();
                                          } );
}


void DIALOG_MDW_CHANGE_ORDERS::OnEcoRetry( wxHyperlinkEvent& aEvent )
{
    loadEcos();
}


void DIALOG_MDW_CHANGE_ORDERS::OnShowClosed( wxCommandEvent& aEvent )
{
    loadEcos();
}


void DIALOG_MDW_CHANGE_ORDERS::OnEcoActivated( wxGridEvent& aEvent )
{
    showEco( aEvent.GetRow() );
}


void DIALOG_MDW_CHANGE_ORDERS::OnEcoDetails( wxCommandEvent& aEvent )
{
    showEco( m_ecoGrid->GetGridCursorRow() );
}


void DIALOG_MDW_CHANGE_ORDERS::OnReviewRetry( wxHyperlinkEvent& aEvent )
{
    loadReviews();
}


void DIALOG_MDW_CHANGE_ORDERS::OnThisRevisionOnly( wxCommandEvent& aEvent )
{
    loadReviews();
}


void DIALOG_MDW_CHANGE_ORDERS::OnReviewActivated( wxGridEvent& aEvent )
{
    showReview( aEvent.GetRow() );
}


void DIALOG_MDW_CHANGE_ORDERS::OnReviewDetails( wxCommandEvent& aEvent )
{
    showReview( m_reviewGrid->GetGridCursorRow() );
}
