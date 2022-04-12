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

#include <sim/sim_value.h>
#include <wx/translation.h>
#include <ki_exception.h>
#include <locale_io.h>
#include <complex>
#include <pegtl/contrib/parse_tree.hpp>


#define CALL_INSTANCE( ValueType, Notation, func, ... )                        \
    switch( ValueType )                                                        \
    {                                                                          \
    case SIM_VALUE::TYPE::INT:                                            \
        switch( Notation )                                                     \
        {                                                                      \
        case NOTATION::SI:                                                     \
            func<SIM_VALUE::TYPE::INT, NOTATION::SI>( __VA_ARGS__ );      \
            break;                                                             \
                                                                               \
        case NOTATION::SPICE:                                                  \
            func<SIM_VALUE::TYPE::INT, NOTATION::SPICE>( __VA_ARGS__ );   \
            break;                                                             \
        }                                                                      \
        break;                                                                 \
                                                                               \
    case SIM_VALUE::TYPE::FLOAT:                                          \
        switch( Notation )                                                     \
        {                                                                      \
        case NOTATION::SI:                                                     \
            func<SIM_VALUE::TYPE::FLOAT, NOTATION::SI>( __VA_ARGS__ );    \
            break;                                                             \
                                                                               \
        case NOTATION::SPICE:                                                  \
            func<SIM_VALUE::TYPE::FLOAT, NOTATION::SPICE>( __VA_ARGS__ ); \
            break;                                                             \
        }                                                                      \
        break;                                                                 \
                                                                               \
    case SIM_VALUE::TYPE::BOOL:                                           \
    case SIM_VALUE::TYPE::COMPLEX:                                        \
    case SIM_VALUE::TYPE::STRING:                                         \
    case SIM_VALUE::TYPE::BOOL_VECTOR:                                    \
    case SIM_VALUE::TYPE::INT_VECTOR:                                     \
    case SIM_VALUE::TYPE::FLOAT_VECTOR:                                   \
    case SIM_VALUE::TYPE::COMPLEX_VECTOR:                                 \
        wxFAIL_MSG( "Unhandled SIM_VALUE type" );                         \
        break;                                                                 \
    }


namespace SIM_VALUE_PARSER
{
    using namespace SIM_VALUE_GRAMMAR;

    template <typename Rule>
    struct numberSelector : std::false_type {};

    // TODO: Reorder. NOTATION should be before TYPE.

    template <> struct numberSelector<SIM_VALUE_GRAMMAR::significand<SIM_VALUE::TYPE::INT>>
        : std::true_type {};
    template <> struct numberSelector<SIM_VALUE_GRAMMAR::significand<SIM_VALUE::TYPE::FLOAT>>
        : std::true_type {};
    template <> struct numberSelector<intPart> : std::true_type {};
    template <> struct numberSelector<fracPart> : std::true_type {};
    template <> struct numberSelector<exponent> : std::true_type {};
    template <> struct numberSelector<metricSuffix<SIM_VALUE::TYPE::INT, NOTATION::SI>>
        : std::true_type {};
    template <> struct numberSelector<metricSuffix<SIM_VALUE::TYPE::INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct numberSelector<metricSuffix<SIM_VALUE::TYPE::FLOAT, NOTATION::SI>>
        : std::true_type {};
    template <> struct numberSelector<metricSuffix<SIM_VALUE::TYPE::FLOAT, NOTATION::SPICE>>
        : std::true_type {};

    struct PARSE_RESULT
    {
        bool isOk = true;
        bool isEmpty = true;
        std::string significand;
        OPT<long> intPart;
        OPT<long> fracPart;
        OPT<long> exponent;
        OPT<long> metricSuffixExponent;
    };

    PARSE_RESULT Parse( const wxString& aString,
                        NOTATION aNotation = NOTATION::SI,
                        SIM_VALUE::TYPE aValueType = SIM_VALUE::TYPE::FLOAT );

    long MetricSuffixToExponent( std::string aMetricSuffix, NOTATION aNotation = NOTATION::SI );
    wxString ExponentToMetricSuffix( double aExponent, long& aReductionExponent,
                                     NOTATION aNotation = NOTATION::SI );
}


template <SIM_VALUE::TYPE ValueType, SIM_VALUE_PARSER::NOTATION Notation>
static inline void doIsValid( tao::pegtl::string_input<>& aIn )
{
    tao::pegtl::parse<SIM_VALUE_PARSER::numberGrammar<ValueType, Notation>>( aIn );
}


