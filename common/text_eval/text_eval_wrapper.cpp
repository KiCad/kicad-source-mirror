/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <text_eval/text_eval_wrapper.h>
#include <fmt/format.h>

// Include the KiCad common functionality
#include <common.h>
#include <fast_float/fast_float.h>
#include <text_eval/text_eval_types.h> // Parser types
#include <text_eval/text_eval_parser.h>
#include <text_eval/text_eval_units.h> // Centralized unit registry

namespace KI_EVAL
{

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#include <text_eval/text_eval.c>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
} // namespace KI_EVAL

#include <wx/log.h>
#include <algorithm>
#include <regex>
#include <span>

// // Token type enum matching the generated parser
enum class TextEvalToken : int
{
    ENDS = KI_EVAL_LT - 1,
    LT = KI_EVAL_LT,
    GT = KI_EVAL_GT,
    LE = KI_EVAL_LE,
    GE = KI_EVAL_GE,
    EQ = KI_EVAL_EQ,
    NE = KI_EVAL_NE,
    PLUS = KI_EVAL_PLUS,
    MINUS = KI_EVAL_MINUS,
    MULTIPLY = KI_EVAL_MULTIPLY,
    DIVIDE = KI_EVAL_DIVIDE,
    MODULO = KI_EVAL_MODULO,
    UMINUS = KI_EVAL_UMINUS,
    POWER = KI_EVAL_POWER,
    COMMA = KI_EVAL_COMMA,
    TEXT = KI_EVAL_TEXT,
    AT_OPEN = KI_EVAL_AT_OPEN,
    CLOSE_BRACE = KI_EVAL_CLOSE_BRACE,
    LPAREN = KI_EVAL_LPAREN,
    RPAREN = KI_EVAL_RPAREN,
    NUMBER = KI_EVAL_NUMBER,
    STRING = KI_EVAL_STRING,
    IDENTIFIER = KI_EVAL_IDENTIFIER,
    DOLLAR_OPEN = KI_EVAL_DOLLAR_OPEN,
};

// UTF-8 <-> UTF-32 conversion utilities
namespace utf8_utils
{

// Concept for UTF-8 byte validation
template <typename T>
concept Utf8Byte = std::same_as<T, char> || std::same_as<T, unsigned char> || std::same_as<T, std::byte>;

// UTF-8 validation and conversion
class UTF8_CONVERTER
{
private:
    // UTF-8 byte classification using bit operations
    static constexpr bool is_ascii( std::byte b ) noexcept { return ( b & std::byte{ 0x80 } ) == std::byte{ 0x00 }; }

    static constexpr bool is_continuation( std::byte b ) noexcept
    {
        return ( b & std::byte{ 0xC0 } ) == std::byte{ 0x80 };
    }

    static constexpr int sequence_length( std::byte first ) noexcept
    {
        if( is_ascii( first ) )
            return 1;
        if( ( first & std::byte{ 0xE0 } ) == std::byte{ 0xC0 } )
            return 2;
        if( ( first & std::byte{ 0xF0 } ) == std::byte{ 0xE0 } )
            return 3;
        if( ( first & std::byte{ 0xF8 } ) == std::byte{ 0xF0 } )
            return 4;
        return 0; // Invalid
    }

public:
    // Convert UTF-8 string to UTF-32 codepoints using C++20 ranges
    static std::u32string to_utf32( std::string_view utf8 )
    {
        std::u32string result;
        result.reserve( utf8.size() ); // Conservative estimate

        auto bytes = std::as_bytes( std::span{ utf8.data(), utf8.size() } );

        for( size_t i = 0; i < bytes.size(); )
        {
            std::byte first = bytes[i];
            int       len = sequence_length( first );

            if( len == 0 || i + len > bytes.size() )
            {
                // Invalid sequence - insert replacement character
                result.push_back( U'\uFFFD' );
                i++;
                continue;
            }

            char32_t codepoint = 0;

            switch( len )
            {
            case 1: codepoint = std::to_integer<char32_t>( first ); break;

            case 2:
            {
                if( !is_continuation( bytes[i + 1] ) )
                {
                    result.push_back( U'\uFFFD' );
                    i++;
                    continue;
                }
                codepoint = ( std::to_integer<char32_t>( first & std::byte{ 0x1F } ) << 6 )
                            | std::to_integer<char32_t>( bytes[i + 1] & std::byte{ 0x3F } );
                break;
            }

            case 3:
            {
                if( !is_continuation( bytes[i + 1] ) || !is_continuation( bytes[i + 2] ) )
                {
                    result.push_back( U'\uFFFD' );
                    i++;
                    continue;
                }
                codepoint = ( std::to_integer<char32_t>( first & std::byte{ 0x0F } ) << 12 )
                            | ( std::to_integer<char32_t>( bytes[i + 1] & std::byte{ 0x3F } ) << 6 )
                            | std::to_integer<char32_t>( bytes[i + 2] & std::byte{ 0x3F } );
                break;
            }

            case 4:
            {
                if( !is_continuation( bytes[i + 1] ) || !is_continuation( bytes[i + 2] )
                    || !is_continuation( bytes[i + 3] ) )
                {
                    result.push_back( U'\uFFFD' );
                    i++;
                    continue;
                }
                codepoint = ( std::to_integer<char32_t>( first & std::byte{ 0x07 } ) << 18 )
                            | ( std::to_integer<char32_t>( bytes[i + 1] & std::byte{ 0x3F } ) << 12 )
                            | ( std::to_integer<char32_t>( bytes[i + 2] & std::byte{ 0x3F } ) << 6 )
                            | std::to_integer<char32_t>( bytes[i + 3] & std::byte{ 0x3F } );
                break;
            }
            }

            // Validate codepoint range
            if( codepoint > 0x10FFFF || ( codepoint >= 0xD800 && codepoint <= 0xDFFF ) )
            {
                result.push_back( U'\uFFFD' ); // Replacement character
            }
            else if( len == 2 && codepoint < 0x80 )
            {
                result.push_back( U'\uFFFD' ); // Overlong encoding
            }
            else if( len == 3 && codepoint < 0x800 )
            {
                result.push_back( U'\uFFFD' ); // Overlong encoding
            }
            else if( len == 4 && codepoint < 0x10000 )
            {
                result.push_back( U'\uFFFD' ); // Overlong encoding
            }
            else
            {
                result.push_back( codepoint );
            }

            i += len;
        }

        return result;
    }

    // Convert UTF-32 to UTF-8
    static std::string to_utf8( std::u32string_view utf32 )
    {
        std::string result;
        result.reserve( utf32.size() * 4 ); // Maximum possible size

        for( char32_t cp : utf32 )
        {
            if( cp <= 0x7F )
            {
                // 1-byte sequence
                result.push_back( static_cast<char>( cp ) );
            }
            else if( cp <= 0x7FF )
            {
                // 2-byte sequence
                result.push_back( static_cast<char>( 0xC0 | ( cp >> 6 ) ) );
                result.push_back( static_cast<char>( 0x80 | ( cp & 0x3F ) ) );
            }
            else if( cp <= 0xFFFF )
            {
                // 3-byte sequence
                if( cp >= 0xD800 && cp <= 0xDFFF )
                {
                    // Surrogate pair - invalid in UTF-32
                    result.append( "\uFFFD" ); // Replacement character in UTF-8
                }
                else
                {
                    result.push_back( static_cast<char>( 0xE0 | ( cp >> 12 ) ) );
                    result.push_back( static_cast<char>( 0x80 | ( ( cp >> 6 ) & 0x3F ) ) );
                    result.push_back( static_cast<char>( 0x80 | ( cp & 0x3F ) ) );
                }
            }
            else if( cp <= 0x10FFFF )
            {
                // 4-byte sequence
                result.push_back( static_cast<char>( 0xF0 | ( cp >> 18 ) ) );
                result.push_back( static_cast<char>( 0x80 | ( ( cp >> 12 ) & 0x3F ) ) );
                result.push_back( static_cast<char>( 0x80 | ( ( cp >> 6 ) & 0x3F ) ) );
                result.push_back( static_cast<char>( 0x80 | ( cp & 0x3F ) ) );
            }
            else
            {
                // Invalid codepoint
                result.append( "\uFFFD" ); // Replacement character in UTF-8
            }
        }

        return result;
    }
};

template <typename T>
concept UnicodeCodepoint = std::same_as<T, char32_t>;

struct CHARACTER_CLASSIFIER
{
    static constexpr bool is_whitespace( UnicodeCodepoint auto cp ) noexcept
    {
        // Unicode whitespace categories
        return cp == U' ' || cp == U'\t' || cp == U'\r' || cp == U'\n' || cp == U'\f' || cp == U'\v' || cp == U'\u00A0'
               || // Non-breaking space
               cp == U'\u2000' || cp == U'\u2001' || cp == U'\u2002' || cp == U'\u2003' || cp == U'\u2004'
               || cp == U'\u2005' || cp == U'\u2006' || cp == U'\u2007' || cp == U'\u2008' || cp == U'\u2009'
               || cp == U'\u200A' || cp == U'\u2028' || cp == U'\u2029' || cp == U'\u202F' || cp == U'\u205F'
               || cp == U'\u3000';
    }

