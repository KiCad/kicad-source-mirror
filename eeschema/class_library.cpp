/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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
#include <kiface_i.h>
#include <gr_basic.h>
#include <macros.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <eda_doc.h>
#include <wxstruct.h>
#include <richio.h>
#include <config_params.h>
#include <wildcards_and_files_ext.h>
#include <project_rescue.h>

#include <general.h>
#include <class_library.h>

#include <boost/foreach.hpp>

#include <wx/tokenzr.h>
#include <wx/regex.h>

#define duplicate_name_msg  \
    _(  "Library '%s' has duplicate entry name '%s'.\n" \
        "This may cause some unexpected behavior when loading components into a schematic." )


PART_LIB::PART_LIB( int aType, const wxString& aFileName ) :
    // start @ != 0 so each additional library added
    // is immediately detectable, zero would not be.
    m_mod_hash( PART_LIBS::s_modify_generation )
{
    type = aType;
    isModified = false;
    timeStamp = 0;
    isCache = false;
    timeStamp = wxDateTime::Now();
    versionMajor = 0;       // Will be updated after reading the lib file
    versionMinor = 0;       // Will be updated after reading the lib file

    fileName = aFileName;

    if( !fileName.IsOk() )
        fileName = wxT( "unnamed.lib" );
}


PART_LIB::~PART_LIB()
{
    // When the library is destroyed, all of the alias objects on the heap should be deleted.
    for( LIB_ALIAS_MAP::iterator it = m_amap.begin();  it != m_amap.end();  ++it )
    {
        wxLogTrace( traceSchLibMem, wxT( "Removing alias %s from library %s." ),
                    GetChars( it->second->GetName() ), GetChars( GetLogicalName() ) );
        LIB_PART* part = it->second->GetPart();
        LIB_ALIAS* alias = it->second;
        delete alias;

        // When the last alias of a part is destroyed, the part is no longer required and it
        // too is destroyed.
        if( part && part->GetAliasCount() == 0 )
            delete part;
    }

    m_amap.clear();
}


