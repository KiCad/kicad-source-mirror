/**********************************************************/
/*  libclass.cpp                                          */
/**********************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "kicad_string.h"
#include "confirm.h"
#include "gestfich.h"
#include "eda_doc.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"
#include "class_library.h"


void FreeLibraryEntry( LibCmpEntry* Entry )
{
    SAFE_DELETE( Entry );
}


bool operator==( const LibraryStruct& lib, const wxChar* name )
{
    return lib.m_Name.CmpNoCase( name ) == 0;
}


bool operator!=( const LibraryStruct& lib, const wxChar* name )
{
    return !( lib == name );
}


LibraryStruct::LibraryStruct( int type, const wxString& name,
                              const wxString& fullname )
{
    m_Type = type;              /* type indicator */
    m_Name = name;              /* Name of library loaded. */
    m_FullFileName = fullname;  /* File name (with path) of library loaded. */
    m_NumOfParts   = 0;         /* Number of parts this library has. */
    m_Pnext        = NULL;      /* Point to next library in chain. */
    m_Entries      = NULL;
    m_Modified     = FALSE;     /* flag indicateur d'edition */
    m_TimeStamp    = 0;
    m_Flags        = 0;
    m_IsLibCache   = FALSE;
    m_DateTime     = wxDateTime::Now();
}


LibraryStruct::LibraryStruct( const wxChar* fileName )
{
    if( fileName == NULL )
        m_fileName = wxT( "unnamed.lib" );
    else
        m_fileName = fileName;
}


LibraryStruct::~LibraryStruct()
{
    if( m_Entries )
        PQFreeFunc( m_Entries, ( void( * ) ( void* ) )FreeLibraryEntry );
}


void LibraryStruct::GetEntryNames( wxArrayString& names, bool sort )
{
    LibCmpEntry* entry = ( LibCmpEntry* ) PQFirst( &m_Entries, false );

    while( entry != NULL )
    {
        names.Add( entry->m_Name.m_Text );
        entry = ( LibCmpEntry* ) PQNext( m_Entries, entry, NULL );
    }

    if( sort )
        names.Sort();
}


void LibraryStruct::SearchEntryNames( wxArrayString& names,
                                      const wxString& nameSearch,
                                      const wxString& keySearch,
                                      bool sort )
{
    LibCmpEntry*  Entry;

    Entry = (LibCmpEntry*) PQFirst( &m_Entries, false );

    while( Entry )
    {
        if( !keySearch.IsEmpty() && KeyWordOk( keySearch, Entry->m_KeyWord ) )
            names.Add( Entry->m_Name.m_Text );
        if( !nameSearch.IsEmpty() && WildCompareString( nameSearch,
                                                        Entry->m_Name.m_Text,
                                                        false ) )
            names.Add( Entry->m_Name.m_Text );

        Entry = (LibCmpEntry*) PQNext( m_Entries, Entry, NULL );
    }
}


LibCmpEntry* LibraryStruct::FindEntry( const wxChar* name )
{
    static LibCmpEntry tmp( ALIAS, wxEmptyString );
    tmp.m_Name.m_Text = name;

    PQCompFunc( ( PQCompFuncType ) LibraryEntryCompare );

    return ( LibCmpEntry* ) PQFind( m_Entries, &tmp );
}


LibCmpEntry* LibraryStruct::FindEntry( const wxChar* name, LibrEntryType type )
{
    LibCmpEntry* entry = FindEntry( name );

    if( entry != NULL && entry->Type != ROOT && type == ROOT )
    {
        EDA_LibCmpAliasStruct* alias = ( EDA_LibCmpAliasStruct* ) entry;
        entry = FindEntry( alias->m_RootName );
    }

    return entry;
}


