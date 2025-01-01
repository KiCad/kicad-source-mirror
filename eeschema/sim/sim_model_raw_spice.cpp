/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sim/sim_model_raw_spice.h>
#include <sim/sim_model_serializer.h>

#include <boost/algorithm/string/predicate.hpp>
#include <fmt/core.h>
#include <pegtl/contrib/parse_tree.hpp>


namespace SIM_MODEL_RAW_SPICE_PARSER
{
    using namespace SIM_MODEL_SERIALIZER_GRAMMAR;

    template <typename Rule> struct legacyPinSequenceSelector : std::false_type {};
    template <> struct legacyPinSequenceSelector<legacyPinNumber> : std::true_type {};
}


std::string SPICE_GENERATOR_RAW_SPICE::ModelLine( const SPICE_ITEM& aItem ) const
{
    return "";
}


std::string SPICE_GENERATOR_RAW_SPICE::ItemName( const SPICE_ITEM& aItem ) const
{
    std::string type = m_model.GetParam( (int) SIM_MODEL_RAW_SPICE::SPICE_PARAM::TYPE ).value;

    if( aItem.refName != "" && boost::starts_with( aItem.refName, type ) )
        return aItem.refName;
    else
        return fmt::format( "{}{}", type, aItem.refName );
}


std::string SPICE_GENERATOR_RAW_SPICE::ItemPins( const SPICE_ITEM& aItem ) const
{
    std::string result;
    int ncCounter = 0;

    if( !GetPins().empty() )
    {
        for( const SIM_MODEL_PIN& pin : GetPins() )
        {
            auto it = std::find( aItem.pinNumbers.begin(), aItem.pinNumbers.end(),
                                 pin.symbolPinNumber );

            if( it != aItem.pinNumbers.end() )
            {
                long symbolPinIndex = std::distance( aItem.pinNumbers.begin(), it );
                result.append( fmt::format( " {}", aItem.pinNetNames.at( symbolPinIndex ) ) );
            }
            else
            {
                result.append( fmt::format( " NC-{}-{}", aItem.refName, ncCounter++ ) );
            }
        }
    }
    else
    {
        // If we don't know what pins the model has, just output the symbol's pins

        for( const std::string& pinNetName : aItem.pinNetNames )
            result.append( fmt::format( " {}", pinNetName ) );
    }

    return result;
}


std::string SPICE_GENERATOR_RAW_SPICE::ItemModelName( const SPICE_ITEM& aItem ) const
{
    return "";
}


std::string SPICE_GENERATOR_RAW_SPICE::ItemParams() const
{
    std::string result;

    for( int ii = 0; ii < m_model.GetParamCount(); ++ii )
    {
        const SIM_MODEL::PARAM& param = m_model.GetParam( ii );

        if( !param.info.isSpiceInstanceParam )
            continue;

        if( param.info.name == "model" )
            result.append( " " + SIM_VALUE::ToSpice( param.value ) );
    }

    return result;
}


std::string SPICE_GENERATOR_RAW_SPICE::Preview( const SPICE_ITEM& aItem ) const
{
    return ItemParams();
}


SIM_MODEL_RAW_SPICE::SIM_MODEL_RAW_SPICE( const std::string& aSpiceSource ) :
    SIM_MODEL( TYPE::RAWSPICE, std::make_unique<SPICE_GENERATOR_RAW_SPICE>( *this ) ),
    m_spiceCode( aSpiceSource )
{
    static std::vector<PARAM::INFO> paramInfos = makeParamInfos();

    for( const PARAM::INFO& paramInfo : paramInfos )
        AddParam( paramInfo );

    SetParamValue( "model", aSpiceSource );
}


void SIM_MODEL_RAW_SPICE::AssignSymbolPinNumberToModelPin( const std::string& aModelPinName,
                                                           const wxString& aSymbolPinNumber )
{
    // SPICE doesn't name model inputs so we have to assume they're indexes here.
    int pinIndex = (int) strtol( aModelPinName.c_str(), nullptr, 10 );

    if( pinIndex > 0 )
    {
        while( (int)m_modelPins.size() < pinIndex )
            m_modelPins.push_back( { fmt::format( "{}", m_modelPins.size() + 1 ), wxEmptyString } );

        m_modelPins[ --pinIndex /* convert to 0-based */ ].symbolPinNumber = aSymbolPinNumber;
    }
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_RAW_SPICE::makeParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;

    for( SPICE_PARAM spiceParam : SPICE_PARAM_ITERATOR() )
    {
        PARAM::INFO paramInfo;

        switch( spiceParam )
        {
        case SPICE_PARAM::TYPE:
            paramInfo.name = "type";
            paramInfo.type = SIM_VALUE::TYPE_STRING;
            paramInfo.unit = "";
            paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
            paramInfo.defaultValue = "";
            paramInfo.description = "Spice element type";
            paramInfo.isSpiceInstanceParam = true;

            paramInfos.push_back( paramInfo );
            break;

        case SPICE_PARAM::MODEL:
            paramInfo.name = "model";
            paramInfo.type = SIM_VALUE::TYPE_STRING;
            paramInfo.unit = "";
            paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
            paramInfo.defaultValue = "";
            paramInfo.description = "Model name or value";
            paramInfo.isSpiceInstanceParam = true;

            paramInfos.push_back( paramInfo );
            break;

        case SPICE_PARAM::LIB:
            paramInfo.name = "lib";
            paramInfo.type = SIM_VALUE::TYPE_STRING;
            paramInfo.unit = "";
            paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
            paramInfo.defaultValue = "";
            paramInfo.description = "Library path to include";
            paramInfo.isSpiceInstanceParam = true;

            paramInfos.push_back( paramInfo );
            break;

        case SPICE_PARAM::_ENUM_END:
            break;
        }
    }

    return paramInfos;
}
