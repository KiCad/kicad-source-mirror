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

#include <fast_float/fast_float.h>
#include <sim/sim_value.h>
#include <wx/translation.h>
#include <ki_exception.h>
#include <locale_io.h>
#include <pegtl/contrib/parse_tree.hpp>
#include <fmt/core.h>
#include <math/util.h>
#include <wx/regex.h>


#define CALL_INSTANCE( ValueType, Notation, func, ... )                  \
    switch( ValueType )                                                  \
    {                                                                    \
    case SIM_VALUE::TYPE_INT:                                            \
        switch( Notation )                                               \
        {                                                                \
        case NOTATION::SI:                                               \
            func<SIM_VALUE::TYPE_INT, NOTATION::SI>( __VA_ARGS__ );      \
            break;                                                       \
                                                                         \
        case NOTATION::SPICE:                                            \
            func<SIM_VALUE::TYPE_INT, NOTATION::SPICE>( __VA_ARGS__ );   \
            break;                                                       \
        }                                                                \
        break;                                                           \
                                                                         \
    case SIM_VALUE::TYPE_FLOAT:                                          \
        switch( Notation )                                               \
        {                                                                \
        case NOTATION::SI:                                               \
            func<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>( __VA_ARGS__ );    \
            break;                                                       \
                                                                         \
        case NOTATION::SPICE:                                            \
            func<SIM_VALUE::TYPE_FLOAT, NOTATION::SPICE>( __VA_ARGS__ ); \
            break;                                                       \
        }                                                                \
        break;                                                           \
                                                                         \
    case SIM_VALUE::TYPE_BOOL:                                           \
    case SIM_VALUE::TYPE_COMPLEX:                                        \
    case SIM_VALUE::TYPE_STRING:                                         \
    case SIM_VALUE::TYPE_BOOL_VECTOR:                                    \
    case SIM_VALUE::TYPE_INT_VECTOR:                                     \
    case SIM_VALUE::TYPE_FLOAT_VECTOR:                                   \
    case SIM_VALUE::TYPE_COMPLEX_VECTOR:                                 \
        wxFAIL_MSG( "Unhandled SIM_VALUE type" );                        \
        break;                                                           \
    }


namespace SIM_VALUE_PARSER
{
    using namespace SIM_VALUE_GRAMMAR;

    template <typename Rule>
    struct numberSelector : std::false_type {};

    // TODO: Reorder. NOTATION should be before TYPE.

    template <> struct numberSelector<SIM_VALUE_GRAMMAR::significand<SIM_VALUE::TYPE_INT>>
        : std::true_type {};
    template <> struct numberSelector<SIM_VALUE_GRAMMAR::significand<SIM_VALUE::TYPE_FLOAT>>
        : std::true_type {};
    template <> struct numberSelector<intPart> : std::true_type {};
    template <> struct numberSelector<fracPart> : std::true_type {};
    template <> struct numberSelector<exponent> : std::true_type {};
    template <> struct numberSelector<unitPrefix<SIM_VALUE::TYPE_INT, NOTATION::SI>>
        : std::true_type {};
    template <> struct numberSelector<unitPrefix<SIM_VALUE::TYPE_INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct numberSelector<unitPrefix<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>>
        : std::true_type {};
    template <> struct numberSelector<unitPrefix<SIM_VALUE::TYPE_FLOAT, NOTATION::SPICE>>
        : std::true_type {};

    struct PARSE_RESULT
    {
        bool                   isOk = true;
        bool                   isEmpty = true;
        std::string            significand;
        std::optional<int64_t> intPart;
        std::optional<int64_t> fracPart;
        std::optional<int>     exponent;
        std::optional<int>     unitPrefixExponent;
    };

    PARSE_RESULT Parse( const std::string& aString,
                        NOTATION aNotation = NOTATION::SI,
                        SIM_VALUE::TYPE aValueType = SIM_VALUE::TYPE_FLOAT );

    int UnitPrefixToExponent( std::string aPrefix, NOTATION aNotation = NOTATION::SI );
    std::string ExponentToUnitPrefix( double aExponent, int& aExponentReduction,
                                      NOTATION aNotation = NOTATION::SI );
    }


template <SIM_VALUE::TYPE ValueType, SIM_VALUE_PARSER::NOTATION Notation>
static inline void doIsValid( tao::pegtl::string_input<>& aIn )
{
    tao::pegtl::parse<SIM_VALUE_PARSER::numberGrammar<ValueType, Notation>>( aIn );
}


bool SIM_VALUE_GRAMMAR::IsValid( const std::string& aString, SIM_VALUE::TYPE aValueType,
                                 NOTATION aNotation )
{
    tao::pegtl::string_input<> in( aString, "from_content" );

    try
    {
        CALL_INSTANCE( aValueType, aNotation, doIsValid, in );
    }
    catch( const tao::pegtl::parse_error& )
    {
        return false;
    }

    return true;
}


