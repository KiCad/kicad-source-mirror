/**
 * This file is part of the common library \n
 * Custom string manipulation routines.
 * @file  kicad_string.h
 * @see   common.h, string.cpp
 */


#ifndef KICAD_STRING_H_
#define KICAD_STRING_H_

#include <wx/string.h>

char*    strupper( char* Text );
char*    strlower( char* Text );


/**
 * Function ReadDelimitedText
 * copies bytes from @a aSource delimited string segment to @a aDest buffer.
 * The extracted string will be null terminated even if truncation is necessary
 * because aDestSize was not large enough.
 *
 * @param aDest is the destination byte buffer.
 * @param aSource is the source bytes as a C string.
 * @param aDestSize is the size of the destination byte buffer.
 * @return int - the number of bytes extracted.
 */
int  ReadDelimitedText( char* aDest, const char* aSource, int aDestSize );


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


/* Read one line line from a file.
 * Returns the first useful line read by eliminating blank lines and comments.
 */
char*    GetLine( FILE* File,
                  char* Line,
                  int*  LineNum = NULL,
                  int   SizeLine = 255 );

/* Remove leading and trailing whitespace.
 */
char*    StrPurge( char* text );


/*Return a string giving the current date and time.
*/
char*    DateAndTime( char* line );
wxString DateAndTime();


/*
 * Routine (compatible with qsort ()) to sort by alphabetical order.
 * Equivalent to strncmp () but the numbers are compared by their integer
 * value not by their ASCII code.
 */
int      StrLenNumCmp( const wxChar* str1,
                       const wxChar* str2,
                       int           NbMax );

/*
 * Routine (compatible with qsort ()) to sort by case insensitive alphabetical
 * order.
 * Equivalent to strnicmp () but the numbers are compared by their integer
 * value not by their ASCII code.
 */
int      StrNumICmp( const wxChar* str1,
                     const wxChar* str2 );


int      StrLenNumICmp( const wxChar* str1,
                        const wxChar* str2,
                        int           NbMax );

/* Compare string against wild card pattern using the usual rules.
 * (Wildcards *,?).
 * The reference string is "pattern"
 * If case_sensitive == TRUE (default), exact comparison
 * Returns TRUE if pattern matched otherwise FALSE.
 */

bool     WildCompareString( const wxString& pattern,
                            const wxString& string_to_tst,
                            bool            case_sensitive = TRUE );

/* Replaces decimal point with commas to generated international numbers.
 */
char*    to_point( char* Text );

/**
 * Function RefDesStringCompare
 * acts just like the strcmp function but treats numbers within the string text
 * correctly for sorting.  eg. A10 > A2
 * return -1 if first string is less than the second
 * return 0 if the strings are equal
 * return 1 if the first string is greater than the second
 */
int  RefDesStringCompare( const wxString& lhs, const wxString& rhs );

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
