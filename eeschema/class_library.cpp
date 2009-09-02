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

#include <boost/foreach.hpp>


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
    m_Type         = type;      /* type indicator */
    m_Name         = name;      /* Name of library loaded. */
    m_FullFileName = fullname;  /* File name (with path) of library loaded. */
    m_Pnext        = NULL;      /* Point to next library in chain. */
    m_IsModified   = false;     /* flag indicateur d'edition */
    m_TimeStamp    = 0;
    m_Flags        = 0;
    m_IsLibCache   = false;
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
}


void LibraryStruct::GetEntryNames( wxArrayString& names, bool sort,
                                   bool makeUpperCase )
{
    BOOST_FOREACH( LibCmpEntry& entry, m_Entries )
    {
        if( makeUpperCase )
            names.Add( entry.m_Name.m_Text.MakeUpper() );
        else
            names.Add( entry.m_Name.m_Text );
    }

    if( sort )
        names.Sort();
}


void LibraryStruct::SearchEntryNames( wxArrayString& names,
                                      const wxString& nameSearch,
                                      const wxString& keySearch,
                                      bool sort )
{
    BOOST_FOREACH( LibCmpEntry& entry, m_Entries )
    {
        if( !keySearch.IsEmpty() && KeyWordOk( keySearch, entry.m_KeyWord ) )
            names.Add( entry.m_Name.m_Text );
        if( !nameSearch.IsEmpty() && WildCompareString( nameSearch,
                                                        entry.m_Name.m_Text,
                                                        false ) )
            names.Add( entry.m_Name.m_Text );
    }

    if( sort )
        names.Sort();
}


LibCmpEntry* LibraryStruct::FindEntry( const wxChar* name )
{
    BOOST_FOREACH( LibCmpEntry& entry, m_Entries )
    {
        if( entry.m_Name.m_Text.CmpNoCase( name ) == 0 )
            return &entry;
    }

    return NULL;
}


LibCmpEntry* LibraryStruct::FindEntry( const wxChar* name, LibrEntryType type )
{
    LibCmpEntry* entry = FindEntry( name );

    if( entry && entry->Type != ROOT && type == ROOT )
    {
        EDA_LibCmpAliasStruct* alias = ( EDA_LibCmpAliasStruct* ) entry;

        entry = FindEntry( alias->m_RootName );

        if( entry && entry->Type != ROOT )
            return NULL;
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
    m_Entries.push_back( (LibCmpEntry*) newCmp );
    m_IsModified = true;

    for( unsigned ii = 0; ii < cmp->m_AliasList.GetCount(); ii += ALIAS_NEXT )
    {
        wxString aliasname = cmp->m_AliasList[ii + ALIAS_NAME];
        newCmp->m_AliasList.Add( aliasname );
        Alias = new EDA_LibCmpAliasStruct( aliasname, newCmp->m_Name.m_Text );
        Alias->m_Doc     = cmp->m_AliasList[ii + ALIAS_DOC];
        Alias->m_KeyWord = cmp->m_AliasList[ii + ALIAS_KEYWORD];
        Alias->m_DocFile = cmp->m_AliasList[ii + ALIAS_DOC_FILENAME];
        m_Entries.push_back( (LibCmpEntry *) Alias );
    }

    m_Entries.sort();

    return newCmp;
}


void LibraryStruct::RemoveEntry( const wxString& name )
{
    LIB_ENTRY_LIST::iterator i;

    for( i = m_Entries.begin(); i < m_Entries.end(); i++ )
    {
        if( i->m_Name.m_Text.CmpNoCase( name ) == 0 )
        {
            m_Entries.erase( i );
            return;
        }
    }
}


void LibraryStruct::RemoveEntry( LibCmpEntry* entry )
{
    wxASSERT( entry != NULL );

    EDA_LibComponentStruct* Root;
    EDA_LibCmpAliasStruct*  Alias;

    m_IsModified = true;

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

        RemoveEntry( Alias->m_Name.m_Text );

        return;
    }

    Root = ( EDA_LibComponentStruct* ) entry;

    /* Entry is a component with no aliases so removal is simple. */
    if( Root->m_AliasList.GetCount() == 0 )
    {
        RemoveEntry( Root->m_Name.m_Text );
        return;
    }

    /* Entry is a component with one or more alias. */
    wxString AliasName = Root->m_AliasList[0];

    /* The root component is not really deleted, it is renamed with the first
     * alias name. */
    Alias = (EDA_LibCmpAliasStruct*) FindEntry( AliasName, ALIAS );

    if( Alias == NULL || Alias->Type == ROOT )
    {
        wxLogWarning( wxT( "Alias <%s> for component <%s> not found in \
library <%s>" ),
                      ( const wxChar* ) AliasName,
                      ( const wxChar* ) Root->m_Name.m_Text,
                      ( const wxChar* ) m_Name );
        return;
    }

    /* Remove the first alias name from the component alias list. */
    Root->m_AliasList.RemoveAt( 0 );

    /* Rename the component to the name of the first alias. */
    Root->m_Doc = Alias->m_Doc;
    Root->m_KeyWord = Alias->m_KeyWord;

    /* Remove the first alias from library. */
    RemoveEntry( AliasName );

    Root->m_Name.m_Text = AliasName;

    /* Change the "RootName" for all other aliases */
    for( size_t ii = 0; ii < Root->m_AliasList.GetCount(); ii++ )
    {
        Alias = (EDA_LibCmpAliasStruct*) FindEntry( Root->m_AliasList[ii],
                                                    ALIAS );

        /* Should not occur if library was saved by the library editor.
         * However, it is possible if the library was edited by hand or
         * some other program or a there is a bug in the library editor. */
        if( Alias == NULL ||  Alias->Type != ALIAS )
        {
            wxLogWarning( wxT( "Alias <%s> for component <%s> not found in \
library <%s>." ),
                          ( const wxChar* ) AliasName,
                          ( const wxChar* ) Root->m_Name.m_Text,
                          ( const wxChar* ) m_Name );
            continue;
        }

        Alias->m_RootName = Root->m_Name.m_Text;
    }
}


