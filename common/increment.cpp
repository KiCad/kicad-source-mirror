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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "increment.h"

#include <wx/wxcrt.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <regex>


KICOMMON_API bool IncrementString( wxString& name, int aIncrement )
{
    if( name.IsEmpty() )
        return true;

    wxString suffix;
    wxString digits;
    wxString outputFormat;
    wxString outputNumber;
    int      ii     = name.Len() - 1;
    int      dCount = 0;

    while( ii >= 0 && !wxIsdigit( name.GetChar( ii ) ) )
    {
        suffix = name.GetChar( ii ) + suffix;
        ii--;
    }

    while( ii >= 0 && wxIsdigit( name.GetChar( ii ) ) )
    {
        digits = name.GetChar( ii ) + digits;
        ii--;
        dCount++;
    }

    if( digits.IsEmpty() )
        return true;

    long number = 0;

    if( digits.ToLong( &number ) )
    {
        number += aIncrement;

        // Don't let result go below zero
        if( number > -1 )
        {
            name.Remove( ii + 1 );

            //write out a format string with correct number of leading zeroes
            outputFormat.Printf( wxS( "%%0%dld" ), dCount );

            //write out the number using the format string
            outputNumber.Printf( outputFormat, number );
            name << outputNumber << suffix;
            return true;
        }
    }

    return false;
}


std::optional<wxString> STRING_INCREMENTER::Increment( const wxString& aStr, int aDelta,
                                                       size_t aRightIndex ) const
{
    if( aStr.IsEmpty() )
        return std::nullopt;

    // Slice the UTF-8 encoding, not the wxString - the regexes report byte offsets, and mixing
    // the two underflows the length arithmetic on non-ASCII input and never shortens the string
    std::string                                           remaining = aStr.utf8_string();
    std::vector<std::pair<std::string, STRING_PART_TYPE>> parts;
    size_t                                                goodParts = 0;

    // Keep popping chunks off the string until we have what we need
    // (compare against aRightIndex directly so a SIZE_MAX index can't wrap the target to zero)
    while( goodParts <= aRightIndex && !remaining.empty() )
    {
        static const std::regex integerRegex( R"(\d+$)" );

        // ABC or abc but not Abc
        static const std::regex sameCaseAlphabetRegex( R"(([a-z]+|[A-Z]+)$)" );

        // Skippables - for now anything that isn't a letter or number
        static const std::regex skipRegex( R"([^a-zA-Z0-9]+$)" );

        std::smatch match;

        if( std::regex_search( remaining, match, integerRegex ) )
        {
            parts.push_back( { match.str(), STRING_PART_TYPE::INTEGER } );
            goodParts++;
        }
        else if( std::regex_search( remaining, match, sameCaseAlphabetRegex ) )
        {
            parts.push_back( { match.str(), STRING_PART_TYPE::ALPHABETIC } );
            goodParts++;
        }
        else if( std::regex_search( remaining, match, skipRegex ) )
        {
            parts.push_back( { match.str(), STRING_PART_TYPE::SKIP } );
        }
        else
        {
            // Out of ideas
            break;
        }

        remaining.erase( remaining.size() - match.str().size() );
    }

    // Couldn't find the part we wanted
    if( goodParts <= aRightIndex )
        return std::nullopt;

    // The incrementable parts are ASCII by construction, so the round trip is lossless
    wxString part = wxString::FromUTF8( parts.back().first );

    if( !incrementPart( part, parts.back().second, aDelta ) )
        return std::nullopt;

    parts.back().first = part.utf8_string();

    // Reassemble the string - the left-over part, then parts in reverse
    std::string result = remaining;

    for( auto it = parts.rbegin(); it != parts.rend(); ++it )
    {
        result += it->first;
    }

    return wxString::FromUTF8( result );
}


static bool containsIOSQXZ( const wxString& aStr )
{
    static const wxString iosqxz = "IOSQXZ";

    for( const wxUniChar& c : aStr )
    {
        if( iosqxz.Contains( c ) )
            return true;
    }

    return false;
}


bool STRING_INCREMENTER::incrementPart( wxString& aPart, STRING_PART_TYPE aType, int aDelta ) const
{
    switch( aType )
    {
    case STRING_PART_TYPE::INTEGER:
    {
        long   number = 0;
        bool   zeroPadded = aPart.StartsWith( '0' );
        size_t oldLen = aPart.Len();

        if( aPart.ToLong( &number ) )
        {
            // Test the sum before forming it; signed overflow is UB and the compiler is free
            // to discard the range check below
            if( aDelta > 0 && number > std::numeric_limits<long>::max() - aDelta )
                return false;

            number += aDelta;

            // Going below zero makes things awkward
            // and is not usually that useful.
            if( number < 0 )
                return false;

            aPart.Printf( "%ld", number );

            // If the number was zero-padded, we need to re-pad it
            // (carrying into a wider number drops the padding rather than underflowing)
            if( zeroPadded && aPart.Len() < oldLen )
                aPart.Prepend( wxString( '0', oldLen - aPart.Len() ) );

            return true;
        }

        break;
    }
    case STRING_PART_TYPE::ALPHABETIC:
    {
        // Covert to uppercase
        wxString upper = aPart.Upper();
        bool     wasUpper = aPart == upper;

        static const wxString alphabetFull = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static const wxString alphaNoIOSQXZ = "ABCDEFGHJKLMNPRTUVWY";

        const wxString& alpha =
                ( m_SkipIOSQXZ & !containsIOSQXZ( aPart ) ) ? alphaNoIOSQXZ : alphabetFull;

        int index = IndexFromAlphabetic( upper, alpha );

        // Something was not in the alphabet
        if( index == -1 )
            return false;

        // It's such a big number that we don't want to increment it
        if( index > m_AlphabeticMaxIndex && m_AlphabeticMaxIndex >= 0 )
            return false;

        // Widen before adding; index is unbounded once m_AlphabeticMaxIndex is disabled
        const long long nextIndex = static_cast<long long>( index ) + aDelta;

        if( nextIndex < 0 || nextIndex > std::numeric_limits<int>::max() )
            return false;

        wxString newStr = AlphabeticFromIndex( static_cast<size_t>( nextIndex ), alpha, true );

        if( !wasUpper )
            newStr = newStr.Lower();

        aPart = newStr;

        return true;
    }
    case STRING_PART_TYPE::SKIP: break;
    }

    return false;
}


KICOMMON_API int IndexFromAlphabetic( const wxString& aStr, const wxString& aAlphabet )
{
    int       index = 0;
    const int radix = aAlphabet.Length();

    for( size_t i = 0; i < aStr.Len(); i++ )
    {
        int alphaIndex = aAlphabet.Find( aStr[i] );

        if( alphaIndex == wxNOT_FOUND )
            return -1;

        if( i != aStr.Len() - 1 )
            alphaIndex++;

        index += alphaIndex * pow( radix, aStr.Len() - 1 - i );
    }

    return index;
}


wxString KICOMMON_API AlphabeticFromIndex( size_t aN, const wxString& aAlphabet,
                                           bool aZeroBasedNonUnitCols )
{
    wxString  itemNum;
    bool      firstRound = true;
    const int radix = aAlphabet.Length();

    do
    {
        int modN = aN % radix;

        if( aZeroBasedNonUnitCols && !firstRound )
            modN--; // Start the "tens/hundreds/etc column" at "Ax", not "Bx"

        itemNum.insert( 0, 1, aAlphabet[modN] );

        aN /= radix;
        firstRound = false;
    } while( aN );

    return itemNum;
}
