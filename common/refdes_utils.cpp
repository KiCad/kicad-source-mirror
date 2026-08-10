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
    wxString retVal;
    size_t   i = 0;

    while( i < aReferences.size() )
    {
        const wxString& reference = aReferences[i];
        wxString        prefix = GetRefDesPrefix( reference );
        wxString        numberText = reference.Mid( prefix.length() );
        long            number;
        bool            hasNumber = numberText.ToLong( &number );
        size_t          range = 1;

        while( hasNumber && i + range < aReferences.size() )
        {
            const wxString& nextReference = aReferences[i + range];
            wxString        nextPrefix = GetRefDesPrefix( nextReference );
            wxString        nextNumberText = nextReference.Mid( nextPrefix.length() );
            long            nextNumber;

            if( nextPrefix != prefix || !nextNumberText.ToLong( &nextNumber )
                || nextNumber != number + static_cast<long>( range ) )
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
            retVal << reference;
        }
        else if( range == 2 || aRefRangeDelimiter.IsEmpty() )
        {
            retVal << reference;
            retVal << aRefDelimiter;
            retVal << aReferences[i + 1];
        }
        else
        {
            retVal << reference;
            retVal << aRefRangeDelimiter;
            retVal << aReferences[i + range - 1];
        }

        i += range;
    }

    return retVal;
}

} // namespace UTIL