    static constexpr bool is_digit( UnicodeCodepoint auto cp ) noexcept { return cp >= U'0' && cp <= U'9'; }

    static constexpr bool is_ascii_alpha( UnicodeCodepoint auto cp ) noexcept
    {
        return ( cp >= U'a' && cp <= U'z' ) || ( cp >= U'A' && cp <= U'Z' );
    }

    static constexpr bool is_alpha( UnicodeCodepoint auto cp ) noexcept
    {
        // Basic Latin + extended Unicode letter ranges
        return is_ascii_alpha( cp ) || ( cp >= 0x80 && cp <= 0x10FFFF && cp != 0xFFFD );
    }

    static constexpr bool is_alnum( UnicodeCodepoint auto cp ) noexcept { return is_alpha( cp ) || is_digit( cp ); }
};

struct SI_PREFIX_HANDLER
{
    struct PREFIX
    {
        char32_t symbol;
        double   multiplier;
    };

    static constexpr std::array<PREFIX, 18> prefixes = { { { U'a', 1e-18 },
                                                           { U'f', 1e-15 },
                                                           { U'p', 1e-12 },
                                                           { U'n', 1e-9 },
                                                           { U'u', 1e-6 },
                                                           { U'µ', 1e-6 },
                                                           { U'μ', 1e-6 }, // Various micro symbols
                                                           { U'm', 1e-3 },
                                                           { U'k', 1e3 },
                                                           { U'K', 1e3 },
                                                           { U'M', 1e6 },
                                                           { U'G', 1e9 },
                                                           { U'T', 1e12 },
                                                           { U'P', 1e15 },
                                                           { U'E', 1e18 } } };

    static constexpr bool is_si_prefix( UnicodeCodepoint auto cp ) noexcept
    {
        return std::ranges::any_of( prefixes,
                                    [cp]( const PREFIX& p )
                                    {
                                        return p.symbol == cp;
                                    } );
    }

    static constexpr double get_multiplier( UnicodeCodepoint auto cp ) noexcept
    {
        auto it = std::ranges::find_if( prefixes,
                                        [cp]( const PREFIX& p )
                                        {
                                            return p.symbol == cp;
                                        } );
        return it != prefixes.end() ? it->multiplier : 1.0;
    }
};
} // namespace utf8_utils

// Unit conversion utilities for the text evaluator
namespace KIEVAL_UNIT_CONV
{

// Internal unit enum matching NUMERIC_EVALUATOR
enum class Unit
{
    Invalid,
    UM,
    MM,
    CM,
    Inch,
    Mil,
    Degrees,
    SI,
    Femtoseconds,
    Picoseconds,
    PsPerInch,
    PsPerCm,
    PsPerMm
};

// Convert EDA_UNITS to internal Unit enum
Unit edaUnitsToInternal( EDA_UNITS aUnits )
{
    switch( aUnits )
    {
    case EDA_UNITS::MM: return Unit::MM;
    case EDA_UNITS::MILS: return Unit::Mil;
    case EDA_UNITS::INCH: return Unit::Inch;
    case EDA_UNITS::DEGREES: return Unit::Degrees;
    case EDA_UNITS::FS: return Unit::Femtoseconds;
    case EDA_UNITS::PS: return Unit::Picoseconds;
    case EDA_UNITS::PS_PER_INCH: return Unit::PsPerInch;
    case EDA_UNITS::PS_PER_CM: return Unit::PsPerCm;
    case EDA_UNITS::PS_PER_MM: return Unit::PsPerMm;
    case EDA_UNITS::UM: return Unit::UM;
    case EDA_UNITS::CM: return Unit::CM;
    case EDA_UNITS::UNSCALED: return Unit::SI;
    default: return Unit::MM;
    }
}

// Parse unit from string using centralized registry
Unit parseUnit( const std::string& aUnitStr )
{
    auto evalUnit = text_eval_units::UnitRegistry::parseUnit( aUnitStr );

    // Convert text_eval_units::Unit to KIEVAL_UNIT_CONV::Unit
    switch( evalUnit )
    {
    case text_eval_units::Unit::MM: return Unit::MM;
    case text_eval_units::Unit::CM: return Unit::CM;
    case text_eval_units::Unit::INCH: return Unit::Inch;
    case text_eval_units::Unit::INCH_QUOTE: return Unit::Inch;
    case text_eval_units::Unit::MIL: return Unit::Mil;
    case text_eval_units::Unit::THOU: return Unit::Mil;
    case text_eval_units::Unit::UM: return Unit::UM;
    case text_eval_units::Unit::DEG: return Unit::Degrees;
    case text_eval_units::Unit::DEGREE_SYMBOL: return Unit::Degrees;
    case text_eval_units::Unit::PS: return Unit::Picoseconds;
    case text_eval_units::Unit::FS: return Unit::Femtoseconds;
    case text_eval_units::Unit::PS_PER_IN: return Unit::PsPerInch;
    case text_eval_units::Unit::PS_PER_CM: return Unit::PsPerCm;
    case text_eval_units::Unit::PS_PER_MM: return Unit::PsPerMm;
    default: return Unit::Invalid;
    }
}

// Get conversion factor from one unit to another (based on numeric_evaluator logic)
double getConversionFactor( Unit aFromUnit, Unit aToUnit )
{
    if( aFromUnit == aToUnit )
        return 1.0;

    // Convert to MM first, then to target unit
    double toMM = 1.0;
    switch( aFromUnit )
    {
    case Unit::Inch: toMM = 25.4; break;
    case Unit::Mil: toMM = 25.4 / 1000.0; break;
    case Unit::UM: toMM = 1.0 / 1000.0; break;
    case Unit::MM: toMM = 1.0; break;
    case Unit::CM: toMM = 10.0; break;
    default: return 1.0; // No conversion for other units
    }

    double fromMM = 1.0;
    switch( aToUnit )
    {
    case Unit::Inch: fromMM = 1.0 / 25.4; break;
    case Unit::Mil: fromMM = 1000.0 / 25.4; break;
    case Unit::UM: fromMM = 1000.0; break;
    case Unit::MM: fromMM = 1.0; break;
    case Unit::CM: fromMM = 1.0 / 10.0; break;
    default: return 1.0; // No conversion for other units
    }

    return toMM * fromMM;
}

// Convert a value with units to the default units using centralized registry
double convertToDefaultUnits( double aValue, const std::string& aUnitStr, EDA_UNITS aDefaultUnits )
{
    return text_eval_units::UnitRegistry::convertToEdaUnits( aValue, aUnitStr, aDefaultUnits );
}
} // namespace KIEVAL_UNIT_CONV


class KIEVAL_TEXT_TOKENIZER
{
private:
    enum class TOKENIZER_CONTEXT
    {
        TEXT,      // Regular text content - alphabetic should be TEXT tokens
        EXPRESSION // Inside @{...} or ${...} - alphabetic should be IDENTIFIER tokens
    };

    std::u32string                m_text;
    size_t                        m_pos{ 0 };
    size_t                        m_line{ 1 };
    size_t                        m_column{ 1 };
    TOKENIZER_CONTEXT             m_context{ TOKENIZER_CONTEXT::TEXT };
    int                           m_braceNestingLevel{ 0 }; // Track nesting level of expressions
    calc_parser::ERROR_COLLECTOR* m_errorCollector{ nullptr };
    EDA_UNITS                     m_defaultUnits{ EDA_UNITS::MM }; // Add default units for conversion

    using CLASSIFIER = utf8_utils::CHARACTER_CLASSIFIER;
    using SI_HANDLER = utf8_utils::SI_PREFIX_HANDLER;

    [[nodiscard]] char32_t current_char() const noexcept { return m_pos < m_text.size() ? m_text[m_pos] : U'\0'; }

    [[nodiscard]] char32_t peek_char( size_t offset = 1 ) const noexcept
    {
        size_t peek_pos = m_pos + offset;
        return peek_pos < m_text.size() ? m_text[peek_pos] : U'\0';
    }

    void advance_position( size_t count = 1 ) noexcept
    {
        for( size_t i = 0; i < count && m_pos < m_text.size(); ++i )
        {
            if( m_text[m_pos] == U'\n' )
            {
                ++m_line;
                m_column = 1;
            }
            else
            {
                ++m_column;
            }
            ++m_pos;
        }
    }

