/*********************************************/
/*               string.cpp                  */
/*   some useful functions to handle strings */
/*********************************************/

#include "fctsys.h"
#include "macros.h"
#include "kicad_string.h"


int  ReadDelimitedText( char* aDest, const char* aSource, int aDestSize )
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

                // do no copy the escape byte if it is followed by \ or "
                if( cc != '"' && cc != '\\' )
                    *aDest++ = '\\';

                if( aDest < limit )
                    *aDest++ = cc;
            }
            else
                *aDest++ = cc;
        }
    }

    *aDest = 0;

    return aSource - start;
}


std::string EscapedUTF8( const wxString& aString )
{
    std::string utf8 = TO_UTF8( aString );

    std::string ret;

    // ret += '"';

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
            ret += *it;
    }

    // ret += '"';

    return ret;
}


/* Remove leading and training spaces, tabs and end of line chars in text
 * return a pointer on the first n char in text
 */
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


/* Read lines from File
 *  Skip void lines and comments (starting by #)
 *  return the first non void line.
 *  increments *LineNum for each line
 */
char* GetLine( FILE* File, char* Line, int* LineNum, int SizeLine )
{
    do  {
        if( fgets( Line, SizeLine, File ) == NULL )
            return NULL;
        if( LineNum )
            *LineNum += 1;
    } while( Line[0] == '#' || Line[0] == '\n' ||  Line[0] == '\r'
             || Line[0] == 0 );

    strtok( Line, "\n\r" );
    return Line;
}


/* return in aBuffer the date and time
 *  time is the local time.
 */
char* DateAndTime( char* aBuffer )
{
    wxString datetime;

    datetime = DateAndTime();
    strcpy( aBuffer, TO_UTF8( datetime ) );

    return aBuffer;
}



/* return the date and time in a wxString
 *  note: does the same thing than strftime()
 *  time is the local time.
 */
wxString DateAndTime()
{
    wxString   Line;

    wxDateTime datetime = wxDateTime::Now();

    datetime.SetCountry( wxDateTime::Country_Default );
    Line = datetime.Format( wxDefaultDateTimeFormat, wxDateTime::Local );

    return Line;
}


/*
 *  sort() function
 *  Same as strncmp() but numbers in strings
 *  are compared according to the value, not the ascii value of each digit
 */
int StrLenNumCmp( const wxChar* str1, const wxChar* str2, int NbMax )
{
    int i;
    int nb1 = 0, nb2 = 0;

    if( ( str1 == NULL ) || ( str2 == NULL ) )
        return 0;

    for( i = 0; i < NbMax; i++ )
    {
        if( isdigit( *str1 ) && isdigit( *str2 ) ) /* digit found */
        {
            nb1 = 0; nb2 = 0;
            while( isdigit( *str1 ) )
            {
                nb1 = nb1 * 10 + *str1 - '0'; str1++;
            }

            while( isdigit( *str2 ) )
            {
                nb2 = nb2 * 10 + *str2 - '0'; str2++;
            }

            if( nb1 < nb2 )
                return -1;
            if( nb1 > nb2 )
                return 1;
        }

        if( *str1 < *str2 )
            return -1;
        if( *str1 > *str2 )
            return 1;
        if( ( *str1 == 0 ) && ( *str2 == 0 ) )
            return 0;
        str1++; str2++;
    }

    return 0;
}


/*
 *  sort() function
 *  Same as stricmp() but numbers in strings
 *  are compared according to the value, not the ascii value of each digit
 */
int StrNumICmp( const wxChar* str1, const wxChar* str2 )
{
    return StrLenNumICmp( str1, str2, 32735 );
}


/*
 *  sort() function
 *  Same as strnicmp() but numbers in strings
 *  are compared according to the value, not the ascii value of each digit
 */
