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

#include <sim/sim_model_ideal.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <fmt/core.h>
#include <wx/regex.h>

std::string SPICE_GENERATOR_IDEAL::ModelLine( const SPICE_ITEM& aItem ) const
{
    return "";
}


std::string SPICE_GENERATOR_IDEAL::ItemLine( const SPICE_ITEM& aItem ) const
{
    SPICE_ITEM item = aItem;
    item.modelName = SIM_VALUE::ToSpice( m_model.GetParam( 0 ).value );

    if( item.modelName != "" )
        return SPICE_GENERATOR::ItemLine( item );
    else
        return "";
}


std::string SPICE_GENERATOR_IDEAL::TunerCommand( const SPICE_ITEM& aItem, double aValue ) const
{
    return fmt::format( "alter @{}={:g}",
                        aItem.model->SpiceGenerator().ItemName( aItem ),
                        aValue );
}


SIM_MODEL_IDEAL::SIM_MODEL_IDEAL( TYPE aType ) :
    SIM_MODEL( aType, std::make_unique<SPICE_GENERATOR_IDEAL>( *this ) )
{
    static PARAM::INFO resistor  = makeParamInfo( "r", "Resistance",  "Ω" );
    static PARAM::INFO capacitor = makeParamInfo( "c", "Capacitance", "F" );
    static PARAM::INFO inductor  = makeParamInfo( "l", "Inductance",  "H" );

    switch( aType )
    {
    case TYPE::R: AddParam( resistor  ); break;
    case TYPE::C: AddParam( capacitor ); break;
    case TYPE::L: AddParam( inductor  ); break;
    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_IDEAL" );
    }
}


SIM_MODEL::PARAM::INFO SIM_MODEL_IDEAL::makeParamInfo( const std::string& aName,
                                                       const std::string& aDescription,
                                                       const std::string& aUnit )
{
    PARAM::INFO paramInfo( aName, 0, PARAM::DIR_INOUT, SIM_VALUE::TYPE_FLOAT, aUnit,
                           PARAM::CATEGORY::PRINCIPAL );

    paramInfo.description = aDescription;

    return paramInfo;
}
