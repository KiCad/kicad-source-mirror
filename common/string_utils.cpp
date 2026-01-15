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

/**
 * @file string_utils.cpp
 * @brief Some useful functions to handle strings.
 */

#include <clocale>
#include <cmath>
#include <map>
#include <core/map_helpers.h>
#include <fmt/core.h>
#include <macros.h>
#include <string_utils.h>
#include <widgets/kistatusbar.h>
#include <wx_filename.h>
#include <fmt/chrono.h>
#include <wx/log.h>
#include <wx/regex.h>
#include <wx/tokenzr.h>
#include <libeval/numeric_evaluator.h>
#include "locale_io.h"
#include <wx/event.h>


/**
 * Illegal file name characters used to ensure file names will be valid on all supported
 * platforms.  This is the list of illegal file name characters for Windows which includes
 * the illegal file name characters for Linux and OSX.
 */
static constexpr std::string_view illegalFileNameChars = "\\/:\"<>|*?";

static const wxChar defaultVariantName[] = wxT( "< Default >" );


// Checks if a full filename is valid, i.e. does not contains illegal chars
bool IsFullFileNameValid( const wxString& aFullFilename )
{

    // Test for forbidden chars in aFullFilename.
    // '\'and '/' are allowed here because aFullFilename can be a full path, and
    // ':' is allowed on Windows as second char in string.
    // So remove allowed separators from string to test
    wxString filtered_fullpath = aFullFilename;

#ifdef __WINDOWS__
    // On MSW, the list returned by wxFileName::GetForbiddenChars() contains separators
    // '\'and '/'
    filtered_fullpath.Replace( "/", "_" );
    filtered_fullpath.Replace( "\\", "_" );

    // A disk identifier is allowed, and therefore remove its separator
    if( filtered_fullpath.Length() > 1 && filtered_fullpath[1] == ':' )
        filtered_fullpath[1] = ' ';
#endif

    if( wxString::npos != filtered_fullpath.find_first_of( wxFileName::GetForbiddenChars() ) )
        return false;

    return true;
}


wxString ConvertToNewOverbarNotation( const wxString& aOldStr )
{
    wxString newStr;
    bool inOverbar = false;

    // Don't get tripped up by the legacy empty-string token.
    if( aOldStr == wxT( "~" ) )
        return aOldStr;

    newStr.reserve( aOldStr.length() );

    for( wxString::const_iterator chIt = aOldStr.begin(); chIt != aOldStr.end(); ++chIt )
    {
        if( *chIt == '~' )
        {
            wxString::const_iterator lookahead = chIt + 1;

            if( lookahead != aOldStr.end() && *lookahead == '~' )
            {
                if( ++lookahead != aOldStr.end() && *lookahead == '{' )
                {
                    // This way the subsequent opening curly brace will not start an
                    // overbar.
                    newStr << wxT( "~~{}" );
                    continue;
                }

                // Two subsequent tildes mean a tilde.
                newStr << wxT( "~" );
                ++chIt;
                continue;
            }
            else if( lookahead != aOldStr.end() && *lookahead == '{' )
            {
                // Could mean the user wants "{" with an overbar, but more likely this
                // is a case of double notation conversion.  Bail out.
                return aOldStr;
            }
            else
            {
                if( inOverbar )
                {
                    newStr << wxT( "}" );
                    inOverbar = false;
                }
                else
                {
                    newStr << wxT( "~{" );
                    inOverbar = true;
                }

                continue;
            }
        }
        else if( ( *chIt == ' ' || *chIt == '}' || *chIt == ')' ) && inOverbar )
        {
            // Spaces were used to terminate overbar as well
            newStr << wxT( "}" );
            inOverbar = false;
        }

        newStr << *chIt;
    }

    // Explicitly end the overbar even if there was no terminating '~' in the aOldStr.
    if( inOverbar )
        newStr << wxT( "}" );

    return newStr;
}


bool ConvertSmartQuotesAndDashes( wxString* aString )
{
    bool retVal = false;

    for( wxString::iterator ii = aString->begin(); ii != aString->end(); ++ii )
    {
        if( *ii == L'\u00B4' || *ii == L'\u2018' || *ii == L'\u2019' )
        {
            *ii = '\'';
            retVal = true;
        }
        if( *ii == L'\u201C' || *ii == L'\u201D' )
        {
            *ii = '"';
            retVal = true;
        }
        if( *ii == L'\u2013' || *ii == L'\u2014' )
        {
            *ii = '-';
            retVal = true;
        }
    }

    return retVal;
}


