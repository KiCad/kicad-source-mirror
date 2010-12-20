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

#include <vector>
using namespace std;



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
 * returns a pointer to the final string segment: "revN..." or NULL if none.
 * @param start is the beginning of string segment to test, the partname or
 *  any middle portion of it.
 * @param tail is a pointer to the terminating nul.
 * @param separator is the separating byte, expected: '.' or '/', depending on context.
 */
static const char* endsWithRev( const char* start, const char* tail, char separator )
{
    bool    sawDigit = false;

    while( isdigit(*--tail) && tail>start )
    {
        sawDigit = true;
    }

    if( sawDigit && tail-3 >= start && tail[-3] == separator )
    {
        tail -= 2;
        if( tail[0]=='r' && tail[1]=='e' && tail[2]=='v' )
        {
            return tail;
        }
    }

    return 0;
}


bool DIR_LIB_SOURCE::makePartFileName( const char* aEntry,
                        const STRING& aCategory, STRING* aPartName )
{
    const char* cp = strrstr( aEntry, ".part" );

    // if base name is not empty, contains ".part", && cp is not NULL
    if( cp > aEntry )
    {
        const char* limit = cp + strlen( cp );

        // if file extension is exactly ".part", and no rev
        if( cp==limit-5 )
        {
            if( aCategory.size() )
                *aPartName = aCategory + "/";
            else
                aPartName->clear();

            aPartName->append( aEntry, cp - aEntry );
            return true;
        }

        // if versioning, test for a trailing "revN.." type of string
        if( useVersioning )
        {
            const char* rev = endsWithRev( cp + sizeof(".part") - 1, limit, '.' );
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
    }

    return false;
}

static bool isCategoryName( const char* aName )
{
    return true;
}


void DIR_LIB_SOURCE::readSExpression( STRING* aResult, const STRING& aFilename ) throw( IO_ERROR )
{
    FILE_WRAP   fw = open( aFilename.c_str(), O_RDONLY );

    if( fw == -1 )
    {
        STRING  msg = aFilename;
        msg += " cannot be open()ed for reading";
        throw IO_ERROR( msg.c_str() );
    }

    struct stat     fs;

    fstat( fw, &fs );

    // sanity check on file size
    if( fs.st_size > (1*1024*1024) )
    {
        STRING msg = aFilename;
        msg += " seems too big.  ( > 1mbyte )";
        throw IO_ERROR( msg.c_str() );
    }

    // we reuse the same readBuffer, which is not thread safe, but the API
    // is not expected to be thread safe.
    readBuffer.resize( fs.st_size );

    size_t count = read( fw, &readBuffer[0], fs.st_size );
    if( count != (size_t) fs.st_size )
    {
        STRING msg = aFilename;
        msg += " cannot be read";
        throw IO_ERROR( msg.c_str() );
    }

    // std::string chars are not gauranteed to be contiguous in
    // future implementations of C++, so this is why we did not read into
    // aResult directly.
    aResult->assign( &readBuffer[0], count );
}


DIR_LIB_SOURCE::DIR_LIB_SOURCE( const STRING& aDirectoryPath,
                                bool doUseVersioning ) throw( IO_ERROR ) :
    readBuffer( 512 )
{
    useVersioning = doUseVersioning;
    sourceURI     = aDirectoryPath;
    sourceType    = "dir";

    if( sourceURI.size() == 0 )
    {
        throw( IO_ERROR( "aDirectoryPath cannot be empty" ) );
    }

    // remove any trailing separator, so we can add it back later without ambiguity
    if( strchr( "/\\", sourceURI[sourceURI.size()-1] ) )
        sourceURI.erase( sourceURI.size()-1 );

    doOneDir( "" );
}


DIR_LIB_SOURCE::~DIR_LIB_SOURCE()
{
    // delete the sweet STRINGS, which "sweets" owns by pointer.
    for( DIR_CACHE::iterator it = sweets.begin();  it != sweets.end();  ++it )
    {
        delete it->second;
    }
}


void DIR_LIB_SOURCE::GetCategoricalPartNames( STRINGS* aResults, const STRING& aCategory )
    throw( IO_ERROR )
{
    aResults->clear();

    if( aCategory.size() )
    {
        STRING  lower = aCategory + "/";
        STRING  upper = aCategory + char( '/' + 1 );

        DIR_CACHE::const_iterator limit = sweets.upper_bound( upper );

        for( DIR_CACHE::const_iterator it = sweets.lower_bound( lower );  it!=limit;  ++it )
        {
            const char* start = it->first.c_str();
            size_t      len   = it->first.size();

            if( !endsWithRev( start, start+len, '/' ) )
                aResults->push_back( it->first );
        }
    }
    else
    {
        for( DIR_CACHE::const_iterator it = sweets.begin();  it!=sweets.end();  ++it )
        {
            const char* start = it->first.c_str();
            size_t      len   = it->first.size();

            if( !endsWithRev( start, start+len, '/' ) )
                aResults->push_back( it->first );
        }
    }
}


void DIR_LIB_SOURCE::ReadPart( STRING* aResult, const STRING& aPartName, const STRING& aRev )
    throw( IO_ERROR )
{
    STRING  partname = aPartName;

    if( aRev.size() )
        partname += "/" + aRev;

    DIR_CACHE::iterator it = sweets.find( partname );

    if( it == sweets.end() )    // part not found
    {
        partname += " not found.";
        throw IO_ERROR( partname.c_str() );
    }

    if( !it->second )   // if the sweet string is not loaded yet
    {
        STRING  filename = sourceURI + "/" + aPartName + ".part";

        if( aRev.size() )
        {
            filename += "." + aRev;
        }

        it->second = new STRING();

        readSExpression( it->second, filename );
    }

    *aResult = *it->second;
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
    *aResults = categories;
}


#if defined(DEBUG)
#include <richio.h>

void DIR_LIB_SOURCE::Show()
{
    printf( "Show categories:\n" );
    for( STRINGS::const_iterator it = categories.begin();  it!=categories.end();  ++it )
        printf( " '%s'\n", it->c_str() );

    printf( "\n" );
    printf( "Show parts:\n" );
    for( DIR_CACHE::const_iterator it = sweets.begin();  it != sweets.end();  ++it )
    {
        printf( " '%s'\n", it->first.c_str() );

        if( it->second )
        {
            STRING_LINE_READER  slr( *it->second, wxString( wxConvertMB2WX( it->first.c_str() ) ) );
            while( slr.ReadLine() )
            {
                printf( "    %s", (char*) slr );
            }
            printf( "\n" );
        }
    }
}
#endif


void DIR_LIB_SOURCE::doOneDir( const STRING& aCategory ) throw( IO_ERROR )
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
            if( S_ISREG( fs.st_mode ) && makePartFileName( entry->d_name, aCategory, &partName ) )
            {
                /*
                if( sweets.find( partName ) != sweets.end() )
                {
                    STRING  msg = partName;
                    msg += " has already been encountered";
                    throw IO_ERROR( msg.c_str() );
                }
                */

                sweets[partName] = NULL;  // NULL for now, load the sweet later.
            }

            else if( S_ISDIR( fs.st_mode ) && !aCategory.size() && isCategoryName( entry->d_name ) )
            {
                // only one level of recursion is used, controlled by the
                // emptiness of aCategory.
                categories.push_back( entry->d_name );

                // somebody needs to test Windows (mingw), make sure it can
                // handle opendir() recursively
                doOneDir( entry->d_name );
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
        DIR_LIB_SOURCE  uut( argv[1] ? argv[1] : "", true );

        // initially, only the DIR_CACHE sweets and STRING categories are loaded:
        uut.Show();

        uut.GetCategoricalPartNames( &partnames, "Category" );

        printf( "GetCategoricalPartNames(Category):\n" );
        for( STRINGS::const_iterator it = partnames.begin();  it!=partnames.end();  ++it )
        {
            printf( " '%s'\n", it->c_str() );
        }

        uut.ReadParts( &sweets, partnames );


        // fetch the part names for ALL categories.
        uut.GetCategoricalPartNames( &partnames );

        printf( "GetCategoricalPartNames(ALL):\n" );
        for( STRINGS::const_iterator it = partnames.begin();  it!=partnames.end();  ++it )
        {
            printf( " '%s'\n", it->c_str() );
        }

        uut.ReadParts( &sweets, partnames );

        printf( "Sweets for ALL parts:\n" );
        STRINGS::const_iterator pn = partnames.begin();
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

