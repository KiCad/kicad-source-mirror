/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <sim/kibis/kibis.h>
#include <sim/sim_model_ibis.h>
#include <sim/sim_library_ibis.h>
#include <fmt/core.h>
#include <wx/filename.h>
#include <kiway.h>
#include <schematic.h>
#include "sim_lib_mgr.h"

std::string SPICE_GENERATOR_IBIS::ModelName( const SPICE_ITEM& aItem ) const
{
    return fmt::format( "{}.{}", aItem.refName, aItem.baseModelName );
}


std::string SPICE_GENERATOR_IBIS::ModelLine( const SPICE_ITEM& aItem ) const
{
    return "";
}

std::vector<std::string> SPICE_GENERATOR_IBIS::CurrentNames( const SPICE_ITEM& aItem ) const
{
    std::vector<std::string> currentNames;

    for( const SIM_MODEL_PIN& pin : GetPins() )
        currentNames.push_back( fmt::format( "I({}:{})", ItemName( aItem ), pin.modelPinName ) );

    return currentNames;
}


std::string SPICE_GENERATOR_IBIS::IbisDevice( const SPICE_ITEM& aItem, SCHEMATIC* aSchematic,
                                              const wxString& aCacheDir, REPORTER& aReporter ) const
{
    std::string ibisLibFilename = GetFieldValue( &aItem.fields, SIM_LIBRARY::LIBRARY_FIELD, true, 0 );
    std::string ibisCompName    = GetFieldValue( &aItem.fields, SIM_LIBRARY::NAME_FIELD, true, 0  );
    std::string ibisPinName     = GetFieldValue( &aItem.fields, SIM_LIBRARY_IBIS::PIN_FIELD, true, 0 );
    std::string ibisModelName   = GetFieldValue( &aItem.fields, SIM_LIBRARY_IBIS::MODEL_FIELD, true, 0 );
    bool        diffMode        = GetFieldValue( &aItem.fields, SIM_LIBRARY_IBIS::DIFF_FIELD, true, 0 ) == "1";

    WX_STRING_REPORTER reporter;
    SIM_LIB_MGR        mgr( &aSchematic->Project() );

    std::vector<EMBEDDED_FILES*> embeddedFilesStack;
    embeddedFilesStack.push_back( aSchematic->GetEmbeddedFiles() );
    mgr.SetFilesStack( std::move( embeddedFilesStack ) );

    wxString path = mgr.ResolveLibraryPath( ibisLibFilename, reporter );

    if( reporter.HasMessage() )
        THROW_IO_ERROR( reporter.GetMessages() );

    KIBIS kibis( std::string( path.c_str() ) );
    kibis.m_cacheDir = std::string( aCacheDir.c_str() );
    kibis.m_Reporter = &aReporter;

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
            kpin->writeSpiceDiffDevice( result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDevice( result, aItem.modelName, *kmodel, kparams );

        break;

    case SIM_MODEL::TYPE::KIBIS_DRIVER_DC:
    {
        std::string paramValue = "";

        if( const SIM_MODEL::PARAM* dc = m_model.FindParam( "dc" ) )
            paramValue = dc->value;

        if( paramValue == "hi-Z" )
            kparams.m_waveform = new KIBIS_WAVEFORM_HIGH_Z( kibis );
        else if( paramValue == "low" )
            kparams.m_waveform = new KIBIS_WAVEFORM_STUCK_LOW( kibis );
        else if( paramValue == "high" )
            kparams.m_waveform = new KIBIS_WAVEFORM_STUCK_HIGH( kibis );

        if( diffMode )
            kpin->writeSpiceDiffDriver( result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDriver( result, aItem.modelName, *kmodel, kparams );

        break;
    }

    case SIM_MODEL::TYPE::KIBIS_DRIVER_RECT:
    {
        KIBIS_WAVEFORM_RECTANGULAR* waveform = new KIBIS_WAVEFORM_RECTANGULAR( kibis );

        if( const SIM_MODEL::PARAM* ton = m_model.FindParam( "ton" ) )
            waveform->m_ton = SIM_VALUE::ToDouble( ton->value, 0 );

        if( const SIM_MODEL::PARAM* toff = m_model.FindParam( "toff" ) )
            waveform->m_toff = SIM_VALUE::ToDouble( toff->value, 0 );

        if( const SIM_MODEL::PARAM* td = m_model.FindParam( "td" ) )
            waveform->m_delay = SIM_VALUE::ToDouble( td->value, 0 );

        if( const SIM_MODEL::PARAM* n = m_model.FindParam( "n" ) )
            waveform->m_cycles = SIM_VALUE::ToInt( n->value, 1 );

        kparams.m_waveform = waveform;

        if( diffMode )
            kpin->writeSpiceDiffDriver( result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDriver( result, aItem.modelName, *kmodel, kparams );

        break;
    }

    case SIM_MODEL::TYPE::KIBIS_DRIVER_PRBS:
    {
        KIBIS_WAVEFORM_PRBS* waveform = new KIBIS_WAVEFORM_PRBS( kibis );

        if( const SIM_MODEL::PARAM* f0 = m_model.FindParam( "f0" ) )
            waveform->m_bitrate = SIM_VALUE::ToDouble( f0->value, 0 );

        if( const SIM_MODEL::PARAM* td = m_model.FindParam( "td" ) )
            waveform->m_delay = SIM_VALUE::ToDouble( td->value, 0 );

        if( const SIM_MODEL::PARAM* n = m_model.FindParam( "n" ) )
            waveform->SetBits( SIM_VALUE::ToInt( n->value, 0 ) );

        kparams.m_waveform = waveform;

        if( diffMode )
            kpin->writeSpiceDiffDriver( result, aItem.modelName, *kmodel, kparams );
        else
            kpin->writeSpiceDriver( result, aItem.modelName, *kmodel, kparams );

        break;
    }

    default:
        wxFAIL_MSG( "Unknown IBIS model type" );
        return "";
    }

    return result;
}


SIM_MODEL_IBIS::SIM_MODEL_IBIS( TYPE aType ) :
        SIM_MODEL( aType, std::make_unique<SPICE_GENERATOR_IBIS>( *this ) ),
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
    default:                                 wxFAIL;                   return;
    }

    for( const PARAM::INFO& paramInfo : *paramInfos )
        AddParam( paramInfo );

    SwitchSingleEndedDiff( false );
}


