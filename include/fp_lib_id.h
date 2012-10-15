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

#ifndef _FP_LIB_ID_H_
#define _FP_LIB_ID_H_

#include <richio.h>


/**
 * Class FP_LIB_ID
 * (aka GUID) is a Logical Part ID and consists of various portions much like a URI.
 * It is a container for the separated portions of a logical part id std::string so they
 * can be accessed individually.  The various portions of an FP_LIB_ID are:
 * logicalLibraryName, category, baseName, and revision.  Only the baseName is
 * mandatory.  There is another construct called "footprintName" which consists of
 * [category/]baseName.  That is the category followed by a slash, but only if
 * the category is not empty.
 * <p>
 * footprintName = [category/]baseName
 * <p>
 * Example FP_LIB_ID string:
 * "smt:R_0805".
 * <p>
 * <ul>
 * <li> "smt" is the logical library name.
 * <li> "R" is the footprint name.
 * <li> "rev6" is the revision, which is optional.  If missing then its
 *      / delimiter should also not be present. A revision must begin with
 *      "rev" and be followed by at least one or more decimal digits.
 * </ul>
 * @author Dick Hollenbeck
 */
class FP_LIB_ID  // aka GUID
{
public:

    FP_LIB_ID() {}

    /**
     * Constructor FP_LIB_ID
     * takes \a aId string and parses it.  A typical FP_LIB_ID string uses a logical
     * library name followed by a footprint name.
     * e.g.: "smt:R_0805", or
     * e.g.: "mylib:R_0805"
     */
    FP_LIB_ID( const std::string& aId ) throw( PARSE_ERROR );

    /**
     * Function Parse
     * [re-]stuffs this FP_LIB_ID with the information from @a aId.
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the
     *  character offset into aId at which an error was detected.
     */
    int Parse( const std::string& aId );

    /**
     * Function GetLogicalLib
     * returns the logical library portion of a FP_LIB_ID.  There is not Set accessor
     * for this portion since it comes from the library table and is considered
     * read only here.
     */
    const std::string& GetLogicalLib() const
    {
        return logical;
    }

    /**
     * Function SetCategory
     * overrides the logical lib name portion of the FP_LIB_ID to @a aLogical, and can be empty.
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the
     *  character offset into the parameter at which an error was detected, usually
     *  because it contained '/' or ':'.
     */
    int SetLogicalLib( const std::string& aLogical );

    /**
     * Function GetBaseName
     * returns the part name without the category.
     */
    const std::string& GetBaseName() const
    {
        return baseName;
    }

    /**
     * Function SetBaseName
     * overrides the base name portion of the FP_LIB_ID to @a aBaseName
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the
     *  character offset into the parameter at which an error was detected, usually
     *  because it contained '/' or ':', or is blank.
     */
    int SetBaseName( const std::string& aBaseName );

    /**
     * Function GetFootprintName
     * returns the part name, i.e. category/baseName without revision.
     */
    const std::string& GetFootprintName() const
    {
        return footprintName;
    }

    /**
     * Function GetFootprintNameAndRev
     * returns the part name with revision if any, i.e. baseName[/revN..]
     */
    std::string GetFootprintNameAndRev() const;

    /**
     * Function SetFootprintName
     * overrides the part name portion of the FP_LIB_ID to @a aFootprintName
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the
     *  character offset into the parameter at which an error was detected, usually
     *  because it contained more than one '/', or one or more ':', or is blank.
     *  A single '/' is allowed, since that is used to separate the category from the
     *  base name.
     */
    int SetFootprintName( const std::string& aFootprintName );

    /**
     * Function GetRevision
     * returns the revision portion of the FP_LIB_ID.
     */
    const std::string& GetRevision() const
    {
        return revision;
    }

    /**
     * Function SetRevision
     * overrides the revision portion of the FP_LIB_ID to @a aRevision and must
     * be in the form "rev<num>" where "<num>" is "1", "2", etc.
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the
     *  character offset into the parameter at which an error was detected,
     *  because it did not look like "rev23"
     */
    int SetRevision( const std::string& aRevision );

    /**
     * Function Format
     * returns the full text of the FP_LIB_ID.
     */
    std::string Format() const;

    /**
     * Function Format
     * returns a std::string in the proper format as an FP_LIB_ID for a combination of
     * aLogicalLib, aFootprintName, and aRevision.
     * @throw PARSE_ERROR if any of the pieces are illegal.
     */
    static std::string Format( const std::string& aLogicalLib, const std::string& aFootprintName,
                               const std::string& aRevision="" )
        throw( PARSE_ERROR );

    void clear();

#if defined(DEBUG)
    static void Test();
#endif

protected:
    std::string  logical;        ///< logical lib name or empty
    std::string  baseName;       ///< without category
    std::string  revision;       ///< "revN[N..]" or empty
    std::string  footprintName;  ///< cannot be set directory, set via SetBaseName() & SetCategory()
};

/**
 * Function EndsWithRev
 * returns a pointer to the final string segment: "revN[N..]" or NULL if none.
 * @param start is the beginning of string segment to test, the partname or
 *  any middle portion of it.
 * @param tail is a pointer to the terminating nul, or one past inclusive end of
 *  segment, i.e. the string segment of interest is [start,tail)
 * @param separator is the separating byte, expected: '.' or '/', depending on context.
 */
const char* EndsWithRev( const char* start, const char* tail, char separator = '/' );

static inline const char* EndsWithRev( const std::string& aFootprintName, char separator = '/' )
{
    return EndsWithRev( aFootprintName.c_str(),  aFootprintName.c_str()+aFootprintName.size(),
                        separator );
}


/**
 * Function RevCmp
 * compares two rev strings in a way like strcmp() except that the highest numbered
 * revision is considered first in the sort order.  The function probably won't work
 * unless you give it two rev strings.
 * @param s1 is a rev string like "rev10"
 * @param s2 is a rev string like "rev1".
 * @return int - either negative, zero, or positive depending on whether the revision
 *  is greater, equal, or less on the left hand side.
 */
int RevCmp( const char* s1, const char* s2 );

#endif // _FP_LIB_ID_H_
