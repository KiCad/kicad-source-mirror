/*
    This file is part of libeval, a simple math expression evaluator

    Copyright (C) 2007 Michael Geselbracht, mgeselbracht3@gmail.com
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

#ifndef __LIBEVAL_COMPILER_H
#define __LIBEVAL_COMPILER_H

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <stack>

#include <base_units.h>
#include <wx/intl.h>

#if defined(WIN32)
// This gets leaked by python headers on MSVC only and will cause chaos
#undef COMPILER
#endif

#define TR_OP_BINARY_MASK 0x200
#define TR_OP_UNARY_MASK 0x100

#define TR_OP_MUL 0x201
#define TR_OP_DIV 0x202
#define TR_OP_ADD 0x203
#define TR_OP_SUB 0x204
#define TR_OP_LESS 0x25
#define TR_OP_GREATER 0x206
#define TR_OP_LESS_EQUAL 0x207
#define TR_OP_GREATER_EQUAL 0x208
#define TR_OP_EQUAL 0x209
#define TR_OP_NOT_EQUAL 0x20a
#define TR_OP_BOOL_AND 0x20b
#define TR_OP_BOOL_OR  0x20c
#define TR_OP_BOOL_NOT 0x100
#define TR_OP_FUNC_CALL 24
#define TR_OP_METHOD_CALL 25
#define TR_UOP_PUSH_VAR 1
#define TR_UOP_PUSH_VALUE 2

// This namespace is used for the lemon parser
namespace LIBEVAL
{

class COMPILER;

enum COMPILATION_STAGE
{
    CST_PARSE = 0,
    CST_CODEGEN,
    CST_RUNTIME
};

struct ERROR_STATUS
{
    bool pendingError = false;

    COMPILATION_STAGE stage;
    wxString          message;
    int               srcPos;
};


enum VAR_TYPE_T
{
    VT_STRING = 1,
    VT_NUMERIC,
    VT_UNDEFINED,
    VT_PARSE_ERROR
};

enum TOKEN_TYPE_T
{
    TR_UNDEFINED  = 0,
    TR_NUMBER     = 1,
    TR_IDENTIFIER = 2,
    TR_ASSIGN     = 3,
    TR_STRUCT_REF = 4,
    TR_STRING     = 5,
    TR_UNIT       = 6,
    TR_ARG_LIST   = 7,
    TR_NULL       = 8
};

class UOP;
class UCODE;
class CONTEXT;
class VAR_REF;

typedef std::function<void( CONTEXT*, void* )> FUNC_CALL_REF;

struct T_TOKEN_VALUE
{
    wxString* str;
    double    num;
    int       idx;
};

// Lemon can't handle c'tors and d'tors, so we provide a poor-man's version.
constexpr T_TOKEN_VALUE defaultTokenValue = { nullptr, 0.0, 0 };


struct T_TOKEN
{
    int           token;
    T_TOKEN_VALUE value;
};

// Lemon can't handle c'tors and d'tors, so we provide a poor-man's version.
constexpr T_TOKEN defaultToken = { TR_UNDEFINED, defaultTokenValue };


class TREE_NODE
{
public:
    T_TOKEN_VALUE value;

    int        op;
    TREE_NODE* leaf[2];
    UOP*       uop;
    bool       valid;
    bool       isTerminal;
    bool       isVisited;
    int        srcPos;

    void SetUop( int aOp, double aValue );
    void SetUop( int aOp, const wxString& aValue, bool aStringIsWildcard );
    void SetUop( int aOp, std::unique_ptr<VAR_REF> aRef = nullptr );
    void SetUop( int aOp, FUNC_CALL_REF aFunc, std::unique_ptr<VAR_REF> aRef = nullptr );
};


TREE_NODE* newNode( LIBEVAL::COMPILER* compiler, int op,
                    const T_TOKEN_VALUE& value = defaultTokenValue );

class UNIT_RESOLVER
{
public:
    UNIT_RESOLVER()
    {
    }

    virtual ~UNIT_RESOLVER()
    {
    }

    virtual const std::vector<wxString>& GetSupportedUnits() const
    {
        static const std::vector<wxString> nullUnits;

        return nullUnits;
    }

    virtual wxString GetSupportedUnitsMessage() const
    {
        return wxEmptyString;
    }

    virtual double Convert( const wxString& aString, int unitType ) const
    {
        return 0.0;
    };
};


class VALUE
{
public:
    VALUE() :
        m_type( VT_UNDEFINED ),
        m_valueDbl( 0 ),
        m_stringIsWildcard( false ),
        m_isDeferredDbl( false )
    {};

    VALUE( const wxString& aStr, bool aIsWildcard = false ) :
        m_type( VT_STRING ),
        m_valueDbl( 0 ),
        m_valueStr( aStr ),
        m_stringIsWildcard( aIsWildcard ),
        m_isDeferredDbl( false )
    {};

    VALUE( const double aVal ) :
        m_type( VT_NUMERIC ),
        m_valueDbl( aVal ),
        m_stringIsWildcard( false ),
        m_isDeferredDbl( false )
    {};

    virtual ~VALUE()
    {};

    virtual double AsDouble() const
    {
        if( m_isDeferredDbl )
        {
            m_valueDbl = m_lambdaDbl();
            m_isDeferredDbl = false;
        }

        return m_valueDbl;
    }

    virtual const wxString& AsString() const
    {
        return m_valueStr;
    }

    virtual bool EqualTo( CONTEXT* aCtx, const VALUE* b ) const;

    // NB: this is not an inverse of EqualTo as they both return false for undefined values.
    virtual bool NotEqualTo( CONTEXT* aCtx, const VALUE* b ) const;

    VAR_TYPE_T GetType() const { return m_type; };

    void Set( double aValue )
    {
        m_type = VT_NUMERIC;
        m_valueDbl = aValue;
    }

    void SetDeferredEval( std::function<double()> aLambda )
    {
        m_type = VT_NUMERIC;
        m_lambdaDbl = aLambda;
        m_isDeferredDbl = true;
    }

    void Set( const wxString& aValue )
    {
        m_type = VT_STRING;
        m_valueStr = aValue;
    }

    void Set( const VALUE &val )
    {
        m_type = val.m_type;
        m_valueDbl = val.m_valueDbl;

        if( m_type == VT_STRING )
            m_valueStr = val.m_valueStr;
    }

private:
    VAR_TYPE_T     m_type;
    mutable double m_valueDbl;
    wxString       m_valueStr;
    bool           m_stringIsWildcard;

    mutable bool            m_isDeferredDbl;
    std::function<double()> m_lambdaDbl;
};

class VAR_REF
{
public:
    VAR_REF() {};
    virtual ~VAR_REF() {};

    virtual VAR_TYPE_T GetType() const = 0;
    virtual VALUE GetValue( CONTEXT* aCtx ) = 0;
};


class CONTEXT
{
public:
    CONTEXT() :
        m_stack(),
        m_stackPtr( 0 )
    {
        m_ownedValues.reserve( 20 );
    }

    virtual ~CONTEXT()
    {
    }

    VALUE* AllocValue()
    {
        m_ownedValues.emplace_back();
        return &m_ownedValues.back();
    }

    void Push( VALUE* v )
    {
        m_stack[ m_stackPtr++ ] = v;
    }

    VALUE* Pop()
    {
        if( m_stackPtr == 0 )
        {
            ReportError( _( "Malformed expression" ) );
            return AllocValue();
        }

        return m_stack[ --m_stackPtr ];
    }

    int SP() const
    {
        return m_stackPtr;
    };

    void SetErrorCallback( std::function<void( const wxString& aMessage, int aOffset )> aCallback )
    {
        m_errorCallback = std::move( aCallback );
    }

    bool HasErrorCallback() { return m_errorCallback != nullptr; }

    void ReportError( const wxString& aErrorMsg );

private:
    std::vector<VALUE>  m_ownedValues;
    VALUE*              m_stack[100];       // std::stack not performant enough
    int                 m_stackPtr;

    std::function<void( const wxString& aMessage, int aOffset )> m_errorCallback;
};


class UCODE
{
public:
    virtual ~UCODE();

    void AddOp( UOP* uop )
    {
        m_ucode.push_back(uop);
    }

    VALUE* Run( CONTEXT* ctx );
    wxString Dump() const;

    virtual std::unique_ptr<VAR_REF> CreateVarRef( const wxString& var, const wxString& field )
    {
        return nullptr;
    };

    virtual FUNC_CALL_REF CreateFuncCall( const wxString& name )
    {
        return nullptr;
    };

protected:

    std::vector<UOP*> m_ucode;
};


class UOP
{
public:
    UOP( int op, std::unique_ptr<VALUE> value ) :
        m_op( op ),
        m_ref(nullptr),
        m_value( std::move( value ) )
    {};

    UOP( int op, std::unique_ptr<VAR_REF> vref ) :
        m_op( op ),
        m_ref( std::move( vref ) ),
        m_value(nullptr)
    {};

    UOP( int op, FUNC_CALL_REF func, std::unique_ptr<VAR_REF> vref = nullptr ) :
        m_op( op ),
        m_func( std::move( func ) ),
        m_ref( std::move( vref ) ),
        m_value(nullptr)
    {};

    ~UOP()
    {
    }

    void Exec( CONTEXT* ctx );

    wxString Format() const;

private:
    int                      m_op;

    FUNC_CALL_REF            m_func;
    std::unique_ptr<VAR_REF> m_ref;
    std::unique_ptr<VALUE>   m_value;
};

class TOKENIZER
{
public:
    void Restart( const wxString& aStr )
    {
        m_str = aStr;
        m_pos = 0;
    }

    void Clear()
    {
        m_str = "";
        m_pos = 0;
    }

    int GetChar() const
    {
        if( m_pos >= m_str.length() )
            return 0;

        return m_str[m_pos];
    }

    bool Done() const
    {
        return m_pos >= m_str.length();
    }

    void NextChar( int aAdvance = 1 )
    {
        m_pos += aAdvance;
    }

    size_t GetPos() const
    {
        return m_pos;
    }

    wxString GetChars( const std::function<bool( wxUniChar )>& cond ) const;

    bool MatchAhead( const wxString& match,
                     const std::function<bool( wxUniChar )>& stopCond ) const;

private:
    wxString m_str;
    size_t   m_pos = 0;
};


class COMPILER
{
public:
    COMPILER();
    virtual ~COMPILER();

    /*
     * clear() should be invoked by the client if a new input string is to be processed. It
     * will reset the parser. User defined variables are retained.
     */
    void Clear();

    /* Used by the lemon parser */
    void parseError( const char* s );
    void parseOk();

    int GetSourcePos() const { return m_sourcePos; }

    void setRoot( LIBEVAL::TREE_NODE *root );
    void freeTree( LIBEVAL::TREE_NODE *tree );

    bool Compile( const wxString& aString, UCODE* aCode, CONTEXT* aPreflightContext );

    void SetErrorCallback( std::function<void( const wxString& aMessage, int aOffset )> aCallback )
    {
        m_errorCallback = std::move( aCallback );
    }

    bool IsErrorPending() const { return m_errorStatus.pendingError; }
    const ERROR_STATUS& GetError() const { return m_errorStatus; }

    void GcItem( TREE_NODE* aItem ) { m_gcItems.push_back( aItem ); }
    void GcItem( wxString* aItem ) { m_gcStrings.push_back( aItem ); }

