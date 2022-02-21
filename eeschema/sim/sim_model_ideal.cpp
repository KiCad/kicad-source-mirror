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

#include <sim/sim_model_ideal.h>

using PARAM = SIM_MODEL::PARAM;


SIM_MODEL_IDEAL::SIM_MODEL_IDEAL( TYPE aType ) : SIM_MODEL( aType )
{
    static PARAM::INFO resistor  = makeParamInfo( "r", "Resistance",  "ohm" );
    static PARAM::INFO capacitor = makeParamInfo( "c", "Capacitance", "F"   );
    static PARAM::INFO inductor  = makeParamInfo( "l", "Inductance",  "H"   );

    switch( aType )
    {
    case TYPE::RESISTOR_IDEAL:  Params().emplace_back( resistor  ); break;
    case TYPE::CAPACITOR_IDEAL: Params().emplace_back( capacitor ); break;
    case TYPE::INDUCTOR_IDEAL:  Params().emplace_back( inductor  ); break;
    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_IDEAL" );
    }
}


void SIM_MODEL_IDEAL::WriteCode( wxString& aCode )
{
    // TODO
}


PARAM::INFO SIM_MODEL_IDEAL::makeParamInfo( wxString aName, wxString aDescription, wxString aUnit )
{
    SIM_MODEL::PARAM::INFO paramInfo = {};

    paramInfo.name = aName;
    paramInfo.type = SIM_VALUE_BASE::TYPE::FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.description = aDescription;

    return paramInfo;
}
