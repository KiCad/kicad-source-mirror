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

#include "dialog_mdw_design_release.h"
#include "git_status_entries.h"
#include "mdw_design_parts.h"

#include <bitmaps.h>
#include <confirm.h>
#include <dialogs/git/dialog_mdw_change_orders.h>
#include <eda_base_frame.h>
#include <kiway.h>
#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_lifecycle.h>
#include <mdw/client/mdw_pending_changes.h>
#include <mdw/client/mdw_source.h>
#include <project.h>
#include <scoped_set_reset.h>
#include <string_utils.h>
#include <widgets/git_commit_pane.h>
#include <widgets/mdw_change_orders.h>

#include <wx/button.h>
#include <wx/hyperlink.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/display.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/intl.h>

#include <algorithm>
#include <numeric>
#include <utility>


namespace
{

enum COLUMN
{
    COL_REFERENCE,
    COL_TYPE,
    COL_PART,
    COL_STATUS
};


/// How the dialog ends when its action continues in another window
constexpr int ID_CHECK_IN = wxID_HIGHEST + 1;

/// How often a shown dialog compares the design files and the editors with its last check
constexpr int RECHECK_POLL_MS = 1000;


enum ICON
{
    ICON_NONE = -1,
    ICON_WARNING = 0,
    ICON_BLOCKING = 1
};

} // namespace


DIALOG_MDW_DESIGN_RELEASE::DIALOG_MDW_DESIGN_RELEASE( EDA_BASE_FRAME* aFrame, GIT_COMMIT_PANE* aPane, PROJECT& aProject,
                                                      std::shared_ptr<MDW_SOURCE>   aSource,
                                                      std::vector<MDW_PLACED_INPUT> aInputs ) :
        DIALOG_SHIM( aFrame, wxID_ANY, _( "Release Design" ), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_frame( aFrame ),
        m_pane( aPane ),
        m_project( aProject ),
        m_source( std::move( aSource ) ),
        m_parts( MdwGroupPlacedParts( aInputs ) ),
        m_savedAt( MdwDesignWriteTimes( aProject ) ),
        m_pollTimer( this )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    m_summary = new wxStaticText( this, wxID_ANY, _( "Checking placed parts against their releases..." ) );
    mainSizer->Add( m_summary, 0, wxALL | wxEXPAND, 10 );

    m_pendingLine = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_pendingLine->Hide();
    mainSizer->Add( m_pendingLine, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10 );

    wxBoxSizer* reviewRow = new wxBoxSizer( wxHORIZONTAL );
    m_reviewLine = new wxStaticText( this, wxID_ANY, wxEmptyString );
    reviewRow->Add( m_reviewLine, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );
    m_reviewLink = new wxHyperlinkCtrl( this, wxID_ANY, _( "View" ), wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                        wxHL_ALIGN_RIGHT | wxNO_BORDER );
    reviewRow->Add( m_reviewLink, 0, wxALIGN_CENTER_VERTICAL );
    mainSizer->Add( reviewRow, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10 );

    // A source without reviews gets no row at all, never a disabled one
    m_reviewLine->Show( m_source->Has( MDW_CAPABILITY_GROUP::REVIEW ) );
    m_reviewLink->Hide();

    m_ecoLine = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_ecoLine->Hide();
    mainSizer->Add( m_ecoLine, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10 );

    m_list = new wxListView( this, wxID_ANY, wxDefaultPosition, FromDIP( wxSize( 760, 340 ) ),
                             wxLC_REPORT | wxLC_HRULES );
    m_list->AppendColumn( _( "Reference" ) );
    m_list->AppendColumn( _( "Type" ) );
    m_list->AppendColumn( _( "Part" ) );
    m_list->AppendColumn( _( "Status" ) );
    m_exceptionColumn = m_list->AppendColumn( _( "Exception" ) );
    findEcoSources();
    m_list->SetSmallImages( { KiBitmapBundle( BITMAPS::small_warning ), KiBitmapBundle( BITMAPS::ercerr ) } );
    mainSizer->Add( m_list, 1, wxLEFT | wxRIGHT | wxEXPAND, 10 );

    mainSizer->Add( new wxStaticText( this, wxID_ANY, _( "Release note:" ) ), 0, wxLEFT | wxRIGHT | wxTOP, 10 );
    m_releaseNote = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP( wxSize( -1, 60 ) ),
                                    wxTE_MULTILINE );
    mainSizer->Add( m_releaseNote, 0, wxLEFT | wxRIGHT | wxTOP | wxEXPAND, 10 );
    // A restored note would describe the previous release
    OptOut( m_releaseNote );

    wxBoxSizer* buttons = new wxBoxSizer( wxHORIZONTAL );
    m_exceptionButton = new wxButton( this, wxID_ANY, _( "Record Exception..." ) );
    buttons->Add( m_exceptionButton, 0, wxRIGHT, 5 );
    buttons->AddStretchSpacer();
    m_releaseButton = new wxButton( this, wxID_OK, _( "Release Design" ) );
    buttons->Add( m_releaseButton, 0, wxRIGHT, 5 );
    m_cancelButton = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    buttons->Add( m_cancelButton, 0 );
    mainSizer->Add( buttons, 0, wxALL | wxEXPAND, 10 );

    SetSizerAndFit( mainSizer );
    fill();
    Centre();

    m_exceptionButton->Bind( wxEVT_BUTTON, &DIALOG_MDW_DESIGN_RELEASE::onRecordException, this );

    // Closing mid-release would cancel the request and could lose the backend's answer
    Bind( wxEVT_CLOSE_WINDOW,
          [this]( wxCloseEvent& aEvent )
          {
              if( m_busy && aEvent.CanVeto() )
                  aEvent.Veto();
              else
                  aEvent.Skip();
          } );

    // Deferred, since a recheck can open a progress window while the activation is still being handled
    Bind( wxEVT_ACTIVATE,
          [this]( wxActivateEvent& aEvent )
          {
              if( aEvent.GetActive() )
                  CallAfter(
                          [this]()
                          {
                              recheckIfChanged( true );
                          } );

              aEvent.Skip();
          } );

    // An editor beside the dialog saves or edits without activating it, so a cheap poll watches for that
    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                if( IsShown() )
                    recheckIfChanged( false );
            },
            m_pollTimer.GetId() );

    Bind( wxEVT_SHOW,
          [this]( wxShowEvent& aEvent )
          {
              if( aEvent.IsShown() )
                  m_pollTimer.Start( RECHECK_POLL_MS );
              else
                  m_pollTimer.Stop();

              aEvent.Skip();
          } );

    // Bound on the dialog, since Ctrl+Enter sends wxID_OK without passing through the button
    Bind( wxEVT_BUTTON, &DIALOG_MDW_DESIGN_RELEASE::onAction, this, wxID_OK );
    m_reviewLink->Bind( wxEVT_HYPERLINK,
                        [this]( wxHyperlinkEvent& )
                        {
                            showReview();
                        } );
    m_list->Bind( wxEVT_LIST_ITEM_SELECTED, [this]( wxListEvent& ) { updateControls(); } );
    m_list->Bind( wxEVT_LIST_ITEM_DESELECTED, [this]( wxListEvent& ) { updateControls(); } );
    m_list->Bind( wxEVT_LIST_ITEM_ACTIVATED,
                  [this]( wxListEvent& aEvent )
                  {
                      showPartEcos( aEvent.GetIndex() );
                  } );

    m_list->Bind( wxEVT_SIZE,
                  [this]( wxSizeEvent& aEvent )
                  {
                      sizeColumns( false );
                      aEvent.Skip();
                  } );

    // A report-mode list has no select-all of its own, and exceptions are often recorded in bulk
    m_list->Bind( wxEVT_KEY_DOWN,
                  [this]( wxKeyEvent& aEvent )
                  {
                      if( aEvent.GetKeyCode() == 'A' && aEvent.GetModifiers() == wxMOD_CONTROL )
                      {
                          for( long i = 0; i < m_list->GetItemCount(); ++i )
                              m_list->Select( i );

                          return;
                      }

                      aEvent.Skip();
                  } );

    // A library source still connecting when the dialog opened answers once it is up
    m_connections = MDW_CLIENT::Get().SubscribeConnections( this,
                                                            [this]( const wxString& )
                                                            {
                                                                onConnectionChanged();
                                                            } );

    startCheck();
}


