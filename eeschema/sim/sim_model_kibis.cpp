/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <../../pcbnew/ibis/kibis.h>
#include <sim/sim_model_kibis.h>
#include <sim/sim_library_kibis.h>
#include <locale_io.h>


#include <fmt/core.h>


std::string SPICE_GENERATOR_KIBIS::ModelLine( const std::string& aModelName ) const
{
    return "";
}

std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> SPICE_GENERATOR_KIBIS::GetInstanceParams() const
{
    std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> vec;
    return vec;
}


std::vector<std::string> SPICE_GENERATOR_KIBIS::CurrentNames( const std::string& aRefName ) const
{
    std::vector<std::string> currentNames;

    for( const SIM_MODEL::PIN& pin : GetPins() )
    {
        currentNames.push_back( fmt::format( "I({:s}:{:s})", ItemName( aRefName ), pin.name ) );
    }

    return currentNames;
}

SIM_MODEL_KIBIS::SIM_MODEL_KIBIS( TYPE aType, std::string aWfType ) :
        SIM_MODEL( aType, std::make_unique<SPICE_GENERATOR_KIBIS>( *this ) )
{
    SetParameters( aType, aWfType );

    if( aType == SIM_MODEL::TYPE::KIBIS_DIFFDEVICE || aType == SIM_MODEL::TYPE::KIBIS_DIFFDRIVER )
    {
        AddPin( { "GND", "1" } );
        AddPin( { "+", "2" } );
        AddPin( { "-", "3" } );
    }
    else
    {
        AddPin( { "GND", "1" } );
        AddPin( { "IN/OUT", "2" } );
    }
}


SIM_MODEL_KIBIS::SIM_MODEL_KIBIS( TYPE aType, const SIM_MODEL_KIBIS& aSource ) : SIM_MODEL_KIBIS( aType )
{
    for( PARAM& param1 : m_params )
    {
        for( auto& param2refwrap : aSource.GetParams() )
        {
            const PARAM& param2 = param2refwrap.get();

            if( param1.info.name == param2.info.name )
                *( param1.value ) = *( param2.value );
        }
    }

    m_componentName = aSource.m_componentName;

    m_ibisPins = aSource.GetIbisPins();
    m_ibisModels = aSource.GetIbisModels();
}

SIM_MODEL_KIBIS::SIM_MODEL_KIBIS( TYPE aType, SIM_MODEL_KIBIS& aSource,
                                  const std::vector<LIB_FIELD>& aFields ) :
        SIM_MODEL_KIBIS( aType, aSource )
{
    ReadDataFields( 2, &aFields );
}

SIM_MODEL_KIBIS::SIM_MODEL_KIBIS( TYPE aType, SIM_MODEL_KIBIS& aSource,
                                  const std::vector<SCH_FIELD>& aFields ) :
        SIM_MODEL_KIBIS( aType, aSource )
{
    ReadDataFields( 2, &aFields );
}

