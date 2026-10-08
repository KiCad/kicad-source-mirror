/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <iterator>
#include <pcb_track.h>
#include <generators/pcb_tuning_pattern.h>
#include <ki_exception.h>
#include <string_any_map.h>

namespace KICAD_FORMAT::LEGACY
{

enum class TUNING_PROPERTY_TYPE
{
    NUMBER,
    BOOL,
    TEXT,
    POINT,
    LINE_CHAIN
};


struct TUNING_PROPERTY
{
    const char*          m_key;
    TUNING_PROPERTY_TYPE m_type;
    bool                 m_v9;
    bool                 m_v10;
};


// Property schemas from 9.0.0 ccafeabf1503 and 10.0.7 93a8a4827b0b.
inline constexpr TUNING_PROPERTY TUNING_PROPERTIES[] = {
    { "origin", TUNING_PROPERTY_TYPE::POINT, true, true },
    { "update_order", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "tuning_mode", TUNING_PROPERTY_TYPE::TEXT, true, true },
    { "initial_side", TUNING_PROPERTY_TYPE::TEXT, true, true },
    { "last_status", TUNING_PROPERTY_TYPE::TEXT, true, true },
    { "end", TUNING_PROPERTY_TYPE::POINT, true, true },
    { "corner_radius_percent", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "single_sided", TUNING_PROPERTY_TYPE::BOOL, true, true },
    { "rounded", TUNING_PROPERTY_TYPE::BOOL, true, true },
    { "max_amplitude", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "min_amplitude", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "min_spacing", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "target_length_min", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "target_length", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "target_length_max", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "target_skew_min", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "target_skew", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "target_skew_max", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "last_track_width", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "last_diff_pair_gap", TUNING_PROPERTY_TYPE::NUMBER, true, true },
    { "last_netname", TUNING_PROPERTY_TYPE::TEXT, true, true },
    { "last_tuning", TUNING_PROPERTY_TYPE::TEXT, true, false },
    { "override_custom_rules", TUNING_PROPERTY_TYPE::BOOL, true, true },
    { "base_line", TUNING_PROPERTY_TYPE::LINE_CHAIN, true, true },
    { "base_line_coupled", TUNING_PROPERTY_TYPE::LINE_CHAIN, true, true },
    { "is_time_domain", TUNING_PROPERTY_TYPE::BOOL, false, true },
    { "target_delay_min", TUNING_PROPERTY_TYPE::NUMBER, false, true },
    { "target_delay", TUNING_PROPERTY_TYPE::NUMBER, false, true },
    { "target_delay_max", TUNING_PROPERTY_TYPE::NUMBER, false, true },
    { "last_tuning_length", TUNING_PROPERTY_TYPE::NUMBER, false, true },
};


inline bool IsReviewedTuningValue( const wxAny& aValue, TUNING_PROPERTY_TYPE aType )
{
    switch( aType )
    {
    case TUNING_PROPERTY_TYPE::NUMBER:
    {
        double value = 0;
        return ( aValue.CheckType<double>() || aValue.CheckType<int>() || aValue.CheckType<long>()
                 || aValue.CheckType<long long>() )
               && aValue.GetAs( &value ) && std::isfinite( value );
    }
    case TUNING_PROPERTY_TYPE::BOOL: return aValue.CheckType<bool>();
    case TUNING_PROPERTY_TYPE::TEXT: return aValue.CheckType<wxString>() || aValue.CheckType<std::string>();
    case TUNING_PROPERTY_TYPE::POINT: return aValue.CheckType<VECTOR2I>();
    case TUNING_PROPERTY_TYPE::LINE_CHAIN: return aValue.CheckType<SHAPE_LINE_CHAIN>();
    }

    return false;
}


inline bool AreTuningPropertiesReviewed( const STRING_ANY_MAP& aProperties )
{
    for( const auto& [key, value] : aProperties )
    {
        const auto schema = std::find_if( std::begin( TUNING_PROPERTIES ), std::end( TUNING_PROPERTIES ),
                                          [&]( const TUNING_PROPERTY& aProperty )
                                          {
                                              return aProperty.m_v10 && key == aProperty.m_key;
                                          } );

        if( schema == std::end( TUNING_PROPERTIES ) || !IsReviewedTuningValue( value, schema->m_type ) )
            return false;
    }

    return true;
}


inline STRING_ANY_MAP TuningPropertiesForEra( const PCB_GENERATOR& aGenerator, bool aV9 )
{
    const auto pattern = dynamic_cast<const PCB_TUNING_PATTERN*>( &aGenerator );

    if( !pattern )
        THROW_IO_ERROR( wxT( "The target format does not support this generator kind." ) );

    const STRING_ANY_MAP current = pattern->GetProperties();

    if( !AreTuningPropertiesReviewed( current ) )
        THROW_IO_ERROR( wxT( "The tuning generator contains unreviewed properties." ) );

    if( aV9 && pattern->GetSettings().m_isTimeDomain )
        THROW_IO_ERROR( wxT( "Time-domain tuning editability must be removed before writing KiCad 9." ) );

    STRING_ANY_MAP result;

    for( const TUNING_PROPERTY& property : TUNING_PROPERTIES )
    {
        if( !( aV9 ? property.m_v9 : property.m_v10 ) )
            continue;

        if( aV9 && std::string( property.m_key ) == "last_tuning" )
        {
            result.set( property.m_key, pattern->GetTuningInfo() );
        }
        else if( const auto value = current.find( property.m_key ); value != current.end() )
        {
            result.emplace( value->first, value->second );
        }
    }

    return result;
}

} // namespace KICAD_FORMAT::LEGACY
