/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file string.cpp
 * @brief Some useful functions to handle strings.
 */

#include <fctsys.h>
#include <macros.h>
#include <richio.h>                        // StrPrintf
#include <kicad_string.h>


/**
 * Illegal file name characters used to insure file names will be valid on all supported
 * platforms.  This is the list of illegal file name characters for Windows which includes
 * the illegal file name characters for Linux and OSX.
 */
static const char illegalFileNameChars[] = "\\/:\"<>|";


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

    *aDest = FROM_UTF8( utf8.c_str() );

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

    while( (cc = *aSource++) != 0 && aDest < limit )
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
    std::string utf8 = TO_UTF8( aString );

    std::string ret;

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


wxString EscapedHTML( const wxString& aString )
{
    wxString converted;

    for( wxUniChar c: aString )
    {
        if( c == '\"' )
            converted += "&quot;";
        else if( c == '\'' )
            converted += "&apos;";
        else if( c == '&' )
            converted += "&amp;";
        else if( c == '<' )
            converted += "&lt;";
        else if( c == '>' )
            converted += "&gt;";
        else
            converted += c;
    }

    return converted;
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
        if( fgets( Line, SizeLine, File ) == NULL )
            return NULL;

        if( LineNum )
            *LineNum += 1;

    } while( Line[0] == '#' || Line[0] == '\n' ||  Line[0] == '\r' || Line[0] == 0 );

    strtok( Line, "\n\r" );
    return Line;
}


wxString DateAndTime()
{
    wxDateTime datetime = wxDateTime::Now();

    datetime.SetCountry( wxDateTime::Country_Default );
    return datetime.Format( wxDefaultDateTimeFormat, wxDateTime::Local );
}


int StrNumCmp( const wxString& aString1, const wxString& aString2, int aLength, bool aIgnoreCase )
{
    int i;
    int nb1 = 0, nb2 = 0;

    wxString::const_iterator str1 = aString1.begin(), str2 = aString2.begin();

    if( ( str1 == aString1.end() ) || ( str2 == aString2.end() ) )
        return 0;

    for( i = 0; i < aLength; i++ )
    {
        if( isdigit( *str1 ) && isdigit( *str2 ) ) /* digit found */
        {
            nb1 = 0;
            nb2 = 0;

            while( isdigit( *str1 ) )
            {
                nb1 = nb1 * 10 + (int) *str1 - '0';
                ++str1;
            }

            while( isdigit( *str2 ) )
            {
                nb2 = nb2 * 10 + (int) *str2 - '0';
                ++str2;
            }

            if( nb1 < nb2 )
                return -1;

            if( nb1 > nb2 )
                return 1;
        }

        if( aIgnoreCase )
        {
            if( toupper( *str1 ) < toupper( *str2 ) )
                return -1;

            if( toupper( *str1 ) > toupper( *str2 ) )
                return 1;

            if( ( *str1 == 0 ) && ( *str2 == 0 ) )
                return 0;
        }
        else
        {
            if( *str1 < *str2 )
                return -1;

            if( *str1 > *str2 )
                return 1;

            if( ( str1 == aString1.end() ) && ( str2 == aString2.end() ) )
                return 0;
        }

        ++str1;
        ++str2;
    }

    return 0;
}


