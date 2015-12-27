/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014  Cirilo Bernardo
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

#include <cctype>
#include <iostream>
#include <sstream>

#include <idf_common.h>
#include <idf_helpers.h>

using namespace std;
using namespace IDF3;

// fetch a line from the given input file and trim the ends
bool IDF3::FetchIDFLine( std::ifstream& aModel, std::string& aLine, bool& isComment, std::streampos& aFilePos )
{
    aLine = "";
    aFilePos = aModel.tellg();

    if( aFilePos == -1 )
        return false;

    std::getline( aModel, aLine );

    isComment = false;

    // A comment begins with a '#' and must be the first character on the line
    if( aLine[0] == '#' )
    {
        // opening '#' is stripped
        isComment = true;
        aLine.erase( aLine.begin() );
    }

    // strip leading and trailing spaces
    while( !aLine.empty() && isspace( *aLine.begin() ) )
        aLine.erase( aLine.begin() );

    while( !aLine.empty() && isspace( *aLine.rbegin() ) )
        aLine.erase( --aLine.end() );

    // a comment line may be empty to improve human readability
    if( aLine.empty() && !isComment )
        return false;

    return true;
}


// extract an IDF string and move the index to point to the character after the substring
bool IDF3::GetIDFString( const std::string& aLine, std::string& aIDFString,
                          bool& hasQuotes, int& aIndex )
{
    // 1. drop all leading spaces
    // 2. if the first character is '"', read until the next '"',
    //    otherwise read until the next space or EOL.

    std::ostringstream ostr;

    int len = aLine.length();
    int idx = aIndex;

    if( idx < 0 || idx >= len )
        return false;

    while( idx < len && isspace( aLine[idx] ) )
        ++idx;

    if( idx == len )
    {
        aIndex = idx;
        return false;
    }

    if( aLine[idx] == '"' )
    {
        hasQuotes = true;
        ++idx;
        while( idx < len && aLine[idx] != '"' )
            ostr << aLine[idx++];

        if( idx == len )
        {
            ERROR_IDF << "unterminated quote mark in line:\n";
            std::cerr << "LINE: " << aLine << "\n";
            aIndex = idx;
            return false;
        }

        ++idx;
    }
    else
    {
        hasQuotes = false;

        while( idx < len && !isspace( aLine[idx] ) )
            ostr << aLine[idx++];

    }

    aIDFString = ostr.str();
    aIndex = idx;

    return true;
}


// perform a comparison between a fixed token string and an input string.
// the token is assumed to be an upper case IDF token and the input string
// is data from an IDF file. Since IDF tokens are case-insensitive, we cannot
// assume anything about the case of the input string.
bool IDF3::CompareToken( const char* aTokenString, const std::string& aInputString )
{
    std::string::size_type i, j;
    std::string bigToken = aInputString;
    j = aInputString.length();

    for( i = 0; i < j; ++i )
        bigToken[i] = std::toupper( bigToken[i] );

    if( !bigToken.compare( aTokenString ) )
        return true;

    return false;
}


// parse a string for an IDF3::KEY_OWNER
bool IDF3::ParseOwner( const std::string& aToken, IDF3::KEY_OWNER& aOwner )
{
    if( CompareToken( "UNOWNED", aToken ) )
    {
        aOwner = UNOWNED;
        return true;
    }
    else if( CompareToken( "ECAD", aToken ) )
    {
        aOwner = ECAD;
        return true;
    }
    else if( CompareToken( "MCAD", aToken ) )
    {
        aOwner = MCAD;
        return true;
    }

    ERROR_IDF << "unrecognized IDF OWNER: '" << aToken << "'\n";

    return false;
}


bool IDF3::ParseIDFLayer( const std::string& aToken, IDF3::IDF_LAYER& aLayer )
{
    if( CompareToken( "TOP", aToken ) )
    {
        aLayer = LYR_TOP;
        return true;
    }
    else if( CompareToken( "BOTTOM", aToken ) )
    {
        aLayer = LYR_BOTTOM;
        return true;
    }
    else if( CompareToken( "BOTH", aToken ) )
    {
        aLayer = LYR_BOTH;
        return true;
    }
    else if( CompareToken( "INNER", aToken ) )
    {
        aLayer = LYR_INNER;
        return true;
    }
    else if( CompareToken( "ALL", aToken ) )
    {
        aLayer = LYR_ALL;
        return true;
    }

    ERROR_IDF << "unrecognized IDF LAYER: '" << aToken << "'\n";

    aLayer = LYR_INVALID;
    return false;
}


bool IDF3::WriteLayersText( std::ofstream& aBoardFile, IDF3::IDF_LAYER aLayer )
{
    switch( aLayer )
    {
        case LYR_TOP:
            aBoardFile << "TOP";
            break;

        case LYR_BOTTOM:
            aBoardFile << "BOTTOM";
            break;

        case LYR_BOTH:
            aBoardFile << "BOTH";
            break;

        case LYR_INNER:
            aBoardFile << "INNER";
            break;

        case LYR_ALL:
            aBoardFile << "ALL";
            break;

        default:
            do{
                std::ostringstream ostr;
                ostr << "invalid IDF layer: " << aLayer;

                throw( IDF_ERROR( __FILE__, __FUNCTION__, __LINE__, ostr.str() ) );
            } while( 0 );

            break;
    }

    return !aBoardFile.fail();
}


std::string IDF3::GetPlacementString( IDF3::IDF_PLACEMENT aPlacement )
{
    switch( aPlacement )
    {
        case PS_UNPLACED:
            return "UNPLACED";

        case PS_PLACED:
            return "PLACED";

        case PS_MCAD:
            return "MCAD";

        case PS_ECAD:
            return "ECAD";

        default:
            break;
    }

    std::ostringstream ostr;
    ostr << "[INVALID PLACEMENT VALUE]:" << aPlacement;

    return ostr.str();
}


std::string IDF3::GetLayerString( IDF3::IDF_LAYER aLayer )
{
    switch( aLayer )
    {
        case LYR_TOP:
            return "TOP";

        case LYR_BOTTOM:
            return "BOTTOM";

        case LYR_BOTH:
            return "BOTH";

        case LYR_INNER:
            return "INNER";

        case LYR_ALL:
            return "ALL";

        default:
            break;
    }

    std::ostringstream ostr;
    ostr << "[INVALID LAYER VALUE]:" << aLayer;

    return ostr.str();
}

std::string IDF3::GetOwnerString( IDF3::KEY_OWNER aOwner )
{
    switch( aOwner )
    {
        case IDF3::UNOWNED:
            return "UNOWNED";

        case IDF3::MCAD:
            return "MCAD";

        case IDF3::ECAD:
            return "ECAD";

        default:
            break;
    }

    ostringstream ostr;
    ostr << "UNKNOWN: " << aOwner;

    return ostr.str();
}
