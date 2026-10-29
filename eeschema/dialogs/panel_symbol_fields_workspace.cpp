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

#include "panel_symbol_fields_workspace.h"

#include <dialogs/git/dialog_mdw_change_orders.h>
#include <dialogs/git/workspace_search_query.h>
#include <grid_tricks.h>
#include <mdw/client/mdw_baseline_design.h>
#include <mdw/client/mdw_paged_read.h>
#include <mdw/client/mdw_part_catalog.h>
#include <mdw/client/mdw_source.h>
#include <string_utils.h>
#include <widgets/mdw_change_orders.h>
#include <widgets/ui_common.h>
#include <widgets/wx_grid.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/utils.h>
#include <wx/datetime.h>
#include <wx/intl.h>

#include <algorithm>


namespace
{

enum COLUMN
{
    COL_STATUS,
    COL_DESIGN,
    COL_DESIGN_QTY,
    COL_WORKSPACE,
    COL_PART,
    COL_WORKSPACE_QTY,
    COL_COUNT
};

/// Members per Resolve request, as the contract allows; the rest follow in later requests
constexpr size_t MAX_RESOLVE = 1000;


/// One sentence for a failed read; the backend's reason may lack its own full stop
wxString readFailure( const wxString& aWhat, const MDW_RESULT& aResult, const wxString& aSourceName )
{
    // Both reads fail together offline, and showActivity says it once
    if( WorkspaceSearchOffline( aResult ) )
        return wxString::Format( _( "%s is offline." ), aSourceName );

    wxString reason = MdwErrorMessage( aResult );

    if( !reason.IsEmpty() && !wxString( wxS( ".!?" ) ).Contains( reason.Last() ) )
        reason << wxS( "." );

    return aWhat + wxS( " " ) + reason;
}


wxString statusText( MDW_BOM_STATUS aStatus )
{
    switch( aStatus )
    {
    case MDW_BOM_STATUS::MATCHES: return _( "Matches" );
    case MDW_BOM_STATUS::DIFFERENT_PART: return _( "Different part" );
    case MDW_BOM_STATUS::PART_UNKNOWN: return _( "Part unknown" );
    case MDW_BOM_STATUS::QUANTITY_DIFFERS: return _( "Quantity differs" );
    case MDW_BOM_STATUS::NOT_IN_WORKSPACE: return _( "Not in workspace BOM" );
    case MDW_BOM_STATUS::NOT_IN_DESIGN: return _( "Not in design" );
    }

    return wxEmptyString;
}


wxString quantityText( double aQuantity, const wxString& aUnit )
{
    wxString text = wxString::Format( wxS( "%g" ), aQuantity );

    return aUnit.IsEmpty() ? text : text + wxS( " " ) + aUnit;
}


wxString memberKey( const MDW_ITEM_REF& aMember )
{
    return aMember.source + wxS( "\n" ) + aMember.item_id;
}

} // namespace


