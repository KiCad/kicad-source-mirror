/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, <dick@softplc.com>
 * Copyright (C) 2010 Kicad Developers, see change_log.txt for contributors.
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



/*  Note: this LIB_SOURCE implementation relies on the posix specified opendir() and
    related functions rather than wx functions which might do the same thing.  This
    is because I did not want to become very dependent on wxWidgets at such a low
    level as this, in case someday this code needs to be used on kde or whatever.

    Mingw and unix, linux, & osx will all have these posix functions.
    MS Visual Studio may need the posix compatible opendir() functions brought in
        http://www.softagalleria.net/dirent.php
    wx has these but they are based on wxString which can be wchar_t based and wx should
    not be introduced at a level this low.

    Part files: have the general form partname.part[.revN...]
    Categories: are any subdirectories immediately below the sourceURI, one level only.
    Part names: [category/]partname[/revN...]
*/


#include <sch_dir_lib_source.h>
using namespace SCH;

#include <kicad_exceptions.h>

#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdio>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include <vector>
using namespace std;

/// This file extension is an implementation detail specific to this LIB_SOURCE
/// implementation, and to a corresponding LIB_SINK.
/// Core EESCHEMA should never have to see this.
#define SWEET_EXT       ".part"


/* __func__ is C99 prescribed, but just in case:
http://www.informit.com/guides/content.aspx?g=cplusplus&seqNum=338
#if defined(__GNUG__)   // The GNU C++ compiler defines this
#define FUNC_NAME(x)    // nothing, GNU C++ defines __func__ just fine.
#else
#define FUNC_NAME(x)    static const char __func__[] = #x;
#endif
*/


/**
 * Class DIR_WRAP
 * provides a destructor which is invoked if an exception is thrown.
 */
class DIR_WRAP
{
    DIR*    dir;

public:
    DIR_WRAP( DIR* aDir ) : dir( aDir ) {}

    ~DIR_WRAP()
    {
        if( dir )
            closedir( dir );
    }

    DIR* operator->()   { return dir; }
    DIR* operator*()    { return dir; }
    operator bool ()    { return dir!=0; }
};


/**
 * Class FILE_WRAP
 * provides a destructor which is invoked if an exception is thrown.
 */
class FILE_WRAP
{
    int     fh;

public:
    FILE_WRAP( int aFileHandle ) : fh( aFileHandle ) {}
    ~FILE_WRAP()
    {
        if( fh != -1 )
            close( fh );
    }

    operator int ()  { return fh; }
};


/**
 * Function strrstr
 * finds the last instance of needle in haystack, if any.
 */
static const char* strrstr( const char* haystack, const char* needle )
{
    const char* ret = 0;
    const char* next = haystack;

    // find last instance of haystack
    while( (next = strstr( next, needle )) != 0 )
    {
        ret = next;
        ++next;     // don't keep finding the same one.
    }

    return ret;
}


/**
 * Function endsWithRev
 * returns a pointer to the final string segment: "revN[N..]" or NULL if none.
 * @param start is the beginning of string segment to test, the partname or
 *  any middle portion of it.
 * @param tail is a pointer to the terminating nul, or one past inclusive end of
 *  segment, i.e. the string segment of interest is [start,tail)
 * @param separator is the separating byte, expected: '.' or '/', depending on context.
 */
static const char* endsWithRev( const char* start, const char* tail, char separator )
{
    bool    sawDigit = false;

    while( tail > start && isdigit( *--tail ) )
    {
        sawDigit = true;
    }

    // if sawDigit, tail points to the 'v' here.

    if( sawDigit && tail-3 >= start )
    {
        tail -= 3;

        if( tail[0]==separator && tail[1]=='r' && tail[2]=='e' && tail[3]=='v' )
        {
            return tail+1;  // omit separator, return "revN[N..]"
        }
    }

    return 0;
}

static inline const char* endsWithRev( const STRING& aPartName, char separator )
{
    return endsWithRev( aPartName.c_str(),  aPartName.c_str()+aPartName.size(), separator );
}


