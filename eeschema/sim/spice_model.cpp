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

#include <iterator>
#include <sim/spice_model.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <locale_io.h>
#include <lib_symbol.h>

/*namespace SPICE_MODEL_PARSER
{
    using namespace tao::pegtl;

    struct directive : sor<TAO_PEGTL_ISTRING( ".model" ),
                           TAO_PEGTL_ISTRING( ".param" ),
                           TAO_PEGTL_ISTRING( ".subckt" )> {};*//*

    struct spaces : star<space> {};
    struct identifierNotFirstChar : sor<alnum, one<'!', '#', '$', '%', '[', ']', '_'>> {};
    struct identifier : seq<alpha, star<identifierNotFirstChar>> {};
    struct digits : plus<digit> {};

    struct sign : opt<one<'+', '-'>> {};
    struct significand : sor<seq<digits, one<'.'>, opt<digits>>, seq<one<'.'>, digits>> {};
    struct exponent : opt<one<'e', 'E'>, sign, digits> {};
    struct metricSuffix : sor<TAO_PEGTL_ISTRING( "T" ),
                              TAO_PEGTL_ISTRING( "G" ),
                              TAO_PEGTL_ISTRING( "Meg" ),
                              TAO_PEGTL_ISTRING( "K" ),
                              TAO_PEGTL_ISTRING( "mil" ),
                              TAO_PEGTL_ISTRING( "m" ),
                              TAO_PEGTL_ISTRING( "u" ),
                              TAO_PEGTL_ISTRING( "n" ),
                              TAO_PEGTL_ISTRING( "p" ),
                              TAO_PEGTL_ISTRING( "f" )> {};
    struct number : seq<sign, significand, exponent, metricSuffix> {};

    struct modelModelType : sor<TAO_PEGTL_ISTRING( "R" ),
                                TAO_PEGTL_ISTRING( "C" ),
                                TAO_PEGTL_ISTRING( "L" ),
                                TAO_PEGTL_ISTRING( "SW" ),
                                TAO_PEGTL_ISTRING( "CSW" ),
                                TAO_PEGTL_ISTRING( "URC" ),
                                TAO_PEGTL_ISTRING( "LTRA" ),
                                TAO_PEGTL_ISTRING( "D" ),
                                TAO_PEGTL_ISTRING( "NPN" ),
                                TAO_PEGTL_ISTRING( "PNP" ),
                                TAO_PEGTL_ISTRING( "NJF" ),
                                TAO_PEGTL_ISTRING( "PJF" ),
                                TAO_PEGTL_ISTRING( "NMOS" ),
                                TAO_PEGTL_ISTRING( "PMOS" ),
                                TAO_PEGTL_ISTRING( "NMF" ),
                                TAO_PEGTL_ISTRING( "PMF" ),
                                TAO_PEGTL_ISTRING( "VDMOS" )> {};
    struct paramValuePair : seq<alnum, spaces, one<'='>, spaces, number> {};
    struct paramValuePairs : opt<paramValuePair, star<spaces, paramValuePair>> {};
    struct modelModelSpec : seq<modelModelType,
                                spaces,
                                one<'('>,
                                spaces,

                                paramValuePairs,

                                spaces,
                                one<')'>,
                                spaces> {};
    struct modelModel : seq<TAO_PEGTL_ISTRING( ".model" ), identifier, modelModelSpec> {};

    struct model : modelModel {};
    //struct model : sor<modelModel, paramModel, subcircuitModel> {};
}*/

namespace SPICE_MODEL_PARSER
{
    using namespace tao::pegtl;

    struct spaces : star<space> {};
    struct digits : plus<digit> {};

    struct sign : opt<one<'+', '-'>> {};
    struct significand : sor<seq<digits, opt<one<'.'>, opt<digits>>>, seq<one<'.'>, digits>> {};
    struct exponent : opt<one<'e', 'E'>, sign, digits> {};
    struct metricSuffix : opt<sor<TAO_PEGTL_ISTRING( "T" ),
                                  TAO_PEGTL_ISTRING( "G" ),
                                  TAO_PEGTL_ISTRING( "Meg" ),
                                  TAO_PEGTL_ISTRING( "K" ),
                                  TAO_PEGTL_ISTRING( "mil" ),
                                  TAO_PEGTL_ISTRING( "m" ),
                                  TAO_PEGTL_ISTRING( "u" ),
                                  TAO_PEGTL_ISTRING( "n" ),
                                  TAO_PEGTL_ISTRING( "p" ),
                                  TAO_PEGTL_ISTRING( "f" )>> {};

    // TODO: Move the `number` grammar to the SPICE_VALUE class.
    struct number : seq<sign, significand, exponent, metricSuffix> {};

    struct param : seq<alnum> {};

    struct paramValuePair : seq<param, spaces, one<'='>, spaces, number> {};
    struct paramValuePairs : opt<paramValuePair, star<spaces, paramValuePair>> {};

    template <typename Rule> struct paramValuePairsSelector : std::false_type {};
    template <> struct paramValuePairsSelector<param> : std::true_type {};
    template <> struct paramValuePairsSelector<number> : std::true_type {};
}


template SPICE_MODEL::TYPE SPICE_MODEL::ReadTypeFromFields( const std::vector<SCH_FIELD>* aFields );
template SPICE_MODEL::TYPE SPICE_MODEL::ReadTypeFromFields( const std::vector<LIB_FIELD>* aFields );