wxString EscapeString( const wxString& aSource, ESCAPE_CONTEXT aContext )
{
    wxString          converted;
    std::vector<bool> braceStack;    // true == formatting construct

    converted.reserve( aSource.length() );

    for( wxUniChar c: aSource )
    {
        if( aContext == CTX_NETNAME )
        {
            if( c == '/' )
                converted += wxT( "{slash}" );
            else if( c == '\n' || c == '\r' )
                converted += wxEmptyString;    // drop
            else
                converted += c;
        }
        else if( aContext == CTX_LIBID || aContext == CTX_LEGACY_LIBID )
        {
            // We no longer escape '/' in LIB_IDs, but we used to
            if( c == '/' && aContext == CTX_LEGACY_LIBID )
                converted += wxT( "{slash}" );
            else if( c == '\\' )
                converted += wxT( "{backslash}" );
            else if( c == '<' )
                converted += wxT( "{lt}" );
            else if( c == '>' )
                converted += wxT( "{gt}" );
            else if( c == ':' )
                converted += wxT( "{colon}" );
            else if( c == '\"' )
                converted += wxT( "{dblquote}" );
            else if( c == '\n' || c == '\r' )
                converted += wxEmptyString;    // drop
            else
                converted += c;
        }
        else if( aContext == CTX_IPC )
        {
            if( c == '/' )
                converted += wxT( "{slash}" );
            else if( c == ',' )
                converted += wxT( "{comma}" );
            else if( c == '\"' )
                converted += wxT( "{dblquote}" );
            else
                converted += c;
        }
        else if( aContext == CTX_QUOTED_STR )
        {
            if( c == '\"' )
                converted += wxT( "{dblquote}" );
            else
                converted += c;
        }
        else if( aContext == CTX_JS_STR )
        {
            if( c >= 0x7F || c == '\'' || c == '\\' || c == '(' || c == ')' )
            {
                unsigned int code = c;
                char buffer[16];
                snprintf( buffer, sizeof(buffer), "\\u%4.4X", code );
                converted += buffer;
            }
            else
            {
                converted += c;
            }
        }
        else if( aContext == CTX_LINE )
        {
            if( c == '\n' || c == '\r' )
                converted += wxT( "{return}" );
            else
                converted += c;
        }
        else if( aContext == CTX_FILENAME )
        {
            if( c == '/' )
                converted += wxT( "{slash}" );
            else if( c == '\\' )
                converted += wxT( "{backslash}" );
            else if( c == '\"' )
                converted += wxT( "{dblquote}" );
            else if( c == '<' )
                converted += wxT( "{lt}" );
            else if( c == '>' )
                converted += wxT( "{gt}" );
            else if( c == '|' )
                converted += wxT( "{bar}" );
            else if( c == ':' )
                converted += wxT( "{colon}" );
            else if( c == '\t' )
                converted += wxT( "{tab}" );
            else if( c == '\n' || c == '\r' )
                converted += wxT( "{return}" );
            else
                converted += c;
        }
        else if( aContext == CTX_NO_SPACE )
        {
            if( c == ' ' )
                converted += wxT( "{space}" );
            else
                converted += c;
        }
        else if( aContext == CTX_CSV )
        {
            if( c == ',' )
                converted += wxT( "{comma}" );
            else if( c == '\n' || c == '\r' )
                converted += wxT( "{return}" );
            else
                converted += c;
        }
        else
        {
            converted += c;
        }
    }

    return converted;
}


wxString UnescapeString( const wxString& aSource )
{
    size_t sourceLen = aSource.length();

    // smallest escape string is three characters, shortcut everything else
    if( sourceLen <= 2 )
    {
        return aSource;
    }

    wxString newbuf;
    newbuf.reserve( sourceLen );

    wxUniChar prev = 0;
    wxUniChar ch = 0;

    for( size_t i = 0; i < sourceLen; ++i )
    {
        prev = ch;
        ch = aSource[i];

        if( ch == '{' )
        {
            wxString token;
            int      depth = 1;
            bool     terminated = false;

            for( i = i + 1; i < sourceLen; ++i )
            {
                ch = aSource[i];

                if( ch == '{' )
                    depth++;
                else if( ch == '}' )
                    depth--;

                if( depth <= 0 )
                {
                    terminated = true;
                    break;
                }
                else
                {
                    token << ch;
                }
            }

            if( !terminated )
            {
                newbuf << wxT( "{" ) << UnescapeString( token );
            }
            else if( prev == '$' || prev == '~' || prev == '^' || prev == '_' )
            {
                newbuf << wxT( "{" ) << UnescapeString( token ) << wxT( "}" );
            }
            else if( token == wxT( "dblquote" ) )  newbuf << wxT( "\"" );
            else if( token == wxT( "quote" ) )     newbuf << wxT( "'" );
            else if( token == wxT( "lt" ) )        newbuf << wxT( "<" );
            else if( token == wxT( "gt" ) )        newbuf << wxT( ">" );
            else if( token == wxT( "backslash" ) ) newbuf << wxT( "\\" );
            else if( token == wxT( "slash" ) )     newbuf << wxT( "/" );
            else if( token == wxT( "bar" ) )       newbuf << wxT( "|" );
            else if( token == wxT( "comma" ) )     newbuf << wxT( "," );
            else if( token == wxT( "colon" ) )     newbuf << wxT( ":" );
            else if( token == wxT( "space" ) )     newbuf << wxT( " " );
            else if( token == wxT( "dollar" ) )    newbuf << wxT( "$" );
            else if( token == wxT( "tab" ) )       newbuf << wxT( "\t" );
            else if( token == wxT( "return" ) )    newbuf << wxT( "\n" );
            else if( token == wxT( "brace" ) )     newbuf << wxT( "{" );
            else
            {
                newbuf << wxT( "{" ) << UnescapeString( token ) << wxT( "}" );
            }
        }
        else
        {
            newbuf << ch;
        }
    }

    return newbuf;
}


wxString TitleCaps( const wxString& aString )
{
    wxArrayString words;
    wxString      result;

    wxStringSplit( aString, words, ' ' );

    result.reserve( aString.length() );

    for( const wxString& word : words )
    {
        if( !result.IsEmpty() )
            result += wxT( " " );

        result += word.Capitalize();
    }

    return result;
}


wxString InitialCaps( const wxString& aString )
{
    wxArrayString words;
    wxString      result;

    wxStringSplit( aString, words, ' ' );

    result.reserve( aString.length() );

    for( const wxString& word : words )
    {
        if( result.IsEmpty() )
            result += word.Capitalize();
        else
            result += wxT( " " ) + word.Lower();
    }

    return result;
}