void SIM_MODEL_IBIS::SwitchSingleEndedDiff( bool aDiff )
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

SIM_MODEL_IBIS::SIM_MODEL_IBIS( TYPE aType, const SIM_MODEL_IBIS& aSource ) :
        SIM_MODEL_IBIS( aType )
{
    for( PARAM& param1 : m_params )
    {
        for( int ii = 0; ii < aSource.GetParamCount(); ++ii )
        {
            const PARAM& param2 = aSource.GetParam( ii );

            if( param1.info.name == param2.info.name )
                param1.value = param2.value;
        }
    }

    m_componentName = aSource.m_componentName;

    m_ibisPins = aSource.GetIbisPins();
    m_ibisModels = aSource.GetIbisModels();

    m_enableDiff = aSource.CanDifferential();
}


bool SIM_MODEL_IBIS::ChangePin( const SIM_LIBRARY_IBIS& aLib, const std::string& aPinNumber )
{
    KIBIS_COMPONENT* kcomp = aLib.m_kibis.GetComponent( std::string( GetComponentName() ) );

    if( !kcomp )
        return false;

    KIBIS_PIN* kpin = kcomp->GetPin( aPinNumber );

    if( !kpin )
        return false;

    m_ibisModels.clear();

    for( KIBIS_MODEL* kmodel : kpin->m_models )
        m_ibisModels.push_back( kmodel->m_name );

    return true;
}


void SIM_MODEL_IBIS::SetBaseModel( const SIM_MODEL& aBaseModel )
{
    // Actual base models can only be of the same type, which is not the case here, as in addition
    // to IBIS device model type we have multiple types of drivers available for the same sourced
    // model. And we don't want to inherit the default values anyway. So we just store these models
    // and use the only for Spice code generation.
    m_sourceModel = dynamic_cast<const SIM_MODEL_IBIS*>( &aBaseModel );
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_IBIS::makeParamInfos( TYPE aType )
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


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_IBIS::makeDcWaveformParamInfos()
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


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_IBIS::makeRectWaveformParamInfos()
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


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_IBIS::makePrbsWaveformParamInfos()
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

