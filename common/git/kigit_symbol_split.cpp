/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <git/kigit_symbol_split.h>
#include <git/kicad_git_memory.h>

#include <git2.h>

#include <algorithm>
#include <cctype>
#include <cstring>

namespace
{

bool isSpace( char aChar )
{
    return aChar == ' ' || aChar == '\t' || aChar == '\r' || aChar == '\n';
}


/// True when only blanks precede aPos on its line
bool atLineStart( const std::string& aText, std::size_t aPos )
{
    while( aPos > 0 && aText[aPos - 1] != '\n' && isSpace( aText[aPos - 1] ) )
        --aPos;

    return aPos == 0 || aText[aPos - 1] == '\n';
}


/// The character the escape at aPos, just past a backslash, stands for as DSNLEXER decodes it; aPos moves past it
char unescape( const std::string& aText, std::size_t& aPos )
{
    static const std::string named = "\"\\abfnrtv";
    static const std::string meant = "\"\\\a\b\f\n\r\t\v";

    if( std::size_t at = named.find( aText[aPos] ); at != std::string::npos )
    {
        ++aPos;
        return meant[at];
    }

    // Up to two hex digits after x, else up to three octal digits; with none the escape reads as x or a backslash
    const bool        hex = aText[aPos] == 'x';
    const std::size_t from = hex ? aPos + 1 : aPos;
    std::size_t       end = from;

    while( end < aText.size() && end - from < ( hex ? 2u : 3u )
           && ( hex ? std::isxdigit( static_cast<unsigned char>( aText[end] ) ) != 0
                    : aText[end] >= '0' && aText[end] <= '7' ) )
    {
        ++end;
    }

    aPos = end;

    if( end == from )
        return hex ? 'x' : '\\';

    return static_cast<char>( std::stoul( aText.substr( from, end - from ), nullptr, hex ? 16 : 8 ) );
}


/// The atom at aPos, quoted or bare, decoded; aPos moves past it and any comment lines before it
std::string readAtom( const std::string& aText, std::size_t& aPos )
{
    std::string atom;

    while( aPos < aText.size() )
    {
        if( isSpace( aText[aPos] ) )
            ++aPos;
        else if( aText[aPos] == '#' && atLineStart( aText, aPos ) )
            aPos = std::min( aText.find( '\n', aPos ), aText.size() );
        else
            break;
    }

    if( aPos < aText.size() && aText[aPos] == '"' )
    {
        for( ++aPos; aPos < aText.size() && aText[aPos] != '"'; )
        {
            if( aText[aPos] == '\\' && aPos + 1 < aText.size() )
            {
                ++aPos;
                atom += unescape( aText, aPos );
            }
            else
            {
                atom += aText[aPos++];
            }
        }

        ++aPos;
        return atom;
    }

    for( ; aPos < aText.size() && !isSpace( aText[aPos] ) && aText[aPos] != '(' && aText[aPos] != ')'
           && aText[aPos] != '"';
         ++aPos )
    {
        atom += aText[aPos];
    }

    return atom;
}

} // namespace


namespace KIGIT
{

SYMBOL_FILE SplitSymbols( const std::string& aText )
{
    SYMBOL_FILE result;
    int         depth = 0;
    bool        quoted = false;
    bool        lineStart = true;
    std::size_t start = 0;
    std::size_t framed = 0;
    bool        derived = false;

    for( std::size_t ii = 0; ii < aText.size() && result.complete; ++ii )
    {
        const char c = aText[ii];

        // As in DSNLEXER, a line whose first non-blank character is '#' is a comment
        if( !quoted && lineStart && c == '#' )
        {
            while( ii + 1 < aText.size() && aText[ii + 1] != '\n' )
                ++ii;

            continue;
        }

        if( !quoted )
            lineStart = c == '\n' || ( lineStart && isSpace( c ) );

        if( quoted )
        {
            if( c == '\\' )
                ++ii;
            else if( c == '"' )
                quoted = false;
        }
        else if( c == '"' )
        {
            quoted = true;
        }
        else if( c == '(' && ++depth == 2 )
        {
            start = ii;
            derived = false;
        }
        else if( c == '(' && depth == 3 )
        {
            std::size_t pos = ii + 1;
            derived = derived || readAtom( aText, pos ) == "extends";
        }
        else if( c == ')' )
        {
            if( --depth < 0 )
            {
                result.complete = false;
                break;
            }

            if( depth == 0 )
                result.close = ii;

            std::size_t pos = start + 1;

            if( depth != 1 || readAtom( aText, pos ) != "symbol" )
                continue;

            const std::string name = readAtom( aText, pos );

            if( name.empty() || result.spans.count( name ) )
            {
                result.complete = false;
                break;
            }

            if( result.spans.empty() )
                result.header = aText.substr( 0, start );

            // A symbol's own line is no frame text, so adding or removing one leaves the frame as it was
            std::size_t cut = start;

            while( cut > framed && ( aText[cut - 1] == ' ' || aText[cut - 1] == '\t' ) )
                --cut;

            result.frame += aText.substr( framed, cut - framed );
            framed = ii + 1 < aText.size() && aText[ii + 1] == '\n' ? ii + 2 : ii + 1;
            result.spans[name] = { start, ii + 1 - start, derived };
        }
    }

    if( quoted || depth != 0 )
        result.complete = false;

    result.frame += aText.substr( std::min( framed, aText.size() ) );

    if( result.spans.empty() )
        result.header = aText.substr( 0, std::min( result.close, aText.size() ) );

    return result;
}


std::string FormatVersion( const std::string& aText )
{
    const std::size_t at = aText.find( "(version " );

    if( at == std::string::npos || at > aText.find( "(symbol " ) )
        return std::string();

    const std::size_t start = at + std::strlen( "(version " );
    const std::size_t end = aText.find_first_not_of( "0123456789", start );

    return aText.substr( start, end == std::string::npos ? end : end - start );
}


bool NewerFormat( const std::string& aVersion, const std::string& aThan )
{
    // Versions are dates without leading zeros, so a longer one is newer
    if( aVersion.size() != aThan.size() )
        return aVersion.size() > aThan.size();

    return aVersion > aThan;
}


std::string BlobDigest( const std::string& aText )
{
    LIBGIT2_SCOPE libgit2;
    git_oid       oid;

    if( git_odb_hash( &oid, aText.data(), aText.size(), GIT_OBJECT_BLOB ) != 0 )
        return std::string();

    return git_oid_tostr_s( &oid );
}

} // namespace KIGIT