// see struct BY_REV
bool BY_REV::operator() ( const STRING& s1, const STRING& s2 ) const
{
    // avoid instantiating new STRINGs, and thank goodness that c_str() is const.

    const char* rev1 = endsWithRev( s1, '/' );
    const char* rev2 = endsWithRev( s2, '/' );

    int rootLen1 =  rev1 ? rev1 - s1.c_str() : s1.size();
    int rootLen2 =  rev2 ? rev2 - s2.c_str() : s2.size();

    int r = memcmp( s1.c_str(), s2.c_str(), min( rootLen1, rootLen2 ) );

    if( r )
    {
        return r < 0;
    }

    if( rootLen1 != rootLen2 )
    {
        return rootLen1 < rootLen2;
    }

    // root strings match at this point, compare the revision number numerically,
    // and chose the higher numbered version as "less", according to std::set lingo.

    if( bool(rev1) != bool(rev2) )
    {
        return bool(rev1) < bool(rev2);
    }

    if( rev1 && rev2 )
    {
        int rnum1 = atoi( rev1+3 );
        int rnum2 = atoi( rev2+3 );

        // higher numbered revs are "less" so that they bubble to top.
        return rnum1 > rnum2;
    }

    return false;   // strings are equal, and they don't have a rev
}


bool DIR_LIB_SOURCE::makePartName( STRING* aPartName, const char* aEntry,
                        const STRING& aCategory )
{
    const char* cp = strrstr( aEntry, SWEET_EXT );

    // if base name is not empty, contains SWEET_EXT, && cp is not NULL
    if( cp > aEntry )
    {
        const char* limit = cp + strlen( cp );

        // If versioning, then must find a trailing "revN.." type of string.
        if( useVersioning )
        {
            const char* rev = endsWithRev( cp + sizeof(SWEET_EXT) - 1, limit, '.' );
            if( rev )
            {
                if( aCategory.size() )
                    *aPartName = aCategory + "/";
                else
                    aPartName->clear();

                aPartName->append( aEntry, cp - aEntry );
                aPartName->append( "/" );
                aPartName->append( rev );
                return true;
            }
        }

        // If using versioning, then all valid partnames must have a rev string,
        // so we don't even bother to try and load any other partfile down here.
        else
        {
            // if file extension is exactly SWEET_EXT, and no rev
            if( cp==limit-5 )
            {
                if( aCategory.size() )
                    *aPartName = aCategory + "/";
                else
                    aPartName->clear();

                aPartName->append( aEntry, cp - aEntry );
                return true;
            }
        }
    }

    return false;
}


STRING DIR_LIB_SOURCE::makeFileName( const STRING& aPartName )
{
    // create a fileName for the sweet string, using a reversible
    // partname <-> fileName conversion protocol:

    STRING  fileName = sourceURI + "/";

    const char* rev = endsWithRev( aPartName, '/' );

    if( rev )
    {
        int basePartLen = rev - aPartName.c_str() - 1;  // omit '/' separator
        fileName.append( aPartName, 0,  basePartLen );
        fileName += SWEET_EXT;
        fileName += '.';
        fileName += rev;
    }
    else
    {
        fileName += aPartName;
        fileName += SWEET_EXT;
    }

    return fileName;
}


void DIR_LIB_SOURCE::readString( STRING* aResult, const STRING& aFileName ) throw( IO_ERROR )
{
    FILE_WRAP   fw = open( aFileName.c_str(), O_RDONLY );

    if( fw == -1 )
    {
        STRING  msg = strerror( errno );
        msg += "; cannot open(O_RDONLY) file " + aFileName;
        throw( IO_ERROR( msg.c_str() ) );
    }

    struct stat     fs;

    fstat( fw, &fs );

    // sanity check on file size
    if( fs.st_size > (1*1024*1024) )
    {
        STRING msg = aFileName;
        msg += " seems too big.  ( > 1 mbyte )";
        throw IO_ERROR( msg.c_str() );
    }

    // reuse same readBuffer, which is not thread safe, but the API
    // is not advertising thread safe (yet, if ever).
    if( (int) fs.st_size > (int) readBuffer.size() )
        readBuffer.resize( fs.st_size + 1000 );

    int count = read( fw, &readBuffer[0], fs.st_size );
    if( count != (int) fs.st_size )
    {
        STRING  msg = strerror( errno );
        msg += "; cannot read file " + aFileName;
        throw( IO_ERROR( msg.c_str() ) );
    }

    // std::string chars are not guaranteed to be contiguous in
    // future implementations of C++, so this is why we did not read into
    // aResult directly.
    aResult->assign( &readBuffer[0], count );
}