void DIALOG_MDW_DESIGN_RELEASE::onConnectionChanged()
{
    if( m_busy )
        return;

    // A recheck under the exception prompt would rebuild the list it records against
    if( !m_checked || m_prompting )
    {
        m_recheck = true;
        return;
    }

    bool unverified = std::any_of( m_checks.begin(), m_checks.end(),
                                   []( const MDW_BASELINE_CHECK& aCheck )
                                   {
                                       return std::find( aCheck.facts.statuses.begin(), aCheck.facts.statuses.end(),
                                                         kiapi::mdw::PPS_UNVERIFIED )
                                              != aCheck.facts.statuses.end();
                                   } );

    if( unverified )
        startCheck();
}


void DIALOG_MDW_DESIGN_RELEASE::setParts( const std::vector<MDW_PLACED_INPUT>& aInputs )
{
    m_op.Cancel();
    m_reviewOp.Cancel();
    m_ecoOp.Cancel();
    std::vector<MDW_DESIGN_PART> parts = MdwGroupPlacedParts( aInputs );
    m_exceptions = MdwCarryExceptions( m_parts, m_exceptions, parts );
    m_parts = std::move( parts );
    m_checks.clear();
    m_ecos = MDW_DESIGN_ECOS();
    m_checked = false;
    findEcoSources();
    fill();
}


void DIALOG_MDW_DESIGN_RELEASE::findEcoSources()
{
    m_ecoSources = { m_source };
    m_ecoMissing.clear();

    // A part's change orders may sit with its library member or with its MDW_Part, which can differ
    for( const MDW_DESIGN_PART& part : m_parts )
    {
        for( const MDW_ITEM_REF& ref : { part.baseline.item, part.baseline.part } )
        {
            if( ref.item_id.IsEmpty() || ref.source.IsEmpty() )
                continue;

            std::shared_ptr<MDW_SOURCE> owner = MDW_CLIENT::Get().FindSource( ref.source );

            if( !owner )
            {
                if( std::find( m_ecoMissing.begin(), m_ecoMissing.end(), ref.source ) == m_ecoMissing.end() )
                    m_ecoMissing.push_back( ref.source );
            }
            else if( std::find( m_ecoSources.begin(), m_ecoSources.end(), owner ) == m_ecoSources.end() )
            {
                m_ecoSources.push_back( owner );
            }
        }
    }

    bool hasEcos = std::any_of( m_ecoSources.begin(), m_ecoSources.end(),
                                []( const std::shared_ptr<MDW_SOURCE>& aEcoSource )
                                {
                                    return aEcoSource->Has( MDW_CAPABILITY_GROUP::ECO );
                                } );

    // A source that is not set up might hold change orders, so its absence is reported, not hidden
    if( ( hasEcos || !m_ecoMissing.empty() ) && m_ecoColumn < 0 )
    {
        m_ecoColumn = m_exceptionColumn++;
        m_list->InsertColumn( m_ecoColumn, _( "Change Orders" ) );
    }
}


