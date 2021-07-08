/*
 * This file is part of libeval, a simple math expression evaluator
 *
 * Copyright (C) 2017 Michael Geselbracht, mgeselbracht3@gmail.com
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kicad_string.h>
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
    struct lconv* lc = localeconv();
    m_localeDecimalSeparator = *lc->decimal_point;

    m_parseError = false;
    m_parseFinished = false;

    m_parser = numEval::ParseAlloc( malloc );

    switch( aUnits )
    {
    case EDA_UNITS::MILLIMETRES: m_defaultUnits = Unit::MM;   break;
    case EDA_UNITS::MILS:        m_defaultUnits = Unit::Mil;  break;
    case EDA_UNITS::INCHES:      m_defaultUnits = Unit::Inch; break;
    default:                     m_defaultUnits = Unit::MM;   break;
    }
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
    free( m_token.token );
    m_token.token = nullptr;
    m_token.input = nullptr;
    m_parseError = true;
    m_originalText = wxEmptyString;
}


void NUMERIC_EVALUATOR::parseError( const char* s )
{
    m_parseError = true;
}


void NUMERIC_EVALUATOR::parseOk()
{
    m_parseFinished = true;
}


void NUMERIC_EVALUATOR::parseSetResult( double val )
{
    if( std::isnan( val ) )
    {
        // Naively printing this with %g produces "nan" on some platforms
        // and "-nan(ind)" on others (e.g. MSVC). So force a "standard" string.
        snprintf( m_token.token, m_token.OutLen, "%s", "NaN" );
    }
    else
    {
        // Can be printed as a floating point
        // Warning: DO NOT use a format like %f or %g, because they can create issues.
        // especially %g can generate an exponent, incompatible with UNIT_BINDER
        // Use the optimized Double2Str
        snprintf( m_token.token, m_token.OutLen, "%s", Double2Str( val ).c_str() );
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

    m_token.token = reinterpret_cast<decltype( m_token.token )>( malloc( TokenStat::OutLen + 1 ) );
    strcpy( m_token.token, "0" );
    m_token.inputLen = aString.length();
    m_token.pos = 0;
    m_token.input = aString.mb_str();

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

    auto isDecimalSeparator = [ & ]( char ch ) -> bool {
        return ( ch == m_localeDecimalSeparator || ch == '.' || ch == ',' );
    };

    // Lambda: get value as string, store into clToken.token and update current index.
    auto extractNumber = [ & ]() {
        bool haveSeparator = false;
        idx = 0;
        auto ch = m_token.input[ m_token.pos ];

        do
        {
            if( isDecimalSeparator( ch ) && haveSeparator )
                break;

            m_token.token[ idx++ ] = ch;

            if( isDecimalSeparator( ch ) )
                haveSeparator = true;

            ch = m_token.input[ ++m_token.pos ];
        } while( isdigit( ch ) || isDecimalSeparator( ch ) );

        m_token.token[ idx ] = 0;

        // Ensure that the systems decimal separator is used
        for( int i = strlen( m_token.token ); i; i-- )
        {
            if( isDecimalSeparator( m_token.token[ i - 1 ] ) )
                m_token.token[ i - 1 ] = m_localeDecimalSeparator;
        }
    };

    // Lamda: Get unit for current token.
    // Valid units are ", in, mm, mil and thou.  Returns Unit::Invalid otherwise.
    auto checkUnit = [ this ]() -> Unit {
        char ch = m_token.input[ m_token.pos ];

        if( ch == '"' )
        {
            m_token.pos++;
            return Unit::Inch;
        }

        // Do not use strcasecmp() as it is not available on all platforms
        const char* cptr = &m_token.input[ m_token.pos ];
        const auto sizeLeft = m_token.inputLen - m_token.pos;

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

    Unit convertFrom;

    if( ch == 0 )
    {
        /* End of input */
    }
    else if( isdigit( ch ) || isDecimalSeparator( ch ))
    {
        // VALUE
        extractNumber();
        retval.token = VALUE;
        retval.value.dValue = atof( m_token.token );
    }
    else if(( convertFrom = checkUnit()) != Unit::Invalid )
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
            case Unit::MM:   retval.value.dValue = 1.0;           break;
            case Unit::CM:   retval.value.dValue = 10.0;          break;
            case Unit::Invalid:                                   break;
            }
        }
        else if( m_defaultUnits == Unit::Inch )
        {
            switch( convertFrom )
            {
            case Unit::Inch: retval.value.dValue = 1.0;          break;
            case Unit::Mil:  retval.value.dValue = 1.0 / 1000.0; break;
            case Unit::MM:   retval.value.dValue = 1.0 / 25.4;   break;
            case Unit::CM:   retval.value.dValue = 1.0 / 2.54;   break;
            case Unit::Invalid:                                  break;
            }
        }
        else if( m_defaultUnits == Unit::Mil )
        {
            switch( convertFrom )
            {
            case Unit::Inch: retval.value.dValue = 1.0 * 1000.0;  break;
            case Unit::Mil:  retval.value.dValue = 1.0;           break;
            case Unit::MM:   retval.value.dValue = 1000.0 / 25.4; break;
            case Unit::CM:   retval.value.dValue = 1000.0 / 2.54; break;
            case Unit::Invalid:                                   break;
            }
        }
    }
    else if( isalpha( ch ))
    {
        // VAR
        const char* cptr = &m_token.input[ m_token.pos ];
        cptr++;

        while( isalnum( *cptr ))
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
    if( m_varMap[ aString ] )
        return m_varMap[ aString ];
    else
        return 0.0;
}
