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

#include <sch_lib.h>    // STRING

/**
 * Class LPID
 * (aka GUID) is a Logical Part ID and consists of various portions much like a URI.
 * It relies heavily on a logical library name to hide where actual physical library
 * sources reside.  Its static functions serve as managers of the "library table" to
 * map logical library names to actual library sources.
 * <p>
 * Example LPID string:
 * "kicad:passives/R/rev6".
 * <p>
 * <ul>
 * <li> "kicad" is the logical library name.
 * <li> "passives" is the category.
 * <li> "passives/R" is the partname.
 * <li> "rev6" is the revision number, which is optional.  If missing then its
 *      delimiter should also not be present.
 * </ul>
 * @author Dick Hollenbeck
 */
class LPID  // aka GUID
{
public:
    /**
     * Constructor LPID
     * takes aLPID string and parses it.  A typical LPID string uses a logical
     * library name followed by a part name.
     * e.g.: "kicad:passives/R/rev2", or
     * e.g.: "mylib:R33"
     */
    LPID( const STRING& aLPID ) throw( PARSE_ERROR );

    /**
     * Function GetLogicalLib
     * returns the logical library portion of a LPID.  There is not Set accessor
     * for this portion since it comes from the library table and is considered
     * read only here.
     */
    STRING  GetLogicalLib() const;

    /**
     * Function SetCategory
     * overrides the logical lib name portion of the LPID to @a aLogical, and can be empty.
     * @return bool - true unless parameter has ':' or '/' in it.
     */
    bool SetLogicalLib( const STRING& aLogical );

    /**
     * Function GetCategory
     * returns the category of this part id, "passives" in the example at the
     * top of the class description.
     */
    STRING  GetCategory() const;

    /**
     * Function SetCategory
     * overrides the category portion of the LPID to @a aCategory and is typically
     * either the empty string or a single word like "passives".
     * @return bool - true unless parameter has ':' or '/' in it.
     */
    bool SetCategory( const STRING& aCategory );

    /**
     * Function GetBaseName
     * returns the part name without the category.
     */
    STRING  GetBaseName() const;

    /**
     * Function SetBaseName
     * overrides the base name portion of the LPID to @a aBaseName
     * @return bool - true unless parameter has ':' or '/' in it.
     */
    bool SetBaseName( const STRING& aBaseName );

    /**
     * Function GetBaseName
     * returns the part name, i.e. category/baseName without revision.
     */
    STRING  GetPartName() const;

    /**
     * Function SetBaseName
     * overrides the part name portion of the LPID to @a aPartName
     not really needed, partname is an agreggate anyway, just parse a new one.
    void SetPartName( const STRING& aPartName );
     */

    /**
     * Function GetRevision
     * returns the revision portion of the LPID or StrEmpty if none.
     */
    STRING GetRevision() const;

    /**
     * Function SetRevision
     * overrides the revision portion of the LPID to @a aRevision and must
     * be in the form "rev<num>" where "<num>" is "1", "2", etc.
     * @return bool - true unless parameter is not of the form "revN]N..]"
     */
    bool SetRevision( const STRING& aRevision );

    /**
     * Function GetFullText
     * returns the full text of the LPID.
     */
    STRING  GetFullText() const;

#if defined(DEBUG)
    static void Test();
#endif

protected:
    STRING  logical;        ///< logical lib name or empty
    STRING  category;       ///< or empty
    STRING  baseName;       ///< excludes category
    STRING  revision;       ///< "revN[N..]" or empty
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

static inline const char* EndsWithRev( const STRING& aPartName, char separator = '/' )
{
    return EndsWithRev( aPartName.c_str(),  aPartName.c_str()+aPartName.size(), separator );
}


#endif // SCH_LPID_H_