void DIALOG_MDW_DESIGN_RELEASE::startCheck()
{
    m_checked = false;
    m_readiness.reset();
    m_editorsDirty = DocumentEditorsDirty( m_frame->Kiway() );
    setLine( m_summary, _( "Checking placed parts against their releases..." ) );
    updateControls();

    // The baseline names the revision the design was checked out at, so edits outside it cannot be released
    m_op = MDW_CLIENT::Get().ProjectState(
            m_project,
            [this]( const MDW_RESULT& aResult, const MDW_PROJECT_STATE& aState )
            {
                // A working copy's design not added yet has no item, and its first commit and push makes one
                const bool notAdded = aResult.error == MDW_ERROR_CODE::ITEM_NOT_FOUND && workingCopy();

                if( !aResult.IsOk() && !notAdded )
                {
                    setLine( m_summary, wxString::Format( _( "Could not read the design's revision: %s" ),
                                                          MdwErrorText( aResult ) ) );
                    return;
                }

                m_state = aState;
                m_projectRef = aState.item;
                m_revisionId = aState.base_revision_id;
                m_revisionLabel = aState.base_revision_label;

                // Pending releases, reviews and change orders are of a revision, which a design without one lacks
                const bool revised = !m_revisionId.IsEmpty();

                if( revised )
                {
                    showPending();
                    startReviewCheck();
                }

                // Without a revision the parts are checked as saved, and the source's hold names the step left
                m_op = MdwCheckDesign( m_source, m_projectRef, m_revisionId, m_parts,
                                       [this, revised]( const MDW_RESULT& aCheckResult, const MDW_DESIGN_CHECK& aCheck )
                                       {
                                           if( !aCheckResult.IsOk() )
                                           {
                                               // The commit that makes a revision does not need the parts checked
                                               if( !revised )
                                                   showReadiness( MDW_BASELINE_HOLD::NONE, wxEmptyString );

                                               setLine( m_summary,
                                                        wxString::Format( _( "Could not check the placed parts: %s" ),
                                                                          MdwErrorText( aCheckResult ) ) );
                                               return;
                                           }

                                           m_parts = aCheck.parts;
                                           m_checks = aCheck.checks;
                                           m_checked = true;

                                           fill();
                                           showReadiness( aCheck.hold, aCheck.holder );

                                           if( revised )
                                               startEcoCheck();

                                           if( std::exchange( m_recheck, false ) )
                                               onConnectionChanged();
                                       } );
            } );
}


void DIALOG_MDW_DESIGN_RELEASE::showReadiness( MDW_BASELINE_HOLD aHold, const wxString& aHolder )
{
    // The source's hold is the one its release refuses on, so the dialog never offers a release it would refuse
    m_readiness = MdwReleaseReadiness( DocumentEditorsDirty( m_frame->Kiway() ), m_state, aHold, aHolder,
                                       m_source->Info().backend.checkin_model );
    updateControls();
}


void DIALOG_MDW_DESIGN_RELEASE::startReviewCheck()
{
    if( !m_source->Has( MDW_CAPABILITY_GROUP::REVIEW ) )
        return;

    m_reviewGate.reset();
    m_reviews.Clear();
    setLine( m_reviewLine, _( "Review: checking..." ) );
    m_reviewLink->Hide();
    m_reviewOp = MdwListDesignReviews( m_source, m_projectRef,
                                       [this]( const MDW_RESULT& aResult, const kiapi::mdw::ListReviewsReply& aList )
                                       {
                                           if( !aResult.IsOk() )
                                           {
                                               showReviewFailure( aResult );
                                               return;
                                           }

                                           m_reviews = aList;
                                           showReviewGate();
                                       } );
}


void DIALOG_MDW_DESIGN_RELEASE::showReviewFailure( const MDW_RESULT& aResult )
{
    wxString name = m_source->Info().display_name;
    wxString line;

    switch( aResult.error )
    {
    case MDW_ERROR_CODE::PERMISSION_DENIED:
        line = wxString::Format( _( "You do not have access to reviews in %s." ), name );
        break;

    case MDW_ERROR_CODE::NETWORK:
    case MDW_ERROR_CODE::TIMEOUT:
    case MDW_ERROR_CODE::BACKEND_LOST:
        line = wxString::Format( _( "Review not checked: %s is offline." ), name );
        break;

    default: line = wxString::Format( _( "Could not check the review: %s" ), MdwErrorText( aResult ) ); break;
    }

    // The gate is advisory, so a failed read leaves Release to the backend's own refusal
    line << wxS( " " ) << wxString::Format( _( "%s decides at release whether an approved review is needed." ), name );
    setLine( m_reviewLine, line );
    updateControls();
}


