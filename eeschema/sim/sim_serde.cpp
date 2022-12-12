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
#include <string_utils.h>


namespace SIM_SERDE_PARSER
{
    using namespace SIM_SERDE_GRAMMAR;

    template <typename Rule> struct fieldParamValuePairsSelector : std::false_type {};
    template <> struct fieldParamValuePairsSelector<param> : std::true_type {};
    template <> struct fieldParamValuePairsSelector<quotedStringContent> : std::true_type {};
    template <> struct fieldParamValuePairsSelector<unquotedString> : std::true_type {};


    template <typename Rule> struct pinSequenceSelector : std::false_type {};
    template <> struct pinSequenceSelector<pinAssignment> : std::true_type {};
    template <> struct pinSequenceSelector<pinSymbolPinNumber> : std::true_type {};
    template <> struct pinSequenceSelector<pinName> : std::true_type {};

    template <typename Rule> struct fieldInferValueSelector : std::false_type {};
    template <> struct fieldInferValueSelector<number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>> : std::true_type {};
}


std::string SIM_SERDE::GenerateDevice() const
{
    return m_model.GetDeviceInfo().fieldValue;
}


std::string SIM_SERDE::GenerateType() const
{
    return m_model.GetTypeInfo().fieldValue;
}


std::string SIM_SERDE::GenerateValue() const
{
    const SIM_MODEL::PARAM& param = m_model.GetUnderlyingParam( 0 );
    std::string result = param.value->ToString();

    if( result == "" )
        result = m_model.GetDeviceInfo().fieldValue;

    return result;
}


std::string SIM_SERDE::GenerateParams() const
{
    std::string result;
    bool        isFirst = true;

    for( int i = 0; i < m_model.GetParamCount(); ++i )
    {
        if( i == 0 && m_model.IsStoredInValue() )
            continue;

        const SIM_MODEL::PARAM& param = m_model.GetUnderlyingParam( i );

        if( param.value->ToString() == ""
            && !( i == 0 && m_model.HasPrimaryValue() && !m_model.IsStoredInValue() ) )
        {
            continue;
        }

        // If the parameter is an enum and the value is default, don't write anything.
        if( param.info.enumValues.size() >= 1 && param.value->ToString() == param.info.defaultValue )
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

    std::vector<std::reference_wrapper<const SIM_MODEL::PIN>> pins = m_model.GetPins();

    // m_model.GetPins() returns pins in the order they appear in the model, but the keys in the
    // key=value pairs we create here are symbol pin numbers, so we sort the pins so that they are
    // ordered by the latter instead.
    std::sort( pins.begin(), pins.end(),
               []( const SIM_MODEL::PIN& lhs, const SIM_MODEL::PIN& rhs )
               {
                   return StrNumCmp( lhs.symbolPinNumber, rhs.symbolPinNumber, true ) < 0;
               } );

    bool isFirst = true;

    for( const SIM_MODEL::PIN& pin : pins )
    {
        if( pin.symbolPinNumber != "" )
        {
            if( !isFirst )
                result.append( " " );
            else
                isFirst = false;

            result.append( fmt::format( "{}={}", pin.symbolPinNumber, pin.name ) );
        }
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
            && aDevice == SIM_MODEL::DeviceInfo( SIM_MODEL::TypeInfo( type ).deviceType ).fieldValue )
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
        tao::pegtl::string_input<> in( aValue, SIM_MODEL::VALUE_FIELD );
        auto root = tao::pegtl::parse_tree::parse<SIM_SERDE_PARSER::fieldInferValueGrammar,
                                                  SIM_SERDE_PARSER::fieldInferValueSelector,
                                                  tao::pegtl::nothing,
                                                  SIM_SERDE_PARSER::control>( in );

        for( const auto& node : root->children )
        {
            if( node->is_type<SIM_SERDE_PARSER::number<SIM_VALUE::TYPE_FLOAT,
                                                       SIM_VALUE::NOTATION::SI>>()
                && node->string() != "" )
            {
                m_model.SetParamValue( 0, node->string() );
            }
        }
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    m_model.SetIsStoredInValue( true );
}


bool SIM_SERDE::ParseParams( const std::string& aParams )
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
    bool isPrimaryValueSet = false;

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

            if( paramName == m_model.GetParam( 0 ).info.name )
                isPrimaryValueSet = true;
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

    return !m_model.HasPrimaryValue() || isPrimaryValueSet;
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

        for( const auto& node : root->children )
        {
            std::string symbolPinNumber = node->children.at( 0 )->string();
            std::string pinName = node->children.at( 1 )->string();

            m_model.SetPinSymbolPinNumber( pinName, symbolPinNumber );
        }
    }
    catch( const tao::pegtl::parse_error& e )
    {
        THROW_IO_ERROR( e.what() );
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


std::string SIM_SERDE::GenerateParamValuePair( const SIM_MODEL::PARAM& aParam ) const
{
    std::string name = aParam.info.name;

    // Because of collisions with instance parameters, we append some model parameters with "_".
    if( boost::ends_with( aParam.info.name, "_" ) )
        name = aParam.info.name.substr( 0, aParam.info.name.length() - 1 );

    std::string value = aParam.value->ToString();
    
    if( value == "" || value.find( " " ) != std::string::npos )
        value = fmt::format( "\"{}\"", value );

    return fmt::format( "{}={}", name, value );
}