int StrLenNumICmp( const wxChar* str1, const wxChar* str2, int NbMax )
{
    int i;
    int nb1 = 0, nb2 = 0;

    if( ( str1 == NULL ) || ( str2 == NULL ) )
        return 0;

    for( i = 0; i < NbMax; i++ )
    {
        if( isdigit( *str1 ) && isdigit( *str2 ) ) /* find number */
        {
            nb1 = 0; nb2 = 0;
            while( isdigit( *str1 ) )
            {
                nb1 = nb1 * 10 + *str1 - '0'; str1++;
            }

            while( isdigit( *str2 ) )
            {
                nb2 = nb2 * 10 + *str2 - '0'; str2++;
            }

            if( nb1 < nb2 )
                return -1;
            if( nb1 > nb2 )
                return 1;
        }

        if( toupper( *str1 ) < toupper( *str2 ) )
            return -1;
        if( toupper( *str1 ) > toupper( *str2 ) )
            return 1;
        if( (*str1 == 0 ) && ( *str2 == 0 ) )
            return 0;
        str1++; str2++;
    }

    return 0;
}


/* compare a string to a pattern
 *  ( usual chars * and ? allowed).
 *  if case_sensitive == true, comparison is case sensitive
 *  return true if match else false
 */
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
            return FALSE;
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


/* Converts a string used to compensate for internalization of printf().
 * Generated floats with a comma instead of point.
 * Obsolete: use SetLocaleTo_C_standard instead
 */
char* to_point( char* Text )
{
    char* line = Text;

    if( Text == NULL )
        return NULL;
    for( ; *Text != 0; Text++ )
    {
        if( *Text == ',' )
            *Text = '.';
    }

    return line;
}


/* Convert string to upper case.
 * Returns pointer to the converted string.
 */
char* strupper( char* Text )
{
    char* code = Text;

    if( Text )
    {
        while( *code )
        {
            if( ( *code >= 'a' ) && ( *code <= 'z' ) )
                *code += 'A' - 'a';
            code++;
        }
    }

    return Text;
}


int RefDesStringCompare( const wxString& strFWord, const wxString& strSWord )
{
    // The different sections of the first string
    wxString strFWordBeg, strFWordMid, strFWordEnd;

    // The different sections of the second string
    wxString strSWordBeg, strSWordMid, strSWordEnd;

    int isEqual = 0;            // The numerical results of a string compare
    int iReturn = 0;            // The variable that is being returned

    long lFirstDigit  = 0;      /* The converted middle section of the first
                                 *string */
    long lSecondDigit = 0;      /* The converted middle section of the second
                                 *string */

    // Split the two strings into separate parts
    SplitString( strFWord, &strFWordBeg, &strFWordMid, &strFWordEnd );
    SplitString( strSWord, &strSWordBeg, &strSWordMid, &strSWordEnd );

    // Compare the Beginning section of the strings
    isEqual = strFWordBeg.CmpNoCase( strSWordBeg );

    if( isEqual > 0 )
        iReturn = 1;
    else if( isEqual < 0 )
        iReturn = -1;
    else
    {
        // If the first sections are equal compare their digits
        strFWordMid.ToLong( &lFirstDigit );
        strSWordMid.ToLong( &lSecondDigit );

        if( lFirstDigit > lSecondDigit )
            iReturn = 1;
        else if( lFirstDigit < lSecondDigit )
            iReturn = -1;
        else
        {
            // If the first two sections are equal compare the endings
            isEqual = strFWordEnd.CmpNoCase( strSWordEnd );

            if( isEqual > 0 )
                iReturn = 1;
            else if( isEqual < 0 )
                iReturn = -1;
            else
                iReturn = 0;
        }
    }

    return iReturn;
}


int SplitString( wxString  strToSplit,
                 wxString* strBeginning,
                 wxString* strDigits,
                 wxString* strEnd )
{
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
        *strBeginning = strToSplit;
    else
    {
        // Since there is at least one digit this is the trailing string
        *strEnd = strToSplit.substr( ii + 1 );

        // Go to the end of the digits
        int position = ii + 1;
        for( ; ii >= 0; ii-- )
        {
            if( !isdigit( strToSplit[ii] ) )
                break;
        }

        // If all that was left was digits, then just set the digits string
        if( ii < 0 )
            *strDigits = strToSplit.substr( 0, position );

        /* We were only looking for the last set of digits everything else is
         *part of the preamble */
        else
        {
            *strDigits    = strToSplit.substr( ii + 1, position - ii - 1 );
            *strBeginning = strToSplit.substr( 0, ii + 1 );
        }
    }

    return 0;
}
