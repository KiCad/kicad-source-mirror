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

#include <sim/sim_model_source.h>

#include <fmt/core.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>


namespace SIM_MODEL_SOURCE_PARSER
{
    using namespace SIM_MODEL_SOURCE_GRAMMAR;

    template <typename Rule> struct pwlValuesSelector : std::false_type {};
    template <> struct pwlValuesSelector<number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>>
        : std::true_type {};
}


std::string SPICE_GENERATOR_SOURCE::ModelLine( const SPICE_ITEM& aItem ) const
{
    return "";
}

std::string SPICE_GENERATOR_SOURCE::TunerCommand( const SPICE_ITEM& aItem, double aValue ) const
{
    std::string result = "";

    switch( aItem.model->GetType() )
    {
    case SIM_MODEL::TYPE::V: // VDC/IDC: it is clear which parameter should be used
    case SIM_MODEL::TYPE::I:
        result = fmt::format( "alter @{}={:g}",
                              aItem.model->SpiceGenerator().ItemName( aItem ),
                              aValue );
        break;

    default:
        break; // other sources: unclear which parameter the user wants
    }
    return result;
}

std::string SPICE_GENERATOR_SOURCE::ItemLine( const SPICE_ITEM& aItem ) const
{
    SPICE_ITEM  item = aItem;

    std::string ac = "";
    std::string dc = "";

    if( const SIM_MODEL::PARAM* ac_param = m_model.FindParam( "ac" ) )
        ac = SIM_VALUE::ToSpice( ac_param->value );
    if( const SIM_MODEL::PARAM* dc_param = m_model.FindParam( "dc" ) )
        dc = SIM_VALUE::ToSpice( dc_param->value );

    bool emptyLine = true;
    item.modelName = "";

    // @FIXME
    // the keyword "DC" refers to both offset of a sine source, and value for DC analysis
    // Because of this, both values are always equal in a sine source.
    //
    // suggestion: rename the sine parameter from "DC" to "offset"

    if( dc != "" )
    {
        emptyLine = false;
        item.modelName += fmt::format( "DC {} ", dc );
    }

    if( m_model.GetSpiceInfo().functionName != ""
        && m_model.GetType() != SIM_MODEL::TYPE::V   // DC-only sources are already processed
        && m_model.GetType() != SIM_MODEL::TYPE::I )
    {
        std::string args = "";

        switch( m_model.GetType() )
        {
        case SIM_MODEL::TYPE::V_PWL:
        case SIM_MODEL::TYPE::I_PWL:
        {
            tao::pegtl::string_input<> in( m_model.GetParam( 0 ).value, "from_content" );
            std::unique_ptr<tao::pegtl::parse_tree::node> root;

            try
            {
                root = tao::pegtl::parse_tree::parse<SIM_MODEL_SOURCE_PARSER::pwlValuesGrammar,
                                                     SIM_MODEL_SOURCE_PARSER::pwlValuesSelector>( in );
            }
            catch( const tao::pegtl::parse_error& )
            {
                break;
            }

            if( root )
            {
                for( const auto& node : root->children )
                {
                    if( node->is_type<SIM_MODEL_SOURCE_PARSER::number<SIM_VALUE::TYPE_FLOAT,
                                                                      SIM_VALUE::NOTATION::SI>>() )
                    {
                        args.append( SIM_VALUE::ToSpice( node->string() ) + " " );
                    }
                }
            }

            break;
        }

        case SIM_MODEL::TYPE::V_WHITENOISE:
        case SIM_MODEL::TYPE::I_WHITENOISE:
            args.append( getParamValueString( "rms", "0" ) + " " );
            args.append( getParamValueString( "dt", "0" ) + " " );
            args.append( "0 0 0 0 0 " );
            break;

        case SIM_MODEL::TYPE::V_PINKNOISE:
        case SIM_MODEL::TYPE::I_PINKNOISE:
            args.append( "0 " );
            args.append( getParamValueString( "dt", "0" ) + " " );
            args.append( getParamValueString( "slope", "0" ) + " " );
            args.append( getParamValueString( "rms", "0" ) + " " );
            args.append( "0 0 0 " );
            break;

        case SIM_MODEL::TYPE::V_BURSTNOISE:
        case SIM_MODEL::TYPE::I_BURSTNOISE:
            args.append( "0 0 0 0 " );
            args.append( getParamValueString( "ampl", "0" ) + " " );
            args.append( getParamValueString( "tcapt", "0" ) + " " );
            args.append( getParamValueString( "temit", "0" ) + " " );
            break;

        case SIM_MODEL::TYPE::V_RANDUNIFORM:
        case SIM_MODEL::TYPE::I_RANDUNIFORM:
        {
            args.append( "1 " );
            args.append( getParamValueString( "ts", "0" ) + " " );
            args.append( getParamValueString( "td", "0" ) + " " );
            args.append( getParamValueString( "range", "1" ) + " " );
            args.append( getParamValueString( "offset", "0" ) + " " );
            break;
        }

        case SIM_MODEL::TYPE::V_RANDGAUSSIAN:
        case SIM_MODEL::TYPE::I_RANDGAUSSIAN:
            args.append( "2 " );
            args.append( getParamValueString( "ts", "0" ) + " " );
            args.append( getParamValueString( "td", "0" ) + " " );
            args.append( getParamValueString( "stddev", "1" ) + " " );
            args.append( getParamValueString( "mean", "0" ) + " " );
            break;

        case SIM_MODEL::TYPE::V_RANDEXP:
        case SIM_MODEL::TYPE::I_RANDEXP:
            args.append( "3 " );
            args.append( getParamValueString( "ts", "0" ) + " " );
            args.append( getParamValueString( "td", "0" ) + " " );
            args.append( getParamValueString( "mean", "1" ) + " " );
            args.append( getParamValueString( "offset", "0" ) + " " );
            break;

        case SIM_MODEL::TYPE::V_RANDPOISSON:
        case SIM_MODEL::TYPE::I_RANDPOISSON:
            args.append( "4 " );
            args.append( getParamValueString( "ts", "0" ) + " " );
            args.append( getParamValueString( "td", "0" ) + " " );
            args.append( getParamValueString( "lambda", "1" ) + " " );
            args.append( getParamValueString( "offset", "0" ) + " " );
            break;

        default:
            for( int ii = 0; ii < m_model.GetParamCount(); ++ii )
            {
                const SIM_MODEL::PARAM& param = m_model.GetParam( ii );

                if( ac != "" && ( param.Matches( "ac" ) || param.Matches( "ph" ) ) )
                    continue;

                std::string argStr = SIM_VALUE::ToSpice( param.value );

                if( argStr != "" )
                    args.append( argStr + " " );
            }

            break;
        }

        emptyLine = false;
        item.modelName += fmt::format( "{}( {}) ", m_model.GetSpiceInfo().functionName, args );
    }
    else
    {
        switch( m_model.GetType() )
        case SIM_MODEL::TYPE::V_VCL:
        case SIM_MODEL::TYPE::I_VCL:
        {
            item.modelName += fmt::format( "{} ", getParamValueString( "gain", "1.0" ) );
            emptyLine = false;

            break;

        case SIM_MODEL::TYPE::V_CCL:
        case SIM_MODEL::TYPE::I_CCL:
            item.modelName += fmt::format( "{} {} ",
                                           getParamValueString( "control", "V?" ),
                                           getParamValueString( "gain", "1.0" ) );
            emptyLine = false;
            break;

        default:
            break;
        }
    }

    if( ac != "" )
    {
        std::string ph = "";

        if( const SIM_MODEL::PARAM* ph_param = m_model.FindParam( "ph" ) )
            ph = SIM_VALUE::ToSpice( ph_param->value );

        emptyLine = false;
        item.modelName += fmt::format( "AC {} {} ", ac, ph );
    }

    std::string portnum = "";

    if( const SIM_MODEL::PARAM* portnum_param = m_model.FindParam( "portnum" ) )
        portnum = SIM_VALUE::ToSpice( portnum_param->value );

    if( portnum != "" )
    {
        item.modelName += fmt::format( "portnum {} ", portnum );

        std::string z0 = "";

        if( const SIM_MODEL::PARAM* z0_param = m_model.FindParam( "z0" ) )
            z0 = SIM_VALUE::ToSpice( z0_param->value );

        if( z0 != "" )
            item.modelName += fmt::format( "z0 {} ", z0 );
    }

    if( emptyLine )
    {
        item.modelName = SIM_VALUE::ToSpice( m_model.GetParam( 0 ).value );
    }

    return SPICE_GENERATOR::ItemLine( item );
}


