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

#include <sim/sim_model_r_pot.h>
#include <fmt/core.h>


std::string SPICE_GENERATOR_R_POT::ModelLine( const SPICE_ITEM& aItem ) const
{
    std::string r = "0";
    std::string position = "";

    if( const SIM_MODEL::PARAM* r_param = m_model.FindParam( "r" ) )
        r = SIM_VALUE::ToSpice( r_param->value );

    if( const SIM_MODEL::PARAM* pos_param = m_model.FindParam( "pos" ) )
        position = SIM_VALUE::ToSpice( pos_param->value );

    if( position != "" )
    {
        return fmt::format( ".model {} potentiometer( r={} position={} )\n", aItem.modelName, r,
                            position );
    }
    else
        return fmt::format( ".model {} potentiometer( r={} )\n", aItem.modelName, r );
}


std::string SPICE_GENERATOR_R_POT::TunerCommand( const SPICE_ITEM& aItem, double aValue ) const
{
    return fmt::format( "altermod @{}[position]={:g}",
                        aItem.model->SpiceGenerator().ItemName( aItem ),
                        aValue );
}


SIM_MODEL_R_POT::SIM_MODEL_R_POT() :
    SIM_MODEL( TYPE::R_POT, std::make_unique<SPICE_GENERATOR_R_POT>( *this ) )
{
    static std::vector<PARAM::INFO> paramInfos = makeParamInfos();

    for( const SIM_MODEL::PARAM::INFO& paramInfo : paramInfos )
        AddParam( paramInfo );
}


const std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_R_POT::makeParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "r";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "Ω";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Resistance";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "pos";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0.5";
    paramInfo.description = "Wiper position";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}
