/**
 * This file is part of the common libary \n
 * Custom string manipulation routines.
 * @file  kicad_string.h
 * @see   common.h, string.cpp
 */


#ifndef __INCLUDE__KICAD_STRING_H__
#define __INCLUDE__KICAD_STRING_H__ 1


char*       strupper( char* Text );
char*       strlower( char* Text );

int         ReadDelimitedText( char* dest,
                               char* source,
                                 int NbMaxChar );

/* lit et place dans dest la chaine de caractere trouvee dans source,
 *   delimitee par " .
 *  transfere NbMaxChar max
 *   retourne le nombre de codes lus dans source
 *  dest est termine par NULL */

char*       GetLine( FILE* File,
                     char* Line,
                      int* LineNum = NULL,
                       int SizeLine = 255 );

/* Routine de lecture de 1 ligne utile
 *  retourne la 1ere ligne utile lue.
 *  elimine lignes vides et commentaires */
char*       StrPurge( char* text );

/* Supprime les caracteres Space en debut de la ligne text
 *  retourne un pointeur sur le 1er caractere non Space de text */

char*       DateAndTime( char* line );
wxString    DateAndTime();

/* Retourne la chaine de caractere donnant date+heure */

int         StrLenNumCmp( const wxChar* str1,
                          const wxChar* str2,
                            int NbMax );

/*
 *  routine (compatible qsort() ) de comparaision pour classement alphab�tique
 *  Analogue a strncmp() mais les nombres sont compar�s selon leur valeur num�rique
 *  et non pas par leur code ascii */

int         StrNumICmp( const wxChar* str1,
                        const wxChar* str2 );

/* routine (compatible qsort() ) de comparaison pour classement alphab�tique,
 *  avec lower case == upper case.
 *  Analogue a stricmp() mais les nombres sont compar�s selon leur valeur num�rique
 *  et non pas par leur code ascii */

int         StrLenNumICmp( const wxChar* str1,
                           const wxChar* str2,
                                     int NbMax );

/* routine (compatible qsort() ) de comparaison pour classement alphab�tique,
 *  avec lower case == upper case.
 *  Analogue a stricmp() mais les nombres sont compar�s selon leur valeur num�rique
 *  et non pas par leur code ascii */

bool        WildCompareString( const wxString& pattern,
                               const wxString& string_to_tst,
                                          bool case_sensitive = TRUE );

/* compare 2 noms de composants, selon regles usuelles
 *  ( Jokers * , ? , autorises).
 *  la chaine de reference est "pattern"
 *  si case_sensitive == TRUE (default), comparaison exacte
 *  retourne TRUE si match FALSE si differences */

char*       to_point( char* Text );

/* convertit les , en . dans une chaine. utilise pour compenser la fct printf
 *  qui genere les flottants avec une virgule au lieu du point en mode international */


#endif /* __INCLUDE__KICAD_STRING_H__ */

