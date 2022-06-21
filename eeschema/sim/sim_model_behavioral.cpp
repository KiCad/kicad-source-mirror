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
    : SIM_MODEL( aType ),
      m_isInferred( false )
{
    static PARAM::INFO resistor  = makeParams( "r", "Expression for resistance",  "Ω" );
    static PARAM::INFO capacitor = makeParams( "c", "Expression for capacitance", "F"   );
    static PARAM::INFO inductor  = makeParams( "l", "Expression for inductance",  "H"   );
    static PARAM::INFO vsource   = makeParams( "v", "Expression for voltage",     "V"   );
    static PARAM::INFO isource   = makeParams( "i", "Expression for current",     "A"   );

    switch( aType )
    {
    case TYPE::R_BEHAVIORAL: AddParam( resistor  ); break;
    case TYPE::C_BEHAVIORAL: AddParam( capacitor ); break;
    case TYPE::L_BEHAVIORAL: AddParam( inductor  ); break;
    case TYPE::V_BEHAVIORAL: AddParam( vsource   ); break;
    case TYPE::I_BEHAVIORAL: AddParam( isource   ); break;
    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_IDEAL" );
    }
}


void SIM_MODEL_BEHAVIORAL::ReadDataSchFields( unsigned aSymbolPinCount,
                                              const std::vector<SCH_FIELD>* aFields )
{
    if( GetFieldValue( aFields, PARAMS_FIELD ) != "" )
        SIM_MODEL::ReadDataSchFields( aSymbolPinCount, aFields );
    else
        inferredReadDataFields( aSymbolPinCount, aFields );
}


void SIM_MODEL_BEHAVIORAL::ReadDataLibFields( unsigned aSymbolPinCount,
                                              const std::vector<LIB_FIELD>* aFields )
{
    if( GetFieldValue( aFields, PARAMS_FIELD ) != "" )
        SIM_MODEL::ReadDataLibFields( aSymbolPinCount, aFields );
    else
        inferredReadDataFields( aSymbolPinCount, aFields );
}


void SIM_MODEL_BEHAVIORAL::WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataSchFields( aFields );

    if( m_isInferred )
        inferredWriteDataFields( aFields );
}


void SIM_MODEL_BEHAVIORAL::WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataLibFields( aFields );

    if( m_isInferred )
        inferredWriteDataFields( aFields );
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
    case TYPE::R_BEHAVIORAL:
    case TYPE::C_BEHAVIORAL:
    case TYPE::L_BEHAVIORAL:
        return SIM_MODEL::GenerateSpiceItemLine( aRefName,
                                                 GetParam( 0 ).value->ToString(),
                                                 aPinNetNames );

    case TYPE::V_BEHAVIORAL:
        return SIM_MODEL::GenerateSpiceItemLine( aRefName,
                wxString::Format( "V=%s", GetParam( 0 ).value->ToString() ), aPinNetNames );

    case TYPE::I_BEHAVIORAL:
        return SIM_MODEL::GenerateSpiceItemLine( aRefName,
                wxString::Format( "I=%s", GetParam( 0 ).value->ToString() ), aPinNetNames );

    default:
        wxFAIL_MSG( "Unhandled SIM_MODEL type in SIM_MODEL_BEHAVIORAL" );
        return "";
    }
}


bool SIM_MODEL_BEHAVIORAL::parseValueField( const wxString& aValueField )
{
    wxString expr = aValueField;
    
    if( !expr.Replace( "=", "", false ) )
        return false;

    SetParamValue( 0, expr.Trim( false ).Trim( true ) );
    return true;
}


template <typename T>
void SIM_MODEL_BEHAVIORAL::inferredReadDataFields( unsigned aSymbolPinCount,
                                                   const std::vector<T>* aFields )
{
    ParsePinsField( aSymbolPinCount, GetFieldValue( aFields, PINS_FIELD ) );

    if( ( InferTypeFromRefAndValue( GetFieldValue( aFields, REFERENCE_FIELD ),
                                    GetFieldValue( aFields, VALUE_FIELD ) ) == GetType()
            && parseValueField( GetFieldValue( aFields, VALUE_FIELD ) ) )
        // If Value is device type, this is an empty model
        || GetFieldValue( aFields, VALUE_FIELD ) == DeviceTypeInfo( GetDeviceType() ).fieldValue )
    {
        m_isInferred = true;
    }
}


template <typename T>
void SIM_MODEL_BEHAVIORAL::inferredWriteDataFields( std::vector<T>& aFields ) const
{
    wxString value = GetParam( 0 ).value->ToString();

    if( value == "" )
        value = GetDeviceTypeInfo().fieldValue;

    WriteInferredDataFields( aFields, "=" + value );
}


SIM_MODEL::PARAM::INFO SIM_MODEL_BEHAVIORAL::makeParams( wxString aName, wxString aDescription,
                                                         wxString aUnit )
{
    PARAM::INFO paramInfo = {};

    paramInfo.name = aName;
    paramInfo.type = SIM_VALUE::TYPE_STRING;
    paramInfo.unit = aUnit;
    paramInfo.category = PARAM::CATEGORY::PRINCIPAL;
    paramInfo.description = aDescription;

    return paramInfo;
}
