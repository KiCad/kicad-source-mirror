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

#include <sim/sim_model_passive.h>

using PARAM = SIM_MODEL::PARAM;


SIM_MODEL_PASSIVE::SIM_MODEL_PASSIVE( TYPE aType )
    : SIM_MODEL( aType )
{
    static std::vector<PARAM::INFO> resistor = makeParamInfos( "r", "Resistance", "Ω" );
    static std::vector<PARAM::INFO> capacitor = makeParamInfos( "c", "Capacitance", "F" );
    static std::vector<PARAM::INFO> inductor = makeParamInfos( "l", "Inductance", "H" );

    switch( aType )
    {
    case TYPE::R_ADV:
        for( const PARAM::INFO& paramInfo : resistor )
            AddParam( paramInfo );
        break;

    case TYPE::C_ADV:
        for( const PARAM::INFO& paramInfo : capacitor )
            AddParam( paramInfo );
        break;

    case TYPE::L_ADV:
        for( const PARAM::INFO& paramInfo : inductor )
            AddParam( paramInfo );
        break;

    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_PASSIVE" );
    }
}


bool SIM_MODEL_PASSIVE::SetParamFromSpiceCode( const wxString& aParamName,
                                               const wxString& aParamValue,
                                               SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    if( aParamName.Lower() == "tc" )
        return SetParamFromSpiceCode( "tc1", aParamValue, aNotation );

    switch( GetType() )
    {
    case TYPE::R_ADV:
        if( aParamName.Lower() == "tc1r" )
            return SIM_MODEL::SetParamFromSpiceCode( "tc1", aParamValue, aNotation );

        /*if( aParamName.Lower() == "tc2r" )
            return SIM_MODEL::SetParamFromSpiceCode( "tc2", aParamValue, aNotation );*/

        if( aParamName.Lower() == "res" )
            return SIM_MODEL::SetParamFromSpiceCode( "r", aParamValue, aNotation );

        break;

    case TYPE::C_ADV:
        if( aParamName.Lower() == "cap" )
            return SIM_MODEL::SetParamFromSpiceCode( "c", aParamValue, aNotation );

        break;

    case TYPE::L_ADV:
        if( aParamName.Lower() == "ind" )
            return SIM_MODEL::SetParamFromSpiceCode( "l", aParamValue, aNotation );

        break;

    default:
        break;
    }

    return SIM_MODEL::SetParamFromSpiceCode( aParamName, aParamValue, aNotation );
}


std::vector<PARAM::INFO> SIM_MODEL_PASSIVE::makeParamInfos( wxString aName,
                                                            wxString aDescription,
                                                            wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo = {};

    paramInfo.name = "temp";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "°C";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "27";
    paramInfo.description = "Temperature";
    paramInfo.isInstanceParam = true;
    paramInfos.push_back( paramInfo );

    paramInfo.name = aName;
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = aDescription;
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tnom";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "°C";
    paramInfo.category = PARAM::CATEGORY::TEMPERATURE;
    paramInfo.defaultValue = "27";
    paramInfo.description = "Nominal temperature";
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tc1";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::TEMPERATURE;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Temperature coefficient";
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );

    /*paramInfo.name = "tc2";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::TEMPERATURE;
    paramInfo.defaultValue = "0";
    paramInfo.description = "2nd order temperature coefficient";
    paramInfo.isInstanceParam = false;
    paramInfos.push_back( paramInfo );*/

    /*if( aName != "l" )
    {
        paramInfo.name = "bv_max";
        paramInfo.type = SIM_VALUE::TYPE::FLOAT;
        paramInfo.unit = aUnit;
        paramInfo.category = PARAM::CATEGORY::LIMITING_VALUES;
        paramInfo.defaultValue = "";
        paramInfo.description = "Max. safe operating voltage";
        paramInfos.push_back( paramInfo );
    }*/

    if( aName == "r" )
    {
        paramInfo.name = "noisy";
        paramInfo.type = SIM_VALUE::TYPE::BOOL;
        paramInfo.unit = "";
        paramInfo.category = PARAM::CATEGORY::NOISE;
        paramInfo.defaultValue = "True";
        paramInfo.description = "Enable thermal noise";
        paramInfo.isInstanceParam = false;
        paramInfos.push_back( paramInfo );
    }

    return paramInfos;
}
