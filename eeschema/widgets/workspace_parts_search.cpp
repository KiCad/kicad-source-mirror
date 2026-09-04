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

#include <widgets/workspace_parts_search.h>

#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_source.h>

#include <wx/intl.h>

#include <algorithm>


WORKSPACE_PARTS_SEARCH::WORKSPACE_PARTS_SEARCH( std::vector<std::shared_ptr<MDW_SOURCE>> aSources, RESOLVER aResolver,
                                                CHANGED aChanged ) :
        m_resolver( std::move( aResolver ) ),
        m_changed( std::move( aChanged ) )
{
    for( std::shared_ptr<MDW_SOURCE>& source : aSources )
    {
        SOURCE_STATE& state = m_sources.emplace_back();
        state.source = std::move( source );

        GROUP& group = m_groups.emplace_back();
        group.source_name = state.source->Info().display_name;
    }

    for( size_t ii = 0; ii < m_groups.size(); ++ii )
    {
        if( m_groups.size() == 1 )
            m_groups[ii].name = wxS( "-- " ) + _( "Workspace Parts" ) + wxS( " --" );
        else
            m_groups[ii].name =
                    wxS( "-- " ) + wxString::Format( _( "%s Parts" ), m_groups[ii].source_name ) + wxS( " --" );
    }
}


std::vector<std::shared_ptr<MDW_SOURCE>> WORKSPACE_PARTS_SEARCH::PartSources()
{
    std::vector<std::shared_ptr<MDW_SOURCE>> candidates;
    std::vector<std::shared_ptr<MDW_SOURCE>> sources;

    if( std::shared_ptr<MDW_SOURCE> project = MDW_CLIENT::Get().ProjectSource() )
        candidates.push_back( project );

    for( const std::shared_ptr<MDW_SOURCE>& library : MDW_CLIENT::Get().LibrarySources() )
        candidates.push_back( library );

    for( const std::shared_ptr<MDW_SOURCE>& candidate : candidates )
    {
        if( !candidate || !candidate->Has( MDW_CAPABILITY_GROUP::PARTS ) )
            continue;

        const wxString locator = candidate->Info().locator;

        bool seen = std::any_of( sources.begin(), sources.end(),
                                 [&]( const std::shared_ptr<MDW_SOURCE>& aSource )
                                 {
                                     return aSource->Info().locator == locator;
                                 } );

        if( !seen )
            sources.push_back( candidate );
    }

    return sources;
}


std::shared_ptr<MDW_SOURCE> WORKSPACE_PARTS_SEARCH::Source( size_t aGroup ) const
{
    return aGroup < m_sources.size() ? m_sources[aGroup].source : nullptr;
}


void WORKSPACE_PARTS_SEARCH::SetQuery( const wxString& aText )
{
    const wxString text = aText.Strip( wxString::both );

    if( text == m_query )
        return;

    m_query = text;

    for( size_t ii = 0; ii < m_sources.size(); ++ii )
    {
        if( m_query.IsEmpty() )
        {
            SOURCE_STATE& state = m_sources[ii];
            state.op.Cancel();

            if( state.retry )
                state.retry->Stop();

            state.parts.clear();
            state.next_page_token.clear();
            m_groups[ii].rows.clear();

            if( m_changed )
                m_changed( ii );
        }
        else
        {
            startPage( ii, true );
        }
    }
}


void WORKSPACE_PARTS_SEARCH::Activate( size_t aGroup, size_t aRow )
{
    if( aGroup >= m_groups.size() || aRow >= m_groups[aGroup].rows.size() )
        return;

    switch( m_groups[aGroup].rows[aRow].kind )
    {
    case ROW_KIND::MORE:  startPage( aGroup, false ); break;
    case ROW_KIND::RETRY: startPage( aGroup, true );  break;
    default:                                          break;
    }
}


void WORKSPACE_PARTS_SEARCH::startPage( size_t aGroup, bool aFirstPage )
{
    SOURCE_STATE& state = m_sources[aGroup];

    if( state.retry )
        state.retry->Stop();

    MDW_PAGE page;
    page.page_size = PAGE_SIZE;

    if( aFirstPage )
    {
        state.parts.clear();
        state.next_page_token.clear();
    }
    else
    {
        page.page_token = state.next_page_token;
    }

    setStatus( aGroup, ROW_KIND::STATUS, _( "Searching" ) + wxS( "…" ) );

    // Assigning the new op cancels any still in flight, so a stale page never lands
    state.op = state.source->Parts().Search(
            m_query, page,
            [this, aGroup, aFirstPage]( const MDW_RESULT& aResult, const MDW_PAGED<MDW_PART_INFO>& aPage )
            {
                onPage( aGroup, aFirstPage, aResult, aPage );
            } );
}