void SIM_MODEL_KIBIS::SetParameters( TYPE aType, std::string aWfType )
{
    static std::vector<PARAM::INFO> kibisparam_device =
            makeKibisParamInfos( SIM_MODEL::TYPE::KIBIS_DEVICE, "Dummy" );
    static std::vector<PARAM::INFO> kibisparam_driver_rect =
            makeKibisParamInfos( SIM_MODEL::TYPE::KIBIS_DRIVER, SIM_MODEL_KIBIS::DRIVER_RECT );
    static std::vector<PARAM::INFO> kibisparam_driver_stuckh =
            makeKibisParamInfos( SIM_MODEL::TYPE::KIBIS_DRIVER, SIM_MODEL_KIBIS::DRIVER_STUCKH );
    static std::vector<PARAM::INFO> kibisparam_driver_stuckl =
            makeKibisParamInfos( SIM_MODEL::TYPE::KIBIS_DRIVER, SIM_MODEL_KIBIS::DRIVER_STUCKL );
    static std::vector<PARAM::INFO> kibisparam_driver_highz =
            makeKibisParamInfos( SIM_MODEL::TYPE::KIBIS_DRIVER, SIM_MODEL_KIBIS::DRIVER_HIGHZ );
    static std::vector<PARAM::INFO> kibisparam_driver_prbs =
            makeKibisParamInfos( SIM_MODEL::TYPE::KIBIS_DRIVER, SIM_MODEL_KIBIS::DRIVER_PRBS );
    static std::vector<PARAM::INFO> kibisparam_driver_allParams =
            makeKibisParamInfos( SIM_MODEL::TYPE::KIBIS_DRIVER, SIM_MODEL_KIBIS::JOCKER );

    m_requiresUIUpdate = true;

    static std::vector<PARAM::INFO>* params;

    switch( aType )
    {
    case SIM_MODEL::TYPE::KIBIS_DEVICE:
    case SIM_MODEL::TYPE::KIBIS_DIFFDEVICE: params = &kibisparam_device; break;
    case SIM_MODEL::TYPE::KIBIS_DIFFDRIVER:
    case SIM_MODEL::TYPE::KIBIS_DRIVER:

        if( aWfType == SIM_MODEL_KIBIS::DRIVER_RECT )
            params = &kibisparam_driver_rect;
        else if( aWfType == SIM_MODEL_KIBIS::DRIVER_STUCKH )
            params = &kibisparam_driver_stuckh;
        else if( aWfType == SIM_MODEL_KIBIS::DRIVER_STUCKL )
            params = &kibisparam_driver_stuckl;
        else if( aWfType == SIM_MODEL_KIBIS::DRIVER_HIGHZ )
            params = &kibisparam_driver_highz;
        else if( aWfType == SIM_MODEL_KIBIS::DRIVER_PRBS )
            params = &kibisparam_driver_prbs;
        else
            params = &kibisparam_driver_allParams;
            // All params allow for a newly created model to read any parameter
        break;

    default: wxFAIL; return;
    }

    m_params.clear();

    /*
    if( m_params.empty() )
    {
        for( const PARAM::INFO& paramInfo : *params )
            AddParam( paramInfo );
    }

    for( PARAM& param : m_params )
    {
        // We don't erase other params because we don't want to reset their values
        if( param.info.category == PARAM::CATEGORY::WAVEFORM )
        {
            m_params.pop_back(); // waveform parameters are at the end of the vector
        }
    }
    */
    
    for( const PARAM::INFO& paramInfo : *params )
        AddParam( paramInfo );
}

void SIM_MODEL_KIBIS::SetParameters( std::string aWfType )
{
    SetParameters( GetType(), aWfType );
}

void SIM_MODEL_KIBIS::CreatePins( unsigned aSymbolPinCount )
{
    SIM_MODEL::CreatePins( aSymbolPinCount );

    // Reset the pins to Not Connected. Linear order is not as common, and reordering the pins is
    // more effort in the GUI than assigning them from scratch.
    for( int pinIndex = 0; pinIndex < GetPinCount(); ++pinIndex )
        SetPinSymbolPinNumber( pinIndex, "" );
}


