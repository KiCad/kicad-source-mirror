/*
    This file is part of libeval, a simple math expression evaluator

    Copyright (C) 2017 Michael Geselbracht, mgeselbracht3@gmail.com
    Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.

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

#include <memory>
#include <set>
#include <vector>

#ifdef DEBUG
#include <stdarg.h>
#endif

#include <reporter.h>
#include <libeval_compiler/libeval_compiler.h>

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

static void libeval_dbg( int level, const char* fmt, ... )
{
#ifdef DEBUG
    if(level < -10) // fixme: tom's debugging.
    {
        va_list ap;
        va_start( ap, fmt );
        fprintf( stderr, "libeval: " );
        vfprintf( stderr, fmt, ap );
        va_end( ap );
    }
#endif
}


static const std::string formatOpName( int op )
{
    static const struct
    {
        int         op;
        std::string mnemonic;
    }
    simpleOps[] =
    {
        { TR_OP_MUL, "MUL" }, { TR_OP_DIV, "DIV" }, { TR_OP_ADD, "ADD" },
        { TR_OP_SUB, "SUB" }, { TR_OP_LESS, "LESS" }, { TR_OP_GREATER, "GREATER" },
        { TR_OP_LESS_EQUAL, "LESS_EQUAL" }, { TR_OP_GREATER_EQUAL, "GREATER_EQUAL" },
        { TR_OP_EQUAL, "EQUAL" }, { TR_OP_NOT_EQUAL, "NEQUAL" }, { TR_OP_BOOL_AND, "AND" },
        { TR_OP_BOOL_OR, "OR" }, { TR_OP_BOOL_NOT, "NOT" }, { -1, "" }
    };

    for( int i = 0; simpleOps[i].op >= 0; i++ )
    {
        if( simpleOps[i].op == op )
            return simpleOps[i].mnemonic;
    }

    return "???";
}


std::string UOP::Format() const
{
    char str[LIBEVAL_MAX_LITERAL_LENGTH];

    switch( m_op )
    {
    case TR_UOP_PUSH_VAR:
        snprintf( str, LIBEVAL_MAX_LITERAL_LENGTH, "PUSH VAR [%p]", m_arg );
        break;

    case TR_UOP_PUSH_VALUE:
    {
        VALUE* val = reinterpret_cast<VALUE*>( m_arg );

        if( !val )
            snprintf( str, LIBEVAL_MAX_LITERAL_LENGTH, "PUSH nullptr" );
        else if( val->GetType() == VT_NUMERIC )
            snprintf( str, LIBEVAL_MAX_LITERAL_LENGTH, "PUSH NUM [%.10f]", val->AsDouble() );
        else
            snprintf( str, LIBEVAL_MAX_LITERAL_LENGTH, "PUSH STR [%ls]", GetChars( val->AsString() ) );
    }
        break;

    case TR_OP_METHOD_CALL:
        snprintf( str, LIBEVAL_MAX_LITERAL_LENGTH, "MCALL" );
        break;

    case TR_OP_FUNC_CALL:
        snprintf( str, LIBEVAL_MAX_LITERAL_LENGTH, "FCALL" );
        break;

    default:
        snprintf( str, LIBEVAL_MAX_LITERAL_LENGTH, "%s %d", formatOpName( m_op ).c_str(), m_op );
        break;
    }

    return str;
}


UCODE::~UCODE()
{
    for ( auto op : m_ucode )
        delete op;
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

bool TOKENIZER::MatchAhead( const std::string& match, std::function<bool( int )> stopCond ) const
{
    int remaining = m_str.length() - m_pos;

    if( remaining < (int) match.length() )
        return false;

    if( m_str.substr( m_pos, match.length() ) == match )
        return ( remaining == (int) match.length() || stopCond( m_str[m_pos + match.length()] ) );

    return false;
}


COMPILER::COMPILER( REPORTER* aReporter, int aSourceLine, int aSourceOffset ) :
        m_lexerState( COMPILER::LS_DEFAULT ),
        m_reporter( aReporter ),
        m_originLine( aSourceLine ),
        m_originOffset( aSourceOffset )
{
    m_localeDecimalSeparator = '.';
    m_sourcePos = 0;
    m_parseFinished = false;
    m_unitResolver = std::make_unique<UNIT_RESOLVER>();
    m_parser = LIBEVAL::ParseAlloc( malloc );
    m_tree = nullptr;
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

    if( m_tree )
    {
        freeTree( m_tree );
    }

    m_tree = nullptr;
}


void COMPILER::parseError( const char* s )
{
    reportError( s );
}


void COMPILER::parseOk()
{
    m_parseFinished = true;
}


bool COMPILER::Compile( const std::string& aString, UCODE* aCode, CONTEXT* aPreflightContext )
{
    // Feed parser token after token until end of input.

    newString( aString );

    if( m_tree )
    {
        freeTree( m_tree );
    }

    m_tree = nullptr;
    m_parseFinished = false;
    T_TOKEN tok;

    libeval_dbg(0, "str: '%s' empty: %d\n", aString.c_str(), !!aString.empty() );

    if( aString.empty() )
    {
        m_parseFinished = true;
        return generateUCode( aCode, aPreflightContext );
    }

    do
    {
        m_sourcePos = m_tokenizer.GetPos();

        tok = getToken();
        libeval_dbg(10, "parse: tok %d\n", tok.token );
        Parse( m_parser, tok.token, tok.value, this );

        if( m_parseFinished || tok.token == G_ENDS )
        {
            // Reset parser by passing zero as token ID, value is ignored.
            Parse( m_parser, 0, tok.value, this );
            break;
        }
    } while( tok.token );

    return generateUCode( aCode, aPreflightContext );
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
    std::string str = m_tokenizer.GetChars( []( int c ) -> bool { return c != '\''; } );
    //printf("STR LIT '%s'\n", (const char *)str.c_str() );

    aToken.token = G_STRING;
    snprintf( aToken.value.value.str, LIBEVAL_MAX_LITERAL_LENGTH, "%s", str.c_str() );

    m_tokenizer.NextChar( str.length() + 1 );
    m_lexerState = LS_DEFAULT;
    return true;
}


int COMPILER::resolveUnits()
{
    int unitId = 0;

    for( const std::string& unitName : m_unitResolver->GetSupportedUnits() )
    {
        if( m_tokenizer.MatchAhead( unitName, []( int c ) -> bool { return !isalnum( c ); } ) )
        {
            libeval_dbg(10, "Match unit '%s'\n", unitName.c_str() );
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
    int         convertFrom;
    wxString    msg;

    retval.token = G_ENDS;

    //printf( "tokdone %d\n", !!m_tokenizer.Done() );
    if( m_tokenizer.Done() )
    {
        aToken = retval;
        return true;
    }

    auto isDecimalSeparator =
            [&]( char ch ) -> bool
            {
                return ( ch == m_localeDecimalSeparator || ch == '.' || ch == ',' );
            };

    // Lambda: get value as string, store into clToken.token and update current index.
    auto extractNumber =
            [&]()
            {
                bool haveSeparator = false;
                int ch             = m_tokenizer.GetChar();

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
                {
                    if( isDecimalSeparator( current[i - 1] ) )
                        current[i - 1] = m_localeDecimalSeparator;
                }

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

    libeval_dbg(10, "LEX ch '%c' pos %lu\n", ch, (unsigned long)m_tokenizer.GetPos() );

    if( ch == 0 )
    {
        /* End of input */
    }
    else if( isdigit( ch ) )
    {
        // VALUE
        extractNumber();
        retval.token = G_VALUE;
        snprintf( retval.value.value.str, LIBEVAL_MAX_LITERAL_LENGTH, "%s", current.c_str() );
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
    else if( ch == '\'' ) // string literal
    {
        //printf( "MATCH STRING LITERAL\n" );
        m_lexerState = LS_STRING;
        m_tokenizer.NextChar();
        return false;
    }
    else if( isalpha( ch ) || ch == '_' )
    {
        //printf("ALPHA\n");
        current = m_tokenizer.GetChars( []( int c ) -> bool { return isalnum( c ) || c == '_'; } );
        //printf("Len: %d\n", current.length() );
        //printf("id '%s'\n", (const char *) current.c_str() );
        //fflush( stdout );
        retval.token = G_IDENTIFIER;
        snprintf( retval.value.value.str, LIBEVAL_MAX_LITERAL_LENGTH, "%s", current.c_str() );
        m_tokenizer.NextChar( current.length() );
    }
    else if( m_tokenizer.MatchAhead( "==", []( int c ) -> bool { return c != '='; } ) )
    {
        retval.token = G_EQUAL;
        m_tokenizer.NextChar( 2 );
    }
    else if( m_tokenizer.MatchAhead( "!=", []( int c ) -> bool { return c != '='; } ) )
    {
        retval.token = G_NOT_EQUAL;
        m_tokenizer.NextChar( 2 );
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
        // Single char tokens
        switch( ch )
        {
        case '+': retval.token = G_PLUS;         break;
        case '!': retval.token = G_BOOL_NOT;     break;
        case '-': retval.token = G_MINUS;        break;
        case '*': retval.token = G_MULT;         break;
        case '/': retval.token = G_DIVIDE;       break;
        case '<': retval.token = G_LESS_THAN;    break;
        case '>': retval.token = G_GREATER_THAN; break;
        case '(': retval.token = G_PARENL;       break;
        case ')': retval.token = G_PARENR;       break;
        case ';': retval.token = G_SEMCOL;       break;
        case '.': retval.token = G_STRUCT_REF;   break;

        default:
            reportError( wxString::Format( _( "Unrecognized character '%c'" ), (char) ch ) );
            break;
        }

        m_tokenizer.NextChar();
    }

    aToken = retval;
    return true;
}


