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

#include <sim/sim_model_switch.h>


SIM_MODEL_SWITCH::SIM_MODEL_SWITCH( TYPE aType ) : SIM_MODEL( aType )
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
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_SWITCH" );
        break;
    }

    SetParamValue( "ic", "none" );
}


wxString SIM_MODEL_SWITCH::GenerateSpiceItemParamValuePair( const PARAM& aParam,
                                                            bool& aIsFirst ) const
{
    // The only instance param is "ic", which is positional.
    wxString value = aParam.value->ToSpiceString();
    
    if( value == "none" )
        return "";
    else
        return value;
}


wxString SIM_MODEL_SWITCH::GenerateSpiceItemLine( const wxString& aRefName,
                                                  const wxString& aModelName,
                                                  const std::vector<wxString>& aSymbolPinNumbers,
                                                  const std::vector<wxString>& aPinNetNames ) const
{
    wxString result;
    
    switch( GetType() )
    {
    case TYPE::SW_V:
    {
        result << SIM_MODEL::GenerateSpiceItemLine( aRefName, aModelName, aSymbolPinNumbers,
                                                    aPinNetNames );
        break;
    }

    case TYPE::SW_I:
    {
        wxString vsourceName = "V__" + aRefName;

        // Current switches measure input current through a voltage source.
        result << vsourceName << " " << aPinNetNames[0] << " " << aPinNetNames[1] << " 0\n";

        result << SIM_MODEL::GenerateSpiceItemLine( aRefName, vsourceName + " " + aModelName,
                                                    aSymbolPinNumbers, aPinNetNames );
        break;
    }

    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_SWITCH" );
        break;
    }

    return result;
}


wxString SIM_MODEL_SWITCH::GenerateParamValuePair( const PARAM& aParam, bool& aIsFirst ) const
{
    if( aParam.info.name == "ic" && aParam.value->ToString() == "none" )
    {
        return "";
    }

    return SIM_MODEL::GenerateParamValuePair( aParam, aIsFirst );
}


std::vector<std::reference_wrapper<const SIM_MODEL::PIN>> SIM_MODEL_SWITCH::GetSpicePins() const
{
    switch( GetType() )
    {
    case TYPE::SW_V:
        return { GetPin( 2 ), GetPin( 3 ), GetPin( 0 ), GetPin( 1 ) };

    case TYPE::SW_I:
        return { GetPin( 2 ), GetPin( 3 ) };

    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_SWITCH" );
        return {};
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
    paramInfo.description = "Resistance when open";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = {};
    paramInfos.push_back( paramInfo );

    paramInfo.name = "roff";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Ω";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1e+12";
    paramInfo.description = "Resistance when closed";
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
    paramInfo.description = "Resistance when open";
    paramInfo.isSpiceInstanceParam = false;
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = {};
    paramInfos.push_back( paramInfo );

    paramInfo.name = "roff";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Ω";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1e+12";
    paramInfo.description = "Resistance when closed";
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
