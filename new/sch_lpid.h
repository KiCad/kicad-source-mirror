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

#ifndef SCH_LPID_H_
#define SCH_LPID_H_

#include <utf8.h>
#include <richio.h>

namespace SCH {

/**
 * Class LPID
 * (aka GUID) is a Logical Part ID and consists of various portions much like a URI.
 * It is a container for the separated portions of a logical part id STRING so they
 * can be accessed individually.  The various portions of an LPID are:
 * logicalLibraryName, category, baseName, and revision.  Only the baseName is
 * mandatory.  There is another construct called "partName" which consists of
 * [category/]baseName.  That is the category followed by a slash, but only if
 * the category is not empty.
 * <p>
 * partName = [category/]baseName
 * <p>
 * Example LPID string:
 * "kicad:passives/R/rev6".
 * <p>
 * <ul>
 * <li> "kicad" is the logical library name.
 * <li> "passives" is the category.
 * <li> "passives/R" is the partname.
 * <li> "rev6" is the revision, which is optional.  If missing then its
 *      / delimiter should also not be present. A revision must begin with
 *      "rev" and be followed by at least one or more decimal digits.
 * </ul>
 * @author Dick Hollenbeck
 */
class LPID  // aka GUID
{
public:

    LPID() {}

    /**
     * Constructor LPID
     * takes aLPID string and parses it.  A typical LPID string uses a logical
     * library name followed by a part name.
     * e.g.: "kicad:passives/R/rev2", or
     * e.g.: "mylib:R33"
     */
    LPID( const STRING& aLPID ) throw( PARSE_ERROR );

    /**
     * Function Parse
     * [re-]stuffs this LPID with the information from @a aLPID.
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the
     *  character offset into aLPID at which an error was detected.
     */
    int Parse( const STRING& aLPID );

    /**
     * Function GetLogicalLib
     * returns the logical library portion of a LPID.  There is not Set accessor
     * for this portion since it comes from the library table and is considered
     * read only here.
     */
    const STRING& GetLogicalLib() const
    {
        return logical;
    }

    /**
     * Function SetCategory
     * overrides the logical lib name portion of the LPID to @a aLogical, and can be empty.
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the
     *  character offset into the parameter at which an error was detected, usually
     *  because it contained '/' or ':'.
     */
    int SetLogicalLib( const STRING& aLogical );

    /**
     * Function GetCategory
     * returns the category of this part id, "passives" in the example at the
     * top of the class description.
     */
    const STRING& GetCategory() const
    {
        return category;
    }

    /**
     * Function SetCategory
     * overrides the category portion of the LPID to @a aCategory and is typically
     * either the empty string or a single word like "passives".
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the
     *  character offset into the parameter at which an error was detected, usually
     *  because it contained '/' or ':'.
     */
    int SetCategory( const STRING& aCategory );

    /**
     * Function GetBaseName
     * returns the part name without the category.
     */
    const STRING&  GetBaseName() const
    {
        return baseName;
    }

    /**
     * Function SetBaseName
     * overrides the base name portion of the LPID to @a aBaseName
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the
     *  character offset into the parameter at which an error was detected, usually
     *  because it contained '/' or ':', or is blank.
     */
    int SetBaseName( const STRING& aBaseName );

    /**
     * Function GetPartName
     * returns the part name, i.e. category/baseName without revision.
     */
    const STRING& GetPartName() const
    {
        return partName;
    }

    /**
     * Function GetPartNameAndRev
     * returns the part name with revision if any, i.e. [category/]baseName[/revN..]
     */
    STRING GetPartNameAndRev() const;

    /**
     * Function SetPartName
     * overrides the part name portion of the LPID to @a aPartName
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the
     *  character offset into the parameter at which an error was detected, usually
     *  because it contained more than one '/', or one or more ':', or is blank.
     *  A single '/' is allowed, since that is used to separate the category from the
     *  base name.
     */
    int SetPartName( const STRING& aPartName );

    /**
     * Function GetRevision
     * returns the revision portion of the LPID.
     */
    const STRING& GetRevision() const
    {
        return revision;
    }

    /**
     * Function SetRevision
     * overrides the revision portion of the LPID to @a aRevision and must
     * be in the form "rev<num>" where "<num>" is "1", "2", etc.
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the
     *  character offset into the parameter at which an error was detected,
     *  because it did not look like "rev23"
     */
    int SetRevision( const STRING& aRevision );

    /**
     * Function Format
     * returns the full text of the LPID.
     */
    STRING  Format() const;

    /**
     * Function Format
     * returns a STRING in the proper format as an LPID for a combination of
     * aLogicalLib, aPartName, and aRevision.
     * @throw PARSE_ERROR if any of the pieces are illegal.
     */
    static STRING Format( const STRING& aLogicalLib, const STRING& aPartName, const STRING& aRevision="" )
        throw( PARSE_ERROR );


#if defined(DEBUG)
    static void Test();
#endif

protected:
    STRING  logical;        ///< logical lib name or empty
    STRING  category;       ///< or empty
    STRING  baseName;       ///< without category
    STRING  revision;       ///< "revN[N..]" or empty
    STRING  partName;       ///< cannot be set directory, set via SetBaseName() & SetCategory()
};

} // namespace SCH

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

static inline const char* EndsWithRev( const STRING& aPartName, char separator = '/' )
{
    return EndsWithRev( aPartName.c_str(),  aPartName.c_str()+aPartName.size(), separator );
}


#endif // SCH_LPID_H_
