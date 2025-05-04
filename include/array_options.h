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

#ifndef PCBNEW_ARRAY_OPTIONS__H
#define PCBNEW_ARRAY_OPTIONS__H

#include <kicommon.h>
#include <math/vector2d.h>
#include <array_axis.h>
#include <geometry/eda_angle.h>

/**
 * Options that govern the setup of an "array" of multiple item.
 *
 * The base #ARRAY_OPTIONS do not encode a specific geometry or numbering
 * method, this is done by derived classes.
 */
class KICOMMON_API ARRAY_OPTIONS
{
public:
    enum ARRAY_TYPE_T
    {
        ARRAY_GRID,     ///< A grid (x*y) array
        ARRAY_CIRCULAR, ///< A circular array
    };

    ARRAY_OPTIONS( ARRAY_TYPE_T aType ) :
        m_type( aType ),
        m_shouldNumber( false ),
        m_arrangeSelection( false ),
        m_reannotateFootprints( false ),
        m_numberingStartIsSpecified( false )
    {
    }

    virtual ~ARRAY_OPTIONS(){};

    /**
     * Transform applied to an object by this array.
     */
    struct TRANSFORM
    {
        VECTOR2I  m_offset;
        EDA_ANGLE m_rotation;
    };

    /**
     * Get the transform of the n-th point in the array.
     *
     * @param  aN the index of the array point (0 is the original point).
     * @param  aPos the existing item position.
     * @return a transform (an offset and a rotation)/
     */
    virtual TRANSFORM GetTransform( int aN, const VECTOR2I& aPos ) const = 0;

    /**
     * The number of points in this array.
     */
    virtual int GetArraySize() const = 0;

    /**
     * Get the position number (name) for the n'th array point.
     *
     * @param  n array point index, from 0 to GetArraySize() - 1.
     * @return the point's name.
     */
    virtual wxString GetItemNumber( int n ) const = 0;

    /**
     * @return are the items in this array numbered, or are all the items numbered the same?
     */
    bool ShouldNumberItems() const
    {
        return m_shouldNumber;
    }

    void SetShouldNumber( bool aShouldNumber )
    {
        m_shouldNumber = aShouldNumber;
    }

    /**
     * @return true if arranging selection, false if creating an array of copies
     */
    bool ShouldArrangeSelection() const
    {
        return m_arrangeSelection;
    }

    void SetShouldArrangeSelection( bool aShouldArrange )
    {
        m_arrangeSelection = aShouldArrange;
    }

    /**
     * @return are the footprints in this array reannotated to be unique (true), or do they
     * keep the original annotation (false)?
     */
    bool ShouldReannotateFootprints() const
    {
        return m_reannotateFootprints;
    }

    void SetSShouldReannotateFootprints( bool aShouldReannotate )
    {
        m_reannotateFootprints = aShouldReannotate;
    }

    /**
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

    ARRAY_TYPE_T m_type;

    /// True if this array numbers the new items
    bool m_shouldNumber;

    /// True if this array should arrange the selected items instead of creating
    /// an array of copies of the selection
    bool m_arrangeSelection;

    /// True if this array will rename any footprints to be unique
    bool m_reannotateFootprints;

    /// True if this array's number starts from the preset point
    /// False if the array numbering starts from some externally provided point
    bool m_numberingStartIsSpecified;
};


struct KICOMMON_API ARRAY_GRID_OPTIONS : public ARRAY_OPTIONS
{
    ARRAY_GRID_OPTIONS()
            : ARRAY_OPTIONS( ARRAY_GRID ),
              m_centred( false ),
              m_nx( 0 ),
              m_ny( 0 ),
              m_horizontalThenVertical( true ),
              m_reverseNumberingAlternate( false ),
              m_stagger( 0 ),
              m_stagger_rows( true ),
              m_2dArrayNumbering( false )
    {
    }

    // Are the grid positions relative to item (0, 0), or the grid center?
    bool             m_centred;
    long             m_nx, m_ny;
    bool             m_horizontalThenVertical, m_reverseNumberingAlternate;
    VECTOR2I         m_delta;
    VECTOR2I         m_offset;
    long             m_stagger;
    bool             m_stagger_rows;
    bool             m_2dArrayNumbering;
    ARRAY_AXIS       m_pri_axis, m_sec_axis;

    TRANSFORM GetTransform( int aN, const VECTOR2I& aPos ) const override;
    int       GetArraySize() const override;
    wxString  GetItemNumber( int n ) const override;

private:
    VECTOR2I gtItemPosRelativeToItem0( int n ) const;
    VECTOR2I getGridCoords( int n ) const;
};


struct KICOMMON_API ARRAY_CIRCULAR_OPTIONS : public ARRAY_OPTIONS
{
    ARRAY_CIRCULAR_OPTIONS()
            : ARRAY_OPTIONS( ARRAY_CIRCULAR ),
              m_nPts( 0 ),
              m_angle( ANGLE_0 ),
              m_angleOffset( ANGLE_0 ),
              m_clockwise( false ),
              m_rotateItems( false )
    {
    }

    /// number of point in the array
    long             m_nPts;

    /// angle between points, or 0 for each point separated by this value (decideg)
    EDA_ANGLE        m_angle;
    EDA_ANGLE        m_angleOffset;
    bool             m_clockwise;
    VECTOR2I         m_centre;
    bool             m_rotateItems;
    ARRAY_AXIS       m_axis;

    TRANSFORM GetTransform( int aN, const VECTOR2I& aPos ) const override;
    int       GetArraySize() const override;
    wxString  GetItemNumber( int n ) const override;
};


#endif // PCBNEW_ARRAY_OPTIONS__H
