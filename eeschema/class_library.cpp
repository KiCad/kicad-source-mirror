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

#include <wx/tokenzr.h>


static const wxChar* duplicate_name_msg = _( "Component library <%s> has \
duplicate entry name <%s>.  This may cause some expected behavior when \
loading components into a schematic." );


static bool DuplicateEntryName( const CMP_LIB_ENTRY& item1,
                                const CMP_LIB_ENTRY& item2 )
{
    return item1.m_Name.m_Text.CmpNoCase( item2.m_Name.m_Text ) == 0;
}


bool operator==( const CMP_LIBRARY& lib, const wxChar* name )
{
    return lib.GetName().CmpNoCase( name ) == 0;
}


bool operator!=( const CMP_LIBRARY& lib, const wxChar* name )
{
    return !( lib == name );
}


bool operator<( const CMP_LIBRARY& item1, const CMP_LIBRARY& item2 )
{
    /* The cache library always is sorted to the end of the library list. */
    if( item1.IsCache() )
        return true;
    if( item2.IsCache() )
        return false;

    /* If the sort order array isn't set, then sort alphabetically except. */
    if( CMP_LIBRARY::GetSortOrder().IsEmpty() )
        return item1.GetName().CmpNoCase( item2.GetName() ) < 0;

    int i1 = CMP_LIBRARY::GetSortOrder().Index( item1.GetName(), false );
    int i2 = CMP_LIBRARY::GetSortOrder().Index( item2.GetName(), false );

    if( i1 == wxNOT_FOUND && i2 == wxNOT_FOUND )
        return true;

    if( i1 == wxNOT_FOUND && i2 != wxNOT_FOUND )
        return false;

    if( i1 != wxNOT_FOUND && i2 == wxNOT_FOUND )
        return true;

    return ( i1 - i2 ) < 0;
}


CMP_LIBRARY::CMP_LIBRARY( int type, const wxFileName& fileName )
{
    m_Type         = type;      /* type indicator */
    m_IsModified   = false;     /* flag indicateur d'edition */
    m_TimeStamp    = 0;
    m_Flags        = 0;
    m_IsCache      = false;
    m_DateTime     = wxDateTime::Now();

    if( fileName.IsOk() )
        m_fileName = fileName;
    else
        m_fileName = wxFileName( wxT( "unnamed.lib" ) );
}


CMP_LIBRARY::~CMP_LIBRARY()
{
}


void CMP_LIBRARY::GetEntryNames( wxArrayString& names, bool sort,
                                 bool makeUpperCase )
{
    BOOST_FOREACH( CMP_LIB_ENTRY& entry, m_Entries )
    {
        if( makeUpperCase )
            names.Add( entry.m_Name.m_Text.MakeUpper() );
        else
            names.Add( entry.GetName() );
    }

    if( sort )
        names.Sort();
}


