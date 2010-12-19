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
 * provides a destructor which may be invoked if an exception is thrown.
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
};


/**
 * Class FILE_WRAP
 * provides a destructor which may be invoked if an exception is thrown.
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


static const char* endsWithRev( const char* cp, const char* limit )
{
    // find last instance of ".rev"
    cp = strrstr( cp, ".rev" );
    if( cp )
    {
        const char* rev = cp + 1;

        cp += sizeof( ".rev" )-1;

        while( isdigit( *cp ) )
            ++cp;

        if( cp != limit )   // there is garbage after "revN.."
            rev = 0;

        return rev;
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
            const char* rev = endsWithRev( cp + sizeof(".part") - 1, limit );
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


#define MAX_PART_FILE_SIZE          (1*1024*1024)   // sanity check

DIR_LIB_SOURCE::DIR_LIB_SOURCE( const STRING& aDirectoryPath, bool doUseVersioning )
    throw( IO_ERROR )
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


void DIR_LIB_SOURCE::Show()
{
    printf( "categories:\n" );
    for( STRINGS::const_iterator it = categories.begin();  it!=categories.end();  ++it )
        printf( " '%s'\n", it->c_str() );

    printf( "\n" );
    printf( "parts:\n" );
    for( DIR_CACHE::const_iterator it = sweets.begin();  it != sweets.end();  ++it )
    {
        printf( " '%s'\n", it->first.c_str() );
    }
}


void DIR_LIB_SOURCE::doOneDir( const STRING& aCategory ) throw( IO_ERROR )
{
    STRING      curDir = sourceURI;

    if( aCategory.size() )
        curDir += "/" + aCategory;

    DIR_WRAP    dir = opendir( curDir.c_str() );

    if( !*dir )
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

        //D( printf("name: '%s'\n", fileName.c_str() );)

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
                //D( printf("part: %s\n", partName.c_str() );)
            }

            else if( S_ISDIR( fs.st_mode ) && !aCategory.size() && isCategoryName( entry->d_name ) )
            {
                // only one level of recursion is used, controlled by the
                // emptiness of aCategory.
                //D( printf("category: %s\n", entry->d_name );)
                categories.push_back( entry->d_name );
                doOneDir( entry->d_name );
            }
            else
            {
                //D( printf( "ignoring %s\n", entry->d_name );)
            }
        }
    }
}


#if 1 || defined( TEST_DIR_LIB_SOURCE )

int main( int argc, char** argv )
{

    try
    {
        DIR_LIB_SOURCE  uut( argv[1] ? argv[1] : "", true );
        uut.Show();
    }

    catch( IO_ERROR ioe )
    {
        printf( "exception: %s\n", (const char*) wxConvertWX2MB( ioe.errorText ) );
    }

    return 0;
}

#endif


