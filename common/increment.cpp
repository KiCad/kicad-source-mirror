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

#include "increment.h"

#include <wx/wxcrt.h>

#include <cmath>
#include <iostream>
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

    wxString                                           remaining = aStr;
    std::vector<std::pair<wxString, STRING_PART_TYPE>> parts;
    size_t                                             goodParts = 0;

    // Keep popping chunks off the string until we have what we need
    while( goodParts < ( aRightIndex + 1 ) && !remaining.IsEmpty() )
    {
        static const std::regex integerRegex( R"(\d+$)" );

        // ABC or abc but not Abc
        static const std::regex sameCaseAlphabetRegex( R"(([a-z]+|[A-Z]+)$)" );

        // Skippables - for now anything that isn't a letter or number
        static const std::regex skipRegex( R"([^a-zA-Z0-9]+$)" );

        std::string remainingStr = remaining.ToStdString();
        std::smatch match;

        if( std::regex_search( remainingStr, match, integerRegex ) )
        {
            parts.push_back( { match.str(), STRING_PART_TYPE::INTEGER } );
            remaining = remaining.Left( remaining.Len() - match.str().size() );
            goodParts++;
        }
        else if( std::regex_search( remainingStr, match, sameCaseAlphabetRegex ) )
        {
            parts.push_back( { match.str(), STRING_PART_TYPE::ALPHABETIC } );
            remaining = remaining.Left( remaining.Len() - match.str().size() );
            goodParts++;
        }
        else if( std::regex_search( remainingStr, match, skipRegex ) )
        {
            parts.push_back( { match.str(), STRING_PART_TYPE::SKIP } );
            remaining = remaining.Left( remaining.Len() - match.str().size() );
        }
        else
        {
            // Out of ideas
            break;
        }
    }

    // Couldn't find the part we wanted
    if( goodParts < aRightIndex + 1 )
        return std::nullopt;

    // Increment the part we wanted
    bool didIncrement = incrementPart( parts.back().first, parts.back().second, aDelta );

    if( !didIncrement )
        return std::nullopt;

    // Reassemble the string - the left-over part, then parts in reverse
    wxString result = remaining;

    for( auto it = parts.rbegin(); it != parts.rend(); ++it )
    {
        result << it->first;
    }

    return result;
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
            number += aDelta;

            // Going below zero makes things awkward
            // and is not usually that useful.
            if( number < 0 )
                return false;

            aPart.Printf( "%ld", number );

            // If the number was zero-padded, we need to re-pad it
            if( zeroPadded )
            {
                aPart = wxString( "0", oldLen - aPart.Len() ) + aPart;
            }

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

        index += aDelta;

        if( index < 0 )
            return false;

        wxString newStr = AlphabeticFromIndex( index, alpha, true );

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