void DIR_LIB_SOURCE::cache() throw( IO_ERROR )
{
    partnames.clear();
    categories.clear();

    cacheOneDir( "" );
}


DIR_LIB_SOURCE::DIR_LIB_SOURCE( const STRING& aDirectoryPath,
                                const STRING& aOptions ) throw( IO_ERROR ) :
    useVersioning( strstr( aOptions.c_str(), "useVersioning" ) )
{
    sourceURI     = aDirectoryPath;
    sourceType    = "dir";

    if( sourceURI.size() == 0 )
    {
        throw( IO_ERROR( "aDirectoryPath cannot be empty" ) );
    }

    // remove any trailing separator, so we can add it back later without ambiguity
    if( strchr( "/\\", sourceURI[sourceURI.size()-1] ) )
        sourceURI.erase( sourceURI.size()-1 );

    cache();
}


DIR_LIB_SOURCE::~DIR_LIB_SOURCE()
{
}


void DIR_LIB_SOURCE::GetCategoricalPartNames( STRINGS* aResults, const STRING& aCategory )
    throw( IO_ERROR )
{
    PN_ITER limit = aCategory.size() ?
                        partnames.lower_bound( aCategory + char( '/' + 1 ) ) :
                        partnames.end();

    PN_ITER it    = aCategory.size() ?
                        partnames.upper_bound( aCategory + "/" ) :
                        partnames.begin();

    aResults->clear();

    if( useVersioning )
    {
        STRING  partName;

        while( it != limit )
        {
            const char* rev   = endsWithRev( *it, '/' );

            // all cached partnames have a rev string in useVersioning mode
            assert( rev );

            // partName is substring which omits the rev AND the rev separator
            partName.assign( *it, 0, rev - it->c_str() - 1 );

            aResults->push_back( partName );

            // skip over all other versions of the same partName.
            it = partnames.lower_bound( partName + char( '/' + 1 ) );
        }

    }

    else
    {
        while( it != limit )
            aResults->push_back( *it++ );
    }
}


void DIR_LIB_SOURCE::ReadPart( STRING* aResult, const STRING& aPartName, const STRING& aRev )
    throw( IO_ERROR )
{
    STRING  partName = aPartName;   // appended with aRev too if not empty
    const char* rev  = endsWithRev( partName, '/' );

    if( aRev.size() )
    {
        if( rev )   // a supplied rev replaces any in aPartName
            partName.resize( rev - partName.c_str() - 1 );

        partName += "/" + aRev;

        rev = endsWithRev( partName, '/' );
    }

    // partName is the exact part name we need here, or if rev is NULL,
    // then look for the highest numbered revision.

    if( rev )
    {
        PN_ITER it = partnames.find( partName );

        if( it == partnames.end() )    // part not found
        {
            partName += " not found.";
            throw IO_ERROR( partName.c_str() );
        }

        readString( aResult, makeFileName( partName ) );
    }
    else
    {
        STRING search = partName + '/';

        // There's no rev on partName string.  Find the most recent rev, i.e. highest,
        // which will be first because of the BY_REV compare method, which treats
        // higher numbered revs as first.
        PN_ITER it = partnames.upper_bound( search );

        // verify that this one that is greater than partName is a match and not
        // some unrelated name that is larger.
        if( it == partnames.end() || it->compare( 0, search.size(), search ) != 0 )
        {
            partName += " rev not found.";
            throw IO_ERROR( partName.c_str() );
        }

        readString( aResult, makeFileName( *it ) );
    }
}


void DIR_LIB_SOURCE::ReadParts( STRINGS* aResults, const STRINGS& aPartNames )
    throw( IO_ERROR )
{
    aResults->clear();

    for( STRINGS::const_iterator n = aPartNames.begin();  n!=aPartNames.end();  ++n )
    {
        aResults->push_back( STRING() );
        ReadPart( &aResults->back(), *n );
    }
}


void DIR_LIB_SOURCE::GetCategories( STRINGS* aResults ) throw( IO_ERROR )
{
    aResults->clear();

    // caller fetches them sorted.
    for( NAME_CACHE::const_iterator it = categories.begin();  it!=categories.end();  ++it )
    {
        aResults->push_back( *it );
    }
}


#if defined(DEBUG)