protected:
    enum LEXER_STATE
    {
        LS_DEFAULT = 0,
        LS_STRING  = 1,
    };

    LEXER_STATE m_lexerState;

    bool generateUCode( UCODE* aCode, CONTEXT* aPreflightContext );

    void reportError( COMPILATION_STAGE stage, const wxString& aErrorMsg, int aPos = -1 );

    /* Begin processing of a new input string */
    void newString( const wxString& aString );

    /* Tokenizer: Next token/value taken from input string. */
    T_TOKEN getToken();
    bool lexDefault( T_TOKEN& aToken );
    bool lexString( T_TOKEN& aToken );

    int resolveUnits();

protected:
    /* Token state for input string. */
    void*        m_parser; // the current lemon parser state machine
    TOKENIZER    m_tokenizer;
    char         m_localeDecimalSeparator;

    std::unique_ptr<UNIT_RESOLVER> m_unitResolver;

    int          m_sourcePos;
    bool         m_parseFinished;
    ERROR_STATUS m_errorStatus;

    std::function<void( const wxString& aMessage, int aOffset )> m_errorCallback;

    TREE_NODE*   m_tree;

    std::vector<TREE_NODE*>  m_gcItems;
    std::vector<wxString*>   m_gcStrings;
};


} // namespace LIBEVAL

#endif /* LIBEVAL_COMPILER_H_ */