std::string SPICE_GENERATOR_SOURCE::getParamValueString( const std::string& aParamName,
                                                         const std::string& aDefaultValue ) const
{
    std::string result = "";

    if ( m_model.FindParam( aParamName ) )
        result = SIM_VALUE::ToSpice( m_model.FindParam( aParamName )->value );

    if( result == "" )
        result = aDefaultValue;

    return result;
}


SIM_MODEL_SOURCE::SIM_MODEL_SOURCE( TYPE aType ) :
        SIM_MODEL( aType, std::make_unique<SPICE_GENERATOR_SOURCE>( *this ),
                   std::make_unique<SIM_MODEL_SOURCE_SERIALIZER>( *this ) )
{
    for( const SIM_MODEL::PARAM::INFO& paramInfo : makeParamInfos( aType ) )
        AddParam( paramInfo );
}


void SIM_MODEL_SOURCE::doSetParamValue( int aParamIndex, const std::string& aValue )
{
    // Sources are special. All preceding parameter values must be filled. If they are not, fill
    // them out automatically. If a value is nulled, delete everything after it.
    if( aValue.empty() )
    {
        for( int paramIndex = aParamIndex; paramIndex < GetParamCount(); ++paramIndex )
        {
            m_params.at( aParamIndex ).value = "";
        }
    }
    else
    {
        for( int paramIndex = 0; paramIndex < aParamIndex; ++paramIndex )
        {
            if( GetParam( paramIndex ).value == "" )
            {
                double   dummy;
                wxString defaultValue = m_params.at( aParamIndex ).info.defaultValue;

                if( !defaultValue.ToDouble( &dummy ) )
                    defaultValue = wxT( "0" );

                m_params.at( aParamIndex ).value = defaultValue;
                SIM_MODEL::SetParamValue( paramIndex, defaultValue.ToStdString() );
            }
        }
    }

    return SIM_MODEL::doSetParamValue( aParamIndex, aValue );
}


