/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file class_library.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <eda_doc.h>
#include <wxstruct.h>
#include <richio.h>

#include <general.h>
#include <class_library.h>

#include <boost/foreach.hpp>

#include <wx/tokenzr.h>
#include <wx/regex.h>

static const wxString duplicate_name_msg =
    _(  "Library '%s' has duplicate entry name '%s'.\n"
        "This may cause some unexpected behavior when loading components into a schematic." );


bool operator==( const CMP_LIBRARY& aLibrary, const wxString& aName )
{
    // See our header class_libentry.h for function Cmp_KEEPCASE().
    return Cmp_KEEPCASE( aLibrary.GetName(), aName ) == 0;
}


bool operator!=( const CMP_LIBRARY& aLibrary, const wxString& aName )
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

    // If the sort order array isn't set, then sort alphabetically except.
    if( CMP_LIBRARY::GetSortOrder().IsEmpty() )
        return Cmp_KEEPCASE( aItem1.GetName(), aItem2.GetName() ) < 0;

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
        LIB_ALIAS*      alias = (*it).second;
        LIB_COMPONENT*  component = alias->GetComponent();

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


/**
 * Function sortFunction
 * simple function used as comparator to sort a std::vector<wxArrayString>&.
 *
 * @param aItem1 is the first comparison parameter.
 * @param aItem2 is the second.
 * @return bool - which item should be put first in the sorted list.
 */
bool sortFunction( wxArrayString aItem1, wxArrayString aItem2 )
{
    return( aItem1.Item( 0 ) < aItem2.Item( 0 ) );
}


void CMP_LIBRARY::SearchEntryNames( std::vector<wxArrayString>& aNames,
                                    const wxString& aNameSearch,
                                    const wxString& aKeySearch,
                                    bool aSort )
{
    LIB_ALIAS_MAP::iterator it;

    for( it = aliases.begin();  it!=aliases.end();  it++ )
    {
        if( !aKeySearch.IsEmpty() && KeyWordOk( aKeySearch, (*it).second->GetKeyWords() ) )
        {
            wxArrayString item;
            item.Add( (*it).first );
            item.Add( GetLogicalName() );
            aNames.push_back( item );
        }

        if( !aNameSearch.IsEmpty() && WildCompareString( aNameSearch,
                                                         (*it).second->GetName(), false ) )
        {
            wxArrayString item;
            item.Add( (*it).first );
            item.Add( GetLogicalName() );
            aNames.push_back( item );
        }
    }

    if( aSort )
        std::sort( aNames.begin(), aNames.end(), sortFunction );
}