    void skip_whitespace() noexcept
    {
        while( m_pos < m_text.size() && CLASSIFIER::is_whitespace( current_char() ) )
            advance_position();
    }

    void add_error( std::string_view message ) const
    {
        if( m_errorCollector )
        {
            auto error_msg = fmt::format( "Line {}, Column {}: {}", m_line, m_column, message );
            m_errorCollector->AddError( error_msg );
        }
    }

    [[nodiscard]] static calc_parser::TOKEN_TYPE make_string_token( std::string value ) noexcept
    {
        calc_parser::TOKEN_TYPE token{};
        token.isString = true;
        std::strncpy( token.text, value.c_str(), sizeof( token.text ) - 1 );
        token.text[sizeof( token.text ) - 1] = '\0';
        token.dValue = 0.0;
        return token;
    }

    [[nodiscard]] static constexpr calc_parser::TOKEN_TYPE make_number_token( double value ) noexcept
    {
        calc_parser::TOKEN_TYPE token{};
        token.isString = false;
        token.dValue = value;
        return token;
    }

    [[nodiscard]] calc_parser::TOKEN_TYPE parse_string_literal( char32_t quote_char )
    {
        advance_position(); // Skip opening quote

        std::u32string content;
        content.reserve( 64 ); // Reasonable default

        while( m_pos < m_text.size() && current_char() != quote_char )
        {
            char32_t c = current_char();

            if( c == U'\\' && m_pos + 1 < m_text.size() )
            {
                char32_t escaped = peek_char();
                advance_position( 2 );

                switch( escaped )
                {
                case U'n': content.push_back( U'\n' ); break;
                case U't': content.push_back( U'\t' ); break;
                case U'r': content.push_back( U'\r' ); break;
                case U'\\': content.push_back( U'\\' ); break;
                case U'"': content.push_back( U'"' ); break;
                case U'\'': content.push_back( U'\'' ); break;
                case U'0': content.push_back( U'\0' ); break;
                case U'x':
                {
                    // Hexadecimal escape \xHH
                    std::u32string hex;

                    for( int i = 0; i < 2 && m_pos < m_text.size(); ++i )
                    {
                        char32_t hex_char = current_char();
                        if( ( hex_char >= U'0' && hex_char <= U'9' ) || ( hex_char >= U'A' && hex_char <= U'F' )
                            || ( hex_char >= U'a' && hex_char <= U'f' ) )
                        {
                            hex.push_back( hex_char );
                            advance_position();
                        }
                        else
                        {
                            break;
                        }
                    }

                    if( !hex.empty() )
                    {
                        try
                        {
                            auto hex_str = utf8_utils::UTF8_CONVERTER::to_utf8( hex );
                            auto value = std::stoul( hex_str, nullptr, 16 );

                            if( value <= 0x10FFFF )
                                content.push_back( static_cast<char32_t>( value ) );
                            else
                                content.push_back( U'\uFFFD' );
                        }
                        catch( ... )
                        {
                            content.push_back( U'\uFFFD' );
                        }
                    }
                    else
                    {
                        content.append( U"\\x" );
                    }

                    break;
                }
                default:
                    content.push_back( U'\\' );
                    content.push_back( escaped );
                    break;
                }
            }
            else if( c == U'\n' )
            {
                add_error( "Unterminated string literal" );
                break;
            }
            else
            {
                content.push_back( c );
                advance_position();
            }
        }

        if( m_pos < m_text.size() && current_char() == quote_char )
        {
            advance_position(); // Skip closing quote
        }
        else
        {
            add_error( "Missing closing quote in string literal" );
        }

        return make_string_token( utf8_utils::UTF8_CONVERTER::to_utf8( content ) );
    }

    [[nodiscard]] calc_parser::TOKEN_TYPE parse_number()
    {
        std::u32string number_text;
        number_text.reserve( 32 );

        double multiplier = 1.0;

        // Parse integer part
        while( m_pos < m_text.size() && CLASSIFIER::is_digit( current_char() ) )
        {
            number_text.push_back( current_char() );
            advance_position();
        }

        // Handle decimal point, SI prefix, or unit suffix
        if( m_pos < m_text.size() )
        {
            char32_t c = current_char();

            // Only treat comma as decimal separator in text context, not expression context
            // This prevents comma from interfering with function argument separation
            if( c == U'.' || ( c == U',' && m_context != TOKENIZER_CONTEXT::EXPRESSION ) )
            {
                number_text.push_back( U'.' );
                advance_position();
            }
            else if( m_context == TOKENIZER_CONTEXT::EXPRESSION && CLASSIFIER::is_alpha( c ) )
            {
                // In expression context, check for unit first before SI prefix (unit strings are longer)
                // Look ahead to see if we have a complete unit string
                std::u32string potential_unit;
                size_t         temp_pos = m_pos;

                while( temp_pos < m_text.size() )
                {
                    char32_t unit_char = m_text[temp_pos];

                    if( CLASSIFIER::is_alpha( unit_char ) || unit_char == U'"' || unit_char == U'\'' )
                    {
                        potential_unit.push_back( unit_char );
                        temp_pos++;
                    }
                    else
                    {
                        break;
                    }
                }

                // Check if we have a valid unit
                if( !potential_unit.empty() )
                {
                    std::string            unit_str = utf8_utils::UTF8_CONVERTER::to_utf8( potential_unit );
                    KIEVAL_UNIT_CONV::Unit parsed_unit = KIEVAL_UNIT_CONV::parseUnit( unit_str );

                    if( parsed_unit != KIEVAL_UNIT_CONV::Unit::Invalid )
                    {
                        // This is a valid unit - don't treat the first character as SI prefix
                        // The unit parsing will happen later
                    }
                    else if( SI_HANDLER::is_si_prefix( c ) )
                    {
                        // Not a valid unit, so treat as SI prefix
                        multiplier = SI_HANDLER::get_multiplier( c );
                        advance_position();
                    }
                }
                else if( SI_HANDLER::is_si_prefix( c ) )
                {
                    // No alphabetic characters following, so treat as SI prefix
                    multiplier = SI_HANDLER::get_multiplier( c );
                    advance_position();
                }
            }
            else if( SI_HANDLER::is_si_prefix( c ) )
            {
                // In text context, treat as SI prefix
                multiplier = SI_HANDLER::get_multiplier( c );
                advance_position();
            }
        }

        // Parse fractional part
        while( m_pos < m_text.size() && CLASSIFIER::is_digit( current_char() ) )
        {
            number_text.push_back( current_char() );
            advance_position();
        }

        // Check for scientific notation (e.g., 1e-3, 3.5E6)
        if( m_pos < m_text.size() )
        {
            char32_t c = current_char();

            if( c == U'e' || c == U'E' )
            {
                // Look ahead to see if this is scientific notation (followed by +, -, or digit)
                size_t temp_pos = m_pos + 1;
                bool   is_scientific = false;

                if( temp_pos < m_text.size() )
                {
                    char32_t next = m_text[temp_pos];
                    if( next == U'+' || next == U'-' || CLASSIFIER::is_digit( next ) )
                    {
                        is_scientific = true;
                    }
                }

                if( is_scientific )
                {
                    // Parse scientific notation exponent
                    number_text.push_back( c ); // Add 'e' or 'E'
                    advance_position();

                    // Optional sign
                    if( m_pos < m_text.size() && ( current_char() == U'+' || current_char() == U'-' ) )
                    {
                        number_text.push_back( current_char() );
                        advance_position();
                    }

                    // Exponent digits (required)
                    if( m_pos < m_text.size() && CLASSIFIER::is_digit( current_char() ) )
                    {
                        while( m_pos < m_text.size() && CLASSIFIER::is_digit( current_char() ) )
                        {
                            number_text.push_back( current_char() );
                            advance_position();
                        }
                    }
                    else
                    {
                        // Invalid scientific notation - will fail in conversion
                        add_error( "Invalid scientific notation: missing exponent digits" );
                    }
                }
            }
        }

        // Check for SI prefix after fractional part (for numbers like 0.3M)
        if( m_pos < m_text.size() && multiplier == 1.0 )
        {
            char32_t c = current_char();

            if( m_context == TOKENIZER_CONTEXT::EXPRESSION && CLASSIFIER::is_alpha( c ) )
            {
                // Look ahead to check for unit vs SI prefix
                std::u32string potential_unit;
                size_t         temp_pos = m_pos;

                while( temp_pos < m_text.size() )
                {
                    char32_t unit_char = m_text[temp_pos];

                    if( CLASSIFIER::is_alpha( unit_char ) || unit_char == U'"' || unit_char == U'\'' )
                    {
                        potential_unit.push_back( unit_char );
                        temp_pos++;
                    }
                    else
                    {
                        break;
                    }
                }

                if( !potential_unit.empty() )
                {
                    std::string            unit_str = utf8_utils::UTF8_CONVERTER::to_utf8( potential_unit );
                    KIEVAL_UNIT_CONV::Unit parsed_unit = KIEVAL_UNIT_CONV::parseUnit( unit_str );

                    if( parsed_unit == KIEVAL_UNIT_CONV::Unit::Invalid && SI_HANDLER::is_si_prefix( c ) )
                    {
                        // Not a valid unit, so treat as SI prefix
                        multiplier = SI_HANDLER::get_multiplier( c );
                        advance_position();
                    }
                }
            }
            else if( SI_HANDLER::is_si_prefix( c ) )
            {
                // In text context, treat as SI prefix
                multiplier = SI_HANDLER::get_multiplier( c );
                advance_position();
            }
        }

        // Convert to double safely
        auto   number_str = utf8_utils::UTF8_CONVERTER::to_utf8( number_text );
        double value = 0.0;

        try
        {
            if( !number_str.empty() && number_str != "." )
            {
                auto result = fast_float::from_chars( number_str.data(), number_str.data() + number_str.size(), value );

                if( result.ec != std::errc() || result.ptr != number_str.data() + number_str.size() )
                    throw std::invalid_argument( fmt::format( "Cannot convert '{}' to number", number_str ) );

                value *= multiplier;

                if( !std::isfinite( value ) )
                {
                    add_error( "Number out of range" );
                    value = 0.0;
                }
            }
        }
        catch( const std::exception& e )
        {
            add_error( fmt::format( "Invalid number format: {}", e.what() ) );
            value = 0.0;
        }

        // Look for unit suffix
        if( m_pos < m_text.size() && m_context == TOKENIZER_CONTEXT::EXPRESSION )
        {
            // Skip any whitespace between number and unit
            size_t whitespace_start = m_pos;
            while( m_pos < m_text.size() && CLASSIFIER::is_whitespace( current_char() ) )
            {
                advance_position();
            }

            // Parse potential unit suffix
            std::u32string unit_text;

            // Look ahead to parse potential unit (letters, quotes, etc.)
            while( m_pos < m_text.size() )
            {
                char32_t c = current_char();

                // Unit characters: letters, quotes for inches
                if( CLASSIFIER::is_alpha( c ) || c == U'"' || c == U'\'' )
                {
                    unit_text.push_back( c );
                    advance_position();
                }
                else
                {
                    break;
                }
            }

            if( !unit_text.empty() )
            {
                // Convert unit text to string and try to parse it
                std::string            unit_str = utf8_utils::UTF8_CONVERTER::to_utf8( unit_text );
                KIEVAL_UNIT_CONV::Unit parsed_unit = KIEVAL_UNIT_CONV::parseUnit( unit_str );

                if( parsed_unit != KIEVAL_UNIT_CONV::Unit::Invalid )
                {
                    // Successfully parsed unit - convert value to default units
                    double converted_value = KIEVAL_UNIT_CONV::convertToDefaultUnits( value, unit_str, m_defaultUnits );
                    value = converted_value;
                }
                else
                {
                    // Not a valid unit - backtrack to before the whitespace
                    m_pos = whitespace_start;
                }
            }
            else
            {
                // No unit found - backtrack to before the whitespace
                m_pos = whitespace_start;
            }
        }

        return make_number_token( value );
    }

