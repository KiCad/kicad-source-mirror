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
            names.Add( entry.GetName() );
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
            names.Add( entry.GetName() );
        if( !nameSearch.IsEmpty() && WildCompareString( nameSearch,
                                                        entry.GetName(),
                                                        false ) )
            names.Add( entry.GetName() );
    }

    if( sort )
        names.Sort();
}


LibCmpEntry* LibraryStruct::FindEntry( const wxChar* name )
{
    BOOST_FOREACH( LibCmpEntry& entry, m_Entries )
    {
        if( entry.GetName().CmpNoCase( name ) == 0 )
            return &entry;
    }

    return NULL;
}


LibCmpEntry* LibraryStruct::FindEntry( const wxChar* name, LibrEntryType type )
{
    BOOST_FOREACH( LibCmpEntry& entry, m_Entries )
    {
        if( entry.GetName().CmpNoCase( name ) == 0 && entry.Type == type )
            return &entry;
    }

    return NULL;
}


EDA_LibComponentStruct* LibraryStruct::FindComponent( const wxChar* name,
                                                      bool searchAliases )
{
    if( !searchAliases )
        return (EDA_LibComponentStruct*) FindEntry( name, ROOT );

    EDA_LibComponentStruct* component = NULL;
    LibCmpEntry* entry = FindEntry( name );

    if( entry != NULL && entry->Type == ALIAS )
    {
        EDA_LibCmpAliasStruct* alias = (EDA_LibCmpAliasStruct*) entry;
        component = (EDA_LibComponentStruct*) FindEntry( alias->m_RootName,
                                                         ROOT );
    }
    else
    {
        component = (EDA_LibComponentStruct*) entry;
    }

    return component;
}


bool LibraryStruct::AddAlias( EDA_LibCmpAliasStruct* alias )
{
    wxASSERT( alias != NULL );

    if( FindEntry( alias->GetName() ) != NULL )
    {
        wxString msg;

        msg.Printf( _( "Cannot add duplicate alias <%s> to library <%s>." ),
                    (const wxChar*) alias->GetName(), (const wxChar*) m_Name );
        return false;
    }

    m_Entries.push_back( (LibCmpEntry*) alias );
    m_IsModified = true;
    return true;
}


EDA_LibComponentStruct* LibraryStruct::AddComponent( EDA_LibComponentStruct* cmp )
{
    wxASSERT( cmp != NULL );

    EDA_LibComponentStruct* newCmp = CopyLibEntryStruct( cmp );

    if( newCmp == NULL )
        return NULL;

    m_Entries.push_back( (LibCmpEntry*) newCmp );
    m_IsModified = true;

    for( size_t i = 0; i < newCmp->m_AliasList.GetCount(); i++ )
    {
        EDA_LibCmpAliasStruct* alias = FindAlias( newCmp->m_AliasList[ i ] );

        if( alias == NULL )
        {
            alias = new EDA_LibCmpAliasStruct( newCmp->m_AliasList[ i ],
                                               newCmp->GetName() );
            m_Entries.push_back( alias );
        }
        else if( alias->m_RootName != newCmp->GetName() )
        {
            wxLogError( _( "Conflict in library <%s>: alias <%s> already has \
root name <%s> and will not be assigned to root name <%s>." ),
                        (const wxChar*) m_Name,
                        (const wxChar*) alias->m_RootName,
                        (const wxChar*) newCmp->GetName() );
        }
    }

    m_Entries.sort();

    return newCmp;
}


