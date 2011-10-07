/**
 * @file class_library.cpp
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"
#include "kicad_string.h"
#include "confirm.h"
#include "gestfich.h"
#include "eda_doc.h"
#include "wxstruct.h"

#include "general.h"
#include "protos.h"
#include "class_library.h"

#include <boost/foreach.hpp>

#include <wx/tokenzr.h>
#include <wx/regex.h>


static const wxChar* duplicate_name_msg =
_( "Library <%s> has duplicate entry name <%s>.\n\
This may cause some unexpected behavior when loading components into a schematic." );


bool operator==( const CMP_LIBRARY& aLibrary, const wxChar* aName )
{
    return aLibrary.GetName().CmpNoCase( aName ) == 0;
}


bool operator!=( const CMP_LIBRARY& aLibrary, const wxChar* aName )
{
    return !( aLibrary == aName );
}


bool operator<( const CMP_LIBRARY& aItem1, const CMP_LIBRARY& aItem2 )
{
    /* The cache library always is sorted to the end of the library list. */
    if( aItem2.IsCache() )
        return true;
    if( aItem1.IsCache() )
        return false;

    /* If the sort order array isn't set, then sort alphabetically except. */
    if( CMP_LIBRARY::GetSortOrder().IsEmpty() )
        return aItem1.GetName().CmpNoCase( aItem2.GetName() ) < 0;

    int i1 = CMP_LIBRARY::GetSortOrder().Index( aItem1.GetName(), false );
    int i2 = CMP_LIBRARY::GetSortOrder().Index( aItem2.GetName(), false );

    if( i1 == wxNOT_FOUND && i2 == wxNOT_FOUND )
        return true;

    if( i1 == wxNOT_FOUND && i2 != wxNOT_FOUND )
        return false;

    if( i1 != wxNOT_FOUND && i2 == wxNOT_FOUND )
        return true;

    return ( i1 - i2 ) < 0;
}


CMP_LIBRARY::CMP_LIBRARY( int aType, const wxFileName& aFileName )
{
    type = aType;
    isModified = false;
    timeStamp = 0;
    isCache = false;
    timeStamp = wxDateTime::Now();

    if( aFileName.IsOk() )
        fileName = aFileName;
    else
        fileName = wxFileName( wxT( "unnamed.lib" ) );
}


CMP_LIBRARY::~CMP_LIBRARY()
{
    for( LIB_ALIAS_MAP::iterator it=aliases.begin();  it!=aliases.end();  it++ )
    {
        LIB_ALIAS* alias = (*it).second;
        LIB_COMPONENT* component = alias->GetComponent();
        alias = component->RemoveAlias( alias );

        if( alias == NULL )
            delete component;
    }
}


void CMP_LIBRARY::GetEntryNames( wxArrayString& aNames, bool aSort, bool aMakeUpperCase )
{
    LIB_ALIAS_MAP::iterator it;

    for( it=aliases.begin();  it!=aliases.end();  it++ )
    {
        if( aMakeUpperCase )
        {
            wxString tmp = (*it).first;
            tmp.MakeUpper();
            aNames.Add( tmp );
        }
        else
        {
            aNames.Add( (*it).first );
        }
    }

    if( aSort )
        aNames.Sort();
}


void CMP_LIBRARY::SearchEntryNames( wxArrayString& aNames,
                                    const wxString& aNameSearch,
                                    const wxString& aKeySearch,
                                    bool aSort )
{
    LIB_ALIAS_MAP::iterator it;

    for( it=aliases.begin();  it!=aliases.end();  it++ )
    {
        if( !aKeySearch.IsEmpty() && KeyWordOk( aKeySearch, (*it).second->GetKeyWords() ) )
            aNames.Add( (*it).first );
        if( !aNameSearch.IsEmpty() && WildCompareString( aNameSearch,
                                                         (*it).second->GetName(), false ) )
            aNames.Add( (*it).first );
    }

    if( aSort )
        aNames.Sort();
}


