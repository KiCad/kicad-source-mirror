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

#include <sim/kibis/kibis.h>
#include <sim/sim_model_kibis.h>
#include <sim/sim_library_kibis.h>
#include <fmt/core.h>
#include <wx/filename.h>
#include <kiway.h>
#include "sim_lib_mgr.h"

std::string SPICE_GENERATOR_KIBIS::ModelName( const SPICE_ITEM& aItem ) const
{
    return fmt::format( "{}.{}", aItem.refName, aItem.baseModelName );
}


std::string SPICE_GENERATOR_KIBIS::ModelLine( const SPICE_ITEM& aItem ) const
{
    return "";
}

std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> SPICE_GENERATOR_KIBIS::GetInstanceParams() const
{
    std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> vec;
    return vec;
}


std::vector<std::string> SPICE_GENERATOR_KIBIS::CurrentNames( const SPICE_ITEM& aItem ) const
{
    std::vector<std::string> currentNames;

    for( const SIM_MODEL::PIN& pin : GetPins() )
        currentNames.push_back( fmt::format( "I({}:{})", ItemName( aItem ), pin.name ) );

    return currentNames;
}


std::string SPICE_GENERATOR_KIBIS::IbisDevice( const SPICE_ITEM& aItem, const PROJECT& aProject,
                                               const wxString& aCacheDir ) const
{
    std::string ibisLibFilename = SIM_MODEL::GetFieldValue( &aItem.fields, SIM_LIBRARY::LIBRARY_FIELD );
    std::string ibisCompName    = SIM_MODEL::GetFieldValue( &aItem.fields, SIM_LIBRARY::NAME_FIELD  );
    std::string ibisPinName     = SIM_MODEL::GetFieldValue( &aItem.fields, SIM_LIBRARY_KIBIS::PIN_FIELD );
    std::string ibisModelName   = SIM_MODEL::GetFieldValue( &aItem.fields, SIM_LIBRARY_KIBIS::MODEL_FIELD );
    bool        diffMode        = SIM_MODEL::GetFieldValue( &aItem.fields, SIM_LIBRARY_KIBIS::DIFF_FIELD ) == "1";

    wxString path = SIM_LIB_MGR::ResolveLibraryPath( ibisLibFilename, &aProject );

    KIBIS kibis( std::string( path.c_str() ) );
    kibis.m_cacheDir = std::string( aCacheDir.c_str() );

    if( !kibis.m_valid )
        THROW_IO_ERROR( wxString::Format( _( "Invalid IBIS file '%s'" ), ibisLibFilename ) );

    KIBIS_COMPONENT* kcomp = kibis.GetComponent( std::string( ibisCompName ) );

    if( !kcomp )
        THROW_IO_ERROR( wxString::Format( _( "Could not find IBIS component '%s'" ), ibisCompName ) );

    KIBIS_PIN* kpin = kcomp->GetPin( ibisPinName );


    if( !kcomp->m_valid )
        THROW_IO_ERROR( wxString::Format( _( "Invalid IBIS component '%s'" ), ibisCompName ) );

    if( !kpin )
    {
        THROW_IO_ERROR( wxString::Format( _( "Could not find IBIS pin '%s' in component '%s'" ),
                                          ibisPinName,
                                          ibisCompName ) );
    }

    if( !kpin->m_valid )
    {
        THROW_IO_ERROR( wxString::Format( _( "Invalid IBIS pin '%s' in component '%s'" ),
                                          ibisPinName,
                                          ibisCompName ) );
    }

    KIBIS_MODEL* kmodel = kibis.GetModel( ibisModelName );

    if( !kmodel )
        THROW_IO_ERROR( wxString::Format( _( "Could not find IBIS model '%s'" ), ibisModelName ) );

    if( !kmodel->m_valid )
        THROW_IO_ERROR( wxString::Format( _( "Invalid IBIS model '%s'" ), ibisModelName ) );

    KIBIS_PARAMETER kparams;

    if( const SIM_MODEL::PARAM* vcc = m_model.FindParam( "vcc" ) )
        kparams.SetCornerFromString( kparams.m_supply, vcc->value );
    
    if( const SIM_MODEL::PARAM* rpin = m_model.FindParam( "rpin" ) )
        kparams.SetCornerFromString( kparams.m_Rpin, rpin->value );

    if( const SIM_MODEL::PARAM* lpin = m_model.FindParam( "lpin" ) )
        kparams.SetCornerFromString( kparams.m_Lpin, lpin->value );

    if( const SIM_MODEL::PARAM* cpin = m_model.FindParam( "cpin" ) )
        kparams.SetCornerFromString( kparams.m_Cpin, cpin->value );

    //kparams.SetCornerFromString( kparams.m_Ccomp, FindParam( "ccomp" )->value );

    std::string result;

    switch( m_model.GetType() )
    {
    case SIM_MODEL::TYPE::KIBIS_DEVICE:
        if( diffMode )
            kpin->writeSpiceDiffDevice( &result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDevice( &result, aItem.modelName, *kmodel, kparams );
        break;

    case SIM_MODEL::TYPE::KIBIS_DRIVER_DC:
    {
        std::string paramValue = "";

        if( const SIM_MODEL::PARAM* dc = m_model.FindParam( "dc" ) )
            paramValue = dc->value;

        if( paramValue == "hi-Z" )
        {
            kparams.m_waveform = static_cast<KIBIS_WAVEFORM*>( new KIBIS_WAVEFORM_HIGH_Z( &kibis ) );
        }
        else if( paramValue == "low" )
        {
            kparams.m_waveform = static_cast<KIBIS_WAVEFORM*>( new KIBIS_WAVEFORM_STUCK_LOW( &kibis ) );
        }
        else if( paramValue == "high" )
        {
            kparams.m_waveform = static_cast<KIBIS_WAVEFORM*>( new KIBIS_WAVEFORM_STUCK_HIGH( &kibis ) );
        }

        if( diffMode )
            kpin->writeSpiceDiffDriver( &result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDriver( &result, aItem.modelName, *kmodel, kparams );
        break;
    }

    case SIM_MODEL::TYPE::KIBIS_DRIVER_RECT:
    {
        KIBIS_WAVEFORM_RECTANGULAR* waveform = new KIBIS_WAVEFORM_RECTANGULAR( &kibis );

        if( const SIM_MODEL::PARAM* ton = m_model.FindParam( "ton" ) )
            waveform->m_ton = SIM_VALUE::ToDouble( ton->value, 1 );

        if( const SIM_MODEL::PARAM* toff = m_model.FindParam( "toff" ) )
            waveform->m_toff = SIM_VALUE::ToDouble( toff->value, 1 );

        if( const SIM_MODEL::PARAM* td = m_model.FindParam( "td" ) )
            waveform->m_delay = SIM_VALUE::ToDouble( td->value, 0 );

        if( const SIM_MODEL::PARAM* n = m_model.FindParam( "n" ) )
            waveform->m_cycles = SIM_VALUE::ToInt( n->value, 1 );

        kparams.m_waveform = waveform;

        if( diffMode )
            kpin->writeSpiceDiffDriver( &result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDriver( &result, aItem.modelName, *kmodel, kparams );
        break;
    }

    case SIM_MODEL::TYPE::KIBIS_DRIVER_PRBS:
    {
        KIBIS_WAVEFORM_PRBS* waveform = new KIBIS_WAVEFORM_PRBS( &kibis );

        if( const SIM_MODEL::PARAM* f0 = m_model.FindParam( "f0" ) )
            waveform->m_bitrate = SIM_VALUE::ToDouble( f0->value, 0 );

        if( const SIM_MODEL::PARAM* td = m_model.FindParam( "td" ) )
            waveform->m_delay = SIM_VALUE::ToDouble( td->value, 0 );

        if( const SIM_MODEL::PARAM* n = m_model.FindParam( "n" ) )
            waveform->m_bits = SIM_VALUE::ToInt( n->value, 0 );

        kparams.m_waveform = waveform;

        if( diffMode )
            kpin->writeSpiceDiffDriver( &result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDriver( &result, aItem.modelName, *kmodel, kparams );
        break;
    }

    default:
        wxFAIL_MSG( "Unknown IBIS model type" );
        return "";
    }

    return result;
}


SIM_MODEL_KIBIS::SIM_MODEL_KIBIS( TYPE aType ) :
        SIM_MODEL( aType, std::make_unique<SPICE_GENERATOR_KIBIS>( *this ) ),
        m_enableDiff( false ),
        m_sourceModel( nullptr )
{
    static std::vector<PARAM::INFO> device = makeParamInfos( TYPE::KIBIS_DEVICE );
    static std::vector<PARAM::INFO> dcDriver = makeParamInfos( TYPE::KIBIS_DRIVER_DC );
    static std::vector<PARAM::INFO> rectDriver = makeParamInfos( TYPE::KIBIS_DRIVER_RECT );
    static std::vector<PARAM::INFO> prbsDriver = makeParamInfos( TYPE::KIBIS_DRIVER_PRBS );

    std::vector<PARAM::INFO>* paramInfos = nullptr;

    switch( aType )
    {
    case SIM_MODEL::TYPE::KIBIS_DEVICE:      paramInfos = &device;     break;
    case SIM_MODEL::TYPE::KIBIS_DRIVER_DC:   paramInfos = &dcDriver;   break;
    case SIM_MODEL::TYPE::KIBIS_DRIVER_RECT: paramInfos = &rectDriver; break;
    case SIM_MODEL::TYPE::KIBIS_DRIVER_PRBS: paramInfos = &prbsDriver; break;

    default:
        wxFAIL;
        return;
    }

    for( const PARAM::INFO& paramInfo : *paramInfos )
        AddParam( paramInfo );

    SwitchSingleEndedDiff( false );
}


void SIM_MODEL_KIBIS::SwitchSingleEndedDiff( bool aDiff )
{
    ClearPins();

    if( aDiff )
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

SIM_MODEL_KIBIS::SIM_MODEL_KIBIS( TYPE aType, const SIM_MODEL_KIBIS& aSource ) :
        SIM_MODEL_KIBIS( aType )
{
    for( PARAM& param1 : m_params )
    {
        for( auto& param2refwrap : aSource.GetParams() )
        {
            const PARAM& param2 = param2refwrap.get();

            if( param1.info.name == param2.info.name )
                param1.value = param2.value;
        }
    }

    m_componentName = aSource.m_componentName;

    m_ibisPins = aSource.GetIbisPins();
    m_ibisModels = aSource.GetIbisModels();
    
    m_enableDiff = aSource.CanDifferential();
}


bool SIM_MODEL_KIBIS::ChangePin( const SIM_LIBRARY_KIBIS& aLib, std::string aPinNumber )
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


void SIM_MODEL_KIBIS::SetBaseModel( const SIM_MODEL& aBaseModel )
{
    // Actual base models can only be of the same type, which is not the case here, as in addition
    // to IBIS device model type we have multiple types of drivers available for the same sourced
    // model. And we don't want to inherit the default values anyway. So we just store these models
    // and use the only for Spice code generation.
    m_sourceModel = dynamic_cast<const SIM_MODEL_KIBIS*>( &aBaseModel );
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_KIBIS::makeParamInfos( TYPE aType )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO              paramInfo;

    paramInfo.name = "vcc";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "typ";
    paramInfo.description = _( "Power supply" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "typ", "min", "max" };
    paramInfos.push_back( paramInfo );

    paramInfo.name = "rpin";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "typ";
    paramInfo.description = _( "Parasitic pin resistance" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "typ", "min", "max" };
    paramInfos.push_back( paramInfo );

    paramInfo.name = "lpin";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "typ";
    paramInfo.description = _( "Parasitic pin inductance" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "typ", "min", "max" };
    paramInfos.push_back( paramInfo );

    paramInfo.name = "cpin";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "typ";
    paramInfo.description = _( "Parasitic pin capacitance" );
    paramInfo.spiceModelName = "";
    paramInfo.enumValues = { "typ", "min", "max" };
    paramInfos.push_back( paramInfo );

    std::vector<PARAM::INFO> dc = makeDcWaveformParamInfos();
    std::vector<PARAM::INFO> rect = makeRectWaveformParamInfos();
    std::vector<PARAM::INFO> prbs = makePrbsWaveformParamInfos();

    switch( aType )
    {
    case TYPE::KIBIS_DRIVER_DC:
        for( const PARAM::INFO& param : makeDcWaveformParamInfos() )
            paramInfos.push_back( param );
        break;

    case TYPE::KIBIS_DRIVER_RECT:
        for( const PARAM::INFO& param : makeRectWaveformParamInfos() )
            paramInfos.push_back( param );
        break;

    case TYPE::KIBIS_DRIVER_PRBS:
        for( const PARAM::INFO& param : makePrbsWaveformParamInfos() )
            paramInfos.push_back( param );
        break;

    default:
        break;
    }

    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_KIBIS::makeDcWaveformParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO              paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "hi-Z";
    paramInfo.description = _( "DC Value" );
    paramInfo.enumValues = { "hi-Z", "low", "high" };
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_KIBIS::makeRectWaveformParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO              paramInfo;

    paramInfo.name = "ton";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "";
    paramInfo.description = _( "ON time" );
    paramInfos.push_back( paramInfo );

    paramInfo.name = "toff";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "";
    paramInfo.description = _( "OFF time" );
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "0";
    paramInfo.description = _( "Delay" );
    paramInfos.push_back( paramInfo );

    paramInfo.name = "n";
    paramInfo.type = SIM_VALUE::TYPE_INT;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "1";
    paramInfo.description = _( "Number of cycles" );
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_KIBIS::makePrbsWaveformParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO              paramInfo;

    paramInfo.name = "f0";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "";
    paramInfo.description = _( "Bitrate" );
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "";
    paramInfo.description = _( "Delay" );
    paramInfos.push_back( paramInfo );

    paramInfo.name = "n";
    paramInfo.type = SIM_VALUE::TYPE_INT;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::WAVEFORM;
    paramInfo.defaultValue = "";
    paramInfo.description = _( "Number of bits" );
    paramInfos.push_back( paramInfo );

    return paramInfos;
}

