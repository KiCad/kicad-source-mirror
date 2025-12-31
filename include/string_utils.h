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

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <algorithm>
#include <string>
#include <vector>

#include <wx/string.h>
#include <wx/filename.h>

#include <kicommon.h>

void ConvertMarkdown2Html( const wxString& aMarkdownInput, wxString& aHtmlOutput );

/**
 * Convert the old `~...~` overbar notation to the new `~{...}` one.
 */
KICOMMON_API wxString ConvertToNewOverbarNotation( const wxString& aOldStr );

/**
 * Convert curly quotes and em/en dashes to straight quotes and dashes.
 *
 * @return true if any characters required conversion.
 */
KICOMMON_API bool ConvertSmartQuotesAndDashes( wxString* aString );

/**
 * Escape/Unescape routines to safely encode reserved-characters in various contexts.
 */
enum ESCAPE_CONTEXT
{
    CTX_NETNAME,
    CTX_LIBID,
    CTX_LEGACY_LIBID,
    CTX_IPC,
    CTX_QUOTED_STR,
    CTX_JS_STR,
    CTX_LINE,
    CTX_CSV,
    CTX_FILENAME,
    CTX_NO_SPACE        // to replace spaces in names that do not accept spaces
};

/**
 * The Escape/Unescape routines use HTML-entity-reference-style encoding to handle
 * characters which are:
 *   (a) not legal in filenames
 *   (b) used as control characters in LIB_IDs
 *   (c) used to delineate hierarchical paths
 */
KICOMMON_API wxString EscapeString( const wxString& aSource, ESCAPE_CONTEXT aContext );

KICOMMON_API wxString UnescapeString( const wxString& aSource );

/**
 * Remove markup (such as overbar or subscript) that we can't render to menu items.
 */
KICOMMON_API wxString PrettyPrintForMenu( const wxString& aString );

/**
 * Capitalize the first letter in each word.
 */
KICOMMON_API wxString TitleCaps( const wxString& aString );

/**
 * Capitalize only the first word.
 */
KICOMMON_API wxString InitialCaps( const wxString& aString );

/**
 * Copy bytes from @a aSource delimited string segment to @a aDest buffer.
 *
 * The extracted string will be null terminated even if truncation is necessary
 * because aDestSize was not large enough.
 *
 * @param aDest is the destination byte buffer.
 * @param aSource is the source bytes as a C string.
 * @param aDestSize is the size of the destination byte buffer.
 * @return the number of bytes read from source, which may be more than the number copied,
 *         due to escaping of double quotes and the escape byte itself.
 * @deprecated should use the one which fetches a wxString, below.
 */
KICOMMON_API int ReadDelimitedText( char* aDest, const char* aSource, int aDestSize );

/**
 * Copy bytes from @a aSource delimited string segment to @a aDest wxString.
 *
 * @param aDest is the destination wxString.
 * @param aSource is the source C string holding utf8 encoded bytes.
 * @return the number of bytes read from source, which may be more than the number copied,
 *         due to escaping of double quotes and the escape byte itself.
 */
KICOMMON_API int ReadDelimitedText( wxString* aDest, const char* aSource );

/**
 * Return an 8 bit UTF8 string given aString in Unicode form.
 *
 * Any double quoted or back slashes are prefixed with a '\\' byte and the form
 * of this UTF8 byte string is compatible with function ReadDelimitedText().
 *
 * @param aString is the input string to convert.
 * @return the escaped input text, without the wrapping double quotes.
 */
KICOMMON_API std::string EscapedUTF8( const wxString& aString );

/**
 * Return a new wxString escaped for embedding in HTML.
 */
KICOMMON_API wxString EscapeHTML( const wxString& aString );

/**
 * Return a new wxString unescaped from HTML format.
 */
KICOMMON_API wxString UnescapeHTML( const wxString& aString );

/**
 * Removes HTML tags from a string.
 *
 * Do not use for filtering potentially malicious inputs and rendering as HTML
 * without escaping.
 */
KICOMMON_API wxString RemoveHTMLTags( const wxString& aInput );

/**
 * Wraps links in HTML <a href=""></a> tags.
 */
KICOMMON_API wxString LinkifyHTML( wxString aStr );