int ReadDelimitedText( wxString* aDest, const char* aSource )
{
    std::string utf8;               // utf8 but without escapes and quotes.
    bool        inside = false;
    const char* start = aSource;
    char        cc;

    while( (cc = *aSource++) != 0  )
    {
        if( cc == '"' )
        {
            if( inside )
                break;          // 2nd double quote is end of delimited text

            inside = true;      // first delimiter found, make note, do not copy
        }

        else if( inside )
        {
            if( cc == '\\' )
            {
                cc = *aSource++;

                if( !cc )
                    break;

                // do no copy the escape byte if it is followed by \ or "
                if( cc != '"' && cc != '\\' )
                    utf8 += '\\';

                utf8 += cc;
            }
            else
            {
                utf8 += cc;
            }
        }
    }

    *aDest = From_UTF8( utf8.c_str() );

    return aSource - start;
}


int ReadDelimitedText( char* aDest, const char* aSource, int aDestSize )
{
    if( aDestSize <= 0 )
        return 0;

    bool        inside = false;
    const char* start = aSource;
    char*       limit = aDest + aDestSize - 1;
    char        cc;

    while( ( cc = *aSource++ ) != 0 && aDest < limit )
    {
        if( cc == '"' )
        {
            if( inside )
                break;          // 2nd double quote is end of delimited text

            inside = true;      // first delimiter found, make note, do not copy
        }
        else if( inside )
        {
            if( cc == '\\' )
            {
                cc = *aSource++;

                if( !cc )
                    break;

                // do no copy the escape byte if it is followed by \ or "
                if( cc != '"' && cc != '\\' )
                    *aDest++ = '\\';

                if( aDest < limit )
                    *aDest++ = cc;
            }
            else
            {
                *aDest++ = cc;
            }
        }
    }

    *aDest = 0;

    return aSource - start;
}


std::string EscapedUTF8( const wxString& aString )
{
    wxString str = aString;

    // No new-lines allowed in quoted strings
    str.Replace( wxT( "\r\n" ), wxT( "\r" ) );
    str.Replace( wxT( "\n" ), wxT( "\r" ) );

    std::string utf8 = TO_UTF8( aString );

    std::string ret;

    ret.reserve( utf8.length() + 2 );

    ret += '"';

    for( std::string::const_iterator it = utf8.begin();  it!=utf8.end();  ++it )
    {
        // this escaping strategy is designed to be compatible with ReadDelimitedText():
        if( *it == '"' )
        {
            ret += '\\';
            ret += '"';
        }
        else if( *it == '\\' )
        {
            ret += '\\';    // double it up
            ret += '\\';
        }
        else
        {
            ret += *it;
        }
    }

    ret += '"';

    return ret;
}


wxString EscapeHTML( const wxString& aString )
{
    wxString converted;

    converted.reserve( aString.length() );

    for( wxUniChar c : aString )
    {
        if( c == '\"' )
            converted += wxT( "&quot;" );
        else if( c == '\'' )
            converted += wxT( "&apos;" );
        else if( c == '&' )
            converted += wxT( "&amp;" );
        else if( c == '<' )
            converted += wxT( "&lt;" );
        else if( c == '>' )
            converted += wxT( "&gt;" );
        else
            converted += c;
    }

    return converted;
}


wxString UnescapeHTML( const wxString& aString )
{
    // clang-format off
    static const std::map<wxString, wxString> c_replacements = {
        { wxS( "quot" ), wxS( "\"" ) },
        { wxS( "apos" ), wxS( "'" ) },
        { wxS( "amp" ), wxS( "&" ) },
        { wxS( "lt" ), wxS( "<" ) },
        { wxS( "gt" ), wxS( ">" ) }
    };
    // clang-format on

    // Construct regex
    wxString regexStr = "&(#(\\d*)|#x([a-zA-Z0-9]{4})";

    for( auto& [key, value] : c_replacements )
        regexStr << '|' << key;

    regexStr << ");";

    wxRegEx regex( regexStr );

    // Process matches
    size_t start = 0;
    size_t len = 0;

    wxString result;
    wxString str = aString;

    while( regex.Matches( str ) )
    {
        std::vector<wxString> matches;
        regex.GetMatch( &start, &len );

        result << str.Left( start );

        wxString code = regex.GetMatch( str, 1 );
        wxString codeDec = regex.GetMatch( str, 2 );
        wxString codeHex = regex.GetMatch( str, 3 );

        if( !codeDec.IsEmpty() || !codeHex.IsEmpty() )
        {
            unsigned long codeVal = 0;

            if( !codeDec.IsEmpty() )
                codeDec.ToCULong( &codeVal );
            else if( !codeHex.IsEmpty() )
                codeHex.ToCULong( &codeVal, 16 );

            if( codeVal != 0 )
                result << wxUniChar( codeVal );
        }
        else if( auto val = get_opt( c_replacements, code ) )
        {
            result << *val;
        }

        str = str.Mid( start + len );
    }

    result << str;

    return result;
}


wxString RemoveHTMLTags( const wxString& aInput )
{
    wxString str = aInput;
    wxRegEx( wxS( "<[^>]*>" ) ).ReplaceAll( &str, wxEmptyString );

    return str;
}


wxString LinkifyHTML( wxString aStr )
{
    static wxRegEx regex( wxS( "\\b(https?|ftp|file)://([-\\w+&@#/%?=~|!:,.;]*[^.,:;<>\\(\\)\\s\u00b6])" ),
                          wxRE_ICASE );

    regex.ReplaceAll( &aStr, "<a href=\"\\0\">\\0</a>" );

    return aStr;
}


bool IsURL( wxString aStr )
{
    static wxRegEx regex( wxS( "(https?|ftp|file)://([-\\w+&@#/%?=~|!:,.;]*[^.,:;<>\\s\u00b6])" ),
                          wxRE_ICASE );

    regex.ReplaceAll( &aStr, "<a href=\"\\0\">\\0</a>" );

    return regex.Matches( aStr );
}


bool NoPrintableChars( const wxString& aString )
{
    wxString tmp = aString;

    return tmp.Trim( true ).Trim( false ).IsEmpty();
}


