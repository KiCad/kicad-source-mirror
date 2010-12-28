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

//#include <wx/string.h>


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
 * <p>
 * This class owns the <b>library table</b>, which is like fstab in concept and maps logical
 * library name to library URI, type, and options. It has the following columns:
 * <ul>
 * <li> Logical Library Name
 * <li> Library Type
 * <li> Library URI.  The full URI to the library source, form dependent on Type.
 * <li> Options, used for access, such as password
 * </ul>
 * <p>
 * For now, the Library Type can be one of:
 * <ul>
 * <li> "dir"
 * <li> "schematic"  i.e. a parts list from another schematic.
 * <li> "subversion"
 * <li> "bazaar"
 * <li> "http"
 * </ul>
 * <p>
 * For now, the Library URI types needed to support the various types can be one of those
 * shown below, which are typical of each type:
 * <ul>
 * <li> "file://C:/mylibdir"
 * <li> "file://home/user/kicadwork/jtagboard.sch"
 * <li> "svn://kicad.org/partlib/trunk"
 * <li> "http://kicad.org/partlib"
 * </ul>
 * <p>
 * The applicable library table is built up from several additive rows (table fragments),
 * and the final table is a merging of the table fragments. Two anticipated sources of
 * the rows are a personal table, and a schematic resident table.  The schematic
 * resident table rows are considered a higher priority in the final dynamically
 * assembled library table. A row in the schematic contribution to the library table
 * will take precedence over the personal table if there is a collision on logical
 * library name, otherwise the rows simply combine without issue to make up the
 * applicable library table.
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
     * Function GetLogLib
     * returns the logical library portion of a LPID.  There is not Set accessor
     * for this portion since it comes from the library table and is considered
     * read only here.
     */
    STRING  GetLogLib() const;

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
     */
    void SetCategory( const STRING& aCategory );

    /**
     * Function GetRevision
     * returns the revision portion of the LPID or StrEmpty if none.
     */
    STRING GetRevision() const;

    /**
     * Function SetRevision
     * overrides the revision portion of the LPID to @a aRevision and must
     * be in the form "rev<num>" where "<num>" is "1", "2", etc.
     */
    void SetRevision( const STRING& aRevision );

    /**
     * Function GetFullText
     * returns the full text of the LPID.
     */
    STRING  GetFullText() const;
};

#endif // SCH_LPID_H_