void CMP_LIBRARY::SearchEntryNames( wxArrayString& aNames, const wxRegEx& aRe, bool aSort )
{
    if( !aRe.IsValid() )
        return;

    LIB_ALIAS_MAP::iterator it;

    for( it=aliases.begin();  it!=aliases.end();  it++ )
    {
        if( aRe.Matches( (*it).second->GetKeyWords() ) )
            aNames.Add( (*it).first );
    }

    if( aSort )
        aNames.Sort();
}


bool CMP_LIBRARY::Conflicts( LIB_COMPONENT* aComponent )
{
    wxCHECK_MSG( aComponent != NULL, false,
                 wxT( "Cannot test NULL component for conflicts in library " ) + GetName() );

    for( size_t i=0;  i<aComponent->m_aliases.size();  i++ )
    {
        LIB_ALIAS_MAP::iterator it = aliases.find( aComponent->m_aliases[i]->GetName() );

        if( it != aliases.end() )
            return true;
    }

    return false;
}


LIB_ALIAS* CMP_LIBRARY::FindEntry( const wxChar* aName )
{

    LIB_ALIAS_MAP::iterator it = aliases.find( wxString( aName ) );

    if( it != aliases.end() )
        return (*it).second;

    return NULL;
}


/**
 * Return the first entry in the library.
 * @return The first entry or NULL if the library has no entries.
 */
LIB_ALIAS* CMP_LIBRARY::GetFirstEntry()
{
    if( aliases.size() )
        return (*aliases.begin()).second;
    else
        return NULL;
}

LIB_COMPONENT* CMP_LIBRARY::FindComponent( const wxChar* aName )
{
    LIB_COMPONENT* component = NULL;
    LIB_ALIAS* entry = FindEntry( aName );

    if( entry != NULL )
        component = entry->GetComponent();

    return component;
}


bool CMP_LIBRARY::AddAlias( LIB_ALIAS* aAlias )
{
    wxASSERT( aAlias != NULL );

    LIB_ALIAS_MAP::iterator it = aliases.find( aAlias->GetName() );

    if( it != aliases.end() )
    {
        wxString msg;

        msg.Printf( _( "Cannot add duplicate alias <%s> to library <%s>." ),
                    GetChars( aAlias->GetName() ),
                    GetChars( fileName.GetName() ) );
        return false;
    }

    aliases[ aAlias->GetName() ] = aAlias;
    isModified = true;
    return true;
}


LIB_COMPONENT* CMP_LIBRARY::AddComponent( LIB_COMPONENT* aComponent )
{
    if( aComponent == NULL )
        return NULL;

    // Conflict detection: See if already existing aliases exist,
    // and if yes, ask user for continue or abort
    // Special case: if the library is the library cache of the project,
    // old aliases are always removed to avoid conflict,
    //      and user is not prompted )
    if( Conflicts( aComponent ) && !IsCache() )
    {
        wxFAIL_MSG( wxT( "Cannot add component <" ) + aComponent->GetName() +
                    wxT( "> to library <" ) + GetName() + wxT( "> due to name conflict." ) );
        return NULL;
    }


    LIB_COMPONENT* newCmp = new LIB_COMPONENT( *aComponent, this );

    for( size_t i = 0; i < newCmp->m_aliases.size(); i++ )
    {
        wxString aliasname = newCmp->m_aliases[i]->GetName();
        LIB_ALIAS* alias = FindAlias( aliasname );

        if( alias != NULL )
            RemoveEntry( alias );

        aliases[ aliasname ] = newCmp->m_aliases[i];
    }

    isModified = true;

    return newCmp;
}