    [[nodiscard]] calc_parser::TOKEN_TYPE parse_identifier()
    {
        std::u32string identifier;
        identifier.reserve( 64 );

        while( m_pos < m_text.size() && ( CLASSIFIER::is_alnum( current_char() ) || current_char() == U'_' ) )
        {
            identifier.push_back( current_char() );
            advance_position();
        }

        return make_string_token( utf8_utils::UTF8_CONVERTER::to_utf8( identifier ) );
    }

    [[nodiscard]] calc_parser::TOKEN_TYPE parse_text_content()
    {
        std::u32string text;
        text.reserve( 256 );

        while( m_pos < m_text.size() )
        {
            char32_t current = current_char();
            char32_t next = peek_char();

            // Stop at special sequences
            if( ( current == U'@' && next == U'{' ) || ( current == U'$' && next == U'{' ) )
            {
                break;
            }

            text.push_back( current );
            advance_position();
        }

        return make_string_token( utf8_utils::UTF8_CONVERTER::to_utf8( text ) );
    }

public:
    explicit KIEVAL_TEXT_TOKENIZER( std::string_view input, calc_parser::ERROR_COLLECTOR* error_collector = nullptr,
                                    EDA_UNITS default_units = EDA_UNITS::MM ) :
            m_errorCollector( error_collector ),
            m_defaultUnits( default_units )
    {
        m_text = utf8_utils::UTF8_CONVERTER::to_utf32( input );
    }

    [[nodiscard]] TextEvalToken get_next_token( calc_parser::TOKEN_TYPE& token_value )
    {
        token_value = calc_parser::TOKEN_TYPE{};

        if( m_pos >= m_text.size() )
        {
            return TextEvalToken::ENDS;
        }

        // Only skip whitespace in expression context
        if( m_context == TOKENIZER_CONTEXT::EXPRESSION )
        {
            skip_whitespace();

            if( m_pos >= m_text.size() )
            {
                return TextEvalToken::ENDS;
            }
        }

        char32_t current = current_char();
        char32_t next = peek_char();

        // Multi-character tokens that switch to expression context
        if( current == U'@' && next == U'{' )
        {
            advance_position( 2 );
            m_context = TOKENIZER_CONTEXT::EXPRESSION; // Switch to expression context
            m_braceNestingLevel++;                     // Increment nesting level
            token_value = make_string_token( "@{" );
            return TextEvalToken::AT_OPEN;
        }

        if( current == U'$' && next == U'{' )
        {
            advance_position( 2 );
            m_context = TOKENIZER_CONTEXT::EXPRESSION; // Switch to expression context
            m_braceNestingLevel++;                     // Increment nesting level
            token_value = make_string_token( "${" );
            return TextEvalToken::DOLLAR_OPEN;
        }

        // Handle closing brace specially to manage context correctly
        if( current == U'}' )
        {
            advance_position();
            m_braceNestingLevel--; // Decrement nesting level
            if( m_braceNestingLevel <= 0 )
            {
                m_braceNestingLevel = 0;             // Clamp to zero
                m_context = TOKENIZER_CONTEXT::TEXT; // Switch back to text context only when fully unnested
            }
            token_value = make_string_token( "}" );
            return TextEvalToken::CLOSE_BRACE;
        }

        // Multi-character comparison operators
        if( current == U'<' && next == U'=' )
        {
            advance_position( 2 );
            token_value = make_string_token( "<=" );
            return TextEvalToken::LE;
        }
        if( current == U'>' && next == U'=' )
        {
            advance_position( 2 );
            token_value = make_string_token( ">=" );
            return TextEvalToken::GE;
        }
        if( current == U'=' && next == U'=' )
        {
            advance_position( 2 );
            token_value = make_string_token( "==" );
            return TextEvalToken::EQ;
        }
        if( current == U'!' && next == U'=' )
        {
            advance_position( 2 );
            token_value = make_string_token( "!=" );
            return TextEvalToken::NE;
        }

        // Single character tokens using structured binding
        // Single character tokens (only in expression context)
        if( m_context == TOKENIZER_CONTEXT::EXPRESSION )
        {
            static constexpr std::array<std::pair<char32_t, TextEvalToken>, 11> single_char_tokens{
                { { U'(', TextEvalToken::LPAREN },
                  { U')', TextEvalToken::RPAREN },
                  { U'+', TextEvalToken::PLUS },
                  { U'-', TextEvalToken::MINUS },
                  { U'*', TextEvalToken::MULTIPLY },
                  { U'/', TextEvalToken::DIVIDE },
                  { U'%', TextEvalToken::MODULO },
                  { U'^', TextEvalToken::POWER },
                  { U',', TextEvalToken::COMMA },
                  { U'<', TextEvalToken::LT },
                  { U'>', TextEvalToken::GT } }
            };

            if( auto it = std::ranges::find_if( single_char_tokens,
                                                [current]( const auto& pair )
                                                {
                                                    return pair.first == current;
                                                } );
                it != single_char_tokens.end() )
            {
                advance_position();
                token_value = make_string_token( utf8_utils::UTF8_CONVERTER::to_utf8( std::u32string{ current } ) );
                return it->second;
            }
        }

        // Complex tokens
        if( current == U'"' || current == U'\'' )
        {
            token_value = parse_string_literal( current );
            return TextEvalToken::STRING;
        }

        if( CLASSIFIER::is_digit( current ) || ( current == U'.' && CLASSIFIER::is_digit( next ) ) )
        {
            token_value = parse_number();
            return TextEvalToken::NUMBER;
        }

        // Context-aware handling of alphabetic content
        if( CLASSIFIER::is_alpha( current ) || current == U'_' )
        {
            if( m_context == TOKENIZER_CONTEXT::EXPRESSION )
            {
                // In expression context, alphabetic content is an identifier
                token_value = parse_identifier();
                return TextEvalToken::IDENTIFIER;
            }
            else
            {
                // In text context, alphabetic content is part of regular text
                token_value = parse_text_content();
                return TextEvalToken::TEXT;
            }
        }

        // Default to text content
        token_value = parse_text_content();
        return token_value.text[0] == U'\0' ? TextEvalToken::ENDS : TextEvalToken::TEXT;
    }

