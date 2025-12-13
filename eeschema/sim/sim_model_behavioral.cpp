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

#include <sim/sim_model_behavioral.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <fmt/core.h>


std::string SPICE_GENERATOR_BEHAVIORAL::ModelLine( const SPICE_ITEM& aItem ) const
{
    return "";
}


std::string SPICE_GENERATOR_BEHAVIORAL::ItemLine( const SPICE_ITEM& aItem ) const
{
    switch( m_model.GetType() )
    {
    case SIM_MODEL::TYPE::R_BEHAVIORAL:
    case SIM_MODEL::TYPE::C_BEHAVIORAL:
    case SIM_MODEL::TYPE::L_BEHAVIORAL:
    {
        SPICE_ITEM item = aItem;
        item.modelName = SIM_VALUE::ToSpice( m_model.GetParam( 0 ).value );
        return SPICE_GENERATOR::ItemLine( item );
    }

    case SIM_MODEL::TYPE::V_BEHAVIORAL:
    {
        SPICE_ITEM item = aItem;
        item.modelName = fmt::format( "V={}", SIM_VALUE::ToSpice( m_model.GetParam( 0 ).value ) );
        return SPICE_GENERATOR::ItemLine( item );
    }

    case SIM_MODEL::TYPE::I_BEHAVIORAL:
    {
        SPICE_ITEM item = aItem;
        item.modelName = fmt::format( "I={}", SIM_VALUE::ToSpice( m_model.GetParam( 0 ).value ) );
        return SPICE_GENERATOR::ItemLine( item );
    }

    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_BEHAVIORAL" );
        return "";
    }
}


SIM_MODEL_BEHAVIORAL::SIM_MODEL_BEHAVIORAL( TYPE aType ) :
    SIM_MODEL( aType, std::make_unique<SPICE_GENERATOR_BEHAVIORAL>( *this ) )
{
    static PARAM::INFO resistor  = makeParams( "r", "Expression for resistance",  "Ω" );
    static PARAM::INFO capacitor = makeParams( "c", "Expression for capacitance", "F" );
    static PARAM::INFO inductor  = makeParams( "l", "Expression for inductance",  "H" );
    static PARAM::INFO vsource   = makeParams( "v", "Expression for voltage",     "V" );
    static PARAM::INFO isource   = makeParams( "i", "Expression for current",     "A" );

    switch( aType )
    {
    case TYPE::R_BEHAVIORAL: AddParam( resistor  ); break;
    case TYPE::C_BEHAVIORAL: AddParam( capacitor ); break;
    case TYPE::L_BEHAVIORAL: AddParam( inductor  ); break;
    case TYPE::V_BEHAVIORAL: AddParam( vsource   ); break;
    case TYPE::I_BEHAVIORAL: AddParam( isource   ); break;
    default: wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_IDEAL" );
    }
}


bool SIM_MODEL_BEHAVIORAL::parseValueField( const std::string& aValueField )
{
    std::string expr = aValueField;

    if( expr.find( "=" ) == std::string::npos )
        return false;

    boost::replace_first( expr, "=", "" );

    SetParamValue( 0, boost::trim_copy( expr ) );
    return true;
}


SIM_MODEL::PARAM::INFO SIM_MODEL_BEHAVIORAL::makeParams( const std::string& aName,
                                                         const std::string& aDescription,
                                                         const std::string& aUnit )
{
    PARAM::INFO paramInfo( aName, 0, PARAM::DIR_INOUT, SIM_VALUE::TYPE_STRING, aUnit,
                           PARAM::CATEGORY::PRINCIPAL );

    paramInfo.description = aDescription;

    return paramInfo;
}
