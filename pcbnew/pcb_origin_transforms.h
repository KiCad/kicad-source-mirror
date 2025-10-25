/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Reece R. Pollack <reece@his.com>
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

#pragma once

#include <origin_transforms.h>

class PCB_BASE_FRAME;

class PCB_ORIGIN_TRANSFORMS : public ORIGIN_TRANSFORMS
{
public:
    PCB_ORIGIN_TRANSFORMS( PCB_BASE_FRAME& aPcbBaseFrame );

    ~PCB_ORIGIN_TRANSFORMS() = default;

    using ORIGIN_TRANSFORMS::ToDisplay;

    virtual long long int ToDisplay( long long int aValue, COORD_TYPES_T aCoordType ) const override;
    virtual double ToDisplay( double aValue, COORD_TYPES_T aCoordType ) const override;
    virtual double ToDisplay( const EDA_ANGLE& aValue, COORD_TYPES_T aCoordType ) const override;

    using ORIGIN_TRANSFORMS::FromDisplay;

    virtual long long int FromDisplay( long long int aValue, COORD_TYPES_T aCoordType ) const override;
    virtual double FromDisplay( double aValue, COORD_TYPES_T aCoordType ) const override;
    virtual EDA_ANGLE FromDisplay( const EDA_ANGLE& aValue, COORD_TYPES_T aCoordType ) const override;

    /**
     * Transform a 2-D coordinate point referenced to the internal origin
     * to the equivalent point referenced to the user-selected display origin.
     *
     * @param aValue a point referenced to the internal origin
     * @returns the point re-referenced to the user-selected display origin
     */

    /**
     * Transform a 2-D coordinate point referenced to the user-selected
     * display origin to the equivalent point referenced to the internal origin.
     *
     * @param aValue a point referenced to the user-selected display origin
     * @returns the point re-referenced to the internal origin
     */

    /**
     * Transform a relative 2-D coordinate delta referenced to the user-selected
     * display origin to the equivalent delta referenced to the internal origin.
     *
     * This is initially intended to handle axis inversion of a delta between
     * two display points, but could be extended to handle other transforms.
     *
     * @param aValue a delta referenced to the internal origin
     * @returns the delta re-referenced to the user-selected display origin
     */

    /**
     * Transform a relative 2-D coordinate delta referenced to the user-selected
     * display origin to the equivalent delta referenced to the internal origin.
     *
     * This is initially intended to handle axis inversion of a delta between
     * two display points, but could be extended to handle other transforms.
     *
     * @param aValue a delta referenced to the user-selected display origin
     * @returns the delta re-referenced to the internal origin
     */


    // =============== Single-axis Relative Transforms ===============

    template<typename T>
    T ToDisplayRelX( T aInternalValue ) const
    {
        return ORIGIN_TRANSFORMS::ToDisplayRel( aInternalValue, invertXAxis() );
    }

    template<typename T>
    T ToDisplayRelY( T aInternalValue ) const
    {
        return ORIGIN_TRANSFORMS::ToDisplayRel( aInternalValue, invertYAxis() );
    }

    template<typename T>
    T FromDisplayRelX( T aDisplayValue ) const
    {
        return ORIGIN_TRANSFORMS::FromDisplayRel( aDisplayValue, invertXAxis() );
    }

    template<typename T>
    T FromDisplayRelY( T aDisplayValue ) const
    {
        return ORIGIN_TRANSFORMS::FromDisplayRel( aDisplayValue, invertYAxis() );
    }


    // =============== Single-axis Absolute Transforms ===============

    template<typename T>
    T ToDisplayAbsX( T aInternalValue ) const
    {
        return ORIGIN_TRANSFORMS::ToDisplayAbs( aInternalValue, getUserXOrigin(), invertXAxis() );
    }

    template<typename T>
    T ToDisplayAbsY( T aInternalValue ) const
    {
        return ORIGIN_TRANSFORMS::ToDisplayAbs( aInternalValue, getUserYOrigin(), invertYAxis() );
    }

    template<typename T>
    T FromDisplayAbsX( T aDisplayValue ) const
    {
        return ORIGIN_TRANSFORMS::FromDisplayAbs( aDisplayValue, getUserXOrigin(), invertXAxis() );
    }

    template<typename T>
    T FromDisplayAbsY( T aDisplayValue ) const
    {
        return ORIGIN_TRANSFORMS::FromDisplayAbs( aDisplayValue, getUserYOrigin(), invertYAxis() );
    }


    // =============== Two-axis Transforms ===============

    template<typename T>
    T ToDisplayAbs( T aInternalValue ) const
    {
        T displayValue;

        displayValue.x = ToDisplayAbsX( aInternalValue.x );
        displayValue.y = ToDisplayAbsY( aInternalValue.y );

        return displayValue;
    }

    template<typename T>
    T FromDisplayAbs( T aDisplayValue ) const
    {
        T internalValue;

        internalValue.x = FromDisplayAbsX( aDisplayValue.x );
        internalValue.y = FromDisplayAbsY( aDisplayValue.y );

        return internalValue;
    }

    template<typename T>
    T ToDisplayRel( T aInternalValue ) const
    {
        T displayValue;

        displayValue.x = ToDisplayRelX( aInternalValue.x );
        displayValue.y = ToDisplayRelY( aInternalValue.y );

        return displayValue;
    }

    template<typename T>
    T FromDisplayRel( T aDisplayValue ) const
    {
        T internalValue;

        internalValue.x = FromDisplayRelX( aDisplayValue.x );
        internalValue.y = FromDisplayRelY( aDisplayValue.y );

        return internalValue;
    }

protected:
    int getUserXOrigin() const;
    int getUserYOrigin() const;

    bool invertXAxis() const;
    bool invertYAxis() const;

protected:
    const PCB_BASE_FRAME& m_pcbBaseFrame;
};
