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

#include <sim/sim_model_source.h>

using PARAM = SIM_MODEL::PARAM;


SIM_MODEL_SOURCE::SIM_MODEL_SOURCE( TYPE aType )
    : SIM_MODEL( aType ),
      m_isInferred( false )
{
    for( const PARAM::INFO& paramInfo : makeParamInfos( aType ) )
        AddParam( paramInfo );
}


void SIM_MODEL_SOURCE::ReadDataSchFields( unsigned aSymbolPinCount,
                                          const std::vector<SCH_FIELD>* aFields )
{
    if( !GetFieldValue( aFields, PARAMS_FIELD ).IsEmpty() )
        SIM_MODEL::ReadDataSchFields( aSymbolPinCount, aFields );
    else
        inferredReadDataFields( aSymbolPinCount, aFields );
}


void SIM_MODEL_SOURCE::ReadDataLibFields( unsigned aSymbolPinCount,
                                          const std::vector<LIB_FIELD>* aFields )
{
    if( !GetFieldValue( aFields, PARAMS_FIELD ).IsEmpty() )
        SIM_MODEL::ReadDataLibFields( aSymbolPinCount, aFields );
    else
        inferredReadDataFields( aSymbolPinCount, aFields );
}


void SIM_MODEL_SOURCE::WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataSchFields( aFields );

    if( m_isInferred )
        inferredWriteDataFields( aFields );
}


void SIM_MODEL_SOURCE::WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataLibFields( aFields );

    if( m_isInferred )
        inferredWriteDataFields( aFields );
}


wxString SIM_MODEL_SOURCE::GenerateSpiceModelLine( const wxString& aModelName ) const
{
    return "";
}


wxString SIM_MODEL_SOURCE::GenerateSpiceItemLine( const wxString& aRefName,
                                                  const wxString& aModelName,
                                                  const std::vector<wxString>& aPinNetNames ) const
{
    wxString argList = "";

    for( unsigned i = 0; i < GetParamCount(); ++i )
        argList << GetParam( i ).value->ToString() << " ";

    wxString model = wxString::Format( GetSpiceInfo().inlineTypeString + "( %s)", argList );

    return SIM_MODEL::GenerateSpiceItemLine( aRefName, model, aPinNetNames );
}


const std::vector<PARAM::INFO>& SIM_MODEL_SOURCE::makeParamInfos( TYPE aType )
{
    static std::vector<PARAM::INFO> vdc = makeDcParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> idc = makeDcParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vsin = makeSinParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> isin = makeSinParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vpulse = makePulseParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> ipulse = makePulseParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vexp = makeExpParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> iexp = makeExpParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vsfam = makeSfamParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> isfam = makeSfamParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vsffm = makeSffmParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> isffm = makeSffmParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vpwl = makePwlParamInfos( "v", "Voltage", "V" );
    static std::vector<PARAM::INFO> ipwl = makePwlParamInfos( "i", "Current", "A" );

    static std::vector<PARAM::INFO> vwhitenoise = makeWhiteNoiseParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> iwhitenoise = makeWhiteNoiseParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vpinknoise = makePinkNoiseParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> ipinknoise = makePinkNoiseParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vburstnoise = makeBurstNoiseParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> iburstnoise = makeBurstNoiseParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vrandomuniform = makeRandomUniformParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> irandomuniform = makeRandomUniformParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vrandomnormal = makeRandomNormalParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> irandomnormal = makeRandomNormalParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vrandomexp = makeRandomExpParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> irandomexp = makeRandomExpParamInfos( "i", "A" );

    static std::vector<PARAM::INFO> vrandompoisson = makeRandomPoissonParamInfos( "v", "V" );
    static std::vector<PARAM::INFO> irandompoisson = makeRandomPoissonParamInfos( "i", "A" );

    switch( aType )
    {
    case TYPE::V_DC:          return vdc;
    case TYPE::I_DC:          return idc;
    case TYPE::V_SIN:         return vsin;
    case TYPE::I_SIN:         return isin;
    case TYPE::V_PULSE:       return vpulse;
    case TYPE::I_PULSE:       return ipulse;
    case TYPE::V_EXP:         return vexp;
    case TYPE::I_EXP:         return iexp;
    case TYPE::V_SFAM:        return vsfam;
    case TYPE::I_SFAM:        return isfam;
    case TYPE::V_SFFM:        return vsffm;
    case TYPE::I_SFFM:        return isffm;
    case TYPE::V_PWL:         return vpwl;
    case TYPE::I_PWL:         return ipwl;
    case TYPE::V_WHITENOISE:  return vwhitenoise;
    case TYPE::I_WHITENOISE:  return iwhitenoise;
    case TYPE::V_PINKNOISE:   return vpinknoise;
    case TYPE::I_PINKNOISE:   return ipinknoise;
    case TYPE::V_BURSTNOISE:  return vburstnoise;
    case TYPE::I_BURSTNOISE:  return iburstnoise;
    case TYPE::V_RANDUNIFORM: return vrandomuniform;
    case TYPE::I_RANDUNIFORM: return irandomuniform;
    case TYPE::V_RANDNORMAL:  return vrandomnormal;
    case TYPE::I_RANDNORMAL:  return irandomnormal;
    case TYPE::V_RANDEXP:     return vrandomexp;
    case TYPE::I_RANDEXP:     return irandomexp;
    case TYPE::V_RANDPOISSON: return vrandompoisson;
    case TYPE::I_RANDPOISSON: return irandompoisson;
    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_SOURCE" );
        static std::vector<PARAM::INFO> empty;
        return empty;
    }
}


