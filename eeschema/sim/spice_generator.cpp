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


std::string SPICE_GENERATOR::ModelName( const std::string& aRefName,
                                        const std::string& aBaseModelName ) const
{
    if( aBaseModelName == "" )
        return fmt::format( "__{}", aRefName );

    // FIXME: This ModelLine() call is relatively expensive.
    if( ModelLine( aBaseModelName ) != "" )
        return fmt::format( "{}.{}", aRefName, aBaseModelName );

    return aBaseModelName;
}


std::string SPICE_GENERATOR::ModelLine( const std::string& aModelName ) const
{
    if( !m_model.HasSpiceNonInstanceOverrides() && !m_model.requiresSpiceModelLine() )
        return "";

    std::string result = "";

    result.append( fmt::format( ".model {} ", aModelName ) );
    size_t indentLength = result.length();

    result.append( fmt::format( "{}\n", m_model.GetSpiceInfo().modelType ) );

    for( const SIM_MODEL::PARAM& param : m_model.GetParams() )
    {
        if( param.info.isSpiceInstanceParam )
            continue;

        std::string name = ( param.info.spiceModelName == "" ) ?
            param.info.name : param.info.spiceModelName;
        std::string value = param.value->ToSpiceString();

        if( value == "" )
            continue;

        result.append( fmt::format( "+{}{}={}\n", std::string( indentLength - 1, ' ' ),
                                    name, value ) );
    }

    return result;
}


std::string SPICE_GENERATOR::ItemLine( const std::string& aRefName,
                                       const std::string& aModelName ) const
{
    // Use linear symbol pin numbers enumeration. Used in model preview.

    std::vector<std::string> pinNumbers;

    for( int i = 0; i < m_model.GetPinCount(); ++i )
        pinNumbers.push_back( fmt::format( "{}", i + 1 ) );

    return ItemLine( aRefName, aModelName, pinNumbers );
}


std::string SPICE_GENERATOR::ItemLine( const std::string& aRefName,
                                       const std::string& aModelName,
                                       const std::vector<std::string>& aSymbolPinNumbers ) const
{
    std::vector<std::string> pinNetNames;

    for( const SIM_MODEL::PIN& pin : GetPins() )
        pinNetNames.push_back( pin.name );

    return ItemLine( aRefName, aModelName, aSymbolPinNumbers, pinNetNames );
}


std::string SPICE_GENERATOR::ItemLine( const std::string& aRefName,
                                       const std::string& aModelName,
                                       const std::vector<std::string>& aSymbolPinNumbers,
                                       const std::vector<std::string>& aPinNetNames ) const
{
    std::string result;
    result.append( ItemName( aRefName ) );
    result.append( ItemPins( aRefName, aModelName, aSymbolPinNumbers, aPinNetNames ) );
    result.append( ItemModelName( aModelName ) );
    result.append( ItemParams() );
    result.append( "\n" );
    return result;
}


std::string SPICE_GENERATOR::ItemName( const std::string& aRefName ) const
{
    if( aRefName != "" && boost::starts_with( aRefName, m_model.GetSpiceInfo().itemType ) )
        return aRefName;
    else
        return m_model.GetSpiceInfo().itemType + aRefName;
}


std::string SPICE_GENERATOR::ItemPins( const std::string& aRefName,
                                       const std::string& aModelName,
                                       const std::vector<std::string>& aSymbolPinNumbers,
                                       const std::vector<std::string>& aPinNetNames ) const
{
    std::string result;
    int ncCounter = 0;

    for( const SIM_MODEL::PIN& pin : GetPins() )
    {
        auto it = std::find( aSymbolPinNumbers.begin(), aSymbolPinNumbers.end(),
                             pin.symbolPinNumber );

        if( it == aSymbolPinNumbers.end() )
            result.append( fmt::format( " NC-{}-{}", aRefName, ncCounter++ ) );
        else
        {
            long symbolPinIndex = std::distance( aSymbolPinNumbers.begin(), it );
            result.append( " " + aPinNetNames.at( symbolPinIndex ) );
        }
    }

    return result;
}


std::string SPICE_GENERATOR::ItemModelName( const std::string& aModelName ) const
{
    return " " + aModelName;
}


std::string SPICE_GENERATOR::ItemParams() const
{
    std::string result;

    for( const SIM_MODEL::PARAM& param : GetInstanceParams() )
    {
        std::string name = ( param.info.spiceInstanceName == "" ) ?
            param.info.name : param.info.spiceInstanceName;
        std::string value = param.value->ToSpiceString();

        if( value != "" )
            result.append( fmt::format( " {}={}", name, value ) );
    }

    return result;
}


std::string SPICE_GENERATOR::TuningLine( const std::string& aSymbol ) const
{
    // TODO.
    return "";
}


std::vector<std::string> SPICE_GENERATOR::CurrentNames( const std::string& aRefName ) const
{
    return { fmt::format( "I({})", ItemName( aRefName ) ) };
}


std::string SPICE_GENERATOR::Preview( const std::string& aModelName ) const
{
    std::string spiceCode = ModelLine( aModelName );

    std::string itemLine = ItemLine( "", aModelName );
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


