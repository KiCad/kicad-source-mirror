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

#include "sim/sim_model_ngspice.h"

#include <boost/algorithm/string.hpp>
#include <fmt/core.h>
#include <ki_exception.h>


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
            return {};
    }
}


SIM_MODEL_NGSPICE::SIM_MODEL_NGSPICE( TYPE aType ) :
    SIM_MODEL_SPICE( aType, std::make_unique<SPICE_GENERATOR_NGSPICE>( *this ) )
{
    const MODEL_INFO& modelInfo = ModelInfo( getModelType() );

    for( const SIM_MODEL::PARAM::INFO& paramInfo : modelInfo.instanceParams )
    {
        // For now, only the geometry and flags parameters.
        if( paramInfo.category == SIM_MODEL::PARAM::CATEGORY::PRINCIPAL
            || paramInfo.category == SIM_MODEL::PARAM::CATEGORY::GEOMETRY
            || paramInfo.category == SIM_MODEL::PARAM::CATEGORY::FLAGS )
        {
            AddParam( paramInfo );
        }
    }

    for( const SIM_MODEL::PARAM::INFO& paramInfo : modelInfo.modelParams )
        AddParam( paramInfo );
}


int SIM_MODEL_NGSPICE::doFindParam( const std::string& aParamName ) const
{
    for( int ii = 0; ii < (int) GetParamCount(); ++ii )
    {
        const PARAM& param = GetParam( ii );

        if( param.Matches( aParamName ) )
            return ii;
    }

    // Look for escaped param names as a second pass (as they're less common)
    for( int ii = 0; ii < (int) GetParamCount(); ++ii )
    {
        const PARAM& param = GetParam( ii );

        if( !param.info.name.ends_with( '_' ) )
            continue;

        if( param.Matches( aParamName + "_" ) )
            return ii;
    }

    return -1;
}


void SIM_MODEL_NGSPICE::SetParamFromSpiceCode( const std::string& aParamName,
                                               const std::string& aValue,
                                               SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    // "level" and "version" are not really parameters - they're part of the type - so silently
    // ignore them.
    if( boost::iequals( aParamName, "level" ) || boost::iequals( aParamName, "version" ) )
        return;

    // First we try to use the name as is. Note that you can't set instance parameters from this
    // function, it's for ".model" cards, not for instantiations.

    for( int ii = 0; ii < (int) GetParamCount(); ++ii )
    {
        const PARAM& param = GetParam( ii );

        if( param.info.isSpiceInstanceParam || param.info.category == PARAM::CATEGORY::SUPERFLUOUS )
            continue;

        if( param.Matches( aParamName ) )
        {
            SetParamValue( ii, aValue, aNotation );
            return;
        }
    }

    // Look for escaped param names as a second pass (as they're less common)
    for( int ii = 0; ii < (int) GetParamCount(); ++ii )
    {
        const PARAM& param = GetParam( ii );

        if( param.info.isSpiceInstanceParam || param.info.category == PARAM::CATEGORY::SUPERFLUOUS )
            continue;

        if( !param.info.name.ends_with( '_' ) )
            continue;

        if( param.Matches( aParamName + "_" ) )
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
        if( ngspiceParamInfo.Matches( aParamName ) )
        {
            // Find an actual parameter with the same id.  Even if the ngspiceParam was
            // superfluous, its alias target might not be.
            for( int ii = 0; ii < (int) GetParamCount(); ++ii )
            {
                const PARAM::INFO& paramInfo = GetParam( ii ).info;

                if( paramInfo.category == PARAM::CATEGORY::SUPERFLUOUS )
                    continue;

                if( paramInfo.id == ngspiceParamInfo.id )
                {
                    SetParamValue( ii, aValue, aNotation );
                    return;
                }
            }

            break;
        }
    }

    if( !canSilentlyIgnoreParam( aParamName ) )
        THROW_IO_ERROR( wxString::Format( "Unknown simulation model parameter '%s'", aParamName ) );
}


