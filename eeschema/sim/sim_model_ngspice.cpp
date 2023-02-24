/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sim/sim_model_ngspice.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <fmt/core.h>


std::vector<std::string> SPICE_GENERATOR_NGSPICE::CurrentNames( const SPICE_ITEM& aItem ) const
{
    switch( m_model.GetTypeInfo().deviceType )
    {
        case SIM_MODEL::DEVICE_T::NPN:
        case SIM_MODEL::DEVICE_T::PNP:
            return { fmt::format( "I({}:c)", ItemName( aItem ) ),
                     fmt::format( "I({}:b)", ItemName( aItem ) ),
                     fmt::format( "I({}:e)", ItemName( aItem ) ) };

        case SIM_MODEL::DEVICE_T::NJFET:
        case SIM_MODEL::DEVICE_T::PJFET:
        case SIM_MODEL::DEVICE_T::NMES:
        case SIM_MODEL::DEVICE_T::PMES:
        case SIM_MODEL::DEVICE_T::NMOS:
        case SIM_MODEL::DEVICE_T::PMOS:
            return { fmt::format( "I({}:d)", ItemName( aItem ) ),
                     fmt::format( "I({}:g)", ItemName( aItem ) ),
                     fmt::format( "I({}:s)", ItemName( aItem ) ) };

        case SIM_MODEL::DEVICE_T::R:
        case SIM_MODEL::DEVICE_T::C:
        case SIM_MODEL::DEVICE_T::L:
        case SIM_MODEL::DEVICE_T::D:
            return SPICE_GENERATOR::CurrentNames( aItem );

        default:
            wxFAIL_MSG( "Unhandled model device type in SIM_MODEL_NGSPICE" );
            return {};
    }
}


SIM_MODEL_NGSPICE::SIM_MODEL_NGSPICE( TYPE aType ) :
    SIM_MODEL_SPICE( aType, std::make_unique<SPICE_GENERATOR_NGSPICE>( *this ) )
{
    const MODEL_INFO& modelInfo = ModelInfo( getModelType() );

    for( const SIM_MODEL::PARAM::INFO& paramInfo : modelInfo.instanceParams )
    {
        // For now, only the geometry parameters.
        if( paramInfo.category == SIM_MODEL::PARAM::CATEGORY::PRINCIPAL
            || paramInfo.category == SIM_MODEL::PARAM::CATEGORY::GEOMETRY )
        {
            AddParam( paramInfo );
        }
    }

    for( const SIM_MODEL::PARAM::INFO& paramInfo : modelInfo.modelParams )
        AddParam( paramInfo );
}


int SIM_MODEL_NGSPICE::doFindParam( const std::string& aParamName ) const
{
    // Special case to allow escaped model parameters (suffixed with "_")

    std::string lowerParamName = boost::to_lower_copy( aParamName );

    std::vector<std::reference_wrapper<const PARAM>> params = GetParams();

    for( int ii = 0; ii < (int) params.size(); ++ii )
    {
        const PARAM& param = params[ii];

        if( param.info.name == lowerParamName || param.info.name == lowerParamName + "_" )
            return ii;
    }

    return -1;
}