const std::vector<SIM_MODEL::PARAM::INFO>& SIM_MODEL_SOURCE::makeParamInfos( TYPE aType )
{
    static std::vector<PARAM::INFO> vdc = makeDcParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> idc = makeDcParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vsin = makeSinParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> isin = makeSinParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vpulse = makePulseParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> ipulse = makePulseParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vexp = makeExpParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> iexp = makeExpParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vam = makeAMParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> iam = makeAMParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vsffm = makeSFFMParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> isffm = makeSFFMParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vcvs = makeVcParamInfos( "" );
    static std::vector<PARAM::INFO> ccvs = makeCcParamInfos( "ohm" );
    static std::vector<PARAM::INFO> vpwl = makePwlParamInfos( "y", "Voltage", "V" );

    static std::vector<PARAM::INFO> vccs = makeVcParamInfos( "S" );
    static std::vector<PARAM::INFO> cccs = makeCcParamInfos( "" );
    static std::vector<PARAM::INFO> ipwl = makePwlParamInfos( "y", "Current", "A" );

    static std::vector<PARAM::INFO> vwhitenoise = makeWhiteNoiseParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> iwhitenoise = makeWhiteNoiseParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vpinknoise = makePinkNoiseParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> ipinknoise = makePinkNoiseParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vburstnoise = makeBurstNoiseParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> iburstnoise = makeBurstNoiseParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vrandomuniform = makeRandomUniformParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> irandomuniform = makeRandomUniformParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vrandomnormal = makeRandomNormalParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> irandomnormal = makeRandomNormalParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vrandomexp = makeRandomExpParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> irandomexp = makeRandomExpParamInfos( "y", "A" );

    static std::vector<PARAM::INFO> vrandompoisson = makeRandomPoissonParamInfos( "y", "V" );
    static std::vector<PARAM::INFO> irandompoisson = makeRandomPoissonParamInfos( "y", "A" );

    switch( aType )
    {
    case TYPE::V:              return vdc;
    case TYPE::I:              return idc;
    case TYPE::V_SIN:          return vsin;
    case TYPE::I_SIN:          return isin;
    case TYPE::V_PULSE:        return vpulse;
    case TYPE::I_PULSE:        return ipulse;
    case TYPE::V_EXP:          return vexp;
    case TYPE::I_EXP:          return iexp;
    case TYPE::V_AM:           return vam;
    case TYPE::I_AM:           return iam;
    case TYPE::V_SFFM:         return vsffm;
    case TYPE::I_SFFM:         return isffm;
    case TYPE::V_VCL:          return vcvs;
    case TYPE::V_CCL:          return ccvs;
    case TYPE::V_PWL:          return vpwl;
    case TYPE::I_VCL:          return vccs;
    case TYPE::I_CCL:          return cccs;
    case TYPE::I_PWL:          return ipwl;
    case TYPE::V_WHITENOISE:   return vwhitenoise;
    case TYPE::I_WHITENOISE:   return iwhitenoise;
    case TYPE::V_PINKNOISE:    return vpinknoise;
    case TYPE::I_PINKNOISE:    return ipinknoise;
    case TYPE::V_BURSTNOISE:   return vburstnoise;
    case TYPE::I_BURSTNOISE:   return iburstnoise;
    case TYPE::V_RANDUNIFORM:  return vrandomuniform;
    case TYPE::I_RANDUNIFORM:  return irandomuniform;
    case TYPE::V_RANDGAUSSIAN: return vrandomnormal;
    case TYPE::I_RANDGAUSSIAN: return irandomnormal;
    case TYPE::V_RANDEXP:      return vrandomexp;
    case TYPE::I_RANDEXP:      return irandomexp;
    case TYPE::V_RANDPOISSON:  return vrandompoisson;
    case TYPE::I_RANDPOISSON:  return irandompoisson;
    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_SOURCE" );
        static std::vector<SIM_MODEL::PARAM::INFO> empty;
        return empty;
    }
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeDcParamInfos( const std::string& aPrefix,
                                                                        const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "DC value";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeSinParamInfos( const std::string& aPrefix,
                                                                         const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "dc";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "ampl";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "f";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1/tstop";
    paramInfo.description = "Frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "theta";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "1/s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Damping factor";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phase";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "°";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Phase";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makePulseParamInfos( const std::string& aPrefix,
                                                                           const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "1";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Initial value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix + "2";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Pulsed value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tr";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Rise time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tf";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Fall time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tw"; // Ngspice calls it "pw".
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstop";
    paramInfo.description = "Pulse width";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "per";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstop";
    paramInfo.description = "Period";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "np";
    paramInfo.type = SIM_VALUE::TYPE_INT;
    paramInfo.unit = "";
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Number of pulses";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeExpParamInfos( const std::string& aPrefix,
                                                                         const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = aPrefix + "1";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Initial value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix + "2";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Pulsed value";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td1";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Rise delay time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tau1";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Rise time constant";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td2";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "td1+tstep";
    paramInfo.description = "Fall delay time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tau2";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "tstep";
    paramInfo.description = "Fall time constant";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeAMParamInfos( const std::string& aPrefix,
                                                                        const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "vo";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Overall offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "vmo";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulation signal offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "vma";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulation signal amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fm";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "5/tstop";
    paramInfo.description = "Modulation signal frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fc";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "500/tstop";
    paramInfo.description = "Carrier signal frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Overall delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phasem";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "°";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Modulation signal phase";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phasec";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "°";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Carrier signal phase";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeSFFMParamInfos( const std::string& aPrefix,
                                                                          const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "vo";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "DC offset";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "va";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fm";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "5/tstop";
    paramInfo.description = "Modulating frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mdi";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Modulation index";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "fc";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "Hz";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "500/tstop";
    paramInfo.description = "Carrier frequency";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phasem";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "°";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Modulating signal phase";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "phasec";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "°";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Carrier signal phase";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeCcParamInfos( const std::string& aGainUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "gain";
    paramInfo.id = 1;
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aGainUnit;
    paramInfo.description = "Gain";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "control";
    paramInfo.id = 2;
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "";
    paramInfo.description = "Controlling voltage source";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeVcParamInfos( const std::string& aGainUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "gain";
    paramInfo.id = 1;
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aGainUnit;
    paramInfo.description = "Gain";
    paramInfos.push_back( paramInfo );

    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makePwlParamInfos( const std::string& aPrefix,
                                                                         const std::string& aQuantity,
                                                                         const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "pwl";
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = "s," + aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = aUnit == "V" ? "Time-voltage points" : "Time-current points";
    paramInfos.push_back( paramInfo );

    // TODO: Ngspice doesn't support "td" and "r" for current sources, so let's disable that for
    // now.

    /*paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = aUnit == "V" ? "Time-voltage points" : "Time-current points";
    paramInfo.isSpiceInstanceParam = true;
    paramInfos.push_back( paramInfo );

    paramInfo.name = "repeat";
    paramInfo.type = SIM_VALUE::TYPE_BOOL;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Repeat forever";
    paramInfo.isSpiceInstanceParam = true;
    paramInfo.spiceInstanceName = "r";
    paramInfos.push_back( paramInfo );*/

    /*paramInfo.name = "t";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT_VECTOR;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Time vector";
    paramInfos.push_back( paramInfo );

    paramInfo.name = aPrefix;
    paramInfo.type = SIM_VALUE::TYPE_FLOAT_VECTOR;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = aQuantity + " vector";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "repeat";
    paramInfo.type = SIM_VALUE::TYPE_BOOL;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Repeat forever";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );*/

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeWhiteNoiseParamInfos( const std::string& aPrefix,
                                                                                const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "rms";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "White noise RMS amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "dt";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makePinkNoiseParamInfos( const std::string& aPrefix,
                                                                               const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "rms";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "1/f noise RMS amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "slope";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "1/f noise exponent";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "dt";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Time step";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeBurstNoiseParamInfos( const std::string& aPrefix,
                                                                                const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "ampl";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise amplitude";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "tcapt";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise trap capture time";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "temit";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Burst noise trap emission time";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeRandomUniformParamInfos( const std::string& aPrefix,
                                                                                   const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "ts";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Individual voltage duration";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "range";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Range";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "offset";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Offset";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeRandomNormalParamInfos( const std::string& aPrefix,
                                                                                  const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "ts";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Individual voltage duration";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "stddev";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Standard deviation";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mean";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Mean";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeRandomExpParamInfos( const std::string& aPrefix,
                                                                               const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "ts";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Individual voltage duration";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "mean";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Mean";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "offset";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Offset";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SOURCE::makeRandomPoissonParamInfos( const std::string& aPrefix,
                                                                                   const std::string& aUnit )
{
    std::vector<PARAM::INFO> paramInfos;
    PARAM::INFO paramInfo;

    paramInfo.name = "ts";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "";
    paramInfo.description = "Individual voltage duration";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "td";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "s";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Delay";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "lambda";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "1";
    paramInfo.description = "Lambda";
    paramInfos.push_back( paramInfo );

    paramInfo.name = "offset";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.defaultValue = "0";
    paramInfo.description = "Offset";
    paramInfos.push_back( paramInfo );

    appendAcParamInfos( paramInfos, aUnit );
    appendSpParamInfos( paramInfos, aUnit );
    return paramInfos;
}

void SIM_MODEL_SOURCE::appendAcParamInfos( std::vector<PARAM::INFO>& aParamInfos, const std::string& aUnit )
{
    PARAM::INFO paramInfo;

    paramInfo.name = "ac";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::AC;
    paramInfo.defaultValue = "0";
    paramInfo.description = "AC magnitude";
    aParamInfos.push_back( paramInfo );

    paramInfo.name = "ph";
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = "°";
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::AC;
    paramInfo.defaultValue = "0";
    paramInfo.description = "AC phase";
    aParamInfos.push_back( paramInfo );
}

void SIM_MODEL_SOURCE::appendSpParamInfos( std::vector<PARAM::INFO>& aParamInfos,
                                           const std::string&        aUnit )
{
    PARAM::INFO paramInfo;

    if( !strcmp( aUnit.c_str(), "V" ) )
    {
        paramInfo.name = "portnum";
        paramInfo.type = SIM_VALUE::TYPE_INT;
        paramInfo.unit = "";
        paramInfo.category = SIM_MODEL::PARAM::CATEGORY::S_PARAM;
        paramInfo.defaultValue = "";
        paramInfo.description = "Port number";
        aParamInfos.push_back( paramInfo );

        paramInfo.name = "z0";
        paramInfo.type = SIM_VALUE::TYPE_FLOAT;
        paramInfo.unit = "Ohm";
        paramInfo.category = SIM_MODEL::PARAM::CATEGORY::S_PARAM;
        paramInfo.defaultValue = "";
        paramInfo.description = "Internal impedance";
        aParamInfos.push_back( paramInfo );
    }
}


std::vector<std::string> SIM_MODEL_SOURCE::GetPinNames() const
{
    if( GetDeviceType() == SIM_MODEL::DEVICE_T::E || GetDeviceType() == SIM_MODEL::DEVICE_T::G )
        return { "+", "-", "C+", "C-" };
    else
        return { "+", "-" };
}

const SIM_MODEL::PARAM* SIM_MODEL_SOURCE::GetTunerParam() const
{
    switch( GetType() )
    {
    case SIM_MODEL::TYPE::V: // VDC/IDC: it is clear which parameter should be used
    case SIM_MODEL::TYPE::I:
        return &GetParam( 0 );

    default:
        break; // other sources: unclear which parameter the user wants
    }
    return nullptr;
}
