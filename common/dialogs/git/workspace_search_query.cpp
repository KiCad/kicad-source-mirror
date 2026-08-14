/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/git/workspace_search_query.h>

#include <mdw/client/mdw_source.h>

#include <wx/intl.h>
#include <wx/numformatter.h>
#include <wx/datetime.h>

#include <algorithm>
#include <utility>


namespace
{

wxString searching()
{
    return _( "Searching" ) + wxS( "…" );
}


wxString count( uint64_t aValue )
{
    return wxNumberFormatter::ToString( static_cast<wxULongLong_t>( aValue ),
                                        wxNumberFormatter::Style_WithThousandsSep );
}


wxString offlineText( const wxDateTime& aLastGood )
{
    if( !aLastGood.IsValid() )
        return _( "Offline." );

    return wxString::Format( _( "Offline · last checked %s" ), aLastGood.Format( wxS( "%H:%M" ) ) );
}

/// The status line for aResult
WORKSPACE_SEARCH_QUERY::STATUS searchStatus( const MDW_RESULT& aResult, const WORKSPACE_SEARCH_VIEW& aView )
{
    WORKSPACE_SEARCH_QUERY::STATUS status;
    const size_t           filters = aView.query.filters.size();

    status.result = aResult;
    status.total_count = aView.total;

    if( aResult.IsOk() )
    {
        if( aView.shown == 0 )
        {
            if( !aView.query.text.IsEmpty() )
                status.text = wxString::Format( _( "No items match '%s'." ), aView.query.text );
            else if( filters )
                status.text = _( "No items match these filters." );
            else
                status.text = wxString::Format( _( "%s has no items." ), aView.source_name );

            return status;
        }

        if( aView.total && *aView.total > aView.shown )
        {
            status.text = wxString::Format( _( "%s of %s items" ), count( aView.shown ), count( *aView.total ) );
        }
        else
        {
            status.text = wxString::Format( wxPLURAL( "%s item", "%s items", aView.shown ), count( aView.shown ) );
        }

        if( filters )
        {
            status.text << wxS( " · " )
                        << wxString::Format( wxPLURAL( "%zu filter", "%zu filters", filters ), filters );
        }

        return status;
    }

    switch( aResult.error )
    {
    case MDW_ERROR_CODE::RATE_LIMITED:
    case MDW_ERROR_CODE::BUSY:
        // The provider retries on a timer, so the request is still in flight
        status.text = searching();
        status.searching = true;
        break;

    case MDW_ERROR_CODE::NETWORK:
    case MDW_ERROR_CODE::TIMEOUT:
    case MDW_ERROR_CODE::BACKEND_LOST:
        status.stale = aView.shown > 0;
        status.text = offlineText( aView.last_good );
        break;

    case MDW_ERROR_CODE::PERMISSION_DENIED:
        status.text = wxString::Format( _( "You do not have access to search %s." ), aView.source_name );

        if( !aResult.message.IsEmpty() )
            status.text << wxS( " " ) << aResult.message;

        break;

    default:
        status.text = wxString::Format( _( "Could not search %s." ), aView.source_name );
        status.can_retry = true;
        break;
    }

    return status;
}

} // namespace


bool WorkspaceSearchOffline( const MDW_RESULT& aResult )
{
    return aResult.error == MDW_ERROR_CODE::NETWORK || aResult.error == MDW_ERROR_CODE::TIMEOUT
           || aResult.error == MDW_ERROR_CODE::BACKEND_LOST;
}


bool WorkspaceSearchRefreshes( const WORKSPACE_SEARCH_QUERY::STATUS& aShown, MDW_CONNECTION aState )
{
    return aState == MDW_CONNECTION::ONLINE && WorkspaceSearchOffline( aShown.result );
}


WORKSPACE_SEARCH_QUERY::STATUS WorkspaceOfflineStatus( const WORKSPACE_SEARCH_QUERY::STATUS& aShown, bool aHasRows,
                                                       const wxDateTime& aLastGood )
{
    WORKSPACE_SEARCH_QUERY::STATUS status;

    // Recorded as a network failure so the page refreshes once the project is online again
    status.result = MDW_RESULT::Fail( MDW_ERROR_CODE::NETWORK );
    status.total_count = aShown.total_count;
    status.stale = aHasRows;
    status.text = offlineText( aLastGood );
    return status;
}


WORKSPACE_SEARCH_QUERY::WORKSPACE_SEARCH_QUERY( std::shared_ptr<MDW_SOURCE> aSource ) :
        m_source( std::move( aSource ) ),
        m_search( m_source->Search() )
{
    m_view.source_name = m_source->Info().display_name;

    m_debounce.Bind( wxEVT_TIMER,
                     [this]( wxTimerEvent& )
                     {
                         run( true );
                     } );

    m_retry.Bind( wxEVT_TIMER,
                  [this]( wxTimerEvent& )
                  {
                      run( m_retryFirstPage );
                  } );
}


WORKSPACE_SEARCH_QUERY::~WORKSPACE_SEARCH_QUERY()
{
    Cancel();
}


