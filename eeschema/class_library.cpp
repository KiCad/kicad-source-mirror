/**********************************************************/
/*	libclass.cpp										  */
/**********************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "kicad_string.h"
#include "confirm.h"
#include "gestfich.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"
#include "class_library.h"

#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/listimpl.cpp>

WX_DEFINE_LIST( LIB_CMP_LIST );


LibraryStruct::LibraryStruct( int type, const wxString& name,
                              const wxString& fullname )
{
    m_Type = type;              /* type indicator */
    m_Name = name;              /* Name of library loaded. */
    m_FullFileName = fullname;  /* File name (with path) of library loaded. */
    m_NumOfParts   = 0;         /* Number of parts this library has. */
    m_Entries   = NULL;         /* Parts themselves are saved here. */
    m_Pnext     = NULL;         /* Point to next library in chain. */
    m_Modified  = FALSE;        /* flag indicateur d'edition */
    m_TimeStamp = 0;
    m_Flags      = 0;
    m_IsLibCache = FALSE;
    m_DateTime   = wxDateTime::Now();
}


LibraryStruct::LibraryStruct( const wxChar* fileName )
{
    if( fileName == NULL )
        m_fileName = wxT( "unnamed.lib" );
    else
        m_fileName = fileName;
}


/*****************************************/
/* Used by PQFreeFunc() to delete all entries
 */
/*****************************************/
void FreeLibraryEntry( LibCmpEntry* Entry )
{
    SAFE_DELETE( Entry );
}


/******************************/
LibraryStruct::~LibraryStruct()
{
    if( m_Entries )
        PQFreeFunc( m_Entries, ( void( * ) ( void* ) )FreeLibraryEntry );

    m_componentList.DeleteContents( true );
}


/*******************************************/
/* Ecrit l'entete du fichier librairie
 */
/*******************************************/
bool LibraryStruct::WriteHeader( FILE* file )
{
    char BufLine[1024];
    bool succes = TRUE;

    DateAndTime( BufLine );
    if( fprintf( file, "%s %d.%d  Date: %s\n", LIBFILE_IDENT,
                 LIB_VERSION_MAJOR, LIB_VERSION_MINOR, BufLine ) != 5 )
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
/* Ecrit l'entete du fichier librairie
 */
/***********************************************************/
bool LibraryStruct::ReadHeader( FILE* libfile, int* LineNum )
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


/**
 * Function SaveLibrary
 * writes the data structures for this object out to 2 file
 * the library in "*.lib" format.
 * the doc file in "*.dcm" format.
 * creates a backup file for each  file (.bak and .bck)
 * @param aFullFileName The full lib filename.
 * @return bool - true if success writing else false.
 */
bool LibraryStruct::SaveLibrary( const wxString& FullFileName )
{
    FILE* libfile, *docfile;
    EDA_LibComponentStruct* LibEntry;
    wxString msg;
    wxFileName libFileName = FullFileName;
    wxFileName backupFileName = FullFileName;
    wxFileName docFileName = FullFileName;

    /* the old .lib file is renamed .bak */
    if( libFileName.FileExists() )
    {
        backupFileName.SetExt( wxT( "bak" ) );
        wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxRenameFile( libFileName.GetFullPath(),
                           backupFileName.GetFullPath() ) )
        {
            msg = wxT( "Failed to rename old lib file " ) +
                backupFileName.GetFullPath();
            DisplayError( NULL, msg, 20 );
        }
    }

    docFileName.SetExt( DOC_EXT );
    /* L'ancien fichier doc lib est renomme en .bck */
    if( wxFileExists( docFileName.GetFullPath() ) )
    {
        backupFileName = docFileName;
        backupFileName.SetExt( wxT( "bck" ) );
        wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxRenameFile( docFileName.GetFullPath(),
                           backupFileName.GetFullPath() ) )
        {
            msg = wxT( "Failed to save old doc lib file " ) +
                backupFileName.GetFullPath();
            DisplayError( NULL, msg, 20 );
        }
    }


    libfile = wxFopen( libFileName.GetFullPath(), wxT( "wt" ) );

    if( libfile == NULL )
    {
        msg = wxT( "Failed to create Lib File " ) + libFileName.GetFullPath();
        DisplayError( NULL, msg, 20 );
        return false;
    }

    docfile = wxFopen( docFileName.GetFullPath(), wxT( "wt" ) );

    if( docfile == NULL )
    {
        msg = wxT( "Failed to create DocLib File " ) +
            docFileName.GetFullPath();
        DisplayError( NULL, msg, 20 );
    }

    m_Modified = 0;

    /* Creation de l'entete de la librairie */
    m_TimeStamp = GetTimeStamp();
    WriteHeader( libfile );

    /* Sauvegarde des composant: */
    PQCompFunc( (PQCompFuncType) LibraryEntryCompare );
    LibEntry = (EDA_LibComponentStruct*) PQFirst( &m_Entries, FALSE );
    char Line[256];
    fprintf( docfile, "%s  Date: %s\n", DOCFILE_IDENT,
             DateAndTime( Line ) );

    bool success = true;
    while( LibEntry )
    {
        if ( LibEntry->Type == ROOT )
        {
            if ( ! LibEntry->Save( libfile ) )
                success = false;
        }
        if ( docfile )
        {
            if ( ! LibEntry->SaveDoc( docfile ) )
                success = false;
        }

        LibEntry = (EDA_LibComponentStruct*) PQNext( m_Entries,
                                                     LibEntry, NULL );
    }

    fprintf( libfile, "#\n#End Library\n" );
    if ( docfile )
        fprintf( docfile, "#\n#End Doc Library\n" );
    fclose( libfile );
    fclose( docfile );
    return success;
}


wxString LibraryStruct::GetName()
{
    return m_fileName.GetName();
}
