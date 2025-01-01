/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kicommon.h>
#include <core/utf8.h>

/**
 * A logical library item identifier and consists of various portions much like a URI.
 *
 * It consists of of a dyad of the library nickname and the name of the item in the library
 * This is a generic library identifier that can be
 * used for any type of library that contains multiple named items such as footprint or
 * symbol libraries.
 *
 * Example LIB_ID string:
 * "smt:R_0805".
 *
 * - "smt" is the logical library name used to look up library information saved in the #LIB_TABLE.
 * - "R" is the name of the item within the library.
 *
 * @author Dick Hollenbeck
 */
class KICOMMON_API LIB_ID
{
public:
    LIB_ID() {}

    // NOTE: don't define any constructors which call Parse() on their arguments.  We want it
    // to be obvious to callers that parsing is involved (and that valid IDs are guaranteed in
    // the presence of disallowed characters, malformed ids, etc.).

    /**
     * This LIB_ID ctor is a special version which ignores the parsing due to symbol
     * names allowing '/' as a valid character.  This was causing the symbol names to
     * be truncated at the first occurrence of '/' in the symbol name.
     *
     * @param aLibraryName is the library name used to look up the library item in the #LIB_TABLE.
     * @param aItemName is the name of the library item which is not parsed by the standard
     *                     LIB_ID::Parse() function.
     */
    LIB_ID( const wxString& aLibraryName, const wxString& aItemName );

    /**
     * Parse LIB_ID with the information from @a aId.
     *
     * A typical LIB_ID string consists of a library nickname followed by a library item name.
     * e.g.: "smt:R_0805", or
     * e.g.: "mylib:R_0805", or
     * e.g.: "ttl:7400"
     *
     * @param aId is the string to populate the #LIB_ID object.
     * @param aFix indicates invalid chars should be replaced with '_'.
     *
     * @return minus 1 (i.e. -1) means success, >= 0 indicates the character offset into
     *         aId at which an error was detected.
     */
    int Parse( const UTF8& aId, bool aFix = false );

    /**
     * Return the logical library name portion of a LIB_ID.
     */
    const UTF8& GetLibNickname() const { return m_libraryName; }
    const wxString GetUniStringLibNickname() const { return m_libraryName.wx_str(); }

    /**
     * Override the logical library name portion of the LIB_ID to @a aLibNickname.
     *
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the  character offset
     *               into the parameter at which an error was detected, usually because it
     *               contained '/' or ':'.
     */
    int SetLibNickname( const UTF8& aLibNickname );

    /**
     * @return the library item name, i.e. footprintName, in UTF8.
     */
    const UTF8& GetLibItemName() const { return m_itemName; }

    /**
     * Get strings for display messages in dialogs.
     *
     * Equivalent to m_itemName.wx_str(), but more explicit when building a Unicode string
     * in messages.
     *
     * @return the library item name, i.e. footprintName in a wxString (UTF16 or 32).
     */
    const wxString GetUniStringLibItemName() const { return m_itemName.wx_str(); }

    /**
     * Override the library item name portion of the LIB_ID to @a aLibItemName
     *
     * @return int - minus 1 (i.e. -1) means success, >= 0 indicates the  character offset
     *               into the parameter at which an error was detected, usually because it
     *               contained '/'.
     */
    int SetLibItemName( const UTF8& aLibItemName );

    /**
     * Some LIB_IDs can have a sub-library identifier in addition to a library nickname.
     * This identifier is *not* part of the canonical LIB_ID and is not written out / parsed.
     * It is only used for internal sorting/grouping, if present.
     *
     * @return the sub-library name for this LIB_ID, if one exists
     */
    UTF8 GetSubLibraryName() const { return m_subLibraryName; }
    void SetSubLibraryName( const UTF8& aName ) { m_subLibraryName = aName; }
    const wxString GetUniStringSubLibraryName() const { return m_subLibraryName.wx_str(); }

    /**
     * @return a display-formatted name of the library and sublibrary (if present)
     */
    const wxString GetFullLibraryName() const;

    /**
     * @return the fully formatted text of the LIB_ID in a UTF8 string.
     */
    UTF8 Format() const;