LIB_ALIAS* CMP_LIBRARY::RemoveEntry( LIB_ALIAS* aEntry )
{
    wxCHECK_MSG( aEntry != NULL, NULL, wxT( "NULL pointer cannot be removed from library." ) );

    LIB_ALIAS_MAP::iterator it = aliases.find( aEntry->GetName() );

    if( it == aliases.end() )
        return NULL;

    // If the entry pointer doesn't match the name it is mapped to in the library, we
    // have done someething terribly wrong.
    wxCHECK_MSG( (*it).second == aEntry, NULL,
                 wxT( "Pointer mismatch while attempting to remove entry <" ) +
                 aEntry->GetName() + wxT( "> from library <" ) + GetName() + wxT( ">." ) );

    LIB_ALIAS* alias = (LIB_ALIAS*) aEntry;
    LIB_COMPONENT* component = alias->GetComponent();
    alias = component->RemoveAlias( alias );

    if( alias == NULL )
    {
        delete component;

        if( aliases.size() > 1 )
        {
            LIB_ALIAS_MAP::iterator next = it;
            next++;

            if( next == aliases.end() )
                next = aliases.begin();

            alias = (*next).second;
        }
    }

    aliases.erase( it );
    isModified = true;

    return alias;
}


LIB_COMPONENT* CMP_LIBRARY::ReplaceComponent( LIB_COMPONENT* aOldComponent,
                                              LIB_COMPONENT* aNewComponent )
{
    wxASSERT( aOldComponent != NULL );
    wxASSERT( aNewComponent != NULL );

    /* Remove the old root component.  The component will automatically be deleted
     * when all it's aliases are deleted.  Do not place any code that accesses
     * aOldComponent inside this loop that gets evaluated after the last alias is
     * removed in RemoveEntry().  Failure to heed this warning will result in a
     * segfault.
     */
    size_t i = aOldComponent->m_aliases.size();

    while( i != 0 )
    {
        i -= 1;
        RemoveEntry( aOldComponent->m_aliases[ i ] );
    }

    LIB_COMPONENT* newCmp = new LIB_COMPONENT( *aNewComponent, this );

    // Add new aliases to library alias map.
    for( i = 0; i < newCmp->m_aliases.size(); i++ )
    {
        aliases[ newCmp->m_aliases[ i ]->GetName() ] = newCmp->m_aliases[ i ];
    }

    isModified = true;

    return newCmp;
}


LIB_ALIAS* CMP_LIBRARY::GetNextEntry( const wxChar* aName )
{
    if( aliases.empty() )
        return NULL;

    LIB_ALIAS_MAP::iterator it = aliases.find( wxString( aName ) );

    it++;

    if( it == aliases.end() )
        it = aliases.begin();

    return (*it).second;
}


LIB_ALIAS* CMP_LIBRARY::GetPreviousEntry( const wxChar* aName )
{
    if( aliases.empty() )
        return NULL;

    LIB_ALIAS_MAP::iterator it = aliases.find( wxString( aName ) );

    if( it == aliases.begin() )
        it = aliases.end();

    it--;

    return (*it).second;
}


