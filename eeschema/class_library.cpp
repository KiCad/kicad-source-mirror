/**********************************************************/
/*	libclass.cpp										  */
/**********************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

/***************************************************************************************/
LibraryStruct::LibraryStruct( int type, const wxString& name, const wxString& fullname )
/***************************************************************************************/
{
    m_Type = type;                  /* type indicator */
    m_Name = name;                  /* Name of library loaded. */
    m_FullFileName = fullname;      /* Full File Name (with path) of library loaded. */
    m_NumOfParts   = 0;             /* Number of parts this library has. */
    m_Entries   = NULL;             /* Parts themselves are saved here. */
    m_Pnext     = NULL;             /* Point on next lib in chain. */
    m_Modified  = FALSE;            /* flag indicateur d'edition */
    m_TimeStamp = 0;
    m_Flags      = 0;
    m_IsLibCache = FALSE;
}


/*****************************************/
void FreeLibraryEntry( LibCmpEntry* Entry )
/*****************************************/

/* Used by PQFreeFunc() to delete all entries
 */
{
    SAFE_DELETE( Entry );
}


/******************************/
LibraryStruct::~LibraryStruct()
/******************************/
{
    if( m_Entries )
        PQFreeFunc( m_Entries, ( void( * ) ( void* ) )FreeLibraryEntry );
}


/*******************************************/
bool LibraryStruct::WriteHeader( FILE* file )
/*******************************************/

/* Ecrit l'entete du fichier librairie
 */
{
    char BufLine[1024];
    bool succes = TRUE;

    DateAndTime( BufLine );
    if( fprintf( file, "%s %d.%d  Date: %s\n", LIBFILE_IDENT,
                 LIB_VERSION_MAJOR, LIB_VERSION_MINOR,
                 BufLine ) != 5 )
        succes = FALSE;
#if 0
    if( fprintf( file, "$HEADER\n" ) != 1 )
        succes = FALSE;
    if( fprintf( file, "TimeStamp %8.8lX\n", m_TimeStamp ) != 2 )
        succes = FALSE;
    if( fprintf( file, "Parts %d\n", m_NumOfParts ) != 2 )
        succes = FALSE;
    if( fprintf( file, "$ENDHEADER\n" ) != 1 )
        succes = FALSE;
#endif
    return succes;
}


/***********************************************************/
bool LibraryStruct::ReadHeader( FILE* libfile, int* LineNum )
/***********************************************************/

/* Ecrit l'entete du fichier librairie
 */
{
    char Line[1024], * text, * data;

    while( GetLine( libfile, Line, LineNum, sizeof(Line) ) )
    {
        text = strtok( Line, " \t\r\n" );
        data = strtok( NULL, " \t\r\n" );
        if( stricmp( text, "TimeStamp" ) == 0 )
            m_TimeStamp = atol( data );
        if( stricmp( text, "$ENDHEADER" ) == 0 )
            return TRUE;
    }

    return FALSE;
}