void CMP_LIBRARY::SearchEntryNames( wxArrayString& names,
                                    const wxString& nameSearch,
                                    const wxString& keySearch,
                                    bool sort )
{
    BOOST_FOREACH( CMP_LIB_ENTRY& entry, m_Entries )
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


CMP_LIB_ENTRY* CMP_LIBRARY::FindEntry( const wxChar* name )
{
    BOOST_FOREACH( CMP_LIB_ENTRY& entry, m_Entries )
    {
        if( entry.GetName().CmpNoCase( name ) == 0 )
            return &entry;
    }

    return NULL;
}


CMP_LIB_ENTRY* CMP_LIBRARY::FindEntry( const wxChar* name, LibrEntryType type )
{
    BOOST_FOREACH( CMP_LIB_ENTRY& entry, m_Entries )
    {
        if( entry.GetName().CmpNoCase( name ) == 0 && entry.Type == type )
            return &entry;
    }

    return NULL;
}


LIB_COMPONENT* CMP_LIBRARY::FindComponent( const wxChar* name )
{
    LIB_COMPONENT* component = NULL;
    CMP_LIB_ENTRY* entry = FindEntry( name );

    if( entry != NULL && entry->Type == ALIAS )
    {
        LIB_ALIAS* alias = (LIB_ALIAS*) entry;
        component = alias->GetComponent();
    }
    else
    {
        component = (LIB_COMPONENT*) entry;
    }

    return component;
}


bool CMP_LIBRARY::AddAlias( LIB_ALIAS* alias )
{
    wxASSERT( alias != NULL );

    if( FindEntry( alias->GetName() ) != NULL )
    {
        wxString msg;

        msg.Printf( _( "Cannot add duplicate alias <%s> to library <%s>." ),
                    (const wxChar*) alias->GetName(),
                    (const wxChar*) m_fileName.GetName() );
        return false;
    }

    m_Entries.push_back( (CMP_LIB_ENTRY*) alias );
    m_IsModified = true;
    return true;
}


LIB_COMPONENT* CMP_LIBRARY::AddComponent( LIB_COMPONENT* cmp )
{
    wxASSERT( cmp != NULL );

    LIB_COMPONENT* newCmp = CopyLibEntryStruct( cmp );

    if( newCmp == NULL )
        return NULL;

    m_Entries.push_back( (CMP_LIB_ENTRY*) newCmp );
    m_IsModified = true;

    /* Cache libraries are component only libraries.  Do not create alias
     * entries. */
    if( !m_IsCache )
    {
        for( size_t i = 0; i < newCmp->m_AliasList.GetCount(); i++ )
        {
            LIB_ALIAS* alias = FindAlias( newCmp->m_AliasList[ i ] );

            if( alias == NULL )
            {
                alias = new LIB_ALIAS( newCmp->m_AliasList[ i ], newCmp );
                m_Entries.push_back( alias );
            }
            else if( alias->GetComponent()->GetName().CmpNoCase( newCmp->GetName() ) != 0 )
            {
                wxLogError( _( "Conflict in library <%s>: alias <%s> already \
has root name <%s> and will not be assigned to root name <%s>." ),
                        (const wxChar*) m_fileName.GetName(),
                        (const wxChar*) alias->GetComponent()->GetName(),
                        (const wxChar*) newCmp->GetName() );
            }
        }
    }

    m_Entries.sort();
    m_Entries.unique( DuplicateEntryName );

    return newCmp;
}


void CMP_LIBRARY::RemoveEntry( const wxString& name )
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


void CMP_LIBRARY::RemoveEntry( CMP_LIB_ENTRY* entry )
{
    wxASSERT( entry != NULL );

    LIB_COMPONENT* Root;
    LIB_ALIAS*  Alias;

    m_IsModified = true;

    if( entry->Type == ALIAS )
    {
        Alias = (LIB_ALIAS*) entry;
        Root = Alias->GetComponent();

        /* Remove alias name from the root component alias list */
        if( Root == NULL )
        {
            wxLogWarning( wxT( "No root component found for alias <%s> in \
library <%s>." ),
                          ( const wxChar* ) entry->GetName(),
                          ( const wxChar* ) m_fileName.GetName() );
        }
        else
        {
            int index = Root->m_AliasList.Index( entry->GetName(), false );

            if( index == wxNOT_FOUND )
                wxLogWarning( wxT( "Alias <%s> not found in component <%s> \
alias list in library <%s>" ),
                              ( const wxChar* ) entry->GetName(),
                              ( const wxChar* ) Root->GetName(),
                              ( const wxChar* ) m_fileName.GetName() );
            else
                Root->m_AliasList.RemoveAt( index );
        }

        RemoveEntry( Alias->GetName() );

        return;
    }

    Root = ( LIB_COMPONENT* ) entry;

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
                      ( const wxChar* ) m_fileName.GetName() );
        return;
    }

    /* Remove the first alias name from the component alias list. */
    Root->m_AliasList.RemoveAt( 0 );

    /* Rename the component to the name of the first alias. */
    Root->m_Doc = Alias->m_Doc;
    Root->m_KeyWord = Alias->m_KeyWord;

    /* Remove the first alias from library. */
    RemoveEntry( AliasName );

    /* Change the root name. */
    Root->m_Name.m_Text = AliasName;
}


