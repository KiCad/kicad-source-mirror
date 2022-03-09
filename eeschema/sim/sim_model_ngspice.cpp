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

#include <sim/sim_model_ngspice.h>

using TYPE = SIM_MODEL::TYPE;


template SIM_MODEL_NGSPICE::SIM_MODEL_NGSPICE( TYPE aType, int symbolPinCount,
                                               const std::vector<void>* aFields );
template SIM_MODEL_NGSPICE::SIM_MODEL_NGSPICE( TYPE aType, int symbolPinCount,
                                               const std::vector<SCH_FIELD>* aFields );
template SIM_MODEL_NGSPICE::SIM_MODEL_NGSPICE( TYPE aType, int symbolPinCount,
                                               const std::vector<LIB_FIELD>* aFields );

template <typename T>
SIM_MODEL_NGSPICE::SIM_MODEL_NGSPICE( TYPE aType, int symbolPinCount,
                                      const std::vector<T>* aFields )
    : SIM_MODEL( aType )
{
    const NGSPICE::MODEL_INFO& modelInfo = NGSPICE::ModelInfo( getModelType() );

    for( const SIM_MODEL::PARAM::INFO& paramInfo : modelInfo.modelParams )
    {
        Params().emplace_back( paramInfo );
        Params().back().isOtherVariant = getIsOtherVariant();
    }

    for( const SIM_MODEL::PARAM::INFO& paramInfo : modelInfo.instanceParams )
    {
        Params().emplace_back( paramInfo );
        Params().back().isOtherVariant = getIsOtherVariant();
    }

    ReadDataFields( symbolPinCount, aFields );
}


void SIM_MODEL_NGSPICE::WriteCode( wxString& aCode )
{
    // TODO
}


std::vector<wxString> SIM_MODEL_NGSPICE::getPinNames()
{
    return NGSPICE::ModelInfo( getModelType() ).pinNames;
}


