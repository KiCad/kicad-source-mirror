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
    : SIM_MODEL( aType )
{
    for( const PARAM::INFO& paramInfo : makeParams( aType ) )
        AddParam( paramInfo );
}


wxString SIM_MODEL_SOURCE::GenerateSpiceIncludeLine( const wxString& aLibraryFilename ) const
{
    return "";
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

    for( int i = 0; i < GetParamCount(); ++i )
        argList << GetParam( i ).value->ToString() << " ";

    wxString model = wxString::Format( GetSpiceInfo().inlineTypeString + "( %s)", argList );

    return SIM_MODEL::GenerateSpiceItemLine( aRefName, model, aPinNetNames );
}


const std::vector<PARAM::INFO>& SIM_MODEL_SOURCE::makeParams( TYPE aType )
{
    static std::vector<PARAM::INFO> vpulse = makePulse( "v", "V" );
    static std::vector<PARAM::INFO> ipulse = makePulse( "i", "A" );

    static std::vector<PARAM::INFO> vsin = makeSin( "v", "V" );
    static std::vector<PARAM::INFO> isin = makeSin( "i", "A" );

    static std::vector<PARAM::INFO> vexp = makeExp( "v", "V" );
    static std::vector<PARAM::INFO> iexp = makeExp( "i", "A" );

    static std::vector<PARAM::INFO> vsfam = makeSfam( "v", "V" );
    static std::vector<PARAM::INFO> isfam = makeSfam( "i", "A" );

    static std::vector<PARAM::INFO> vsffm = makeSffm( "v", "V" );
    static std::vector<PARAM::INFO> isffm = makeSffm( "i", "A" );

    static std::vector<PARAM::INFO> vpwl = makePwl( "v", "Voltage", "V" );
    static std::vector<PARAM::INFO> ipwl = makePwl( "i", "Current", "A" );

    static std::vector<PARAM::INFO> vwhitenoise = makeWhiteNoise( "v", "V" );
    static std::vector<PARAM::INFO> iwhitenoise = makeWhiteNoise( "i", "A" );

    static std::vector<PARAM::INFO> vpinknoise = makePinkNoise( "v", "V" );
    static std::vector<PARAM::INFO> ipinknoise = makePinkNoise( "i", "A" );

    static std::vector<PARAM::INFO> vburstnoise = makeBurstNoise( "v", "V" );
    static std::vector<PARAM::INFO> iburstnoise = makeBurstNoise( "i", "A" );

    static std::vector<PARAM::INFO> vrandomuniform = makeRandomUniform( "v", "V" );
    static std::vector<PARAM::INFO> irandomuniform = makeRandomUniform( "i", "A" );

    static std::vector<PARAM::INFO> vrandomnormal = makeRandomNormal( "v", "V" );
    static std::vector<PARAM::INFO> irandomnormal = makeRandomNormal( "i", "A" );

    static std::vector<PARAM::INFO> vrandomexp = makeRandomExp( "v", "V" );
    static std::vector<PARAM::INFO> irandomexp = makeRandomExp( "i", "A" );

    static std::vector<PARAM::INFO> vrandompoisson = makeRandomPoisson( "v", "V" );
    static std::vector<PARAM::INFO> irandompoisson = makeRandomPoisson( "i", "A" );

    switch( aType )
    {
    case TYPE::VSOURCE_PULSE:          return vpulse;
    case TYPE::ISOURCE_PULSE:          return ipulse;
    case TYPE::VSOURCE_SIN:            return vsin;
    case TYPE::ISOURCE_SIN:            return isin;
    case TYPE::VSOURCE_EXP:            return vexp;
    case TYPE::ISOURCE_EXP:            return iexp;
    case TYPE::VSOURCE_SFAM:           return vsfam;
    case TYPE::ISOURCE_SFAM:           return isfam;
    case TYPE::VSOURCE_SFFM:           return vsffm;
    case TYPE::ISOURCE_SFFM:           return isffm;
    case TYPE::VSOURCE_PWL:            return vpwl;
    case TYPE::ISOURCE_PWL:            return ipwl;
    case TYPE::VSOURCE_WHITE_NOISE:    return vwhitenoise;
    case TYPE::ISOURCE_WHITE_NOISE:    return iwhitenoise;
    case TYPE::VSOURCE_PINK_NOISE:     return vpinknoise;
    case TYPE::ISOURCE_PINK_NOISE:     return ipinknoise;
    case TYPE::VSOURCE_BURST_NOISE:    return vburstnoise;
    case TYPE::ISOURCE_BURST_NOISE:    return iburstnoise;
    case TYPE::VSOURCE_RANDOM_UNIFORM: return vrandomuniform;
    case TYPE::ISOURCE_RANDOM_UNIFORM: return irandomuniform;
    case TYPE::VSOURCE_RANDOM_NORMAL:  return vrandomnormal;
    case TYPE::ISOURCE_RANDOM_NORMAL:  return irandomnormal;
    case TYPE::VSOURCE_RANDOM_EXP:     return vrandomexp;
    case TYPE::ISOURCE_RANDOM_EXP:     return irandomexp;
    case TYPE::VSOURCE_RANDOM_POISSON: return vrandompoisson;
    case TYPE::ISOURCE_RANDOM_POISSON: return irandompoisson;
    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_SOURCE" );
        static std::vector<PARAM::INFO> empty;
        return empty;
    }
}


