/*
    This file is part of libeval, a simple math expression evaluator

    Copyright (C) 2017 Michael Geselbracht, mgeselbracht3@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <set>
#include <vector>

#ifdef DEBUG
#include <stdarg.h>
#endif

#include "libeval_compiler.h"

/* The (generated) lemon parser is written in C.
 * In order to keep its symbol from the global namespace include the parser code with
 * a C++ namespace.
 */
namespace LIBEVAL
{

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#include "grammar.c"
#include "grammar.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

static void libeval_dbg( const char* fmt, ... )
{
#ifdef DEBUG
    va_list ap;
    va_start( ap, fmt );
    fprintf( stderr, "libeval: " );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
#endif
}

static const std::string formatOpName( int op )
{
    static const struct
    {
        int         op;
        std::string mnemonic;
    } simpleOps[] = { { TR_OP_MUL, "MUL" }, { TR_OP_DIV, "DIV" }, { TR_OP_ADD, "ADD" },
        { TR_OP_SUB, "SUB" }, { TR_OP_LESS, "LESS" }, { TR_OP_GREATER, "GREATER" },
        { TR_OP_LESS_EQUAL, "LESS_EQUAL" }, { TR_OP_GREATER_EQUAL, "GREATER_EQUAL" },
        { TR_OP_EQUAL, "EQUAL" }, { TR_OP_NOT_EQUAL, "NEQUAL" }, { TR_OP_BOOL_AND, "AND" },
        { TR_OP_BOOL_OR, "OR" }, { TR_OP_BOOL_NOT, "NOT" }, { -1, "" } };

    for( int i = 0; simpleOps[i].op >= 0; i++ )
    {
        if( simpleOps[i].op == op )
        {
            return simpleOps[i].mnemonic;
        }
    }

    return "???";
}


std::string UCODE::UOP::Format() const
{
    char str[1024];

    switch( m_op )
    {
    case TR_UOP_PUSH_VAR:
        sprintf( str, "PUSH VAR [%p]", m_arg );
        break;
    case TR_UOP_PUSH_VALUE:
    {
        auto val = reinterpret_cast<VALUE*>( m_arg );
        if( val->GetType() == VT_NUMERIC )
            sprintf( str, "PUSH NUM [%.10f]", val->AsDouble() );
        else
            sprintf( str, "PUSH STR [%s]", val->AsString().c_str() );
        break;
    }
    default:
        sprintf( str, "%s", formatOpName( m_op ).c_str() );
        break;
    }
    return str;
}

std::string UCODE::Dump() const
{
    std::string rv;
    for( auto op : m_ucode )
    {
        rv += op->Format();
        rv += "\n";
    }
    return rv;
};

std::string TOKENIZER::GetChars( std::function<bool( int )> cond ) const
{
    std::string rv;
    size_t      p = m_pos;
    //   printf("p %d len %d\n", p, str.length() );
    while( p < m_str.length() && cond( m_str[p] ) )
    {
        rv.append( 1, m_str[p] );
        p++;
    }
    return rv;
}

bool TOKENIZER::MatchAhead( std::string match, std::function<bool( int )> stopCond ) const
{
    int remaining = m_str.length() - m_pos;
    if( remaining < (int) match.length() )
        return false;

    if( m_str.substr( m_pos, match.length() ) == match )
    {
        return ( remaining == (int) match.length() || stopCond( m_str[m_pos + match.length()] ) );
    }
    return false;
}


COMPILER::COMPILER()
{
    m_localeDecimalSeparator = '.';
    m_parseError             = false;
    m_parseFinished          = false;
    m_unitResolver.reset( new UNIT_RESOLVER );
    m_parser = LIBEVAL::ParseAlloc( malloc );
}


COMPILER::~COMPILER()
{
    LIBEVAL::ParseFree( m_parser, free );

    // Allow explicit call to destructor
    m_parser = nullptr;

    Clear();
}


void COMPILER::Clear()
{
    //free( current.token );
    m_tokenizer.Clear();
    m_parseError = true;
}


void COMPILER::parseError( const char* s )
{
    m_parseError = true;
}


void COMPILER::parseOk()
{
    m_parseFinished = true;
}


bool COMPILER::Compile( const std::string& aString, UCODE* aCode )
{
    // Feed parser token after token until end of input.

    newString( aString );
    m_tree          = nullptr;
    m_parseError    = false;
    m_parseFinished = false;
    T_TOKEN tok;

    libeval_dbg( "str: '%s' empty: %d\n", aString.c_str(), !!aString.empty() );

    if( aString.empty() )
    {
        m_parseFinished = true;
        return generateUCode( aCode );
    }

    do
    {
        tok = getToken();
        libeval_dbg( "parse: tok %d\n", tok.token );
        Parse( m_parser, tok.token, tok.value, this );
        //printf('error')
        if( m_parseError )
        {
            //printf( "PARSE ERR\n" );
            m_parseErrorToken = "";
            m_parseErrorPos   = m_tokenizer.GetPos();
            return false;
        }

        if( m_parseFinished || tok.token == G_ENDS )
        {
            // Reset parser by passing zero as token ID, value is ignored.
            Parse( m_parser, 0, tok.value, this );
            break;
        }
    } while( tok.token );

    return generateUCode( aCode );
}


void COMPILER::newString( const std::string& aString )
{
    Clear();

    m_lexerState = LS_DEFAULT;
    m_tokenizer.Restart( aString );
    m_parseFinished = false;
}

COMPILER::T_TOKEN COMPILER::getToken()
{
    T_TOKEN rv;
    bool    done = false;
    do
    {

        switch( m_lexerState )
        {
        case LS_DEFAULT:
            done = lexDefault( rv );
            break;
        case LS_STRING:
            done = lexString( rv );
            break;
        }
        //printf( "-> lstate %d done %d\n", m_lexerState, !!done );
    } while( !done );

    return rv;
}


bool COMPILER::lexString( COMPILER::T_TOKEN& aToken )
{
    auto str = m_tokenizer.GetChars( []( int c ) -> bool { return c != '"'; } );
    //printf("STR LIT '%s'\n", (const char *)str.c_str() );

    aToken.token = G_STRING;
    strcpy( aToken.value.value.str, str.c_str() );

    m_tokenizer.NextChar( str.length() + 1 );
    m_lexerState = LS_DEFAULT;
    return true;
}


int COMPILER::resolveUnits()
{
    int unitId = 0;
    for( auto unitName : m_unitResolver->GetSupportedUnits() )
    {
        if( m_tokenizer.MatchAhead( unitName, []( int c ) -> bool { return !isalnum( c ); } ) )
        {
            libeval_dbg( "Match unit '%s'\n", unitName.c_str() );
            m_tokenizer.NextChar( unitName.length() );
            return unitId;
        }

        unitId++;
    }

    return -1;
}

bool COMPILER::lexDefault( COMPILER::T_TOKEN& aToken )
{
    T_TOKEN     retval;
    std::string current;
    size_t      idx;
    int         convertFrom;

    retval.token = G_ENDS;

    //printf( "tokdone %d\n", !!m_tokenizer.Done() );
    if( m_tokenizer.Done() )
    {
        aToken = retval;
        return true;
    }

    auto isDecimalSeparator = [&]( char ch ) -> bool {
        return ( ch == m_localeDecimalSeparator || ch == '.' || ch == ',' );
    };

    // Lambda: get value as string, store into clToken.token and update current index.
    auto extractNumber = [&]() {
        bool haveSeparator = false;
        idx                = 0;
        auto ch            = m_tokenizer.GetChar();

        do
        {
            if( isDecimalSeparator( ch ) && haveSeparator )
                break;

            current.append( 1, ch );

            if( isDecimalSeparator( ch ) )
                haveSeparator = true;

            m_tokenizer.NextChar();
            ch = m_tokenizer.GetChar();
        } while( isdigit( ch ) || isDecimalSeparator( ch ) );

        // Ensure that the systems decimal separator is used
        for( int i = current.length(); i; i-- )
            if( isDecimalSeparator( current[i - 1] ) )
                current[i - 1] = m_localeDecimalSeparator;


        //printf("-> NUM: '%s'\n", (const char *) current.c_str() );
    };


    int ch;

    // Start processing of first/next token: Remove whitespace
    for( ;; )
    {
        ch = m_tokenizer.GetChar();

        if( ch == ' ' )
            m_tokenizer.NextChar();
        else
            break;
    }

    libeval_dbg( "LEX ch '%c' pos %d\n", ch, m_tokenizer.GetPos() );

    if( ch == 0 )
    {
        /* End of input */
    }
    else if( isdigit( ch ) )
    {
        // VALUE
        extractNumber();
        retval.token = G_VALUE;
        strcpy( retval.value.value.str, current.c_str() );
    }
    else if( ( convertFrom = resolveUnits() ) >= 0 )
    {
        //printf("unit\n");
        // UNIT
        // Units are appended to a VALUE.
        // Determine factor to default unit if unit for value is given.
        // Example: Default is mm, unit is inch: factor is 25.4
        // The factor is assigned to the terminal UNIT. The actual
        // conversion is done within a parser action.
        retval.token            = G_UNIT;
        retval.value.value.type = convertFrom;
    }
    else if( ch == '\"' ) // string literal
    {
        //printf( "MATCH STRING LITERAL\n" );
        m_lexerState = LS_STRING;
        m_tokenizer.NextChar();
        return false;
    }
    else if( isalpha( ch ) )
    {
        //printf("ALPHA\n");
        current = m_tokenizer.GetChars( []( int c ) -> bool { return isalnum( c ); } );
        //printf("Len: %d\n", current.length() );
        //printf("id '%s'\n", (const char *) current.c_str() );
        fflush( stdout );
        retval.token = G_IDENTIFIER;
        strcpy( retval.value.value.str, current.c_str() );
        m_tokenizer.NextChar( current.length() );
    }
    else if( m_tokenizer.MatchAhead( "==", []( int c ) -> bool { return c != '='; } ) )
    {
        retval.token = G_EQUAL;
        m_tokenizer.NextChar( 2 );
        //printf( "nc pos %d\n", m_tokenizer.GetPos() );
    }
    else if( m_tokenizer.MatchAhead( "<=", []( int c ) -> bool { return c != '='; } ) )
    {
        retval.token = G_LESS_EQUAL_THAN;
        m_tokenizer.NextChar( 2 );
    }
    else if( m_tokenizer.MatchAhead( ">=", []( int c ) -> bool { return c != '='; } ) )
    {
        retval.token = G_GREATER_EQUAL_THAN;
        m_tokenizer.NextChar( 2 );
    }
    else if( m_tokenizer.MatchAhead( "&&", []( int c ) -> bool { return c != '&'; } ) )
    {
        retval.token = G_BOOL_AND;
        m_tokenizer.NextChar( 2 );
    }
    else if( m_tokenizer.MatchAhead( "||", []( int c ) -> bool { return c != '|'; } ) )
    {
        retval.token = G_BOOL_OR;
        m_tokenizer.NextChar( 2 );
    }
    else
    {
        //printf( "WTF: '%c'\n", ch );

        // Single char tokens
        switch( ch )
        {
        case '+':
            retval.token = G_PLUS;
            break;
        case '!':
            retval.token = G_BOOL_NOT;
            break;
        case '-':
            retval.token = G_MINUS;
            break;
        case '*':
            retval.token = G_MULT;
            break;
        case '/':
            retval.token = G_DIVIDE;
            break;
        case '<':
            retval.token = G_LESS_THAN;
            break;
        case '>':
            retval.token = G_GREATER_THAN;
            break;
        case '(':
            retval.token = G_PARENL;
            break;
        case ')':
            retval.token = G_PARENR;
            break;
        case ';':
            retval.token = G_SEMCOL;
            break;
        case '.':
            retval.token = G_STRUCT_REF;
            break;
        default:
            m_parseError = true;
            break; /* invalid character */
        }

        m_tokenizer.NextChar();
    }

    aToken = retval;
    return true;
}

const std::string formatNode( TREE_NODE* tok )
{
    char str[1024];
    //   printf("fmt tok %p v %p ", tok, tok->value.v );
    fflush( stdout );
    sprintf( str, "%s", (const char*) tok->value.str );
    return str;
}


void dumpNode( std::string& buf, TREE_NODE* tok, int depth = 0 )
{
    char str[1024];
    sprintf( str, "\n[%p] ", tok ); //[tok %p] ", tok);
    buf += str;
    for( int i = 0; i < 2 * depth; i++ )
        buf += "  ";

    if( tok->op & TR_OP_BINARY_MASK )
    {
        sprintf( str, "%s", (const char*) formatOpName( tok->op ).c_str() );
        buf += str;
        dumpNode( buf, tok->leaf[0], depth + 1 );
        dumpNode( buf, tok->leaf[1], depth + 1 );
    }

    switch( tok->op )
    {
    case TR_NUMBER:
        sprintf( str, "NUMERIC: " );
        buf += str;
        sprintf( str, "%s", formatNode( tok ).c_str() );
        buf += str;
        if( tok->leaf[0] )
            dumpNode( buf, tok->leaf[0], depth + 1 );
        break;
    case TR_STRING:
        sprintf( str, "STRING: " );
        buf += str;
        sprintf( str, "%s", formatNode( tok ).c_str() );
        buf += str;
        break;
    case TR_IDENTIFIER:
        sprintf( str, "ID: " );
        buf += str;
        sprintf( str, "%s", formatNode( tok ).c_str() );
        buf += str;
        break;
    case TR_STRUCT_REF:
        sprintf( str, "SREF: " );
        buf += str;
        dumpNode( buf, tok->leaf[0], depth + 1 );
        dumpNode( buf, tok->leaf[1], depth + 1 );
        break;
    case TR_UNIT:
        sprintf( str, "UNIT: %d ", tok->value.type );
        buf += str;
        break;
    }
}

void COMPILER::setRoot( TREE_NODE root )
{
    m_tree = copyNode( root );
}


bool COMPILER::generateUCode( UCODE* aCode )
{
    std::vector<TREE_NODE*> stack;
    std::set<TREE_NODE*>    visitedNodes;

    auto visited = [&]( TREE_NODE* node ) -> bool {
        return visitedNodes.find( node ) != visitedNodes.end();
    };

    UCODE code;

    assert( m_tree );

    stack.push_back( m_tree );

    //printf("compile: tree %p\n", m_tree);

    while( !stack.empty() )
    {
        auto node           = stack.back();
        bool isTerminalNode = true;

        //   printf( "process node %p [op %d] [stack %d]\n", node, node->op, stack.size() );

        // process terminal nodes first
        switch( node->op )
        {
        case TR_STRUCT_REF:
        {
            assert( node->leaf[0]->op == TR_IDENTIFIER );
            assert( node->leaf[1]->op == TR_IDENTIFIER );

            auto vref = aCode->createVarRef( node->leaf[0]->value.str, node->leaf[1]->value.str );
            aCode->AddOp( TR_UOP_PUSH_VAR, vref );
            break;
        }

        case TR_NUMBER:
        {
            auto son = node->leaf[0];

            double value = atof( node->value.str ); // fixme: locale

            if( son && son->op == TR_UNIT )
            {
                //printf( "HandleUnit: %s unit %d\n", node->value.str, son->value.type );

                value = m_unitResolver->Convert( node->value.str, son->value.type );
                visitedNodes.insert( son );
            }

            aCode->AddOp( TR_UOP_PUSH_VALUE, value );

            break;
        }
        case TR_STRING:
        {
            aCode->AddOp( TR_UOP_PUSH_VALUE, node->value.str );
            break;
        }
        default:
            isTerminalNode = false;
            break;
        }

        if( isTerminalNode )
        {
            visitedNodes.insert( node );
            stack.pop_back();
            continue;
        }

        if( node->leaf[0] && !visited( node->leaf[0] ) )
        {
            stack.push_back( node->leaf[0] );
        }
        else if( node->leaf[1] && !visited( node->leaf[1] ) )
        {
            stack.push_back( node->leaf[1] );
        }
        else
        {
            aCode->AddOp( node->op );
            visitedNodes.insert( node );
            stack.pop_back();
        }
    }


    return true;
}


void UCODE::UOP::Exec( CONTEXT* ctx, UCODE* ucode )
{

    switch( m_op )
    {
    case TR_UOP_PUSH_VAR:
    {
        auto value = ctx->AllocValue();
        value->Set( reinterpret_cast<VAR_REF*>( m_arg )->GetValue( ucode ) );
        ctx->Push( value );
        break;
    }
    case TR_UOP_PUSH_VALUE:
        ctx->Push( reinterpret_cast<VALUE*>( m_arg ) );
        return;
    default:
        break;
    }

    if( m_op & TR_OP_BINARY_MASK )
    {
        auto   arg2 = ctx->Pop();
        auto   arg1 = ctx->Pop();
        double result;

        switch( m_op )
        {
        case TR_OP_ADD:
            result = arg1->AsDouble() + arg2->AsDouble();
            break;
        case TR_OP_SUB:
            result = arg1->AsDouble() - arg2->AsDouble();
            break;
        case TR_OP_MUL:
            result = arg1->AsDouble() * arg2->AsDouble();
            break;
        case TR_OP_DIV:
            result = arg1->AsDouble() / arg2->AsDouble();
            break;
        case TR_OP_LESS_EQUAL:
            result = arg1->AsDouble() <= arg2->AsDouble() ? 1 : 0;
            break;
        case TR_OP_GREATER_EQUAL:
            result = arg1->AsDouble() >= arg2->AsDouble() ? 1 : 0;
            break;
        case TR_OP_LESS:
            result = arg1->AsDouble() < arg2->AsDouble() ? 1 : 0;
            break;
        case TR_OP_GREATER:
            result = arg1->AsDouble() > arg2->AsDouble() ? 1 : 0;
            break;
        case TR_OP_EQUAL:
            result = arg1->EqualTo( arg2 ) ? 1 : 0;
            break;
        case TR_OP_NOT_EQUAL:
            result = arg1->EqualTo( arg2 ) ? 0 : 1;
            break;
        case TR_OP_BOOL_AND:
            result = ( ( arg1->AsDouble() != 0.0 ? true : false )
                             && ( arg2->AsDouble() != 0.0 ? true : false ) ) ?
                             1 :
                             0;
            break;
        case TR_OP_BOOL_OR:
            result = ( ( arg1->AsDouble() != 0.0 ? true : false )
                             || ( arg2->AsDouble() != 0.0 ? true : false ) ) ?
                             1 :
                             0;
            break;
        default:
            result = 0.0;
            break;
        }

        auto rp = ctx->AllocValue();
        rp->Set( result );
        ctx->Push( rp );
        return;
    }
    else if( m_op & TR_OP_UNARY_MASK )
    {
        // fixme : not operator
    }
}

VALUE* UCODE::Run()
{
    CONTEXT ctx;
    for( const auto op : m_ucode )
        op->Exec( &ctx, this );

    assert( ctx.SP() == 1 );
    return ctx.Pop();
}

} // namespace LIBEVAL
