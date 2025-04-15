/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams <mike@mikebwilliams.com>
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

#include <settings/grid_settings.h>
#include <json_common.h>
#include <wx/translation.h>
#include <core/json_serializers.h>

#include <units_provider.h>

wxString GRID::MessageText( EDA_IU_SCALE aScale, EDA_UNITS aUnits, bool aDisplayUnits ) const
{
    EDA_DATA_TYPE type = EDA_DATA_TYPE::DISTANCE;

    wxString xStr = EDA_UNIT_UTILS::UI::MessageTextFromValue(
                        aScale, aUnits,
                        EDA_UNIT_UTILS::UI::DoubleValueFromString( aScale, EDA_UNITS::MM, x, type ),
                        aDisplayUnits );
    wxString yStr = EDA_UNIT_UTILS::UI::MessageTextFromValue(
                        aScale, aUnits,
                        EDA_UNIT_UTILS::UI::DoubleValueFromString( aScale, EDA_UNITS::MM, y, type ),
                        aDisplayUnits );

    if( xStr == yStr )
        return xStr;

    return wxString::Format( wxS( "%s x %s" ), xStr, yStr );
}

wxString GRID::UserUnitsMessageText( UNITS_PROVIDER* aProvider, bool aDisplayUnits ) const
{
    return MessageText( aProvider->GetIuScale(), aProvider->GetUserUnits(), aDisplayUnits );
}


VECTOR2D GRID::ToDouble( EDA_IU_SCALE aScale ) const
{
    return VECTOR2D( EDA_UNIT_UTILS::UI::DoubleValueFromString( aScale, EDA_UNITS::MM, x ),
                     EDA_UNIT_UTILS::UI::DoubleValueFromString( aScale, EDA_UNITS::MM, y ) );
}


bool GRID::operator==( const GRID& aOther ) const
{
    return x == aOther.x && y == aOther.y && name == aOther.name;
}


bool operator!=( const GRID& lhs, const GRID& rhs )
{
    return !( lhs == rhs );
}


bool operator<( const GRID& lhs, const GRID& rhs )
{
    return lhs.name < rhs.name;
}


void to_json( nlohmann::json& j, const GRID& g )
{
    j = nlohmann::json{
        { "name", g.name },
        { "x", g.x },
        { "y", g.y },
    };
}


void from_json( const nlohmann::json& j, GRID& g )
{
    j.at( "name" ).get_to( g.name );
    j.at( "x" ).get_to( g.x );
    j.at( "y" ).get_to( g.y );
}