int PrintableCharCount( const wxString& aString )
{
    int char_count = 0;
    int overbarDepth = -1;
    int superSubDepth = -1;
    int braceNesting = 0;

    for( auto chIt = aString.begin(), end = aString.end(); chIt < end; ++chIt )
    {
        if( *chIt == '\t' )
        {
            // We don't format tabs in bitmap text (where this is currently used), so just
            // drop them from the count.
            continue;
        }
        else if( *chIt == '^' && superSubDepth == -1 )
        {
            auto lookahead = chIt;

            if( ++lookahead != end && *lookahead == '{' )
            {
                chIt = lookahead;
                superSubDepth = braceNesting;
                braceNesting++;
                continue;
            }
        }
        else if( *chIt == '_' && superSubDepth == -1 )
        {
            auto lookahead = chIt;

            if( ++lookahead != end && *lookahead == '{' )
            {
                chIt = lookahead;
                superSubDepth = braceNesting;
                braceNesting++;
                continue;
            }
        }
        else if( *chIt == '~' && overbarDepth == -1 )
        {
            auto lookahead = chIt;

            if( ++lookahead != end && *lookahead == '{' )
            {
                chIt = lookahead;
                overbarDepth = braceNesting;
                braceNesting++;
                continue;
            }
        }
        else if( *chIt == '{' )
        {
            braceNesting++;
        }
        else if( *chIt == '}' )
        {
            if( braceNesting > 0 )
                braceNesting--;

            if( braceNesting == superSubDepth )
            {
                superSubDepth = -1;
                continue;
            }

            if( braceNesting == overbarDepth )
            {
                overbarDepth = -1;
                continue;
            }
        }

        char_count++;
    }

    return char_count;
}


char* StrPurge( char* text )
{
    static const char whitespace[] = " \t\n\r\f\v";

    if( text )
    {
        while( *text && strchr( whitespace, *text ) )
            ++text;

        char* cp = text + strlen( text ) - 1;

        while( cp >= text && strchr( whitespace, *cp ) )
            *cp-- = '\0';
    }

    return text;
}


char* GetLine( FILE* File, char* Line, int* LineNum, int SizeLine )
{
    do {
        if( fgets( Line, SizeLine, File ) == nullptr )
            return nullptr;

        if( LineNum )
            *LineNum += 1;

    } while( Line[0] == '#' || Line[0] == '\n' ||  Line[0] == '\r' || Line[0] == 0 );

    strtok( Line, "\n\r" );
    return Line;
}


wxString GetISO8601CurrentDateTime()
{
    return wxDateTime::Now().FormatISOCombined( 'T' );
}


int StrNumCmp( const wxString& aString1, const wxString& aString2, bool aIgnoreCase )
{
    int nb1 = 0, nb2 = 0;

    auto str1 = aString1.begin();
    auto str2 = aString2.begin();

    while( str1 != aString1.end() && str2 != aString2.end() )
    {
        wxUniChar c1 = *str1;
        wxUniChar c2 = *str2;

        if( wxIsdigit( c1 ) && wxIsdigit( c2 ) ) // Both characters are digits, do numeric compare.
        {
            nb1 = 0;
            nb2 = 0;

            do
            {
                c1 = *str1;
                nb1 = nb1 * 10 + (int) c1 - '0';
                ++str1;
            } while( str1 != aString1.end() && wxIsdigit( *str1 ) );

            do
            {
                c2 = *str2;
                nb2 = nb2 * 10 + (int) c2 - '0';
                ++str2;
            } while( str2 != aString2.end() && wxIsdigit( *str2 ) );

            if( nb1 < nb2 )
                return -1;

            if( nb1 > nb2 )
                return 1;

            c1 = ( str1 != aString1.end() ) ? *str1 : wxUniChar( 0 );
            c2 = ( str2 != aString2.end() ) ? *str2 : wxUniChar( 0 );
        }

        // Any numerical comparisons to here are identical.
        if( aIgnoreCase )
        {
            if( c1 != c2 )
            {
                wxUniChar uc1 = wxToupper( c1 );
                wxUniChar uc2 = wxToupper( c2 );

                if( uc1 != uc2 )
                    return uc1 < uc2 ? -1 : 1;
            }
        }
        else
        {
            if( c1 < c2 )
                return -1;

            if( c1 > c2 )
                return 1;
        }

        if( str1 != aString1.end() )
            ++str1;

        if( str2 != aString2.end() )
            ++str2;
    }

    if( str1 == aString1.end() && str2 != aString2.end() )
    {
        return -1;   // Identical to here but aString1 is longer.
    }
    else if( str1 != aString1.end() && str2 == aString2.end() )
    {
        return 1;    // Identical to here but aString2 is longer.
    }

    return 0;
}


bool WildCompareString( const wxString& pattern, const wxString& string_to_tst,
                        bool case_sensitive )
{
    const wxChar* cp = nullptr;
    const wxChar* mp = nullptr;
    const wxChar* wild = nullptr;
    const wxChar* str = nullptr;
    wxString      _pattern, _string_to_tst;

    if( case_sensitive )
    {
        wild = pattern.GetData();
        str = string_to_tst.GetData();
    }
    else
    {
        _pattern = pattern;
        _pattern.MakeUpper();
        _string_to_tst = string_to_tst;
        _string_to_tst.MakeUpper();
        wild = _pattern.GetData();
        str = _string_to_tst.GetData();
    }

    while( ( *str ) && ( *wild != '*' ) )
    {
        if( ( *wild != *str ) && ( *wild != '?' ) )
            return false;

        wild++;
        str++;
    }

    while( *str )
    {
        if( *wild == '*' )
        {
            if( !*++wild )
                return true;

            mp = wild;
            cp = str + 1;
        }
        else if( ( *wild == *str ) || ( *wild == '?' ) )
        {
            wild++;
            str++;
        }
        else
        {
            wild   = mp;
            str = cp++;
        }
    }

    while( *wild == '*' )
    {
        wild++;
    }

    return !*wild;
}