    [[nodiscard]] bool             has_more_tokens() const noexcept { return m_pos < m_text.size(); }
    [[nodiscard]] constexpr size_t get_line() const noexcept { return m_line; }
    [[nodiscard]] constexpr size_t get_column() const noexcept { return m_column; }
};

EXPRESSION_EVALUATOR::EXPRESSION_EVALUATOR( bool aClearVariablesOnEvaluate ) :
        m_clearVariablesOnEvaluate( aClearVariablesOnEvaluate ),
        m_useCustomCallback( false ),
        m_defaultUnits( EDA_UNITS::MM ) // Default to millimeters
{
    m_lastErrors = std::make_unique<calc_parser::ERROR_COLLECTOR>();
}

EXPRESSION_EVALUATOR::EXPRESSION_EVALUATOR( VariableCallback aVariableCallback, bool aClearVariablesOnEvaluate ) :
        m_clearVariablesOnEvaluate( aClearVariablesOnEvaluate ),
        m_customCallback( std::move( aVariableCallback ) ),
        m_useCustomCallback( true ),
        m_defaultUnits( EDA_UNITS::MM ) // Default to millimeters
{
    m_lastErrors = std::make_unique<calc_parser::ERROR_COLLECTOR>();
}

EXPRESSION_EVALUATOR::EXPRESSION_EVALUATOR( EDA_UNITS aUnits, bool aClearVariablesOnEvaluate ) :
        m_clearVariablesOnEvaluate( aClearVariablesOnEvaluate ),
        m_useCustomCallback( false ),
        m_defaultUnits( aUnits )
{
    m_lastErrors = std::make_unique<calc_parser::ERROR_COLLECTOR>();
}

EXPRESSION_EVALUATOR::EXPRESSION_EVALUATOR( EDA_UNITS aUnits, VariableCallback aVariableCallback,
                                            bool aClearVariablesOnEvaluate ) :
        m_clearVariablesOnEvaluate( aClearVariablesOnEvaluate ),
        m_customCallback( std::move( aVariableCallback ) ),
        m_useCustomCallback( true ),
        m_defaultUnits( aUnits )
{
    m_lastErrors = std::make_unique<calc_parser::ERROR_COLLECTOR>();
}

EXPRESSION_EVALUATOR::~EXPRESSION_EVALUATOR() = default;

EXPRESSION_EVALUATOR::EXPRESSION_EVALUATOR( const EXPRESSION_EVALUATOR& aOther ) :
        m_variables( aOther.m_variables ),
        m_clearVariablesOnEvaluate( aOther.m_clearVariablesOnEvaluate ),
        m_customCallback( aOther.m_customCallback ),
        m_useCustomCallback( aOther.m_useCustomCallback ),
        m_defaultUnits( aOther.m_defaultUnits )
{
    m_lastErrors = std::make_unique<calc_parser::ERROR_COLLECTOR>();
    if( aOther.m_lastErrors )
    {
        // Copy error state
        for( const auto& error : aOther.m_lastErrors->GetErrors() )
            m_lastErrors->AddError( error );
    }
}

EXPRESSION_EVALUATOR& EXPRESSION_EVALUATOR::operator=( const EXPRESSION_EVALUATOR& aOther )
{
    if( this != &aOther )
    {
        m_variables = aOther.m_variables;
        m_clearVariablesOnEvaluate = aOther.m_clearVariablesOnEvaluate;
        m_customCallback = aOther.m_customCallback;
        m_useCustomCallback = aOther.m_useCustomCallback;
        m_defaultUnits = aOther.m_defaultUnits;

        m_lastErrors = std::make_unique<calc_parser::ERROR_COLLECTOR>();
        if( aOther.m_lastErrors )
        {
            for( const auto& error : aOther.m_lastErrors->GetErrors() )
                m_lastErrors->AddError( error );
        }
    }
    return *this;
}

EXPRESSION_EVALUATOR::EXPRESSION_EVALUATOR( EXPRESSION_EVALUATOR&& aOther ) noexcept :
        m_variables( std::move( aOther.m_variables ) ),
        m_lastErrors( std::move( aOther.m_lastErrors ) ),
        m_clearVariablesOnEvaluate( aOther.m_clearVariablesOnEvaluate ),
        m_customCallback( std::move( aOther.m_customCallback ) ),
        m_useCustomCallback( aOther.m_useCustomCallback ),
        m_defaultUnits( aOther.m_defaultUnits )
{
}

EXPRESSION_EVALUATOR& EXPRESSION_EVALUATOR::operator=( EXPRESSION_EVALUATOR&& aOther ) noexcept
{
    if( this != &aOther )
    {
        m_variables = std::move( aOther.m_variables );
        m_lastErrors = std::move( aOther.m_lastErrors );
        m_clearVariablesOnEvaluate = aOther.m_clearVariablesOnEvaluate;
        m_customCallback = std::move( aOther.m_customCallback );
        m_useCustomCallback = aOther.m_useCustomCallback;
        m_defaultUnits = aOther.m_defaultUnits;
    }
    return *this;
}

void EXPRESSION_EVALUATOR::SetVariableCallback( VariableCallback aCallback )
{
    m_customCallback = std::move( aCallback );
    m_useCustomCallback = true;
}

void EXPRESSION_EVALUATOR::ClearVariableCallback()
{
    m_customCallback = VariableCallback{};
    m_useCustomCallback = false;
}

bool EXPRESSION_EVALUATOR::HasVariableCallback() const
{
    return m_useCustomCallback && m_customCallback;
}

void EXPRESSION_EVALUATOR::SetDefaultUnits( EDA_UNITS aUnits )
{
    m_defaultUnits = aUnits;
}

EDA_UNITS EXPRESSION_EVALUATOR::GetDefaultUnits() const
{
    return m_defaultUnits;
}

void EXPRESSION_EVALUATOR::SetVariable( const wxString& aName, double aValue )
{
    std::string name = wxStringToStdString( aName );
    m_variables[name] = calc_parser::Value{ aValue };
}

void EXPRESSION_EVALUATOR::SetVariable( const wxString& aName, const wxString& aValue )
{
    std::string name = wxStringToStdString( aName );
    std::string value = wxStringToStdString( aValue );
    m_variables[name] = calc_parser::Value{ value };
}

void EXPRESSION_EVALUATOR::SetVariable( const std::string& aName, const std::string& aValue )
{
    m_variables[aName] = calc_parser::Value{ aValue };
}

bool EXPRESSION_EVALUATOR::RemoveVariable( const wxString& aName )
{
    std::string name = wxStringToStdString( aName );
    return m_variables.erase( name ) > 0;
}

void EXPRESSION_EVALUATOR::ClearVariables()
{
    m_variables.clear();
}

bool EXPRESSION_EVALUATOR::HasVariable( const wxString& aName ) const
{
    std::string name = wxStringToStdString( aName );
    return m_variables.find( name ) != m_variables.end();
}

wxString EXPRESSION_EVALUATOR::GetVariable( const wxString& aName ) const
{
    std::string name = wxStringToStdString( aName );
    auto        it = m_variables.find( name );
    if( it != m_variables.end() )
    {
        if( std::holds_alternative<double>( it->second ) )
        {
            double val = std::get<double>( it->second );
            // Smart formatting - whole numbers don't need decimal places
            if( val == std::floor( val ) && std::abs( val ) < 1e15 )
                return wxString::Format( "%.0f", val );
            else
                return wxString::Format( "%g", val );
        }
        else
        {
            return stdStringToWxString( std::get<std::string>( it->second ) );
        }
    }
    return wxString{};
}

std::vector<wxString> EXPRESSION_EVALUATOR::GetVariableNames() const
{
    std::vector<wxString> names;
    names.reserve( m_variables.size() );

    for( const auto& [name, value] : m_variables )
        names.push_back( stdStringToWxString( name ) );

    return names;
}

