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

#ifndef  UNITS_PROVIDER_H
#define  UNITS_PROVIDER_H

#include <eda_units.h>
#include <origin_transforms.h>
#include <core/minoptmax.h>
#include <optional>
#include <utility>


class UNITS_PROVIDER
{
public:
    UNITS_PROVIDER( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits ) :
            m_iuScale( aIuScale ),
            m_userUnits( aUnits )
    {}

    virtual ~UNITS_PROVIDER()
    {}

    EDA_UNITS GetUserUnits() const { return m_userUnits; }
    void SetUserUnits( EDA_UNITS aUnits ) { m_userUnits = aUnits; }

    virtual void GetUnitPair( EDA_UNITS& aPrimaryUnit, EDA_UNITS& aSecondaryUnits )
    {
        aPrimaryUnit    = GetUserUnits();
        aSecondaryUnits = EDA_UNIT_UTILS::IsImperialUnit( aPrimaryUnit ) ? EDA_UNITS::MM
                                                                         : EDA_UNITS::MILS;
    }

    const EDA_IU_SCALE& GetIuScale() const { return m_iuScale; }
    // No SetIuScale(); scale is invariant

    virtual ORIGIN_TRANSFORMS& GetOriginTransforms()
    {
        static ORIGIN_TRANSFORMS identityTransform;

        return identityTransform;
    }

    /**
     * Converts \a aValue in internal units into a united string.
     *
     * For readability, trailing 0s are removed if the mantissa has 3 or more digits.
     * This function should be used to display values in dialogs because a value entered in mm
     * (for instance 2.0 mm) could need up to 8 digits mantissa if displayed in inch to avoid
     * truncation or rounding made just by the printf function.
     *
     * @param aValue = value in internal units
     * @param aAddUnitLabel = true to add symbol unit to the string value
     * @param aType is the type of this value, and controls the way the value is converted
     * to a string, and the suitable unit
     * @return A wxString object containing value and optionally the symbol unit (like 2.000 mm)
     */
    wxString StringFromValue( double aValue, bool aAddUnitLabel = false,
                              EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE ) const
    {
        return EDA_UNIT_UTILS::UI::StringFromValue( GetIuScale(), GetUnitsFromType( aType ), aValue,
                                                    aAddUnitLabel, aType );
    }

    /**
     * Converts an optional \a aValue in internal units into a united string.
     *
     * For readability, trailing 0s are removed if the mantissa has 3 or more digits.
     * This function should be used to display values in dialogs because a value entered in mm
     * (for instance 2.0 mm) could need up to 8 digits mantissa if displayed in inch to avoid
     * truncation or rounding made just by the printf function.
     *
     * @param aValue = optional value in internal units
     * @param aAddUnitLabel = true to add symbol unit to the string value
     * @param aType is the type of this value, and controls the way the value is converted
     * to a string, and the suitable unit
     * @return A wxString object containing value and optionally the symbol unit (like 2.000 mm)
     */
    wxString StringFromOptionalValue( std::optional<int> aValue, bool aAddUnitLabel = false,
                                      EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE ) const
    {
        if( !aValue )
        {
            return NullUiString;
        }
        else
        {
            return EDA_UNIT_UTILS::UI::StringFromValue( GetIuScale(), GetUnitsFromType( aType ),
                                                        aValue.value(), aAddUnitLabel, aType );
        }
    }

    wxString StringFromValue( const EDA_ANGLE& aValue, bool aAddUnitLabel = false ) const
    {
        return EDA_UNIT_UTILS::UI::StringFromValue( unityScale, EDA_UNITS::DEGREES,
                                                    aValue.AsDegrees(), aAddUnitLabel,
                                                    EDA_DATA_TYPE::DISTANCE );
    }

    /**
     * A lower-precision version of StringFromValue().
     *
     * Should ONLY be used for status text and messages.  Not suitable for dialogs, files, etc.
     * where the loss of precision matters.
     */
    wxString MessageTextFromValue( double aValue, bool aAddUnitLabel = true,
                                   EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE ) const
    {
        return EDA_UNIT_UTILS::UI::MessageTextFromValue( GetIuScale(), GetUnitsFromType( aType ),
                                                         aValue, aAddUnitLabel, aType );
    }

    wxString MessageTextFromValue( const EDA_ANGLE& aValue, bool aAddUnitLabel = true ) const
    {
        return EDA_UNIT_UTILS::UI::MessageTextFromValue( unityScale, EDA_UNITS::DEGREES,
                                                         aValue.AsDegrees(), aAddUnitLabel,
                                                         EDA_DATA_TYPE::DISTANCE );
    }

    wxString MessageTextFromUnscaledValue( double aValue, bool aAddUnitLabel = true,
                                           EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE ) const
    {
        return EDA_UNIT_UTILS::UI::MessageTextFromValue( unityScale, EDA_UNITS::UNSCALED,
                                                         aValue, aAddUnitLabel, aType );
    }

