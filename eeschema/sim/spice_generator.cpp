/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <sim/spice_generator.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <fmt/core.h>


std::string SPICE_GENERATOR::ModelName( const SPICE_ITEM& aItem ) const
{
    if( aItem.baseModelName == "" )
        return fmt::format( "__{}", aItem.refName );

    // FIXME: This ModelLine() call is relatively expensive.
    if( ModelLine( aItem ) != "" )
        return fmt::format( "{}.{}", aItem.refName, aItem.baseModelName );

    return aItem.baseModelName;
}


std::string SPICE_GENERATOR::ModelLine( const SPICE_ITEM& aItem ) const
{
    if( !m_model.HasSpiceNonInstanceOverrides() && !m_model.requiresSpiceModelLine() )
        return "";

    std::string result;

    result.append( fmt::format( ".model {} ", aItem.modelName ) );
    size_t indentLength = result.length();

    result.append( fmt::format( "{}\n", m_model.GetSpiceInfo().modelType ) );

    for( const SIM_MODEL::PARAM& param : m_model.GetParams() )
    {
        if( param.info.isSpiceInstanceParam )
            continue;

        std::string name;
        std::string value;

        if( !param.info.spiceModelName.empty() )
        {
            name = param.info.spiceModelName;
        }
        else
        {
            // Because of collisions with instance parameters, we append some model parameters
            // with "_".
            if( boost::ends_with( param.info.name, "_" ) )
                name = name.substr( 0, param.info.name.length() - 1 );
            else
                name = param.info.name;
        }

        value = param.value->ToSpiceString();

        if( value == "" )
            continue;

        result.append( fmt::format( "+{}{}={}\n",
                                    std::string( indentLength - 1, ' ' ),
                                    name,
                                    value ) );
    }

    return result;
}


std::string SPICE_GENERATOR::ItemLine( const SPICE_ITEM& aItem ) const
{
    SPICE_ITEM item = aItem;

    if( item.pinNumbers.empty() )
    {
        for( int i = 0; i < m_model.GetPinCount(); ++i )
            item.pinNumbers.push_back( fmt::format( "{}", i + 1 ) );
    }

    if( item.pinNetNames.empty() )
    {
        for( const SIM_MODEL::PIN& pin : GetPins() )
            item.pinNetNames.push_back( pin.name );
    }

    std::string result;
    result.append( ItemName( aItem ) );
    result.append( ItemPins( aItem ) );
    result.append( ItemModelName( aItem ) );
    result.append( ItemParams() );
    result.append( "\n" );
    return result;
}


std::string SPICE_GENERATOR::ItemName( const SPICE_ITEM& aItem ) const
{
    if( aItem.refName != "" && boost::starts_with( aItem.refName, m_model.GetSpiceInfo().itemType ) )
        return aItem.refName;
    else
        return fmt::format( "{}{}", m_model.GetSpiceInfo().itemType, aItem.refName );
}


std::string SPICE_GENERATOR::ItemPins( const SPICE_ITEM& aItem ) const
{
    std::string result;
    int ncCounter = 0;

    for( const SIM_MODEL::PIN& pin : GetPins() )
    {
        auto it = std::find( aItem.pinNumbers.begin(), aItem.pinNumbers.end(),
                             pin.symbolPinNumber );

        if( it == aItem.pinNumbers.end() )
            result.append( fmt::format( " NC-{}-{}", aItem.refName, ncCounter++ ) );
        else
        {
            long symbolPinIndex = std::distance( aItem.pinNumbers.begin(), it );
            result.append( fmt::format( " {}", aItem.pinNetNames.at( symbolPinIndex ) ) );
        }
    }

    return result;
}


std::string SPICE_GENERATOR::ItemModelName( const SPICE_ITEM& aItem ) const
{
    return fmt::format( " {}", aItem.modelName );
}


std::string SPICE_GENERATOR::ItemParams() const
{
    std::string result;

    for( const SIM_MODEL::PARAM& param : GetInstanceParams() )
    {
        std::string name = param.info.spiceInstanceName.empty() ? param.info.name
                                                                : param.info.spiceInstanceName;
        std::string value = param.value->ToSpiceString();

        if( value != "" )
            result.append( fmt::format( " {}={}", name, value ) );
    }

    return result;
}


std::string SPICE_GENERATOR::TunerCommand( const SPICE_ITEM& aItem,
                                           const SIM_VALUE_FLOAT& aValue ) const
{
    // No tuning available by default.
    return "";
}


std::vector<std::string> SPICE_GENERATOR::CurrentNames( const SPICE_ITEM& aItem ) const
{
    return { fmt::format( "I({})", ItemName( aItem ) ) };
}


std::string SPICE_GENERATOR::Preview( const SPICE_ITEM& aItem ) const
{
    std::string spiceCode = ModelLine( aItem );

    std::string itemLine = ItemLine( aItem );
    if( spiceCode != "" )
        spiceCode.append( "\n" );

    spiceCode.append( itemLine );
    return boost::trim_copy( spiceCode );
}


std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> SPICE_GENERATOR::GetInstanceParams() const
{
    std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> instanceParams;

    for( const SIM_MODEL::PARAM& param : m_model.GetParams() )
    {
        if( param.info.isSpiceInstanceParam )
            instanceParams.emplace_back( param );
    }

    return instanceParams;
}


