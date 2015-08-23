/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 KiCad Developers, see change_log.txt for contributors.
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
 * @file  kicad_string.h
 * @see   common.h, string.cpp
 */


#ifndef KICAD_STRING_H_
#define KICAD_STRING_H_

#include <wx/string.h>
#include <wx/filename.h>


/**
 * Function ReadDelimitedText
 * copies bytes from @a aSource delimited string segment to @a aDest buffer.
 * The extracted string will be null terminated even if truncation is necessary
 * because aDestSize was not large enough.
 *
 * @param aDest is the destination byte buffer.
 * @param aSource is the source bytes as a C string.
 * @param aDestSize is the size of the destination byte buffer.
 * @return int - the number of bytes read from source, which may be more than
 *   the number copied, due to escaping of double quotes and the escape byte itself.
 * @deprecated should use the one which fetches a wxString, below.
 */
int ReadDelimitedText( char* aDest, const char* aSource, int aDestSize );

/**
 * Function ReadDelimitedText
 * copies bytes from @a aSource delimited string segment to @a aDest wxString.
 *
 * @param aDest is the destination wxString
 * @param aSource is the source C string holding utf8 encoded bytes.
 * @return int - the number of bytes read from source, which may be more than
 *   the number copied, due to escaping of double quotes and the escape byte itself.
 */
int ReadDelimitedText( wxString* aDest, const char* aSource );

/**
 * Function EscapedUTF8
 * returns an 8 bit UTF8 string given aString in unicode form.
 * Any double quoted or back slashes are prefixed with a '\\' byte and the form
 * of this UTF8 byte string is compatible with function ReadDelimitedText().
 *
 * @param aString is the input string to convert.
 * @return std::string - the escaped input text, without the wrapping double quotes.
 */
std::string EscapedUTF8( const wxString& aString );

/**
 * Function GetLine
 * reads one line line from \a aFile.
 * @return A pointer the first useful line read by eliminating blank lines and comments.
 */
char* GetLine( FILE* aFile, char* Line, int* LineNum = NULL, int SizeLine = 255 );

/**
 * Function StrPurge
 * removes leading and training spaces, tabs and end of line chars in \a text
 * return a pointer on the first n char in text
 */
char* StrPurge( char* text );

/**
 * Function DateAndTime
 * @return a string giving the current date and time.
 */
wxString DateAndTime();

/**
 * Function StrLenNumCmp
 * is a routine compatible with qsort() to sort by alphabetical order.
 *
 * This function is equivalent to strncmp() or strnicmp() if \a aIgnoreCase is true
 * except that strings containing numbers are compared by their integer value not
 * by their ASCII code.
 *
 * @param aString1 A wxString reference to the reference string.
 * @param aString2 A wxString reference to the comparison string.
 * @param aLength The number of characters to compare.  Set to -1 to compare
 *                the entire string.
 * @param aIgnoreCase Use true to make the comparison case insensitive.
 * @return An integer value of -1 if \a aString1 is less than \a aString2, 0 if
 *         \a aString1 is equal to \a aString2, or 1 if \a aString1 is greater
 *         than \a aString2.
 */
int StrNumCmp( const wxString& aString1, const wxString& aString2, int aLength = INT_MAX,
               bool aIgnoreCase = false );

/**
 * Function WildCompareString
 * compares a string against wild card (* and ?) pattern using the usual rules.
 * @return true if pattern matched otherwise false.
 */
bool WildCompareString( const wxString& pattern,
                        const wxString& string_to_tst,
                        bool            case_sensitive = true );

/**
 * Function RefDesStringCompare
 * acts just like the strcmp function but treats numbers within the string text
 * correctly for sorting.  eg. A10 > A2
 * return -1 if first string is less than the second
 * return 0 if the strings are equal
 * return 1 if the first string is greater than the second
 */
int RefDesStringCompare( const wxString& lhs, const wxString& rhs );

/**
 * Function SplitString
 * breaks a string into three parts.
 * The alphabetic preamble
 * The numeric part
 * Any alphabetic ending
 * For example C10A is split to C 10 A
 */
int SplitString( wxString  strToSplit,
                 wxString* strBeginning,
                 wxString* strDigits,
                 wxString* strEnd );

/**
 * Function GetIllegalFileNameWxChars
 * @return a wString object containing the illegal file name characters for all platforms.
 */
wxString GetIllegalFileNameWxChars();

/**
 * Function ReplaceIllegalFileNameChars
 * checks \a aName for illegal file name characters.
 *
 * The Windows (DOS) file system forbidden characters already include the forbidden file
 * name characters for both Posix and OSX systems.  The characters \/?*|"\<\> are illegal
 * and are replaced with %xx where xx the hexadecimal equivalent of the replaced character.
 * This replacement may not be as elegant as using an underscore ('_') or hyphen ('-') but
 * it guarentees that there will be no naming conflicts when fixing footprint library names.
 * however, if aReplaceChar is given, it will replace the illegal chars
 *
 * @param aName is a point to a std::string object containing the footprint name to verify.
 * @param aReplaceChar (if not 0) is the replacement char.
 * @return true if any characters have been replaced in \a aName.
 */
bool ReplaceIllegalFileNameChars( std::string* aName, int aReplaceChar = 0 );

#ifndef HAVE_STRTOKR
// common/strtok_r.c optionally:
extern "C" char* strtok_r( char* str, const char* delim, char** nextp );
#endif

#endif  // KICAD_STRING_H_
