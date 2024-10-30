/*
 * This file is part of libeval, a simple math expression evaluator
 *
 * Copyright (C) 2017 Michael Geselbracht, mgeselbracht3@gmail.com
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string_utils.h>
#include <libeval/numeric_evaluator.h>

/* The (generated) lemon parser is written in C.
 * In order to keep its symbol from the global namespace include the parser code with
 * a C++ namespace.
 */
namespace numEval
{

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#include <libeval/grammar.c>
#include <libeval/grammar.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

} /* namespace numEval */


NUMERIC_EVALUATOR::NUMERIC_EVALUATOR( EDA_UNITS aUnits )
{
    LocaleChanged();

    m_parseError = false;
    m_parseFinished = false;

    m_parser = numEval::ParseAlloc( malloc );

    SetDefaultUnits( aUnits );
}


NUMERIC_EVALUATOR::~NUMERIC_EVALUATOR()
{
    numEval::ParseFree( m_parser, free );

    // Allow explicit call to destructor
    m_parser = nullptr;

    Clear();
}


void NUMERIC_EVALUATOR::Clear()
{
    delete[] m_token.token;
    m_token.token = nullptr;
    m_token.input = nullptr;
    m_parseError = true;
    m_originalText = wxEmptyString;
}


void NUMERIC_EVALUATOR::SetDefaultUnits( EDA_UNITS aUnits )
{
    switch( aUnits )
    {
    case EDA_UNITS::MILLIMETRES: m_defaultUnits = Unit::MM;      break;
    case EDA_UNITS::MILS:        m_defaultUnits = Unit::Mil;     break;
    case EDA_UNITS::INCHES:      m_defaultUnits = Unit::Inch;    break;
    case EDA_UNITS::DEGREES:     m_defaultUnits = Unit::Degrees; break;
    case EDA_UNITS::UNSCALED:    m_defaultUnits = Unit::SI;      break;
    default:                     m_defaultUnits = Unit::MM;      break;
    }
}


void NUMERIC_EVALUATOR::LocaleChanged()
{
    struct lconv* lc = localeconv();
    m_localeDecimalSeparator = *lc->decimal_point;
}


// NOT UNUSED.  Called by LEMON grammar.
void NUMERIC_EVALUATOR::parseError( const char* s )
{
    m_parseError = true;
}


// NOT UNUSED.  Called by LEMON grammar.
void NUMERIC_EVALUATOR::parseOk()
{
    m_parseFinished = true;
}


// NOT UNUSED.  Called by LEMON grammar.
void NUMERIC_EVALUATOR::parseSetResult( double val )
{
    if( std::isnan( val ) )
    {
        // Naively printing this with %g produces "nan" on some platforms
        // and "-nan(ind)" on others (e.g. MSVC). So force a "standard" string.
        snprintf( m_token.token, m_token.outputLen, "%s", "NaN" );
    }
    else
    {
        // Can be printed as a floating point
        // Warning: DO NOT use a format like %f or %g, because they can create issues.
        // especially %g can generate an exponent, incompatible with UNIT_BINDER
        // Use the optimized UIDouble2Str
        snprintf( m_token.token, m_token.outputLen, "%s", UIDouble2Str( val ).c_str() );
    }
}


wxString NUMERIC_EVALUATOR::OriginalText() const
{
    return m_originalText;
}


bool NUMERIC_EVALUATOR::Process( const wxString& aString )
{
    // Feed parser token after token until end of input.

    newString( aString );
    m_parseError = false;
    m_parseFinished = false;
    Token tok;

    if( aString.IsEmpty() )
    {
        m_parseFinished = true;
        return true;
    }

    do
    {
        tok = getToken();
        numEval::Parse( m_parser, tok.token, tok.value, this );

        if( m_parseFinished || tok.token == ENDS )
        {
            // Reset parser by passing zero as token ID, value is ignored.
            numEval::Parse( m_parser, 0, tok.value, this );
            break;
        }
    } while( tok.token );

    return !m_parseError;
}


void NUMERIC_EVALUATOR::newString( const wxString& aString )
{
    Clear();

    m_originalText = aString;
    m_token.input = aString.mb_str();
    m_token.inputLen = strlen( m_token.input );
    m_token.outputLen = std::max<std::size_t>( 64, m_token.inputLen + 1 );
    m_token.pos = 0;
    m_token.token = new char[m_token.outputLen]();
    m_token.token[0] = '0';

    m_parseFinished = false;
}