const std::string formatNode( TREE_NODE* tok )
{
    //   printf("fmt tok %p v %p ", tok, tok->value.v );
    //   fflush( stdout );

    char str[LIBEVAL_MAX_LITERAL_LENGTH];
    snprintf( str, LIBEVAL_MAX_LITERAL_LENGTH, "%s", (const char*) tok->value.str );
    return str;
}


void dumpNode( std::string& buf, TREE_NODE* tok, int depth = 0 )
{
    char str[LIBEVAL_MAX_LITERAL_LENGTH];
    snprintf( str, LIBEVAL_MAX_LITERAL_LENGTH, "\n[%p L0:%-20p L1:%-20p] ",
                                               tok,
                                               tok->leaf[0],
                                               tok->leaf[1] );
    buf += str;

    for( int i = 0; i < 2 * depth; i++ )
        buf += "  ";

    if( tok->op & TR_OP_BINARY_MASK )
    {
        buf += formatOpName( tok->op );
        dumpNode( buf, tok->leaf[0], depth + 1 );
        dumpNode( buf, tok->leaf[1], depth + 1 );
    }

    switch( tok->op )
    {
    case TR_NUMBER:
        buf += "NUMERIC: ";
        buf += formatNode( tok );

        if( tok->leaf[0] )
            dumpNode( buf, tok->leaf[0], depth + 1 );

        break;

    case TR_STRING:
        buf += "STRING: ";
        buf +=  formatNode( tok );
        break;

    case TR_IDENTIFIER:
        buf += "ID: ";
        buf += formatNode( tok );
        break;

    case TR_STRUCT_REF:
        buf += "SREF: ";
        dumpNode( buf, tok->leaf[0], depth + 1 );
        dumpNode( buf, tok->leaf[1], depth + 1 );
        break;

     case TR_OP_FUNC_CALL:
        buf += "CALL '";
        buf += tok->leaf[0]->value.str;
        buf += "': ";
        dumpNode( buf, tok->leaf[1], depth + 1 );
        break;

    case TR_UNIT:
        sprintf( str, "UNIT: %d ", tok->value.type );
        buf += str;
        break;
    }
}