std::unique_ptr<WORKSPACE_SEARCH_QUERY> WORKSPACE_SEARCH_QUERY::Create( const std::shared_ptr<MDW_SOURCE>& aSource )
{
    if( !Offers( aSource ) )
        return nullptr;

    return std::make_unique<WORKSPACE_SEARCH_QUERY>( aSource );
}


bool WORKSPACE_SEARCH_QUERY::Offers( const std::shared_ptr<MDW_SOURCE>& aSource )
{
    return aSource && aSource->Has( MDW_CAPABILITY_GROUP::SEARCH );
}


wxString WORKSPACE_SEARCH_QUERY::Hint()
{
    return _( "Search documents" );
}


void WORKSPACE_SEARCH_QUERY::Begin( const MDW_SEARCH_QUERY& aQuery, ROWS aRows )
{
    m_rows = std::move( aRows );

    // Only the query whose rows are on the page may count them as its own
    const bool own = aQuery == m_displayed.query;

    m_view.shown = own ? m_displayed.shown : 0;
    m_view.total = own ? m_displayed.total : std::nullopt;
    m_view.query = aQuery;
    m_op.Cancel();
    m_inFlight = false;
    m_retry.Stop();

    // Every keystroke lands here; only a pause in typing reaches the source
    m_debounce.StartOnce( DEBOUNCE_MS );
}


void WORKSPACE_SEARCH_QUERY::More()
{
    if( m_nextPageToken.IsEmpty() || m_inFlight || m_debounce.IsRunning() || m_retry.IsRunning() )
        return;

    run( false );
}


void WORKSPACE_SEARCH_QUERY::Retry()
{
    m_debounce.Stop();
    run( true );
}


void WORKSPACE_SEARCH_QUERY::Cancel()
{
    m_debounce.Stop();
    m_retry.Stop();
    m_op.Cancel();
    m_inFlight = false;
}


void WORKSPACE_SEARCH_QUERY::run( bool aFirstPage )
{
    m_retry.Stop();

    MDW_PAGE page;
    page.page_size = PAGE_SIZE;

    // A cursor from an earlier query would page through the wrong results
    if( aFirstPage )
        m_nextPageToken.clear();
    else
        page.page_token = m_nextPageToken;

    m_inFlight = true;
    m_op = m_search.Run( m_view.query, page,
                         [this, aFirstPage]( const MDW_RESULT& aResult, const MDW_PAGED<MDW_ITEM_INFO>& aPage )
                         {
                             onPage( aFirstPage, aResult, aPage );
                         } );

    // The consumer may cancel or delete us from inside ROWS, so the op is owned first and nothing follows
    ROWS rows = m_rows;

    if( rows )
    {
        WORKSPACE_SEARCH_QUERY::STATUS status;
        status.text = searching();
        status.searching = true;
        rows( {}, false, !m_nextPageToken.IsEmpty(), status );
    }
}


void WORKSPACE_SEARCH_QUERY::onPage( bool aFirstPage, const MDW_RESULT& aResult,
                                     const MDW_PAGED<MDW_ITEM_INFO>& aPage )
{
    m_inFlight = false;

    // ROWS may delete us, so it runs from a copy and nothing follows it
    ROWS rows = m_rows;

    if( !aResult.IsOk() )
    {
        bool throttled = false;

        switch( aResult.error )
        {
        case MDW_ERROR_CODE::VERSION_CONFLICT:
            // The listing snapshot behind the page token expired, so the traversal starts over
            if( !aFirstPage )
            {
                run( true );
                return;
            }

            break;

        case MDW_ERROR_CODE::RATE_LIMITED:
        case MDW_ERROR_CODE::BUSY:
            throttled = true;
            m_retryFirstPage = aFirstPage;
            // A longer wait than the cap is left to the Retry button
            m_retry.StartOnce( aResult.retry_after_ms
                                       ? static_cast<int>( std::min<uint64_t>( aResult.retry_after_ms, MAX_RETRY_MS ) )
                                       : DEFAULT_RETRY_MS );
            break;

        case MDW_ERROR_CODE::PERMISSION_DENIED:
            m_view.shown = 0;
            m_view.total.reset();
            m_nextPageToken.clear();
            m_displayed = m_view;

            if( rows )
                rows( {}, true, false, searchStatus( aResult, m_view ) );

            return;

        default:
            break;
        }

        // With none of this query's rows shown, what the page holds answers another query
        const bool replace = m_view.shown == 0 && !throttled;

        if( replace )
            m_displayed = m_view;

        if( rows )
            rows( {}, replace, !m_nextPageToken.IsEmpty() && !WorkspaceSearchOffline( aResult ),
                  searchStatus( aResult, m_view ) );

        return;
    }

    m_view.shown = ( aFirstPage ? 0 : m_view.shown ) + aPage.rows.size();
    m_view.total = aPage.total_count;
    m_view.last_good = wxDateTime::Now();
    m_nextPageToken = aPage.next_page_token;
    m_displayed = m_view;

    if( rows )
        rows( aPage.rows, aFirstPage, !m_nextPageToken.IsEmpty(), searchStatus( aResult, m_view ) );
}