bool CMP_LIBRARY::Load( wxString& aErrorMsg )
{
    FILE*          file;
    int            lineNumber = 0;
    char           line[LINE_BUFFER_LEN_LARGE];  // Use a very large buffer to load data
    LIB_COMPONENT* libEntry;
    wxString       msg;

    if( fileName.GetFullPath().IsEmpty() )
    {
        aErrorMsg = _( "The component library file name is not set." );
        return false;
    }

    file = wxFopen( fileName.GetFullPath(), wxT( "rt" ) );

    if( file == NULL )
    {
        aErrorMsg = _( "The file could not be opened." );
        return false;
    }

    if( GetLine( file, line, &lineNumber, sizeof( line ) ) == NULL )
    {
        aErrorMsg = _( "The file is empty!" );
        fclose( file );
        return false;
    }

    /* There is no header if this is a symbol library. */
    if( type == LIBRARY_TYPE_EESCHEMA )
    {
        wxString tmp;

        header = FROM_UTF8( line );

        wxStringTokenizer tkn( header );

        /*
         * The file header (first line) in library versions 2.0 and lower
         * apparently started with EESchema-LIB.  Sometime after 2.0, it
         * was changed to EESchema-LIBRARY.  Therefore, the test for
         * EESchema-LIB will work in both cases.  Don't change this unless
         * backwards compatibility is no longer required.
         */
        if( !tkn.HasMoreTokens()
            || !tkn.GetNextToken().Upper().StartsWith(wxT( "EESCHEMA-LIB" ) ) )
        {
            aErrorMsg = _( "The file is NOT an EESCHEMA library!" );
            fclose( file );
            return false;
        }

        if( !tkn.HasMoreTokens() )
        {
            aErrorMsg = _( "The file header is missing version and time stamp information." );
            fclose( file );
            return false;
        }

        if( tkn.GetNextToken() != wxT( "Version" ) || !tkn.HasMoreTokens() )
        {
            aErrorMsg = wxT( "The file header version information is invalid." );
            fclose( file );
            return false;
        }

        long major, minor;
        wxStringTokenizer vers( tkn.GetNextToken(), wxT( "." ) );

        if( !vers.HasMoreTokens() || !vers.GetNextToken().ToLong( &major )
            || major < 1L || !vers.HasMoreTokens()
            || !vers.GetNextToken().ToLong( & minor ) || minor < 0L
            || minor > 99 )
        {
#if 0   // Note for developers:
        // Not sure this warning is very useful: old designs *must* be always loadable
            wxLogWarning( wxT( "The component library <%s> header version \
number is invalid.\n\nIn future versions of Eeschema this library may not \
load correctly.  To resolve this problem open the library in the library \
editor and save it.  If this library is the project cache library, save \
the current schematic." ),
                          GetChars( GetName() ) );
#endif
        }
        else
        {
            versionMajor = (int) major;
            versionMinor = (int) minor;
//            wxLogDebug( wxT( "Component library <%s> is version %d.%d." ),
//                        GetChars( GetName() ), versionMajor, versionMinor );
        }
    }

    while( GetLine( file, line, &lineNumber, sizeof( line ) ) )
    {
        if( type == LIBRARY_TYPE_EESCHEMA && strnicmp( line, "$HEADER", 7 ) == 0 )
        {
            if( !LoadHeader( file, &lineNumber ) )
            {
                aErrorMsg = _( "An error occurred attempting to read the header." );
                fclose( file );
                return false;
            }

            continue;
        }

        if( strnicmp( line, "DEF", 3 ) == 0 )
        {
            /* Read one DEF/ENDDEF part entry from library: */
            libEntry = new LIB_COMPONENT( wxEmptyString, this );

            if( libEntry->Load( file, line, &lineNumber, msg ) )
            {
                /* Check for duplicate entry names and warn the user about
                 * the potential conflict.
                 */
                if( FindEntry( libEntry->GetName() ) != NULL )
                {
                    wxString msg( wxGetTranslation( duplicate_name_msg ) );
                    wxLogWarning( msg,
                                  GetChars( fileName.GetName() ),
                                  GetChars( libEntry->GetName() ) );
                }

                LoadAliases( libEntry );
            }
            else
            {
                wxLogWarning( _( "Library <%s> component load error %s." ),
                              GetChars( fileName.GetName() ),
                              GetChars( msg ) );
                msg.Clear();
                delete libEntry;
            }
        }
    }

    fclose( file );

    return true;
}


void CMP_LIBRARY::LoadAliases( LIB_COMPONENT* component )
{
    wxCHECK_RET( component != NULL,
                 wxT( "Cannot load aliases of NULL component object.  Bad programmer!" ) );

    for( size_t i = 0; i < component->m_aliases.size(); i++ )
    {
        if( FindEntry( component->m_aliases[i]->GetName() ) != NULL )
        {
            wxString msg( wxGetTranslation( duplicate_name_msg ) );
            wxLogError( msg,
                        GetChars( fileName.GetName() ),
                        GetChars( component->m_aliases[i]->GetName() ) );
        }

        aliases[ component->m_aliases[i]->GetName() ] = component->m_aliases[i];
    }
}