bool ApplyModifier( double& value, const wxString& aString )
{
    /// Although the two 'μ's look the same, they are U+03BC and U+00B5
    static const wxString modifiers( wxT( "afpnuµμmLRFkKMGTPE" ) );

    if( !aString.length() )
        return false;

    wxChar   modifier;
    wxString units;

    if( modifiers.Find( aString[ 0 ] ) >= 0 )
    {
        modifier = aString[ 0 ];
        units = aString.Mid( 1 ).Trim();
    }
    else
    {
        modifier = ' ';
        units = aString.Mid( 0 ).Trim();
    }

    if( units.length()
            && !units.IsSameAs( wxT( "F" ), false )
            && !units.IsSameAs( wxT( "hz" ), false )
            && !units.IsSameAs( wxT( "W" ), false )
            && !units.IsSameAs( wxT( "V" ), false )
            && !units.IsSameAs( wxT( "A" ), false )
            && !units.IsSameAs( wxT( "H" ), false ) )
    {
        return false;
    }

    // Note: most of these are SI, but some (L, R, F) are IEC 60062.
    if( modifier == 'a' )
        value *= 1.0e-18;
    else if( modifier == 'f' )
        value *= 1.0e-15;
    if( modifier == 'p' )
        value *= 1.0e-12;
    if( modifier == 'n' )
        value *= 1.0e-9;
    else if( modifier == 'u' || modifier == wxS( "µ" )[0] || modifier == wxS( "μ" )[0] )
        value *= 1.0e-6;
    else if( modifier == 'm' || modifier == 'L' )
        value *= 1.0e-3;
    else if( modifier == 'R' || modifier == 'F' )
        ; // unity scalar
    else if( modifier == 'k' || modifier == 'K' )
        value *= 1.0e3;
    else if( modifier == 'M' )
        value *= 1.0e6;
    else if( modifier == 'G' )
        value *= 1.0e9;
    else if( modifier == 'T' )
        value *= 1.0e12;
    else if( modifier == 'P' )
        value *= 1.0e15;
    else if( modifier == 'E' )
        value *= 1.0e18;

    return true;
}


bool convertSeparators( wxString* value )
{
    // Note: fetching the decimal separator from the current locale isn't a silver bullet because
    // it assumes the current computer's locale is the same as the locale the schematic was
    // authored in -- something that isn't true, for instance, when sharing designs through
    // DIYAudio.com.
    //
    // Some values are self-describing: multiple instances of a single separator character must be
    // thousands separators; a single instance of each character must be a thousands separator
    // followed by a decimal separator; etc.
    //
    // Only when presented with an ambiguous value do we fall back on the current locale.

    value->Replace( wxS( " " ), wxEmptyString );

    wxChar ambiguousSeparator = '?';
    wxChar thousandsSeparator = '?';
    bool   thousandsSeparatorFound = false;
    wxChar decimalSeparator = '?';
    bool   decimalSeparatorFound = false;
    int    digits = 0;

    for( int ii = (int) value->length() - 1; ii >= 0; --ii )
    {
        wxChar c = value->GetChar( ii );

        if( c >= '0' && c <= '9' )
        {
            digits += 1;
        }
        else if( c == '.' || c == ',' )
        {
            if( decimalSeparator != '?' || thousandsSeparator != '?' )
            {
                // We've previously found a non-ambiguous separator...

                if( c == decimalSeparator )
                {
                    if( thousandsSeparatorFound )
                        return false;       // decimal before thousands
                    else if( decimalSeparatorFound )
                        return false;       // more than one decimal
                    else
                        decimalSeparatorFound = true;
                }
                else if( c == thousandsSeparator )
                {
                    if( digits != 3 )
                        return false;       // thousands not followed by 3 digits
                    else
                        thousandsSeparatorFound = true;
                }
            }
            else if( ambiguousSeparator != '?' )
            {
                // We've previously found a separator, but we don't know for sure which...

                if( c == ambiguousSeparator )
                {
                    // They both must be thousands separators
                    thousandsSeparator = ambiguousSeparator;
                    thousandsSeparatorFound = true;
                    decimalSeparator = c == '.' ? ',' : '.';
                }
                else
                {
                    // The first must have been a decimal, and this must be a thousands.
                    decimalSeparator = ambiguousSeparator;
                    decimalSeparatorFound = true;
                    thousandsSeparator = c;
                    thousandsSeparatorFound = true;
                }
            }
            else
            {
                // This is the first separator...

                // If it's preceded by a '0' (only), or if it's followed by some number of
                // digits not equal to 3, then it -must- be a decimal separator.
                //
                // In all other cases we don't really know what it is yet.

                if( ( ii == 1 && value->GetChar( 0 ) == '0' ) || digits != 3 )
                {
                    decimalSeparator = c;
                    decimalSeparatorFound = true;
                    thousandsSeparator = c == '.' ? ',' : '.';
                }
                else
                {
                    ambiguousSeparator = c;
                }
            }

            digits = 0;
        }
        else
        {
            digits = 0;
        }
    }

    // If we found nothing definitive then we have to look at the current locale
    if( decimalSeparator == '?' && thousandsSeparator == '?' )
    {
        const struct lconv* lc = localeconv();

        decimalSeparator = lc->decimal_point[0];
        thousandsSeparator = decimalSeparator == '.' ? ',' : '.';
    }

    // Convert to C-locale
    value->Replace( thousandsSeparator, wxEmptyString );
    value->Replace( decimalSeparator, '.' );

    return true;
}


