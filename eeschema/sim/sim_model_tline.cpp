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

#include <sim/sim_model_tline.h>

#include <fmt/core.h>

using PARAM = SIM_MODEL::PARAM;


std::string SPICE_GENERATOR_TLINE::ModelLine( const SPICE_ITEM& aItem ) const
{
    std::string r="0" , l="0", g="0", c="0", len="1";

    switch( m_model.GetType() )
    {
    case SIM_MODEL::TYPE::TLINE_Z0:
    {
        auto z0 = static_cast<const SIM_VALUE_FLOAT&>( *m_model.FindParam( "z0" )->value );
        auto td = static_cast<const SIM_VALUE_FLOAT&>( *m_model.FindParam( "td" )->value );

        if( !z0.ToString().empty() || !td.ToString().empty() )
            return fmt::format( ".model {} LTRA()\n", aItem.modelName );

        r = SIM_VALUE_FLOAT( 0 ).ToSpiceString();
        l = ( td * z0 ).ToSpiceString();
        g = SIM_VALUE_FLOAT( 0 ).ToSpiceString();
        c = ( td / z0 ).ToSpiceString();
        len = SIM_VALUE_FLOAT( 1 ).ToSpiceString();
        break;
    }
    case SIM_MODEL::TYPE::TLINE_RLGC:
    {
        if( m_model.FindParam( "r" ) )
            r = m_model.FindParam( "r" )->value->ToSpiceString();
        if( m_model.FindParam( "l" ) )
            l = m_model.FindParam( "l" )->value->ToSpiceString();
        if( m_model.FindParam( "g" ) )
            g = m_model.FindParam( "g" )->value->ToSpiceString();
        if( m_model.FindParam( "c" ) )
            c = m_model.FindParam( "c" )->value->ToSpiceString();
        if( m_model.FindParam( "len" ) )
            len = m_model.FindParam( "len" )->value->ToSpiceString();
        break;
    }
    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_TLINE" );
        return "";
    }

    return fmt::format( ".model {} LTRA( r={} l={} g={} c={} len={} )\n",
                        aItem.modelName, r, l, g, c, len );
}


SIM_MODEL_TLINE::SIM_MODEL_TLINE( TYPE aType ) :
        SIM_MODEL( aType, std::make_unique<SPICE_GENERATOR_TLINE>( *this ) )
{
    static std::vector<PARAM::INFO> z0 = makeZ0ParamInfos();
    static std::vector<PARAM::INFO> rlgc = makeRlgcParamInfos();

    switch( aType )
    {
    case TYPE::TLINE_Z0:
        for( const PARAM::INFO& paramInfo : z0 )
            AddParam( paramInfo );
        break;

    case TYPE::TLINE_RLGC:
        for( const PARAM::INFO& paramInfo : rlgc )
            AddParam( paramInfo );
        break;

    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_TLINE" );
        break;
    }
}


std::vector<PARAM::INFO> SIM_MODEL_TLINE::makeZ0ParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo = {};

    paramInfo.name = "z0";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Ω";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Characteristic impedance";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = true;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Transmission delay";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = true;
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_TLINE::makeRlgcParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo = {};

    paramInfo.name = "len";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "m";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Length";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = true;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "r";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Ω/m";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Resistance per length";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "l";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "H/m";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Inductance per length";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "g";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "1/(Ω m)";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Conductance per length";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "c";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "C/m";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Capacitance per length";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );

    return paramInfos;
}