void DIR_LIB_SOURCE::Show()
{
    printf( "Show categories:\n" );
    for( NAME_CACHE::const_iterator it = categories.begin();  it!=categories.end();  ++it )
        printf( " '%s'\n", it->c_str() );

    printf( "\n" );
    printf( "Show parts:\n" );
    for( PART_CACHE::const_iterator it = partnames.begin();  it != partnames.end();  ++it )
    {
        printf( " '%s'\n", it->c_str() );
    }
}

#endif


void DIR_LIB_SOURCE::cacheOneDir( const STRING& aCategory ) throw( IO_ERROR )
{
    STRING      curDir = sourceURI;

    if( aCategory.size() )
        curDir += "/" + aCategory;

    DIR_WRAP    dir = opendir( curDir.c_str() );

    if( !dir )
    {
        STRING  msg = strerror( errno );
        msg += "; scanning directory " + curDir;
        throw( IO_ERROR( msg.c_str() ) );
    }

    struct stat     fs;
    STRING          partName;
    STRING          fileName;
    dirent*         entry;

    while( (entry = readdir( *dir )) != NULL )
    {
        if( !strcmp( ".", entry->d_name ) || !strcmp( "..", entry->d_name ) )
            continue;

        fileName = curDir + "/" + entry->d_name;

        if( !stat( fileName.c_str(), &fs ) )
        {
            // is this a valid part name?
            if( S_ISREG( fs.st_mode ) && makePartName( &partName, entry->d_name, aCategory ) )
            {
                std::pair<NAME_CACHE::iterator, bool> pair = partnames.insert( partName );

                if( !pair.second )
                {
                    STRING  msg = partName;
                    msg += " has already been encountered";
                    throw IO_ERROR( msg.c_str() );
                }
            }

            // is this an acceptable category name?
            else if( S_ISDIR( fs.st_mode ) && !aCategory.size() && isCategoryName( entry->d_name ) )
            {
                // only one level of recursion is used, controlled by the
                // emptiness of aCategory.
                categories.insert( entry->d_name );

                // somebody needs to test Windows (mingw), make sure it can
                // handle opendir() recursively
                cacheOneDir( entry->d_name );
            }
            else
            {
                //D( printf( "ignoring %s\n", entry->d_name );)
            }
        }
    }
}


#if (1 || defined( TEST_DIR_LIB_SOURCE )) && defined(DEBUG)

int main( int argc, char** argv )
{
    STRINGS     partnames;
    STRINGS     sweets;

    try
    {
        STRINGS::const_iterator  pn;

//        DIR_LIB_SOURCE  uut( argv[1] ? argv[1] : "", "" );
        DIR_LIB_SOURCE  uut( argv[1] ? argv[1] : "", "useVersioning" );

        // initially, only the NAME_CACHE sweets and STRING categories are loaded:
        uut.Show();

        uut.GetCategoricalPartNames( &partnames, "lions" );

        printf( "\nGetCategoricalPartNames( aCatagory = 'lions' ):\n" );
        for( STRINGS::const_iterator it = partnames.begin();  it!=partnames.end();  ++it )
        {
            printf( " '%s'\n", it->c_str() );
        }

        uut.ReadParts( &sweets, partnames );

        printf( "\nSweets for Category = 'lions' parts:\n" );
        pn = partnames.begin();
        for( STRINGS::const_iterator it = sweets.begin();  it!=sweets.end();  ++it, ++pn )
        {
            printf( " %s: %s", pn->c_str(), it->c_str() );
        }

        // fetch the part names for ALL categories.
        uut.GetCategoricalPartNames( &partnames );

        printf( "\nGetCategoricalPartNames( aCategory = '' i.e. ALL):\n" );
        for( STRINGS::const_iterator it = partnames.begin();  it!=partnames.end();  ++it )
        {
            printf( " '%s'\n", it->c_str() );
        }

        uut.ReadParts( &sweets, partnames );

        printf( "\nSweets for ALL parts:\n" );
        pn = partnames.begin();
        for( STRINGS::const_iterator it = sweets.begin();  it!=sweets.end();  ++it, ++pn )
        {
            printf( " %s: %s", pn->c_str(), it->c_str() );
        }
    }

    catch( std::exception& ex )
    {
        printf( "std::exception\n" );
    }

    catch( IO_ERROR ioe )
    {
        printf( "exception: %s\n", (const char*) wxConvertWX2MB( ioe.errorText ) );
    }

    return 0;
}

#endif