int ValueStringCompare( const wxString& strFWord, const wxString& strSWord )
{
    // Compare unescaped text
    wxString fWord = UnescapeString( strFWord );
    wxString sWord = UnescapeString( strSWord );

    // The different sections of the two strings
    wxString strFWordBeg, strFWordMid, strFWordEnd;
    wxString strSWordBeg, strSWordMid, strSWordEnd;

    // Split the two strings into separate parts
    SplitString( fWord, &strFWordBeg, &strFWordMid, &strFWordEnd );
    SplitString( sWord, &strSWordBeg, &strSWordMid, &strSWordEnd );

    // Compare the Beginning section of the strings
    int isEqual = strFWordBeg.CmpNoCase( strSWordBeg );

    if( isEqual > 0 )
    {
        return 1;
    }
    else if( isEqual < 0 )
    {
        return -1;
    }
    else
    {
        // If the first sections are equal compare their digits
        double lFirstNumber  = 0;
        double lSecondNumber = 0;
        bool   endingIsModifier = false;

        convertSeparators( &strFWordMid );
        convertSeparators( &strSWordMid );

        strFWordMid.ToCDouble( &lFirstNumber );
        strSWordMid.ToCDouble( &lSecondNumber );

        endingIsModifier |= ApplyModifier( lFirstNumber, strFWordEnd );
        endingIsModifier |= ApplyModifier( lSecondNumber, strSWordEnd );

        if( lFirstNumber > lSecondNumber )
            return 1;
        else if( lFirstNumber < lSecondNumber )
            return -1;
        // If the first two sections are equal and the endings are modifiers then compare them
        else if( !endingIsModifier )
            return strFWordEnd.CmpNoCase( strSWordEnd );
        // Ran out of things to compare; they must match
        else
            return 0;
    }
}


int SplitString( const wxString& strToSplit,
                 wxString* strBeginning,
                 wxString* strDigits,
                 wxString* strEnd )
{
    static const wxString separators( wxT( ".," ) );
    wxUniChar             infix = 0;

    // Clear all the return strings
    strBeginning->Empty();
    strDigits->Empty();
    strEnd->Empty();

    // There no need to do anything if the string is empty
    if( strToSplit.length() == 0 )
        return 0;

    // Starting at the end of the string look for the first digit
    int ii;

    for( ii = (strToSplit.length() - 1); ii >= 0; ii-- )
    {
        if( wxIsdigit( strToSplit[ii] ) )
            break;
    }

    // If there were no digits then just set the single string
    if( ii < 0 )
    {
        *strBeginning = strToSplit;
    }
    else
    {
        // Since there is at least one digit this is the trailing string
        *strEnd = strToSplit.substr( ii + 1 );

        // Go to the end of the digits
        int position = ii + 1;

        for( ; ii >= 0; ii-- )
        {
            double    scale;
            wxUniChar c = strToSplit[ii];

            if( wxIsdigit( c ) )
            {
                continue;
            }
            if( infix == 0 && NUMERIC_EVALUATOR::IsOldSchoolDecimalSeparator( c, &scale ) )
            {
                infix = c;
                continue;
            }
            else if( separators.Find( strToSplit[ii] ) >= 0 )
            {
                continue;
            }
            else
            {
                break;
            }
        }

        // If all that was left was digits, then just set the digits string
        if( ii < 0 )
        {
            *strDigits = strToSplit.substr( 0, position );
        }
        // Otherwise everything else is part of the preamble
        else
        {
            *strDigits    = strToSplit.substr( ii + 1, position - ii - 1 );
            *strBeginning = strToSplit.substr( 0, ii + 1 );
        }

        if( infix > 0 )
        {
            strDigits->Replace( infix, '.' );
            *strEnd = infix + *strEnd;
        }
    }

    return 0;
}


int GetTrailingInt( const wxString& aStr )
{
    int number = 0;
    int base = 1;

    // Trim and extract the trailing numeric part
    int index = aStr.Len() - 1;

    while( index >= 0 )
    {
        const char chr = aStr.GetChar( index );

        if( chr < '0' || chr > '9' )
            break;

        number += ( chr - '0' ) * base;
        base *= 10;
        index--;
    }

    return number;
}


wxString GetIllegalFileNameWxChars()
{
    return wxString::FromUTF8( illegalFileNameChars.data(), illegalFileNameChars.length() );
}


bool ReplaceIllegalFileNameChars( std::string& aName, int aReplaceChar )
{
    size_t first_illegal_pos = aName.find_first_of( illegalFileNameChars );

    if( first_illegal_pos == std::string::npos )
    {
        return false;
    }

    std::string result;
    // result will be at least equal to original, add 16 in case of hex replacements
    result.reserve( aName.length() + 16 );
    // append the valid part
    result.append( aName, 0, first_illegal_pos );

    for( size_t i = first_illegal_pos; i < aName.length(); ++i )
    {
        char c = aName[i];

        // Check if this specific char is illegal
        if( illegalFileNameChars.find( c ) != std::string_view::npos )
        {
            if( aReplaceChar )
            {
                result.push_back( aReplaceChar );
            }
            else
            {
                fmt::format_to( std::back_inserter( result ), "%{:02x}", static_cast<unsigned char>( c ) );
            }
        }
        else
        {
            result.push_back( c );
        }
    }

    aName = std::move( result );
    return true;
}


bool ReplaceIllegalFileNameChars( wxString& aName, int aReplaceChar )
{
    bool changed = false;
    wxString result;
    result.reserve( aName.Length() );
    wxString illWChars = GetIllegalFileNameWxChars();

    for( wxString::iterator it = aName.begin();  it != aName.end();  ++it )
    {
        if( illWChars.Find( *it ) != wxNOT_FOUND )
        {
            if( aReplaceChar )
                result += aReplaceChar;
            else
                result += wxString::Format( "%%%02x", *it );

            changed = true;
        }
        else
        {
            result += *it;
        }
    }

    if( changed )
        aName = std::move( result );

    return changed;
}