bool SIM_VALUE_GRAMMAR::IsValid( const wxString& aString,
                                SIM_VALUE::TYPE aValueType,
                                NOTATION aNotation )
{
    tao::pegtl::string_input<> in( aString.ToStdString(), "from_content" );

    try
    {
        CALL_INSTANCE( aValueType, aNotation, doIsValid, in );
    }
    catch( const tao::pegtl::parse_error& e )
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
            if( subnode->is_type<SIM_VALUE_PARSER::intPart>() )
                aParseResult.intPart = std::stol( subnode->string() );
            else if( subnode->is_type<SIM_VALUE_PARSER::fracPart>() )
                aParseResult.fracPart = std::stol( subnode->string() );
        }
    }
    else if( aNode.is_type<SIM_VALUE_PARSER::exponent>() )
    {
        aParseResult.exponent = std::stol( aNode.string() );
        aParseResult.isEmpty = false;
    }
    else if( aNode.is_type<SIM_VALUE_PARSER::metricSuffix<ValueType, Notation>>() )
    {
        aParseResult.metricSuffixExponent =
            SIM_VALUE_PARSER::MetricSuffixToExponent( aNode.string(), Notation );
        aParseResult.isEmpty = false;
    }
    else
        wxFAIL_MSG( "Unhandled parse tree node" );
}


SIM_VALUE_PARSER::PARSE_RESULT SIM_VALUE_PARSER::Parse( const wxString& aString,
                                                        NOTATION aNotation,
                                                        SIM_VALUE::TYPE aValueType )
{
    LOCALE_IO toggle;

    tao::pegtl::string_input<> in( aString.ToStdString(), "from_content" );
    std::unique_ptr<tao::pegtl::parse_tree::node> root;
    PARSE_RESULT result;

    try
    {
        CALL_INSTANCE( aValueType, aNotation, root = doParse, in );
    }
    catch( tao::pegtl::parse_error& e )
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
    catch( std::invalid_argument& e )
    {
        wxFAIL_MSG( wxString::Format( "Parsing simulator value failed: %s", e.what() ) );
        result.isOk = false;
    }

    return result;
}


long SIM_VALUE_PARSER::MetricSuffixToExponent( std::string aMetricSuffix, NOTATION aNotation )
{
    switch( aNotation )
    {
    case NOTATION::SI:
        if( aMetricSuffix.empty() )
            return 0;

        switch( aMetricSuffix[0] )
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
        std::transform( aMetricSuffix.begin(), aMetricSuffix.end(), aMetricSuffix.begin(),
                        ::tolower );

        if( aMetricSuffix == "f" )
            return -15;
        else if( aMetricSuffix == "p" )
            return -12;
        else if( aMetricSuffix == "n" )
            return -9;
        else if( aMetricSuffix == "u" )
            return -6;
        else if( aMetricSuffix == "m" )
            return -3;
        else if( aMetricSuffix == "" )
            return 0;
        else if( aMetricSuffix == "k" )
            return 3;
        else if( aMetricSuffix == "meg" )
            return 6;
        else if( aMetricSuffix == "g" )
            return 9;
        else if( aMetricSuffix == "t" )
            return 12;

        break;
    }

    wxFAIL_MSG( wxString::Format( "Unknown simulator value suffix: '%s'", aMetricSuffix ) );
    return 0;
}


wxString SIM_VALUE_PARSER::ExponentToMetricSuffix( double aExponent, long& aReductionExponent,
                                                   NOTATION aNotation )
{
    if( aNotation == NOTATION::SI && aExponent >= -18 && aExponent <= -15 )
    {
        aReductionExponent = -18;
        return "a";
    }
    else if( aExponent > -15 && aExponent <= -12 )
    {
        aReductionExponent = -15;
        return "f";
    }
    else if( aExponent > -12 && aExponent <= -9 )
    {
        aReductionExponent = -12;
        return "p";
    }
    else if( aExponent > -9 && aExponent <= -6 )
    {
        aReductionExponent = -9;
        return "n";
    }
    else if( aExponent > -6 && aExponent <= -3 )
    {
        aReductionExponent = -6;
        return "u";
    }
    else if( aExponent > -3 && aExponent < 0 )
    {
        aReductionExponent = -3;
        return "m";
    }
    else if( aExponent > 0 && aExponent < 3 )
    {
        aReductionExponent = 0;
        return "";
    }
    else if( aExponent >= 3 && aExponent < 6 )
    {
        aReductionExponent = 3;
        return "k";
    }
    else if( aExponent >= 6 && aExponent < 9 )
    {
        aReductionExponent = 6;
        return ( aNotation == NOTATION::SI ) ? "M" : "Meg";
    }
    else if( aExponent >= 9 && aExponent < 12 )
    {
        aReductionExponent = 9;
        return "G";
    }
    else if( aExponent >= 12 && aExponent < 15 )
    {
        aReductionExponent = 12;
        return "T";
    }
    else if( aNotation == NOTATION::SI && aExponent >= 15 && aExponent < 18 )
    {
        aReductionExponent = 15;
        return "P";
    }
    else if( aNotation == NOTATION::SI && aExponent >= 18 && aExponent <= 21 )
    {
        aReductionExponent = 18;
        return "E";
    }
    
    aReductionExponent = 0;
    return "";
}


