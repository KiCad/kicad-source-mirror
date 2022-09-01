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

#include <sim/sim_model_spice.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>


namespace SIM_MODEL_SPICE_PARSER
{
    using namespace SIM_MODEL_GRAMMAR;

    template <typename Rule> struct legacyPinSequenceSelector : std::false_type {};
    template <> struct legacyPinSequenceSelector<legacyPinNumber> : std::true_type {};
}


SIM_MODEL_SPICE::SIM_MODEL_SPICE( TYPE aType )
    : SIM_MODEL( aType )
{
    static std::vector<PARAM::INFO> paramInfos = makeParamInfos();

    for( const PARAM::INFO& paramInfo : paramInfos )
        AddParam( paramInfo );
}


void SIM_MODEL_SPICE::ReadDataSchFields( unsigned aSymbolPinCount,
                                         const std::vector<SCH_FIELD>* aFields )
{
    SIM_MODEL::ReadDataSchFields( aSymbolPinCount, aFields );
    readLegacyDataFields( aSymbolPinCount, aFields );
}


void SIM_MODEL_SPICE::ReadDataLibFields( unsigned aSymbolPinCount,
                                         const std::vector<LIB_FIELD>* aFields )
{
    SIM_MODEL::ReadDataLibFields( aSymbolPinCount, aFields );
    readLegacyDataFields( aSymbolPinCount, aFields );
}


void SIM_MODEL_SPICE::WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataSchFields( aFields );
    
    // Erase the legacy fields.
    SetFieldValue( aFields, LEGACY_TYPE_FIELD, "" );
    SetFieldValue( aFields, LEGACY_PINS_FIELD, "" );
    SetFieldValue( aFields, LEGACY_MODEL_FIELD, "" );
    SetFieldValue( aFields, LEGACY_ENABLED_FIELD, "" );
    SetFieldValue( aFields, LEGACY_LIB_FIELD, "" );
}


void SIM_MODEL_SPICE::WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const
{
    SIM_MODEL::WriteDataLibFields( aFields );
}


wxString SIM_MODEL_SPICE::GenerateSpiceModelLine( const wxString& aModelName ) const
{
    return "";
}


wxString SIM_MODEL_SPICE::GenerateSpiceItemName( const wxString& aRefName ) const
{
    wxString elementType = GetParam( static_cast<int>( SPICE_PARAM::TYPE ) ).value->ToString();

    if( aRefName != "" && aRefName.StartsWith( elementType ) )
        return aRefName;
    else
        return elementType + aRefName;
}


void SIM_MODEL_SPICE::CreatePins( unsigned aSymbolPinCount )
{
    for( unsigned symbolPinIndex = 0; symbolPinIndex < aSymbolPinCount; ++symbolPinIndex )
        AddPin( { "", wxString::FromCDouble( symbolPinIndex + 1 ) } );
}


wxString SIM_MODEL_SPICE::GenerateSpiceItemPins( const wxString& aRefName,
                                                 const wxString& aModelName,
                                                 const std::vector<wxString>& aSymbolPinNumbers,
                                                 const std::vector<wxString>& aPinNetNames ) const
{
    wxString result;

    for( const PIN& pin : GetSpicePins() )
    {
        auto it = std::find( aSymbolPinNumbers.begin(), aSymbolPinNumbers.end(),
                             pin.symbolPinNumber );

        if( it != aSymbolPinNumbers.end() )
        {
            long symbolPinIndex = std::distance( aSymbolPinNumbers.begin(), it );
            result << aPinNetNames.at( symbolPinIndex ) << " ";
        }
    }

    return result;
}


wxString SIM_MODEL_SPICE::GenerateSpiceItemParamValuePair( const PARAM& aParam,
                                                           bool& aIsFirst ) const
{
    if( aParam.info.name != "model" )
        return "";

    return aParam.value->ToString();
}


bool SIM_MODEL_SPICE::SetParamFromSpiceCode( const wxString& aParamName,
                                             const wxString& aParamValue,
                                             SIM_VALUE_GRAMMAR::NOTATION aNotation )
{
    int paramIndex = 0;

    for(; paramIndex < GetParamCount(); ++paramIndex )
    {
        if( GetParam( paramIndex ).info.name == aParamName.Lower() )
            break;
    }

    if( paramIndex == GetParamCount() )
    {
        // No parameter with this name found. Create a new one.
        std::unique_ptr<PARAM::INFO> paramInfo = std::make_unique<PARAM::INFO>();

        paramInfo->name = aParamName.Lower();
        paramInfo->type = SIM_VALUE::TYPE_STRING;
        m_paramInfos.push_back( std::move( paramInfo ) );

        AddParam( *m_paramInfos.back() );
    }

    return GetParam( paramIndex ).value->FromString( wxString( aParamValue ), aNotation );
}


