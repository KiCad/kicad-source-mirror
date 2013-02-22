/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
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

#ifndef _FPID_H_
#define _FPID_H_

#include <richio.h>


/**
 * Class FPID
 * is a Logical Footprint ID and consists of various portions much like a URI.
 * It is a container for the separated portions of a logical footprint id so they
 * can be accessed individually.  The various portions of an FPID are:
 * logicalLibraryName (nick name), footprint name, and revision.  The logical library
 * name and the footprint name are mandatory.  The revision is optional and currently is
 * not used.
 *
 * Example FPID string:
 * "smt:R_0805/rev0".
 *
 * <p>
 * <ul>
 * <li> "smt" is the logical library name used to look up library information saved in the
 *      #FP_LIB_TABLE.
 * <li> "R" is the name of the footprint within the library.
 * <li> "rev0" is the revision, which is optional.  If missing then its
 *      / delimiter should also not be present. A revision must begin with
 *      "rev" and be followed by at least one or more decimal digits.
 * </ul>
 *
 * @author Dick Hollenbeck
 */
class FPID  // aka GUID
{
public:

    FPID() {}

    /**
     * Constructor FPID
     * takes \a aId string and parses it.  A typical FPID string consists of a
     * library nickname followed by a footprint name.
     * e.g.: "smt:R_0805", or
     * e.g.: "mylib:R_0805"
     *
     * @param aId is a string to be parsed into the FPID object.
     */
    FPID( const std::string& aId ) throw( PARSE_ERROR );

    /**
     * Function Parse
     * [re-]stuffs this FPID with the information from @a aId.
     *
     * @param aId is the string to populate the #FPID object.
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the character offset into
     *               aId at which an error was detected.
     */
    int Parse( const std::string& aId );

    /**
     * Function GetLibNickname
     * returns the logical library name  portion of a FPID.
     */
    const std::string& GetLibNickname() const
    {
        return nickname;
    }

    /**
     * Function SetLibNickname
     * overrides the logical footprint library name portion of the FPID to @a aNickname.
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the  character offset
     *               into the parameter at which an error was detected, usually because it
     *               contained '/' or ':'.
     */
    int SetLibNickname( const std::string& aNickname );

    /**
     * Function GetFootprintName
     * returns the footprint name, i.e. footprintName.
     */
    const std::string& GetFootprintName() const;

    /**
     * Function SetFootprintName
     * overrides the footprint name portion of the FPID to @a aFootprintName
     */
    int SetFootprintName( const std::string& aFootprintName );

    int SetRevision( const std::string& aRevision );

    const std::string& GetRevision() const { return revision; }

    std::string GetFootprintNameAndRev() const;

    /**
     * Function Format
     * returns the fully formatted text of the FPID.
     */
    std::string Format() const;

    /**
     * Function Format
     * returns a wxString in the proper format as an FPID for a combination of
     * aLibNickname, aFootprintName, and aRevision.
     *
     * @throw PARSE_ERROR if any of the pieces are illegal.
     */
    static std::string Format( const std::string& aLibNickname, const std::string& aFootprintName,
                               const std::string& aRevision )
        throw( PARSE_ERROR );

    void clear();

#if defined(DEBUG)
    static void Test();
#endif

protected:
    std::string    nickname;       ///< The nickname of the footprint library or empty.
    std::string    footprint;      ///< The name of the footprint in the logical library.
    std::string    revision;       ///< The footprint revision.
};


#endif // _FPID_H_
