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

#include <sim/sim_serde.h>
#include <fmt/core.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>


namespace SIM_SERDE_PARSER
{
    using namespace SIM_SERDE_GRAMMAR;

    template <typename Rule> struct fieldParamValuePairsSelector : std::false_type {};
    template <> struct fieldParamValuePairsSelector<param> : std::true_type {};
    template <> struct fieldParamValuePairsSelector<quotedStringContent> : std::true_type {};
    template <> struct fieldParamValuePairsSelector<unquotedString> : std::true_type {};


    template <typename Rule> struct pinSequenceSelector : std::false_type {};
    template <> struct pinSequenceSelector<pinNumber> : std::true_type {};

    template <typename Rule> struct fieldInferValueSelector : std::false_type {};
    template <> struct fieldInferValueSelector<fieldInferValueType> : std::true_type {};
    template <> struct fieldInferValueSelector<fieldInferValuePrimaryValue> : std::true_type {};
    template <> struct fieldInferValueSelector<number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>> : std::true_type {};
    template <> struct fieldInferValueSelector<fieldParamValuePairs> : std::true_type {};
}


std::string SIM_SERDE::GenerateDevice() const
{
    return m_model.GetDeviceTypeInfo().fieldValue;
}


std::string SIM_SERDE::GenerateType() const
{
    return m_model.GetTypeInfo().fieldValue;
}


std::string SIM_SERDE::GenerateValue() const
{
    std::string result;

    for( int i = 0; i < m_model.GetParamCount(); ++i )
    {
        const SIM_MODEL::PARAM& param = m_model.GetUnderlyingParam( i );

        if( i == 0 && m_model.HasPrimaryValue() )
        {
            result.append( param.value->ToString() );
            continue;
        }

        if( param.value->ToString() == "" )
            continue;

        std::string paramValuePair = GenerateParamValuePair( param );

        if( paramValuePair == "" )
            continue; // Prevent adding empty spaces.

        result.append( fmt::format( " {}", paramValuePair ) );
    }

    if( result == "" )
        result = m_model.GetDeviceTypeInfo().fieldValue;

    return result;
}


std::string SIM_SERDE::GenerateParams() const
{
    std::string result;
    bool        isFirst = true;

    for( int i = 0; i < m_model.GetParamCount(); ++i )
    {
        const SIM_MODEL::PARAM& param = m_model.GetUnderlyingParam( i );

        if( param.value->ToString() == "" )
            continue;

        std::string paramValuePair = GenerateParamValuePair( param );

        if( paramValuePair == "" )
            continue; // Prevent adding empty spaces.

        if( isFirst ) // Don't add a space at the beginning.
            isFirst = false;
        else
            result.append( " " );

        result.append( paramValuePair );
    }

    return result;
}


std::string SIM_SERDE::GeneratePins() const
{
    std::string result;

    for( int i = 0; i < m_model.GetPinCount(); ++i )
    {
        const SIM_MODEL::PIN& pin = m_model.GetPin( i );

        if( i != 0 )
            result.append( " " );

        if( pin.symbolPinNumber == "" )
            result.append( "~" );
        else
            result.append( pin.symbolPinNumber );
    }

    return result;
}


std::string SIM_SERDE::GenerateEnable() const
{
    return m_model.IsEnabled() ? "" : "0";
}


SIM_MODEL::TYPE SIM_SERDE::ParseDeviceAndType( const std::string& aDevice,
                                               const std::string& aType )
{
    for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
    {
        if( aType == SIM_MODEL::TypeInfo( type ).fieldValue
            && aDevice == SIM_MODEL::DeviceTypeInfo( SIM_MODEL::TypeInfo( type ).deviceType ).fieldValue )
        {
            return type;
        }
    }

    return SIM_MODEL::TYPE::NONE;
}


void SIM_SERDE::ParseValue( const std::string& aValue )
{
    try
    {
        // TODO: Don't call this multiple times.
        tao::pegtl::string_input<> in( aValue, SIM_MODEL::VALUE_FIELD );
        auto root = tao::pegtl::parse_tree::parse<SIM_SERDE_PARSER::fieldInferValueGrammar,
                                                  SIM_SERDE_PARSER::fieldInferValueSelector,
                                                  tao::pegtl::nothing,
                                                  SIM_SERDE_PARSER::control>( in );

        for( const auto& node : root->children )
        {
            if( node->is_type<SIM_SERDE_PARSER::fieldInferValuePrimaryValue>() )
            {
                if( m_model.HasPrimaryValue() )
                {
                    for( const auto& subnode : node->children )
                    {
                        if( subnode->is_type<SIM_SERDE_PARSER::number<SIM_VALUE::TYPE_FLOAT,
                                                                      SIM_VALUE::NOTATION::SI>>() )
                        {
                            m_model.SetParamValue( 0, subnode->string() );
                        }
                    }
                }
                else
                {
                    THROW_IO_ERROR(
                        wxString::Format( _( "Simulation model of type '%s' cannot have a primary value (which is '%s') in Value field" ),
                                          m_model.GetTypeInfo().fieldValue,
                                          node->string() ) );
                }
            }
            else if( node->is_type<SIM_SERDE_PARSER::fieldParamValuePairs>() )
                ParseParams( node->string() );
        }
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }
}


