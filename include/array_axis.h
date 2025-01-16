/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef ARRAY_AXIS__H
#define ARRAY_AXIS__H

#include <kicommon.h>
#include <optional>

#include <wx/string.h>

/**
 * Class that contains information about a single array axis and the numbering
 * of items along that axis.
 *
 * For example, a rectangular grid has two axes, X and Y, but a circular array
 * has only one, that runs around the circle.
 */
class KICOMMON_API ARRAY_AXIS
{
public:
    enum NUMBERING_TYPE
    {
        NUMBERING_NUMERIC = 0, ///< Arabic numerals: 0,1,2,3,4,5,6,7,8,9,10,11...
        NUMBERING_HEX,

        /**
         * Alphabet, excluding IOSQXZ
         *
         * Per ASME Y14.35M-1997 sec. 5.2 (previously MIL-STD-100 sec. 406.5) as these can be
         * confused with numerals and are often not used for pin numbering on BGAs, etc.
         */
        NUMBERING_ALPHA_NO_IOSQXZ,
        NUMBERING_ALPHA_FULL,      ///< Full 26-character alphabet
    };

    /**
     * Check if a numbering type is a numeric type.
     */
    static bool TypeIsNumeric( NUMBERING_TYPE type )
    {
        return type == NUMBERING_NUMERIC || type == NUMBERING_HEX;
    };

    ARRAY_AXIS();

    /**
     * Get the alphabet for the current numbering scheme.
     * @param  type the numbering scheme.
     * @return the alphabet (as a string).
     */
    const wxString& GetAlphabet() const;

    /**
     * Set the axis numbering type.
     */
    void SetAxisType( NUMBERING_TYPE aType );

    /**
     * Set the axis start (as a string, which should decode to a valid index
     * in the alphabet),
     */
    bool SetOffset( const wxString& aOffsetName );

    /**
     * Set the start offset for the series (e.g. 0 to start at 0/A, 4 to start
     * at 4/E).
     *
     * @param aOffset offset of the first item in the.
     */
    void SetOffset( int aOffset );

    /**
     * Get the numbering offset for the axis
     *
     * @return the current offset.
     */
    int GetOffset() const;

    /**
     * Set the skip between consecutive numbers (useful when doing a partial
     * array, e.g. only one side of a connector).
     */
    void SetStep( int aStep );

    /**
     * Get the position number (name) for the n'th axis point
     *
     * @param  n array point index, from 0.
     * @return the point's name.
     */
    wxString GetItemNumber( int n ) const;

private:
    /**
     * Get the numbering offset for a given numbering string
     *
     * @param  str is a numbering string, say "B" or "5".
     * @return the offset, if found, else empty.
     */
    std::optional<int> getNumberingOffset( const wxString& str ) const;

    NUMBERING_TYPE m_type;
    int            m_offset;

    /// Skip every 'n' numbers.
    int m_step;
};

#endif // ARRAY_AXIS__H
