/**
 * This file is part of the common library \n
 * Custom string manipulation routines.
 * @file  kicad_string.h
 * @see   common.h, string.cpp
 */


#ifndef __INCLUDE__KICAD_STRING_H__
#define __INCLUDE__KICAD_STRING_H__ 1


char*    strupper( char* Text );
char*    strlower( char* Text );

/* Read string delimited with (") character.
 * Upload NbMaxChar max
 * Returns the number of codes read in source
 * dest is terminated by NULL
 */
int      ReadDelimitedText( char* dest,
                            char* source,
                            int   NbMaxChar );

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


#endif /* __INCLUDE__KICAD_STRING_H__ */
