/****************************************************************************/
/*	 MODULE:					 string.cpp									*/
/*	 ROLE: fonctions complementaires de traitement de chaines de caracteres */
/****************************************************************************/

#include "fctsys.h"
#include <time.h>
#include "common.h"


/*********************************************************************/
int ReadDelimitedText(char * dest, char * source, int NbMaxChar )
/*********************************************************************/
/* lit et place dans dest la chaine de caractere trouvee dans source,
	delimitee par " .
	transfere NbMaxChar max
	retourne le nombre de codes lus dans source
	dest est termine par NULL
*/
{
int ii, jj, flag = 0;

	 for ( ii = 0, jj = 0; ii < NbMaxChar - 1 ; jj++, source++)
		{
		if ( * source == 0 ) break; /* fin de ligne */
		if ( * source == '"' )			/* delimiteur trouve */
			{
			if ( flag ) break;			/* Fin de texte delimite */
			flag = 1;					/* Marque 1er delimiteur trouve */
			}
		else if ( flag )
			{
			* dest = * source; dest++; ii++;
			}
		}
	*dest = 0;  /* Null terminaison */
	return (jj);
}


/********************************/
char * StrPurge(char * text)
/********************************/
/* Supprime les caracteres Space en debut de la ligne text
	retourne un pointeur sur le 1er caractere non Space de text
*/
{
char * ptspace;
	if ( text == NULL ) return NULL;

	while( (*text <= ' ') && *text ) text++;
	ptspace = text + strlen(text) -1;
	while( (*ptspace <= ' ') && *ptspace && (ptspace >= text) )
		{
		*ptspace = 0; ptspace--;
		}
	return(text);
}


/*****************************************************************/
char * GetLine(FILE *File, char *Line, int *LineNum, int SizeLine)
/*****************************************************************/
/* Routine de lecture de 1 ligne utile
	retourne la 1ere ligne utile lue.
	elimine lignes vides et commentaires
	incremente *LineNum a chaque ligne lue
*/
{
	do  {
		if (fgets(Line, SizeLine, File) == NULL) return NULL;
		if( LineNum ) *LineNum += 1;
		} while (Line[0] == '#' || Line[0] == '\n' ||  Line[0] == '\r' ||
				 Line[0] == 0);

	strtok(Line,"\n\r");
	return Line;
}


/*******************************/
char * DateAndTime(char * Line)
/*******************************/
/* Retourne la chaine de caractere donnant date+heure */
{
time_t Time_Buf;
struct tm * Date;

	time(&Time_Buf);
	Date = gmtime(&Time_Buf);
	sprintf(Line,"%d/%d/%d-%2.2d:%2.2d:%2.2d",
			Date->tm_mday, Date->tm_mon + 1, Date->tm_year + 1900,
			Date->tm_hour, Date->tm_min, Date->tm_sec );

	return(Line);
}

/*******************************/
wxString DateAndTime(void)
/*******************************/
/* Retourne la chaine de caractere donnant date+heure */
{
time_t Time_Buf;
struct tm * Date;
wxString Line;

	time(&Time_Buf);
	Date = gmtime(&Time_Buf);
	Line.Printf( wxT("%d/%d/%d-%2.2d:%2.2d:%2.2d"),
			Date->tm_mday, Date->tm_mon + 1, Date->tm_year + 1900,
			Date->tm_hour, Date->tm_min, Date->tm_sec );

	return(Line);
}


/************************************************************/
int StrLenNumCmp(const wxChar *str1,const wxChar *str2, int NbMax)
/************************************************************/
/*
routine (compatible qsort() ) de comparaison pour classement alphab‚tique
Analogue a strncmp() mais les nombres sont compar‚s selon leur valeur num‚rique
et non pas par leur code ascii
*/
{
int i;
int nb1 = 0 , nb2 = 0;

	if( (str1 == NULL) || (str2 == NULL) ) return(0);

	for ( i = 0 ; i < NbMax ; i++ )
		{
		if (isdigit(*str1) && isdigit(*str2) ) /* nombres en jeu */
			{
			nb1 = 0 ; nb2 = 0 ;
			while (isdigit(*str1))
				{
				nb1 = nb1*10 + *str1 -'0'; str1++;
				}
			while (isdigit(*str2))
				{
				nb2 = nb2*10 + *str2 -'0'; str2++;
				}
			if ( nb1 < nb2 ) return(-1) ;
			if ( nb1 > nb2 ) return(1) ;
			}

		if( *str1 < *str2 ) return(-1) ;
		if( *str1 > *str2 ) return(1) ;
		if( (*str1 == 0 ) && ( *str2 == 0 ) ) return(0) ;
		str1++ ; str2++ ;
		}

	return(0);
}


