/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <footprint_filter.h>
#include <stdexcept>
#include <wx/tokenzr.h>

using FOOTPRINT_FILTER_IT = FOOTPRINT_FILTER::ITERATOR;


FOOTPRINT_FILTER::ITERATOR::ITERATOR() :
        m_pos( 0 ),
        m_filter( nullptr )
{
}


FOOTPRINT_FILTER::ITERATOR::ITERATOR( FOOTPRINT_FILTER_IT const& aOther ) :
        m_pos( aOther.m_pos ),
        m_filter( aOther.m_filter )
{
}


FOOTPRINT_FILTER::ITERATOR::ITERATOR( FOOTPRINT_FILTER& aFilter ) :
        m_pos( (size_t) -1 ),
        m_filter( &aFilter )
{
    increment();
}


void FOOTPRINT_FILTER_IT::increment()
{
    if( !m_filter || !m_filter->m_list || m_filter->m_list->GetCount() == 0 )
    {
        m_pos = 0;
        return;
    }

    int             filter_type = m_filter->m_filter_type;
    FOOTPRINT_LIST* list        = m_filter->m_list;
    wxString&       lib_name    = m_filter->m_lib_name;

    for( ++m_pos; m_pos < list->GetCount(); ++m_pos )
    {
        FOOTPRINT_INFO& candidate = list->GetItem( m_pos );

        if( filter_type == FOOTPRINT_FILTER::UNFILTERED_FP_LIST )
            break;

        if( filter_type & FOOTPRINT_FILTER::FILTERING_BY_PIN_COUNT )
        {
            if( !PinCountMatch( candidate ) )
                continue;
        }

        if( filter_type & FOOTPRINT_FILTER::FILTERING_BY_LIBRARY )
        {
            if( !lib_name.IsEmpty() && !candidate.InLibrary( lib_name ) )
                continue;
        }

        if( filter_type & FOOTPRINT_FILTER::FILTERING_BY_COMPONENT_FP_FILTER )
        {
            if( !FootprintFilterMatch( candidate ) )
                continue;
        }

        if( ( filter_type & FOOTPRINT_FILTER::FILTERING_BY_TEXT_PATTERN ) )
        {
            bool exclude = false;

            for( std::unique_ptr<EDA_COMBINED_MATCHER>& matcher : m_filter->m_pattern_filters )
            {
                std::vector<SEARCH_TERM> searchTerms = candidate.GetSearchTerms();

                if( !matcher->ScoreTerms( searchTerms ) )
                {
                    exclude = true;
                    break;
                }
            }

            if( exclude )
                continue;
        }

        // Candidate passed all filters; exit loop
        break;
    }
}


bool FOOTPRINT_FILTER_IT::equal( FOOTPRINT_FILTER_IT const& aOther ) const
{
    // Invalid iterators are always equal
    return ( m_pos == aOther.m_pos ) && ( m_filter == aOther.m_filter || m_pos == (size_t) -1 );
}


FOOTPRINT_INFO& FOOTPRINT_FILTER_IT::dereference() const
{
    if( m_filter && m_filter->m_list && m_pos < m_filter->m_list->GetCount() )
        return m_filter->m_list->GetItem( m_pos );
    else
        throw std::out_of_range( "Attempt to dereference past FOOTPRINT_FILTER::end()" );
}


bool FOOTPRINT_FILTER_IT::FootprintFilterMatch( FOOTPRINT_INFO& aItem )
{
    if( m_filter->m_footprint_filters.empty() )
        return true;

    // The matching is case insensitive
    wxString name;

    for( const std::unique_ptr<EDA_PATTERN_MATCH>& each_filter : m_filter->m_footprint_filters )
    {
        name.Empty();

        // If the filter contains a ':' character, include the library name in the pattern
        if( each_filter->GetPattern().Contains( wxS( ":" ) ) )
            name = aItem.GetLibNickname().Lower() + wxS( ":" );

        name += aItem.GetFootprintName().Lower();

        if( each_filter->Find( name ) )
            return true;
    }

    return false;
}


bool FOOTPRINT_FILTER_IT::PinCountMatch( FOOTPRINT_INFO& aItem )
{
    return m_filter->m_pin_count >= 0
            && (unsigned) m_filter->m_pin_count == aItem.GetUniquePadCount();
}


FOOTPRINT_FILTER::FOOTPRINT_FILTER( FOOTPRINT_LIST& aList ) :
        FOOTPRINT_FILTER()
{
    SetList( aList );
}


FOOTPRINT_FILTER::FOOTPRINT_FILTER() :
        m_list( nullptr ),
        m_pin_count( -1 ),
        m_filter_type( UNFILTERED_FP_LIST )
{
}


void FOOTPRINT_FILTER::SetList( FOOTPRINT_LIST& aList )
{
    m_list = &aList;
}


void FOOTPRINT_FILTER::ClearFilters()
{
    m_filter_type = UNFILTERED_FP_LIST;
}


void FOOTPRINT_FILTER::FilterByLibrary( const wxString& aLibName )
{
    m_lib_name = aLibName;
    m_filter_type |= FILTERING_BY_LIBRARY;
}


void FOOTPRINT_FILTER::FilterByPinCount( int aPinCount )
{
    m_pin_count = aPinCount;
    m_filter_type |= FILTERING_BY_PIN_COUNT;
}


void FOOTPRINT_FILTER::FilterByFootprintFilters( const wxArrayString& aFilters )
{
    m_footprint_filters.clear();

    for( const wxString& each_pattern : aFilters )
    {
        m_footprint_filters.push_back( std::make_unique<EDA_PATTERN_MATCH_WILDCARD_ANCHORED>() );
        m_footprint_filters.back()->SetPattern( each_pattern.Lower() );
    }

    m_filter_type |= FILTERING_BY_COMPONENT_FP_FILTER;
}


void FOOTPRINT_FILTER::FilterByTextPattern( wxString const& aPattern )
{
    m_filter_pattern = aPattern;

    wxStringTokenizer tokenizer( aPattern.Lower(), " \t\r\n", wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
    {
        const wxString term = tokenizer.GetNextToken().Lower();
        m_pattern_filters.push_back( std::make_unique<EDA_COMBINED_MATCHER>( term, CTX_LIBITEM ) );
    }

    m_filter_type |= FILTERING_BY_TEXT_PATTERN;
}


FOOTPRINT_FILTER_IT FOOTPRINT_FILTER::begin()
{
    return FOOTPRINT_FILTER_IT( *this );
}


FOOTPRINT_FILTER_IT FOOTPRINT_FILTER::end()
{
    FOOTPRINT_FILTER_IT end_it( *this );
    end_it.m_pos = m_list ? m_list->GetCount() : 0;
    return end_it;
}
