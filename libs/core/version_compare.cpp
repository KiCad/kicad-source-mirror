/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
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

#include <charconv>
#include <core/version_compare.h>

const std::vector<int> parseVersionString( const std::string& versionString )
{
    std::vector<int> versionComponents;
    auto             it = versionString.begin();
    auto             end = versionString.end();
    const char*      end_char = versionString.data() + versionString.size();
    while( it != end )
    {
        versionComponents.emplace_back( 0 );

        // On error, the last element will be 0
        auto result = std::from_chars( &( *it ), end_char, versionComponents.back() );

        it += std::distance( &( *it ), result.ptr ); // Update the iterator
        if( it != end && *it == '.' )
        {
            ++it; // Skip the dot separator
        }
    }

    return versionComponents;
}


bool compareVersionStrings( const std::string& aVersionStr1, const std::string& aVersionStr2 )
{
    std::vector<int> ver1 = parseVersionString( aVersionStr1 );
    std::vector<int> ver2 = parseVersionString( aVersionStr2 );
    std::size_t      len = std::min( ver1.size(), ver2.size() );

    // Compare each component
    for( size_t ii = 0; ii < len; ++ii )
    {
        if( ver1[ii] < ver2[ii] )
        {
            return true; // aVersionStr1 < aVersionStr2
        }
        else if( ver1[ii] > ver2[ii] )
        {
            return false; // aVersionStr1 > aVersionStr2
        }
    }

    return ver1.size() >= ver2.size();
}