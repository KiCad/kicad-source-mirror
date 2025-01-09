/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_PROPERTY_VALIDATORS_H
#define KICAD_PROPERTY_VALIDATORS_H

#include <properties/property_validator.h>
#include <units_provider.h>

/*
 * This file includes some standard validators and errors.  Include it only where needed, use
 * properties/property_validator.h for most things.
 */


template<typename T>
class VALIDATION_ERROR_TOO_LARGE : public VALIDATION_ERROR
{
public:
    T Actual;
    T Maximum;
    EDA_DATA_TYPE DataType;

    VALIDATION_ERROR_TOO_LARGE( T aActual, T aMaximum,
                                EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE ) :
            Actual( aActual ),
            Maximum( aMaximum ),
            DataType( aType )
    {}

    wxString Format( UNITS_PROVIDER* aUnits ) const override
    {
        bool addUnit = DataType != EDA_DATA_TYPE::UNITLESS;
        return wxString::Format( wxS( "Value must be less than or equal to %s" ),
                                 aUnits->StringFromValue( Maximum, addUnit ) );
    }
};


template<typename T>
class VALIDATION_ERROR_TOO_SMALL : public VALIDATION_ERROR
{
public:
    T Actual;
    T Minimum;
    EDA_DATA_TYPE DataType;

    VALIDATION_ERROR_TOO_SMALL( T aActual, T aMinimum,
                                EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE ) :
            Actual( aActual ),
            Minimum( aMinimum ),
            DataType( aType )
    {}

    wxString Format( UNITS_PROVIDER* aUnits ) const override
    {
        bool addUnit = DataType != EDA_DATA_TYPE::UNITLESS;
        return wxString::Format( wxS( "Value must be greater than or equal to %s" ),
                                 aUnits->StringFromValue( Minimum, addUnit ) );
    }
};


/**
 * A validator for use when you just need to return an error string rather than also packaging some
 * other data (for example, a limit number)
 */
class VALIDATION_ERROR_MSG : public VALIDATION_ERROR
{
public:
    wxString Message;

    VALIDATION_ERROR_MSG( const wxString& aMessage ) : Message( aMessage ) {}

    wxString Format( UNITS_PROVIDER* aUnits ) const override
    {
        return Message;
    }
};


/**
 * A set of generic validators
 */
class PROPERTY_VALIDATORS
{
public:

    template<int Min, int Max>
    static VALIDATOR_RESULT RangeIntValidator( const wxAny&& aValue, EDA_ITEM* aItem )
    {
        wxASSERT_MSG( aValue.CheckType<int>() || aValue.CheckType<std::optional<int>>(),
                      "Expecting int-containing value" );

        int val = 0;

        if( aValue.CheckType<int>() )
        {
            val = aValue.As<int>();
        }
        else if( aValue.CheckType<std::optional<int>>() )
        {
            if( aValue.As<std::optional<int>>().has_value() )
                val = aValue.As<std::optional<int>>().value();
            else
                return std::nullopt;     // no value for a std::optional is always valid
        }

        if( val > Max )
            return std::make_unique<VALIDATION_ERROR_TOO_LARGE<int>>( val, Max );
        else if( val < Min )
            return std::make_unique<VALIDATION_ERROR_TOO_SMALL<int>>( val, Min );

        return std::nullopt;
    }

    static VALIDATOR_RESULT PositiveIntValidator( const wxAny&& aValue, EDA_ITEM* aItem )
    {
        wxASSERT_MSG( aValue.CheckType<int>() || aValue.CheckType<std::optional<int>>(),
                      "Expecting int-containing value" );

        int val = 0;

        if( aValue.CheckType<int>() )
        {
            val = aValue.As<int>();
        }
        else if( aValue.CheckType<std::optional<int>>() )
        {
            if( aValue.As<std::optional<int>>().has_value() )
                val = aValue.As<std::optional<int>>().value();
            else
                return std::nullopt;     // no value for a std::optional is always valid
        }

        if( val < 0 )
            return std::make_unique<VALIDATION_ERROR_TOO_SMALL<int>>( val, 0 );

        return std::nullopt;
    }

    static VALIDATOR_RESULT PositiveRatioValidator( const wxAny&& aValue, EDA_ITEM* aItem )
    {
        wxASSERT_MSG( aValue.CheckType<double>(), "Expecting double-containing value" );

        double val = aValue.As<double>();

        if( val > 1.0 )
        {
            return std::make_unique<VALIDATION_ERROR_TOO_LARGE<double>>( val, 1.0,
                                                                         EDA_DATA_TYPE::UNITLESS );
        }
        else if( val < 0.0 )
        {
            return std::make_unique<VALIDATION_ERROR_TOO_SMALL<double>>( val, 0.0,
                                                                         EDA_DATA_TYPE::UNITLESS );
        }

        return std::nullopt;
    }
};

#endif //KICAD_PROPERTY_VALIDATORS_H