/**
 * Performs a URL sniff-test on a string.
 */
KICOMMON_API bool IsURL( wxString aStr );

/**
 * Read one line line from \a aFile.
 *
 * @return a pointer the first useful line read by eliminating blank lines and comments.
 */
KICOMMON_API char* GetLine( FILE* aFile, char* Line, int* LineNum = nullptr, int SizeLine = 255 );

/**
 * Return true if the string is empty or contains only whitespace.
 */
KICOMMON_API bool NoPrintableChars( const wxString& aString );

/**
 * Return the number of printable (ie: non-formatting) chars.  Used to approximate rendered
 * text size when speed is more important than accuracy.
 */
KICOMMON_API int PrintableCharCount( const wxString& aString );

/**
 * Remove leading and training spaces, tabs and end of line chars in \a text
 *
 * @return a pointer on the first n char in text
 */
KICOMMON_API char* StrPurge( char* text );

/**
 * @return a string giving the current date and time.
 */
KICOMMON_API wxString GetISO8601CurrentDateTime();

/**
 * Compare two strings with alphanumerical content.
 *
 * This function is equivalent to strncmp() or strncasecmp() if \a aIgnoreCase is true
 * except that strings containing numbers are compared by their integer value not
 * by their ASCII code.  In other words U10 would be greater than U2.
 *
 * @param aString1 A wxString reference to the reference string.
 * @param aString2 A wxString reference to the comparison string.
 * @param aIgnoreCase Use true to make the comparison case insensitive.
 * @return An integer value of -1 if \a aString1 is less than \a aString2, 0 if
 *         \a aString1 is equal to \a aString2, or 1 if \a aString1 is greater
 *         than \a aString2.
 */
KICOMMON_API int StrNumCmp( const wxString& aString1, const wxString& aString2,
                            bool aIgnoreCase = false );


enum class CASE_SENSITIVITY
{
    SENSITIVE,
    INSENSITIVE
};


/**
 * Sort a container of wxString objects, in place, using the StrNumCmp() function.
 */
template <typename T>
inline void StrNumSort( T& aList, CASE_SENSITIVITY aCaseSensitivity )
{
    std::sort( aList.begin(), aList.end(),
               [aCaseSensitivity]( const wxString& lhs, const wxString& rhs )
               {
                   return StrNumCmp( lhs, rhs, aCaseSensitivity == CASE_SENSITIVITY::INSENSITIVE ) < 0;
               } );
}


/**
 * Compare a string against wild card (* and ?) pattern using the usual rules.
 *
 * @return true if pattern matched otherwise false.
 */
KICOMMON_API bool WildCompareString( const wxString& pattern,
                        const wxString& string_to_tst,
                        bool            case_sensitive = true );

/**
 * Compare strings like the strcmp function but handle numbers and modifiers within the
 * string text correctly for sorting.  eg. 1mF > 55uF
 *
 * @return -1 if first string is less than the second, 0 if the strings are equal, or
 *          1 if the first string is greater than the second.
 */
KICOMMON_API int ValueStringCompare( const wxString& strFWord, const wxString& strSWord );

/**
 * Break a string into three parts: he alphabetic preamble, the numeric part, and any
 * alphabetic ending.
 *
 * For example C10A is split to C 10 A
 */
KICOMMON_API int SplitString( const wxString& strToSplit,
                 wxString* strBeginning,
                 wxString* strDigits,
                 wxString* strEnd );

/**
 * Gets the trailing int, if any, from a string.
 *
 * @param aStr the string to check.
 * @return the trailing int or 0 if none found.
 */
KICOMMON_API int GetTrailingInt( const wxString& aStr );

/**
 * @return a wxString object containing the illegal file name characters for all platforms.
 */
KICOMMON_API wxString GetIllegalFileNameWxChars();

/**
 * Checks if a full filename is valid, i.e. does not contains illegal chars
 * path separators are allowed
 * @return true if OK.
 */
KICOMMON_API bool IsFullFileNameValid( const wxString& aFullFilename );