void DIALOG_MDW_DESIGN_RELEASE::showReviewGate()
{
    // The line speaks for this design, so it names only reviews the gate could count
    const std::vector<kiapi::mdw::Review> ofDesign = MdwReviewsOf( m_reviews, m_projectRef );

    m_reviewGate = MdwReviewGate( m_reviews, m_projectRef, m_revisionId );
    wxString line = MdwReviewStatusLine( ofDesign, m_revisionId, m_source->Info().display_name );

    if( m_reviewGate->blocks )
        line << wxS( " " ) << _( "This design needs an approved review before release." );

    m_reviewLink->Show( MdwShownReview( ofDesign, m_revisionId ) != nullptr );
    setLine( m_reviewLine, line );
    updateControls();
}


void DIALOG_MDW_DESIGN_RELEASE::startEcoCheck()
{
    if( m_ecoColumn < 0 )
        return;

    for( long row = 0; row < m_list->GetItemCount(); ++row )
        m_list->SetItem( row, m_ecoColumn, wxS( "..." ) );

    m_ecoOp = MdwListDesignEcos(
            m_ecoSources, m_projectRef, m_parts,
            [this]( const MDW_RESULT& aResult, const MDW_DESIGN_ECOS& aEcos )
            {
                m_ecos = aEcos;

                for( long row = 0; row < m_list->GetItemCount(); ++row )
                    m_list->SetItem( row, m_ecoColumn,
                                     ecoIds( m_ecos.parts.empty() ? std::vector<size_t>() : m_ecos.parts[row] ) );

                wxString line;

                if( !aResult.IsOk() )
                {
                    line = aResult.error == MDW_ERROR_CODE::PERMISSION_DENIED
                                   ? wxString( _( "You do not have access to change orders." ) )
                                   : wxString::Format( _( "Change orders not checked: %s" ),
                                                       MdwErrorText( aResult ) );
                }
                else
                {
                    std::vector<kiapi::mdw::Eco> onDesign;

                    for( size_t index : m_ecos.design )
                        onDesign.push_back( m_ecos.ecos[index].eco );

                    line = MdwDesignEcoNote( onDesign );

                    for( const wxString& failure : m_ecos.failures )
                    {
                        if( !line.IsEmpty() )
                            line << wxS( "\n" );

                        line << wxString::Format( _( "Change orders not checked: %s" ), failure );
                    }
                }

                for( const wxString& locator : m_ecoMissing )
                {
                    if( !line.IsEmpty() )
                        line << wxS( "\n" );

                    line << wxString::Format( _( "Change orders not checked: %s is not set up on this computer." ),
                                              locator );
                }

                setLine( m_ecoLine, line );
                m_ecoLine->Show( !line.IsEmpty() );
                Layout();
                sizeColumns();
            } );
}


wxString DIALOG_MDW_DESIGN_RELEASE::ecoIds( const std::vector<size_t>& aIndices ) const
{
    wxString ids;

    for( size_t index : aIndices )
    {
        AccumulateDescription( ids, MdwEcoName( m_ecos.ecos[index].eco ) );
    }

    return ids;
}


void DIALOG_MDW_DESIGN_RELEASE::setLine( wxStaticText* aLine, const wxString& aText )
{
    int width = m_list->GetSize().x;

    // The review line shares its row with the View link
    if( aLine == m_reviewLine && m_reviewLink->IsShown() )
        width -= m_reviewLink->GetSize().x + FromDIP( 10 );

    // Wrap rather than clip or widen the dialog when a reason runs long
    aLine->SetLabel( aText );
    aLine->Wrap( width );
    Layout();
}


void DIALOG_MDW_DESIGN_RELEASE::showPending()
{
    wxString waiting;

    for( const MDW_PENDING_ENTRY& entry : MDW_PENDING_CHANGES::Get().For( m_projectRef ) )
    {
        if( entry.kind != MDW_PENDING_KIND::DESIGN_BASELINE )
            continue;

        AccumulateDescription( waiting, entry.change.display_id.IsEmpty() ? entry.change.change_id
                                                                          : entry.change.display_id );
    }

    // Advisory, since the backend decides whether another release may start while one waits
    if( !waiting.IsEmpty() )
    {
        setLine( m_pendingLine,
                 wxString::Format( _( "A release of this design is waiting for approval in %s." ), waiting ) );
    }

    m_pendingLine->Show( !waiting.IsEmpty() );
    Layout();
}


void DIALOG_MDW_DESIGN_RELEASE::showReview()
{
    MDW_PROJECT_STATE state;
    state.item = m_projectRef;
    state.base_revision_id = m_revisionId;

    DIALOG_MDW_CHANGE_ORDERS::Open(
            this, m_source, MdwDesignSubject( m_project.GetProjectName(), state, MDW_CHANGE_ORDERS_PAGE::REVIEWS ) );
}


void DIALOG_MDW_DESIGN_RELEASE::showPartEcos( long aRow )
{
    if( aRow < 0 || static_cast<size_t>( aRow ) >= m_ecos.parts.size() || m_ecos.parts[aRow].empty() )
        return;

    const MDW_DESIGN_PART& part = m_parts[aRow];

    // Each source lists its own change orders, so open the one holding the first listed for this part
    std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().FindSource( m_ecos.ecos[m_ecos.parts[aRow][0]].source );

    if( !source )
        return;

    std::vector<MDW_ITEM_REF> items;

    if( !part.baseline.item.item_id.IsEmpty() )
        items.push_back( part.baseline.item );

    if( !part.baseline.part.item_id.IsEmpty() )
        items.push_back( part.baseline.part );

    DIALOG_MDW_CHANGE_ORDERS::Open( this, source, { part.baseline.lib_id, items, std::nullopt, wxEmptyString } );
}