bool SIM_MODEL_SOURCE::SetParamValue( int aParamIndex, const wxString& aValue )
{
    // Sources are special. All preceding parameter values must be filled. If they are not, fill
    // them out automatically. If a value is nulled, delete everything after it.
    if( aValue.IsEmpty() )
    {
        for( int i = aParamIndex; i < GetParamCount(); ++i )
            SIM_MODEL::SetParamValue( i, "" );
    }
    else
    {
        for( int i = 0; i < aParamIndex; ++i )
        {
            if( GetParam( i ).value->ToString().IsEmpty() )
                SIM_MODEL::SetParamValue( i, "0" );
        }
    }

    return SIM_MODEL::SetParamValue( aParamIndex, aValue );
}


std::vector<wxString> SIM_MODEL_SOURCE::getPinNames() const
{
    return { "+", "-" };
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makePulse( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "1";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Initial value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix + "2";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Initial value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tr";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Rise time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tf";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Fall time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "pw";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstop";
    paramInfo.description = "Pulse width";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "per";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstop";
    paramInfo.description = "Period";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phase";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "deg";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Phase";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeSin( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "o";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix + "a";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "freq";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1/tstop";
    paramInfo.description = "Frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "theta";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "1/s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Damping factor";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phase";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "deg";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Phase";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeExp( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "1";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Initial value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix + "2";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Pulsed value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td1";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Rise delay time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tau1";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Rise time constant";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td2";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "td1+tstep";
    paramInfo.description = "Fall delay time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tau2";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Fall time constant";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeSfam( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "o";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";

    paramInfo.name = aPrefix + "a";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Amplitude";

    paramInfos.push_back( paramInfo );
    paramInfo.name = "mo";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulating signal offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fc";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Carrier frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mf";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulating frequency";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeSffm( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "o";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix + "a";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fc";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1/tstop";
    paramInfo.description = "Carrier frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mdi";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulation index";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fs";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1/tstop";
    paramInfo.description = "Signal frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phasec";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "deg";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Carrier phase";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phases";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "deg";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Signal phase";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makePwl( wxString aPrefix, wxString aQuantity,
                                                    wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "t";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT_VECTOR;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Time vector";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix;
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT_VECTOR;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = aQuantity + " vector";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "repeat";
    paramInfo.type = SIM_VALUE_BASE::TYPE::BOOL;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Repeat forever";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeWhiteNoise( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "o";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "na";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "White noise RMS amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "nt";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makePinkNoise( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "o";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "nalpha";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "1/f noise exponent";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "namp";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "1/f noise RMS amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "nt";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeBurstNoise( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "rtsam";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "rtscapt";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise trap capture time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "rtsemt";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise trap emission time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "nt";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeRandomUniform( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "min";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "-0.5";
    paramInfo.description = "Min. value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "max";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0.5";
    paramInfo.description = "Max. value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeRandomNormal( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "mean";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Mean";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "stddev";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Standard deviation";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeRandomExp( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "offset";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mean";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Mean";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<PARAM::INFO> SIM_MODEL_SOURCE::makeRandomPoisson( wxString aPrefix, wxString aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "offset";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "lambda";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Mean";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}