LibCmpEntry* LibraryStruct::GetNextEntry( const wxChar* name )
{
    size_t i;
    LibCmpEntry* entry = NULL;

    for( i = 0; i < m_Entries.size(); i++ )
    {
        if( m_Entries[i].m_Name.m_Text.CmpNoCase( name ) == 0 )
        {
            if( i < m_Entries.size() - 1 )
            {
                entry = &m_Entries[ i + 1 ];
                break;
            }
        }
    }

    if( entry == NULL )
        entry = &m_Entries.front();

    return entry;
}


LibCmpEntry* LibraryStruct::GetPreviousEntry( const wxChar* name )
{
    size_t i;
    LibCmpEntry* entry = NULL;

    for( i = 0; i < m_Entries.size(); i++ )
    {
        if( m_Entries[i].m_Name.m_Text.CmpNoCase( name ) == 0 && entry )
            break;

        entry = &m_Entries[i];
    }

    return entry;
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

    /* There is no header if this is a symbol library. */
    if( m_Type == LIBRARY_TYPE_EESCHEMA )
        m_Header = CONV_FROM_UTF8( Line );

    while( GetLine( f, Line, &LineNum, sizeof( Line ) ) )
    {
        if( m_Type == LIBRARY_TYPE_EESCHEMA
            && strnicmp( Line, "$HEADER", 7 ) == 0 )
        {
            if( !LoadHeader( f, &LineNum ) )
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
                m_Entries.push_back( LibEntry );
                LoadAliases( LibEntry );
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

    m_Entries.sort();

    return true;
}


void LibraryStruct::LoadAliases( EDA_LibComponentStruct* component )
{
    wxASSERT( component != NULL );

    EDA_LibCmpAliasStruct* AliasEntry;
    unsigned               ii;

    for( ii = 0; ii < component->m_AliasList.GetCount(); ii++ )
    {
        AliasEntry =
            new EDA_LibCmpAliasStruct( component->m_AliasList[ii],
                                       component->m_Name.m_Text );
        m_Entries.push_back( AliasEntry );
    }
}


bool LibraryStruct::LoadHeader( FILE* libfile, int* LineNum )
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


bool LibraryStruct::LoadDocs( wxString& errMsg )
{
    int      LineNum = 0;
    char     Line[1024], * Name, * Text;
    LibCmpEntry* Entry;
    FILE*    f;
    wxString msg;
    wxFileName     fn;

    fn = m_FullFileName;
    fn.SetExt( DOC_EXT );

    f = wxFopen( fn.GetFullPath(), wxT( "rt" ) );

    if( f == NULL )
    {
        errMsg.Printf( _( "Could not open component document libray file\n<%s>." ),
                       (const wxChar*) fn.GetFullPath() );
        return false;
    }

    if( GetLine( f, Line, &LineNum, sizeof(Line) ) == NULL )
    {
        errMsg.Printf( _( "Component document libray file <%s> is empty." ),
                       (const wxChar*) fn.GetFullPath() );
        fclose( f );
        return false;
    }

    if( strnicmp( Line, DOCFILE_IDENT, 10 ) != 0 )
    {
        errMsg.Printf( _( "File <%s> is not a valid component library \
document file." ),
                       (const wxChar*) fn.GetFullPath() );
        fclose( f );
        return false;
    }

    while( GetLine( f, Line, &LineNum, sizeof(Line) ) )
    {
        if( strncmp( Line, "$CMP", 4 ) != 0 )
        {
            errMsg.Printf( wxT( "$CMP command expected in line %d, aborted." ),
                           LineNum );
            fclose( f );
            return false;
        }

        /* Read one $CMP/$ENDCMP part entry from library: */
        Name = strtok( Line + 5, "\n\r" );

        wxString cmpname = CONV_FROM_UTF8( Name );

        Entry = FindEntry( cmpname, ALIAS );

        while( GetLine( f, Line, &LineNum, sizeof(Line) ) )
        {
            if( strncmp( Line, "$ENDCMP", 7 ) == 0 )
                break;
            Text = strtok( Line + 2, "\n\r" );

            switch( Line[0] )
            {
            case 'D':
                if( Entry )
                    Entry->m_Doc = CONV_FROM_UTF8( Text );
                break;

            case 'K':
                if( Entry )
                    Entry->m_KeyWord = CONV_FROM_UTF8( Text );
                break;

            case 'F':
                if( Entry )
                    Entry->m_DocFile = CONV_FROM_UTF8( Text );
                break;
            }
        }
    }

    fclose( f );
    return true;
}


bool LibraryStruct::Save( const wxString& FullFileName )
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

    m_IsModified = false;

    /* Creation de l'entete de la librairie */
    m_TimeStamp = GetTimeStamp();
    SaveHeader( libfile );

    /* Sauvegarde des composant: */
    char Line[256];
    fprintf( docfile, "%s  Date: %s\n", DOCFILE_IDENT,
             DateAndTime( Line ) );

    bool success = true;

    BOOST_FOREACH( LibCmpEntry& entry, m_Entries )
    {
        if ( entry.Type == ROOT )
        {
            EDA_LibComponentStruct* component =
                ( EDA_LibComponentStruct* ) &entry;
            if ( ! component->Save( libfile ) )
                success = false;
        }
        if ( docfile )
        {
            if ( ! entry.SaveDoc( docfile ) )
                success = false;
        }
    }

    fprintf( libfile, "#\n#End Library\n" );
    if ( docfile )
        fprintf( docfile, "#\n#End Doc Library\n" );
    fclose( libfile );
    fclose( docfile );
    return success;
}


bool LibraryStruct::SaveHeader( FILE* file )
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
        || ( fprintf( file, "Parts %d\n", m_Entries.size() ) != 2 )
        || ( fprintf( file, "$ENDHEADER\n" ) != 1 ) )
        succes = false;
#endif
    return succes;
}


wxString LibraryStruct::GetName()
{
    return m_Name;
}