void SIM_MODEL_NGSPICE::SetParamFromSpiceCode( const std::string& aParamName,
                                               const std::string& aValue,
                                               SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    std::string paramName = boost::to_lower_copy( aParamName );

    // "level" and "version" are not really parameters - they're part of the type - so silently
    // ignore them.
    if( paramName == "level" || paramName == "version" )
        return;

    // First we try to use the name as is. Note that you can't set instance parameters from this
    // function, it's for ".model" cards, not for instantiations.

    std::string lowerParamName = boost::to_lower_copy( paramName );

    std::vector<std::reference_wrapper<const PARAM>> params = GetParams();

    for( int ii = 0; ii < (int) params.size(); ++ii )
    {
        const PARAM& param = params[ii];

        if( !param.info.isSpiceInstanceParam
            && param.info.category != PARAM::CATEGORY::SUPERFLUOUS
            && ( param.info.name == lowerParamName || param.info.name == lowerParamName + "_" ) )
        {
            SetParamValue( ii, aValue, aNotation );
            return;
        }
    }

    // One Spice param can have multiple names, we need to take this into account.

    // Now we search the base model parameters without excluding superfluous parameters (which
    // may be aliases to non-superfluous parameters).

    for( const PARAM::INFO& ngspiceParamInfo : ModelInfo( getModelType() ).modelParams )
    {
        if( ngspiceParamInfo.name == lowerParamName )
        {
            // Find an actual parameter with the same id.  Even if the ngspiceParam was
            // superfluous, its alias target might not be.
            for( int ii = 0; ii < (int) params.size(); ++ii )
            {
                const PARAM& param = params[ii];

                if( param.info.id == ngspiceParamInfo.id
                        && param.info.category != PARAM::CATEGORY::SUPERFLUOUS )
                {
                    SetParamValue( ii, aValue, aNotation );
                    return;
                }
            }

            break;
        }
    }

    if( !canSilentlyIgnoreParam( paramName ) )
    {
        THROW_IO_ERROR( wxString::Format( "Failed to set parameter '%s' to value '%s'",
                                          aParamName,
                                          aValue ) );
    }
}


bool SIM_MODEL_NGSPICE::canSilentlyIgnoreParam( const std::string& aParamName )
{
    // Ignore the purely informative LTspice-specific parameters "mfg" and "type".
    if( aParamName == "mfg" || aParamName == "type" )
        return true;

    if( GetDeviceType() == DEVICE_T::D )
    {
        if( aParamName == "perim"
            || aParamName == "isw"
            || aParamName == "ns"
            || aParamName == "rsw"
            || aParamName == "cjsw"
            || aParamName == "vjsw"
            || aParamName == "mjsw"
            || aParamName == "fcs" )
        {
            return true;
        }
    }

    if( GetDeviceType() == DEVICE_T::NPN || GetDeviceType() == DEVICE_T::PNP )
    {
        // Ignore the purely informative LTspice-specific parameters "icrating" and "vceo".
        if( aParamName == "icrating" || aParamName == "vceo" )
            return true;
    }

    if( GetType() == TYPE::NPN_GUMMELPOON || GetType() == TYPE::PNP_GUMMELPOON )
    {
        // Ignore unused parameters.
        if( aParamName == "bvcbo"
            || aParamName == "nbvcbo"
            || aParamName == "tbvcbo1"
            || aParamName == "tbvcbo2"
            || aParamName == "bvbe"
            || aParamName == "ibvbe"
            || aParamName == "nbvbe" )
        {
            return true;
        }
    }

    return false;
}


bool SIM_MODEL_NGSPICE::requiresSpiceModelLine() const
{
    for( int ii = 0; ii < GetParamCount(); ++ii )
    {
        const PARAM& param = m_params[ii];

        // Instance parameters are written in item lines
        if( param.info.isSpiceInstanceParam )
            continue;

        // Empty parameters are interpreted as default-value
        if ( param.value == "" )
            continue;

        // Any non-empty parameter must be written if there's no base model
        if( !m_baseModel )
            return true;

        const SIM_MODEL_NGSPICE* baseModel = dynamic_cast<const SIM_MODEL_NGSPICE*>( m_baseModel );
        std::string              baseValue = baseModel->m_params[ii].value;

        if( param.value == baseValue )
            continue;

        // One more check for equivalence, mostly for early 7.0 files which wrote all parameters
        // to the Sim.Params field in normalized format
        if( param.value == SIM_VALUE::Normalize( SIM_VALUE::ToDouble( baseValue ) ) )
            continue;

        // Overrides must be written
        return true;
    }

    return false;
}


std::vector<std::string> SIM_MODEL_NGSPICE::GetPinNames() const
{
    return ModelInfo( getModelType() ).pinNames;
}