template <typename T>
SPICE_MODEL::TYPE SPICE_MODEL::ReadTypeFromFields( const std::vector<T>* aFields )
{
    wxString typeFieldValue = getFieldValue( aFields, TYPE_FIELD );
    wxString deviceTypeFieldValue = getFieldValue( aFields, DEVICE_TYPE_FIELD );
    bool typeFound = false;

    for( TYPE type : TYPE_ITERATOR() )
    {
        if( typeFieldValue == TypeInfo( type ).fieldValue )
        {
            typeFound = true;

            if( deviceTypeFieldValue == DeviceTypeInfo( TypeInfo( type ).deviceType ).fieldValue )
                return type;
        }
    }

    if( !typeFound )
        throw KI_PARAM_ERROR( wxString::Format( _( "Invalid \"%s\" field value: \"%s\"" ),
                                                TYPE_FIELD, typeFieldValue ) );

    throw KI_PARAM_ERROR( wxString::Format( _( "Invalid \"%s\" field value: \"%s\"" ),
                                            DEVICE_TYPE_FIELD, deviceTypeFieldValue ) );
}


SPICE_MODEL::SPICE_MODEL( TYPE aType ) : m_type( aType )
{
}


template SPICE_MODEL::SPICE_MODEL( const std::vector<SCH_FIELD>* aFields );
template SPICE_MODEL::SPICE_MODEL( const std::vector<LIB_FIELD>* aFields );

template <typename T>
SPICE_MODEL::SPICE_MODEL( const std::vector<T>* aFields )
    : m_type( ReadTypeFromFields( aFields ) )
{
    SetFile( getFieldValue( aFields, "Model_File" ) );
    parseParamValuePairs( getFieldValue( aFields, "Model_Params" ) );
}


SPICE_MODEL::SPICE_MODEL( const wxString& aCode )
{
}


template void SPICE_MODEL::WriteFields( std::vector<SCH_FIELD>* aFields );
template void SPICE_MODEL::WriteFields( std::vector<LIB_FIELD>* aFields );

template <typename T>
void SPICE_MODEL::WriteFields( std::vector<T>* aFields )
{
    setFieldValue( aFields, DEVICE_TYPE_FIELD,
                   DeviceTypeInfo( TypeInfo( m_type ).deviceType ).fieldValue );
    setFieldValue( aFields, TYPE_FIELD, TypeInfo( m_type ).fieldValue );
    setFieldValue( aFields, FILE_FIELD, GetFile() );
    setFieldValue( aFields, PARAMS_FIELD, generateParamValuePairs() );
}


void SPICE_MODEL::WriteCode( wxString& aCode )
{
}


void SPICE_MODEL::parseParamValuePairs( const wxString& aParamValuePairs )
{
    LOCALE_IO toggle;
    
    tao::pegtl::string_input<> in( aParamValuePairs.ToStdString(), "from_input" );
    auto root = tao::pegtl::parse_tree::parse<SPICE_MODEL_PARSER::paramValuePairs,
                                              SPICE_MODEL_PARSER::paramValuePairsSelector>( in );

    wxString paramName = "";

    for( const auto& node : root->children )
    {
        if( node->is_type<SPICE_MODEL_PARSER::param>() )
            paramName = node->string();
        else if( node->is_type<SPICE_MODEL_PARSER::number>() )
        {
            wxASSERT( paramName != "" );

            try
            {
                SPICE_VALUE value( node->string() );

                /*if( !SetParamValue( paramName, value ) )
                {
                    m_params.clear();
                    throw KI_PARAM_ERROR( wxString::Format( _( "Unknown parameter \"%s\"" ),
                                                            paramName ) );
                }*/
            }
            catch( KI_PARAM_ERROR& e )
            {
                m_params.clear();
                throw KI_PARAM_ERROR( wxString::Format( _( "Invalid \"%s\" parameter value: \"%s\"" ),
                                                        paramName, e.What() ) );
            }
        }
        else
            wxFAIL;
    }
}


wxString SPICE_MODEL::generateParamValuePairs()
{
    wxString result = "";

    /*for( auto it = GetParams().cbegin(); it != GetParams().cend(); it++ )
    {
        result += it->first;
        result += "=";
        result += it->second.value.ToString();

        if( std::next( it ) != GetParams().cend() )
            result += " ";
    }*/

    return result;
}


template <typename T>
wxString SPICE_MODEL::getFieldValue( const std::vector<T>* aFields, const wxString& aFieldName )
{
    static_assert( std::is_same<T, SCH_FIELD>::value || std::is_same<T, LIB_FIELD>::value );

    auto fieldIt = std::find_if( aFields->begin(), aFields->end(),
                                 [&]( const T& f )
                                 {
                                     return f.GetName() == aFieldName;
                                 } );

    if( fieldIt != aFields->end() )
        return fieldIt->GetText();

    return wxEmptyString;
}


template <typename T>
void SPICE_MODEL::setFieldValue( std::vector<T>* aFields, const wxString& aFieldName,
                                 const wxString& aValue )
{
    static_assert( std::is_same<T, SCH_FIELD>::value || std::is_same<T, LIB_FIELD>::value );

    auto fieldIt = std::find_if( aFields->begin(), aFields->end(),
                                 [&]( const T& f )
                                 {
                                    return f.GetName() == aFieldName;
                                 } );

    if( fieldIt != aFields->end() )
    {
        fieldIt->SetText( aValue );
        return;
    }


    if constexpr( std::is_same<T, SCH_FIELD>::value )
    {
        wxASSERT( aFields->size() >= 1 );

        SCH_ITEM* parent = static_cast<SCH_ITEM*>( aFields->at( 0 ).GetParent() );
        aFields->emplace_back( wxPoint(), aFields->size(), parent, aFieldName );
    }
    else if constexpr( std::is_same<T, LIB_FIELD>::value )
        aFields->emplace_back( aFields->size(), aFieldName );

    aFields->back().SetText( aValue );
}
