/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017  Cirilo Bernardo
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

#include <cctype>
#include <iostream>
#include <sstream>

#include <io/idf/idf_common.h>
#include <io/idf/idf_helpers.h>

using namespace std;
using namespace IDF3;


bool IDF3::FetchIDFLine( std::istream& aModel, std::string& aLine, bool& isComment,
                         std::streampos& aFilePos )
{
    aLine = "";
    aFilePos = aModel.tellg();

    if( aModel.fail() )
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


bool IDF3::GetIDFString( const std::string& aLine, std::string& aIDFString, bool& hasQuotes,
                         int& aIndex )
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


bool IDF3::ParseOwner( const std::string& aToken, IDF3::KEY_OWNER& aOwner )
{
    if( CompareToken( "UNOWNED", aToken ) )
    {
        aOwner = KEY_OWNER::UNOWNED;
        return true;
    }
    else if( CompareToken( "ECAD", aToken ) )
    {
        aOwner = KEY_OWNER::ECAD;
        return true;
    }
    else if( CompareToken( "MCAD", aToken ) )
    {
        aOwner = KEY_OWNER::MCAD;
        return true;
    }

    ERROR_IDF << "unrecognized IDF OWNER: '" << aToken << "'\n";

    return false;
}


bool IDF3::ParseIDFLayer( const std::string& aToken, IDF3::IDF_LAYER& aLayer )
{
    if( CompareToken( "TOP", aToken ) )
    {
        aLayer = IDF_LAYER::LYR_TOP;
        return true;
    }
    else if( CompareToken( "BOTTOM", aToken ) )
    {
        aLayer = IDF_LAYER::LYR_BOTTOM;
        return true;
    }
    else if( CompareToken( "BOTH", aToken ) )
    {
        aLayer = IDF_LAYER::LYR_BOTH;
        return true;
    }
    else if( CompareToken( "INNER", aToken ) )
    {
        aLayer = IDF_LAYER::LYR_INNER;
        return true;
    }
    else if( CompareToken( "ALL", aToken ) )
    {
        aLayer = IDF_LAYER::LYR_ALL;
        return true;
    }

    ERROR_IDF << "unrecognized IDF LAYER: '" << aToken << "'\n";

    aLayer = IDF_LAYER::LYR_INVALID;
    return false;
}


bool IDF3::WriteLayersText( std::ostream& aBoardFile, IDF3::IDF_LAYER aLayer )
{
    switch( aLayer )
    {
    case IDF_LAYER::LYR_TOP:
        aBoardFile << "TOP";
        break;

    case IDF_LAYER::LYR_BOTTOM:
        aBoardFile << "BOTTOM";
        break;

    case IDF_LAYER::LYR_BOTH:
        aBoardFile << "BOTH";
        break;

    case IDF_LAYER::LYR_INNER:
        aBoardFile << "INNER";
        break;

    case IDF_LAYER::LYR_ALL:
        aBoardFile << "ALL";
        break;

    default:
        do
        {
            std::ostringstream ostr;
            ostr << "invalid IDF layer: " << static_cast<int>( aLayer );

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
    case IDF_PLACEMENT::PS_UNPLACED:
        return "UNPLACED";

    case IDF_PLACEMENT::PS_PLACED:
        return "PLACED";

    case IDF_PLACEMENT::PS_MCAD:
        return "MCAD";

    case IDF_PLACEMENT::PS_ECAD:
        return "ECAD";

    default:
        break;
    }

    std::ostringstream ostr;
    ostr << "[INVALID PLACEMENT VALUE]:" << static_cast<int>( aPlacement );

    return ostr.str();
}


std::string IDF3::GetLayerString( IDF3::IDF_LAYER aLayer )
{
    switch( aLayer )
    {
    case IDF_LAYER::LYR_TOP:
        return "TOP";

    case IDF_LAYER::LYR_BOTTOM:
        return "BOTTOM";

    case IDF_LAYER::LYR_BOTH:
        return "BOTH";

    case IDF_LAYER::LYR_INNER:
        return "INNER";

    case IDF_LAYER::LYR_ALL:
        return "ALL";

    default:
        break;
    }

    std::ostringstream ostr;
    ostr << "[INVALID LAYER VALUE]:" << static_cast<int>( aLayer );

    return ostr.str();
}


std::string IDF3::GetOwnerString( IDF3::KEY_OWNER aOwner )
{
    switch( aOwner )
    {
    case KEY_OWNER::UNOWNED:
        return "UNOWNED";

    case KEY_OWNER::MCAD:
        return "MCAD";

    case KEY_OWNER::ECAD:
        return "ECAD";

    default:
        break;
    }

    ostringstream ostr;
    ostr << "UNKNOWN: " << static_cast<int>( aOwner );

    return ostr.str();
}