SIM_MODEL_NGSPICE::MODEL_TYPE SIM_MODEL_NGSPICE::getModelType() const
{
    switch( GetType() )
    {
    case TYPE::NONE:                 return MODEL_TYPE::NONE;
    case TYPE::D:                    return MODEL_TYPE::DIODE;

    case TYPE::NPN_VBIC:
    case TYPE::PNP_VBIC:             return MODEL_TYPE::VBIC;
    case TYPE::NPN_GUMMELPOON:
    case TYPE::PNP_GUMMELPOON:       return MODEL_TYPE::BJT;
    case TYPE::NPN_HICUM2:
    case TYPE::PNP_HICUM2:           return MODEL_TYPE::HICUM2;

    case TYPE::NJFET_SHICHMANHODGES:
    case TYPE::PJFET_SHICHMANHODGES: return MODEL_TYPE::JFET;
    case TYPE::NJFET_PARKERSKELLERN:
    case TYPE::PJFET_PARKERSKELLERN: return MODEL_TYPE::JFET2;

    case TYPE::NMES_STATZ:
    case TYPE::PMES_STATZ:           return MODEL_TYPE::MES;
    case TYPE::NMES_YTTERDAL:
    case TYPE::PMES_YTTERDAL:        return MODEL_TYPE::MESA;
    case TYPE::NMES_HFET1:
    case TYPE::PMES_HFET1:           return MODEL_TYPE::HFET1;
    case TYPE::NMES_HFET2:
    case TYPE::PMES_HFET2:           return MODEL_TYPE::HFET2;

    case TYPE::NMOS_VDMOS:
    case TYPE::PMOS_VDMOS:           return MODEL_TYPE::VDMOS;
    case TYPE::NMOS_MOS1:
    case TYPE::PMOS_MOS1:            return MODEL_TYPE::MOS1;
    case TYPE::NMOS_MOS2:
    case TYPE::PMOS_MOS2:            return MODEL_TYPE::MOS2;
    case TYPE::NMOS_MOS3:
    case TYPE::PMOS_MOS3:            return MODEL_TYPE::MOS3;
    case TYPE::NMOS_BSIM1:
    case TYPE::PMOS_BSIM1:           return MODEL_TYPE::BSIM1;
    case TYPE::NMOS_BSIM2:
    case TYPE::PMOS_BSIM2:           return MODEL_TYPE::BSIM2;
    case TYPE::NMOS_MOS6:
    case TYPE::PMOS_MOS6:            return MODEL_TYPE::MOS6;
    case TYPE::NMOS_BSIM3:
    case TYPE::PMOS_BSIM3:           return MODEL_TYPE::BSIM3;
    case TYPE::NMOS_MOS9:
    case TYPE::PMOS_MOS9:            return MODEL_TYPE::MOS9;
    case TYPE::NMOS_B4SOI:
    case TYPE::PMOS_B4SOI:           return MODEL_TYPE::B4SOI;
    case TYPE::NMOS_BSIM4:
    case TYPE::PMOS_BSIM4:           return MODEL_TYPE::BSIM4;
    case TYPE::NMOS_B3SOIFD:
    case TYPE::PMOS_B3SOIFD:         return MODEL_TYPE::B3SOIFD;
    case TYPE::NMOS_B3SOIDD:
    case TYPE::PMOS_B3SOIDD:         return MODEL_TYPE::B3SOIDD;
    case TYPE::NMOS_B3SOIPD:
    case TYPE::PMOS_B3SOIPD:         return MODEL_TYPE::B3SOIPD;
    case TYPE::NMOS_HISIM2:
    case TYPE::PMOS_HISIM2:          return MODEL_TYPE::HISIM2;
    case TYPE::NMOS_HISIMHV1:
    case TYPE::PMOS_HISIMHV1:        return MODEL_TYPE::HISIMHV1;
    case TYPE::NMOS_HISIMHV2:
    case TYPE::PMOS_HISIMHV2:        return MODEL_TYPE::HISIMHV2;

    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_NGSPICE" );
        return MODEL_TYPE::NONE;
    }
}
