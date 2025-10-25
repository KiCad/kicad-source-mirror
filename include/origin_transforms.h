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

class EDA_ANGLE;

/**
 * A class to perform either relative or absolute display origin
 * transforms for a single axis of a point.
 *
 * The coordinate argument is transformed between an offset from
 * the internal origin and an offset from the user-specified origin
 * and coordinate direction.
 *
 * The functions are templated to allow use with any size scalar
 * parameter: an int, a long long int, or a double.
 */
class ORIGIN_TRANSFORMS
{
public:
    /**
     * The supported Display Origin Transform types
     *
     * Absolute coordinates require both translation and direction
     * inversion. Relative coordinates require only direction inversion.
     */
    enum COORD_TYPES_T {
        NOT_A_COORD,    //< A non-coordinate value, never transformed
        ABS_X_COORD,    //< An absolute X coordinate
        ABS_Y_COORD,    //< An absolute Y coordinate
        REL_X_COORD,    //< A relative X coordinate
        REL_Y_COORD,    //< A relative Y coordinate
    };

    ~ORIGIN_TRANSFORMS() = default;

    virtual int ToDisplay( int aValue, COORD_TYPES_T aCoordType ) const;
    virtual long long int ToDisplay( long long int aValue, COORD_TYPES_T aCoordType ) const;
    virtual double ToDisplay( double aValue, COORD_TYPES_T aCoordType ) const;
    virtual double ToDisplay( const EDA_ANGLE& aValue, COORD_TYPES_T aCoordType ) const;

    virtual int FromDisplay( int aValue, COORD_TYPES_T aCoordType ) const;
    virtual long long int FromDisplay( long long int aValue, COORD_TYPES_T aCoordType ) const;
    virtual double FromDisplay( double aValue, COORD_TYPES_T aCoordType ) const;
    virtual EDA_ANGLE FromDisplay( const EDA_ANGLE& aValue, COORD_TYPES_T aCoordType ) const;

    template<class T>
    T ToDisplayAbs( const T& aValue ) const
    {
        T displayValue;

        displayValue.x = ToDisplay( aValue.x, ABS_X_COORD );
        displayValue.y = ToDisplay( aValue.y, ABS_Y_COORD );
        return displayValue;
    }

    template<class T>
    T ToDisplayRel( const T& aValue ) const
    {
        T displayValue;

        displayValue.x = ToDisplay( aValue.x, REL_X_COORD );
        displayValue.y = ToDisplay( aValue.y, REL_Y_COORD );
        return displayValue;
    }

    template<class T>
    T FromDisplayAbs( const T& aValue ) const
    {
        T displayValue;

        displayValue.x = FromDisplay( aValue.x, ABS_X_COORD );
        displayValue.y = FromDisplay( aValue.y, ABS_Y_COORD );
        return displayValue;
    }

    template<class T>
    T FromDisplayRel( const T& aValue ) const
    {
        T displayValue;

        displayValue.x = FromDisplay( aValue.x, REL_X_COORD );
        displayValue.y = FromDisplay( aValue.y, REL_Y_COORD );
        return displayValue;
    }

protected:
    template<class T>
    inline static T ToDisplayRel( T aInternalValue, bool aInvertAxis )
    {
        T displayValue = aInternalValue;

       // Invert the direction if needed
        if( aInvertAxis && ( displayValue != static_cast<T>( 0 ) ) )
            displayValue = -displayValue;

        return displayValue;
    }

    template<class T>
    inline static T FromDisplayRel( T aDisplayValue, bool aInvertAxis )
    {
        T internalValue = aDisplayValue;

       // Invert the direction if needed
        if( aInvertAxis && ( internalValue != static_cast<T>( 0 ) ) )
            internalValue = -internalValue;

        return internalValue;
    }

    template<class T>
    inline static T ToDisplayAbs( T aInternalValue, int aUserOrigin, bool aInvertAxis )
    {
        T displayValue = aInternalValue;

        // Make the value relative to the internal origin
        displayValue -= aUserOrigin;

        // Invert the direction if needed
        if( aInvertAxis && ( displayValue != static_cast<T>( 0 ) ) )
            displayValue = -displayValue;

        return displayValue;
    }

    template<class T>
    inline static T FromDisplayAbs( T aDisplayValue, int aUserOrigin, bool aInvertAxis )
    {
        T internalValue = aDisplayValue;

        // Invert the direction if needed
        if( aInvertAxis && ( internalValue != static_cast<T>( 0 ) ) )
            internalValue = -internalValue;

        // Make the value relative to the internal origin
        internalValue += aUserOrigin;

        return internalValue;
    }
};