NGSPICE::MODEL_TYPE SIM_MODEL_NGSPICE::getModelType()
{
    switch( GetType() )
    {
    case TYPE::NONE:                return NGSPICE::MODEL_TYPE::NONE;
    case TYPE::RESISTOR_ADVANCED:   return NGSPICE::MODEL_TYPE::RESISTOR;
    case TYPE::CAPACITOR_ADVANCED:  return NGSPICE::MODEL_TYPE::CAPACITOR;
    case TYPE::INDUCTOR_ADVANCED:   return NGSPICE::MODEL_TYPE::INDUCTOR;
    case TYPE::TLINE_LOSSY:         return NGSPICE::MODEL_TYPE::LTRA;
    case TYPE::TLINE_LOSSLESS:      return NGSPICE::MODEL_TYPE::TRANLINE;
    case TYPE::TLINE_UNIFORM_RC:    return NGSPICE::MODEL_TYPE::URC;
    //case TYPE::TLINE_KSPICE:        return NGSPICE::MODEL_TYPE::TRANSLINE;
    case TYPE::SWITCH_VCTRL:        return NGSPICE::MODEL_TYPE::SWITCH;
    case TYPE::SWITCH_ICTRL:        return NGSPICE::MODEL_TYPE::CSWITCH;
    case TYPE::DIODE:               return NGSPICE::MODEL_TYPE::DIODE;

    case TYPE::NPN_GUMMEL_POON:
    case TYPE::PNP_GUMMEL_POON:     return NGSPICE::MODEL_TYPE::BJT;
    case TYPE::NPN_VBIC:
    case TYPE::PNP_VBIC:            return NGSPICE::MODEL_TYPE::VBIC;
    case TYPE::NPN_HICUM_L2:
    case TYPE::PNP_HICUM_L2:        return NGSPICE::MODEL_TYPE::HICUM2;

    case TYPE::NJF_SHICHMAN_HODGES:
    case TYPE::PJF_SHICHMAN_HODGES: return NGSPICE::MODEL_TYPE::JFET;
    case TYPE::NJF_PARKER_SKELLERN:
    case TYPE::PJF_PARKER_SKELLERN: return NGSPICE::MODEL_TYPE::JFET2;

    case TYPE::NMES_STATZ:
    case TYPE::PMES_STATZ:          return NGSPICE::MODEL_TYPE::MES;
    case TYPE::NMES_YTTERDAL:
    case TYPE::PMES_YTTERDAL:       return NGSPICE::MODEL_TYPE::MESA;
    case TYPE::NMES_HFET1:
    case TYPE::PMES_HFET1:          return NGSPICE::MODEL_TYPE::HFET1;
    case TYPE::PMES_HFET2:
    case TYPE::NMES_HFET2:          return NGSPICE::MODEL_TYPE::HFET2;

    case TYPE::NMOS_MOS1:
    case TYPE::PMOS_MOS1:           return NGSPICE::MODEL_TYPE::MOS1;
    case TYPE::NMOS_MOS2:
    case TYPE::PMOS_MOS2:           return NGSPICE::MODEL_TYPE::MOS2;
    case TYPE::NMOS_MOS3:
    case TYPE::PMOS_MOS3:           return NGSPICE::MODEL_TYPE::MOS3;
    case TYPE::NMOS_BSIM1:
    case TYPE::PMOS_BSIM1:          return NGSPICE::MODEL_TYPE::BSIM1;
    case TYPE::NMOS_BSIM2:
    case TYPE::PMOS_BSIM2:          return NGSPICE::MODEL_TYPE::BSIM2;
    case TYPE::NMOS_MOS6:
    case TYPE::PMOS_MOS6:           return NGSPICE::MODEL_TYPE::MOS6;
    case TYPE::NMOS_BSIM3:
    case TYPE::PMOS_BSIM3:          return NGSPICE::MODEL_TYPE::BSIM3;
    case TYPE::NMOS_MOS9:
    case TYPE::PMOS_MOS9:           return NGSPICE::MODEL_TYPE::MOS9;
    case TYPE::NMOS_B4SOI:
    case TYPE::PMOS_B4SOI:          return NGSPICE::MODEL_TYPE::B4SOI;
    case TYPE::NMOS_BSIM4:
    case TYPE::PMOS_BSIM4:          return NGSPICE::MODEL_TYPE::BSIM4;
    case TYPE::NMOS_B3SOIFD:
    case TYPE::PMOS_B3SOIFD:        return NGSPICE::MODEL_TYPE::B3SOIFD;
    case TYPE::NMOS_B3SOIDD:
    case TYPE::PMOS_B3SOIDD:        return NGSPICE::MODEL_TYPE::B3SOIDD;
    case TYPE::NMOS_B3SOIPD:
    case TYPE::PMOS_B3SOIPD:        return NGSPICE::MODEL_TYPE::B3SOIPD;
    case TYPE::NMOS_HISIM2:
    case TYPE::PMOS_HISIM2:         return NGSPICE::MODEL_TYPE::HISIM2;
    case TYPE::NMOS_HISIM_HV1:
    case TYPE::PMOS_HISIM_HV1:      return NGSPICE::MODEL_TYPE::HISIMHV1;
    case TYPE::NMOS_HISIM_HV2:
    case TYPE::PMOS_HISIM_HV2:      return NGSPICE::MODEL_TYPE::HISIMHV2;

    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_NGSPICE" );
        return NGSPICE::MODEL_TYPE::NONE;
    }
}


bool SIM_MODEL_NGSPICE::getIsOtherVariant()
{
    switch( GetType() )
    {
    case TYPE::PNP_GUMMEL_POON:
    case TYPE::PNP_VBIC:
    case TYPE::PNP_HICUM_L2:
    case TYPE::PJF_SHICHMAN_HODGES:
    case TYPE::PJF_PARKER_SKELLERN:
    case TYPE::PMES_STATZ:
    case TYPE::PMES_YTTERDAL:
    case TYPE::PMES_HFET1:
    case TYPE::PMES_HFET2:
    case TYPE::PMOS_MOS1:
    case TYPE::PMOS_MOS2:
    case TYPE::PMOS_MOS3:
    case TYPE::PMOS_BSIM1:
    case TYPE::PMOS_BSIM2:
    case TYPE::PMOS_MOS6:
    case TYPE::PMOS_BSIM3:
    case TYPE::PMOS_MOS9:
    case TYPE::PMOS_B4SOI:
    case TYPE::PMOS_BSIM4:
    case TYPE::PMOS_B3SOIFD:
    case TYPE::PMOS_B3SOIDD:
    case TYPE::PMOS_B3SOIPD:
    case TYPE::PMOS_HISIM2:
    case TYPE::PMOS_HISIM_HV1:
    case TYPE::PMOS_HISIM_HV2:
        return true;

    default:
        return false;
    }
}
