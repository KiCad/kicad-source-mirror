/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCBNEW_ARRAY_OPTIONS__H
#define PCBNEW_ARRAY_OPTIONS__H

#include <math/vector2d.h>

/**
 * Options that govern the setup of an "array" of multiple item.
 * The base #ARRAY_OPTIONS do not encode a specific geometry or numbering
 * method, this is done by derived classes.
 */
class ARRAY_OPTIONS
{
public:
    enum ARRAY_TYPE_T
    {
        ARRAY_GRID,     ///< A grid (x*y) array
        ARRAY_CIRCULAR, ///< A circular array
    };

    // NOTE: do not change order relative to charSetDescriptions
    enum NUMBERING_TYPE_T
    {
        NUMBERING_NUMERIC = 0, ///< Arabic numerals: 0,1,2,3,4,5,6,7,8,9,10,11...
        NUMBERING_HEX,
        NUMBERING_ALPHA_NO_IOSQXZ, /*!< Alphabet, excluding IOSQXZ
                                     *
                                     * Per ASME Y14.35M-1997 sec. 5.2 (previously MIL-STD-100 sec. 406.5)
                                     * as these can be confused with numerals and are often not used
                                     * for pin numbering on BGAs, etc
                                     */
        NUMBERING_ALPHA_FULL,      ///< Full 26-character alphabet
    };

    ARRAY_OPTIONS( ARRAY_TYPE_T aType )
            : m_type( aType ), m_shouldNumber( false ), m_numberingStartIsSpecified( false )
    {
    }

    virtual ~ARRAY_OPTIONS(){};

    /**
     * Get the alphabet for a particular numbering scheme.
     * @param  type the numbering scheme
     * @return      the alphabet (as a string)
     */
    static const wxString& AlphabetFromNumberingScheme( NUMBERING_TYPE_T type );

    /**
     * @return False for schemes like 0,1...9,10
     *         True for schemes like A,B..Z,AA (where the tens column starts with char 0)
     */
    static bool SchemeNonUnitColsStartAt0( NUMBERING_TYPE_T type );

    /**
     * Get the numbering offset for a given numbering string
     * @param  str   a numbering string, say "B" or "5"
     * @param  type  the type this string should be
     * @param  offsetToFill the offset to set, if found
     * @return       true if the string is a valid offset of this type
     */
    static bool GetNumberingOffset(
            const wxString& str, ARRAY_OPTIONS::NUMBERING_TYPE_T type, int& offsetToFill );

    /**
     * Transform applied to an object by this array
     */
    struct TRANSFORM
    {
        VECTOR2I m_offset;
        double   m_rotation; // in degrees
    };

    /**
     * Get the transform of the n-th point in the array
     * @param  aN the index of the array point (0 is the original point)
     * @param  aPos the existing item position
     * @return  a transform (an offset and a rotation)
     */
    virtual TRANSFORM GetTransform( int aN, const VECTOR2I& aPos ) const = 0;

    /**
     * The number of points in this array
     */
    virtual int GetArraySize() const = 0;

    /**
     * Get the position number (name) for the n'th array point
     * @param  n array point index, from 0 to GetArraySize() - 1
     * @return   the point's name
     */
    virtual wxString GetItemNumber( int n ) const = 0;

    /*!
     * @return are the items in this array numbered, or are all the
     * items numbered the same?
     */
    bool ShouldNumberItems() const
    {
        return m_shouldNumber;
    }

    void SetShouldNumber( bool aShouldNumber )
    {
        m_shouldNumber = aShouldNumber;
    }

    /*!
     * @return is the numbering is enabled and should start at a point
     * specified in these options or is it implicit according to the calling
     * code?
     */
    bool GetNumberingStartIsSpecified() const
    {
        return m_shouldNumber && m_numberingStartIsSpecified;
    }

    void SetNumberingStartIsSpecified( bool aIsSpecified )
    {
        m_numberingStartIsSpecified = aIsSpecified;
    }

protected:
    static wxString getCoordinateNumber( int n, NUMBERING_TYPE_T type );

    ARRAY_TYPE_T m_type;

    /// True if this array numbers the new items
    bool m_shouldNumber;

    /// True if this array's number starts from the preset point
    /// False if the array numbering starts from some externally provided point
    bool m_numberingStartIsSpecified;
};


struct ARRAY_GRID_OPTIONS : public ARRAY_OPTIONS
{
    ARRAY_GRID_OPTIONS()
            : ARRAY_OPTIONS( ARRAY_GRID ),
              m_nx( 0 ),
              m_ny( 0 ),
              m_horizontalThenVertical( true ),
              m_reverseNumberingAlternate( false ),
              m_stagger( 0 ),
              m_stagger_rows( true ),
              m_2dArrayNumbering( false ),
              m_numberingOffsetX( 0 ),
              m_numberingOffsetY( 0 ),
              m_priAxisNumType( NUMBERING_NUMERIC ),
              m_secAxisNumType( NUMBERING_NUMERIC )
    {
    }

    long             m_nx, m_ny;
    bool             m_horizontalThenVertical, m_reverseNumberingAlternate;
    VECTOR2I         m_delta;
    VECTOR2I         m_offset;
    long             m_stagger;
    bool             m_stagger_rows;
    bool             m_2dArrayNumbering;
    int              m_numberingOffsetX, m_numberingOffsetY;
    NUMBERING_TYPE_T m_priAxisNumType, m_secAxisNumType;

    TRANSFORM GetTransform( int aN, const VECTOR2I& aPos ) const override;
    int       GetArraySize() const override;
    wxString  GetItemNumber( int n ) const override;

private:
    VECTOR2I getGridCoords( int n ) const;
};


struct ARRAY_CIRCULAR_OPTIONS : public ARRAY_OPTIONS
{
    ARRAY_CIRCULAR_OPTIONS()
            : ARRAY_OPTIONS( ARRAY_CIRCULAR ),
              m_nPts( 0 ),
              m_angle( 0.0f ),
              m_rotateItems( false ),
              m_numberingType( NUMBERING_NUMERIC ),
              m_numberingOffset( 0 )
    {
    }

    /// number of point in the array
    long             m_nPts;
    /// angle between points, or 0 for each point separated by this value (decideg)
    double           m_angle;
    VECTOR2I         m_centre;
    bool             m_rotateItems;
    NUMBERING_TYPE_T m_numberingType;
    long             m_numberingOffset;

    TRANSFORM GetTransform( int aN, const VECTOR2I& aPos ) const override;
    int       GetArraySize() const override;
    wxString  GetItemNumber( int n ) const override;
};


#endif // PCBNEW_ARRAY_OPTIONS__H