void wxStringSplit( const wxString& aText, wxArrayString& aStrings, wxChar aSplitter )
{
    wxString tmp;

    for( unsigned ii = 0; ii < aText.Length(); ii++ )
    {
        if( aText[ii] == aSplitter )
        {
            aStrings.Add( tmp );
            tmp.Clear();
        }
        else
        {
            tmp << aText[ii];
        }
    }

    if( !tmp.IsEmpty() )
        aStrings.Add( tmp );
}


void StripTrailingZeros( wxString& aStringValue, unsigned aTrailingZeroAllowed )
{
    struct lconv* lc      = localeconv();
    char          sep     = lc->decimal_point[0];
    unsigned      sep_pos = aStringValue.Find( sep );

    if( sep_pos > 0 )
    {
        // We want to keep at least aTrailingZeroAllowed digits after the separator
        unsigned min_len = sep_pos + aTrailingZeroAllowed + 1;

        while( aStringValue.Len() > min_len )
        {
            if( aStringValue.Last() == '0' )
                aStringValue.RemoveLast();
            else
                break;
        }
    }
}


std::string FormatDouble2Str( double aValue )
{
    std::string buf;

    if( aValue != 0.0 && std::fabs( aValue ) <= 0.0001 )
    {
        buf = fmt::format( "{:.16f}", aValue );

        // remove trailing zeros (and the decimal marker if needed)
        while( !buf.empty() && buf[buf.size() - 1] == '0' )
        {
            buf.pop_back();
        }

        // if the value was really small
        // we may have just stripped all the zeros after the decimal
        if( buf[buf.size() - 1] == '.' )
        {
            buf.pop_back();
        }
    }
    else
    {
        buf = fmt::format( "{:.10g}", aValue );
    }

    return buf;
}


std::string UIDouble2Str( double aValue )
{
    char    buf[50];
    int     len;

    if( aValue != 0.0 && std::fabs( aValue ) <= 0.0001 )
    {
        // For these small values, %f works fine,
        // and %g gives an exponent
        len = snprintf( buf, sizeof( buf ), "%.16f", aValue );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

        if( buf[len] == '.' || buf[len] == ',' )
            buf[len] = '\0';
        else
            ++len;
    }
    else
    {
        // For these values, %g works fine, and sometimes %f
        // gives a bad value (try aValue = 1.222222222222, with %.16f format!)
        len = snprintf( buf, sizeof( buf ), "%.10g", aValue );
    }

    return std::string( buf, len );
}


wxString From_UTF8( const char* cstring )
{
    // Convert an expected UTF8 encoded C string to a wxString
    wxString line = wxString::FromUTF8( cstring );

    if( line.IsEmpty() )  // happens when cstring is not a valid UTF8 sequence
    {
        line = wxConvCurrent->cMB2WC( cstring );    // try to use locale conversion

        if( line.IsEmpty() )
            line = wxString::From8BitData( cstring );    // try to use native string
    }

    return line;
}


wxString From_UTF8( const std::string& aString )
{
    // Convert an expected UTF8 encoded std::string to a wxString
    wxString line = wxString::FromUTF8( aString );

    if( line.IsEmpty() )  // happens when aString is not a valid UTF8 sequence
    {
        line = wxConvCurrent->cMB2WC( aString.c_str() );    // try to use locale conversion

        if( line.IsEmpty() )
            line = wxString::From8BitData( aString.c_str() );    // try to use native string
    }

   return line;
}


wxString NormalizeFileUri( const wxString& aFileUri )
{
    wxString uriPathAndFileName;

    wxCHECK( aFileUri.StartsWith( wxS( "file://" ), &uriPathAndFileName ), aFileUri );

    wxString tmp = uriPathAndFileName;
    wxString retv = wxS( "file://" );

    tmp.Replace( wxS( "\\" ), wxS( "/" ) );
    tmp.Replace( wxS( ":" ), wxS( "" ) );

    if( !tmp.IsEmpty() && tmp[0] != '/' )
        tmp = wxS( "/" ) + tmp;

    retv += tmp;

    return retv;
}


namespace
{
    // Extract (prefix, numericValue) where numericValue = -1 if no numeric suffix
    std::pair<wxString, long> ParseAlphaNumericPin( const wxString& pinNum )
    {
        wxString prefix;
        long     numValue = -1;

        size_t numStart = pinNum.length();
        for( int i = static_cast<int>( pinNum.length() ) - 1; i >= 0; --i )
        {
            if( !wxIsdigit( pinNum[i] ) )
            {
                numStart = i + 1;
                break;
            }
            if( i == 0 )
                numStart = 0; // all digits
        }

        if( numStart < pinNum.length() )
        {
            prefix = pinNum.Left( numStart );
            wxString numericPart = pinNum.Mid( numStart );
            numericPart.ToLong( &numValue );
        }

        return { prefix, numValue };
    }
}

