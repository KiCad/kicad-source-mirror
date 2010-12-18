
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

#ifndef DIR_LIB_SOURCE_H_
#define DIR_LIB_SOURCE_H_


/*  Note: this LIB_SOURCE implementation relies on the posix specified opendir() and
    related functions.  Mingw and unix, linux, & osx will all have these posix functions.
    MS Visual Studio may need the posix compatible opendir() functions brought in
        http://www.softagalleria.net/dirent.php
    wx has these but they are based on wxString and wx should not be introduced
    at a level this low.
*/



namespace SCH {


/**
 * Class DIR_LIB_SOURCE
 * implements a LIB_SOURCE in a file system directory.
 *
 * @author Dick Hollenbeck
 */
class DIR_LIB_SOURCE : public LIB_SOURCE
{
    friend class LIBS;   ///< LIBS::GetLib() can construct one.

    STRING      path;       ///< base directory path of LIB_SOURCE


protected:

    /**
     * Constructor DIR_LIB_SOURCE( const STRING& aDirectoryPath )
     * sets up a LIB_SOURCE using aDirectoryPath in a file system.
     * @see LIBS::GetLibrary().
     *
     * @param aDirectoryPath is a full pathname of a directory which contains
     *  the library source of part files.  Examples might be "C:\kicad_data\mylib" or
     *  "/home/designer/mylibdir".
     */
    DIR_LIB_SOURCE( const STRING& aDirectoryPath ) throws( IO_ERROR, PARSE_ERROR );


};

}       // namespace SCH

#endif  // DIR_LIB_SOURCE_H_



#include <dirent.h>
#include <cstring>
#include <ki_exceptions.h>

#include <sys/types.h>


/**
 * Class DIR_WRAP
 * provides a destructor which may be invoked if an exception is thrown,
 * thereby closing the DIR.
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

    DIR* operator->() { return dir; }
};


DIR_LIB_SOURCE::DIR_LIB_SOURCE( const STRING& aDirectoryPath ) throws( IO_ERROR, PARSE_ERROR )
{
    DIR_WRAP*   dir = opendir( aDirectoryPath.c_str() );

    if( !dir )
    {
        char    buf[256];

        strerror_r( errno, buf, sizeof(buf) );
        throw( IO_ERROR( buf ) );
    }

    path = aDirectoryPath;


}



#if 1 || defined( TEST_DIR_LIB_SOURCE )

int main( int argv, char** argv )
{
}

#endif