EDA_LibComponentStruct* LibraryStruct::AddComponent( EDA_LibComponentStruct* cmp )
{
    wxASSERT( cmp != NULL );

    EDA_LibCmpAliasStruct*  Alias;
    EDA_LibComponentStruct* newCmp = CopyLibEntryStruct( cmp );

    if( newCmp == NULL )
        return NULL;

    newCmp->m_AliasList.Clear();
    PQCompFunc( (PQCompFuncType) LibraryEntryCompare );
    PQInsert( &m_Entries, (void*) newCmp );
    m_NumOfParts++;
    m_Modified = 1;

    for( unsigned ii = 0; ii <  cmp->m_AliasList.GetCount(); ii += ALIAS_NEXT )
    {
        wxString aliasname = cmp->m_AliasList[ii + ALIAS_NAME];
        newCmp->m_AliasList.Add( aliasname );
        Alias = new EDA_LibCmpAliasStruct( aliasname, newCmp->m_Name.m_Text );
        Alias->m_Doc     = cmp->m_AliasList[ii + ALIAS_DOC];
        Alias->m_KeyWord = cmp->m_AliasList[ii + ALIAS_KEYWORD];
        Alias->m_DocFile = cmp->m_AliasList[ii + ALIAS_DOC_FILENAME];

        PQInsert( &m_Entries, (void*) Alias );
        m_NumOfParts++;
    }

    return newCmp;
}


void LibraryStruct::RemoveEntry( LibCmpEntry* entry )
{
    wxASSERT( entry != NULL );

    EDA_LibComponentStruct* Root;
    EDA_LibCmpAliasStruct*  Alias;

    PQCompFunc( (PQCompFuncType) LibraryEntryCompare );
    m_Modified = 1;

    if( entry->Type == ALIAS )
    {
        Alias = (EDA_LibCmpAliasStruct*) entry;
        Root = ( EDA_LibComponentStruct* ) FindEntry( Alias->m_RootName, ROOT );

        /* Remove alias name from the root component alias list */
        if( Root == NULL )
        {
            wxLogWarning( wxT( "No root component found for alias <%s> in " \
                               "library <%s>." ),
                          ( const wxChar* ) entry->m_Name.m_Text,
                          ( const wxChar* ) m_Name );
        }
        else
        {
            int index = Root->m_AliasList.Index( entry->m_Name.m_Text, false );

            if( index == wxNOT_FOUND )
                wxLogWarning( wxT( "Alias <%s> not found in component <%s> \
alias list in library <%s>" ),
                              ( const wxChar* ) entry->m_Name.m_Text,
                              ( const wxChar* ) Root->m_Name.m_Text,
                              ( const wxChar* ) m_Name );
            else
                Root->m_AliasList.RemoveAt( index );
        }

        /* Effacement memoire pour cet alias */
        PQDelete( &m_Entries, (void*) Alias );
        SAFE_DELETE( Alias );
        if( m_NumOfParts > 0 )
            m_NumOfParts--;

        return;
    }

    Root = ( EDA_LibComponentStruct* ) entry;

    /* Entry is a component with no aliases so removal is simple */
    if( Root->m_AliasList.GetCount() == 0 )
    {
        PQDelete( &m_Entries, (void*) Root );
        SAFE_DELETE( Root );
        if( m_NumOfParts > 0 )
            m_NumOfParts--;
        return;
    }

    /* Entry is a component with alias
     *  We must change the first alias to a "root" component, and for all the
     *  aliases we must change the root component (which is deleted) by the
     * first alias */
    wxString AliasName = Root->m_AliasList[0];

    /* The root component is not really deleted, it is renamed with the first
     * alias name */
    Alias = (EDA_LibCmpAliasStruct*) FindEntry( AliasName, ALIAS );

    if( Alias == NULL || Alias->Type == ROOT )
    {
        wxLogWarning( wxT( "Alias <%s> for component <%s> not found in \
library <%s>" ),
                      ( const wxChar* ) AliasName,
                      ( const wxChar* ) Root->m_Name.m_Text,
                      ( const wxChar* ) m_Name );
    }
    else
    {
        if( m_NumOfParts > 0 )
            m_NumOfParts--;

        /* remove the root component from library */
        PQDelete( &m_Entries, Root );

        /* remove the first alias from library*/
        PQDelete( &m_Entries, Alias );

        /* remove the first alias name from alias list: */
        Root->m_AliasList.RemoveAt( 0 );
        /* change the old name. New name for "root" is the name of the first
         * alias */
        entry->m_Name.m_Text = AliasName;
        entry->m_Doc = Alias->m_Doc;
        entry->m_KeyWord = Alias->m_KeyWord;

        FreeLibraryEntry( ( LibCmpEntry* ) Alias );

        /* root component (renamed) placed in library */
        PQInsert( &m_Entries, entry );
    }

    /* Change the "RootName", for other aliases */
    for( unsigned ii = 0; ii < Root->m_AliasList.GetCount(); ii++ )
    {
        AliasName = Root->m_AliasList[ii];
        Alias = (EDA_LibCmpAliasStruct*) FindEntry( AliasName, ALIAS );

        if( Alias == NULL )
        {
            // Should not occurs. If happens, this is an error (or bug)
            wxLogWarning( wxT( "Alias <%s> for component <%s> not found in \
library <%s>." ),
                          ( const wxChar* ) AliasName,
                          ( const wxChar* ) Root->m_Name.m_Text,
                          ( const wxChar* ) m_Name );
            continue;
        }
        if( Alias->Type != ALIAS )
        {
            // Should not occurs. If happens, this is an error (or bug)
            wxLogWarning( wxT( "Entry <%s> for component <%s> in library \
<%s> is not an alias." ),
                          ( const wxChar* ) AliasName,
                          ( const wxChar* ) Root->m_Name.m_Text,
                          ( const wxChar* ) m_Name );
            continue;
        }

        Alias->m_RootName = entry->m_Name.m_Text;
    }
}