void DIALOG_MDW_DESIGN_RELEASE::fill()
{
    m_list->DeleteAllItems();

    std::vector<bool> excepted( m_parts.size(), false );

    for( const MDW_BASELINE_EXCEPTION& exception : m_exceptions )
        excepted[exception.part_index] = true;

    // Parts are listed before their check so the design can be reviewed while a step remains
    for( size_t i = 0; i < m_parts.size(); ++i )
    {
        const MDW_DESIGN_PART&    part = m_parts[i];
        const MDW_BASELINE_CHECK* check = i < m_checks.size() ? &m_checks[i] : nullptr;

        // An excepted part is cleared for release, so it keeps no flag
        int  icon = !check || excepted[i]                       ? ICON_NONE
                    : check->severity == MDW_SEVERITY::BLOCKING ? ICON_BLOCKING
                    : check->severity == MDW_SEVERITY::WARNING  ? ICON_WARNING
                                                                : ICON_NONE;
        long row = m_list->InsertItem( static_cast<long>( i ), AccumulateDescriptions( part.baseline.references ),
                                       icon );

        m_list->SetItem( row, COL_TYPE, part.kind == MDW_PLACED_KIND::SYMBOL ? _( "Symbol" ) : _( "Footprint" ) );
        m_list->SetItem( row, COL_PART,
                         part.pin_label.IsEmpty()
                                 ? part.baseline.lib_id
                                 : wxString::Format( _( "%s (pinned to %s)" ), part.baseline.lib_id, part.pin_label ) );

        if( check )
            m_list->SetItem( row, COL_STATUS, MdwPlacedStatusText( check->facts ) );
    }

    for( const MDW_BASELINE_EXCEPTION& exception : m_exceptions )
        m_list->SetItem( exception.part_index, m_exceptionColumn, exceptionText( exception ) );

    sizeColumns();
    growToColumns();
    updateControls();
}


void DIALOG_MDW_DESIGN_RELEASE::sizeColumns( bool aMeasure )
{
    const int last = m_list->GetColumnCount() - 1;
    const int client = m_list->GetClientSize().x;

    // Autosizing measures every row, which a drag resize would repeat for each step
    if( aMeasure || m_contentWidths.size() != static_cast<size_t>( last + 1 ) )
    {
        m_contentWidths.clear();

        for( int col = 0; col <= last; ++col )
        {
            // MSW stretches a last column autosized by its header to fill the list, growing the dialog every refresh
            m_list->SetColumnWidth( col, col == last ? wxLIST_AUTOSIZE : wxLIST_AUTOSIZE_USEHEADER );
            m_contentWidths.push_back( m_list->GetColumnWidth( col ) );
        }
    }

    std::vector<int> widths = m_contentWidths;
    int              used = std::accumulate( widths.begin(), widths.begin() + last, 0 );

    // The exception notes always keep a usable share, so GTK's generous autosizing cannot push them out of view
    const int noteFloor = std::min( std::max( widths[last], FromDIP( 160 ) ), std::max( client / 2, FromDIP( 160 ) ) );

    // Trim the widest column first, since GTK pads short content generously and narrow ones then stay whole
    while( client > 0 && used + noteFloor > client )
    {
        auto widest = std::max_element( widths.begin(), widths.begin() + last );
        int  excess = used + noteFloor - client;
        int  trim = std::min( excess, *widest - FromDIP( 50 ) );

        if( trim <= 0 )
            break;

        *widest -= trim;
        used -= trim;
    }

    for( int col = 0; col < last; ++col )
        m_list->SetColumnWidth( col, widths[col] );

    m_list->SetColumnWidth( last, std::max( noteFloor, client - used ) );
}


void DIALOG_MDW_DESIGN_RELEASE::growToColumns()
{
    const int last = m_list->GetColumnCount() - 1;

    if( m_contentWidths.size() != static_cast<size_t>( last + 1 ) )
        return;

    const int wanted = std::accumulate( m_contentWidths.begin(), m_contentWidths.begin() + last, 0 )
                       + std::max( m_contentWidths[last], FromDIP( 160 ) )
                       + std::max( 0, wxSystemSettings::GetMetric( wxSYS_VSCROLL_X, m_list ) );
    const int grow = wanted - m_list->GetClientSize().x;
    const int width = std::min( GetSize().x + grow, wxDisplay( this ).GetClientArea().GetWidth() );

    if( grow <= 0 || width <= GetSize().x )
        return;

    SetSize( width, GetSize().y );
    Layout();
    sizeColumns( false );

    if( !IsShown() )
        Centre();
}


int DIALOG_MDW_DESIGN_RELEASE::unexcepted( MDW_SEVERITY aSeverity ) const
{
    std::vector<bool> excepted( m_parts.size(), false );

    for( const MDW_BASELINE_EXCEPTION& exception : m_exceptions )
        excepted[exception.part_index] = true;

    int count = 0;

    for( size_t i = 0; i < m_checks.size(); ++i )
    {
        if( !excepted[i] && m_checks[i].severity == aSeverity )
            ++count;
    }

    return count;
}