const std::vector<SIM_MODEL::PARAM::INFO>
SIM_MODEL_KIBIS::makeKibisParamInfos( TYPE aType, std::string aWfType )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO              paramInfo;

    paramInfo.name = "vcc";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "TYP";
    paramInfo.description = _( "Power supply" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "TYP", "MIN", "MAX" };
    paramInfos.push_back( paramInfo );

    paramInfo.name = "rpin";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "TYP";
    paramInfo.description = _( "Parasitic Resistance" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "TYP", "MIN", "MAX" };
    paramInfos.push_back( paramInfo );

    paramInfo.name = "lpin";
        paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "TYP";
    paramInfo.description = _( "Parasitic Pin Inductance" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "TYP", "MIN", "MAX" };
    paramInfos.push_back( paramInfo );

    paramInfo.name = "cpin";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "TYP";
    paramInfo.description = _( "Parasitic Pin Capacitance" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "TYP", "MIN", "MAX" };
    paramInfos.push_back( paramInfo );

    if( aType == SIM_MODEL::TYPE::KIBIS_DEVICE || aType == SIM_MODEL::TYPE::KIBIS_DIFFDEVICE )
        return paramInfos; // Devices have no waveform parameters



    paramInfo.name = "wftype";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = DRIVER_HIGHZ;
    paramInfo.description = _( "Waveform" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { DRIVER_RECT, DRIVER_STUCKH, DRIVER_STUCKL, DRIVER_HIGHZ, DRIVER_PRBS };
    paramInfos.push_back( paramInfo );

    if( aWfType == DRIVER_STUCKH || aWfType == DRIVER_STUCKL || aWfType == DRIVER_HIGHZ )
    {
        return paramInfos;
    }

    paramInfo.name = "ac";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "normal";
    paramInfo.description = _( "Modeling accuracy" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "low", "normal", "high" };
    paramInfos.push_back( paramInfo );

    if ( aWfType == DRIVER_RECT || aWfType == JOCKER  )

    {
        paramInfo.name = "ton";
        paramInfo.type = SIM_VALUE::TYPE_FLOAT;
        paramInfo.unit = "s";
        paramInfo.category = PARAM::CATEGORY::WAVEFORM;
        paramInfo.defaultValue = "";
        paramInfo.description = _( "ON time" );
        paramInfo.spiceModelName = "";
        paramInfos.push_back( paramInfo );

        paramInfo.name = "toff";
        paramInfo.type = SIM_VALUE::TYPE_FLOAT;
        paramInfo.unit = "s";
        paramInfo.category = PARAM::CATEGORY::WAVEFORM;
        paramInfo.defaultValue = "";
        paramInfo.description = _( "OFF time" );
        paramInfo.spiceModelName = "";
        paramInfos.push_back( paramInfo );

        paramInfo.name = "delay";
        paramInfo.type = SIM_VALUE::TYPE_FLOAT;
        paramInfo.unit = "s";
        paramInfo.category = PARAM::CATEGORY::WAVEFORM;
        paramInfo.defaultValue = "0";
        paramInfo.description = _( "Delay" );
        paramInfo.spiceModelName = "";
        paramInfos.push_back( paramInfo );
    }

    if ( ( aWfType == DRIVER_PRBS ) || ( aWfType == JOCKER )  )

    {
        paramInfo.name = "f0";
        paramInfo.type = SIM_VALUE::TYPE_FLOAT;
        paramInfo.unit = "Hz";
        paramInfo.category = PARAM::CATEGORY::WAVEFORM;
        paramInfo.defaultValue = "";
        paramInfo.description = _( "Bitrate" );
        paramInfo.spiceModelName = "";
        paramInfos.push_back( paramInfo );

        paramInfo.name = "bits";
        paramInfo.type = SIM_VALUE::TYPE_FLOAT;
        paramInfo.unit = "Hz";
        paramInfo.category = PARAM::CATEGORY::WAVEFORM;
        paramInfo.defaultValue = "";
        paramInfo.description = _( "Number of bits" );
        paramInfo.spiceModelName = "";
        paramInfos.push_back( paramInfo );
    }

    return paramInfos;
}

bool SIM_MODEL_KIBIS::ChangePin( SIM_LIBRARY_KIBIS& aLib, std::string aPinNumber )
{
    KIBIS_COMPONENT* kcomp = aLib.m_kibis.GetComponent( std::string( GetComponentName() ) );

    if( !kcomp )
        return false;

    KIBIS_PIN* kpin = kcomp->GetPin( std::string( aPinNumber.c_str() ) );

    if( !kpin )
        return false;

    m_ibisModels.clear();

    for( KIBIS_MODEL* kmodel : kpin->m_models )
        m_ibisModels.push_back( kmodel->m_name );

    return true;
}