bool CMP_LIBRARY::LoadHeader( FILE* libfile, int* LineNum )
{
    char Line[LINE_BUFFER_LEN], * text, * data;

    while( GetLine( libfile, Line, LineNum, sizeof(Line) ) )
    {
        text = strtok( Line, " \t\r\n" );
        data = strtok( NULL, " \t\r\n" );

        if( stricmp( text, "TimeStamp" ) == 0 )
            timeStamp = atol( data );

        if( stricmp( text, "$ENDHEADER" ) == 0 )
            return true;
    }

    return FALSE;
}


bool CMP_LIBRARY::LoadDocs( wxString& aErrorMsg )
{
    int        lineNumber = 0;
    char       line[LINE_BUFFER_LEN_LARGE], * name, * text;
    LIB_ALIAS* entry;
    FILE*      file;
    wxString   msg;
    wxFileName fn = fileName;

    fn.SetExt( DOC_EXT );

    file = wxFopen( fn.GetFullPath(), wxT( "rt" ) );

    if( file == NULL )
    {
        aErrorMsg.Printf( _( "Could not open component document library file <%s>." ),
                          GetChars( fn.GetFullPath() ) );
        return false;
    }

    if( GetLine( file, line, &lineNumber, sizeof(line) ) == NULL )
    {
        aErrorMsg.Printf( _( "Component document library file <%s> is empty." ),
                          GetChars( fn.GetFullPath() ) );
        fclose( file );
        return false;
    }

    if( strnicmp( line, DOCFILE_IDENT, 10 ) != 0 )
    {
        aErrorMsg.Printf( _( "File <%s> is not a valid component library document file." ),
                          GetChars( fn.GetFullPath() ) );
        fclose( file );
        return false;
    }

    while( GetLine( file, line, &lineNumber, sizeof(line) ) )
    {
        if( strncmp( line, "$CMP", 4 ) != 0 )
        {
            aErrorMsg.Printf( wxT( "$CMP command expected in line %d, aborted." ), lineNumber );
            fclose( file );
            return false;
        }

        /* Read one $CMP/$ENDCMP part entry from library: */
        name = strtok( line + 5, "\n\r" );

        wxString cmpname = FROM_UTF8( name );

        entry = FindEntry( cmpname );

        while( GetLine( file, line, &lineNumber, sizeof(line) ) )
        {
            if( strncmp( line, "$ENDCMP", 7 ) == 0 )
                break;
            text = strtok( line + 2, "\n\r" );

            if( entry )
            {
                switch( line[0] )
                {
                case 'D':
                    entry->SetDescription( FROM_UTF8( text ) );
                    break;

                case 'K':
                    entry->SetKeyWords( FROM_UTF8( text ) );
                    break;

                case 'F':
                    entry->SetDocFileName( FROM_UTF8( text ) );
                    break;
                }
            }
        }
    }

    fclose( file );
    return true;
}


bool CMP_LIBRARY::Save( const wxString& aFullFileName, bool aOldDocFormat )
{
    FILE* libfile;
    wxString msg;
    wxFileName libFileName = aFullFileName;
    wxFileName backupFileName = aFullFileName;

    /* the old .lib file is renamed .bak */
    if( libFileName.FileExists() )
    {
        backupFileName.SetExt( wxT( "bak" ) );
        wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxRenameFile( libFileName.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            libFileName.MakeAbsolute();
            msg = wxT( "Failed to rename old component library file " ) +
                backupFileName.GetFullPath();
            DisplayError( NULL, msg );
        }
    }

    libfile = wxFopen( libFileName.GetFullPath(), wxT( "wt" ) );

    if( libfile == NULL )
    {
        libFileName.MakeAbsolute();
        msg = wxT( "Failed to create component library file " ) + libFileName.GetFullPath();
        DisplayError( NULL, msg );
        return false;
    }

    isModified = false;

    timeStamp = GetTimeStamp();
    if( !SaveHeader( libfile ) )
    {
        fclose( libfile );
        return false;
    }

    bool success = true;

    for( LIB_ALIAS_MAP::iterator it=aliases.begin();  it!=aliases.end();  it++ )
    {
        if( !(*it).second->IsRoot() )
            continue;

        if ( !(*it).second->GetComponent()->Save( libfile ) )
            success = false;
    }

    if( fprintf( libfile, "#\n#End Library\n" ) < 0 )
        success = false;

    fclose( libfile );

    if( USE_OLD_DOC_FILE_FORMAT( versionMajor, versionMinor ) && aOldDocFormat )
        success = SaveDocFile( aFullFileName );

    return success;
}


