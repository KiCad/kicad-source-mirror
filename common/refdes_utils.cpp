/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <refdes_utils.h>

#include <string_utils.h>

#include <algorithm>
#include <cctype>


namespace UTIL
{

wxString GetRefDesPrefix( const wxString& aRefDes )
{
    // find the first non-digit, non-question-mark character from the back
    auto res = std::find_if( aRefDes.rbegin(), aRefDes.rend(),
            []( wxUniChar aChr )
            {
                return aChr != '?' && !std::isdigit( aChr );
            } );

    return { aRefDes.begin(), res.base() };
}


wxString GetRefDesUnannotated( const wxString& aSource )
{
   return UTIL::GetRefDesPrefix( aSource ) + wxT( "?" );
}


int GetRefDesNumber( const wxString& aRefDes )
{
    int    retval = -1; // negative to indicate not found
    size_t firstnum = aRefDes.find_first_of( wxS( "0123456789" ) );

    if( firstnum != wxString::npos )
    {
        wxString candidateValue = aRefDes.Mid( firstnum );
        long     result;

        if( !candidateValue.ToLong( &result ) )
            retval = -1;
        else
            retval = static_cast<int>( result );
    }

    return retval;
}


wxString FormatRefDesRanges( const std::vector<wxString>& aReferences, const wxString& aRefDelimiter,
                             const wxString& aRefRangeDelimiter )
{
    std::vector<std::pair<wxString, wxString>> bases;

    for( const wxString& reference : aReferences )
        bases.emplace_back( reference, wxEmptyString );

    return FormatRefDesRanges( bases, aRefDelimiter, aRefRangeDelimiter );
}


wxString FormatRefDesRanges( const std::vector<std::pair<wxString, wxString>>& aBasesAndSuffixes,
                             const wxString& aRefDelimiter, const wxString& aRefRangeDelimiter )
{
    std::vector<std::pair<wxString, wxString>> sorted( aBasesAndSuffixes );

    // A range must not span channels, so each suffix's references are made adjacent
    std::vector<wxString> suffixOrder;

    for( const auto& [base, suffix] : sorted )
    {
        if( std::find( suffixOrder.begin(), suffixOrder.end(), suffix ) == suffixOrder.end() )
            suffixOrder.push_back( suffix );
    }

    std::stable_sort( sorted.begin(), sorted.end(),
                      [&]( const auto& a, const auto& b )
                      {
                          return std::find( suffixOrder.begin(), suffixOrder.end(), a.second )
                                 < std::find( suffixOrder.begin(), suffixOrder.end(), b.second );
                      } );

    auto composed =
            [&]( size_t aIndex )
            {
                return sorted[aIndex].first + sorted[aIndex].second;
            };

    wxString retVal;
    size_t   i = 0;

    while( i < sorted.size() )
    {
        const wxString& reference = sorted[i].first;
        const wxString& suffix = sorted[i].second;
        wxString        prefix = GetRefDesPrefix( reference );
        wxString        numberText = reference.Mid( prefix.length() );
        long            number;
        bool            hasNumber = numberText.ToLong( &number );
        size_t          range = 1;

        while( hasNumber && i + range < sorted.size() )
        {
            const wxString& nextReference = sorted[i + range].first;
            wxString        nextPrefix = GetRefDesPrefix( nextReference );
            wxString        nextNumberText = nextReference.Mid( nextPrefix.length() );
            long            nextNumber;

            if( nextPrefix != prefix || sorted[i + range].second != suffix
                || !nextNumberText.ToLong( &nextNumber ) || nextNumber != number + static_cast<long>( range ) )
            {
                break;
            }

            range++;

            if( range == 2 && aRefRangeDelimiter.IsEmpty() )
                break;
        }

        if( !retVal.IsEmpty() )
            retVal << aRefDelimiter;

        if( range == 1 )
        {
            retVal << composed( i );
        }
        else if( range == 2 || aRefRangeDelimiter.IsEmpty() )
        {
            retVal << composed( i );
            retVal << aRefDelimiter;
            retVal << composed( i + 1 );
        }
        else
        {
            retVal << composed( i );
            retVal << aRefRangeDelimiter;
            retVal << composed( i + range - 1 );
        }

        i += range;
    }

    return retVal;
}

} // namespace UTIL