void EXPRESSION_EVALUATOR::SetVariables( const std::unordered_map<wxString, double>& aVariables )
{
    for( const auto& [name, value] : aVariables )
        SetVariable( name, value );
}

void EXPRESSION_EVALUATOR::SetVariables( const std::unordered_map<wxString, wxString>& aVariables )
{
    for( const auto& [name, value] : aVariables )
        SetVariable( name, value );
}

wxString EXPRESSION_EVALUATOR::Evaluate( const wxString& aInput )
{
    std::unordered_map<wxString, double>   emptyNumVars;
    std::unordered_map<wxString, wxString> emptyStringVars;
    return Evaluate( aInput, emptyNumVars, emptyStringVars );
}

wxString EXPRESSION_EVALUATOR::Evaluate( const wxString&                             aInput,
                                         const std::unordered_map<wxString, double>& aTempVariables )
{
    std::unordered_map<wxString, wxString> emptyStringVars;
    return Evaluate( aInput, aTempVariables, emptyStringVars );
}

wxString EXPRESSION_EVALUATOR::Evaluate( const wxString&                               aInput,
                                         const std::unordered_map<wxString, double>&   aTempNumericVars,
                                         const std::unordered_map<wxString, wxString>& aTempStringVars )
{
    // Clear previous errors
    ClearErrors();

    // Expand ${variable} patterns that are OUTSIDE of @{} expressions
    wxString processedInput = expandVariablesOutsideExpressions( aInput, aTempNumericVars, aTempStringVars );

    // Convert processed input to std::string
    std::string input = wxStringToStdString( processedInput ); // Create combined callback for all variable sources
    auto        combinedCallback = createCombinedCallback( &aTempNumericVars, &aTempStringVars );

    // Evaluate using parser
    auto [result, hadErrors] = evaluateWithParser( input, combinedCallback );

    // Update error state if evaluation had errors
    if( hadErrors && !m_lastErrors )
    {
        m_lastErrors = std::make_unique<calc_parser::ERROR_COLLECTOR>();
    }
    if( hadErrors )
    {
        m_lastErrors->AddError( "Evaluation failed" );
    }

    // Clear variables if requested
    if( m_clearVariablesOnEvaluate )
        ClearVariables();

    // Convert result back to wxString
    return stdStringToWxString( result );
}

bool EXPRESSION_EVALUATOR::HasErrors() const
{
    return m_lastErrors && m_lastErrors->HasErrors();
}

wxString EXPRESSION_EVALUATOR::GetErrorSummary() const
{
    if( !m_lastErrors )
        return wxString{};

    return stdStringToWxString( m_lastErrors->GetAllMessages() );
}

size_t EXPRESSION_EVALUATOR::GetErrorCount() const
{
    if( !m_lastErrors )
        return 0;

    return m_lastErrors->GetErrors().size();
}

std::vector<wxString> EXPRESSION_EVALUATOR::GetErrors() const
{
    std::vector<wxString> result;

    if( m_lastErrors )
    {
        const auto& errors = m_lastErrors->GetErrors();
        result.reserve( errors.size() );

        for( const auto& error : errors )
            result.push_back( stdStringToWxString( error ) );
    }

    return result;
}

void EXPRESSION_EVALUATOR::ClearErrors()
{
    if( m_lastErrors )
        m_lastErrors->Clear();
}

void EXPRESSION_EVALUATOR::SetClearVariablesOnEvaluate( bool aEnable )
{
    m_clearVariablesOnEvaluate = aEnable;
}

bool EXPRESSION_EVALUATOR::GetClearVariablesOnEvaluate() const
{
    return m_clearVariablesOnEvaluate;
}

bool EXPRESSION_EVALUATOR::TestExpression( const wxString& aExpression )
{
    // Create a test input with the expression wrapped in @{}
    wxString testInput = "@{" + aExpression + "}";

    // Create a minimal callback that returns errors for all variables
    auto testCallback = []( const std::string& aVarName ) -> calc_parser::Result<calc_parser::Value>
    {
        return calc_parser::MakeError<calc_parser::Value>( "Test mode - no variables available" );
    };

    // Try to parse it
    std::string input = wxStringToStdString( testInput );
    auto [result, hadErrors] = evaluateWithParser( input, testCallback );

    // Check if there were parsing errors (ignore evaluation errors for undefined variables)
    if( m_lastErrors )
    {
        const auto& errors = m_lastErrors->GetErrors();
        // Filter out "Test mode - no variables available" errors, look for syntax errors
        for( const auto& error : errors )
        {
            if( error.find( "Syntax error" ) != std::string::npos
                || error.find( "Parser failed" ) != std::string::npos )
            {
                return false; // Found syntax error
            }
        }
    }

    return true; // No syntax errors found
}

size_t EXPRESSION_EVALUATOR::CountExpressions( const wxString& aInput ) const
{
    size_t count = 0;
    size_t pos = 0;

    while( ( pos = aInput.find( "@{", pos ) ) != wxString::npos )
    {
        count++;
        pos += 2; // Move past "@{"
    }

    return count;
}

std::vector<wxString> EXPRESSION_EVALUATOR::ExtractExpressions( const wxString& aInput ) const
{
    std::vector<wxString> expressions;
    size_t                pos = 0;

    while( ( pos = aInput.find( "@{", pos ) ) != wxString::npos )
    {
        size_t start = pos + 2; // Skip "@{"
        size_t end = aInput.find( "}", start );

        if( end != wxString::npos )
        {
            expressions.push_back( aInput.substr( start, end - start ) );
            pos = end + 1;
        }
        else
        {
            break; // No closing brace found
        }
    }

    return expressions;
}

std::string EXPRESSION_EVALUATOR::wxStringToStdString( const wxString& aWxStr ) const
{
    return aWxStr.ToStdString( wxConvUTF8 );
}

wxString EXPRESSION_EVALUATOR::stdStringToWxString( const std::string& aStdStr ) const
{
    return wxString( aStdStr.c_str(), wxConvUTF8 );
}

wxString EXPRESSION_EVALUATOR::expandVariablesOutsideExpressions(
        const wxString& aInput, const std::unordered_map<wxString, double>& aTempNumericVars,
        const std::unordered_map<wxString, wxString>& aTempStringVars ) const
{
    wxString result = aInput;
    size_t   pos = 0;

    // Track positions of @{} expressions to avoid substituting inside them
    std::vector<std::pair<size_t, size_t>> expressionRanges;

    // Find all @{} expression ranges
    while( ( pos = result.find( "@{", pos ) ) != std::string::npos )
    {
        size_t start = pos;
        size_t braceCount = 1;
        size_t searchPos = start + 2; // Skip "@{"

        // Find matching closing brace
        while( searchPos < result.length() && braceCount > 0 )
        {
            if( result[searchPos] == '{' )
                braceCount++;
            else if( result[searchPos] == '}' )
                braceCount--;
            searchPos++;
        }

        if( braceCount == 0 )
        {
            expressionRanges.emplace_back( start, searchPos ); // searchPos is after '}'
        }

        pos = searchPos;
    }

    // Now find and replace ${variable} patterns that are NOT inside @{} expressions
    pos = 0;
    while( ( pos = result.find( "${", pos ) ) != std::string::npos )
    {
        // Check if this ${} is inside any @{} expression
        bool insideExpression = false;
        for( const auto& range : expressionRanges )
        {
            if( pos >= range.first && pos < range.second )
            {
                insideExpression = true;
                break;
            }
        }

        if( insideExpression )
        {
            // Special case: if this variable is immediately followed by unit text,
            // we should expand it to allow proper unit parsing
            size_t closePos = result.find( "}", pos + 2 );
            if( closePos != std::string::npos )
            {
                // Check what comes after the closing brace
                size_t afterBrace = closePos + 1;
                bool   followedByUnit = false;

                if( afterBrace < result.length() )
                {
                    // Check if followed by any supported unit strings using centralized registry
                    const auto units = text_eval_units::UnitRegistry::getAllUnitStrings();
                    for( const auto& unit : units )
                    {
                        if( afterBrace + unit.length() <= result.length()
                            && result.substr( afterBrace, unit.length() ) == unit )
                        {
                            followedByUnit = true;
                            break;
                        }
                    }
                }

                if( !followedByUnit )
                {
                    pos += 2; // Skip this ${} since it's inside an expression and not followed by units
                    continue;
                }
                // If followed by units, continue with variable expansion below
            }
            else
            {
                pos += 2; // Invalid pattern, skip
                continue;
            }
        }

        // Find the closing brace
        size_t closePos = result.find( "}", pos + 2 );
        if( closePos == std::string::npos )
        {
            pos += 2; // Invalid ${} pattern, skip
            continue;
        }

        // Extract variable name
        wxString varName = result.substr( pos + 2, closePos - pos - 2 );
        wxString replacement;
        bool     found = false;

        // Check temporary string variables first
        auto stringIt = aTempStringVars.find( varName );
        if( stringIt != aTempStringVars.end() )
        {
            replacement = stringIt->second;
            found = true;
        }
        else
        {
            // Check temporary numeric variables
            auto numIt = aTempNumericVars.find( varName );
            if( numIt != aTempNumericVars.end() )
            {
                replacement = wxString::FromDouble( numIt->second );
                found = true;
            }
            else
            {
                // Check instance variables
                std::string stdVarName = wxStringToStdString( varName );
                auto        instIt = m_variables.find( stdVarName );
                if( instIt != m_variables.end() )
                {
                    const calc_parser::Value& value = instIt->second;
                    if( std::holds_alternative<std::string>( value ) )
                    {
                        replacement = stdStringToWxString( std::get<std::string>( value ) );
                        found = true;
                    }
                    else if( std::holds_alternative<double>( value ) )
                    {
                        replacement = wxString::FromDouble( std::get<double>( value ) );
                        found = true;
                    }
                }
            }
        }

        if( found )
        {
            // Replace ${variable} with its value
            result.replace( pos, closePos - pos + 1, replacement );
            pos += replacement.length();
        }
        else
        {
            // Variable not found, record error but leave ${variable} unchanged
            if( !m_lastErrors )
                m_lastErrors = std::make_unique<calc_parser::ERROR_COLLECTOR>();
            m_lastErrors->AddError( fmt::format( "Undefined variable: {}", wxStringToStdString( varName ) ) );
            pos = closePos + 1;
        }
    }

    return result;
}

