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

#include <sim/sim_model_behavioral.h>


template SIM_MODEL_BEHAVIORAL::SIM_MODEL_BEHAVIORAL( TYPE aType, int symbolPinCount,
                                                     const std::vector<void>* aFields );
template SIM_MODEL_BEHAVIORAL::SIM_MODEL_BEHAVIORAL( TYPE aType, int symbolPinCount,
                                                     const std::vector<SCH_FIELD>* aFields );
template SIM_MODEL_BEHAVIORAL::SIM_MODEL_BEHAVIORAL( TYPE aType, int symbolPinCount,
                                                     const std::vector<LIB_FIELD>* aFields );

template <typename T>
SIM_MODEL_BEHAVIORAL::SIM_MODEL_BEHAVIORAL( TYPE aType, int symbolPinCount,
                                            const std::vector<T>* aFields )
    : SIM_MODEL( aType )
{
    static PARAM::INFO resistor  = makeParamInfo( "r", "Expression for resistance",  "ohm" );
    static PARAM::INFO capacitor = makeParamInfo( "c", "Expression for capacitance", "F"   );
    static PARAM::INFO inductor  = makeParamInfo( "l", "Expression for inductance",  "H"   );
    static PARAM::INFO vsource   = makeParamInfo( "v", "Expression for voltage",     "V"   );
    static PARAM::INFO isource   = makeParamInfo( "i", "Expression for current",     "A"   );

    switch( aType )
    {
    case TYPE::RESISTOR_BEHAVIORAL:  Params().emplace_back( resistor  ); break;
    case TYPE::CAPACITOR_BEHAVIORAL: Params().emplace_back( capacitor ); break;
    case TYPE::INDUCTOR_BEHAVIORAL:  Params().emplace_back( inductor  ); break;
    case TYPE::VSOURCE_BEHAVIORAL:   Params().emplace_back( vsource   ); break;
    case TYPE::ISOURCE_BEHAVIORAL:   Params().emplace_back( isource   ); break;
    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_IDEAL" );
    }

    ReadDataFields( symbolPinCount, aFields );
}


void SIM_MODEL_BEHAVIORAL::WriteCode( wxString& aCode )
{
    // TODO
}


SIM_MODEL::PARAM::INFO SIM_MODEL_BEHAVIORAL::makeParamInfo( wxString name, wxString description,
                                                            wxString unit )
{
    SIM_MODEL::PARAM::INFO paramInfo = {};

    paramInfo.name = name;
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = unit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.description = description;

    return paramInfo;
}