void SIM_SERDE::ParseParams( const std::string& aParams )
{
    tao::pegtl::string_input<> in( aParams, SIM_MODEL::PARAMS_FIELD );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        // Using parse tree instead of actions because we don't care about performance that much,
        // and having a tree greatly simplifies things.
        root = tao::pegtl::parse_tree::parse<
            SIM_SERDE_PARSER::fieldParamValuePairsGrammar,
            SIM_SERDE_PARSER::fieldParamValuePairsSelector,
            tao::pegtl::nothing,
            SIM_SERDE_PARSER::control>
                ( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    std::string paramName;

    for( const auto& node : root->children )
    {
        if( node->is_type<SIM_SERDE_PARSER::param>() )
            paramName = node->string();
        // TODO: Do something with number<SIM_VALUE::TYPE_INT, ...>.
        // It doesn't seem too useful?
        else if( node->is_type<SIM_SERDE_PARSER::quotedStringContent>()
            || node->is_type<SIM_SERDE_PARSER::unquotedString>() )
        {
            wxASSERT( paramName != "" );
            // TODO: Shouldn't be named "...fromSpiceCode" here...

            m_model.SetParamValue( paramName, node->string(), SIM_VALUE_GRAMMAR::NOTATION::SI );
        }
        else if( node->is_type<SIM_SERDE_PARSER::quotedString>() )
        {
            std::string str = node->string();

            // Unescape quotes.
            boost::replace_all( str, "\\\"", "\"" );

            m_model.SetParamValue( paramName, str, SIM_VALUE_GRAMMAR::NOTATION::SI );
        }
        else
        {
            wxFAIL;
        }
    }
}


void SIM_SERDE::ParsePins( const std::string& aPins )
{
    if( aPins == "" )
        return;

    tao::pegtl::string_input<> in( aPins, PINS_FIELD );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;

    try
    {
        root = tao::pegtl::parse_tree::parse<SIM_SERDE_PARSER::pinSequenceGrammar,
                                             SIM_SERDE_PARSER::pinSequenceSelector,
                                             tao::pegtl::nothing,
                                             SIM_SERDE_PARSER::control>( in );
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    if( static_cast<int>( root->children.size() ) != m_model.GetPinCount() )
    {
        THROW_IO_ERROR( wxString::Format( _( "%s describes %lu pins, expected %u" ),
                                          PINS_FIELD,
                                          root->children.size(),
                                          m_model.GetPinCount() ) );
    }

    for( int pinIndex = 0; pinIndex < static_cast<int>( root->children.size() ); ++pinIndex )
    {
        if( root->children.at( pinIndex )->string() == "~" )
            m_model.SetPinSymbolPinNumber( pinIndex, "" );
        else
            m_model.SetPinSymbolPinNumber( pinIndex, root->children.at( pinIndex )->string() );
    }
}


void SIM_SERDE::ParseEnable( const std::string& aEnable )
{
    if( aEnable == "" )
        return;

    char c = boost::to_lower_copy( aEnable )[0];

    if( c == 'n' || c == 'f' || c == '0' )
        m_model.SetIsEnabled( false );
}


SIM_MODEL::TYPE SIM_SERDE::InferTypeFromRefAndValue( const std::string& aRef,
                                                     const std::string& aValue,
                                                     int aSymbolPinCount )
{
    std::string typeString;

    try
    {
        tao::pegtl::string_input<> in( aValue, VALUE_FIELD );
        auto root = tao::pegtl::parse_tree::parse<SIM_SERDE_PARSER::fieldInferValueGrammar,
                                                  SIM_SERDE_PARSER::fieldInferValueSelector,
                                                  tao::pegtl::nothing,
                                                  SIM_SERDE_PARSER::control>( in );

        for( const auto& node : root->children )
        {
            if( node->is_type<SIM_SERDE_PARSER::fieldInferValueType>() )
                typeString = node->string();
        }
    }
    catch( const tao::pegtl::parse_error& )
    {
    }

    SIM_MODEL::DEVICE_TYPE_ deviceType = SIM_MODEL::InferDeviceTypeFromRef( aRef );

    // Exception. Potentiometer model is determined from pin count.
    if( deviceType == SIM_MODEL::DEVICE_TYPE_::R && aSymbolPinCount == 3 )
        return SIM_MODEL::TYPE::R_POT;

    for( SIM_MODEL::TYPE type : SIM_MODEL::TYPE_ITERATOR() )
    {
        if( SIM_MODEL::TypeInfo( type ).deviceType == deviceType
            && SIM_MODEL::TypeInfo( type ).fieldValue == typeString )
        {
            return type;
        }
    }

    return SIM_MODEL::TYPE::NONE;
}


std::string SIM_SERDE::GenerateParamValuePair( const SIM_MODEL::PARAM& aParam ) const
{
    std::string name = aParam.info.name;

    // Because of collisions with instance parameters, we append some model parameters with "_".
    if( boost::ends_with( aParam.info.name, "_" ) )
        name = aParam.info.name.substr( 0, aParam.info.name.length() - 1 );

    std::string value = aParam.value->ToString();
    
    if( value.find( " " ) != std::string::npos )
        value = fmt::format( "\"{}\"", value );

    return fmt::format( "{}={}", aParam.info.name, value );
}