void WORKSPACE_PARTS_SEARCH::onPage( size_t aGroup, bool aFirstPage, const MDW_RESULT& aResult,
                                     const MDW_PAGED<MDW_PART_INFO>& aPage )
{
    SOURCE_STATE&   state = m_sources[aGroup];
    const wxString& source = m_groups[aGroup].source_name;

    if( !aResult.IsOk() )
    {
        switch( aResult.error )
        {
        case MDW_ERROR_CODE::CANCELLED:
            return;

        case MDW_ERROR_CODE::RATE_LIMITED:
        case MDW_ERROR_CODE::BUSY:
        {
            unsigned delay = aResult.retry_after_ms ? aResult.retry_after_ms : DEFAULT_RETRY_MS;

            if( !state.retry )
            {
                state.retry = std::make_unique<wxTimer>();
                state.retry->Bind( wxEVT_TIMER,
                                   [this, aGroup]( wxTimerEvent& )
                                   {
                                       startPage( aGroup, m_sources[aGroup].parts.empty() );
                                   } );
            }

            // Still searching from the user's view; the next query restarts the wait anyway
            state.retry->StartOnce( static_cast<int>( delay ) );
            return;
        }

        case MDW_ERROR_CODE::NETWORK:
        case MDW_ERROR_CODE::TIMEOUT:
        case MDW_ERROR_CODE::BACKEND_LOST:
            setStatus( aGroup, ROW_KIND::STATUS, wxString::Format( _( "%s is offline." ), source ) );
            return;

        case MDW_ERROR_CODE::PERMISSION_DENIED:
        {
            wxString text = wxString::Format( _( "You do not have access to parts in %s." ), source );

            if( !aResult.message.IsEmpty() )
                text << wxS( " " ) << aResult.message;

            setStatus( aGroup, ROW_KIND::STATUS, text );
            return;
        }

        default:
            setStatus( aGroup, ROW_KIND::RETRY,
                       wxString::Format( _( "Could not search %s. Select to retry." ), source ) );
            return;
        }
    }

    if( aFirstPage )
        state.parts.clear();

    for( const MDW_PART_INFO& part : aPage.rows )
    {
        ROW& row = state.parts.emplace_back();
        row.kind = ROW_KIND::PART;
        row.text = part.part_number.IsEmpty() ? part.name : part.part_number;
        row.part = part;
        row.resolution = m_resolver ? m_resolver( part, row.symbol ) : RESOLUTION::NO_SYMBOL;
    }

    state.next_page_token = aPage.next_page_token;

    std::vector<ROW> tail;

    if( state.parts.empty() )
    {
        ROW& row = tail.emplace_back();
        row.text = wxString::Format( _( "No parts in %s match '%s'." ), source, m_query );
    }
    else if( !state.next_page_token.IsEmpty() )
    {
        ROW& row = tail.emplace_back();
        row.kind = ROW_KIND::MORE;
        row.text = _( "Show more" ) + wxS( "…" );
    }

    publish( aGroup, std::move( tail ) );
}


void WORKSPACE_PARTS_SEARCH::Reresolve()
{
    if( !m_resolver )
        return;

    for( size_t ii = 0; ii < m_sources.size(); ++ii )
    {
        std::vector<ROW>& parts = m_sources[ii].parts;
        bool              changed = false;

        for( ROW& row : parts )
        {
            if( row.resolution != RESOLUTION::NOT_SET_UP )
                continue;

            LIB_ID     symbol;
            RESOLUTION resolution = m_resolver( row.part, symbol );

            if( resolution != row.resolution )
            {
                row.resolution = resolution;
                row.symbol = symbol;
                changed = true;
            }
        }

        if( !changed )
            continue;

        const std::vector<ROW>& shown = m_groups[ii].rows;
        size_t                  tail = std::min( parts.size(), shown.size() );

        publish( ii, std::vector<ROW>( shown.begin() + tail, shown.end() ) );
    }
}


void WORKSPACE_PARTS_SEARCH::setStatus( size_t aGroup, ROW_KIND aKind, const wxString& aText )
{
    ROW row;
    row.kind = aKind;
    row.text = aText;

    publish( aGroup, { row } );
}


void WORKSPACE_PARTS_SEARCH::publish( size_t aGroup, std::vector<ROW> aTail )
{
    std::vector<ROW>& rows = m_groups[aGroup].rows;

    rows = m_sources[aGroup].parts;
    rows.insert( rows.end(), std::make_move_iterator( aTail.begin() ), std::make_move_iterator( aTail.end() ) );

    if( m_changed )
        m_changed( aGroup );
}