NUMERIC_EVALUATOR::Token NUMERIC_EVALUATOR::getToken()
{
    Token retval;
    size_t idx;

    retval.token = ENDS;
    retval.value.dValue = 0;
    retval.value.valid = false;
    retval.value.text[0] = 0;

    if( m_token.token == nullptr )
        return retval;

    if( m_token.input == nullptr )
        return retval;

    if( m_token.pos >= m_token.inputLen )
        return retval;

    // Support for old school decimal separators (ie: "2K2")
    auto isOldSchoolDecimalSeparator =
            []( char ch, double* siScaler ) -> bool
            {
                switch( ch )
                {
                case 'a': *siScaler = 1.0e-18; return true;
                case 'f': *siScaler = 1.0e-15; return true;
                case 'p': *siScaler = 1.0e-12; return true;
                case 'n': *siScaler = 1.0e-9;  return true;
                case 'u': *siScaler = 1.0e-6;  return true;
                case 'm': *siScaler = 1.0e-3;  return true;
                case 'k':
                case 'K': *siScaler = 1.0e3;   return true;
                case 'M': *siScaler = 1.0e6;   return true;
                case 'G': *siScaler = 1.0e9;   return true;
                case 'T': *siScaler = 1.0e12;  return true;
                case 'P': *siScaler = 1.0e15;  return true;
                case 'E': *siScaler = 1.0e18;  return true;
                default:                       return false;
                }
            };

    auto isDecimalSeparator =
            [&]( char ch ) -> bool
            {
                double dummy;

                if( ch == m_localeDecimalSeparator || ch == '.' || ch == ',' )
                    return true;

                if( m_defaultUnits == Unit::SI && isOldSchoolDecimalSeparator( ch, &dummy ) )
                    return true;

                return false;
            };

    // Lambda: get value as string, store into clToken.token and update current index.
    auto extractNumber =
            [&]( double* aScaler )
            {
                bool   haveSeparator = false;
                double siScaler = 1.0;
                char   ch = m_token.input[ m_token.pos ];

                idx = 0;

                do
                {
                    if( isDecimalSeparator( ch ) )
                    {
                        if( haveSeparator )
                            break;
                        else
                            haveSeparator = true;

                        if( isOldSchoolDecimalSeparator( ch, &siScaler ) )
                            *aScaler = siScaler;

                        m_token.token[ idx++ ] = m_localeDecimalSeparator;
                    }
                    else
                    {
                        m_token.token[ idx++ ] = ch;
                    }

                    ch = m_token.input[ ++m_token.pos ];
                } while( isdigit( (unsigned char) ch ) || isDecimalSeparator( ch ) );

                m_token.token[ idx ] = 0;
            };

    // Lamda: Get unit for current token.
    // Valid units are ", in, um, cm, mm, mil and thou. Returns Unit::Invalid otherwise.
    auto checkUnit =
            [&]( double* siScaler ) -> Unit
            {
                char ch = m_token.input[ m_token.pos ];

                if( ch == '"' )
                {
                    m_token.pos++;
                    return Unit::Inch;
                }

                // Do not use strcasecmp() as it is not available on all platforms
                const char* cptr = &m_token.input[ m_token.pos ];
                const auto sizeLeft = m_token.inputLen - m_token.pos;

                // We should really give this unicode support
                if( sizeLeft >= 2 && ch == '\xC2' && cptr[1] == '\xB0' )
                {
                    m_token.pos += 2;
                    return Unit::Degrees;
                }

                // Ideally we should also handle the unicode characters that can be used for micro,
                // but unicode handling in this tokenizer doesn't work.
                // (e.g. add support for μm (µ is MICRO SIGN), µm (µ is GREEK SMALL LETTER MU) later)
                if( sizeLeft >= 2 && ch == 'u' && cptr[ 1 ] == 'm' && !isalnum( cptr[ 2 ] ) )
                {
                    m_token.pos += 2;
                    return Unit::UM;
                }

                if( sizeLeft >= 2 && ch == 'm' && cptr[ 1 ] == 'm' && !isalnum( cptr[ 2 ] ) )
                {
                    m_token.pos += 2;
                    return Unit::MM;
                }

                if( sizeLeft >= 2 && ch == 'c' && cptr[ 1 ] == 'm' && !isalnum( cptr[ 2 ] ) )
                {
                    m_token.pos += 2;
                    return Unit::CM;
                }

                if( sizeLeft >= 2 && ch == 'i' && cptr[ 1 ] == 'n' && !isalnum( cptr[ 2 ] ) )
                {
                    m_token.pos += 2;
                    return Unit::Inch;
                }

                if( sizeLeft >= 3 && ch == 'm' && cptr[ 1 ] == 'i' && cptr[ 2 ] == 'l'
                        && !isalnum( cptr[ 3 ] ) )
                {
                    m_token.pos += 3;
                    return Unit::Mil;
                }

                if( sizeLeft >= 4 && ch == 't' && cptr[ 1 ] == 'h' && cptr[ 2 ] == 'o'
                        && cptr[ 3 ] == 'u' && !isalnum( cptr[ 4 ] ) )
                {
                    m_token.pos += 4;
                    return Unit::Mil;
                }

                if( m_defaultUnits == Unit::SI && sizeLeft >= 1
                        && isOldSchoolDecimalSeparator( ch, siScaler ) )
                {
                    m_token.pos++;
                    return Unit::SI;
                }

                return Unit::Invalid;
            };

    char ch;

    // Start processing of first/next token: Remove whitespace
    for( ;; )
    {
        ch = m_token.input[ m_token.pos ];

        if( ch == ' ' )
            m_token.pos++;
        else
            break;
    }

    double siScaler = 1.0;
    Unit   convertFrom = Unit::Invalid;

    if( ch == 0 )
    {
        /* End of input */
    }
    else if( isdigit( (unsigned char) ch ) || isDecimalSeparator( ch ) )
    {
        // VALUE
        extractNumber( &siScaler );
        retval.token = VALUE;
        retval.value.dValue = atof( m_token.token ) * siScaler;
    }
    else if( ( convertFrom = checkUnit( &siScaler ) ) != Unit::Invalid )
    {
        // UNIT
        // Units are appended to a VALUE.
        // Determine factor to default unit if unit for value is given.
        // Example: Default is mm, unit is inch: factor is 25.4
        // The factor is assigned to the terminal UNIT. The actual
        // conversion is done within a parser action.
        retval.token = UNIT;

        if( m_defaultUnits == Unit::MM )
        {
            switch( convertFrom )
            {
            case Unit::Inch: retval.value.dValue = 25.4;          break;
            case Unit::Mil:  retval.value.dValue = 25.4 / 1000.0; break;
            case Unit::UM:   retval.value.dValue = 1 / 1000.0;    break;
            case Unit::MM:   retval.value.dValue = 1.0;           break;
            case Unit::CM:   retval.value.dValue = 10.0;          break;
            default:
            case Unit::Invalid:                                   break;
            }
        }
        else if( m_defaultUnits == Unit::Inch )
        {
            switch( convertFrom )
            {
            case Unit::Inch: retval.value.dValue = 1.0;           break;
            case Unit::Mil:  retval.value.dValue = 1.0 / 1000.0;  break;
            case Unit::UM:   retval.value.dValue = 1.0 / 25400.0; break;
            case Unit::MM:   retval.value.dValue = 1.0 / 25.4;    break;
            case Unit::CM:   retval.value.dValue = 1.0 / 2.54;    break;
            default:
            case Unit::Invalid:                                   break;
            }
        }
        else if( m_defaultUnits == Unit::Mil )
        {
            switch( convertFrom )
            {
            case Unit::Inch: retval.value.dValue = 1.0 * 1000.0;   break;
            case Unit::Mil:  retval.value.dValue = 1.0;            break;
            case Unit::UM:   retval.value.dValue = 1.0 / 25.4;     break;
            case Unit::MM:   retval.value.dValue = 1000.0 / 25.4;  break;
            case Unit::CM:   retval.value.dValue = 1000.0 / 2.54;  break;
            default:
            case Unit::Invalid:                                   break;
            }
        }
        else if( m_defaultUnits == Unit::Degrees && convertFrom == Unit::Degrees )
        {
            retval.value.dValue = 1.0;
        }
        else if( m_defaultUnits == Unit::SI )
        {
            retval.value.dValue = siScaler;
        }
    }
    else if( isalpha( ch ) )
    {
        // VAR
        const char* cptr = &m_token.input[ m_token.pos ];
        cptr++;

        while( isalnum( *cptr ) )
            cptr++;

        retval.token = VAR;
        size_t bytesToCopy = cptr - &m_token.input[ m_token.pos ];

        if( bytesToCopy >= sizeof( retval.value.text ) )
            bytesToCopy = sizeof( retval.value.text ) - 1;

        strncpy( retval.value.text, &m_token.input[ m_token.pos ], bytesToCopy );
        retval.value.text[ bytesToCopy ] = 0;
        m_token.pos += cptr - &m_token.input[ m_token.pos ];
    }
    else
    {
        // Single char tokens
        switch( ch )
        {
        case '+': retval.token = PLUS;   break;
        case '-': retval.token = MINUS;  break;
        case '*': retval.token = MULT;   break;
        case '/': retval.token = DIVIDE; break;
        case '(': retval.token = PARENL; break;
        case ')': retval.token = PARENR; break;
        case '=': retval.token = ASSIGN; break;
        case ';': retval.token = SEMCOL; break;
        default:  m_parseError = true;   break;   /* invalid character */
        }

        m_token.pos++;
    }

    if( !m_parseError )
        retval.value.valid = true;

    return retval;
}

void NUMERIC_EVALUATOR::SetVar( const wxString& aString, double aValue )
{
    m_varMap[ aString ] = aValue;
}

double NUMERIC_EVALUATOR::GetVar( const wxString& aString )
{
    auto it = m_varMap.find( aString );

    if( it != m_varMap.end() )
    {
        return it->second;
    }
    else
    {
        m_parseError = true;
        return 0.0;
    }
}
