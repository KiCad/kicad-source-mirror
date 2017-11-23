/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2010-2017 KiCad Developers, see change_log.txt for contributors.
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

#ifndef _LIB_ID_H_
#define _LIB_ID_H_

#include <richio.h>
#include <utf8.h>

/**
 * A logical library item identifier and consists of various portions much like a URI.
 *
 * It consists of of triad of the library nickname, the name of the item in the library,
 * and an optional revision of the item.  This is a generic library identifier that can be
 * used for any type of library that contains multiple named items such as footprint or
 * symbol libraries.
 *
 * Example LIB_ID string:
 * "smt:R_0805/rev0".
 *
 * - "smt" is the logical library name used to look up library information saved in the #LIB_TABLE.
 * - "R" is the name of the item within the library.
 * - "rev0" is the revision, which is optional.  If missing then its delimiter should also not
 *    be present. A revision must begin with "rev" and be followed by at least one or more
 *    decimal digits.
 *
 * @author Dick Hollenbeck
 */
class LIB_ID
{
public:

    LIB_ID() {}

    /**
     * Takes \a aId string and parses it.
     *
     * A typical LIB_ID string consists of a library nickname followed by a library item name.
     * e.g.: "smt:R_0805", or
     * e.g.: "mylib:R_0805", or
     * e.g.: "ttl:7400"
     *
     * @param aId is a string to be parsed into the LIB_ID object.
     */
    LIB_ID( const UTF8& aId );

    LIB_ID( const wxString& aId );

    /**
     * This LIB_ID ctor is a special version which ignores the parsing due to symbol
     * names allowing '/' as a valid character.  This was causing the symbol names to
     * be truncated at the first occurrence of '/' in the symbol name.
     *
     * @param aLibName is the library nickname used to look up the library item in the #LIB_TABLE.
     * @param aLibItemName is the name of the library item which is not parsed by the standard
     *                     LIB_ID::Parse() function.
     * @param aRevision is the revision of the library item.
     */
    LIB_ID( const wxString& aLibName, const wxString& aLibItemName,
            const wxString& aRevision = wxEmptyString );

    /**
     * Parse LIB_ID with the information from @a aId.
     *
     * @param aId is the string to populate the #LIB_ID object.
     *
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the character offset into
     *               aId at which an error was detected.
     */
    int Parse( const UTF8& aId );


    /**
     * Return the logical library name portion of a LIB_ID.
     */
    const UTF8& GetLibNickname() const
    {
        return nickname;
    }

    /**
     * Override the logical library name portion of the LIB_ID to @a aNickname.
     *
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the  character offset
     *               into the parameter at which an error was detected, usually because it
     *               contained '/' or ':'.
     */
    int SetLibNickname( const UTF8& aNickname );

    /**
     * @return the library item name, i.e. footprintName.
     */
    const UTF8& GetLibItemName() const { return item_name; }

    /**
     * Override the library item name portion of the LIB_ID to @a aLibItemName
     *
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the  character offset
     *               into the parameter at which an error was detected, usually because it
     *               contained '/'.
     */
    int SetLibItemName( const UTF8& aLibItemName, bool aTestForRev = true );

    int SetRevision( const UTF8& aRevision );

    const UTF8& GetRevision() const { return revision; }

    UTF8 GetLibItemNameAndRev() const;

    /**
     * @return the fully formatted text of the LIB_ID.
     */
    UTF8 Format() const;

    /**
     * @return a string in the proper format as an LIB_ID for a combination of
     *         aLibNickname, aLibItemName, and aRevision.
     *
     * @throw PARSE_ERROR if any of the pieces are illegal.
     */
    static UTF8 Format( const UTF8& aLibNickname, const UTF8& aLibItemName,
                        const UTF8& aRevision = "" );

    /**
     * @return true is the #LIB_ID is valid.
     *
     * A valid #LIB_ID must have both the library nickname and the library item name defined.
     * The revision field is optional.
     *
     * @note A return value of true does not indicated that the #LIB_ID is a valid #LIB_TABLE
     *       entry.
     */
    bool IsValid() const { return !nickname.empty() && !item_name.empty(); }

    /**
     * @return true if the #LIB_ID only has the #item_name name defined.
     */
    bool IsLegacy() const { return nickname.empty() && !item_name.empty() && revision.empty(); }

    /**
     * Clear the contents of the library nickname, library entry name, and revision strings.
     */
    void clear();

    /**
     * @return a boolean true value if the LIB_ID is empty.  Otherwise return false.
     */
    bool empty() const { return nickname.empty() && item_name.empty() && revision.empty(); }

    /**
     * Compare the contents of LIB_ID objects by performing a std::string comparison of the
     * library nickname, library entry name, and revision strings respectively.
     *
     * @param aLibId is the LIB_ID to compare against.
     *
     * @return -1 if less than \a aLibId, 1 if greater than \a aLibId, and 0 if equal to \a aLibId.
     */
    int compare( const LIB_ID& aLibId ) const;

    bool operator < ( const LIB_ID& aLibId ) const { return this->compare( aLibId ) < 0; }
    bool operator > ( const LIB_ID& aLibId ) const { return this->compare( aLibId ) > 0; }
    bool operator ==( const LIB_ID& aLibId ) const { return this->compare( aLibId ) == 0; }
    bool operator !=( const LIB_ID& aLibId ) const { return !(*this == aLibId); }

    /**
     * Examine \a aLibItemName for invalid #LIB_ID item name characters.
     *
     * @param aLibItemName is the #LIB_ID name to test for illegal characters.
     * @return true if \a aLibItemName contain illegal characters otherwise false.
     */
    static bool HasIllegalChars( const UTF8& aLibItemName );

    /**
     * Replace illegal #LIB_ID item name characters with underscores '_'.
     *
     * @param aLibItemName is the #LIB_ID item name to replace illegal characters.
     * @return the corrected version of \a aLibItemName.
     */
    static UTF8 FixIllegalChars( const UTF8& aLibItemName );

#if defined(DEBUG)
    static void Test();
#endif

protected:
    UTF8    nickname;       ///< The nickname of the library or empty.
    UTF8    item_name;      ///< The name of the entry in the logical library.
    UTF8    revision;       ///< The revision of the entry.
};


#endif // _LIB_ID_H_