    /**
     * @return the fully formatted text of the LIB_ID in a wxString (UTF16 or UTF32),
     *         suitable to display the LIB_ID in dialogs.
     */
    wxString GetUniStringLibId() const
    {
        return Format().wx_str();
    }

    /**
     * @return a string in the proper format as an LIB_ID for a combination of
     *         aLibraryName, aLibItemName
     *
     * @throw PARSE_ERROR if any of the pieces are illegal.
     */
    static UTF8 Format( const UTF8& aLibraryName, const UTF8& aLibItemName );

    /**
     * Check if this LID_ID is valid.
     *
     * A valid #LIB_ID must have both the library nickname and the library item name defined.
     *
     * @note A return value of true does not indicated that the #LIB_ID is a valid #LIB_TABLE
     *       entry.
     *
     * @return true is the #LIB_ID is valid.
     *
     */
    bool IsValid() const
    {
        return !m_libraryName.empty() && !m_itemName.empty();
    }

    /**
     * @return true if the #LIB_ID only has the #m_itemName name defined.
     */
    bool IsLegacy() const
    {
        return m_libraryName.empty() && !m_itemName.empty();
    }

    /**
     * Clear the contents of the library nickname, library entry name
     */
    void clear();

    /**
     * @return a boolean true value if the LIB_ID is empty.  Otherwise return false.
     */
    bool empty() const
    {
        return m_libraryName.empty() && m_itemName.empty();
    }

    /**
     * Compare the contents of LIB_ID objects by performing a std::string comparison of the
     * library nickname, library entry name
     *
     * @param aLibId is the LIB_ID to compare against.
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
     * @return offset of first illegal character otherwise -1.
     */
    static int HasIllegalChars( const UTF8& aLibItemName );

    /**
     * Replace illegal #LIB_ID item name characters with underscores '_'.
     *
     * @param aLibItemName is the #LIB_ID item name to replace illegal characters.
     * @param aLib True if we are checking library names, false if we are checking item names
     * @return the corrected version of \a aLibItemName.
     */
    static UTF8 FixIllegalChars( const UTF8& aLibItemName, bool aLib );

    /**
     * Looks for characters that are illegal in library nicknames.
     *
     * @param aLibraryName is the logical library name to be tested.
     * @return Invalid character found in the name or 0 is the name is valid.
     */
    static unsigned FindIllegalLibraryNameChar( const UTF8& aLibraryName );

protected:
    /**
     * Tests whether a Unicode character is a legal LIB_ID item name character.
     *
     * The criteria for legal LIB_ID character is as follows:
     * - For both symbol and footprint names, neither '/' or ':' are legal.  They are
     *   reserved characters used by #LIB_ID::Parse.
     * - Spaces are allowed in footprint names as they are a legal filename character
     *   on all operating systems.
     * - Spaces are not allowed in symbol names since symbol names are not quoted in the
     *   schematic or symbol library file formats.
     * - Spaces are allowed in footprint library nicknames as they are quoted in the
     *   footprint library table file format.
     * - Spaces are now also allowed in symbol library nicknames since they are quoted in
     *   the new symbol library sexpr file format.
     * - Illegal file name characters are not allowed in footprint names since the file
     *   name is the footprint name.
     * - Illegal file name characters except '/' are allowed in symbol names since the
     *   name is not the file name.
     *
     *
     * @note @a aUniChar is expected to be a 32 bit Unicode character, not a UTF8 char, that use
     *                   a variable length coding value.
     */
    static bool isLegalChar( unsigned aUniChar );

    /**
     * Tests whether a Unicode character is a legal LIB_ID library nickname character
     *
     * @note @a aUniChar is expected to be a 32 bit Unicode character, not a UTF8 char, that use
     *                   a variable length coding value.
     */
    static bool isLegalLibraryNameChar( unsigned aUniChar );

    UTF8    m_libraryName;    ///< The nickname of the library or empty.
    UTF8    m_itemName;       ///< The name of the entry in the logical library.
    UTF8    m_subLibraryName; ///< Optional sub-library name used for grouping within a library
};


#endif // _LIB_ID_H_
