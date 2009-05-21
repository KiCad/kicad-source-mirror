/*********************************************/
/*	             string.cpp				  	 */
/*	 some useful functions to handle strings */
/*********************************************/

#include "fctsys.h"
#include "macros.h"
#include "kicad_string.h"


/*********************************************************************/
int ReadDelimitedText( char* dest, char* source, int NbMaxChar )
/*********************************************************************/

/* read a double-quote delimited text from source and put it in in dest,
 *  read NbMaxChar bytes max
 *  return the char count read from source
 */
{
    int ii, jj, flag = 0;

    for( ii = 0, jj = 0; ii < NbMaxChar - 1; jj++, source++ )
    {
        if( *source == 0 )
            break;                      /* E.O.L. */
        if( *source == '"' )            /* delimiteur trouve */
        {
            if( flag )
                break;                  /* Fin de texte delimite */
            flag = 1;                   /* Marque 1er delimiteur trouve */
        }
        else if( flag )
        {
            *dest = *source; dest++; ii++;
        }
    }

    *dest = 0;  /* Null termined */
    return jj;
}


/********************************/
char* StrPurge( char* text )
/********************************/

/* Remove training spaces in text
 *  return a pointer on the first non space char in text
 */
{
    char* ptspace;

    if( text == NULL )
        return NULL;

    while( (*text <= ' ') && *text )
        text++;

    ptspace = text + strlen( text ) - 1;
    while( (*ptspace <= ' ') && *ptspace && (ptspace >= text) )
    {
        *ptspace = 0; ptspace--;
    }

    return text;
}


/*****************************************************************/
char* GetLine( FILE* File, char* Line, int* LineNum, int SizeLine )
/*****************************************************************/

/* Read lines from File
 *  Skip void lines and comments (starting by #)
 *  return the first non void line.
 *  increments *LineNum for ecah line
 */
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


/*******************************/
char* DateAndTime( char* aBuffer )
/*******************************/

/* return in aBuffer the date and time
 *  time is the local time.
 */
{
    wxString datetime;

    datetime = DateAndTime();
    strcpy( aBuffer, CONV_TO_UTF8( datetime ) );

    return aBuffer;
}


/*******************************/
wxString DateAndTime()
/*******************************/

/* return the date and time in a wxString
 *  note: does the same thing than strftime()
 *  time is the local time.
 */
{
    wxString   Line;

    wxDateTime datetime = wxDateTime::Now();

    datetime.SetCountry( wxDateTime::Country_Default );
    Line = datetime.Format( wxDefaultDateTimeFormat, wxDateTime::Local );

    return Line;
}


/************************************************************/
int StrLenNumCmp( const wxChar* str1, const wxChar* str2, int NbMax )
/************************************************************/

/*
 *  sort() function
 *  Same as strncmp() but numbers in strings
 *  are compared according to the value, not the ascii value of each digit
 */
{
    int i;
    int nb1 = 0, nb2 = 0;

    if( (str1 == NULL) || (str2 == NULL) )
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
        if( (*str1 == 0 ) && ( *str2 == 0 ) )
            return 0;
        str1++; str2++;
    }

    return 0;
}


/***********************************************/
int StrNumICmp( const wxChar* str1, const wxChar* str2 )
/***********************************************/

/*
 *  sort() function
 *  Same as stricmp() but numbers in strings
 *  are compared according to the value, not the ascii value of each digit
 */
{
    return StrLenNumICmp( str1, str2, 32735 );
}


/**************************************************************/
int StrLenNumICmp( const wxChar* str1, const wxChar* str2, int NbMax )
/**************************************************************/

/*
 *  sort() function
 *  Same as strnicmp() but numbers in strings
 *  are compared according to the value, not the ascii value of each digit
 */
{
    int i;
    int nb1 = 0, nb2 = 0;

    if( (str1 == NULL) || (str2 == NULL) )
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


/***********************************************************************/
bool WildCompareString( const wxString& pattern, const wxString& string_to_tst,
                        bool case_sensitive )
/***********************************************************************/

/* compare a string to a pattern
 *  ( usual chars * and ? allowed).
 *  if case_sensitive == true, comparison is case sensitive
 *  return true if match else false
 */
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

    while( (*string) && (*wild != '*') )
    {
        if( (*wild != *string) && (*wild != '?') )
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
        else if( (*wild == *string) || (*wild == '?') )
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


/***************************/
char* to_point( char* Text )
/**************************/

/* convertit les , en . dans une chaine. utilise pour compenser
 *  l'internalisation de la fct printf
 *  qui genere les flottants avec une virgule au lieu du point
 * Obsolete: use SetLocaleTo_C_standard insteed
 */
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


/********************************/
char* strupper( char* Text )
/********************************/

/* Change les caracteres 'a' ... 'z' en 'A' ... 'Z'. dans la chaine Text.
 *  Retourne Text
 */
{
    char* code = Text;

    if( Text )
    {
        while( *code )
        {
            if( (*code >= 'a') && (*code <= 'z') )
                *code += 'A' - 'a';
            code++;
        }
    }

    return Text;
}