bool CMP_LIBRARY::SaveDocFile( const wxString& aFullFileName )
{
    FILE* docfile;
    wxString msg;
    wxFileName backupFileName = aFullFileName;
    wxFileName docFileName = aFullFileName;

    docFileName.SetExt( DOC_EXT );

    /* Save current doc file as .bck */
    if( docFileName.FileExists() )
    {
        backupFileName = docFileName;
        backupFileName.SetExt( wxT( "bck" ) );
        wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxRenameFile( docFileName.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            msg = wxT( "Failed to save old library document file " ) +
                backupFileName.GetFullPath();
            DisplayError( NULL, msg );
        }
    }

    docfile = wxFopen( docFileName.GetFullPath(), wxT( "wt" ) );

    if( docfile == NULL )
    {
        docFileName.MakeAbsolute();
        msg = wxT( "Failed to create component document library file " ) +
            docFileName.GetFullPath();
        DisplayError( NULL, msg );
        return false;
    }

    char line[256];
    if( fprintf( docfile, "%s  Date: %s\n", DOCFILE_IDENT, DateAndTime( line ) ) < 0 )
    {
        fclose( docfile );
        return false;
    }

    bool success = true;

    for( LIB_ALIAS_MAP::iterator it=aliases.begin();  it!=aliases.end();  it++ )
    {
        if ( !(*it).second->SaveDoc( docfile ) )
            success = false;
    }

    if ( fprintf( docfile, "#\n#End Doc Library\n" ) < 0 )
        success = false;

    fclose( docfile );

    return success;
}


bool CMP_LIBRARY::SaveHeader( FILE* aFile )
{
    char BufLine[1024];
    bool succes = true;

    DateAndTime( BufLine );
    if( fprintf( aFile, "%s %d.%d  Date: %s\n", LIBFILE_IDENT,
                 LIB_VERSION_MAJOR, LIB_VERSION_MINOR, BufLine ) < 0 )
        succes = false;

    if( fprintf( aFile, "#encoding utf-8\n") < 0 )
        succes = false;

#if 0
    if( ( fprintf( aFile, "$HEADER\n" ) < 0 )
        || ( fprintf( aFile, "TimeStamp %8.8lX\n", m_TimeStamp ) < 0 )
        || ( fprintf( aFile, "Parts %d\n", aliases.size() ) != 2 )
        || ( fprintf( aFile, "$ENDHEADER\n" ) != 1 ) )
        succes = false;
#endif
    return succes;
}


/*
 * The static library list and list management methods.
 */
CMP_LIBRARY_LIST CMP_LIBRARY::libraryList;
wxArrayString CMP_LIBRARY::libraryListSortOrder;


CMP_LIBRARY* CMP_LIBRARY::LoadLibrary( const wxFileName& aFileName, wxString& aErrorMsg )
{
    CMP_LIBRARY* lib = NULL;

    lib = new CMP_LIBRARY( LIBRARY_TYPE_EESCHEMA, aFileName );

    wxBusyCursor ShowWait;

    if( !lib->Load( aErrorMsg ) )
    {
        delete lib;
        return NULL;
    }

    if( USE_OLD_DOC_FILE_FORMAT( lib->versionMajor, lib->versionMinor ) )
        lib->LoadDocs( aErrorMsg );

    return lib;
}