std::vector<SIM_MODEL::PARAM::INFO> SIM_MODEL_SPICE::makeParamInfos()
{
    std::vector<PARAM::INFO> paramInfos;

    for( SPICE_PARAM spiceParam : SPICE_PARAM_ITERATOR() )
    {
        PARAM::INFO paramInfo;

        switch( spiceParam )
        {
        case SPICE_PARAM::TYPE:
            paramInfo.name = "type";
            paramInfo.type = SIM_VALUE::TYPE_STRING;
            paramInfo.unit = "";
            paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
            paramInfo.defaultValue = "";
            paramInfo.description = "Spice element type";
            paramInfo.isSpiceInstanceParam = true;

            paramInfos.push_back( paramInfo );
            break;

        case SPICE_PARAM::MODEL:
            paramInfo.name = "model";
            paramInfo.type = SIM_VALUE::TYPE_STRING;
            paramInfo.unit = "";
            paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
            paramInfo.defaultValue = "";
            paramInfo.description = "Model name or value";
            paramInfo.isSpiceInstanceParam = true;

            paramInfos.push_back( paramInfo );
            break;

        case SPICE_PARAM::LIB:
            paramInfo.name = "lib";
            paramInfo.type = SIM_VALUE::TYPE_STRING;
            paramInfo.unit = "";
            paramInfo.category = SIM_MODEL::PARAM::CATEGORY::PRINCIPAL;
            paramInfo.defaultValue = "";
            paramInfo.description = "Library path to include";
            paramInfo.isSpiceInstanceParam = true;

            paramInfos.push_back( paramInfo );
            break;

        case SPICE_PARAM::_ENUM_END:
            break;
        }
    }

    return paramInfos;
}


template <typename T>
void SIM_MODEL_SPICE::readLegacyDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields )
{
    // Fill in the blanks with the legacy parameters.

    if( GetParam( static_cast<int>( SPICE_PARAM::TYPE ) ).value->ToString() == "" )
    {
        SetParamValue( static_cast<int>( SPICE_PARAM::TYPE ),
                       GetFieldValue( aFields, LEGACY_TYPE_FIELD ) );
    }

    if( GetFieldValue( aFields, PINS_FIELD ) == "" )
        parseLegacyPinsField( aSymbolPinCount, GetFieldValue( aFields, LEGACY_PINS_FIELD ) );

    if( GetParam( static_cast<int>( SPICE_PARAM::MODEL ) ).value->ToString() == "" )
    {
        SetParamValue( static_cast<int>( SPICE_PARAM::MODEL ),
                       GetFieldValue( aFields, LEGACY_MODEL_FIELD ) );
    }

    // If model param is still empty, then use Value field.
    if( GetParam( static_cast<int>( SPICE_PARAM::MODEL ) ).value->ToString() == "" )
    {
        SetParamValue( static_cast<int>( SPICE_PARAM::MODEL ),
                       GetFieldValue( aFields, SIM_MODEL::VALUE_FIELD ) );
    }

    if( GetParam( static_cast<int>( SPICE_PARAM::LIB ) ).value->ToString() == "" )
    {
        SetParamValue( static_cast<int>( SPICE_PARAM::LIB ),
                       GetFieldValue( aFields, LEGACY_LIB_FIELD ) );
    }
}


void SIM_MODEL_SPICE::parseLegacyPinsField( unsigned aSymbolPinCount,
                                            const wxString& aLegacyPinsField )
{
    if( aLegacyPinsField == "" )
        return;

    // Initially set all pins to Not Connected to match the legacy behavior.
    for( int modelPinIndex = 0; modelPinIndex < GetPinCount(); ++modelPinIndex )
        SetPinSymbolPinNumber( static_cast<int>( modelPinIndex ), "" );

    tao::pegtl::string_input<> in( aLegacyPinsField.ToUTF8(), PINS_FIELD );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_MODEL_SPICE_PARSER::legacyPinSequenceGrammar,
                                             SIM_MODEL_SPICE_PARSER::legacyPinSequenceSelector>
                ( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    for( int pinIndex = 0; pinIndex < static_cast<int>( root->children.size() ); ++pinIndex )
    {
        std::string symbolPinStr = root->children.at( pinIndex )->string();
        int symbolPinIndex = std::stoi( symbolPinStr ) - 1;

        if( symbolPinIndex < 0 || symbolPinIndex >= static_cast<int>( aSymbolPinCount ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Invalid symbol pin index: '%s'" ),
                                              symbolPinStr ) );
        }
                                              

        SetPinSymbolPinNumber( pinIndex, root->children.at( pinIndex )->string() );
    }
}