void PART_LIB::GetEntryNames( wxArrayString& aNames, bool aSort, bool aMakeUpperCase )
{
    for( LIB_ALIAS_MAP::iterator it = m_amap.begin();  it!=m_amap.end();  it++ )
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


void PART_LIB::GetEntryTypePowerNames( wxArrayString& aNames, bool aSort, bool aMakeUpperCase )
{
    for( LIB_ALIAS_MAP::iterator it = m_amap.begin();  it!=m_amap.end();  it++ )
    {
        LIB_ALIAS* alias = it->second;
        LIB_PART* root = alias->GetPart();

        if( !root || !root->IsPower() )
            continue;

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


void PART_LIB::SearchEntryNames( std::vector<wxArrayString>& aNames,
                                    const wxString& aNameSearch,
                                    const wxString& aKeySearch,
                                    bool aSort )
{
    for( LIB_ALIAS_MAP::iterator it = m_amap.begin();  it != m_amap.end();  ++it )
    {
        if( !!aKeySearch && KeyWordOk( aKeySearch, it->second->GetKeyWords() ) )
        {
            wxArrayString item;

            item.Add( it->first );
            item.Add( GetLogicalName() );
            aNames.push_back( item );
        }

        if( !aNameSearch.IsEmpty() &&
                WildCompareString( aNameSearch, it->second->GetName(), false ) )
        {
            wxArrayString item;

            item.Add( it->first );
            item.Add( GetLogicalName() );
            aNames.push_back( item );
        }
    }

    if( aSort )
        std::sort( aNames.begin(), aNames.end(), sortFunction );
}


void PART_LIB::SearchEntryNames( wxArrayString& aNames, const wxRegEx& aRe, bool aSort )
{
    if( !aRe.IsValid() )
        return;

    LIB_ALIAS_MAP::iterator it;

    for( it = m_amap.begin();  it!=m_amap.end();  it++ )
    {
        if( aRe.Matches( it->second->GetKeyWords() ) )
            aNames.Add( it->first );
    }

    if( aSort )
        aNames.Sort();
}


bool PART_LIB::Conflicts( LIB_PART* aPart )
{
    wxCHECK_MSG( aPart != NULL, false,
                 wxT( "Cannot test NULL component for conflicts in library " ) + GetName() );

    for( size_t i=0;  i<aPart->m_aliases.size();  i++ )
    {
        LIB_ALIAS_MAP::iterator it = m_amap.find( aPart->m_aliases[i]->GetName() );

        if( it != m_amap.end() )
            return true;
    }

    return false;
}


LIB_ALIAS* PART_LIB::FindEntry( const wxString& aName )
{
    LIB_ALIAS_MAP::iterator it = m_amap.find( aName );

    if( it != m_amap.end() )
        return it->second;

    return NULL;
}


LIB_ALIAS* PART_LIB::GetFirstEntry()
{
    if( m_amap.size() )
        return m_amap.begin()->second;
    else
        return NULL;
}


LIB_PART* PART_LIB::FindPart( const wxString& aName )
{
#if 0 && defined(DEBUG)
    if( !aName.Cmp( wxT( "TI_STELLARIS_BOOSTERPACK" ) ) )
    {
        int breakhere = 1;
        (void) breakhere;
    }
#endif

    if( LIB_ALIAS* alias = FindEntry( aName ) )
    {
        return alias->GetPart();
    }

    return NULL;
}


bool PART_LIB::HasPowerParts()
{
    // return true if at least one power part is found in lib
    for( LIB_ALIAS_MAP::iterator it = m_amap.begin();  it!=m_amap.end();  it++ )
    {
        LIB_ALIAS* alias = it->second;
        LIB_PART* root = alias->GetPart();

        if( root && root->IsPower() )
            return true;
    }

    return false;
}


bool PART_LIB::AddAlias( LIB_ALIAS* aAlias )
{
    wxASSERT( aAlias );

#if defined(DEBUG) && 0
    if( !aAlias->GetName().Cmp( wxT( "TI_STELLARIS_BOOSTERPACK" ) ) )
    {
        int breakhere = 1;
        (void) breakhere;
    }
#endif

    LIB_ALIAS_MAP::iterator it = m_amap.find( aAlias->GetName() );

    if( it != m_amap.end() )
    {
        wxString msg;

        msg.Printf( _( "Cannot add duplicate alias '%s' to library '%s'." ),
                    GetChars( aAlias->GetName() ),
                    GetChars( fileName.GetName() ) );
        return false;
    }

    wxString name = aAlias->GetName();

    m_amap[ name ] = aAlias;
    isModified = true;
    ++m_mod_hash;

    return true;
}


bool PART_LIB::AddPart( LIB_PART* aPart )
{
    // Conflict detection: See if already existing aliases exist,
    // and if yes, ask user for continue or abort
    // Special case: if the library is the library cache of the project,
    // old aliases are always removed to avoid conflict,
    //      and user is not prompted )
    if( Conflicts( aPart ) && !IsCache() )
    {
        wxFAIL_MSG( wxT( "Cannot add component <" ) + aPart->GetName() +
                    wxT( "> to library <" ) + GetName() + wxT( "> due to name conflict." ) );
        return false;
    }

    // add a clone, not the caller's copy
    LIB_PART* my_part = new LIB_PART( *aPart, this );

    for( size_t i = 0; i < my_part->m_aliases.size(); i++ )
    {
        wxString aliasname = my_part->m_aliases[i]->GetName();

        if( LIB_ALIAS* alias = FindAlias( aliasname ) )
            RemoveEntry( alias );

        m_amap[ aliasname ] = my_part->m_aliases[i];
    }

    isModified = true;
    ++m_mod_hash;

    return true;
}


LIB_ALIAS* PART_LIB::RemoveEntry( LIB_ALIAS* aEntry )
{
    wxCHECK_MSG( aEntry != NULL, NULL, wxT( "NULL pointer cannot be removed from library." ) );

    LIB_ALIAS_MAP::iterator it = m_amap.find( aEntry->GetName() );

    if( it == m_amap.end() )
        return NULL;

    // If the entry pointer doesn't match the name it is mapped to in the library, we
    // have done something terribly wrong.
    wxCHECK_MSG( *it->second == aEntry, NULL,
                 wxT( "Pointer mismatch while attempting to remove entry <" ) +
                 aEntry->GetName() + wxT( "> from library <" ) + GetName() + wxT( ">." ) );

    LIB_ALIAS*  alias = aEntry;
    LIB_PART*   part = alias->GetPart();

    alias = part->RemoveAlias( alias );

    if( !alias )
    {
        delete part;

        if( m_amap.size() > 1 )
        {
            LIB_ALIAS_MAP::iterator next = it;
            next++;

            if( next == m_amap.end() )
                next = m_amap.begin();

            alias = next->second;
        }
    }

    m_amap.erase( it );
    isModified = true;
    ++m_mod_hash;
    return alias;
}


LIB_PART* PART_LIB::ReplacePart( LIB_PART* aOldPart, LIB_PART* aNewPart )
{
    wxASSERT( aOldPart != NULL );
    wxASSERT( aNewPart != NULL );

    /* Remove the old root component.  The component will automatically be deleted
     * when all it's aliases are deleted.  Do not place any code that accesses
     * aOldPart inside this loop that gets evaluated after the last alias is
     * removed in RemoveEntry().  Failure to heed this warning will result in a
     * segfault.
     */
    size_t i = aOldPart->m_aliases.size();

    while( i > 0 )
    {
        i -= 1;
        RemoveEntry( aOldPart->m_aliases[ i ] );
    }

    LIB_PART* my_part = new LIB_PART( *aNewPart, this );

    // Add new aliases to library alias map.
    for( i = 0; i < my_part->m_aliases.size(); i++ )
    {
        wxString aname = my_part->m_aliases[ i ]->GetName();
        m_amap[ aname ] = my_part->m_aliases[ i ];
    }

    isModified = true;
    ++m_mod_hash;
    return my_part;
}


LIB_ALIAS* PART_LIB::GetNextEntry( const wxString& aName )
{
    if( m_amap.empty() )
        return NULL;

    LIB_ALIAS_MAP::iterator it = m_amap.find( aName );

    it++;

    if( it == m_amap.end() )
        it = m_amap.begin();

    return it->second;
}


LIB_ALIAS* PART_LIB::GetPreviousEntry( const wxString& aName )
{
    if( m_amap.empty() )
        return NULL;

    LIB_ALIAS_MAP::iterator it = m_amap.find( aName );

    if( it == m_amap.begin() )
        it = m_amap.end();

    it--;

    return it->second;
}


bool PART_LIB::Load( wxString& aErrorMsg )
{
    FILE*          file;
    char*          line;
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

    // There is no header if this is a symbol library.
    if( type == LIBRARY_TYPE_EESCHEMA )
    {
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
#if 0       // Note for developers:
            // Not sure this warning is very useful: old designs *must* be always loadable
            wxLogWarning( wxT(
                "The component library '%s' header version "
                "number is invalid.\n\nIn future versions of Eeschema this library may not "
                "load correctly.  To resolve this problem open the library in the library "
                "editor and save it.  If this library is the project cache library, save "
                "the current schematic." ),
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
            // Read one DEF/ENDDEF part entry from library:
            LIB_PART* part = new LIB_PART( wxEmptyString, this );

            if( part->Load( reader, msg ) )
            {
                // Check for duplicate entry names and warn the user about
                // the potential conflict.
                if( FindEntry( part->GetName() ) != NULL )
                {
                    wxString msg = duplicate_name_msg;

                    wxLogWarning( msg,
                                  GetChars( fileName.GetName() ),
                                  GetChars( part->GetName() ) );
                }

                LoadAliases( part );
            }
            else
            {
                wxLogWarning( _( "Library '%s' component load error %s." ),
                              GetChars( fileName.GetName() ),
                              GetChars( msg ) );
                msg.Clear();
                delete part;
            }
        }
    }

    ++m_mod_hash;

    return true;
}


void PART_LIB::LoadAliases( LIB_PART* aPart )
{
    wxCHECK_RET( aPart, wxT( "Cannot load aliases of NULL part.  Bad programmer!" ) );

    for( size_t i = 0; i < aPart->m_aliases.size(); i++ )
    {
        if( FindEntry( aPart->m_aliases[i]->GetName() ) != NULL )
        {
            wxString msg = duplicate_name_msg;

            wxLogError( msg,
                        GetChars( fileName.GetName() ),
                        GetChars( aPart->m_aliases[i]->GetName() ) );
        }

        wxString aname = aPart->m_aliases[i]->GetName();
        m_amap[ aname ] = aPart->m_aliases[i];
    }
}


bool PART_LIB::LoadHeader( LINE_READER& aLineReader )
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


bool PART_LIB::LoadDocs( wxString& aErrorMsg )
{
    int        lineNumber = 0;
    char       line[8000], * name, * text;
    LIB_ALIAS* entry;
    FILE*      file;
    wxFileName fn = fileName;

    fn.SetExt( DOC_EXT );

    file = wxFopen( fn.GetFullPath(), wxT( "rt" ) );

    if( file == NULL )
    {
        aErrorMsg.Printf( _( "Could not open component document library file '%s'." ),
                          GetChars( fn.GetFullPath() ) );
        return false;
    }

    if( GetLine( file, line, &lineNumber, sizeof(line) ) == NULL )
    {
        aErrorMsg.Printf( _( "Part document library file '%s' is empty." ),
                          GetChars( fn.GetFullPath() ) );
        fclose( file );
        return false;
    }

    if( strnicmp( line, DOCFILE_IDENT, 10 ) != 0 )
    {
        aErrorMsg.Printf( _( "File '%s' is not a valid component library document file." ),
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

        // Read one $CMP/$ENDCMP part entry from library:
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


bool PART_LIB::Save( OUTPUTFORMATTER& aFormatter )
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

        for( LIB_ALIAS_MAP::iterator it=m_amap.begin();  it!=m_amap.end();  it++ )
        {
            if( !it->second->IsRoot() )
                continue;

            it->second->GetPart()->Save( aFormatter );
        }

        aFormatter.Print( 0, "#\n#End Library\n" );
    }
    catch( const IO_ERROR& ioe )
    {
        success = false;
    }

    return success;
}


bool PART_LIB::SaveDocs( OUTPUTFORMATTER& aFormatter )
{
    bool success = true;

    try
    {
        aFormatter.Print( 0, "%s\n", DOCFILE_IDENT );

        for( LIB_ALIAS_MAP::iterator it=m_amap.begin();  it!=m_amap.end();  it++ )
        {
            if( !it->second->SaveDoc( aFormatter ) )
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


bool PART_LIB::SaveHeader( OUTPUTFORMATTER& aFormatter )
{
    aFormatter.Print( 0, "%s %d.%d\n", LIBFILE_IDENT,
                      LIB_VERSION_MAJOR, LIB_VERSION_MINOR );

    aFormatter.Print( 0, "#encoding utf-8\n");

#if 0
    aFormatter.Print( 0, "$HEADER\n" );
    aFormatter.Print( 0, "TimeStamp %8.8lX\n", m_TimeStamp );
    aFormatter.Print( 0, "Parts %d\n", m_amap.size() );
    aFormatter.Print( 0, "$ENDHEADER\n" ) != 1 );
#endif

    return true;
}


PART_LIB* PART_LIB::LoadLibrary( const wxString& aFileName ) throw( IO_ERROR, boost::bad_pointer )
{
    std::auto_ptr<PART_LIB> lib( new PART_LIB( LIBRARY_TYPE_EESCHEMA, aFileName ) );

    wxBusyCursor ShowWait;

    wxString errorMsg;

    if( !lib->Load( errorMsg ) )
        THROW_IO_ERROR( errorMsg );

    if( USE_OLD_DOC_FILE_FORMAT( lib->versionMajor, lib->versionMinor ) )
    {
#if 1
        // not fatal if error here.
        lib->LoadDocs( errorMsg );
#else
        if( !lib->LoadDocs( errorMsg ) )
            THROW_IO_ERROR( errorMsg );
#endif
    }

    PART_LIB* ret = lib.release();

    return ret;
}


PART_LIB* PART_LIBS::AddLibrary( const wxString& aFileName ) throw( IO_ERROR, boost::bad_pointer )
{
    PART_LIB* lib;

#if 1
    wxFileName fn = aFileName;
    // Don't reload the library if it is already loaded.
    lib = FindLibrary( fn.GetName() );
    if( lib )
        return lib;
#endif

    lib = PART_LIB::LoadLibrary( aFileName );

    push_back( lib );

    return lib;
}


PART_LIB* PART_LIBS::AddLibrary( const wxString& aFileName, PART_LIBS::iterator& aIterator )
    throw( IO_ERROR, boost::bad_pointer )
{
#if 1
    // Don't reload the library if it is already loaded.
    wxFileName fn( aFileName );
    PART_LIB* lib = FindLibrary( fn.GetName() );

    if( lib )
        return lib;
#endif

    lib = PART_LIB::LoadLibrary( aFileName );

    if( aIterator >= begin() && aIterator < end() )
        insert( aIterator, lib );
    else
        push_back( lib );

    return lib;
}


void PART_LIBS::RemoveLibrary( const wxString& aName )
{
    if( aName.IsEmpty() )
        return;

    for( PART_LIBS::iterator it = begin(); it < end();  ++it )
    {
        if( it->GetName().CmpNoCase( aName ) == 0 )
        {
            erase( it );
            return;
        }
    }
}


PART_LIB* PART_LIBS::FindLibrary( const wxString& aName )
{
    for( PART_LIBS::iterator it = begin();  it!=end();  ++it )
    {
        if( it->GetName() == aName )
            return &*it;
    }

    return NULL;
}


wxArrayString PART_LIBS::GetLibraryNames( bool aSorted )
{
    wxArrayString cacheNames;
    wxArrayString names;

    BOOST_FOREACH( PART_LIB& lib, *this )
    {
        if( lib.IsCache() && aSorted )
            cacheNames.Add( lib.GetName() );
        else
            names.Add( lib.GetName() );
    }

    // Even sorted, the cache library is always at the end of the list.
    if( aSorted )
        names.Sort();

    for( unsigned int i = 0; i<cacheNames.Count(); i++ )
        names.Add( cacheNames.Item( i ) );

    return names;
}


LIB_PART* PART_LIBS::FindLibPart( const wxString& aPartName, const wxString& aLibraryName )
{
    LIB_PART* part = NULL;

    BOOST_FOREACH( PART_LIB& lib, *this )
    {
        if( !aLibraryName.IsEmpty() && lib.GetName() != aLibraryName )
            continue;

        part = lib.FindPart( aPartName );

        if( part )
            break;
    }

    return part;
}


LIB_ALIAS* PART_LIBS::FindLibraryEntry( const wxString& aEntryName, const wxString& aLibraryName )
{
    LIB_ALIAS* entry = NULL;

    BOOST_FOREACH( PART_LIB& lib, *this )
    {
        if( !!aLibraryName && lib.GetName() != aLibraryName )
            continue;

        entry = lib.FindEntry( aEntryName );

        if( entry )
            break;
    }

    return entry;
}

void PART_LIBS::FindLibraryEntries( const wxString& aEntryName, std::vector<LIB_ALIAS*>& aEntries )
{
    BOOST_FOREACH( PART_LIB& lib, *this )
    {
        LIB_ALIAS* entry = lib.FindEntry( aEntryName );

        if( entry )
            aEntries.push_back( entry );
    }
}

/* searches all libraries in the list for an entry, using a case insensitive comparison.
 * Used to find an entry, when the normal (case sensitive) search fails.
  */
void PART_LIBS::FindLibraryNearEntries( std::vector<LIB_ALIAS*>& aCandidates,
                                        const wxString& aEntryName,
                                        const wxString& aLibraryName )
{
    BOOST_FOREACH( PART_LIB& lib, *this )
    {
        if( !!aLibraryName && lib.GetName() != aLibraryName )
            continue;

        LIB_ALIAS* entry = lib.GetFirstEntry();

        if( ! entry )
            continue;

        wxString first_entry_name = entry->GetName();
        wxString entry_name = first_entry_name;

        for( ;; )
        {
            if( entry_name.CmpNoCase( aEntryName ) == 0 )
                aCandidates.push_back( entry );

            entry = lib.GetNextEntry( entry_name );
            entry_name = entry->GetName();

            if( first_entry_name == entry_name )
                break;
        }
    }
}


int PART_LIBS::s_modify_generation = 1;     // starts at 1 and goes up


int PART_LIBS::GetModifyHash()
{
    int hash = 0;

    for( PART_LIBS::const_iterator it = begin();  it != end();  ++it )
    {
        hash += it->m_mod_hash;
    }

    return hash;
}


/*
void PART_LIBS::RemoveCacheLibrary()
{
    for( PART_LIBS::iterator it = begin(); it < end();  ++it )
    {
        if( it->IsCache() )
            erase( it-- );
    }
}
*/


void PART_LIBS::LibNamesAndPaths( PROJECT* aProject, bool doSave,
                                  wxString* aPaths, wxArrayString* aNames )
    throw( IO_ERROR, boost::bad_pointer )
{
    wxString pro = aProject->GetProjectFullName();

    PARAM_CFG_ARRAY ca;

    if( aPaths )
        ca.push_back( new PARAM_CFG_FILENAME( wxT( "LibDir" ), aPaths ) );

    if( aNames )
        ca.push_back( new PARAM_CFG_LIBNAME_LIST( wxT( "LibName" ),  aNames, GROUP_SCH_LIBS ) );

    if( doSave )
    {
        aProject->ConfigSave( Kiface().KifaceSearch(), GROUP_SCH, ca );

        /*
        {
            wxString msg = wxString::Format( _(
                "Unable save project's '%s' file" ),
                GetChars( pro )
                );
            THROW_IO_ERROR( msg );
        }
        */
    }
    else
    {
        if( !aProject->ConfigLoad( Kiface().KifaceSearch(), GROUP_SCH, ca ) )
        {
            wxString msg = wxString::Format( _(
                "Unable to load project's '%s' file" ),
                GetChars( pro )
                );
            THROW_IO_ERROR( msg );
        }
    }
}


const wxString PART_LIBS::CacheName( const wxString& aFullProjectFilename )
{
    /* until apr 2009 the project cache lib was named: <root_name>.cache.lib,
     * and after: <root_name>-cache.lib.  So if the <name>-cache.lib is not found,
     * the old file will be renamed and returned.
     */
    wxFileName  new_name = aFullProjectFilename;

    new_name.SetName( new_name.GetName() + wxT( "-cache" ) );
    new_name.SetExt( SchematicLibraryFileExtension );

    if( new_name.FileExists() )
        return new_name.GetFullPath();
    else
    {
        wxFileName old_name = aFullProjectFilename;
        old_name.SetExt( wxT( "cache.lib" ) );

        if( old_name.FileExists() )
        {
            wxRenameFile( old_name.GetFullPath(), new_name.GetFullPath() );
            return new_name.GetFullPath();
        }
    }
    return wxEmptyString;
}


void PART_LIBS::LoadAllLibraries( PROJECT* aProject ) throw( IO_ERROR, boost::bad_pointer )
{
    wxFileName      fn;
    wxString        filename;
    wxString        libs_not_found;
    SEARCH_STACK*   lib_search = aProject->SchSearchS();

#if defined(DEBUG) && 1
    lib_search->Show( __func__ );
#endif

    wxArrayString   lib_names;

    LibNamesAndPaths( aProject, false, NULL, &lib_names );

    // If the list is empty, force loading the standard power symbol library.
    if( !lib_names.GetCount() )
        lib_names.Add( wxT( "power" ) );

    wxASSERT( !size() );    // expect to load into "this" empty container.

    for( unsigned i = 0; i < lib_names.GetCount();  ++i )
    {
        fn.Clear();
        fn.SetName( lib_names[i] );
        fn.SetExt( SchematicLibraryFileExtension );

        // Skip if the file name is not valid..
        if( !fn.IsOk() )
            continue;

        if( !fn.FileExists() )
        {
            filename = lib_search->FindValidPath( fn.GetFullPath() );

            if( !filename )
            {
                libs_not_found += fn.GetName();
                libs_not_found += wxT( '\n' );
                continue;
            }
        }
        else
        {
            filename = fn.GetFullPath();
        }

        try
        {
            AddLibrary( filename );
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg = wxString::Format( _(
                    "Part library '%s' failed to load. Error:\n"
                    "%s" ),
                    GetChars( filename ),
                    GetChars( ioe.errorText )
                    );

            THROW_IO_ERROR( msg );
        }
    }

    // add the special cache library.
    wxString cache_name = CacheName( aProject->GetProjectFullName() );
    PART_LIB* cache_lib;

    if( !!cache_name )
    {
        try
        {
            cache_lib = AddLibrary( cache_name );
            if( cache_lib )
                cache_lib->SetCache();
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg = wxString::Format( _(
                    "Part library '%s' failed to load.\nError: %s" ),
                    GetChars( cache_name ),
                    GetChars( ioe.errorText )
                    );

            THROW_IO_ERROR( msg );
        }
    }

    // Print the libraries not found
    if( !!libs_not_found )
    {
        // Use a different exception type so catch()er can route to proper use
        // of the HTML_MESSAGE_BOX.
        THROW_PARSE_ERROR( wxEmptyString, UTF8( __func__ ),
            UTF8( libs_not_found ), 0, 0 );
    }

#if defined(DEBUG) && 1
    printf( "%s: lib_names:\n", __func__ );

    for( PART_LIBS::const_iterator it = begin(); it < end(); ++it )
        printf( " %s\n", TO_UTF8( it->GetName() ) );
#endif
}