void DIALOG_MDW_DESIGN_RELEASE::updateControls()
{
    const int  blocking = unexcepted( MDW_SEVERITY::BLOCKING );
    const int  warnings = unexcepted( MDW_SEVERITY::WARNING );
    const bool acting = m_busy || m_committing;

    // The step left before release outranks the check, and an action under way keeps its own line
    if( !acting && m_readiness && m_readiness->step != MDW_RELEASE_STEP::READY )
    {
        setLine( m_summary, m_readiness->summary );
    }
    else if( !acting && m_checked )
    {
        const int parts = static_cast<int>( m_parts.size() );
        wxString  summary = wxString::Format( wxPLURAL( "Releasing revision %s with %d placed part.",
                                                        "Releasing revision %s with %d placed parts.", parts ),
                                              m_revisionLabel, parts );

        if( blocking )
        {
            summary << wxS( " " )
                    << wxString::Format( wxPLURAL( "%d blocked part needs an exception.",
                                                   "%d blocked parts need an exception.", blocking ),
                                         blocking );
        }
        else if( warnings )
        {
            summary << wxS( " " )
                    << wxString::Format( wxPLURAL( "%d part has warnings.", "%d parts have warnings.", warnings ),
                                         warnings );
        }

        setLine( m_summary, summary );
    }

    bool selectedFlagged = false;

    for( long row = m_list->GetFirstSelected(); row >= 0; row = m_list->GetNextSelected( row ) )
    {
        if( static_cast<size_t>( row ) < m_checks.size() )
            selectedFlagged |= m_checks[row].severity != MDW_SEVERITY::NONE;
    }

    // A commit and push ends by reading the parts again, which would renumber the rows under the prompt
    m_exceptionButton->Enable( m_checked && !acting && selectedFlagged );
    m_cancelButton->Enable( !m_busy );
    m_releaseNote->Enable( !m_busy );
    bool reviewBlocks = m_reviewGate && m_reviewGate->blocks;
    wxString label = _( "Release Design" );
    bool     enable = false;

    if( m_readiness && !acting )
    {
        switch( m_readiness->action )
        {
        case MDW_RELEASE_ACTION::NONE: break;

        case MDW_RELEASE_ACTION::SAVE_ALL:
            label = _( "Save All" );
            enable = true;
            break;

        case MDW_RELEASE_ACTION::COMMIT:
            label = workingCopy() ? _( "Commit and Push..." ) : _( "Check In..." );
            enable = m_pane.get() != nullptr && ( workingCopy() || !m_projectRef.item_id.IsEmpty() );
            break;

        case MDW_RELEASE_ACTION::RELEASE: enable = m_checked && blocking == 0 && !reviewBlocks; break;
        }
    }

    if( m_releaseButton->GetLabel() != label )
    {
        m_releaseButton->SetLabel( label );
        Layout();
    }

    m_releaseButton->Enable( enable );
    m_releaseButton->SetToolTip( reviewBlocks ? _( "This design needs an approved review before release." )
                                              : wxString() );
}


void DIALOG_MDW_DESIGN_RELEASE::onRecordException( wxCommandEvent& aEvent )
{
    std::vector<long> rows;

    for( long row = m_list->GetFirstSelected(); row >= 0; row = m_list->GetNextSelected( row ) )
        rows.push_back( row );

    m_prompting = true;
    recordException( rows );
    m_prompting = false;

    if( std::exchange( m_recheck, false ) )
        onConnectionChanged();
}


void DIALOG_MDW_DESIGN_RELEASE::recordException( const std::vector<long>& aRows )
{
    // The change order check may finish under the prompt, and the picked index must name what was offered
    const MDW_DESIGN_ECOS ecos = m_ecos;

    // Offer the change orders on the selected parts, since an exception usually exists because of one
    std::vector<size_t> offered;

    for( long row : aRows )
    {
        if( static_cast<size_t>( row ) >= ecos.parts.size() )
            continue;

        for( size_t index : ecos.parts[row] )
        {
            if( std::find( offered.begin(), offered.end(), index ) == offered.end() )
                offered.push_back( index );
        }
    }

    DIALOG_SHIM dlg( this, wxID_ANY, _( "Record Exception" ) );
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    sizer->Add( new wxStaticText( &dlg, wxID_ANY, _( "Why may the selected parts be released as placed?" ) ), 0, wxALL,
                10 );

    wxTextCtrl* text =
            new wxTextCtrl( &dlg, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( dlg.FromDIP( 400 ), -1 ) );
    sizer->Add( text, 0, wxLEFT | wxRIGHT | wxEXPAND, 10 );

    wxChoice* choice = nullptr;

    if( !offered.empty() )
    {
        wxArrayString labels;
        labels.Add( _( "None" ) );

        for( size_t index : offered )
        {
            const kiapi::mdw::Eco& eco = ecos.ecos[index].eco;
            labels.Add( wxString::FromUTF8( eco.display_id() + "  " + eco.title() ) );
        }

        wxBoxSizer* row = new wxBoxSizer( wxHORIZONTAL );
        row->Add( new wxStaticText( &dlg, wxID_ANY, _( "Change order:" ) ), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );
        choice = new wxChoice( &dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, labels );
        choice->SetSelection( 0 );
        row->Add( choice, 1 );
        sizer->Add( row, 0, wxLEFT | wxRIGHT | wxTOP | wxEXPAND, 10 );
    }

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    wxButton*               ok = new wxButton( &dlg, wxID_OK );
    ok->SetDefault();
    buttons->AddButton( ok );
    buttons->AddButton( new wxButton( &dlg, wxID_CANCEL ) );
    buttons->Realize();
    sizer->Add( buttons, 0, wxALL | wxEXPAND, 10 );

    // A restored reason or change order would apply to the previous parts
    dlg.OptOut( &dlg );
    dlg.SetSizerAndFit( sizer );
    dlg.SetInitialFocus( text );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString note = text->GetValue();

    if( note.Trim().Trim( false ).IsEmpty() )
        return;

    MDW_ECO_REF eco;

    if( choice && choice->GetSelection() > 0 )
    {
        const MDW_SOURCED_ECO& picked = ecos.ecos[offered[choice->GetSelection() - 1]];
        eco = MDW_ECO_REF{ picked.source, wxString::FromUTF8( picked.eco.eco_id() ) };
    }

    for( long row : aRows )
    {
        if( m_checks[row].severity == MDW_SEVERITY::NONE )
            continue;

        auto it = std::find_if( m_exceptions.begin(), m_exceptions.end(),
                                [row]( const MDW_BASELINE_EXCEPTION& aException )
                                {
                                    return aException.part_index == static_cast<unsigned>( row );
                                } );

        if( it == m_exceptions.end() )
        {
            MDW_BASELINE_EXCEPTION added;
            added.part_index = static_cast<unsigned>( row );
            it = m_exceptions.insert( m_exceptions.end(), added );
        }

        it->note = note;
        it->eco = eco;
        m_list->SetItem( row, m_exceptionColumn, exceptionText( *it ) );
        m_list->SetItemImage( row, ICON_NONE );
    }

    sizeColumns();
    updateControls();
}