EXPRESSION_EVALUATOR::VariableCallback
EXPRESSION_EVALUATOR::createCombinedCallback( const std::unordered_map<wxString, double>*   aTempNumericVars,
                                              const std::unordered_map<wxString, wxString>* aTempStringVars ) const
{
    return [this, aTempNumericVars,
            aTempStringVars]( const std::string& aVarName ) -> calc_parser::Result<calc_parser::Value>
    {
        // Priority 1: Custom callback (if set)
        if( m_useCustomCallback && m_customCallback )
        {
            auto customResult = m_customCallback( aVarName );
            if( customResult.HasValue() )
                return customResult;

            // If custom callback returned an error, continue to fallback options
            // unless the error indicates a definitive "not found" vs "lookup failed"
            // For simplicity, we'll always try fallbacks
        }

        // Priority 2: Temporary string variables
        if( aTempStringVars )
        {
            wxString wxVarName = stdStringToWxString( aVarName );
            if( auto it = aTempStringVars->find( wxVarName ); it != aTempStringVars->end() )
            {
                std::string stdValue = wxStringToStdString( it->second );
                return calc_parser::MakeValue<calc_parser::Value>( stdValue );
            }
        }

        // Priority 3: Temporary numeric variables
        if( aTempNumericVars )
        {
            wxString wxVarName = stdStringToWxString( aVarName );
            if( auto it = aTempNumericVars->find( wxVarName ); it != aTempNumericVars->end() )
            {
                return calc_parser::MakeValue<calc_parser::Value>( it->second );
            }
        }

        // Priority 4: Stored variables
        if( auto it = m_variables.find( aVarName ); it != m_variables.end() )
        {
            return calc_parser::MakeValue<calc_parser::Value>( it->second );
        }

        // Priority 5: Use KiCad's ExpandTextVars for system/project variables
        try
        {
            wxString varName = stdStringToWxString( aVarName );
            wxString testString = wxString::Format( "${%s}", varName );

            // Create a resolver that will return true if the variable was found
            bool                             wasResolved = false;
            std::function<bool( wxString* )> resolver = [&wasResolved]( wxString* token ) -> bool
            {
                // If we get here, ExpandTextVars found the variable and wants to resolve it
                // For our purposes, we just want to know if it exists, so return false
                // to keep the original ${varname} format, and set our flag
                wasResolved = true;
                return false; // Don't replace, just detect
            };

            wxString expandedResult = ExpandTextVars( testString, &resolver );

            if( wasResolved )
            {
                // Variable exists in KiCad's system, now get its actual value
                std::function<bool( wxString* )> valueResolver = []( wxString* token ) -> bool
                {
                    // Let ExpandTextVars resolve this normally
                    // We'll get the resolved value in token
                    return false; // Use default resolution
                };

                wxString resolvedValue = ExpandTextVars( testString, &valueResolver );

                // Check if it was actually resolved (not still ${varname})
                if( resolvedValue != testString )
                {
                    std::string resolvedStd = wxStringToStdString( resolvedValue );

                    // Try to parse as number first
                    try
                    {
                        double numValue;
                        auto   result = fast_float::from_chars( resolvedStd.data(),
                                                                resolvedStd.data() + resolvedStd.size(), numValue );

                        if( result.ec != std::errc() || result.ptr != resolvedStd.data() + resolvedStd.size() )
                            throw std::invalid_argument( fmt::format( "Cannot convert '{}' to number", resolvedStd ) );

                        return calc_parser::MakeValue<calc_parser::Value>( numValue );
                    }
                    catch( ... )
                    {
                        // Not a number, return as string
                        return calc_parser::MakeValue<calc_parser::Value>( resolvedStd );
                    }
                }
            }
        }
        catch( const std::exception& e )
        {
            // ExpandTextVars failed, continue to error
        }

        // Priority 6: If custom callback was tried and failed, return its error
        if( m_useCustomCallback && m_customCallback )
        {
            return m_customCallback( aVarName ); // Return the original error
        }

        // No variable found anywhere
        return calc_parser::MakeError<calc_parser::Value>( fmt::format( "Undefined variable: {}", aVarName ) );
    };
}

std::pair<std::string, bool> EXPRESSION_EVALUATOR::evaluateWithParser( const std::string& aInput,
                                                                       VariableCallback   aVariableCallback )
{
    try
    {
        // Try partial error recovery first
        auto [partialResult, partialHadErrors] = evaluateWithPartialErrorRecovery( aInput, aVariableCallback );

        // If partial recovery made any progress (result differs from input), use it
        if( partialResult != aInput )
        {
            // Partial recovery made progress - always report errors collected during partial recovery
            return { std::move( partialResult ), partialHadErrors };
        }

        // If no progress was made, try original full parsing approach as fallback
        return evaluateWithFullParser( aInput, std::move( aVariableCallback ) );
    }
    catch( const std::bad_alloc& )
    {
        if( m_lastErrors )
        {
            m_lastErrors->AddError( "Out of memory" );
        }
        return { aInput, true };
    }
    catch( const std::exception& e )
    {
        if( m_lastErrors )
        {
            m_lastErrors->AddError( fmt::format( "Exception: {}", e.what() ) );
        }
        return { aInput, true };
    }
}

std::pair<std::string, bool>
EXPRESSION_EVALUATOR::evaluateWithPartialErrorRecovery( const std::string& aInput, VariableCallback aVariableCallback )
{
    std::string result = aInput;
    bool        hadAnyErrors = false;
    size_t      pos = 0;

    // Process expressions from right to left to avoid position shifts
    std::vector<std::pair<size_t, size_t>> expressionRanges;

    // Find all expression ranges
    while( ( pos = result.find( "@{", pos ) ) != std::string::npos )
    {
        size_t start = pos;
        size_t exprStart = pos + 2; // Skip "@{"
        size_t braceCount = 1;
        size_t searchPos = exprStart;

        // Find matching closing brace, handling nested braces
        while( searchPos < result.length() && braceCount > 0 )
        {
            if( result[searchPos] == '{' )
            {
                braceCount++;
            }
            else if( result[searchPos] == '}' )
            {
                braceCount--;
            }
            searchPos++;
        }

        if( braceCount == 0 )
        {
            size_t end = searchPos; // Position after the '}'
            expressionRanges.emplace_back( start, end );
            pos = end;
        }
        else
        {
            pos = exprStart; // Skip this malformed expression
        }
    }

    // Process expressions from right to left to avoid position shifts
    for( auto it = expressionRanges.rbegin(); it != expressionRanges.rend(); ++it )
    {
        auto [start, end] = *it;
        std::string fullExpr = result.substr( start, end - start );
        std::string innerExpr = result.substr( start + 2, end - start - 3 ); // Remove @{ and }

        // Try to evaluate this single expression
        try
        {
            // Create a simple expression for evaluation
            std::string testExpr = "@{" + innerExpr + "}";

            // Create a temporary error collector to capture errors for this specific expression
            auto tempErrors = std::make_unique<calc_parser::ERROR_COLLECTOR>();
            auto oldErrors = std::move( m_lastErrors );
            m_lastErrors = std::move( tempErrors );

            // Use the full parser for this single expression
            auto [evalResult, evalHadErrors] = evaluateWithFullParser( testExpr, aVariableCallback );

            if( !evalHadErrors )
            {
                // Successful evaluation, replace in result
                result.replace( start, end - start, evalResult );
            }
            else
            {
                // Expression failed - add a specific error for this expression
                hadAnyErrors = true;

                // Restore main error collector and add error
                if( !oldErrors )
                    oldErrors = std::make_unique<calc_parser::ERROR_COLLECTOR>();

                oldErrors->AddError( fmt::format( "Failed to evaluate expression: {}", fullExpr ) );
            }

            // Restore the main error collector
            m_lastErrors = std::move( oldErrors );
        }
        catch( ... )
        {
            // Report exception as an error for this expression
            if( !m_lastErrors )
                m_lastErrors = std::make_unique<calc_parser::ERROR_COLLECTOR>();

            m_lastErrors->AddError( fmt::format( "Exception in expression: {}", fullExpr ) );
            hadAnyErrors = true;
        }
    }

    return { std::move( result ), hadAnyErrors };
}