void COMPILER::reportError( const wxString& aErrorMsg, int aPos )
{
    if( aPos == -1 )
        aPos = m_sourcePos;

    wxString rest;
    wxString first = aErrorMsg.BeforeFirst( '|', &rest );
    wxString msg = wxString::Format( _( "ERROR: <a href='%d:%d'>%s</a>%s" ),
                                     m_originLine,
                                     m_originOffset + aPos,
                                     first,
                                     rest );

    m_reporter->Report( msg, RPT_SEVERITY_ERROR );
}


void COMPILER::setRoot( TREE_NODE root )
{
    m_tree = copyNode( root );
}

void COMPILER::freeTree( LIBEVAL::TREE_NODE *tree )
{
    if ( tree->leaf[0] )
        freeTree( tree->leaf[0] );
    if ( tree->leaf[1] )
        freeTree( tree->leaf[1] );

    delete tree;
}

bool COMPILER::generateUCode( UCODE* aCode, CONTEXT* aPreflightContext )
{
    std::vector<TREE_NODE*> stack;
    std::set<TREE_NODE*>    visitedNodes;
    wxString                msg;

    auto visited = [&]( TREE_NODE* node ) -> bool
                   {
                       return visitedNodes.find( node ) != visitedNodes.end();
                   };

    assert( m_tree );

    stack.push_back( m_tree );

    std::string dump;

    dumpNode( dump, m_tree, 0 );
    libeval_dbg(3,"Tree dump:\n%s\n\n", dump.c_str() );

    while( !stack.empty() )
    {
        TREE_NODE* node = stack.back();

        libeval_dbg( 4, "process node %p [op %d] [stack %lu]\n",
                     node, node->op, (unsigned long)stack.size() );

        // process terminal nodes first
        switch( node->op )
        {
        case TR_OP_FUNC_CALL:
            break;

        case TR_STRUCT_REF:
        {
            assert( node->leaf[0]->op == TR_IDENTIFIER );
            //assert( node->leaf[1]->op == TR_IDENTIFIER );

            switch( node->leaf[1]->op )
            {
                case TR_IDENTIFIER:
                {
                    char*    itemName = node->leaf[0]->value.str;
                    char*    propName = node->leaf[1]->value.str;
                    VAR_REF* vref = aCode->CreateVarRef( itemName, propName );

                    if( !vref )
                    {
                        msg.Printf( _( "Unrecognized item '%s'" ), itemName );
                        reportError( msg, node->leaf[0]->srcPos - (int) strlen( itemName ) );
                        return false;
                    }

                    if( vref->GetType() == VT_PARSE_ERROR )
                    {
                        msg.Printf( _( "Unrecognized property '%s'" ), propName );
                        reportError( msg, node->leaf[1]->srcPos - (int) strlen( propName ) );
                        return false;
                    }

                    node->uop = makeUop( TR_UOP_PUSH_VAR, vref );
                    node->isTerminal = true;
                    break;
                }
                case TR_OP_FUNC_CALL:
                {
                    char*    itemName = node->leaf[0]->value.str;
                    VAR_REF* vref = aCode->CreateVarRef( itemName, "" );

                    if( !vref )
                    {
                        msg.Printf( _( "Unrecognized item '%s'" ), itemName );
                        reportError( msg, node->leaf[0]->srcPos - (int) strlen( itemName ) );
                        return false;
                    }

                    char* functionName = node->leaf[1]->leaf[0]->value.str;
                    auto  func = aCode->CreateFuncCall( functionName );

                    if( !func )
                    {
                        msg.Printf( _( "Unrecognized function '%s'" ), functionName );
                        reportError( msg, node->leaf[1]->leaf[0]->srcPos + 1 );
                        return false;
                    }

                    // Preflight the function call
                    VALUE*  param = aPreflightContext->AllocValue();
                    param->Set( node->value.str );
                    aPreflightContext->Push( param );

                    try
                    {
                        func( aPreflightContext, vref );
                        aPreflightContext->Pop();           // return value
                    }
                    catch( ... )
                    {
                    }

                    /* SREF -> FUNC_CALL -> leaf0/1 */
                    node->leaf[1]->leaf[0]->leaf[0] = nullptr;
                    node->leaf[1]->leaf[0]->leaf[1] = nullptr;

                    if( !aPreflightContext->GetError().IsEmpty() )
                    {
                        reportError( aPreflightContext->GetError(),
                                     node->leaf[1]->leaf[1]->srcPos
                                            - (int) strlen( node->value.str ) - 1 );
                        return false;
                    }

                    visitedNodes.insert( node->leaf[0] );
                    visitedNodes.insert( node->leaf[1]->leaf[0] );

                    node->uop = makeUop( TR_OP_METHOD_CALL, func, vref );
                    node->isTerminal = false;
                }
                    break;
                }
            }
            break;

        case TR_NUMBER:
        {
            int        units = 1;
            TREE_NODE* son = node->leaf[0];

            if( son && son->op == TR_UNIT )
            {
                //printf( "HandleUnit: %s unit %d\n", node->value.str, son->value.type );
                units = son->value.type;
                visitedNodes.insert( son );
            }

            double value = m_unitResolver->Convert( node->value.str, units );

            node->uop = makeUop( TR_UOP_PUSH_VALUE, value );
            node->isTerminal = true;

            break;
        }

        case TR_STRING:
        {
            node->uop = makeUop( TR_UOP_PUSH_VALUE, node->value.str );
            node->isTerminal = true;
            break;
        }

        case TR_IDENTIFIER:
        {
            VAR_REF* vref = aCode->CreateVarRef( node->value.str, "" );

            if( !vref )
            {
                msg.Printf( _( "Unrecognized item '%s'" ), node->value.str );
                reportError( msg, node->leaf[0]->srcPos - (int) strlen( node->value.str  ) );
                return false;
            }

            node->uop = makeUop( TR_UOP_PUSH_VALUE, vref );
            break;
        }

        default:
            node->uop = makeUop( node->op );
            break;
        }

        if( !node->isTerminal && node->leaf[0] && !visited( node->leaf[0] ) )
        {
            stack.push_back( node->leaf[0] );
            visitedNodes.insert( node->leaf[0] );
            continue;
        }
        else if( !node->isTerminal && node->leaf[1] && !visited( node->leaf[1] ) )
        {
            stack.push_back( node->leaf[1] );
            visitedNodes.insert( node->leaf[1] );
            continue;
        }

        visitedNodes.insert( node );

        if( node->uop )
            aCode->AddOp( node->uop );

        stack.pop_back();
    }

    libeval_dbg(2,"DUMp: \n%s\n", aCode->Dump().c_str() );

    return true;
}


