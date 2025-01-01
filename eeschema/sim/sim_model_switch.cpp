/* This program source code file is part of KiCad, a free EDA CAD application.
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

#include <sim/sim_model_switch.h>

#include <fmt/core.h>


std::string SPICE_GENERATOR_SWITCH::ItemLine( const SPICE_ITEM& aItem ) const
{
    std::string result;

    switch( m_model.GetType() )
    {
    case SIM_MODEL::TYPE::SW_V:
    {
        result = SPICE_GENERATOR::ItemLine( aItem );
        break;
    }

    case SIM_MODEL::TYPE::SW_I:
    {
        std::string vsourceName = fmt::format( "V__{}", aItem.refName );

        wxCHECK_MSG( aItem.pinNetNames.size() >= 2, "", wxS( "Missing two pin net names for SW_I" ) );

        // Current switches measure input current through a voltage source.
        result.append( fmt::format( "{0} {1} 0\n", aItem.pinNetNames[0], aItem.pinNetNames[1] ) );

        SPICE_ITEM item = aItem;
        item.modelName = fmt::format( "{0} {1}", vsourceName, aItem.modelName );
        result.append( SPICE_GENERATOR::ItemLine( item ) );
        break;
    }

    default:
        wxFAIL_MSG( wxS( "Unhandled SIM_MODEL type in SIM_MODEL_SWITCH" ) );
        break;
    }

    return result;
}


std::string SPICE_GENERATOR_SWITCH::ItemParams() const
{
    std::string result;

    for( int ii = 0; ii < m_model.GetParamCount(); ++ii )
    {
        const SIM_MODEL::PARAM& param = m_model.GetParam( ii );

        if( !param.info.isSpiceInstanceParam )
            continue;

        // The only instance param is "ic", which is positional.
        std::string value = param.value;

        if( value != "none" )
            result.append( " " + value );
    }

    return result;
}


std::vector<std::reference_wrapper<const SIM_MODEL_PIN>> SPICE_GENERATOR_SWITCH::GetPins() const
{
    switch( m_model.GetType() )
    {
    case SIM_MODEL::TYPE::SW_V:
        return { m_model.GetPin( 2 ), m_model.GetPin( 3 ), m_model.GetPin( 0 ), m_model.GetPin( 1 ) };

    case SIM_MODEL::TYPE::SW_I:
        return { m_model.GetPin( 2 ), m_model.GetPin( 3 ) };

    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_SWITCH" );
        return {};
    }
}


SIM_MODEL_SWITCH::SIM_MODEL_SWITCH( TYPE aType ) :
    SIM_MODEL( aType,
               std::make_unique<SPICE_GENERATOR_SWITCH>( *this ) )
{
    static std::vector<PARAM::INFO> vsw = makeSwVParamInfos();
    static std::vector<PARAM::INFO> isw = makeSwIParamInfos();

    switch( aType )
    {
    case TYPE::SW_V:
        for( const PARAM::INFO& paramInfo : vsw )
            AddParam( paramInfo );
        break;

    case TYPE::SW_I:
        for( const PARAM::INFO& paramInfo : isw )
            AddParam( paramInfo );
        break;

    default:
        wxFAIL_MSG( wxS( "Unhandled SIM_MODEL type in SIM_MODEL_SWITCH" ) );
        break;
    }
}


const std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SWITCH::makeSwVParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "thr";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "V";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Threshold voltage";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.spiceModelName = "vt";
    paramInfo.enumValues = {};
    paramInfos.push_back( paramInfo );

    paramInfo.name = "his";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "V";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Hysteresis voltage";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.spiceModelName = "vh";
    paramInfo.enumValues = {};
    paramInfos.push_back( paramInfo );

    paramInfo.name = "ron";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Ω";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Resistance when closed";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = {};
    paramInfos.push_back( paramInfo );

    paramInfo.name = "roff";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Ω";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1e+12";
    paramInfo.description = "Resistance when open";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = {};
    paramInfos.push_back( paramInfo );

    paramInfo.name = "ic";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "none";
    paramInfo.description = "Initial state";
    paramInfo.isSpiceInstanceParam = true;
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "none", "off", "on" };
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


const std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SWITCH::makeSwIParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "thr";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "A";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Threshold current";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.spiceModelName = "it";
    paramInfo.enumValues = {};
    paramInfos.push_back( paramInfo );

    paramInfo.name = "his";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "A";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Hysteresis current";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.spiceModelName = "ih";
    paramInfo.enumValues = {};
    paramInfos.push_back( paramInfo );

    paramInfo.name = "ron";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Ω";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Resistance when closed";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = {};
    paramInfos.push_back( paramInfo );

    paramInfo.name = "roff";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Ω";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1e+12";
    paramInfo.description = "Resistance when open";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = {};
    paramInfos.push_back( paramInfo );

    paramInfo.name = "ic";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Initial state";
    paramInfo.isSpiceInstanceParam = true;
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "none", "off", "on" };
    paramInfos.push_back( paramInfo );

    return paramInfos;
}
