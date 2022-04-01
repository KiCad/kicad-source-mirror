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
#include <locale_io.h>


SIM_MODEL_BEHAVIORAL::SIM_MODEL_BEHAVIORAL( TYPE aType )
    : SIM_MODEL( aType )
{
    static PARAM::INFO resistor  = makeParamInfo( "r", "Expression for resistance",  "ohm" );
    static PARAM::INFO capacitor = makeParamInfo( "c", "Expression for capacitance", "F"   );
    static PARAM::INFO inductor  = makeParamInfo( "l", "Expression for inductance",  "H"   );
    static PARAM::INFO vsource   = makeParamInfo( "v", "Expression for voltage",     "V"   );
    static PARAM::INFO isource   = makeParamInfo( "i", "Expression for current",     "A"   );

    switch( aType )
    {
    case TYPE::RESISTOR_BEHAVIORAL:  AddParam( resistor  ); break;
    case TYPE::CAPACITOR_BEHAVIORAL: AddParam( capacitor ); break;
    case TYPE::INDUCTOR_BEHAVIORAL:  AddParam( inductor  ); break;
    case TYPE::VSOURCE_BEHAVIORAL:   AddParam( vsource   ); break;
    case TYPE::ISOURCE_BEHAVIORAL:   AddParam( isource   ); break;
    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_IDEAL" );
    }
}


wxString SIM_MODEL_BEHAVIORAL::GenerateSpiceIncludeLine( const wxString& aLibraryFilename ) const
{
    return "";
}


wxString SIM_MODEL_BEHAVIORAL::GenerateSpiceModelLine( const wxString& aModelName ) const
{
    return "";
}


wxString SIM_MODEL_BEHAVIORAL::GenerateSpiceItemLine( const wxString& aRefName,
                                                      const wxString& aModelName,
                                                      const std::vector<wxString>& aPinNetNames ) const
{
    LOCALE_IO toggle;

    switch( GetType() )
    {
    case TYPE::RESISTOR_BEHAVIORAL:
    case TYPE::CAPACITOR_BEHAVIORAL:
    case TYPE::INDUCTOR_BEHAVIORAL:
        return SIM_MODEL::GenerateSpiceItemLine( aRefName,
                                                 GetParam( 0 ).value->ToString(),
                                                 aPinNetNames );

    case TYPE::VSOURCE_BEHAVIORAL:
        return SIM_MODEL::GenerateSpiceItemLine( aRefName,
                wxString::Format( "V=%s", GetParam( 0 ).value->ToString() ), aPinNetNames );

    case TYPE::ISOURCE_BEHAVIORAL:
        return SIM_MODEL::GenerateSpiceItemLine( aRefName,
                wxString::Format( "I=%s", GetParam( 0 ).value->ToString() ), aPinNetNames );

    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_BEHAVIORAL" );
        return "";
    }
}


std::vector<wxString> SIM_MODEL_BEHAVIORAL::getPinNames() const
{
    return { "+", "-" };
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