wxString DIALOG_MDW_DESIGN_RELEASE::exceptionText( const MDW_BASELINE_EXCEPTION& aException ) const
{
    for( const MDW_SOURCED_ECO& known : m_ecos.ecos )
    {
        if( known.source == aException.eco.source && known.eco.eco_id() == aException.eco.eco_id.utf8_string()
            && !aException.eco.eco_id.IsEmpty() )
        {
            return wxString::FromUTF8( known.eco.display_id() ) + wxS( ": " ) + aException.note;
        }
    }

    return aException.note;
}


void DIALOG_MDW_DESIGN_RELEASE::onAction( wxCommandEvent& aEvent )
{
    if( !m_releaseButton->IsEnabled() || !m_readiness )
        return;

    switch( m_readiness->action )
    {
    case MDW_RELEASE_ACTION::NONE: break;
    case MDW_RELEASE_ACTION::SAVE_ALL: saveAll(); break;
    case MDW_RELEASE_ACTION::RELEASE: release(); break;

    case MDW_RELEASE_ACTION::COMMIT:
        if( workingCopy() )
            commitAndPush();
        else
            EndQuasiModal( ID_CHECK_IN );

        break;
    }
}


bool DIALOG_MDW_DESIGN_RELEASE::workingCopy() const
{
    return m_source->Info().backend.checkin_model == MDW_CHECKIN_MODEL::WORKING_COPY;
}


void DIALOG_MDW_DESIGN_RELEASE::saveAll()
{
    if( SaveDocumentEditors( m_frame ) )
        reloadParts();
}


void DIALOG_MDW_DESIGN_RELEASE::reloadParts()
{
    // The read's progress and error windows run events, so the poll must not start a second read under it
    SCOPED_SET_RESET<bool> reading( m_reading, true );

    m_stale = false;

    std::vector<MDW_PLACED_INPUT>                      inputs;
    const std::vector<std::filesystem::file_time_type> times = MdwDesignWriteTimes( m_project );

    // The list no longer matches the files, so nothing may be released from it until they are read
    m_checked = false;
    m_readiness.reset();
    updateControls();

    // The saved files are what a release names, so the list follows them
    if( wxString unread = MdwCollectPlacedParts( this, m_frame->Kiway(), m_project, inputs ); !unread.IsEmpty() )
    {
        m_failedAt = times;
        setLine( m_summary, unread );
        DisplayErrorMessage( this, unread );
        return;
    }

    m_savedAt = times;
    setParts( inputs );
    startCheck();
}


void DIALOG_MDW_DESIGN_RELEASE::recheckIfChanged( bool aActivated )
{
    // A recheck would rebuild the rows a prompt, a read or an action under way works on
    if( m_busy || m_committing || m_prompting || m_reading )
        return;

    const std::vector<std::filesystem::file_time_type> times = MdwDesignWriteTimes( m_project );

    if( times != m_savedAt )
    {
        // A failed read waits for the files to change again, since its own error window reactivates the dialog
        if( times == m_failedAt )
            return;

        if( aActivated )
        {
            reloadParts();
        }
        else if( !m_stale )
        {
            // The read's progress window would pop up over the editor on every save, so it waits for a return
            m_stale = true;
            m_op.Cancel();
            m_checked = false;
            m_readiness.reset();
            setLine( m_summary, _( "The design was saved. It is checked again when you return to this window." ) );
            updateControls();
        }

        return;
    }

    if( !m_op.IsDone() )
        return;

    // Lifecycle changes and commits made elsewhere leave the files alone, so a return always checks again
    if( aActivated || DocumentEditorsDirty( m_frame->Kiway() ) != m_editorsDirty )
        startCheck();
}