void LibraryStruct::RemoveEntry( const wxString& name )
{
    LIB_ENTRY_LIST::iterator i;

    for( i = m_Entries.begin(); i < m_Entries.end(); i++ )
    {
        if( i->GetName().CmpNoCase( name ) == 0 )
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
        Root = FindComponent( Alias->m_RootName );

        /* Remove alias name from the root component alias list */
        if( Root == NULL )
        {
            wxLogWarning( wxT( "No root component found for alias <%s> in \
library <%s>." ),
                          ( const wxChar* ) entry->GetName(),
                          ( const wxChar* ) m_Name );
        }
        else
        {
            int index = Root->m_AliasList.Index( entry->GetName(), false );

            if( index == wxNOT_FOUND )
                wxLogWarning( wxT( "Alias <%s> not found in component <%s> \
alias list in library <%s>" ),
                              ( const wxChar* ) entry->GetName(),
                              ( const wxChar* ) Root->GetName(),
                              ( const wxChar* ) m_Name );
            else
                Root->m_AliasList.RemoveAt( index );
        }

        RemoveEntry( Alias->GetName() );

        return;
    }

    Root = ( EDA_LibComponentStruct* ) entry;

    /* Entry is a component with no aliases so removal is simple. */
    if( Root->m_AliasList.GetCount() == 0 )
    {
        RemoveEntry( Root->GetName() );
        return;
    }

    /* Entry is a component with one or more alias. */
    wxString AliasName = Root->m_AliasList[0];

    /* The root component is not really deleted, it is renamed with the first
     * alias name. */
    Alias = FindAlias( AliasName );

    if( Alias == NULL )
    {
        wxLogWarning( wxT( "Alias <%s> for component <%s> not found in \
library <%s>" ),
                      ( const wxChar* ) AliasName,
                      ( const wxChar* ) Root->GetName(),
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
        Alias = FindAlias( Root->m_AliasList[ii] );

        /* Should not occur if library was saved by the library editor.
         * However, it is possible if the library was edited by hand or
         * some other program or a there is a bug in the library editor. */
        if( Alias == NULL )
        {
            wxLogWarning( wxT( "Alias <%s> for component <%s> not found in \
library <%s>." ),
                          ( const wxChar* ) AliasName,
                          ( const wxChar* ) Root->GetName(),
                          ( const wxChar* ) m_Name );
            continue;
        }

        Alias->m_RootName = Root->GetName();
    }
}


EDA_LibComponentStruct* LibraryStruct::ReplaceComponent(
    EDA_LibComponentStruct* oldComponent,
    EDA_LibComponentStruct* newComponent )
{
    wxASSERT( oldComponent != NULL && newComponent != NULL
              && oldComponent->GetName().CmpNoCase( newComponent->GetName() )== 0 );

    size_t i;
    int index;
    EDA_LibCmpAliasStruct* alias;

    if( oldComponent->m_AliasList != newComponent->m_AliasList )
    {
        /* Remove extra aliases. */
        for( i = 0; i < oldComponent->m_AliasList.GetCount(); i++ )
        {
             index =
                newComponent->m_AliasList.Index( oldComponent->m_AliasList[ i ] );

            if( index != wxNOT_FOUND )
                continue;

            wxLogDebug( wxT( "Removing extra alias <%s> from component <%s> \
in library <%s>." ),
                        (const wxChar*) oldComponent->m_AliasList[ i ],
                        (const wxChar*) oldComponent->GetName(),
                        (const wxChar*) m_Name );

            RemoveEntry( oldComponent->m_AliasList[ i ] );
        }

        /* Add new aliases. */
        for( i = 0; i < newComponent->m_AliasList.GetCount(); i++ )
        {
             index =
                oldComponent->m_AliasList.Index( newComponent->m_AliasList[ i ] );

            if( index != wxNOT_FOUND
                || FindEntry( newComponent->m_AliasList[ i ] ) != NULL )
                continue;

            wxLogDebug( wxT( "Adding extra alias <%s> from component <%s> \
in library <%s>." ),
                        (const wxChar*) newComponent->m_AliasList[ i ],
                        (const wxChar*) newComponent->GetName(),
                        (const wxChar*) m_Name );

            alias = new EDA_LibCmpAliasStruct( newComponent->m_AliasList[ i ],
                                               newComponent->GetName() );
            m_Entries.push_back( alias );
        }
    }

    RemoveEntry( oldComponent->GetName() );

    EDA_LibComponentStruct* newCmp = CopyLibEntryStruct( newComponent );

    if( newCmp == NULL )
        return NULL;

    m_Entries.push_back( (LibCmpEntry*) newCmp );
    m_Entries.sort();
    m_IsModified = true;
    return newCmp;
}


LibCmpEntry* LibraryStruct::GetNextEntry( const wxChar* name )
{
    size_t i;
    LibCmpEntry* entry = NULL;

    for( i = 0; i < m_Entries.size(); i++ )
    {
        if( m_Entries[i].GetName().CmpNoCase( name ) == 0 )
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
        if( m_Entries[i].GetName().CmpNoCase( name ) == 0 && entry )
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
                                       component->GetName() );
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

        Entry = FindEntry( cmpname );

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