std::pair<std::string, bool> EXPRESSION_EVALUATOR::evaluateWithFullParser( const std::string& aInput,
                                                                           VariableCallback   aVariableCallback )
{
    if( aInput.empty() )
    {
        return { std::string{}, false };
    }

    // RAII guard for error collector cleanup
    struct ErrorCollectorGuard
    {
        ~ErrorCollectorGuard() { calc_parser::g_errorCollector = nullptr; }
    } guard;

    try
    {
        // Clear previous errors
        if( m_lastErrors )
        {
            m_lastErrors->Clear();
        }

        // Set up error collector
        calc_parser::g_errorCollector = m_lastErrors.get();

        // Create tokenizer with default units
        KIEVAL_TEXT_TOKENIZER tokenizer{ aInput, m_lastErrors.get(), m_defaultUnits };

        // Create parser deleter function
        auto parser_deleter = []( void* p )
        {
            KI_EVAL::ParseFree( p, free );
        };

        // Allocate parser with RAII cleanup
        std::unique_ptr<void, decltype( parser_deleter )> parser{ KI_EVAL::ParseAlloc( malloc ), parser_deleter };

        if( !parser )
        {
            if( m_lastErrors )
            {
                m_lastErrors->AddError( "Failed to allocate parser" );
            }
            return { aInput, true };
        }

        // Parse document
        calc_parser::DOC* document = nullptr;

        calc_parser::TOKEN_TYPE token_value;
        TextEvalToken           token_type;

        do
        {
            token_type = tokenizer.get_next_token( token_value );

            // Send token to parser
            KI_EVAL::Parse( parser.get(), static_cast<int>( token_type ), token_value, &document );

            // Early exit on errors
            if( m_lastErrors && m_lastErrors->HasErrors() )
            {
                break;
            }

        } while( token_type != TextEvalToken::ENDS && tokenizer.has_more_tokens() );

        // Finalize parsing
        KI_EVAL::Parse( parser.get(), static_cast<int>( TextEvalToken::ENDS ), calc_parser::TOKEN_TYPE{}, &document );

        // Process document if parsing succeeded
        if( document && ( !m_lastErrors || !m_lastErrors->HasErrors() ) )
        {
            calc_parser::DOC_PROCESSOR processor;
            auto [result, had_errors] = processor.Process( *document, std::move( aVariableCallback ) );

            // If processing had any evaluation errors, return original input unchanged
            // This preserves the original expression syntax while still reporting errors
            if( had_errors )
            {
                delete document;
                return { aInput, true };
            }

            delete document;
            return { std::move( result ), had_errors };
        }

        // Cleanup and return original on error
        delete document;
        return { aInput, true };
    }
    catch( const std::bad_alloc& )
    {
        if( m_lastErrors )
        {
            m_lastErrors->AddError( "Out of memory" );
        }
        return { aInput, true };
    }
    catch( const std::exception& e )
    {
        if( m_lastErrors )
        {
            m_lastErrors->AddError( fmt::format( "Exception: {}", e.what() ) );
        }
        return { aInput, true };
    }
}

NUMERIC_EVALUATOR_COMPAT::NUMERIC_EVALUATOR_COMPAT( EDA_UNITS aUnits ) :
        m_evaluator( aUnits ),
        m_lastValid( false )
{
}

NUMERIC_EVALUATOR_COMPAT::~NUMERIC_EVALUATOR_COMPAT() = default;

void NUMERIC_EVALUATOR_COMPAT::Clear()
{
    m_lastInput.clear();
    m_lastResult.clear();
    m_lastValid = false;
    m_evaluator.ClearErrors();
}

void NUMERIC_EVALUATOR_COMPAT::SetDefaultUnits( EDA_UNITS aUnits )
{
    m_evaluator.SetDefaultUnits( aUnits );
}

void NUMERIC_EVALUATOR_COMPAT::LocaleChanged()
{
    // No-op: EXPRESSION_EVALUATOR handles locale properly internally
}

bool NUMERIC_EVALUATOR_COMPAT::IsValid() const
{
    return m_lastValid;
}

wxString NUMERIC_EVALUATOR_COMPAT::Result() const
{
    return m_lastResult;
}

bool NUMERIC_EVALUATOR_COMPAT::Process( const wxString& aString )
{
    m_lastInput = aString;
    m_evaluator.ClearErrors();

    // Convert bare variable names to ${variable} syntax for compatibility
    // This allows NUMERIC_EVALUATOR-style variable access to work with EXPRESSION_EVALUATOR
    wxString processedExpression = aString;

    // Get all variable names that are currently defined
    auto varNames = m_evaluator.GetVariableNames();

    // Sort variable names by length (longest first) to avoid partial replacements
    std::sort( varNames.begin(), varNames.end(),
               []( const wxString& a, const wxString& b )
               {
                   return a.length() > b.length();
               } );

    // Replace bare variable names with ${variable} syntax
    for( const auto& varName : varNames )
    {
        // Create a regex to match the variable name as a whole word
        // This avoids replacing parts of other words
        wxString pattern = "\\b" + varName + "\\b";
        wxString replacement = "${" + varName + "}";

        // Simple string replacement (not regex for now to avoid complexity)
        // Look for the variable name surrounded by non-alphanumeric characters
        size_t pos = 0;

        while( ( pos = processedExpression.find( varName, pos ) ) != wxString::npos )
        {
            // Check if this is a whole word (not part of another identifier)
            bool isWholeWord = true;

            // Check character before
            if( pos > 0 )
            {
                wxChar before = processedExpression[pos - 1];
                if( wxIsalnum( before ) || before == '_' || before == '$' )
                    isWholeWord = false;
            }

            // Check character after
            if( isWholeWord && pos + varName.length() < processedExpression.length() )
            {
                wxChar after = processedExpression[pos + varName.length()];
                if( wxIsalnum( after ) || after == '_' )
                    isWholeWord = false;
            }

            if( isWholeWord )
            {
                processedExpression.replace( pos, varName.length(), replacement );
                pos += replacement.length();
            }
            else
            {
                pos += varName.length();
            }
        }
    }

    // Wrap the processed expression in @{...} syntax for EXPRESSION_EVALUATOR
    wxString wrappedExpression = "@{" + processedExpression + "}";

    m_lastResult = m_evaluator.Evaluate( wrappedExpression );
    m_lastValid = !m_evaluator.HasErrors();

    // Additional check: if the result is exactly the wrapped expression,
    // it means the expression wasn't evaluated (likely due to errors)
    if( m_lastResult == wrappedExpression )
    {
        m_lastValid = false;
        m_lastResult = "NaN";
    }

    // If there were errors, set result to "NaN" to match NUMERIC_EVALUATOR behavior
    if( !m_lastValid )
    {
        m_lastResult = "NaN";
        return false;
    }

    return true;
}

wxString NUMERIC_EVALUATOR_COMPAT::OriginalText() const
{
    return m_lastInput;
}

void NUMERIC_EVALUATOR_COMPAT::SetVar( const wxString& aString, double aValue )
{
    m_evaluator.SetVariable( aString, aValue );
}

double NUMERIC_EVALUATOR_COMPAT::GetVar( const wxString& aString )
{
    if( !m_evaluator.HasVariable( aString ) )
        return 0.0;

    wxString value = m_evaluator.GetVariable( aString );

    // Try to convert to double
    double result = 0.0;

    if( !value.ToDouble( &result ) )
        return 0.0;

    return result;
}

void NUMERIC_EVALUATOR_COMPAT::RemoveVar( const wxString& aString )
{
    m_evaluator.RemoveVariable( aString );
}

void NUMERIC_EVALUATOR_COMPAT::ClearVar()
{
    m_evaluator.ClearVariables();
}