template <SIM_VALUE::TYPE ValueType, SIM_VALUE_PARSER::NOTATION Notation>
static inline std::unique_ptr<tao::pegtl::parse_tree::node> doParse(
        tao::pegtl::string_input<>& aIn )
{
    return tao::pegtl::parse_tree::parse<SIM_VALUE_PARSER::numberGrammar<ValueType, Notation>,
                                         SIM_VALUE_PARSER::numberSelector>
        ( aIn );
}


template <SIM_VALUE::TYPE ValueType, SIM_VALUE_PARSER::NOTATION Notation>
static inline void handleNodeForParse( tao::pegtl::parse_tree::node& aNode,
                                       SIM_VALUE_PARSER::PARSE_RESULT& aParseResult )
{
    if( aNode.is_type<SIM_VALUE_PARSER::significand<ValueType>>() )
    {
        aParseResult.significand = aNode.string();
        aParseResult.isEmpty = false;

        for( const auto& subnode : aNode.children )
        {
            try
            {
                if( subnode->is_type<SIM_VALUE_PARSER::intPart>() )
                    aParseResult.intPart = std::stoll( subnode->string() );
                else if( subnode->is_type<SIM_VALUE_PARSER::fracPart>() )
                    aParseResult.fracPart = std::stoll( subnode->string() );
            }
            catch( const std::exception& )
            {
                aParseResult.isOk = false;
            }
        }
    }
    else if( aNode.is_type<SIM_VALUE_PARSER::exponent>() )
    {
        aParseResult.exponent = std::stoi( aNode.string() );
        aParseResult.isEmpty = false;
    }
    else if( aNode.is_type<SIM_VALUE_PARSER::unitPrefix<ValueType, Notation>>() )
    {
        aParseResult.unitPrefixExponent = SIM_VALUE_PARSER::UnitPrefixToExponent( aNode.string(),
                                                                                  Notation );
        aParseResult.isEmpty = false;
    }
    else
        wxFAIL_MSG( "Unhandled parse tree node" );
}


