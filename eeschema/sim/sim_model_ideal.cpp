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
    item.modelName = m_model.GetParam( 0 ).value->ToString( SIM_VALUE::NOTATION::SPICE );

    if( item.modelName != "" )
        return SPICE_GENERATOR::ItemLine( item );
    else
        return "";
}


std::string SPICE_GENERATOR_IDEAL::TunerCommand( const SPICE_ITEM& aItem,
                                                 const SIM_VALUE_FLOAT& aValue ) const
{
    return fmt::format( "alter @{}={}",
                        aItem.model->SpiceGenerator().ItemName( aItem ),
                        aValue.ToSpiceString() );
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


wxString SIM_MODEL_IDEAL::InferSimParams( const wxString& aPrefix, const wxString& aValue )
{
    wxString spiceModel;

    if( aPrefix == wxT( "R" ) || aPrefix == wxT( "L" ) || aPrefix == wxT( "C" ) )
    {
        wxRegEx passiveVal(
            wxT( "^([0-9\\. ]+)([fFpPnNuUmMkKgGtTμµ𝛍𝜇𝝁 ]|M(e|E)(g|G))?([fFhHΩΩ𝛀𝛺𝝮]|ohm)?([-1-9 ]*)$" ) );

        if( passiveVal.Matches( aValue ) )
        {
            wxString valuePrefix( passiveVal.GetMatch( aValue, 1 ) );
            wxString valueUnits( passiveVal.GetMatch( aValue, 2 ) );
            wxString valueSuffix( passiveVal.GetMatch( aValue, 6 ) );

            if( valueUnits == "M" )
                valueUnits = "Meg";

            spiceModel = valuePrefix + valueUnits;
        }
        else
        {
            spiceModel = aValue;
        }
    }

    if( !spiceModel.IsEmpty() )
        return wxString::Format( wxT( "%s=\"%s\"" ), aPrefix.Lower(), spiceModel );
    else
        return wxEmptyString;
}


SIM_MODEL::PARAM::INFO SIM_MODEL_IDEAL::makeParamInfo( std::string aName, std::string aDescription,
                                                       std::string aUnit )
{
    SIM_MODEL::PARAM::INFO paramInfo = {};

    paramInfo.name = aName;
    paramInfo.type = SIM_VALUE::TYPE_FLOAT;
    paramInfo.unit = aUnit;
    paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
    paramInfo.description = aDescription;

    return paramInfo;
}
