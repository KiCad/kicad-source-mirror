/*
 * This file is part of libeval, a simple math expression evaluator
 *
 * Copyright (C) 2017 Michael Geselbracht, mgeselbracht3@gmail.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>
#include <set>
#include <vector>
#include <algorithm>

#include <eda_units.h>
#include <string_utils.h>
#include <wx/log.h>

#ifdef DEBUG
#include <cstdarg>
#endif

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

#include <libeval_compiler/grammar.c>
#include <libeval_compiler/grammar.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif


#define libeval_dbg(level, fmt, ...) \
    wxLogTrace( "libeval_compiler", fmt, __VA_ARGS__ );


TREE_NODE* newNode( LIBEVAL::COMPILER* compiler, int op, const T_TOKEN_VALUE& value )
{
    TREE_NODE* t2    = new TREE_NODE();

    t2->valid        = true;
    t2->value.str    = value.str ? new wxString( *value.str ) : nullptr;
    t2->value.num    = value.num;
    t2->value.idx    = value.idx;
    t2->op           = op;
    t2->leaf[0]      = nullptr;
    t2->leaf[1]      = nullptr;
    t2->isTerminal   = false;
    t2->srcPos       = compiler->GetSourcePos();
    t2->uop          = nullptr;

    libeval_dbg(10, " ostr %p nstr %p nnode %p op %d", value.str, t2->value.str, t2, t2->op );

    if(t2->value.str)
        compiler->GcItem( t2->value.str );

    compiler->GcItem( t2 );

    return t2;
}


static const wxString formatOpName( int op )
{
    static const struct
    {
        int      op;
        wxString mnemonic;
    }
    simpleOps[] =
    {
        { TR_OP_MUL, "MUL" },
        { TR_OP_DIV, "DIV" },
        { TR_OP_ADD, "ADD" },
        { TR_OP_SUB, "SUB" },
        { TR_OP_LESS, "LESS" },
        { TR_OP_GREATER, "GREATER" },
        { TR_OP_LESS_EQUAL, "LESS_EQUAL" },
        { TR_OP_GREATER_EQUAL, "GREATER_EQUAL" },
        { TR_OP_EQUAL, "EQUAL" },
        { TR_OP_NOT_EQUAL, "NEQUAL" },
        { TR_OP_BOOL_AND, "AND" },
        { TR_OP_BOOL_OR, "OR" },
        { TR_OP_BOOL_NOT, "NOT" },
        { -1, "" }
    };

    for( int i = 0; simpleOps[i].op >= 0; i++ )
    {
        if( simpleOps[i].op == op )
            return simpleOps[i].mnemonic;
    }

    return "???";
}


bool VALUE::EqualTo( CONTEXT* aCtx, const VALUE* b ) const
{
    if( m_type == VT_UNDEFINED || b->m_type == VT_UNDEFINED )
        return false;

    if( m_type == VT_NULL && b->m_type == VT_NULL )
        return true;

    if( m_type == VT_NUMERIC && b->m_type == VT_NUMERIC )
    {
        return AsDouble() == b->AsDouble();
    }
    else if( m_type == VT_STRING && b->m_type == VT_STRING )
    {
        if( b->m_stringIsWildcard )
            return WildCompareString( b->AsString(), AsString(), false );
        else
            return AsString().IsSameAs( b->AsString(), false );
    }

    return false;
}


bool VALUE::NotEqualTo( CONTEXT* aCtx, const VALUE* b ) const
{
    if( m_type == VT_UNDEFINED || b->m_type == VT_UNDEFINED )
        return false;

    return !EqualTo( aCtx, b );
}


wxString UOP::Format() const
{
    wxString str;

    switch( m_op )
    {
    case TR_UOP_PUSH_VAR:
        str = wxString::Format( "PUSH VAR [%p]", m_ref.get() );
        break;

    case TR_UOP_PUSH_VALUE:
        if( !m_value )
            str = wxString::Format( "PUSH nullptr" );
        else if( m_value->GetType() == VT_NUMERIC )
            str = wxString::Format( "PUSH NUM [%.10f]", m_value->AsDouble() );
        else
            str = wxString::Format( "PUSH STR [%ls]", m_value->AsString() );
        break;

    case TR_OP_METHOD_CALL:
        str = wxString::Format( "MCALL" );
        break;

    case TR_OP_FUNC_CALL:
        str = wxString::Format( "FCALL" );
        break;

    default:
        str = wxString::Format( "%s %d", formatOpName( m_op ).c_str(), m_op );
        break;
    }

    return str;
}


UCODE::~UCODE()
{
    for( UOP* op : m_ucode )
        delete op;
}


wxString UCODE::Dump() const
{
    wxString rv;

    for( UOP* op : m_ucode )
    {
        rv += op->Format();
        rv += "\n";
    }

    return rv;
};


wxString TOKENIZER::GetString()
{
    wxString rv;

    while( m_pos < m_str.length() && m_str[ m_pos ] != '\'' )
    {
        if( m_str[ m_pos ] == '\\' && m_pos + 1 < m_str.length() && m_str[ m_pos + 1 ] == '\'' )
            m_pos++;

        rv.append( 1, m_str[ m_pos++ ] );
    }

    m_pos++;

    return rv;
}


wxString TOKENIZER::GetChars( const std::function<bool( wxUniChar )>& cond ) const
{
    wxString rv;
    size_t      p = m_pos;

    while( p < m_str.length() && cond( m_str[p] ) )
    {
        rv.append( 1, m_str[p] );
        p++;
    }

    return rv;
}


bool TOKENIZER::MatchAhead( const wxString& match,
                            const std::function<bool( wxUniChar )>& stopCond ) const
{
    int remaining = (int) m_str.Length() - m_pos;

    if( remaining < (int) match.length() )
        return false;

    if( m_str.substr( m_pos, match.length() ) == match )
        return ( remaining == (int) match.length() || stopCond( m_str[m_pos + match.length()] ) );

    return false;
}


COMPILER::COMPILER() :
        m_lexerState( COMPILER::LS_DEFAULT )
{
    m_localeDecimalSeparator = '.';
    m_sourcePos = 0;
    m_parseFinished = false;
    m_unitResolver = std::make_unique<UNIT_RESOLVER>();
    m_parser = LIBEVAL::ParseAlloc( malloc );
    m_tree = nullptr;
    m_errorStatus.pendingError = false;
}


COMPILER::~COMPILER()
{
    LIBEVAL::ParseFree( m_parser, free );

    if( m_tree )
    {
        freeTree( m_tree );
        m_tree = nullptr;
    }

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
        m_tree = nullptr;
    }

    m_tree = nullptr;

    for( TREE_NODE* tok : m_gcItems )
        delete tok;

    for( wxString* tok: m_gcStrings )
        delete tok;

    m_gcItems.clear();
    m_gcStrings.clear();
}


void COMPILER::parseError( const char* s )
{
    reportError( CST_PARSE, s );
}


void COMPILER::parseOk()
{
    m_parseFinished = true;
}


bool COMPILER::Compile( const wxString& aString, UCODE* aCode, CONTEXT* aPreflightContext )
{
    // Feed parser token after token until end of input.

    newString( aString );

    if( m_tree )
    {
        freeTree( m_tree );
        m_tree = nullptr;
    }

    m_tree = nullptr;
    m_parseFinished = false;
    T_TOKEN tok( defaultToken );

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

        if( tok.value.str )
            GcItem( tok.value.str );

        libeval_dbg(10, "parse: tok %d valstr %p\n", tok.token, tok.value.str );
        Parse( m_parser, tok.token, tok, this );

        if ( m_errorStatus.pendingError )
            return false;

        if( m_parseFinished || tok.token == G_ENDS )
        {
            // Reset parser by passing zero as token ID, value is ignored.
            Parse( m_parser, 0, tok, this );
            break;
        }
    } while( tok.token );

    return generateUCode( aCode, aPreflightContext );
}


void COMPILER::newString( const wxString& aString )
{
    Clear();

    m_lexerState = LS_DEFAULT;
    m_tokenizer.Restart( aString );
    m_parseFinished = false;
}


T_TOKEN COMPILER::getToken()
{
    T_TOKEN rv;
    rv.value = defaultTokenValue;

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
    } while( !done );

    return rv;
}


bool COMPILER::lexString( T_TOKEN& aToken )
{
    aToken.token = G_STRING;
    aToken.value.str = new wxString( m_tokenizer.GetString() );

    m_lexerState = LS_DEFAULT;
    return true;
}


int COMPILER::resolveUnits()
{
    int unitId = 0;

    for( const wxString& unitName : m_unitResolver->GetSupportedUnits() )
    {
        if( m_tokenizer.MatchAhead( unitName,
                                    []( int c ) -> bool
                                    {
                                        return !isalnum( c );
                                    } ) )
        {
            libeval_dbg(10, "Match unit '%s'\n", unitName.c_str() );
            m_tokenizer.NextChar( unitName.length() );
            return unitId;
        }

        unitId++;
    }

    return -1;
}


bool COMPILER::lexDefault( T_TOKEN& aToken )
{
    T_TOKEN  retval;
    wxString current;
    int      convertFrom;
    wxString msg;

    retval.value.str = nullptr;
    retval.value.num = 0.0;
    retval.value.idx = -1;
    retval.token = G_ENDS;

    if( m_tokenizer.Done() )
    {
        aToken = retval;
        return true;
    }

    auto isDecimalSeparator =
            [&]( wxUniChar ch ) -> bool
            {
                return ( ch == m_localeDecimalSeparator || ch == '.' || ch == ',' );
            };

    // Lambda: get value as string, store into clToken.token and update current index.
    auto extractNumber =
            [&]()
            {
                bool      haveSeparator = false;
                wxUniChar ch = m_tokenizer.GetChar();

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
        retval.value.str = new wxString( current );
    }
    else if( ( convertFrom = resolveUnits() ) >= 0 )
    {
        // UNIT
        // Units are appended to a VALUE.
        // Determine factor to default unit if unit for value is given.
        // Example: Default is mm, unit is inch: factor is 25.4
        // The factor is assigned to the terminal UNIT. The actual
        // conversion is done within a parser action.
        retval.token            = G_UNIT;
        retval.value.idx        = convertFrom;
    }
    else if( ch == '\'' ) // string literal
    {
        m_lexerState = LS_STRING;
        m_tokenizer.NextChar();
        return false;
    }
    else if( isalpha( ch ) || ch == '_' )
    {
        current = m_tokenizer.GetChars( []( int c ) -> bool { return isalnum( c ) || c == '_'; } );
        retval.token = G_IDENTIFIER;
        retval.value.str = new wxString( current );
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
        case ',': retval.token = G_COMMA;        break;

        default:
            if( m_tokenizer.MatchAhead( "${", []( int c ) -> bool { return c != '{'; } ) )
                reportError( CST_PARSE, _( "Unresolved text variable reference" ), m_sourcePos + 2 );
            else
                reportError( CST_PARSE, wxString::Format( _( "Unrecognized character '%c'" ), (char) ch ) );
            break;
        }

        m_tokenizer.NextChar();
    }

    aToken = retval;
    return true;
}


const wxString formatNode( TREE_NODE* node )
{
    return node->value.str ? *(node->value.str) : wxString( wxEmptyString );
}


void dumpNode( wxString& buf, TREE_NODE* tok, int depth = 0 )
{
    wxString str;

    if( !tok )
        return;

    str.Printf( "\n[%p L0:%-20p L1:%-20p] ", tok, tok->leaf[0], tok->leaf[1] );
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

    case TR_ARG_LIST:
        buf += "ARG_LIST: ";
        buf += formatNode( tok );

        if( tok->leaf[0] )
            dumpNode( buf, tok->leaf[0], depth + 1 );
        if( tok->leaf[1] )
            dumpNode( buf, tok->leaf[1], depth + 1 );

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
        buf += formatNode( tok->leaf[0] );
        buf += "': ";
        dumpNode( buf, tok->leaf[1], depth + 1 );
        break;

    case TR_UNIT:
        str.Printf( "UNIT: %d ", tok->value.idx );
        buf += str;
        break;
    }
}


void CONTEXT::ReportError( const wxString& aErrorMsg )
{
    if( m_errorCallback )
        m_errorCallback( aErrorMsg, -1 );
}


void COMPILER::reportError( COMPILATION_STAGE stage, const wxString& aErrorMsg, int aPos )
{
    if( aPos == -1 )
        aPos = m_sourcePos;

    m_errorStatus.pendingError = true;
    m_errorStatus.stage = stage;
    m_errorStatus.message = aErrorMsg;
    m_errorStatus.srcPos = aPos;

    if( m_errorCallback )
        m_errorCallback( aErrorMsg, aPos );
}


void COMPILER::setRoot( TREE_NODE *root )
{
    m_tree = root;
}


void COMPILER::freeTree( LIBEVAL::TREE_NODE *tree )
{
    if ( tree->leaf[0] )
        freeTree( tree->leaf[0] );

    if ( tree->leaf[1] )
        freeTree( tree->leaf[1] );

    delete tree->uop;
    tree->uop = nullptr;
}


void TREE_NODE::SetUop( int aOp, double aValue, EDA_UNITS aUnits )
{
    delete uop;

    std::unique_ptr<VALUE> val = std::make_unique<VALUE>( aValue );
    val->SetUnits( aUnits );
    uop = new UOP( aOp, std::move( val ) );
}


void TREE_NODE::SetUop( int aOp, const wxString& aValue, bool aStringIsWildcard )
{
    delete uop;

    std::unique_ptr<VALUE> val = std::make_unique<VALUE>( aValue, aStringIsWildcard );
    uop = new UOP( aOp, std::move( val ) );
}


void TREE_NODE::SetUop( int aOp, std::unique_ptr<VAR_REF> aRef )
{
    delete uop;

    uop = new UOP( aOp, std::move( aRef ) );
}


void TREE_NODE::SetUop( int aOp, FUNC_CALL_REF aFunc, std::unique_ptr<VAR_REF> aRef )
{
    delete uop;

    uop = new UOP( aOp, std::move( aFunc ), std::move( aRef ) );
}


static void prepareTree( LIBEVAL::TREE_NODE *node )
{
    node->isVisited = false;

    // fixme: for reasons I don't understand the lemon parser isn't initializing the
    // leaf node pointers of function name nodes.  -JY
    if( node->op == TR_OP_FUNC_CALL && node->leaf[0] )
    {
        node->leaf[0]->leaf[0] = nullptr;
        node->leaf[0]->leaf[1] = nullptr;
    }

    if ( node->leaf[0] )
        prepareTree( node->leaf[0] );

    if ( node->leaf[1] )
        prepareTree( node->leaf[1] );
}


static std::vector<TREE_NODE*> squashParamList( TREE_NODE* root )
{
    std::vector<TREE_NODE*> args;

    if( !root )
        return args;

    if( root->op != TR_ARG_LIST && root->op != TR_NULL )
    {
        args.push_back( root );
    }
    else
    {
        TREE_NODE *n = root;
        do
        {
            if( n->leaf[1] )
                args.push_back(n->leaf[1]);

            n = n->leaf[0];
        } while ( n && n->op == TR_ARG_LIST );

        if( n )
            args.push_back( n );
    }

    std::reverse( args.begin(), args.end() );

    for( size_t i = 0; i < args.size(); i++ )
        libeval_dbg( 10, "squash arg%d: %s\n", int( i ), formatNode( args[i] ) );

    return args;
}


bool COMPILER::generateUCode( UCODE* aCode, CONTEXT* aPreflightContext )
{
    std::vector<TREE_NODE*> stack;
    wxString                msg;
    int                     numericValueCount = 0;
    wxString                missingUnitsMsg;
    int                     missingUnitsSrcPos = 0;

    if( !m_tree )
    {
        std::unique_ptr<VALUE> val = std::make_unique<VALUE>( 1.0 );
        // Empty expression returns true
        aCode->AddOp( new UOP( TR_UOP_PUSH_VALUE, std::move(val) ) );
        return true;
    }

    prepareTree( m_tree );

    stack.push_back( m_tree );

    wxString dump;

    dumpNode( dump, m_tree, 0 );
    libeval_dbg( 3, "Tree dump:\n%s\n\n", (const char*) dump.c_str() );

    while( !stack.empty() )
    {
        TREE_NODE* node = stack.back();

        libeval_dbg( 4, "process node %p [op %d] [stack %lu]\n", node, node->op, (unsigned long)stack.size() );

        // process terminal nodes first
        switch( node->op )
        {
        case TR_OP_FUNC_CALL:
            // Function call's uop was generated inside TR_STRUCT_REF
            if( !node->uop )
            {
                // This function call is bare so we don't know who to apply it to
                // Set a safe default value to exit gracefully with an error
                reportError( CST_CODEGEN, _( "Unknown parent of function parameters" ), node->srcPos );
                node->SetUop( TR_UOP_PUSH_VALUE, 0.0, EDA_UNITS::UNSCALED );
            }

            node->isTerminal = true;
            break;

        case TR_STRUCT_REF:
        {
            // leaf[0]: object
            // leaf[1]: field (TR_IDENTIFIER) or TR_OP_FUNC_CALL

            if( node->leaf[0]->op != TR_IDENTIFIER )
            {
                int pos = node->leaf[0]->srcPos;

                if( node->leaf[0]->value.str )
                    pos -= static_cast<int>( formatNode( node->leaf[0] ).length() );

                reportError( CST_CODEGEN,  _( "Unknown parent of property" ), pos );

                node->leaf[0]->isVisited = true;
                node->leaf[1]->isVisited = true;

                node->SetUop( TR_UOP_PUSH_VALUE, 0.0, EDA_UNITS::UNSCALED );
                node->isTerminal = true;
                break;
            }

            switch( node->leaf[1]->op )
            {
                case TR_IDENTIFIER:
                {
                    // leaf[0]: object
                    // leaf[1]: field

                    wxString itemName = formatNode( node->leaf[0] );
                    wxString propName = formatNode( node->leaf[1] );
                    std::unique_ptr<VAR_REF> vref = aCode->CreateVarRef( itemName, propName );

                    if( !vref )
                    {
                        msg.Printf( _( "Unrecognized item '%s'" ), itemName );
                        reportError( CST_CODEGEN, msg, node->leaf[0]->srcPos - (int) itemName.length() );
                    }
                    else if( vref->GetType() == VT_PARSE_ERROR )
                    {
                        msg.Printf( _( "Unrecognized property '%s'" ), propName );
                        reportError( CST_CODEGEN, msg, node->leaf[1]->srcPos - (int) propName.length() );
                    }

                    node->leaf[0]->isVisited = true;
                    node->leaf[1]->isVisited = true;

                    node->SetUop( TR_UOP_PUSH_VAR, std::move( vref ) );
                    node->isTerminal = true;
                    break;
                }
                case TR_OP_FUNC_CALL:
                {
                    // leaf[0]: object
                    // leaf[1]: TR_OP_FUNC_CALL
                    //    leaf[0]: function name
                    //    leaf[1]: parameter

                    wxString                 itemName = formatNode( node->leaf[0] );
                    std::unique_ptr<VAR_REF> vref = aCode->CreateVarRef( itemName, "" );

                    if( !vref )
                    {
                        msg.Printf( _( "Unrecognized item '%s'" ), itemName );
                        reportError( CST_CODEGEN, msg, node->leaf[0]->srcPos - (int) itemName.length() );
                    }

                    wxString functionName = formatNode( node->leaf[1]->leaf[0] );
                    auto  func = aCode->CreateFuncCall( functionName );
                    std::vector<TREE_NODE*> params = squashParamList( node->leaf[1]->leaf[1] );

                    libeval_dbg( 10, "emit func call: %s\n", functionName );

                    if( !func )
                    {
                        msg.Printf( _( "Unrecognized function '%s'" ), functionName );
                        reportError( CST_CODEGEN, msg, node->leaf[0]->srcPos + 1 );
                    }

                    if( func )
                    {
                        // Preflight the function call

                        for( TREE_NODE* pnode : params )
                        {
                            VALUE*   param = aPreflightContext->AllocValue();
                            param->Set( formatNode( pnode ) );
                            aPreflightContext->Push( param );
                        }

                        aPreflightContext->SetErrorCallback(
                                [&]( const wxString& aMessage, int aOffset )
                                {
                                    size_t loc = node->leaf[1]->leaf[1]->srcPos;
                                    reportError( CST_CODEGEN, aMessage, (int) loc - 1 );
                                } );

                        try
                        {
                            func( aPreflightContext, vref.get() );
                            aPreflightContext->Pop();           // return value
                        }
                        catch( ... )
                        {
                        }
                    }

                    node->leaf[0]->isVisited = true;
                    node->leaf[1]->isVisited = true;
                    node->leaf[1]->leaf[0]->isVisited = true;
                    node->leaf[1]->leaf[1]->isVisited = true;

                    // Our non-terminal-node stacking algorithm can't handle doubly-nested
                    // structures so we need to pop a level by replacing the TR_STRUCT_REF with
                    // a TR_OP_FUNC_CALL and its function parameter
                    stack.pop_back();
                    stack.push_back( node->leaf[1] );

                    for( TREE_NODE* pnode : params )
                        stack.push_back( pnode );

                    node->leaf[1]->SetUop( TR_OP_METHOD_CALL, std::move( func ), std::move( vref ) );
                    node->isTerminal = false;
                    break;
                }

                default:
                    // leaf[0]: object
                    // leaf[1]: malformed syntax

                    wxString itemName = formatNode( node->leaf[0] );
                    wxString propName = formatNode( node->leaf[1] );
                    std::unique_ptr<VAR_REF> vref = aCode->CreateVarRef( itemName, propName );

                    if( !vref )
                    {
                        msg.Printf( _( "Unrecognized item '%s'" ), itemName );
                        reportError( CST_CODEGEN, msg, node->leaf[0]->srcPos - (int) itemName.length() );
                    }

                    msg.Printf( _( "Unrecognized property '%s'" ), propName );
                    reportError( CST_CODEGEN, msg, node->leaf[0]->srcPos + 1 );

                    node->leaf[0]->isVisited = true;
                    node->leaf[1]->isVisited = true;

                    node->SetUop( TR_UOP_PUSH_VALUE, 0.0, EDA_UNITS::UNSCALED );
                    node->isTerminal = true;
                    break;
            }

            break;
        }

        case TR_NUMBER:
        {
            TREE_NODE* son = node->leaf[0];
            double     value;
            EDA_UNITS  unitsType = EDA_UNITS::UNSCALED;

            if( !node->value.str )
            {
                value = 0.0;
            }
            else if( son && son->op == TR_UNIT )
            {
                if( m_unitResolver->GetSupportedUnits().empty() )
                {
                    msg.Printf( _( "Unexpected units for '%s'" ), formatNode( node ) );
                    reportError( CST_CODEGEN, msg, node->srcPos );
                }

                int units = son->value.idx;
                unitsType = m_unitResolver->GetSupportedUnitsTypes().at( units );
                value =  m_unitResolver->Convert( formatNode( node ), units );
                son->isVisited = true;
            }
            else
            {
                if( !m_unitResolver->GetSupportedUnitsMessage().empty() )
                {
                    missingUnitsMsg.Printf( _( "Missing units for '%s'| (%s)" ),
                                            formatNode( node ),
                                            m_unitResolver->GetSupportedUnitsMessage() );
                    missingUnitsSrcPos = node->srcPos;
                }

                value = EDA_UNIT_UTILS::UI::DoubleValueFromString( formatNode( node ) );
            }

            node->SetUop( TR_UOP_PUSH_VALUE, value, unitsType );
            node->isTerminal = true;
            numericValueCount++;
            break;
        }

        case TR_STRING:
        {
            wxString str = formatNode( node );
            bool isWildcard = str.Contains("?") || str.Contains("*");
            node->SetUop( TR_UOP_PUSH_VALUE, str, isWildcard );
            node->isTerminal = true;
            break;
        }

        case TR_IDENTIFIER:
        {
            std::unique_ptr<VAR_REF> vref = aCode->CreateVarRef( formatNode( node ), "" );

            if( !vref )
            {
                msg.Printf( _( "Unrecognized item '%s'" ), formatNode( node ) );
                reportError( CST_CODEGEN, msg, node->srcPos - (int) formatNode( node ).length() );
            }

            node->SetUop( TR_UOP_PUSH_VAR, std::move( vref ) );
            node->isTerminal = true;
            break;
        }

        default:
            node->SetUop( node->op );
            node->isTerminal = ( !node->leaf[0] || node->leaf[0]->isVisited )
                                    && ( !node->leaf[1] || node->leaf[1]->isVisited );
            break;
        }

        if( !node->isTerminal )
        {
            if( node->leaf[0] && !node->leaf[0]->isVisited )
            {
                stack.push_back( node->leaf[0] );
                node->leaf[0]->isVisited = true;
            }
            else if( node->leaf[1] && !node->leaf[1]->isVisited )
            {
                stack.push_back( node->leaf[1] );
                node->leaf[1]->isVisited = true;
            }

            continue;
        }

        node->isVisited = true;

        if( node->uop )
        {
            aCode->AddOp( node->uop );
            node->uop = nullptr;
        }

        stack.pop_back();
    }

    // Report the common error condition of a single numeric value with no units (which will result in
    // nanometers, and rarely leads to anything useful).
    // Reporting missing units with multiple numeric values is far more complicated as we have to
    // separate comparison operators from multiplicative operators from additive operators.  Consider:
    //   2 * 1.5mm
    //   2mm + 105um
    //   2mm > 20um
    //   (2mm + 1.5mm) * 3
    if( !missingUnitsMsg.IsEmpty() && numericValueCount == 1 )
        reportError( CST_CODEGEN, missingUnitsMsg, missingUnitsSrcPos );

    libeval_dbg(2,"dump: \n%s\n", aCode->Dump().c_str() );

    return true;
}


void UOP::Exec( CONTEXT* ctx )
{
    switch( m_op )
    {
    case TR_UOP_PUSH_VAR:
    {
        VALUE* value = nullptr;

        if( m_ref )
            value = ctx->StoreValue( m_ref->GetValue( ctx ) );
        else
            value = ctx->AllocValue();

        ctx->Push( value );
        break;
    }

    case TR_UOP_PUSH_VALUE:
        ctx->Push( m_value.get() );
        return;

    case TR_OP_METHOD_CALL:
        if( m_func )
            m_func( ctx, m_ref.get() );

        return;

    default:
        break;
    }

#define AS_DOUBLE( arg ) ( arg ? arg->AsDouble() : 0.0 )

    if( m_op & TR_OP_BINARY_MASK )
    {
        LIBEVAL::VALUE* arg2 = ctx->Pop();
        LIBEVAL::VALUE* arg1 = ctx->Pop();
        double          result;

        if( ctx->HasErrorCallback() )
        {
            if( arg1 && arg1->GetType() == VT_STRING && arg2 && arg2->GetType() == VT_NUMERIC )
            {
                ctx->ReportError( wxString::Format( _( "Type mismatch between '%s' and %lf" ),
                                                    arg1->AsString(),
                                                    arg2->AsDouble() ) );
            }
            else if( arg1 && arg1->GetType() == VT_NUMERIC && arg2 && arg2->GetType() == VT_STRING )
            {
                ctx->ReportError( wxString::Format( _( "Type mismatch between %lf and '%s'" ),
                                                    arg1->AsDouble(),
                                                    arg2->AsString() ) );
            }
        }

        // TODO: This doesn't fully calculate units correctly yet. We really need to work out the dimensional analysis
        // TODO: (and do this independently of unit specifics - e.g. MM + INCH needs to return one of the dimension
        // TODO: types, but our units framework doesn't currently allow this. Therefore, use some heuristics to
        // TODO: determine the resulting operation unit type for now
        auto getOpResultUnits =
                []( const VALUE* aVal1, const VALUE* aVal2 )
                {
                    wxCHECK( aVal1 && aVal2, EDA_UNITS::UNSCALED );

                    // This condition can occur in, e.g., a unary negation operation
                    if( aVal1->GetUnits() == EDA_UNITS::UNSCALED && aVal2->GetUnits() != EDA_UNITS::UNSCALED )
                        return aVal2->GetUnits();

                    if( aVal1->GetUnits() != EDA_UNITS::UNSCALED && aVal2->GetUnits() == EDA_UNITS::UNSCALED )
                        return aVal1->GetUnits();

                    return aVal2->GetUnits();
                };

        EDA_UNITS resultUnits = EDA_UNITS::UNSCALED;

        switch( m_op )
        {
        case TR_OP_ADD:
            result = AS_DOUBLE( arg1 ) + AS_DOUBLE( arg2 );
            resultUnits = getOpResultUnits( arg1, arg2 );
            break;

        case TR_OP_SUB:
            result = AS_DOUBLE( arg1 ) - AS_DOUBLE( arg2 );
            resultUnits = getOpResultUnits( arg1, arg2 );
            break;

        case TR_OP_MUL:
            result = AS_DOUBLE( arg1 ) * AS_DOUBLE( arg2 );
            resultUnits = getOpResultUnits( arg1, arg2 );
            break;

        case TR_OP_DIV:
            result = AS_DOUBLE( arg1 ) / AS_DOUBLE( arg2 );
            resultUnits = getOpResultUnits( arg1, arg2 );
            break;

        case TR_OP_LESS_EQUAL:
            result = AS_DOUBLE( arg1 ) <= AS_DOUBLE( arg2 ) ? 1 : 0;
            break;

        case TR_OP_GREATER_EQUAL:
            result = AS_DOUBLE( arg1 ) >= AS_DOUBLE( arg2 ) ? 1 : 0;
            break;

        case TR_OP_LESS:
            result = AS_DOUBLE( arg1 ) < AS_DOUBLE( arg2 ) ? 1 : 0;
            break;
        case TR_OP_GREATER:
            result = AS_DOUBLE( arg1 ) > AS_DOUBLE( arg2 ) ? 1 : 0;
            break;

        case TR_OP_EQUAL:
            if( !arg1 || !arg2 )
                result = arg1 == arg2 ? 1 : 0;
            else if( arg2->GetType() == VT_UNDEFINED )
                result = arg2->EqualTo( ctx, arg1 ) ? 1 : 0;
            else
                result = arg1->EqualTo( ctx, arg2 ) ? 1 : 0;
            break;

        case TR_OP_NOT_EQUAL:
            if( !arg1 || !arg2 )
                result = arg1 != arg2 ? 1 : 0;
            else if( arg2->GetType() == VT_UNDEFINED )
                result = arg2->NotEqualTo( ctx, arg1 ) ? 1 : 0;
            else
                result = arg1->NotEqualTo( ctx, arg2 ) ? 1 : 0;
            break;

        case TR_OP_BOOL_AND:
            result = AS_DOUBLE( arg1 ) != 0.0 && AS_DOUBLE( arg2 ) != 0.0 ? 1 : 0;
            break;

        case TR_OP_BOOL_OR:
            result = AS_DOUBLE( arg1 ) != 0.0 || AS_DOUBLE( arg2 ) != 0.0 ? 1 : 0;
            break;

        default:
            result = 0.0;
            break;
        }

        VALUE* rp = ctx->AllocValue();
        rp->Set( result );
        rp->SetUnits( resultUnits );
        ctx->Push( rp );
        return;
    }
    else if( m_op & TR_OP_UNARY_MASK )
    {
        LIBEVAL::VALUE* arg1 = ctx->Pop();
        double          ARG1VALUE = arg1 ? arg1->AsDouble() : 0.0;
        double          result;
        EDA_UNITS       resultUnits = arg1 ? arg1->GetUnits() : EDA_UNITS::UNSCALED;

        switch( m_op )
        {
        case TR_OP_BOOL_NOT:
            result = ARG1VALUE != 0.0 ? 0 : 1;
            break;
        default:
            result = ARG1VALUE != 0.0 ? 1 : 0;
            break;
        }

        VALUE* rp = ctx->AllocValue();
        rp->Set( result );
        rp->SetUnits( resultUnits );
        ctx->Push( rp );
        return;
    }
}


VALUE* UCODE::Run( CONTEXT* ctx )
{
    try
    {
        for( UOP* op : m_ucode )
            op->Exec( ctx );
    }
    catch(...)
    {
        // rules which fail outright should not be fired; return 0/false
        return ctx->StoreValue( new VALUE( 0 ) );
    }

    if( ctx->SP() == 1 )
    {
        return ctx->Pop();
    }
    else
    {
        // If stack is corrupted after execution it suggests a problem with the compiler, not
        // the rule....

        // do not use "assert"; it crashes outright on OSX
        wxASSERT( ctx->SP() == 1 );

        // non-well-formed rules should not be fired on a release build
        return ctx->StoreValue( new VALUE( 0 ) );
    }
}


} // namespace LIBEVAL