bool WildCompareString( const wxString& pattern, const wxString& string_to_tst,
                        bool case_sensitive )
{
    const wxChar* cp = NULL, * mp = NULL;
    const wxChar* wild, * string;
    wxString      _pattern, _string_to_tst;

    if( case_sensitive )
    {
        wild   = pattern.GetData();
        string = string_to_tst.GetData();
    }
    else
    {
        _pattern = pattern;
        _pattern.MakeUpper();
        _string_to_tst = string_to_tst;
        _string_to_tst.MakeUpper();
        wild   = _pattern.GetData();
        string = _string_to_tst.GetData();
    }

    while( ( *string ) && ( *wild != '*' ) )
    {
        if( ( *wild != *string ) && ( *wild != '?' ) )
            return false;

        wild++; string++;
    }

    while( *string )
    {
        if( *wild == '*' )
        {
            if( !*++wild )
                return 1;
            mp = wild;
            cp = string + 1;
        }
        else if( ( *wild == *string ) || ( *wild == '?' ) )
        {
            wild++;
            string++;
        }
        else
        {
            wild   = mp;
            string = cp++;
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
    static const wxString modifiers( wxT( "pnumkKM" ) );

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
            && !units.CmpNoCase( wxT( "F" ) )
            && !units.CmpNoCase( wxT( "hz" ) )
            && !units.CmpNoCase( wxT( "W" ) )
            && !units.CmpNoCase( wxT( "V" ) )
            && !units.CmpNoCase( wxT( "H" ) ) )
        return false;

    if( modifier == 'p' )
        value *= 1.0e-12;
    if( modifier == 'n' )
        value *= 1.0e-9;
    else if( modifier == 'u' )
        value *= 1.0e-6;
    else if( modifier == 'm' )
        value *= 1.0e-3;
    else if( modifier == 'k' || modifier == 'K' )
        value *= 1.0e3;
    else if( modifier == 'M' )
        value *= 1.0e6;
    else if( modifier == 'G' )
        value *= 1.0e9;

    return true;
}


// Should handle:
// a) Purely numerical e.g. '22'
// b) Numerical with included units e.g. '15uF'
// c) Numerical with included prefix but no units e.g. '20n'
// d) Numerical with prefix inside number e.g. '4K7'
// e) Other, e.g. 'MAX232'
//
// TODO: case (d) unimplemented !!!
//
int ValueStringCompare( const wxString& strFWord, const wxString& strSWord )
{
    // The different sections of the two strings
    wxString strFWordBeg, strFWordMid, strFWordEnd;
    wxString strSWordBeg, strSWordMid, strSWordEnd;

    // Split the two strings into separate parts
    SplitString( strFWord, &strFWordBeg, &strFWordMid, &strFWordEnd );
    SplitString( strSWord, &strSWordBeg, &strSWordMid, &strSWordEnd );

    // Compare the Beginning section of the strings
    int isEqual = strFWordBeg.CmpNoCase( strSWordBeg );

    if( isEqual > 0 )
        return 1;
    else if( isEqual < 0 )
        return -1;
    else
    {
        // If the first sections are equal compare their digits
        double lFirstNumber  = 0;
        double lSecondNumber = 0;
        bool   endingIsModifier = false;

        strFWordMid.ToDouble( &lFirstNumber );
        strSWordMid.ToDouble( &lSecondNumber );

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


int RefDesStringCompare( const wxString& strFWord, const wxString& strSWord )
{
    // The different sections of the two strings
    wxString strFWordBeg, strFWordMid, strFWordEnd;
    wxString strSWordBeg, strSWordMid, strSWordEnd;

    // Split the two strings into separate parts
    SplitString( strFWord, &strFWordBeg, &strFWordMid, &strFWordEnd );
    SplitString( strSWord, &strSWordBeg, &strSWordMid, &strSWordEnd );

    // Compare the Beginning section of the strings
    int isEqual = strFWordBeg.CmpNoCase( strSWordBeg );

    if( isEqual > 0 )
        return 1;
    else if( isEqual < 0 )
        return -1;
    else
    {
        // If the first sections are equal compare their digits
        long lFirstDigit  = 0;
        long lSecondDigit = 0;

        strFWordMid.ToLong( &lFirstDigit );
        strSWordMid.ToLong( &lSecondDigit );

        if( lFirstDigit > lSecondDigit )
            return 1;
        else if( lFirstDigit < lSecondDigit )
            return -1;
        // If the first two sections are equal compare the endings
        else
            return strFWordEnd.CmpNoCase( strSWordEnd );
    }
}


int SplitString( wxString  strToSplit,
                 wxString* strBeginning,
                 wxString* strDigits,
                 wxString* strEnd )
{
    static const wxString separators( wxT( ".," ) );

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
        if( isdigit( strToSplit[ii] ) )
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
            if( !isdigit( strToSplit[ii] ) && separators.Find( strToSplit[ii] ) < 0 )
                break;
        }

        // If all that was left was digits, then just set the digits string
        if( ii < 0 )
            *strDigits = strToSplit.substr( 0, position );

        /* We were only looking for the last set of digits everything else is
         * part of the preamble */
        else
        {
            *strDigits    = strToSplit.substr( ii + 1, position - ii - 1 );
            *strBeginning = strToSplit.substr( 0, ii + 1 );
        }
    }

    return 0;
}


wxString GetIllegalFileNameWxChars()
{
    return FROM_UTF8( illegalFileNameChars );
}


bool ReplaceIllegalFileNameChars( std::string* aName, int aReplaceChar )
{
    bool changed = false;
    std::string result;
    result.reserve( aName->length() );

    for( std::string::iterator it = aName->begin();  it != aName->end();  ++it )
    {
        if( strchr( illegalFileNameChars, *it ) )
        {
            if( aReplaceChar )
                StrPrintf( &result, "%c", aReplaceChar );
            else
                StrPrintf( &result, "%%%02x", *it );

            changed = true;
        }
        else
        {
            result += *it;
        }
    }

    if( changed )
        *aName = result;

    return changed;
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
        aName = result;

    return changed;
}
