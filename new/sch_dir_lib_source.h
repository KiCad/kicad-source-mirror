/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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


#include <sch_lib.h>

#include <map>


/**
 * Type DIR_CACHE
 * is a tuple, where the key is partname (prefixed with the category if any),
 * and value is pointer to Sweet string which is loaded lazily, so can be NULL
 * until loaded.
 */
typedef std::map< STRING, STRING* >     DIR_CACHE;


namespace SCH {

/**
 * Class DIR_LIB_SOURCE
 * implements a LIB_SOURCE in a file system directory.
 *
 * @author Dick Hollenbeck
 */
class DIR_LIB_SOURCE : public LIB_SOURCE
{
    friend class LIBS;          ///< LIBS::GetLib() can construct one.

    bool        useVersioning;  ///< use files with extension ".revNNN..", else not

    DIR_CACHE   sweets;
    STRINGS     categories;

    /**
     * Function isPartFileName
     * returns true iff aName is a valid part file name.
     */
    bool  isPartFileName( const char* aName );

    /**
     * Function makePartFileName
     * returns true iff aEntry holds a valid part filename, in the form of
     * "someroot.part[.revNNNN]"  where NNN are number characters [0-9]
     * @param aEntry is the raw directory entry without path information.
     * @param aCategory is the last portion of the directory path.
     * @param aPartName is where to put a part name, assuming aEntry is legal.
     * @return bool - true only if aEntry is a legal part file name.
     */
    bool makePartFileName( const char* aEntry,
                           const STRING& aCategory, STRING* aPartName );

    /**
     * Function doOneDir
     * loads part names [and categories] from a directory given by
     * "sourceURI + '/' + category"
     * Categories are only loaded if processing the top most directory because
     * only one level of categories are supported.  We know we are in the
     * top most directory if aCategory is empty.
     */
    void doOneDir( const STRING& aCategory ) throw( IO_ERROR );

//protected:
public:

    /**
     * Constructor DIR_LIB_SOURCE( const STRING& aDirectoryPath )
     * sets up a LIB_SOURCE using aDirectoryPath in a file system.
     * @see LIBS::GetLibrary().
     *
     * @param aDirectoryPath is a full file pathname of a directory which contains
     *  the library source of part files.  Examples might be "C:\kicad_data\mylib" or
     *  "/home/designer/mylibdir".  This is not a URI, but an OS specific path that
     *  can be given to opendir().
     *
     * @param doUseVersioning if true means support versioning in the directory tree, otherwise
     *  only a single version of each part is recognized.
     */
    DIR_LIB_SOURCE( const STRING& aDirectoryPath, bool doUseVersioning = false )
        throw( IO_ERROR );

    ~DIR_LIB_SOURCE();

    //-----<LIB_SOURCE implementation functions >------------------------------

    void ReadPart( STRING* aResult, const STRING& aPartName, const STRING& aRev=StrEmpty )
        throw( IO_ERROR )
    {
    }

    void ReadParts( STRING_TOKS* aResults, const STRINGS& aPartNames )
        throw( IO_ERROR )
    {
    }

    void GetCategories( STRING_TOKS* aResults ) throw( IO_ERROR )
    {
    }

    void GetCategoricalPartNames( STRING_TOKS* aResults,
                    const STRING& aCategory=StrEmpty ) throw( IO_ERROR )
    {
    }

    void GetRevisions( STRING_TOKS* aResults, const STRING& aPartName ) throw( IO_ERROR )
    {
    }

    void FindParts( STRING_TOKS* aResults, const STRING& aQuery ) throw( IO_ERROR )
    {
    }

    //-----</LIB_SOURCE implementation functions >------------------------------

    /**
     * Function Show
     * will output a debug dump of contents.
     */
    void Show();
};

}       // namespace SCH

#endif  // DIR_LIB_SOURCE_H_