std::unique_ptr<SIM_VALUE> SIM_VALUE::Create( TYPE aType, wxString aString )
{
    std::unique_ptr<SIM_VALUE> value = SIM_VALUE::Create( aType );
    value->FromString( aString );
    return value;
}


std::unique_ptr<SIM_VALUE> SIM_VALUE::Create( TYPE aType )
{
    switch( aType )
    {
    case TYPE::BOOL:           return std::make_unique<SIM_VALUE_INSTANCE<bool>>();
    case TYPE::INT:            return std::make_unique<SIM_VALUE_INSTANCE<long>>();
    case TYPE::FLOAT:          return std::make_unique<SIM_VALUE_INSTANCE<double>>();
    case TYPE::COMPLEX:        return std::make_unique<SIM_VALUE_INSTANCE<std::complex<double>>>();
    case TYPE::STRING:         return std::make_unique<SIM_VALUE_INSTANCE<wxString>>();
    case TYPE::BOOL_VECTOR:    return std::make_unique<SIM_VALUE_INSTANCE<bool>>();
    case TYPE::INT_VECTOR:     return std::make_unique<SIM_VALUE_INSTANCE<long>>();
    case TYPE::FLOAT_VECTOR:   return std::make_unique<SIM_VALUE_INSTANCE<double>>();
    case TYPE::COMPLEX_VECTOR: return std::make_unique<SIM_VALUE_INSTANCE<std::complex<double>>>();
    }
    
    wxFAIL_MSG( _( "Unknown SIM_VALUE type" ) );
    return nullptr;
}


void SIM_VALUE::operator=( const wxString& aString )
{
    FromString( aString );
}


template <typename T>
SIM_VALUE_INSTANCE<T>::SIM_VALUE_INSTANCE( const T& aValue ) : m_value( aValue )
{
}


template <>
bool SIM_VALUE_INSTANCE<bool>::FromString( const wxString& aString, NOTATION aNotation )
{
    SIM_VALUE_PARSER::PARSE_RESULT parseResult = SIM_VALUE_PARSER::Parse( aString, aNotation );
    m_value = NULLOPT;

    if( !parseResult.isOk )
        return false;

    if( parseResult.isEmpty )
        return true;

    if( !parseResult.intPart
        || ( *parseResult.intPart != 0 && *parseResult.intPart != 1 )
        || parseResult.fracPart
        || parseResult.exponent
        || parseResult.metricSuffixExponent )
    {
        return false;
    }

    m_value = *parseResult.intPart;
    return true;
}


template <>
bool SIM_VALUE_INSTANCE<long>::FromString( const wxString& aString, NOTATION aNotation )
{
    SIM_VALUE_PARSER::PARSE_RESULT parseResult = SIM_VALUE_PARSER::Parse( aString, aNotation );
    m_value = NULLOPT;

    if( !parseResult.isOk )
        return false;

    if( parseResult.isEmpty )
        return true;

    if( !parseResult.intPart || parseResult.fracPart )
        return false;

    long exponent = parseResult.exponent ? *parseResult.exponent : 0;
    exponent += parseResult.metricSuffixExponent ? *parseResult.metricSuffixExponent : 0;

    m_value = static_cast<double>( *parseResult.intPart ) * std::pow( 10, exponent );
    return true;
}


