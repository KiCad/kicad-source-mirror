/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 KiCad Developers, see change_log.txt for contributors.
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


#include <set>
#include <vector>

#include <sch_lib.h>


/// This file extension is an implementation detail specific to this LIB_SOURCE
/// and to a corresponding LIB_SINK.
/// Core EESCHEMA should never have to see this.
#define SWEET_EXT       ".part"
#define SWEET_EXTZ      (sizeof(SWEET_EXT)-1)


/**
 * struct BY_REV
 * is here to provide a custom way to compare STRINGs.  Namely, the revN[N..]
 * string if present, is collated according to a 'higher revision first', but
 * any part string without a revision, is even 'before' that.
 */
struct BY_REV
{
    bool operator() ( const STRING& s1, const STRING& s2 ) const;
};


/**
 * Type PART_CACHE
 * holds a set of part names in sorted order, according to the sort
 * order given by struct BY_REV.
 */
typedef std::set< STRING, BY_REV >  PART_CACHE;


/**
 * Type NAME_CACHE
 * holds a set of categories in sorted order.
 */
typedef std::set< STRING >          NAME_CACHE;


namespace SCH {

/**
 * Class DIR_LIB_SOURCE
 * implements a LIB_SOURCE in a file system directory.
 *
 * @author Dick Hollenbeck
 */
class DIR_LIB_SOURCE : public LIB_SOURCE
{
    friend class LIB_TABLE;     ///< constructor is protected, LIB_TABLE can construct

    bool                useVersioning;  ///< use files with extension ".revNNN..", else not

    /// normal partnames, some of which may be prefixed with a category,
    /// and some of which may have legal "revN[N..]" type strings.
    PART_CACHE          partnames;

    typedef PART_CACHE::const_iterator PN_ITER;

    /// categories which we expect to find in the set of @a partnames
    NAME_CACHE          categories;

    std::vector<char>   readBuffer;     ///< used by readString()

    /**
     * Function cache
     * [re-]loads the directory cache(s).
     */
    void cache();

    /**
     * Function isCategoryName
     * returns true iff aName is a valid category name.
     */
    bool isCategoryName( const char* aName )
    {
        return true;
    }

    /**
     * Function makePartName
     * returns true iff aEntry holds a valid part filename, in the form of
     * "someroot.part[.revNNNN]"  where NNN are number characters [0-9]
     * @param aEntry is the raw directory entry without path information.
     * @param aCategory is the last portion of the directory path.
     * @param aPartName is where to put a part name, assuming @a aEntry is legal.
     * @return bool - true only if aEntry is a legal part file name.
     */
    bool makePartName( STRING* aPartName, const char* aEntry, const STRING& aCategory );

    /**
     * Function readString
     * reads a Sweet string into aResult.  Candidate for virtual function later.
     */
    void readString( STRING* aResult, const STRING& aFileName );

    /**
     * Function cacheOneDir
     * loads part names [and categories] from a directory given by
     * "sourceURI + '/' + category"
     * Categories are only loaded if processing the top most directory because
     * only one level of categories is supported.  We know we are in the
     * top most directory if aCategory is empty.
     */
    void cacheOneDir( const STRING& aCategory );

    /**
     * Function makeFileName
     * converts a part name into a filename and returns it.
     */
    STRING makeFileName( const STRING& aPartName );

protected:

    /**
     * Constructor DIR_LIB_SOURCE( const STRING& aDirectoryPath )
     * sets up a LIB_SOURCE using aDirectoryPath in a file system.
     * @see LIB_TABLE::LookupPart().
     *
     * @param aDirectoryPath is a full file pathname of a directory which contains
     *  the library source of part files.  Examples might be "C:\kicad_data\mylib" or
     *  "/home/designer/mylibdir".  This is not a URI, but an OS specific path that
     *  can be given to opendir().
     *
     * @param aOptions is the options string from the library table, currently
     *  the only supported option, that this LIB_SOURCE knows about is
     *  "useVersioning".  If present means support versioning in the directory
     *  tree, otherwise only a single version of each part is recognized, namely the
     *  one without the ".revN[N..]" trailer.
     */
    DIR_LIB_SOURCE( const STRING& aDirectoryPath, const STRING& aOptions = "" );

    ~DIR_LIB_SOURCE();

    //-----<LIB_SOURCE implementation functions >------------------------------

    void ReadPart( STRING* aResult, const STRING& aPartName, const STRING& aRev = "" );

    void ReadParts( STRINGS* aResults, const STRINGS& aPartNames );

    void GetCategories( STRINGS* aResults );

    void GetCategoricalPartNames( STRINGS* aResults, const STRING& aCategory = "" );

    void GetRevisions( STRINGS* aResults, const STRING& aPartName );

    void FindParts( STRINGS* aResults, const STRING& aQuery )
    {
        // @todo
    }

    //-----</LIB_SOURCE implementation functions >------------------------------

#if defined(DEBUG)
    /**
     * Function Show
     * will output a debug dump of contents.
     */
    void Show();

public:
    static void Test( int argc, char** argv );

#endif
};

}       // namespace SCH

#endif  // DIR_LIB_SOURCE_H_