std::vector<wxString> ExpandStackedPinNotation( const wxString& aPinName, bool* aValid )
{
    if( aValid )
        *aValid = true;

    std::vector<wxString> expanded;

    const bool hasOpenBracket  = aPinName.Contains( wxT( "[" ) );
    const bool hasCloseBracket = aPinName.Contains( wxT( "]" ) );

    if( hasOpenBracket || hasCloseBracket )
    {
        if( !aPinName.StartsWith( wxT( "[" ) ) || !aPinName.EndsWith( wxT( "]" ) ) )
        {
            if( aValid )
                *aValid = false;
            expanded.push_back( aPinName );
            return expanded;
        }
    }

    if( !aPinName.StartsWith( wxT( "[" ) ) || !aPinName.EndsWith( wxT( "]" ) ) )
    {
        expanded.push_back( aPinName );
        return expanded;
    }

    const wxString inner = aPinName.Mid( 1, aPinName.Length() - 2 );

    size_t start = 0;
    while( start < inner.length() )
    {
        size_t comma = inner.find( ',', start );
        wxString part = ( comma == wxString::npos ) ? inner.Mid( start ) : inner.Mid( start, comma - start );
        part.Trim( true ).Trim( false );
        if( part.empty() )
        {
            start = ( comma == wxString::npos ) ? inner.length() : comma + 1;
            continue;
        }

        int dashPos = part.Find( '-' );
        if( dashPos != wxNOT_FOUND )
        {
            wxString startTxt = part.Left( dashPos );
            wxString endTxt   = part.Mid( dashPos + 1 );
            startTxt.Trim( true ).Trim( false );
            endTxt.Trim( true ).Trim( false );

            auto [startPrefix, startVal] = ParseAlphaNumericPin( startTxt );
            auto [endPrefix, endVal]     = ParseAlphaNumericPin( endTxt );

            if( startPrefix != endPrefix || startVal == -1 || endVal == -1 || startVal > endVal )
            {
                if( aValid )
                    *aValid = false;
                expanded.clear();
                expanded.push_back( aPinName );
                return expanded;
            }

            for( long ii = startVal; ii <= endVal; ++ii )
            {
                if( startPrefix.IsEmpty() )
                    expanded.emplace_back( wxString::Format( wxT( "%ld" ), ii ) );
                else
                    expanded.emplace_back( wxString::Format( wxT( "%s%ld" ), startPrefix, ii ) );
            }
        }
        else
        {
            expanded.push_back( part );
        }

        if( comma == wxString::npos )
            break;
        start = comma + 1;
    }

    if( expanded.empty() )
    {
        expanded.push_back( aPinName );
        if( aValid )
            *aValid = false;
    }

    return expanded;
}


int CountStackedPinNotation( const wxString& aPinName, bool* aValid )
{
    if( aValid )
        *aValid = true;

    // Fast path: if no brackets, it's a single pin
    const bool hasOpenBracket  = aPinName.Contains( wxT( "[" ) );
    const bool hasCloseBracket = aPinName.Contains( wxT( "]" ) );

    if( hasOpenBracket || hasCloseBracket )
    {
        if( !aPinName.StartsWith( wxT( "[" ) ) || !aPinName.EndsWith( wxT( "]" ) ) )
        {
            if( aValid )
                *aValid = false;
            return 1;
        }
    }

    if( !aPinName.StartsWith( wxT( "[" ) ) || !aPinName.EndsWith( wxT( "]" ) ) )
        return 1;

    const wxString inner = aPinName.Mid( 1, aPinName.Length() - 2 );

    int count = 0;
    size_t start = 0;

    while( start < inner.length() )
    {
        size_t comma = inner.find( ',', start );
        wxString part = ( comma == wxString::npos ) ? inner.Mid( start ) : inner.Mid( start, comma - start );
        part.Trim( true ).Trim( false );

        if( part.empty() )
        {
            start = ( comma == wxString::npos ) ? inner.length() : comma + 1;
            continue;
        }

        int dashPos = part.Find( '-' );
        if( dashPos != wxNOT_FOUND )
        {
            wxString startTxt = part.Left( dashPos );
            wxString endTxt   = part.Mid( dashPos + 1 );
            startTxt.Trim( true ).Trim( false );
            endTxt.Trim( true ).Trim( false );

            auto [startPrefix, startVal] = ParseAlphaNumericPin( startTxt );
            auto [endPrefix, endVal]     = ParseAlphaNumericPin( endTxt );

            if( startPrefix != endPrefix || startVal == -1 || endVal == -1 || startVal > endVal )
            {
                if( aValid )
                    *aValid = false;

                return 1;
            }

            // Count pins in the range
            count += static_cast<int>( endVal - startVal + 1 );
        }
        else
        {
            // Single pin
            ++count;
        }

        if( comma == wxString::npos )
            break;

        start = comma + 1;
    }

    if( count == 0 )
    {
        if( aValid )
            *aValid = false;

        return 1;
    }

    return count;
}


wxString GetDefaultVariantName()
{
    return wxString( defaultVariantName );
}


int SortVariantNames( const wxString& aLhs, const wxString& aRhs )
{
    if( ( aLhs == defaultVariantName ) && ( aRhs != defaultVariantName ) )
        return -1;

    if( ( aLhs != defaultVariantName ) && ( aRhs == defaultVariantName ) )
        return 1;

    return StrNumCmp( aLhs, aRhs );
}


std::vector<LOAD_MESSAGE> ExtractLibraryLoadErrors( const wxString& aErrorString, int aSeverity )
{
    std::vector<LOAD_MESSAGE> messages;

    if( aErrorString.IsEmpty() )
        return messages;

    // Errors are separated by newlines. We want to keep:
    // - Lines starting with "Library '" (library-level errors)
    // - Lines containing "Expecting" (file error location)
    // And strip:
    // - Lines starting with "from " (internal code location info)
    wxStringTokenizer tokenizer( aErrorString, wxS( "\n" ), wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
    {
        wxString line = tokenizer.GetNextToken();

        // Skip internal code location lines (e.g., "from pcb_io_kicad_sexpr_parser.cpp : ...")
        if( line.StartsWith( wxS( "from " ) ) )
            continue;

        if( line.StartsWith( wxS( "Library '" ) ) || line.Contains( wxS( "Expecting" ) ) )
            messages.push_back( { line, static_cast<SEVERITY>( aSeverity ) } );
    }

    return messages;
}