bool CMP_LIBRARY::AddLibrary( const wxFileName& aFileName, wxString& aErrorMsg )
{
    CMP_LIBRARY* lib;

    /* Don't reload the library if it is already loaded. */
    lib = FindLibrary( aFileName.GetName() );

    if( lib != NULL )
        return true;

    lib = LoadLibrary( aFileName, aErrorMsg );

    if( lib == NULL )
        return false;

    libraryList.push_back( lib );

    return true;
}


bool CMP_LIBRARY::AddLibrary( const wxFileName& aFileName, wxString& aErrorMsg,
                              CMP_LIBRARY_LIST::iterator& aIterator )
{
    CMP_LIBRARY* lib;

    /* Don't reload the library if it is already loaded. */
    lib = FindLibrary( aFileName.GetName() );

    if( lib != NULL )
        return true;

    lib = LoadLibrary( aFileName, aErrorMsg );

    if( lib == NULL )
        return false;

    if( aIterator >= libraryList.begin() && aIterator < libraryList.end() )
        libraryList.insert( aIterator, lib );
    else
        libraryList.push_back( lib );

    return true;
}


void CMP_LIBRARY::RemoveLibrary( const wxString& aName )
{
    if( aName.IsEmpty() )
        return;

    CMP_LIBRARY_LIST::iterator i;

    for ( i = libraryList.begin(); i < libraryList.end(); i++ )
    {
        if( i->GetName().CmpNoCase( aName ) == 0 )
        {
            CMP_LIBRARY::libraryList.erase( i );
            return;
        }
    }
}


 /**
 * Test for an existing library.
 * @param aLibptr - aLibptr.
 * @return true found.  false if not found.
 */
bool CMP_LIBRARY::LibraryExists( const CMP_LIBRARY* aLibptr )
{
    BOOST_FOREACH( CMP_LIBRARY& lib, libraryList )
    {
        if( &lib == aLibptr )
            return true;
    }

    return false;
}

CMP_LIBRARY* CMP_LIBRARY::FindLibrary( const wxString& aName )
{
    BOOST_FOREACH( CMP_LIBRARY& lib, libraryList )
    {
        if( lib == aName )
            return &lib;
    }

    return NULL;
}


wxArrayString CMP_LIBRARY::GetLibraryNames( bool aSorted )
{
    wxString cacheName;
    wxArrayString names;

    BOOST_FOREACH( CMP_LIBRARY& lib, CMP_LIBRARY::libraryList )
    {
        if( lib.isCache && aSorted )
            cacheName = lib.GetName();
        else
            names.Add( lib.GetName() );
    }

    /* Even sorted, the cache library is always at the end of the list. */
    if( aSorted )
        names.Sort();

    if( !cacheName.IsEmpty() )
        names.Add( cacheName );

    return names;
}


LIB_COMPONENT* CMP_LIBRARY::FindLibraryComponent( const wxString& aName,
                                                  const wxString& aLibraryName )
{
    LIB_COMPONENT* component = NULL;

    BOOST_FOREACH( CMP_LIBRARY& lib, libraryList )
    {
        if( !aLibraryName.IsEmpty() && lib.GetName() != aLibraryName )
            continue;

        component = lib.FindComponent( aName );

        if( component != NULL )
            break;
    }

    return component;
}


LIB_ALIAS* CMP_LIBRARY::FindLibraryEntry( const wxString& aName, const wxString& aLibraryName )
{
    LIB_ALIAS* entry = NULL;

    BOOST_FOREACH( CMP_LIBRARY& lib, libraryList )
    {
        if( !aLibraryName.IsEmpty() && lib.GetName() != aLibraryName )
            continue;

        entry = lib.FindEntry( aName );

        if( entry != NULL )
            break;
    }

    return entry;
}


void CMP_LIBRARY::RemoveCacheLibrary()
{
    CMP_LIBRARY_LIST::iterator i;

    for ( i = libraryList.begin(); i < libraryList.end(); i++ )
    {
        if( i->isCache )
            libraryList.erase( i-- );
    }
}