/**
 * Checks \a aName for illegal file name characters.
 *
 * The Windows (DOS) file system forbidden characters already include the forbidden file
 * name characters for both Posix and OSX systems.  The characters \/?*|"\<\> are illegal
 * and are replaced with %xx where xx the hexadecimal equivalent of the replaced character.
 * This replacement may not be as elegant as using an underscore ('_') or hyphen ('-') but
 * it guarantees that there will be no naming conflicts when fixing footprint library names.
 * however, if aReplaceChar is given, it will replace the illegal chars
 *
 * @param aName is a point to a std::string object containing the footprint name to verify.
 * @param aReplaceChar (if not 0) is the replacement char.
 * @return true if any characters have been replaced in \a aName.
 */
KICOMMON_API bool ReplaceIllegalFileNameChars( std::string& aName, int aReplaceChar = 0 );
KICOMMON_API bool  ReplaceIllegalFileNameChars( wxString& aName, int aReplaceChar = 0 );


/**
 * A helper for sorting strings from the rear.
 *
 * Useful for things like 3D model names where they tend to be largely repetitious at the front.
 */
struct rsort_wxString
{
    bool operator() ( const wxString& strA, const wxString& strB ) const
    {
        wxString::const_reverse_iterator sA = strA.rbegin();
        wxString::const_reverse_iterator eA = strA.rend();

        wxString::const_reverse_iterator sB = strB.rbegin();
        wxString::const_reverse_iterator eB = strB.rend();

        if( strA.empty() )
        {
            if( strB.empty() )
                return false;

            // note: this rule implies that a null string is first in the sort order
            return true;
        }

        if( strB.empty() )
            return false;

        while( sA != eA && sB != eB )
        {
            if( ( *sA ) == ( *sB ) )
            {
                ++sA;
                ++sB;
                continue;
            }

            if( ( *sA ) < ( *sB ) )
                return true;
            else
                return false;
        }

        if( sB == eB )
            return false;

        return true;
    }
};

/**
 * Split the input string into a vector of output strings
 *
 * @note Multiple delimiters are considered to be separate records with empty strings
 *
 * @param aStr Input string with 0 or more delimiters.
 * @param aDelim The string of delimiter.  Multiple characters here denote alternate delimiters.
 * @return a vector of strings
 */
static inline std::vector<std::string> split( const std::string& aStr, const std::string& aDelim )
{
    size_t pos = 0;
    size_t last_pos = 0;
    size_t len;

    std::vector<std::string> tokens;

    while( pos < aStr.size() )
    {
        pos = aStr.find_first_of( aDelim, last_pos );

        if( pos == std::string::npos )
            pos = aStr.size();

        len = pos - last_pos;

        tokens.push_back( aStr.substr( last_pos, len ) );

        last_pos = pos + 1;
    }

    return tokens;
}

/// Utility to build comma separated lists in messages
inline void AccumulateDescription( wxString& aDesc, const wxString& aItem )
{
    if( !aDesc.IsEmpty() )
        aDesc << wxT( ", " );

    aDesc << aItem;
}


/**
 * Build a comma-separated list from a collection of wxStrings.
 * (e.g. std::vector, wxArrayString, etc).
 */
template <typename T>
inline void AccumulateDescriptions( wxString& aDesc, const T& aItemCollection )
{
    for( const auto& item : aItemCollection )
        AccumulateDescription( aDesc, item );
}


template <typename T>
inline wxString AccumulateDescriptions( const T& aItemCollection )
{
    wxString desc;
    AccumulateDescriptions( desc, aItemCollection );
    return desc;
}

/**
 * Split \a aString to a string list separated at \a aSplitter.
 *
 * @param aText is the text to split.
 * @param aStrings will contain the split lines.
 * @param aSplitter is the 'split' character.
 */
KICOMMON_API void wxStringSplit( const wxString& aText, wxArrayString& aStrings, wxChar aSplitter );

/**
 * Remove trailing zeros from a string containing a converted float number.
 *
 * The trailing zeros are removed if the mantissa has more than \a aTrailingZeroAllowed
 * digits and some trailing zeros.
 */
KICOMMON_API void StripTrailingZeros( wxString& aStringValue, unsigned aTrailingZeroAllowed = 1 );