void DIALOG_MDW_DESIGN_RELEASE::commitAndPush()
{
    GIT_COMMIT_PANE* pane = m_pane.get();

    if( !pane )
        return;

    const wxString message = wxString::Format( _( "Prepare release of %s" ), m_project.GetProjectName() );

    if( !IsOK( this,
               wxString::Format( _( "Commit every changed file in the project as \"%s\" and push?" ), message ) ) )
    {
        return;
    }

    m_committing = true;
    setLine( m_summary, _( "Committing and pushing the design..." ) );
    updateControls();

    wxWeakRef<DIALOG_MDW_DESIGN_RELEASE> self( this );

    pane->CommitAllAndPush( message,
                            [self]()
                            {
                                // The pane is still inside the step that ended the commit and push
                                if( self )
                                {
                                    self->CallAfter(
                                            [self]()
                                            {
                                                // Getting a teammate's changes to push over can rewrite the saved files
                                                if( self )
                                                {
                                                    self->m_committing = false;
                                                    self->reloadParts();
                                                }
                                            } );
                                }
                            } );
}


void DIALOG_MDW_DESIGN_RELEASE::release()
{
    const std::vector<MDW_BASELINE_CHECK> shown = m_checks;

    m_busy = true;
    setLine( m_summary, _( "Checking placed parts against their releases..." ) );
    updateControls();

    // The backend trusts the facts other sources gave, so they are read again right before it gets them
    m_op = MdwCheckDesign(
            m_source, m_projectRef, m_revisionId, m_parts,
            [this, shown]( const MDW_RESULT& aResult, const MDW_DESIGN_CHECK& aCheck )
            {
                if( !aResult.IsOk() )
                {
                    m_busy = false;
                    updateControls();
                    DisplayErrorMessage(
                            this, _( "The design was not released." ),
                            wxString::Format( _( "Could not check the placed parts: %s" ), MdwErrorText( aResult ) ) );
                    return;
                }

                m_parts = aCheck.parts;
                m_checks = aCheck.checks;
                fill();
                showReadiness( aCheck.hold, aCheck.holder );

                bool worse = m_checks.size() != shown.size();

                for( size_t i = 0; !worse && i < m_checks.size(); ++i )
                    worse = m_checks[i].severity > shown[i].severity;

                // A part flagged since the user last looked is shown before anything is released
                if( worse || m_readiness->step != MDW_RELEASE_STEP::READY || unexcepted( MDW_SEVERITY::BLOCKING ) > 0 )
                {
                    m_busy = false;
                    updateControls();
                    DisplayInfoMessage( this, _( "The placed parts changed since they were checked. Review them "
                                                 "before releasing." ) );
                    return;
                }

                createBaseline();
            } );
}


void DIALOG_MDW_DESIGN_RELEASE::createBaseline()
{
    MDW_BASELINE_REQUEST request;
    request.project = m_projectRef;
    request.revision_id = m_revisionId;
    request.exceptions = m_exceptions;
    request.comment = m_releaseNote->GetValue().Strip( wxString::both );

    for( const MDW_DESIGN_PART& part : m_parts )
        request.parts.push_back( part.baseline );

    m_busy = true;
    setLine( m_summary, _( "Releasing the design..." ) );

    m_op = m_source->Baselines().Create(
            request,
            [this]( const MDW_RESULT& aResult, const MDW_CHANGE& aChange )
            {
                m_busy = false;

                if( !aResult.IsOk() )
                {
                    updateControls();
                    DisplayErrorMessage( this, _( "The design was not released." ), MdwErrorText( aResult ) );
                    return;
                }

                if( aChange.outcome == MDW_CHANGE_OUTCOME::PENDING )
                {
                    const MDW_PENDING_CHANGE& pending = aChange.pending;
                    wxString                  detail = pending.reason;

                    if( !pending.url.IsEmpty() )
                        detail << wxS( "\n" ) << pending.url;

                    DisplayInfoMessage( this,
                                        pending.display_id.IsEmpty()
                                                ? wxString( _( "The release is waiting for approval." ) )
                                                : wxString::Format( _( "The release is waiting for approval "
                                                                       "in %s." ),
                                                                    pending.display_id ),
                                        detail );
                }
                else
                {
                    DisplayInfoMessage( this, wxString::Format( _( "Released the design as %s." ),
                                                                MdwRevisionText( aChange.revision ) ) );
                }

                EndQuasiModal( wxID_OK );
            } );

    updateControls();
}


void ShowDesignRelease( EDA_BASE_FRAME* aFrame, GIT_COMMIT_PANE* aPane )
{
    PROJECT&                    project = aFrame->Prj();
    std::shared_ptr<MDW_SOURCE> source = MDW_CLIENT::Get().ProjectSource();

    if( !source )
    {
        DisplayErrorMessage( aFrame, _( "The project is not in a workspace." ) );
        return;
    }

    if( !source->Has( MDW_CAPABILITY_GROUP::REVISIONS ) )
    {
        DisplayErrorMessage( aFrame,
                             wxString::Format( _( "%s cannot release designs." ), source->Info().display_name ) );
        return;
    }

    std::vector<MDW_PLACED_INPUT> inputs;

    if( wxString unread = MdwCollectPlacedParts( aFrame, aFrame->Kiway(), project, inputs ); !unread.IsEmpty() )
    {
        DisplayErrorMessage( aFrame, unread );
        return;
    }

    // Quasi-modal keeps the project open under the dialog while the editors stay free to change the design
    DIALOG_MDW_DESIGN_RELEASE dlg( aFrame, aPane, project, source, std::move( inputs ) );
    int                       result = dlg.ShowQuasiModal();

    if( result == ID_CHECK_IN && aPane )
        aPane->ShowWorkspaceItem( dlg.DesignItem().item_id );
}
