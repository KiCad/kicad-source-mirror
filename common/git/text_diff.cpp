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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <git/text_diff.h>

#include <algorithm>
#include <cstring>

namespace KIGIT
{

std::vector<wxString> SplitTextLines( const std::string& aContent )
{
    std::vector<wxString> lines;

    auto appendLine = [&lines]( std::string aLine )
    {
        if( !aLine.empty() && aLine.back() == '\r' )
            aLine.pop_back();

        wxString text = wxString::FromUTF8( aLine.c_str(), aLine.size() );

        if( text.IsEmpty() && !aLine.empty() )
            text = wxString::From8BitData( aLine.c_str(), aLine.size() );

        lines.push_back( text );
    };

    size_t start = 0;

    while( start <= aContent.size() )
    {
        size_t end = aContent.find( '\n', start );

        if( end == std::string::npos )
        {
            // A trailing newline does not add an empty last line.
            if( start < aContent.size() )
                appendLine( aContent.substr( start ) );

            break;
        }

        appendLine( aContent.substr( start, end - start ) );
        start = end + 1;
    }

    return lines;
}


bool TextLooksBinary( const std::string& aContent )
{
    // Bounded deliberately. std::string::find would scan the whole file first.
    size_t probe = std::min<size_t>( aContent.size(), 8000 );

    return std::memchr( aContent.data(), '\0', probe ) != nullptr;
}


std::vector<TEXT_DIFF_LINE> BuildTextDiff( const std::vector<wxString>& aOld, const std::vector<wxString>& aNew )
{
    std::vector<TEXT_DIFF_LINE> result;

    size_t prefix = 0;

    while( prefix < aOld.size() && prefix < aNew.size() && aOld[prefix] == aNew[prefix] )
        prefix++;

    size_t suffix = 0;

    while( suffix < aOld.size() - prefix && suffix < aNew.size() - prefix
           && aOld[aOld.size() - 1 - suffix] == aNew[aNew.size() - 1 - suffix] )
    {
        suffix++;
    }

    for( size_t i = 0; i < prefix; i++ )
        result.push_back( { TEXT_DIFF_KIND::CONTEXT, aOld[i] } );

    size_t n = aOld.size() - prefix - suffix;
    size_t m = aNew.size() - prefix - suffix;

    // A pair too big for an O(n*m) table takes the cruder path below instead. Dividing rather
    // than multiplying keeps the check from overflowing.
    const size_t maxCells = 4000000;
    const size_t rows = n + 1;
    const size_t stride = m + 1;

    if( n > 0 && m > 0 && rows <= maxCells / stride )
    {
        std::vector<unsigned> lcs( rows * stride, 0 );

        auto cell = [&lcs, stride]( size_t aRow, size_t aCol ) -> unsigned&
        {
            return lcs[aRow * stride + aCol];
        };

        for( size_t i = n; i-- > 0; )
        {
            for( size_t j = m; j-- > 0; )
            {
                if( aOld[prefix + i] == aNew[prefix + j] )
                    cell( i, j ) = cell( i + 1, j + 1 ) + 1;
                else
                    cell( i, j ) = std::max( cell( i + 1, j ), cell( i, j + 1 ) );
            }
        }

        size_t i = 0, j = 0;

        while( i < n && j < m )
        {
            if( aOld[prefix + i] == aNew[prefix + j] )
            {
                result.push_back( { TEXT_DIFF_KIND::CONTEXT, aOld[prefix + i] } );
                i++;
                j++;
            }
            else if( cell( i + 1, j ) >= cell( i, j + 1 ) )
            {
                result.push_back( { TEXT_DIFF_KIND::REMOVED, aOld[prefix + i] } );
                i++;
            }
            else
            {
                result.push_back( { TEXT_DIFF_KIND::ADDED, aNew[prefix + j] } );
                j++;
            }
        }

        for( ; i < n; i++ )
            result.push_back( { TEXT_DIFF_KIND::REMOVED, aOld[prefix + i] } );

        for( ; j < m; j++ )
            result.push_back( { TEXT_DIFF_KIND::ADDED, aNew[prefix + j] } );
    }
    else
    {
        for( size_t i = 0; i < n; i++ )
            result.push_back( { TEXT_DIFF_KIND::REMOVED, aOld[prefix + i] } );

        for( size_t j = 0; j < m; j++ )
            result.push_back( { TEXT_DIFF_KIND::ADDED, aNew[prefix + j] } );
    }

    for( size_t i = aOld.size() - suffix; i < aOld.size(); i++ )
        result.push_back( { TEXT_DIFF_KIND::CONTEXT, aOld[i] } );

    return result;
}

} // namespace KIGIT
