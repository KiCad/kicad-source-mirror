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
#include <utf8.h>

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
class FPID
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

    FPID( const wxString& aId ) throw( PARSE_ERROR );

    /**
     * Function Parse
     * [re-]stuffs this FPID with the information from @a aId.
     *
     * @param aId is the string to populate the #FPID object.
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the character offset into
     *               aId at which an error was detected.
     */
    int Parse( const UTF8& aId );


    /**
     * Function GetLibNickname
     * returns the logical library name  portion of a FPID.
     */
    const UTF8& GetLibNickname() const
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
    int SetLibNickname( const UTF8& aNickname );

    /**
     * Function GetFootprintName
     * returns the footprint name, i.e. footprintName.
     */
    const UTF8& GetFootprintName() const { return footprint; }

    /**
     * Function SetFootprintName
     * overrides the footprint name portion of the FPID to @a aFootprintName
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the  character offset
     *               into the parameter at which an error was detected, usually because it
     *               contained '/'.
     */
    int SetFootprintName( const UTF8& aFootprintName );

    int SetRevision( const UTF8& aRevision );

    const UTF8& GetRevision() const { return revision; }

    UTF8 GetFootprintNameAndRev() const;

    /**
     * Function Format
     * returns the fully formatted text of the FPID.
     */
    UTF8 Format() const;

    /**
     * Function Format
     * returns a wxString in the proper format as an FPID for a combination of
     * aLibNickname, aFootprintName, and aRevision.
     *
     * @throw PARSE_ERROR if any of the pieces are illegal.
    static UTF8 Format( const UTF8& aLibNickname, const UTF8& aFootprintName,
                               const UTF8& aRevision = "" )
        throw( PARSE_ERROR );
     */

    /**
     * Function IsValid
     * @return true is the #FPID is valid.
     *
     * A valid #FPID must have both the footprint library nickname and the footprint name
     * defined.  The revision field is optional.
     *
     * @note A return value of true does not indicated that the #FPID is a valid #FP_LIB_TABLE
     *       entry.
     */
    bool IsValid() const { return !nickname.empty() && !footprint.empty(); }

    /**
     * Function IsLegacy
     * @return true if the #FPID only has the #footprint name defined.
     */
    bool IsLegacy() const { return nickname.empty() && !footprint.empty() && revision.empty(); }

    /**
     * Function clear
     * clears the contents of the library nickname, footprint name, and revision strings.
     */
    void clear();

    /**
     * Function empty
     * @return a boolean true value if the FPID is empty.  Otherwise return false.
     */
    bool empty() const { return nickname.empty() && footprint.empty() && revision.empty(); }

    /**
     * Function Compare
     * compares the contents of FPID objects by performing a std::string comparison of the
     * library nickname, footprint name, and revision strings respectively.
     *
     * @param aFPID is the FPID to compare against.
     * @return -1 if less than \a aFPID, 1 if greater than \a aFPID, and 0 if equal to \a aFPID.
     */
    int compare( const FPID& aFPID ) const;

    bool operator < ( const FPID& aFPID ) const { return this->compare( aFPID ) < 0; }
    bool operator > ( const FPID& aFPID ) const { return this->compare( aFPID ) > 0; }
    bool operator ==( const FPID& aFPID ) const { return this->compare( aFPID ) == 0; }
    bool operator !=( const FPID& aFPID ) const { return !(*this == aFPID); }

#if defined(DEBUG)
    static void Test();
#endif

protected:
    UTF8    nickname;       ///< The nickname of the footprint library or empty.
    UTF8    footprint;      ///< The name of the footprint in the logical library.
    UTF8    revision;       ///< The footprint revision.
};


#endif // _FPID_H_