LIB_COMPONENT* CMP_LIBRARY::ReplaceComponent( LIB_COMPONENT* oldComponent,
                                              LIB_COMPONENT* newComponent )
{
    wxASSERT( oldComponent != NULL && newComponent != NULL
              && oldComponent->GetName().CmpNoCase( newComponent->GetName() )== 0 );

    size_t i;
    int index;
    LIB_ALIAS* alias;

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
                        (const wxChar*) m_fileName.GetName() );

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
                        (const wxChar*) m_fileName.GetName() );

            alias = new LIB_ALIAS( newComponent->m_AliasList[ i ],
                                   newComponent );
            m_Entries.push_back( alias );
        }
    }

    RemoveEntry( oldComponent->GetName() );

    LIB_COMPONENT* newCmp = CopyLibEntryStruct( newComponent );

    if( newCmp == NULL )
        return NULL;

    m_Entries.push_back( (CMP_LIB_ENTRY*) newCmp );
    m_Entries.sort();
    m_IsModified = true;
    return newCmp;
}


CMP_LIB_ENTRY* CMP_LIBRARY::GetNextEntry( const wxChar* name )
{
    size_t i;
    CMP_LIB_ENTRY* entry = NULL;

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


CMP_LIB_ENTRY* CMP_LIBRARY::GetPreviousEntry( const wxChar* name )
{
    size_t i;
    CMP_LIB_ENTRY* entry = NULL;

    for( i = 0; i < m_Entries.size(); i++ )
    {
        if( m_Entries[i].GetName().CmpNoCase( name ) == 0 && entry )
            break;

        entry = &m_Entries[i];
    }

    return entry;
}


bool CMP_LIBRARY::Load( wxString& errMsg )
{
    FILE*          f;
    int            LineNum = 0;
    char           Line[1024];
    LIB_COMPONENT* LibEntry;
    wxString       msg;

    if( m_fileName.GetFullPath().IsEmpty() )
    {
        errMsg = _( "The component library file name is not set." );
        return false;
    }

    f = wxFopen( m_fileName.GetFullPath(), wxT( "rt" ) );

    if( f == NULL )
    {
        errMsg = _( "The file could not be opened." );
        return false;
    }

    if( GetLine( f, Line, &LineNum, sizeof( Line ) ) == NULL )
    {
        errMsg = _( "The file is empty!" );
        return false;
    }

    /* There is no header if this is a symbol library. */
    if( m_Type == LIBRARY_TYPE_EESCHEMA )
    {
        wxString tmp;

        m_Header = CONV_FROM_UTF8( Line );

        wxStringTokenizer tkn( m_Header );

        /*
         * The file header (first line) in library versions 2.0 and lower
         * apparently started with EESchema-LIB.  Sometime after 2.0, it
         * was changed to EESchema-LIBRARY.  Therefore, the test for
         * EESchema-LIB will work in both cases.  Don't change this unless
         * backwards compatability is no longer required.
         */
        if( !tkn.HasMoreTokens()
            || !tkn.GetNextToken().Upper().StartsWith(wxT( "EESCHEMA-LIB" ) ) )
        {
            errMsg = _( "The file is NOT an EESCHEMA library!" );
            return false;
        }

        if( !tkn.HasMoreTokens() )
        {
            errMsg = _( "The file header is missing version and time stamp \
information." );
            return false;
        }

        if( tkn.GetNextToken() != wxT( "Version" ) || !tkn.HasMoreTokens() )
        {
            errMsg = _( "The file header version information is invalid." );
            return false;
        }

        long major, minor;
        wxStringTokenizer vers( tkn.GetNextToken(), wxT( "." ) );

        if( !vers.HasMoreTokens() || !vers.GetNextToken().ToLong( &major )
            || major < 1L || !vers.HasMoreTokens()
            || !vers.GetNextToken().ToLong( & minor ) || minor < 0L
            || minor > 99 )
        {
            wxLogWarning( _( "The component library <%s> header version \
number is invalid.\n\nIn future versions of EESchema this library may not \
load correctly.\nTo resolve this problem open the library in the library \
editor and save it.\nIf this library is the project cache library, save \
the current schematic." ),
                          (const wxChar*) GetName() );
        }
        else
        {
            m_verMajor = (int) major;
            m_verMinor = (int) minor;

            wxLogDebug( wxT( "Component library <%s> is version %d.%d." ),
                        (const wxChar*) GetName(), m_verMajor, m_verMinor );
        }
    }

    while( GetLine( f, Line, &LineNum, sizeof( Line ) ) )
    {
        if( m_Type == LIBRARY_TYPE_EESCHEMA
            && strnicmp( Line, "$HEADER", 7 ) == 0 )
        {
            if( !LoadHeader( f, &LineNum ) )
            {
                errMsg = _( "An error occured attempting to read the header." );
                return false;
            }

            continue;
        }

        if( strnicmp( Line, "DEF", 3 ) == 0 )
        {
            /* Read one DEF/ENDDEF part entry from library: */
            LibEntry = new LIB_COMPONENT( wxEmptyString, this );

            if( LibEntry->Load( f, Line, &LineNum, msg ) )
            {
                /* Check for duplicate entry names and warn the user about
                 * the potential conflict.
                 */
                if( FindEntry( LibEntry->GetName() ) != NULL )
                {
                    wxLogWarning( duplicate_name_msg,
                                  (const wxChar*) m_fileName.GetName(),
                                  (const wxChar*) LibEntry->GetName() );
                }

                /* If we are here, this part is O.k. - put it in: */
                m_Entries.push_back( LibEntry );
                LoadAliases( LibEntry );
            }
            else
            {
                wxLogWarning( _( "Library <%s> component load error %s." ),
                              (const wxChar*) m_fileName.GetName(),
                              (const wxChar*) msg );
                msg.Clear();
                delete LibEntry;
            }
        }
    }

    m_Entries.sort();

    return true;
}


void CMP_LIBRARY::LoadAliases( LIB_COMPONENT* component )
{
    wxASSERT( component != NULL && component->Type == ROOT );

    LIB_ALIAS* alias;
    unsigned   ii;

    for( ii = 0; ii < component->m_AliasList.GetCount(); ii++ )
    {
        if( FindEntry( component->m_AliasList[ii] ) != NULL )
        {
            wxLogError( duplicate_name_msg,
                        (const wxChar*) m_fileName.GetName(),
                        (const wxChar*) component->m_AliasList[ii] );
        }

        alias = new LIB_ALIAS( component->m_AliasList[ii], component, this );
        m_Entries.push_back( alias );
    }
}


bool CMP_LIBRARY::LoadHeader( FILE* libfile, int* LineNum )
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


bool CMP_LIBRARY::LoadDocs( wxString& errMsg )
{
    int            LineNum = 0;
    char           Line[1024], * Name, * Text;
    CMP_LIB_ENTRY* Entry;
    FILE*          f;
    wxString       msg;
    wxFileName     fn = m_fileName;

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


bool CMP_LIBRARY::Save( const wxString& FullFileName )
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
        msg = wxT( "Failed to create component library file " ) +
            libFileName.GetFullPath();
        DisplayError( NULL, msg );
        return false;
    }

    docfile = wxFopen( docFileName.GetFullPath(), wxT( "wt" ) );

    if( docfile == NULL )
    {
        msg = wxT( "Failed to create component document library file " ) +
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

    BOOST_FOREACH( CMP_LIB_ENTRY& entry, m_Entries )
    {
        if ( entry.Type == ROOT )
        {
            LIB_COMPONENT* component = ( LIB_COMPONENT* ) &entry;
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


bool CMP_LIBRARY::SaveHeader( FILE* file )
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


/*
 * The static library list and list management methods.
 */
CMP_LIBRARY_LIST CMP_LIBRARY::m_LibraryList;
wxArrayString CMP_LIBRARY::m_LibraryListSortOrder;


CMP_LIBRARY* CMP_LIBRARY::LoadLibrary( const wxFileName& fileName,
                                       wxString& errMsg )
{
    CMP_LIBRARY* lib = NULL;

    lib = new CMP_LIBRARY( LIBRARY_TYPE_EESCHEMA, fileName );

    wxBusyCursor ShowWait;

    if( !lib->Load( errMsg ) )
    {
        delete lib;
        return NULL;
    }

    lib->LoadDocs( errMsg );

    return lib;
}


bool CMP_LIBRARY::AddLibrary( const wxFileName& fileName, wxString& errMsg )
{
    CMP_LIBRARY* lib;

    /* Don't reload the library if it is already loaded. */
    lib = FindLibrary( fileName.GetName() );

    if( lib != NULL )
        return true;

    lib = LoadLibrary( fileName, errMsg );

    if( lib == NULL )
        return false;

    m_LibraryList.push_back( lib );

    return true;
}


bool CMP_LIBRARY::AddLibrary( const wxFileName& fileName, wxString& errMsg,
                              CMP_LIBRARY_LIST::iterator& i )
{
    CMP_LIBRARY* lib;

    /* Don't reload the library if it is already loaded. */
    lib = FindLibrary( fileName.GetName() );

    if( lib != NULL )
        return true;

    lib = LoadLibrary( fileName, errMsg );

    if( lib == NULL )
        return false;

    if( i >= m_LibraryList.begin() && i < m_LibraryList.end() )
        m_LibraryList.insert( i, lib );
    else
        m_LibraryList.push_back( lib );

    return true;
}


void CMP_LIBRARY::RemoveLibrary( const wxString& name )
{
    if( name.IsEmpty() )
        return;

    CMP_LIBRARY_LIST::iterator i;

    for ( i = m_LibraryList.begin(); i < m_LibraryList.end(); i++ )
    {
        if( i->GetName().CmpNoCase( name ) == 0 )
        {
            CMP_LIBRARY::m_LibraryList.erase( i );
            return;
        }
    }
}


CMP_LIBRARY* CMP_LIBRARY::FindLibrary( const wxString& name )
{
    BOOST_FOREACH( CMP_LIBRARY& lib, m_LibraryList )
    {
        if( lib == name )
            return &lib;
    }

    return NULL;
}

wxArrayString CMP_LIBRARY::GetLibraryNames( bool sorted )
{
    wxString cacheName;
    wxArrayString names;

    BOOST_FOREACH( CMP_LIBRARY& lib, CMP_LIBRARY::m_LibraryList )
    {
        if( lib.m_IsCache && sorted )
            cacheName = lib.GetName();
        else
            names.Add( lib.GetName() );
    }

    /* Even sorted, the cache library is always at the end of the list. */
    if( sorted )
        names.Sort();

    if( !cacheName.IsEmpty() )
        names.Add( cacheName );

    return names;
}


LIB_COMPONENT* CMP_LIBRARY::FindLibraryComponent( const wxString& name,
                                                  const wxString& libName )
{
    LIB_COMPONENT* component = NULL;

    BOOST_FOREACH( CMP_LIBRARY& lib, m_LibraryList )
    {
        if( !libName.IsEmpty() && lib.GetName() != libName )
            continue;

        component = lib.FindComponent( name );

        if( component != NULL )
            break;
    }

    return component;
}


CMP_LIB_ENTRY* CMP_LIBRARY::FindLibraryEntry( const wxString& name,
                                              const wxString& libName )
{
    CMP_LIB_ENTRY* entry = NULL;

    BOOST_FOREACH( CMP_LIBRARY& lib, m_LibraryList )
    {
        if( !libName.IsEmpty() && lib.GetName() != libName )
            continue;

        entry = lib.FindEntry( name );

        if( entry != NULL )
            break;
    }

    return entry;
}


void CMP_LIBRARY::RemoveCacheLibrary( void )
{
    CMP_LIBRARY_LIST::iterator i;

    for ( i = m_LibraryList.begin(); i < m_LibraryList.end(); i++ )
    {
        if( i->m_IsCache )
            m_LibraryList.erase( i-- );
    }
}