/***********************************************/
int StrNumICmp(const wxChar *str1,const wxChar *str2)
/***********************************************/
/*
routine (compatible qsort() ) de comparaison pour classement alphabétique,
avec lower case == upper case.
Analogue a stricmp() mais les nombres sont comparés selon leur valeur numérique
et non pas par leur code ascii
*/
{
	return StrLenNumICmp( str1, str2, 32735);
}


/**************************************************************/
int StrLenNumICmp(const wxChar *str1,const wxChar *str2, int NbMax)
/**************************************************************/
/*
routine (compatible qsort() ) de comparaison pour classement alphabetique,
avec lower case == upper case.
Analogue a stricmp() mais les nombres sont compares selon leur valeur numerique
et non pas par leur code ascii
*/
{
int i;
int nb1 = 0 , nb2 = 0;

	if( (str1 == NULL) || (str2 == NULL) ) return(0);

	for ( i = 0 ; i < NbMax ; i++ )
		{
		if (isdigit(*str1) && isdigit(*str2) ) /* nombres en jeu */
			{
			nb1 = 0 ; nb2 = 0 ;
			while (isdigit(*str1))
				{
				nb1 = nb1*10 + *str1 -'0'; str1++;
				}
			while (isdigit(*str2))
				{
				nb2 = nb2*10 + *str2 -'0'; str2++;
				}
			if ( nb1 < nb2 ) return(-1) ;
			if ( nb1 > nb2 ) return(1) ;
			}

		if( toupper(*str1) < toupper(*str2) ) return(-1) ;
		if( toupper(*str1) > toupper(*str2) ) return(1) ;
		if( (*str1 == 0 ) && ( *str2 == 0 ) ) return(0) ;
		str1++ ; str2++ ;
		}

	return(0);
}


/***********************************************************************/
bool WildCompareString(const wxString & pattern, const wxString & string_to_tst,
			bool case_sensitive )
/***********************************************************************/
/* compare 2 noms de composants, selon regles usuelles
	( Jokers * , ? , autorisés).
	la chaine de reference est "pattern"
	si case_sensitive == TRUE, comparaison exacte
	retourne TRUE si match
	retourne FALSE si differences
*/
{
const wxChar *cp = NULL, *mp = NULL;
const wxChar * wild, * string;
wxString _pattern, _string_to_tst;

	if ( case_sensitive )
	{
		wild = pattern.GetData(); string = string_to_tst.GetData();
	}
	else
	{
		_pattern = pattern; _pattern.MakeUpper();
		_string_to_tst = string_to_tst; _string_to_tst.MakeUpper();
		wild = _pattern.GetData(); string = _string_to_tst.GetData();
	}

	while ( (*string) && (*wild != '*') )
	{
		if ( (*wild != *string) && (*wild != '?') ) return FALSE;
		wild++; string++;
	}

	while (*string)
	{
		if (*wild == '*')
		{
			if (!*++wild) return 1;
			mp = wild;
			cp = string+1;
		}
		else if ((*wild == *string) || (*wild == '?'))
		{
			wild++;
			string++;
		}
		else
		{
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*') {
		wild++;
	}
	return !*wild;
}

/***********************************************/
void ChangeSpaces(char * Text, int NewChar)
/***********************************************/
/* Change dans un texte les espaces en NewChar */
{
	if ( Text == NULL ) return;
	while( *Text )
		{
		if( *Text == ' ') *Text = (char) NewChar;
		Text++;
		}
}



/***************************/
char * to_point(char * Text)
/**************************/
/* convertit les , en . dans une chaine. utilisé pour compenser
l'internalisation imbecile de la fct printf
qui genere les flottants avec une virgule au lieu du point
*/
{
char * line = Text;

	if ( Text == NULL ) return NULL;
	for ( ; *Text != 0; Text++ )
		if (*Text == g_FloatSeparator) *Text = '.';

	return line;
}

/****************************/
char * from_point(char * Text)
/****************************/
/* convertit les . en , dans une chaine. utilisé pour compenser
l'internalisation imbecile de la fct scanf
qui lit les flottants avec une virgule au lieu du point
*/
{
char * line = Text;
	if ( Text == NULL ) return NULL;

	for ( ; *Text != 0; Text++ )
		if (*Text == '.') *Text = g_FloatSeparator;

	return line;
}

/********************************/
char * strupper(char * Text)
/********************************/
/* Change les caracteres 'a' ... 'z' en 'A' ... 'Z'. dans la chaine Text.
Retourne Text
*/
{
char * code = Text;

	if( Text )
		{
		while( * code)
			{
			if( (*code >= 'a') && (*code <= 'z') ) *code += 'A' - 'a';
			code ++;
			}
		}

	return(Text);
}

