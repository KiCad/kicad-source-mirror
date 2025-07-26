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

#include <sim/sim_model_tline.h>

#include <fmt/core.h>

using SIMPARAM = SIM_MODEL::PARAM;


std::string SPICE_GENERATOR_TLINE::ModelLine( const SPICE_ITEM& aItem ) const
{
    std::string r="0", l="0", g="0", c="0", len="1";

    switch( m_model.GetType() )
    {
    case SIM_MODEL::TYPE::TLINE_Z0:
    {
        double z0 = SIM_VALUE::ToDouble( m_model.FindParam( "z0" )->value );
        double td = SIM_VALUE::ToDouble( m_model.FindParam( "td" )->value );

        if( std::isnan( z0 ) || std::isnan( td ) )
            return fmt::format( ".model {} LTRA()\n", aItem.modelName );

        l = fmt::format( "{:g}", td * z0 );
        c = fmt::format( "{:g}", td / z0 );
        break;
    }
    case SIM_MODEL::TYPE::TLINE_RLGC:
    {
        if( m_model.FindParam( "r" ) )
            r = SIM_VALUE::ToSpice( m_model.FindParam( "r" )->value );
        if( m_model.FindParam( "l" ) )
            l = SIM_VALUE::ToSpice( m_model.FindParam( "l" )->value );
        if( m_model.FindParam( "g" ) )
            g = SIM_VALUE::ToSpice( m_model.FindParam( "g" )->value );
        if( m_model.FindParam( "c" ) )
            c = SIM_VALUE::ToSpice( m_model.FindParam( "c" )->value );
        if( m_model.FindParam( "len" ) )
            len = SIM_VALUE::ToSpice( m_model.FindParam( "len" )->value );
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
    static std::vector<SIMPARAM::INFO> z0 = makeZ0ParamInfos();
    static std::vector<SIMPARAM::INFO> rlgc = makeRlgcParamInfos();

    switch( aType )
    {
    case TYPE::TLINE_Z0:
        for( const SIMPARAM::INFO& paramInfo : z0 )
            AddParam( paramInfo );
        break;

    case TYPE::TLINE_RLGC:
        for( const SIMPARAM::INFO& paramInfo : rlgc )
            AddParam( paramInfo );
        break;

    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_TLINE" );
        break;
    }
}


std::vector<SIMPARAM::INFO> SIM_MODEL_TLINE::makeZ0ParamInfos()
{
    std::vector<SIMPARAM::INFO> paramInfos;
    SIMPARAM::INFO paramInfo = {};

    paramInfo.name = "z0";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Ω";
    paramInfo.category = SIMPARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Characteristic impedance";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = true;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIMPARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Transmission delay";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = true;
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<SIMPARAM::INFO> SIM_MODEL_TLINE::makeRlgcParamInfos()
{
    std::vector<SIMPARAM::INFO> paramInfos;
    SIMPARAM::INFO paramInfo = {};

    paramInfo.name = "len";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "m";
    paramInfo.category = SIMPARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Length";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = true;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "r";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Ω/m";
    paramInfo.category = SIMPARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Resistance per length";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "l";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "H/m";
    paramInfo.category = SIMPARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Inductance per length";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "g";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "1/(Ω m)";
    paramInfo.category = SIMPARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Conductance per length";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "c";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "F/m";
    paramInfo.category = SIMPARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Capacitance per length";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );

    return paramInfos;
}