template <>
bool SIM_VALUE_INSTANCE<double>::FromString( const wxString& aString, NOTATION aNotation )
{
    SIM_VALUE_PARSER::PARSE_RESULT parseResult = SIM_VALUE_PARSER::Parse( aString, aNotation );
    m_value = NULLOPT;

    if( !parseResult.isOk )
        return false;

    if( parseResult.isEmpty )
        return true;

    // Single dot should be allowed in fields.
    // TODO: disallow single dot in models.
    if( parseResult.significand.empty() || parseResult.significand == "." )
        return false;

    long exponent = parseResult.exponent ? *parseResult.exponent : 0;
    exponent += parseResult.metricSuffixExponent ? *parseResult.metricSuffixExponent : 0;

    try
    {
        m_value = std::stod( parseResult.significand ) * std::pow( 10, exponent );
    }
    catch( const std::invalid_argument& e )
    {
        return false;
    }

    return true;
}


template <>
bool SIM_VALUE_INSTANCE<std::complex<double>>::FromString( const wxString& aString,
                                                           NOTATION aNotation )
{
    // TODO

    /*LOCALE_IO toggle;

    double value = 0;

    if( !aString.ToDouble( &value ) )
        throw KI_PARAM_ERROR( _( "Invalid complex sim value string" ) );

    m_value = value;*/
    return true;
}


template <>
bool SIM_VALUE_INSTANCE<wxString>::FromString( const wxString& aString, NOTATION aNotation )
{
    m_value = aString;
    return true;
}


template <typename T>
wxString SIM_VALUE_INSTANCE<T>::ToString( NOTATION aNotation ) const
{
    static_assert( std::is_same<T, std::vector<T>>::value );

    wxString string = "";

    for( auto it = m_value.cbegin(); it != m_value.cend(); it++ )
    {
        string += SIM_VALUE_INSTANCE<T>( *it ).ToString();
        string += ",";
    }

    return string;
}


template <>
wxString SIM_VALUE_INSTANCE<bool>::ToString( NOTATION aNotation ) const
{
    LOCALE_IO toggle;

    if( m_value.has_value() )
        return wxString::Format( "%d", *m_value );

    return "";
}


template <>
wxString SIM_VALUE_INSTANCE<long>::ToString( NOTATION aNotation ) const
{
    LOCALE_IO toggle;

    if( m_value.has_value() )
    {
        long value = *m_value;
        long exponent = 0;

        while( value % 1000 == 0 )
        {
            exponent += 3;
            value /= 1000;
        }

        long dummy = 0;
        wxString metricSuffix = SIM_VALUE_PARSER::ExponentToMetricSuffix(
                static_cast<double>( exponent ), dummy, aNotation );
        return wxString::Format( "%d%s", value, metricSuffix );
    }

    return "";
}


template <>
wxString SIM_VALUE_INSTANCE<double>::ToString( NOTATION aNotation ) const
{
    LOCALE_IO toggle;

    if( m_value.has_value() )
    {
        double exponent = std::log10( *m_value );
        long reductionExponent = 0;

        wxString metricSuffix = SIM_VALUE_PARSER::ExponentToMetricSuffix( exponent,
                                                                          reductionExponent );
        double reducedValue = *m_value / std::pow( 10, reductionExponent );

        return wxString::Format( "%g%s", reducedValue, metricSuffix );
    }
    else
        return "";
}


template <>
wxString SIM_VALUE_INSTANCE<std::complex<double>>::ToString( NOTATION aNotation ) const
{
    LOCALE_IO toggle;

    if( m_value.has_value() )
        return wxString::Format( "%g+%gi", m_value->real(), m_value->imag() );

    return "";
}


template <>
wxString SIM_VALUE_INSTANCE<wxString>::ToString( NOTATION aNotation ) const
{
    LOCALE_IO toggle;

    if( m_value.has_value() )
        return *m_value;

    return ""; // Empty string is completely equivalent to null string.
}


template <typename T>
wxString SIM_VALUE_INSTANCE<T>::ToSimpleString() const
{
    if( m_value.has_value() )
    {
        wxString result = "";
        result << *m_value;
        return result;
    }

    return "";
}


template <>
wxString SIM_VALUE_INSTANCE<std::complex<double>>::ToSimpleString() const
{
    // TODO

    /*wxString result = "";
    result << *m_value;

    return result;*/
    return "";
}


template <typename T>
bool SIM_VALUE_INSTANCE<T>::operator==( const SIM_VALUE& aOther ) const
{
    const SIM_VALUE_INSTANCE* otherNumber = dynamic_cast<const SIM_VALUE_INSTANCE*>( &aOther );

    if( otherNumber )
        return m_value == otherNumber->m_value;

    return false;
}