PANEL_SYMBOL_FIELDS_WORKSPACE::PANEL_SYMBOL_FIELDS_WORKSPACE( wxWindow* aParent, std::shared_ptr<MDW_SOURCE> aSource,
                                                              const MDW_PROJECT_STATE& aProjectState,
                                                              const MDW_RESULT& aLink, const wxString& aProjectName,
                                                              DESIGN_PROVIDER aDesign, CROSS_PROBE aCrossProbe ) :
        wxPanel( aParent ),
        m_source( std::move( aSource ) ),
        m_projectState( aProjectState ),
        m_link( aLink ),
        m_projectName( aProjectName ),
        m_project( aProjectState.item ),
        m_design( std::move( aDesign ) ),
        m_crossProbe( std::move( aCrossProbe ) )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* headerSizer = new wxBoxSizer( wxHORIZONTAL );

    m_header = new wxStaticText( this, wxID_ANY, wxEmptyString );
    headerSizer->Add( m_header, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

    m_refresh = new wxButton( this, wxID_ANY, _( "Refresh" ) );
    headerSizer->Add( m_refresh, 0, wxALIGN_CENTER_VERTICAL );
    mainSizer->Add( headerSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5 );

    // The link gets its own line so a long activity sentence never pushes it off the page
    wxBoxSizer* activitySizer = new wxBoxSizer( wxVERTICAL );

    // A wrapped label's best width is the width it last wrapped to, which would stop the page shrinking
    m_activity = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_activity->SetMinSize( wxSize( FromDIP( 200 ), -1 ) );
    activitySizer->Add( m_activity, 0, wxEXPAND | wxBOTTOM, 2 );

    m_activityLink = new wxHyperlinkCtrl( this, wxID_ANY, _( "Change Orders and Reviews..." ), wxEmptyString,
                                          wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
    activitySizer->Add( m_activityLink, 0 );
    mainSizer->Add( activitySizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5 );

    wxBoxSizer* filterSizer = new wxBoxSizer( wxHORIZONTAL );

    m_differencesOnly = new wxCheckBox( this, wxID_ANY, _( "Show differences only" ) );
    m_differencesOnly->SetValue( true );
    filterSizer->Add( m_differencesOnly, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10 );
    filterSizer->AddStretchSpacer();

    m_summary = new wxStaticText( this, wxID_ANY, wxEmptyString );
    filterSizer->Add( m_summary, 0, wxALIGN_CENTER_VERTICAL );
    mainSizer->Add( filterSizer, 0, wxEXPAND | wxALL, 5 );

    wxBoxSizer* messageSizer = new wxBoxSizer( wxHORIZONTAL );

    m_message = new wxStaticText( this, wxID_ANY, wxEmptyString );
    messageSizer->Add( m_message, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

    m_retry = new wxHyperlinkCtrl( this, wxID_ANY, _( "Retry" ), wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                   wxHL_DEFAULT_STYLE );
    messageSizer->Add( m_retry, 0, wxALIGN_CENTER_VERTICAL );
    mainSizer->Add( messageSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    m_grid = new WX_GRID( this, wxID_ANY );
    m_grid->CreateGrid( 0, COL_COUNT );
    m_grid->EnableEditing( false );
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_grid->SetRowLabelSize( 0 );
    m_grid->SetColLabelValue( COL_STATUS, _( "Status" ) );
    m_grid->SetColLabelValue( COL_DESIGN, _( "Design" ) );
    m_grid->SetColLabelValue( COL_DESIGN_QTY, _( "Design Qty" ) );
    m_grid->SetColLabelValue( COL_WORKSPACE, _( "Workspace" ) );
    m_grid->SetColLabelValue( COL_PART, _( "Workspace Part" ) );
    m_grid->SetColLabelValue( COL_WORKSPACE_QTY, _( "Workspace Qty" ) );
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    // The grid scrolls, so its full column width must not become the dialog's minimum
    m_grid->SetMinSize( FromDIP( wxSize( 300, 150 ) ) );
    mainSizer->Add( m_grid, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    SetSizer( mainSizer );

    m_refresh->Bind( wxEVT_BUTTON, &PANEL_SYMBOL_FIELDS_WORKSPACE::onRefresh, this );
    m_retry->Bind( wxEVT_HYPERLINK, &PANEL_SYMBOL_FIELDS_WORKSPACE::onRetry, this );
    m_activityLink->Bind( wxEVT_HYPERLINK, &PANEL_SYMBOL_FIELDS_WORKSPACE::onActivityLink, this );
    Bind( wxEVT_SIZE, &PANEL_SYMBOL_FIELDS_WORKSPACE::onSize, this );

    // A visited link's theme colour can be unreadable in dark mode, and these links are actions
    for( wxHyperlinkCtrl* link : { m_activityLink, m_retry } )
        link->SetVisitedColour( link->GetNormalColour() );

    m_differencesOnly->Bind( wxEVT_CHECKBOX, &PANEL_SYMBOL_FIELDS_WORKSPACE::onDifferencesOnly, this );
    m_grid->Bind( wxEVT_GRID_SELECT_CELL, &PANEL_SYMBOL_FIELDS_WORKSPACE::onSelectCell, this );
    m_grid->Bind( wxEVT_GRID_RANGE_SELECTED, &PANEL_SYMBOL_FIELDS_WORKSPACE::onRangeSelected, this );

    // Filling and showing the grid move its cursor too, and only the user's selection may move the schematic
    m_grid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &PANEL_SYMBOL_FIELDS_WORKSPACE::onUserSelecting, this );
    m_grid->Bind( wxEVT_GRID_LABEL_LEFT_CLICK, &PANEL_SYMBOL_FIELDS_WORKSPACE::onUserSelecting, this );
    m_grid->Bind( wxEVT_KEY_DOWN,
                  [this]( wxKeyEvent& aEvent )
                  {
                      m_userSelecting = true;
                      aEvent.Skip();
                  } );

    showActivity();
    showState( STATE::LOADING );
}


PANEL_SYMBOL_FIELDS_WORKSPACE::~PANEL_SYMBOL_FIELDS_WORKSPACE()
{
    m_op.Cancel();
    m_ecoOp.Cancel();
    m_reviewOp.Cancel();
    m_grid->PopEventHandler( true );
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::Detach()
{
    m_op.Cancel();
    m_ecoOp.Cancel();
    m_reviewOp.Cancel();
    m_design = {};
    m_crossProbe = {};
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::Activate()
{
    if( m_activated )
        return;

    m_activated = true;
    fetch();
    fetchActivity();
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::DesignChanged()
{
    // An offline or failed page keeps its message until a successful read replaces it
    if( m_state == STATE::LOADED && m_bom && m_op.IsDone() )
        resolveParts();
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::fetch()
{
    // A reload also retries members whose earlier lookup failed
    m_op.Cancel();
    m_candidates.clear();

    // The server would answer for no design at all, which reads as a product without a BOM
    if( m_project.item_id.IsEmpty() )
    {
        // Only the client's own not-found means unlinked; an offline backend never got to say
        if( m_link.IsOk() || m_link.error == MDW_ERROR_CODE::ITEM_NOT_FOUND )
            showState( STATE::UNLINKED );
        else
            onBom( m_link, MDW_BOM_DATA() );

        return;
    }

    showState( STATE::LOADING );

    m_op = m_source->Bom().Get( m_project, wxEmptyString,
                                [this]( const MDW_RESULT& aResult, const MDW_BOM_DATA& aBom )
                                {
                                    onBom( aResult, aBom );
                                } );
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::onBom( const MDW_RESULT& aResult, const MDW_BOM_DATA& aBom )
{
    if( aResult.IsOk() )
    {
        m_bom = aBom;
        m_checked = wxDateTime::Now();
        resolveParts();
    }
    else if( aResult.error == MDW_ERROR_CODE::ITEM_NOT_FOUND )
    {
        m_bom.reset();
        showState( STATE::EMPTY );
    }
    else if( WorkspaceSearchOffline( aResult ) )
    {
        showState( STATE::OFFLINE );
    }
    else if( aResult.error == MDW_ERROR_CODE::PERMISSION_DENIED )
    {
        showState( STATE::DENIED, aResult.message );
    }
    else
    {
        showState( STATE::FAILED, MdwErrorMessage( aResult ) );
    }
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::resolveParts()
{
    m_designGroups = m_design ? m_design() : std::vector<std::vector<MDW_DESIGN_SYMBOL>>();

    std::vector<MDW_ITEM_REF> unresolved;

    for( std::vector<MDW_DESIGN_SYMBOL>& group : m_designGroups )
    {
        for( MDW_DESIGN_SYMBOL& symbol : group )
        {
            if( symbol.part || !symbol.member )
                continue;

            auto it = m_candidates.find( memberKey( *symbol.member ) );

            if( it != m_candidates.end() )
                symbol.candidates = it->second;
            else if( std::none_of( unresolved.begin(), unresolved.end(),
                                   [&]( const MDW_ITEM_REF& aMember )
                                   {
                                       return memberKey( aMember ) == memberKey( *symbol.member );
                                   } ) )
                unresolved.push_back( *symbol.member );
        }
    }

    if( unresolved.size() > MAX_RESOLVE )
        unresolved.resize( MAX_RESOLVE );

    if( unresolved.empty() || !m_source->Has( MDW_CAPABILITY_GROUP::PARTS ) )
    {
        compare();
        return;
    }

    m_op = m_source->Parts().Resolve(
            unresolved,
            [this, unresolved]( const MDW_RESULT& aResult, const std::vector<MDW_PART_CANDIDATES>& aParts )
            {
                // A failed lookup leaves the symbols "Part unknown", which is still a useful comparison
                for( const MDW_ITEM_REF& member : unresolved )
                    m_candidates[memberKey( member )];

                // Answers follow the request's order, and a returned member may name its source differently
                if( aResult.IsOk() )
                {
                    for( size_t ii = 0; ii < std::min( aParts.size(), unresolved.size() ); ++ii )
                    {
                        std::vector<MDW_ITEM_REF>& candidates = m_candidates[memberKey( unresolved[ii] )];

                        for( const MDW_PART_INFO& part : aParts[ii].candidates )
                            candidates.push_back( part.part );
                    }
                }

                resolveParts();
            } );
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::compare()
{
    m_rows = MdwCompareBom( m_designGroups, *m_bom, m_source->Info().locator );
    showState( STATE::LOADED );
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::showState( STATE aState, const wxString& aDetail )
{
    const wxString source = m_source->Info().Name();
    wxString       message;
    bool           retry = false;
    bool           grid = false;

    m_state = aState;

    m_refresh->Enable( aState != STATE::LOADING );
    m_differencesOnly->Enable( aState == STATE::LOADED );
    m_grid->Enable( aState == STATE::LOADED );

    switch( aState )
    {
    case STATE::LOADING:
        m_header->SetLabel( wxEmptyString );
        message = _( "Reading the workspace BOM…" );
        break;

    case STATE::EMPTY:
        m_header->SetLabel( wxEmptyString );
        message = wxString::Format( _( "%s has no BOM for this project yet." ), source );
        break;

    case STATE::UNLINKED:
        m_header->SetLabel( wxEmptyString );
        message = _( "This project is not linked to a workspace item." );
        break;

    case STATE::OFFLINE:
        if( m_bom )
        {
            // The last good comparison stays readable, greyed
            m_header->SetLabel(
                    wxString::Format( _( "Offline · last checked %s" ), m_checked.Format( wxS( "%H:%M" ) ) ) );
            grid = true;
        }
        else
        {
            m_header->SetLabel( wxEmptyString );
            message = wxString::Format( _( "%s is offline." ), source );
        }

        retry = true;
        break;

    case STATE::DENIED:
        m_header->SetLabel( wxEmptyString );
        message = wxString::Format( _( "You do not have access to BOMs in %s." ), source );

        if( !aDetail.IsEmpty() )
            message << wxS( " " ) << aDetail;

        break;

    case STATE::FAILED:
        m_header->SetLabel( wxEmptyString );
        message = _( "Could not read the workspace BOM." ) + wxS( " " ) + aDetail;
        retry = true;
        break;

    case STATE::LOADED:
    {
        wxString header = wxString::Format( _( "Workspace BOM: %s" ),
                                            m_bom->bom_type.IsEmpty() ? m_bom->label : m_bom->bom_type );

        if( !m_bom->bom_type.IsEmpty() && !m_bom->label.IsEmpty() )
            header << wxS( " · " ) << m_bom->label;

        if( m_bom->updated.IsValid() && !m_bom->updated_by.IsEmpty() )
        {
            header << wxS( " · " )
                   << wxString::Format( _( "updated %s by %s" ), m_bom->updated.FormatISODate(), m_bom->updated_by );
        }
        else if( m_bom->updated.IsValid() )
        {
            header << wxS( " · " ) << wxString::Format( _( "updated %s" ), m_bom->updated.FormatISODate() );
        }

        m_header->SetLabel( header );
        fillGrid();
        return;
    }
    }

    m_summary->SetLabel( wxEmptyString );
    m_message->SetLabel( message );
    m_message->Show( !message.IsEmpty() );
    m_retry->Show( retry );
    m_grid->Show( grid );
    Layout();
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::fillGrid()
{
    m_userSelecting = false;

    MDW_BOM_VIEW view = MdwBomView( m_rows, m_differencesOnly->GetValue() );

    m_shownRows = view.shown;

    m_grid->ClearRows();

    if( !m_shownRows.empty() )
        m_grid->AppendRows( static_cast<int>( m_shownRows.size() ) );

    for( size_t gridRow = 0; gridRow < m_shownRows.size(); ++gridRow )
    {
        const MDW_BOM_ROW&  row = m_rows[m_shownRows[gridRow]];
        const MDW_BOM_LINE* line = row.line >= 0 ? &m_bom->lines[row.line] : nullptr;
        int                 r = static_cast<int>( gridRow );

        m_grid->SetCellValue( r, COL_STATUS, statusText( row.status ) );
        m_grid->SetCellValue( r, COL_DESIGN, AccumulateDescriptions( row.design_refs ) );
        m_grid->SetCellValue( r, COL_WORKSPACE, AccumulateDescriptions( row.workspace_refs ) );

        if( !row.design_refs.empty() )
            m_grid->SetCellValue( r, COL_DESIGN_QTY,
                                  wxString::Format( wxS( "%d" ), static_cast<int>( row.design_refs.size() ) ) );

        if( !line )
            continue;

        m_grid->SetCellValue( r, COL_PART, line->part_number.IsEmpty() ? line->part.path_hint : line->part_number );
        m_grid->SetCellValue( r, COL_WORKSPACE_QTY, quantityText( row.workspace_quantity, line->unit ) );
    }

    wxString message;

    // Hidden unidentified parts are not a match, so the all-clear waits for every part to be known
    if( m_shownRows.empty() && m_differencesOnly->GetValue() && view.unknown > 0 )
    {
        message = wxString::Format( _( "No known differences. %d parts could not be identified." ), view.unknown );
    }
    else if( m_shownRows.empty() && m_differencesOnly->GetValue() )
    {
        wxString when = m_bom->updated.IsValid() ? m_bom->updated.FormatISODate() : wxString();
        wxString name = m_bom->bom_type.IsEmpty() ? m_bom->label : m_bom->bom_type;

        message = when.IsEmpty() ? wxString::Format( _( "The design matches the workspace BOM (%s)." ), name )
                                 : wxString::Format( _( "The design matches the workspace BOM (%s, updated %s)." ),
                                                     name, when );
    }

    if( view.unknown > 0 )
    {
        m_summary->SetLabel( wxString::Format( _( "%d differences, %d unknown in %d references" ), view.differences,
                                               view.unknown, view.references ) );
    }
    else
    {
        m_summary->SetLabel(
                wxString::Format( _( "%d differences in %d references" ), view.differences, view.references ) );
    }

    m_message->SetLabel( message );
    m_message->Show( !message.IsEmpty() );
    m_retry->Hide();
    m_grid->Show( message.IsEmpty() );
    m_grid->AutoSizeColumns( false );
    Layout();
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::fetchActivity()
{
    m_ecoNote.clear();
    m_reviewNote.clear();
    showActivity();

    if( m_source->Has( MDW_CAPABILITY_GROUP::ECO ) )
        fetchEcoActivity();

    if( !m_source->Has( MDW_CAPABILITY_GROUP::REVIEW ) )
        return;

    m_reviewOp = MdwReadReviews( m_source, m_project, m_projectState.base_revision_id,
                         [this]( const MDW_RESULT& aResult, const MDW_REVIEW_READ& aRead )
                         {
                             if( !aResult.IsOk() )
                                 m_reviewNote = readFailure( _( "Could not read reviews." ), aResult,
                                                             m_source->Info().Name() );
                             else if( aRead.partial )
                                 m_reviewNote = _( "Too many reviews to summarize here." );
                             else if( !aRead.reviews.empty() )
                                 m_reviewNote = MdwReviewStatusLine( aRead.reviews, m_projectState.base_revision_id,
                                                                     m_source->Info().Name() );

                             showActivity();
                         },
                         MDW_READ_CAP::PARTIAL );
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::fetchEcoActivity()
{
    // A missing sentence would read as no change orders, so a failed read says so
    m_ecoOp = MdwReadAllPages<kiapi::mdw::Eco>(
            [this]( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<kiapi::mdw::Eco>> aDone )
            {
                return m_source->Ecos().List( { m_project }, false, aPage, std::move( aDone ) );
            },
            0,
            [this]( const MDW_RESULT& aResult, const MDW_PAGED<kiapi::mdw::Eco>& aRead )
            {
                if( !aResult.IsOk() )
                    m_ecoNote = readFailure( _( "Could not read change orders." ), aResult, m_source->Info().Name() );
                else if( !aRead.next_page_token.IsEmpty() ) // A partial list would undercount
                    m_ecoNote = _( "Too many change orders to list here." );
                else
                    m_ecoNote = MdwDesignEcoNote( MdwOpenEcos( aRead.rows ) );

                showActivity();
            },
            MDW_READ_CAP::PARTIAL );
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::showActivity()
{
    wxString text = m_ecoNote;

    if( !m_reviewNote.IsEmpty() && m_reviewNote != m_ecoNote )
        text << ( text.IsEmpty() ? wxString() : wxString( wxS( " " ) ) ) << m_reviewNote;

    m_activityText = text;
    wrapActivity();
    m_activity->Show( !text.IsEmpty() );
    m_activityLink->Show( m_source->Has( MDW_CAPABILITY_GROUP::ECO ) || m_source->Has( MDW_CAPABILITY_GROUP::REVIEW ) );
    Layout();
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::wrapActivity()
{
    m_activity->SetLabel( m_activityText );

    int width = GetClientSize().x - FromDIP( 10 );

    if( width > 0 )
    {
        // wx 3.3 skips Wrap at its last width even when the label has changed since
        m_activity->Wrap( -1 );
        m_activity->Wrap( width );
    }
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::onSize( wxSizeEvent& aEvent )
{
    // Rewrap before the default handler lays the page out at the new width
    wrapActivity();
    aEvent.Skip();
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::onActivityLink( wxHyperlinkEvent& aEvent )
{
    DIALOG_MDW_CHANGE_ORDERS::Open( this, m_source,
                                    MdwDesignSubject( m_projectName, m_projectState,
                                                      MDW_CHANGE_ORDERS_PAGE::CHANGE_ORDERS ) );
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::onRefresh( wxCommandEvent& aEvent )
{
    fetch();
    fetchActivity();
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::onRetry( wxHyperlinkEvent& aEvent )
{
    fetch();
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::onDifferencesOnly( wxCommandEvent& aEvent )
{
    if( m_bom )
        fillGrid();
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::onSelectCell( wxGridEvent& aEvent )
{
    aEvent.Skip();

    // The selection settles after this event, so the probe reads it afterwards
    if( m_userSelecting )
        CallAfter( &PANEL_SYMBOL_FIELDS_WORKSPACE::probeSelection );
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::onRangeSelected( wxGridRangeSelectEvent& aEvent )
{
    aEvent.Skip();

    if( m_userSelecting )
        CallAfter( &PANEL_SYMBOL_FIELDS_WORKSPACE::probeSelection );
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::onUserSelecting( wxGridEvent& aEvent )
{
    m_userSelecting = true;
    aEvent.Skip();
}


void PANEL_SYMBOL_FIELDS_WORKSPACE::probeSelection()
{
    // A drag keeps extending the selection until the button comes up
    m_userSelecting = wxGetMouseState().LeftIsDown();

    if( !m_crossProbe )
        return;

    wxArrayInt            rows = m_grid->GetSelectedRows();
    std::vector<wxString> paths;

    if( rows.empty() && m_grid->GetGridCursorRow() >= 0 )
        rows.push_back( m_grid->GetGridCursorRow() );

    for( int row : rows )
    {
        if( row < 0 || row >= static_cast<int>( m_shownRows.size() ) )
            continue;

        const std::vector<wxString>& rowPaths = m_rows[m_shownRows[row]].design_paths;
        paths.insert( paths.end(), rowPaths.begin(), rowPaths.end() );
    }

    m_crossProbe( paths );
}