/**
 * Print a float number without using scientific notation and no trailing 0
 * We want to avoid scientific notation in S-expr files (not easy to read)
 * for floating numbers.
 *
 * We cannot always just use the %g or the %f format to print a fp number
 * this helper function uses the %f format when needed, or %g when %f is
 * not well working and then removes trailing 0
 */
KICOMMON_API std::string UIDouble2Str( double aValue );

/**
 * Print a float number without using scientific notation and no trailing 0
 * This function is intended in uses to write to file, it ignores locale
 *
 * We cannot always just use the %g or the %f format to print a fp number
 * this helper function uses the %f format when needed, or %g when %f is
 * not well working and then removes trailing 0
 */
KICOMMON_API std::string FormatDouble2Str( double aValue );

/**
 * Convert a wxString to a UTF8 encoded C string for all wxWidgets build modes.
 *
 * wxstring is a wxString, not a wxT() or _().  The scope of the return value
 * is very limited and volatile, but can be used with printf() style functions well.
 *
 * @note Trying to convert it to a function is tricky because of the type of the
 *       parameter!
 */
#define TO_UTF8( wxstring ) ( (const char*) ( wxstring ).utf8_str() )

/**
 * Convert an expected UTF8 encoded std::string to a wxString.
 * If fails, try to convert using current locale
 * If still fails, return the initial string (can be already a converted string)
 */
KICOMMON_API wxString From_UTF8( const std::string& aString );
KICOMMON_API wxString  From_UTF8( const char* cstring );

/**
 * Normalize file path \a aFileUri to URI convention.
 *
 * Unfortunately none of the wxWidgets objects results in acceptable file URIs which breaks
 * PDF plotting URI links.  This is an attempt to normalize Windows local file paths to a
 * URI that PDF readers that can run JavaScript can handle.
 *
 * @note This does not expand environment or user variables.  Variable expansion should be
 *       performed before calling.  If \a aFileUri does not begin with 'file://', \a aFileUri
 *       returned unchanged.
 *
 * @param aFileUri is the string to be normalized.
 * @return the normalized string.
 */
KICOMMON_API wxString NormalizeFileUri( const wxString& aFileUri );

/**
 * Expand stacked pin notation like [1,2,3], [1-4], [A1-A4], or [AA1-AA3,AB4,CD12-CD14]
 * into individual pin numbers, supporting both numeric and alphanumeric pin prefixes.
 *
 * Examples:
 *   "[1,2,3]" -> {"1", "2", "3"}
 *   "[1-4]" -> {"1", "2", "3", "4"}
 *   "[A1-A3]" -> {"A1", "A2", "A3"}
 *   "[AA1-AA3,AB4]" -> {"AA1", "AA2", "AA3", "AB4"}
 *   "5" -> {"5"} (non-bracketed pins returned as-is)
 *
 * @param aPinName is the pin name to expand (may or may not use stacked notation)
 * @param aValid is optionally set to indicate whether the notation was valid
 * @return vector of individual pin numbers
 */
KICOMMON_API std::vector<wxString> ExpandStackedPinNotation( const wxString& aPinName,
                                                            bool* aValid = nullptr );

/**
 * Count the number of pins represented by stacked pin notation without allocating strings.
 *
 * This is a fast alternative to ExpandStackedPinNotation().size() for cases where only
 * the count is needed.
 *
 * @param aPinName is the pin name to count (may or may not use stacked notation)
 * @param aValid is optionally set to indicate whether the notation was valid
 * @return count of individual pins represented (always >= 1)
 */
KICOMMON_API int CountStackedPinNotation( const wxString& aPinName, bool* aValid = nullptr );


KICOMMON_API wxString GetDefaultVariantName();

KICOMMON_API int SortVariantNames( const wxString& aLhs, const wxString& aRhs );

struct LOAD_MESSAGE;

/**
 * Parse library load error messages, extracting user-facing information while
 * stripping internal code locations.
 *
 * @param aErrorString is the raw error string from GetLibraryLoadErrors()
 * @param aSeverity is the severity to assign to all extracted messages
 * @return vector of LOAD_MESSAGE with cleaned error text
 */
KICOMMON_API std::vector<LOAD_MESSAGE> ExtractLibraryLoadErrors( const wxString& aErrorString,
                                                                  int aSeverity );

#endif  // STRING_UTILS_H