bool SIM_MODEL_SOURCE::SetParamValue( unsigned aParamIndex, const wxString& aValue,
                                      SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    // Sources are special. All preceding parameter values must be filled. If they are not, fill
    // them out automatically. If a value is nulled, delete everything after it.
    if( aValue.IsEmpty() )
    {
        for( unsigned i = aParamIndex; i < GetParamCount(); ++i )
            SIM_MODEL::SetParamValue( i, "", aNotation );
    }
    else
    {
        for( unsigned i = 0; i < aParamIndex; ++i )
        {
            if( GetParam( i ).value->ToString().IsEmpty() )
                SIM_MODEL::SetParamValue( i, "0", aNotation );
        }
    }

    return SIM_MODEL::SetParamValue( aParamIndex, aValue, aNotation );
}


wxString SIM_MODEL_SOURCE::GenerateParamValuePair( const PARAM& aParam, bool& aIsFirst ) const
{
    if( aParam.value->ToString() == "0" )
        return "";

    return SIM_MODEL::GenerateParamValuePair( aParam, aIsFirst );
}


template <typename T>
void SIM_MODEL_SOURCE::inferredReadDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields )
{
    ParsePinsField( aSymbolPinCount, PINS_FIELD );

    if( ( InferTypeFromRef( GetFieldValue( aFields, REFERENCE_FIELD ) ) == GetType()
            && ParseParamsField( GetFieldValue( aFields, VALUE_FIELD ) ) )
        || GetFieldValue( aFields, VALUE_FIELD ) == DeviceTypeInfo( GetDeviceType() ).fieldValue )
    {
        m_isInferred = true;
    }
}


template <typename T>
void SIM_MODEL_SOURCE::inferredWriteDataFields( std::vector<T>& aFields ) const
{
    wxString value = GetFieldValue( &aFields, PARAMS_FIELD );

    if( value.IsEmpty() )
        value = DeviceTypeInfo( GetDeviceType() ).fieldValue;

    WriteInferredDataFields( aFields, value );
}


std::vector<wxString> SIM_MODEL_SOURCE::getPinNames() const
{
    return { "+", "-" };
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeDcParamInfos( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "DC value";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeSinParamInfos( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "ampl";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "f";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1/tstop";
    paramInfo.description = "Frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "theta";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "1/s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Damping factor";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phase";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "deg";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Phase";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makePulseParamInfos( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "1";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Initial value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix + "2";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Initial value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tr";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Rise time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tf";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Fall time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "pw";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstop";
    paramInfo.description = "Pulse width";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "per";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstop";
    paramInfo.description = "Period";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phase";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "deg";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Phase";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeExpParamInfos( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "1";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Initial value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix + "2";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Pulsed value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td1";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Rise delay time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tau1";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Rise time constant";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td2";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "td1+tstep";
    paramInfo.description = "Fall delay time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tau2";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Fall time constant";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeSfamParamInfos( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";

    paramInfo.name = "ampl";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Amplitude";

    paramInfos.push_back( paramInfo );
    paramInfo.name = "mo";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulating signal offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fc";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Carrier frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mf";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulating frequency";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeSffmParamInfos( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "ampl";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fc";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1/tstop";
    paramInfo.description = "Carrier frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mdi";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulation index";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fs";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1/tstop";
    paramInfo.description = "Signal frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phasec";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "deg";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Carrier phase";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phases";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "deg";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Signal phase";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makePwlParamInfos( wxString aPrefix, wxString aQuantity,
                                                              wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "t";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT_VECTOR;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Time vector";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix;
    paramInfo.type = SIM_VALUE::TYPE::FLOAT_VECTOR;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = aQuantity + " vector";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "repeat";
    paramInfo.type = SIM_VALUE::TYPE::BOOL;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Repeat forever";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeWhiteNoiseParamInfos( wxString aPrefix,
                                                                     wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "na";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "White noise RMS amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "nt";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makePinkNoiseParamInfos( wxString aPrefix, 
                                                                    wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "nalpha";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "1/f noise exponent";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "namp";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "1/f noise RMS amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "nt";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeBurstNoiseParamInfos( wxString aPrefix,
                                                                     wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "rtsam";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "rtscapt";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise trap capture time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "rtsemt";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise trap emission time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "nt";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeRandomUniformParamInfos( wxString aPrefix,
                                                                        wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "min";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "-0.5";
    paramInfo.description = "Min. value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "max";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0.5";
    paramInfo.description = "Max. value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeRandomNormalParamInfos( wxString aPrefix,
                                                                       wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "mean";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Mean";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "stddev";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Standard deviation";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeRandomExpParamInfos( wxString aPrefix,
                                                                    wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "offset";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mean";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Mean";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeRandomPoissonParamInfos( wxString aPrefix,
                                                                        wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "offset";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "lambda";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Mean";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}