void UOP::Exec( CONTEXT* ctx )
{
    switch( m_op )
    {
    case TR_UOP_PUSH_VAR:
    {
        auto value = ctx->AllocValue();
        value->Set( reinterpret_cast<VAR_REF*>( m_arg )->GetValue( ctx ) );
        ctx->Push( value );
    }
        break;

    case TR_UOP_PUSH_VALUE:
        ctx->Push( reinterpret_cast<VALUE*>( m_arg ) );
        return;

    case TR_OP_METHOD_CALL:
        //printf("CALL METHOD %s\n" );
        m_func( ctx, m_arg );
        return;

    default:
        break;
    }

    if( m_op & TR_OP_BINARY_MASK )
    {
        LIBEVAL::VALUE* arg2 = ctx->Pop();
        LIBEVAL::VALUE* arg1 = ctx->Pop();
        double          arg2Value = arg2 ? arg2->AsDouble() : 0.0;
        double          arg1Value = arg1 ? arg1->AsDouble() : 0.0;
        double          result;

        switch( m_op )
        {
        case TR_OP_ADD:
            result = arg1Value + arg2Value;
            break;
        case TR_OP_SUB:
            result = arg1Value - arg2Value;
            break;
        case TR_OP_MUL:
            result = arg1Value * arg2Value;
            break;
        case TR_OP_DIV:
            result = arg1Value / arg2Value;
            break;
        case TR_OP_LESS_EQUAL:
            result = arg1Value <= arg2Value ? 1 : 0;
            break;
        case TR_OP_GREATER_EQUAL:
            result = arg1Value >= arg2Value ? 1 : 0;
            break;
        case TR_OP_LESS:
            result = arg1Value < arg2Value ? 1 : 0;
            break;
        case TR_OP_GREATER:
            result = arg1Value > arg2Value ? 1 : 0;
            break;
        case TR_OP_EQUAL:
            result = arg1 && arg2 && arg1->EqualTo( arg2 ) ? 1 : 0;
            break;
        case TR_OP_NOT_EQUAL:
            result = arg1 && arg2 && arg1->EqualTo( arg2 ) ? 0 : 1;
            break;
        case TR_OP_BOOL_AND:
            result = arg1Value != 0.0 && arg2Value != 0.0 ? 1 : 0;
            break;
        case TR_OP_BOOL_OR:
            result = arg1Value != 0.0 || arg2Value != 0.0 ? 1 : 0;
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


VALUE* UCODE::Run( CONTEXT* ctx )
{
    for( UOP* op : m_ucode )
        op->Exec( ctx );

    assert( ctx->SP() == 1 );
    return ctx->Pop();
}


} // namespace LIBEVAL
