/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

using FOOTPRINT_FILTER_IT = FOOTPRINT_FILTER::ITERATOR;


FOOTPRINT_FILTER::ITERATOR::ITERATOR() : m_pos( 0 ), m_filter( nullptr )
{
}


FOOTPRINT_FILTER::ITERATOR::ITERATOR( FOOTPRINT_FILTER_IT const& aOther )
        : m_pos( aOther.m_pos ), m_filter( aOther.m_filter )
{
}


FOOTPRINT_FILTER::ITERATOR::ITERATOR( FOOTPRINT_FILTER& aFilter )
        : m_pos( (size_t) -1 ), m_filter( &aFilter )
{
    increment();
}


void FOOTPRINT_FILTER_IT::increment()
{
    bool found = false;

    if( !m_filter || !m_filter->m_list || m_filter->m_list->GetCount() == 0 )
    {
        m_pos = 0;
        return;
    }

    auto  filter_type       = m_filter->m_filter_type;
    auto  list              = m_filter->m_list;
    auto& lib_name          = m_filter->m_lib_name;
    auto& filter_pattern    = m_filter->m_filter_pattern;
    auto& filter            = m_filter->m_filter;

    for( ++m_pos; m_pos < list->GetCount() && !found; ++m_pos )
    {
        found = true;

        if( ( filter_type & FOOTPRINT_FILTER::FILTERING_BY_LIBRARY ) && !lib_name.IsEmpty()
                && !list->GetItem( m_pos ).InLibrary( lib_name ) )
            found = false;

        if( ( filter_type & FOOTPRINT_FILTER::FILTERING_BY_COMPONENT_KEYWORD )
                && !FootprintFilterMatch( list->GetItem( m_pos ) ) )
            found = false;

        if( ( filter_type & FOOTPRINT_FILTER::FILTERING_BY_PIN_COUNT )
                && !PinCountMatch( list->GetItem( m_pos ) ) )
            found = false;

        if( ( filter_type & FOOTPRINT_FILTER::FILTERING_BY_NAME ) && !filter_pattern.IsEmpty() )
        {
            wxString currname;

            // If the search string contains a ':' character,
            // include the library name in the search string
            // e.g. LibName:FootprintName
            if( filter_pattern.Contains( ":" ) )
                currname = list->GetItem( m_pos ).GetLibNickname().Lower() + ":";

            currname += list->GetItem( m_pos ).GetFootprintName().Lower();

            if( filter.Find( currname ) == EDA_PATTERN_NOT_FOUND )
                found = false;
        }

        if( filter_type == FOOTPRINT_FILTER::UNFILTERED_FP_LIST )
        {
            // override
            found = true;
        }
    }

    // for loop will stop one past the correct item
    if( found )
        --m_pos;
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

    for( auto const& each_filter : m_filter->m_footprint_filters )
    {
        name.Empty();

        // If the filter contains a ':' character, include the library name in the pattern
        if( each_filter->GetPattern().Contains( ":" ) )
        {
            name = aItem.GetLibNickname().Lower() + ":";
        }

        name += aItem.GetFootprintName().Lower();

        if( each_filter->Find( name ) != EDA_PATTERN_NOT_FOUND )
        {
            return true;
        }
    }

    return false;
}


bool FOOTPRINT_FILTER_IT::PinCountMatch( FOOTPRINT_INFO& aItem )
{
    return m_filter->m_pin_count >= 0 &&
        (unsigned) m_filter->m_pin_count == aItem.GetUniquePadCount();
}


FOOTPRINT_FILTER::FOOTPRINT_FILTER( FOOTPRINT_LIST& aList ) : FOOTPRINT_FILTER()
{
    SetList( aList );
}


FOOTPRINT_FILTER::FOOTPRINT_FILTER()
        : m_list( nullptr ), m_pin_count( -1 ), m_filter_type( UNFILTERED_FP_LIST )
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


void FOOTPRINT_FILTER::FilterByLibrary( wxString const& aLibName )
{
    m_lib_name = aLibName;
    m_filter_type |= FILTERING_BY_LIBRARY;
}


void FOOTPRINT_FILTER::FilterByPinCount( int aPinCount )
{
    m_pin_count = aPinCount;
    m_filter_type |= FILTERING_BY_PIN_COUNT;
}


void FOOTPRINT_FILTER::FilterByFootprintFilters( wxArrayString const& aFilters )
{
    m_footprint_filters.clear();

    for( auto const& each_pattern : aFilters )
    {
        m_footprint_filters.push_back( std::make_unique<EDA_PATTERN_MATCH_WILDCARD_EXPLICIT>() );
        m_footprint_filters.back()->SetPattern( each_pattern.Lower() );
    }

    m_filter_type |= FILTERING_BY_COMPONENT_KEYWORD;
}


void FOOTPRINT_FILTER::FilterByPattern( wxString const& aPattern )
{
    m_filter_pattern = aPattern;
    m_filter.SetPattern( aPattern.Lower() );
    m_filter_type |= FILTERING_BY_NAME;
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