SIM_VALUE_PARSER::PARSE_RESULT SIM_VALUE_PARSER::Parse( const std::string& aString,
                                                        NOTATION aNotation,
                                                        SIM_VALUE::TYPE aValueType )
{
    LOCALE_IO toggle;

    tao::pegtl::string_input<> in( aString, "from_content" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;
    PARSE_RESULT result;

    try
    {
        CALL_INSTANCE( aValueType, aNotation, root = doParse, in );
    }
    catch( tao::pegtl::parse_error& )
    {
        result.isOk = false;
        return result;
    }

    wxASSERT( root );

    try
    {
        for( const auto& node : root->children )
        {
            CALL_INSTANCE( aValueType, aNotation, handleNodeForParse, *node, result );
        }
    }
    catch( const std::invalid_argument& e )
    {
        wxFAIL_MSG( fmt::format( "Parsing simulator value failed: {:s}", e.what() ) );
        result.isOk = false;
    }

    return result;
}


int SIM_VALUE_PARSER::UnitPrefixToExponent( std::string aPrefix, NOTATION aNotation )
{
    switch( aNotation )
    {
    case NOTATION::SI:
        if( aPrefix.empty() )
            return 0;

        switch( aPrefix[0] )
        {
        case 'a': return -18;
        case 'f': return -15;
        case 'p': return -12;
        case 'n': return -9;
        case 'u': return -6;
        case 'm': return -3;
        case 'k':
        case 'K': return 3;
        case 'M': return 6;
        case 'G': return 9;
        case 'T': return 12;
        case 'P': return 15;
        case 'E': return 18;
        }

        break;

    case NOTATION::SPICE:
        std::transform( aPrefix.begin(), aPrefix.end(), aPrefix.begin(),
                        ::tolower );

        if( aPrefix == "f" )
            return -15;
        else if( aPrefix == "p" )
            return -12;
        else if( aPrefix == "n" )
            return -9;
        else if( aPrefix == "u" )
            return -6;
        else if( aPrefix == "m" )
            return -3;
        else if( aPrefix == "" )
            return 0;
        else if( aPrefix == "k" )
            return 3;
        else if( aPrefix == "meg" )
            return 6;
        else if( aPrefix == "g" )
            return 9;
        else if( aPrefix == "t" )
            return 12;

        break;
    }

    wxFAIL_MSG( fmt::format( "Unknown simulator value suffix: '{:s}'", aPrefix ) );
    return 0;
}


std::string SIM_VALUE_PARSER::ExponentToUnitPrefix( double aExponent, int& aExponentReduction,
                                                      NOTATION aNotation )
{
    if( aNotation == NOTATION::SI && aExponent >= -18 && aExponent <= -15 )
    {
        aExponentReduction = -18;
        return "a";
    }
    else if( aExponent >= -15 && aExponent < -12 )
    {
        aExponentReduction = -15;
        return "f";
    }
    else if( aExponent >= -12 && aExponent < -9 )
    {
        aExponentReduction = -12;
        return "p";
    }
    else if( aExponent >= -9 && aExponent < -6 )
    {
        aExponentReduction = -9;
        return "n";
    }
    else if( aExponent >= -6 && aExponent < -3 )
    {
        aExponentReduction = -6;
        return "u";
    }
    else if( aExponent >= -3 && aExponent < 0 )
    {
        aExponentReduction = -3;
        return "m";
    }
    else if( aExponent >= 0 && aExponent < 3 )
    {
        aExponentReduction = 0;
        return "";
    }
    else if( aExponent >= 3 && aExponent < 6 )
    {
        aExponentReduction = 3;
        return "k";
    }
    else if( aExponent >= 6 && aExponent < 9 )
    {
        aExponentReduction = 6;
        return ( aNotation == NOTATION::SI ) ? "M" : "Meg";
    }
    else if( aExponent >= 9 && aExponent < 12 )
    {
        aExponentReduction = 9;
        return "G";
    }
    else if( aExponent >= 12 && aExponent < 15 )
    {
        aExponentReduction = 12;
        return "T";
    }
    else if( aNotation == NOTATION::SI && aExponent >= 15 && aExponent < 18 )
    {
        aExponentReduction = 15;
        return "P";
    }
    else if( aNotation == NOTATION::SI && aExponent >= 18 && aExponent <= 21 )
    {
        aExponentReduction = 18;
        return "E";
    }

    aExponentReduction = 0;
    return "";
}


std::string SIM_VALUE::ConvertNotation( const std::string& aString, NOTATION aFromNotation,
                                        NOTATION aToNotation )
{
    wxString buf( aString );
    buf.Replace( ',', '.' );

    SIM_VALUE_PARSER::PARSE_RESULT parseResult = SIM_VALUE_PARSER::Parse( buf.ToStdString(),
                                                                          aFromNotation );

    if( parseResult.isOk && !parseResult.isEmpty && !parseResult.significand.empty() )
    {
        int exponent = parseResult.exponent ? *parseResult.exponent : 0;
        exponent += parseResult.unitPrefixExponent ? *parseResult.unitPrefixExponent : 0;

        int         expReduction = 0;
        std::string prefix = SIM_VALUE_PARSER::ExponentToUnitPrefix( exponent, expReduction,
                                                                        aToNotation );
        double significand{};

        fast_float::from_chars(
                parseResult.significand.data(),
                parseResult.significand.data() + parseResult.significand.size(),
                significand,
                fast_float::chars_format::skip_white_space | fast_float::chars_format::allow_leading_plus );

        exponent -= expReduction;
        return fmt::format( "{:g}{}", significand * std::pow( 10, exponent ),
                            prefix );
    }

    return aString;
}


std::string SIM_VALUE::Normalize( double aValue )
{
    double exponent = std::log10( std::abs( aValue ) );
    int    expReduction = 0;

    std::string prefix = SIM_VALUE_PARSER::ExponentToUnitPrefix( exponent, expReduction,
                                                                 NOTATION::SI );
    double reducedValue = aValue / std::pow( 10, expReduction );

    return fmt::format( "{:g}{}", reducedValue, prefix );
}


std::string SIM_VALUE::ToSpice( const std::string& aString )
{
    // Notation conversion is very slow.  Avoid if possible.

    auto plainNumber =
            []( const std::string& aStringVal )
            {
                for( char c : aStringVal )
                {
                    if( c != '.' && ( c < '0' || c > '9' )  )
                        return false;
                }

                return true;
            };

    if( plainNumber( aString ) )
        return aString;
    else
        return ConvertNotation( aString, NOTATION::SI, NOTATION::SPICE );
}


double SIM_VALUE::ToDouble( const std::string& aString, double aDefault )
{
    SIM_VALUE_PARSER::PARSE_RESULT parseResult = SIM_VALUE_PARSER::Parse( aString, NOTATION::SI );

    if( parseResult.isOk && !parseResult.isEmpty && !parseResult.significand.empty() )
    {
        int       exponent = parseResult.exponent ? *parseResult.exponent : 0;

        exponent += parseResult.unitPrefixExponent ? *parseResult.unitPrefixExponent : 0;
        double significand{};

        fast_float::from_chars( parseResult.significand.data(),
                                parseResult.significand.data() + parseResult.significand.size(),
                                significand,
                                fast_float::chars_format::skip_white_space | fast_float::chars_format::allow_leading_plus );

        return significand * std::pow( 10, exponent );
    }

    return aDefault;
}


int SIM_VALUE::ToInt( const std::string& aString, int aDefault )
{
    SIM_VALUE_PARSER::PARSE_RESULT parseResult = SIM_VALUE_PARSER::Parse( aString, NOTATION::SI );

    if( parseResult.isOk
        && !parseResult.isEmpty
        && parseResult.intPart
        && ( !parseResult.fracPart || *parseResult.fracPart == 0 ) )
    {
        int exponent = parseResult.exponent ? *parseResult.exponent : 0;
        exponent += parseResult.unitPrefixExponent ? *parseResult.unitPrefixExponent : 0;

        if( exponent >= 0 )
            return (int) *parseResult.intPart * (int) std::pow( 10, exponent );
    }

    return aDefault;
}


bool SIM_VALUE::Equal( double aLH, const std::string& aRH )
{
    return std::abs( aLH - ToDouble( aRH ) ) <= std::numeric_limits<double>::epsilon();
}