    wxString MessageTextFromMinOptMax( const MINOPTMAX<int>& aValue,
                                       EDA_DATA_TYPE         aType = EDA_DATA_TYPE::DISTANCE ) const
    {
        return EDA_UNIT_UTILS::UI::MessageTextFromMinOptMax( GetIuScale(), GetUnitsFromType( aType ), aValue );
    };

    /**
     * Converts \a aTextValue in \a aUnits to internal units used by the frame.
     * @warning This utilizes the current locale and will break if decimal formats differ
     * @param aType is the type of this value, and controls the way the string is converted
     * to a value
     *
     * @param aTextValue A reference to a wxString object containing the string to convert.
     * @return internal units value
     */
    int ValueFromString( const wxString& aTextValue, EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE ) const
    {
        double value = EDA_UNIT_UTILS::UI::DoubleValueFromString( GetIuScale(), GetUnitsFromType( aType ),
                                                                  aTextValue, aType );


        return KiROUND<double, int>( value );
    }

    /**
     * Converts \a aTextValue in \a aUnits to internal units used by the frame. Allows the return
     * of an empty optional if the string represents a null value (currently empty string)
     * @warning This utilizes the current locale and will break if decimal formats differ
     * @param aType is the type of this value, and controls the way the string is converted
     * to a value
     *
     * @param aTextValue A reference to a wxString object containing the string to convert.
     * @return internal units value
     */
    std::optional<int> OptionalValueFromString( const wxString& aTextValue,
                                                EDA_DATA_TYPE   aType = EDA_DATA_TYPE::DISTANCE ) const
    {
        // Handle null (empty) values
        if( aTextValue == NullUiString )
            return {};

        double value = EDA_UNIT_UTILS::UI::DoubleValueFromString( GetIuScale(), GetUnitsFromType( aType ),
                                                                  aTextValue, aType );

        return KiROUND<double, int>( value );
    }

    EDA_ANGLE AngleValueFromString( const wxString& aTextValue ) const
    {
        double angle = EDA_UNIT_UTILS::UI::DoubleValueFromString( GetIuScale(), EDA_UNITS::DEGREES,
                                                                  aTextValue );

        return EDA_ANGLE( angle, DEGREES_T );
    }

    /**
     * Gets the units to use in the conversion based on the underlying user units
     */
    EDA_UNITS GetUnitsFromType( EDA_DATA_TYPE aType ) const
    {
        // Get the unit type depending on the requested unit type and the underlying user setting
        if( aType == EDA_DATA_TYPE::TIME )
        {
            return EDA_UNITS::PS;
        }
        else if( aType == EDA_DATA_TYPE::LENGTH_DELAY )
        {
            if( EDA_UNIT_UTILS::IsMetricUnit( GetUserUnits() ) )
                return EDA_UNITS::PS_PER_CM;
            else
                return EDA_UNITS::PS_PER_INCH;
        }

        return GetUserUnits();
    }

    /**
     * Gets the inferred type from the given units. Note: will always return the most simple
     * type (e.g. a DISTANCE rather than AREA or VOLUME for a measurement unit).
     */
    static EDA_DATA_TYPE GetTypeFromUnits( const EDA_UNITS aUnits )
    {
        switch( aUnits )
        {
        case EDA_UNITS::INCH:
        case EDA_UNITS::MM:
        case EDA_UNITS::UM:
        case EDA_UNITS::CM:
        case EDA_UNITS::MILS:
            return EDA_DATA_TYPE::DISTANCE;
        case EDA_UNITS::DEGREES:
        case EDA_UNITS::PERCENT:
        case EDA_UNITS::UNSCALED:
            return EDA_DATA_TYPE::UNITLESS;
        case EDA_UNITS::FS:
        case EDA_UNITS::PS:
            return EDA_DATA_TYPE::TIME;
        case EDA_UNITS::PS_PER_INCH:
        case EDA_UNITS::PS_PER_CM:
        case EDA_UNITS::PS_PER_MM:
            return EDA_DATA_TYPE::LENGTH_DELAY;
        }

        wxString msg = wxString::Format( wxT( "Unhandled unit data type %d" ), static_cast<int>( aUnits ) );
        wxCHECK_MSG( false, EDA_DATA_TYPE::UNITLESS, msg );
        return EDA_DATA_TYPE::UNITLESS; // Note that this is unreachable but g++-12 doesn't know that.
    }

    /// @brief The string that is used in the UI to represent a null value
    static inline const wxString NullUiString = "";

private:
    const EDA_IU_SCALE& m_iuScale;
    EDA_UNITS           m_userUnits;
};

#endif  // UNITS_PROVIDER_H