void CMP_LIBRARY::SearchEntryNames( wxArrayString& aNames, const wxRegEx& aRe, bool aSort )
{
    if( !aRe.IsValid() )
        return;

    LIB_ALIAS_MAP::iterator it;

    for( it = aliases.begin();  it!=aliases.end();  it++ )
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


LIB_ALIAS* CMP_LIBRARY::FindEntry( const wxString& aName )
{
    LIB_ALIAS_MAP::iterator it = aliases.find( aName );

    if( it != aliases.end() )
        return (*it).second;

    return NULL;
}


LIB_ALIAS* CMP_LIBRARY::GetFirstEntry()
{
    if( aliases.size() )
        return (*aliases.begin()).second;
    else
        return NULL;
}


LIB_COMPONENT* CMP_LIBRARY::FindComponent( const wxString& aName )
{

#if 0 && defined(DEBUG)
    if( !aName.Cmp( wxT( "TI_STELLARIS_BOOSTERPACK" ) ) )
    {
        int breakhere = 1;
        (void) breakhere;
    }
#endif

    LIB_COMPONENT*  component = NULL;
    LIB_ALIAS*      entry = FindEntry( aName );

    if( entry )
        component = entry->GetComponent();

    return component;
}


bool CMP_LIBRARY::AddAlias( LIB_ALIAS* aAlias )
{
    wxASSERT( aAlias );

#if 0 && defined(DEBUG)
    if( !aAlias->GetName().Cmp( wxT( "TI_STELLARIS_BOOSTERPACK" ) ) )
    {
        int breakhere = 1;
        (void) breakhere;
    }
#endif

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
    if( !aComponent )
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
    // have done something terribly wrong.
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


LIB_ALIAS* CMP_LIBRARY::GetNextEntry( const wxString& aName )
{
    if( aliases.empty() )
        return NULL;

    LIB_ALIAS_MAP::iterator it = aliases.find( aName );

    it++;

    if( it == aliases.end() )
        it = aliases.begin();

    return (*it).second;
}


LIB_ALIAS* CMP_LIBRARY::GetPreviousEntry( const wxString& aName )
{
    if( aliases.empty() )
        return NULL;

    LIB_ALIAS_MAP::iterator it = aliases.find( aName );

    if( it == aliases.begin() )
        it = aliases.end();

    it--;

    return (*it).second;
}


bool CMP_LIBRARY::Load( wxString& aErrorMsg )
{
    FILE*          file;
    char*          line;
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

    FILE_LINE_READER reader( file, fileName.GetFullPath() );

    if( !reader.ReadLine() )
    {
        aErrorMsg = _( "The file is empty!" );
        return false;
    }

    /* There is no header if this is a symbol library. */
    if( type == LIBRARY_TYPE_EESCHEMA )
    {
        wxString tmp;

        line = reader.Line();

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
            aErrorMsg = _( "The file is NOT an Eeschema library!" );
            return false;
        }

        if( !tkn.HasMoreTokens() )
        {
            aErrorMsg = _( "The file header is missing version and time stamp information." );
            return false;
        }

        if( tkn.GetNextToken() != wxT( "Version" ) || !tkn.HasMoreTokens() )
        {
            aErrorMsg = wxT( "The file header version information is invalid." );
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
        }
    }

    while( reader.ReadLine() )
    {
        line = reader.Line();

        if( type == LIBRARY_TYPE_EESCHEMA && strnicmp( line, "$HEADER", 7 ) == 0 )
        {
            if( !LoadHeader( reader ) )
            {
                aErrorMsg = _( "An error occurred attempting to read the header." );
                return false;
            }

            continue;
        }

        if( strnicmp( line, "DEF", 3 ) == 0 )
        {
            /* Read one DEF/ENDDEF part entry from library: */
            libEntry = new LIB_COMPONENT( wxEmptyString, this );

            if( libEntry->Load( reader, msg ) )
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


bool CMP_LIBRARY::LoadHeader( LINE_READER& aLineReader )
{
    char* line, * text, * data;

    while( aLineReader.ReadLine() )
    {
        line = (char*) aLineReader;

        text = strtok( line, " \t\r\n" );
        data = strtok( NULL, " \t\r\n" );

        if( stricmp( text, "TimeStamp" ) == 0 )
            timeStamp = atol( data );

        if( stricmp( text, "$ENDHEADER" ) == 0 )
            return true;
    }

    return false;
}


bool CMP_LIBRARY::LoadDocs( wxString& aErrorMsg )
{
    int        lineNumber = 0;
    char       line[8000], * name, * text;
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


bool CMP_LIBRARY::Save( OUTPUTFORMATTER& aFormatter )
{
    if( isModified )
    {
        timeStamp = GetNewTimeStamp();
        isModified = false;
    }

    bool success = true;

    try
    {
        SaveHeader( aFormatter );

        for( LIB_ALIAS_MAP::iterator it=aliases.begin();  it!=aliases.end();  it++ )
        {
            if( !(*it).second->IsRoot() )
                continue;

            (*it).second->GetComponent()->Save( aFormatter );
        }

        aFormatter.Print( 0, "#\n#End Library\n" );
    }
    catch( const IO_ERROR& ioe )
    {
        success = false;
    }

    return success;
}


bool CMP_LIBRARY::SaveDocs( OUTPUTFORMATTER& aFormatter )
{
    bool success = true;

    try
    {
        aFormatter.Print( 0, "%s\n", DOCFILE_IDENT );

        for( LIB_ALIAS_MAP::iterator it=aliases.begin();  it!=aliases.end();  it++ )
        {
            if ( !(*it).second->SaveDoc( aFormatter ) )
                success = false;
        }

        aFormatter.Print( 0, "#\n#End Doc Library\n" );
    }
    catch( const IO_ERROR& ioe )
    {
        success = false;
    }

    return success;
}


bool CMP_LIBRARY::SaveHeader( OUTPUTFORMATTER& aFormatter )
{
    aFormatter.Print( 0, "%s %d.%d\n", LIBFILE_IDENT,
                      LIB_VERSION_MAJOR, LIB_VERSION_MINOR );

    aFormatter.Print( 0, "#encoding utf-8\n");

#if 0
    aFormatter.Print( 0, "$HEADER\n" );
    aFormatter.Print( 0, "TimeStamp %8.8lX\n", m_TimeStamp );
    aFormatter.Print( 0, "Parts %d\n", aliases.size() );
    aFormatter.Print( 0, "$ENDHEADER\n" ) != 1 );
#endif

    return true;
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

    for( i = libraryList.begin(); i < libraryList.end(); i++ )
    {
        if( i->GetName().CmpNoCase( aName ) == 0 )
        {
            CMP_LIBRARY::libraryList.erase( i );
            return;
        }
    }
}


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
    wxArrayString cacheNames;
    wxArrayString names;

    BOOST_FOREACH( CMP_LIBRARY& lib, CMP_LIBRARY::libraryList )
    {
        if( lib.isCache && aSorted )
            cacheNames.Add( lib.GetName() );
        else
            names.Add( lib.GetName() );
    }

    /* Even sorted, the cache library is always at the end of the list. */
    if( aSorted )
        names.Sort();

    for( unsigned int i = 0; i<cacheNames.Count(); i++ )
        names.Add( cacheNames.Item( i ) );

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

    for( i = libraryList.begin(); i < libraryList.end(); i++ )
    {
        if( i->isCache )
            libraryList.erase( i-- );
    }
}