bool SIM_MODEL_NGSPICE::canSilentlyIgnoreParam( const std::string& aParamName )
{
    // Ignore the purely informative LTspice-specific parameters "mfg" and "type".
    if( boost::iequals( aParamName, "mfg" ) || boost::iequals( aParamName, "type" ) )
        return true;

    if( GetDeviceType() == DEVICE_T::D )
    {
        if( boost::iequals( aParamName, "perim" )
            || boost::iequals( aParamName, "isw" )
            || boost::iequals( aParamName, "ns" )
            || boost::iequals( aParamName, "rsw" )
            || boost::iequals( aParamName, "cjsw" )
            || boost::iequals( aParamName, "vjsw" )
            || boost::iequals( aParamName, "mjsw" )
            || boost::iequals( aParamName, "fcs" ) )
        {
            return true;
        }
    }

    if( GetDeviceType() == DEVICE_T::NPN || GetDeviceType() == DEVICE_T::PNP )
    {
        // Ignore the purely informative LTspice-specific parameters "icrating" and "vceo".
        if( boost::iequals( aParamName, "icrating" ) || boost::iequals( aParamName, "vceo" ) )
            return true;
    }

    if( GetType() == TYPE::NPN_GUMMELPOON || GetType() == TYPE::PNP_GUMMELPOON )
    {
        // Ignore unused parameters.
        if( boost::iequals( aParamName, "bvcbo" )
            || boost::iequals( aParamName, "nbvcbo" )
            || boost::iequals( aParamName, "tbvcbo1" )
            || boost::iequals( aParamName, "tbvcbo2" )
            || boost::iequals( aParamName, "bvbe" )
            || boost::iequals( aParamName, "ibvbe" )
            || boost::iequals( aParamName, "nbvbe" ) )
        {
            return true;
        }
    }

    if( GetType() == TYPE::NMOS_VDMOS || GetType() == TYPE::PMOS_VDMOS )
    {
        // Ignore the purely informative LTspice-specific parameters "Vds", "Ron" and "Qg".
        if( boost::iequals( aParamName, "vds" )
            || boost::iequals( aParamName, "ron" )
            || boost::iequals( aParamName, "qg" ) )
        {
            return true;
        }
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


static std::mutex                              s_ModelInfoMapMutex;
static std::unique_ptr<NGSPICE_MODEL_INFO_MAP> s_ModelInfoMap;

const SIM_MODEL_NGSPICE::MODEL_INFO& SIM_MODEL_NGSPICE::ModelInfo( MODEL_TYPE aType )
{
    // Because spice netlisting has paralleizing, and we can encounter a first init of
    // s_ModelInfoMap as a result
    // Note, we could just init this at the static variable declaration above
    // or do away with making it a unique_ptr altogether and static decl the map directly
    // but its a real big boi if you look at what it will contain
    // so lets avoid pulling it to memory if a user isn't simming
    if( !s_ModelInfoMap )
    {
        std::lock_guard<std::mutex> lock( s_ModelInfoMapMutex );

        // Someone else may have filled it in while we were waiting for the lock, so
        // check it again.
        if( !s_ModelInfoMap )
            s_ModelInfoMap = std::make_unique<NGSPICE_MODEL_INFO_MAP>();
    }

    return s_ModelInfoMap->modelInfos.at( aType );
}


NGSPICE_MODEL_INFO_MAP::NGSPICE_MODEL_INFO_MAP()
{
    modelInfos[SIM_MODEL_NGSPICE::MODEL_TYPE::NONE] = {};
    addBJT();
    addBSIM1();
    addBSIM2();
    addBSIM3();
    addBSIM4();
    addB3SOI();
    addB4SOI();
    addDIODE();
    addHFET();
    addHICUM2();
    addHSIM();
    addJFET();
    addMES();
    addMOS();
    addMOS6();
    addMOS9();
    addVBIC();
}