LibCmpEntry* LibraryStruct::GetNextEntry( const wxChar* name )
{
    PQCompFunc( (PQCompFuncType) LibraryEntryCompare );

    LibCmpEntry* entry = (LibCmpEntry*) PQFirst( &m_Entries, false );

    while( entry )
    {
        if( entry->m_Name.m_Text.CmpNoCase( name ) == 0 )
        {
            entry = (LibCmpEntry*) PQNext( m_Entries, entry, NULL );
            break;
        }

        entry = (LibCmpEntry*) PQNext( m_Entries, entry, NULL );
    }

    if( entry == NULL )
        entry = (LibCmpEntry*) PQFirst( &m_Entries, false );

    return entry;
}


LibCmpEntry* LibraryStruct::GetPreviousEntry( const wxChar* name )
{
    LibCmpEntry* previousEntry = NULL;

    PQCompFunc( (PQCompFuncType) LibraryEntryCompare );

    LibCmpEntry* entry = (LibCmpEntry*) PQFirst( &m_Entries, false );

    while( entry )
    {
        if( entry->m_Name.m_Text.CmpNoCase( name ) == 0 )
        {
            if( previousEntry )
                break;
        }

        previousEntry = entry;
        entry = (LibCmpEntry*) PQNext( m_Entries, entry, NULL );
    }

    return previousEntry;
}


bool LibraryStruct::WriteHeader( FILE* file )
{
    char BufLine[1024];
    bool succes = false;

    DateAndTime( BufLine );
    if( fprintf( file, "%s %d.%d  Date: %s\n", LIBFILE_IDENT,
                 LIB_VERSION_MAJOR, LIB_VERSION_MINOR, BufLine ) < 0 )
        succes = false;
#if 0
    if( ( fprintf( file, "$HEADER\n" ) < 0 )
        || ( fprintf( file, "TimeStamp %8.8lX\n", m_TimeStamp ) < 0 )
        || ( fprintf( file, "Parts %d\n", m_NumOfParts ) != 2 )
        || ( fprintf( file, "$ENDHEADER\n" ) != 1 ) )
        succes = false;
#endif
    return succes;
}


