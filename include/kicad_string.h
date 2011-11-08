/**
 * This file is part of the common library \n
 * Custom string manipulation routines.
 * @file  kicad_string.h
 * @see   common.h, string.cpp
 */


#ifndef KICAD_STRING_H_
#define KICAD_STRING_H_

#include <wx/string.h>


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
 * Funxtion StrPurge
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
 * @param aString1 A wxChar pointer to the reference string.
 * @param aString2 A wxChar pointer to the comparison string.
 * @param aLength The numbere of characters to compare.  Set to -1 to compare
 *                the entire string.
 * @param aIgnoreCase Use true to make the comparison case insensitive.
 * @return An integer value of -1 if \a aString1 is less than \a aString2, 0 if
 *         \a aString1 is equal to \a aString2, or 1 if \a aString1 is greater
 *         than \a aString2.
 */
int StrNumCmp( const wxChar* aString1, const wxChar* aString2, int aLength = INT_MAX,
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
 * Function to_point
 * converts a string used to compensate for internalization of printf().  It generates
 * floating point numbers with a comma instead of point.
 * @deprecated Use SetLocaleTo_C_standard instead.
 */
char* to_point( char* Text );

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

#endif  // KICAD_STRING_H_