bool LibraryStruct::Load( wxString& errMsg )
{
    FILE*                   f;
    int                     LineNum = 0;
    char                    Line[1024];
    EDA_LibComponentStruct* LibEntry;
    wxString                msg;

    if( m_FullFileName.IsEmpty() )
    {
        errMsg = _( "library file name not set" );
        return false;
    }

    f = wxFopen( m_FullFileName, wxT( "rt" ) );

    if( f == NULL )
    {
        errMsg = _( "could not open file" );
        return false;
    }

    m_NumOfParts = 0;

    if( GetLine( f, Line, &LineNum, sizeof( Line ) ) == NULL )
    {
        errMsg = _( "file is empty!" );
        return false;
    }

    if( strnicmp( Line, LIBFILE_IDENT, 10 ) != 0 )
    {
        errMsg = _( "file is NOT an EESCHEMA library!" );
        return false;
    }

    PQInit( &m_Entries );
    PQCompFunc( (PQCompFuncType) LibraryEntryCompare );

    /* There is no header if this is a symbol library. */
    if( m_Type == LIBRARY_TYPE_EESCHEMA )
        m_Header = CONV_FROM_UTF8( Line );

    while( GetLine( f, Line, &LineNum, sizeof( Line ) ) )
    {
        if( m_Type == LIBRARY_TYPE_EESCHEMA
            && strnicmp( Line, "$HEADER", 7 ) == 0 )
        {
            if( !ReadHeader( f, &LineNum ) )
            {
                errMsg = _( "header read error" );
                return false;
            }

            continue;
        }

        if( strnicmp( Line, "DEF", 3 ) == 0 )
        {
            /* Read one DEF/ENDDEF part entry from library: */
            LibEntry = new EDA_LibComponentStruct( NULL );

            if( LibEntry->Load( f, Line, &LineNum, msg ) )
            {
                /* If we are here, this part is O.k. - put it in: */
                m_NumOfParts += 1;
                PQInsert( &m_Entries, LibEntry );
                InsertAliases( &m_Entries, LibEntry );
            }
            else
            {
                wxLogWarning( wxT( "Library <%s> component load error %s." ),
                              (const wxChar*) m_Name,
                              (const wxChar*) msg );
                msg.Clear();
                delete LibEntry;
            }
        }
    }

    return true;
}


void LibraryStruct::InsertAliases( PriorQue** PQ,
                                   EDA_LibComponentStruct* component )
{
    wxASSERT( component != NULL && PQ != NULL );

    EDA_LibCmpAliasStruct* AliasEntry;
    unsigned               ii;

    for( ii = 0; ii < component->m_AliasList.GetCount(); ii++ )
    {
        AliasEntry =
            new EDA_LibCmpAliasStruct( component->m_AliasList[ii],
                                       component->m_Name.m_Text );
        PQInsert( PQ, AliasEntry );
        m_NumOfParts += 1;
    }
}


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


bool LibraryStruct::SaveLibrary( const wxString& FullFileName )
{
    FILE* libfile, *docfile;
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
            DisplayError( NULL, msg );
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
            DisplayError( NULL, msg );
        }
    }


    libfile = wxFopen( libFileName.GetFullPath(), wxT( "wt" ) );

    if( libfile == NULL )
    {
        msg = wxT( "Failed to create Lib File " ) + libFileName.GetFullPath();
        DisplayError( NULL, msg );
        return false;
    }

    docfile = wxFopen( docFileName.GetFullPath(), wxT( "wt" ) );

    if( docfile == NULL )
    {
        msg = wxT( "Failed to create DocLib File " ) +
            docFileName.GetFullPath();
        DisplayError( NULL, msg );
    }

    m_Modified = 0;

    /* Creation de l'entete de la librairie */
    m_TimeStamp = GetTimeStamp();
    WriteHeader( libfile );

    /* Sauvegarde des composant: */
    char Line[256];
    fprintf( docfile, "%s  Date: %s\n", DOCFILE_IDENT,
             DateAndTime( Line ) );

    bool success = true;

    LibCmpEntry* entry = ( LibCmpEntry* ) PQFirst( &m_Entries, false );

    while( entry != NULL )
    {
        if ( entry->Type == ROOT )
        {
            EDA_LibComponentStruct* component =
                ( EDA_LibComponentStruct* ) entry;
            if ( ! component->Save( libfile ) )
                success = false;
        }
        if ( docfile )
        {
            if ( ! entry->SaveDoc( docfile ) )
                success = false;
        }

        entry = ( LibCmpEntry* ) PQNext( m_Entries, entry, NULL );
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
    return m_Name;
}
