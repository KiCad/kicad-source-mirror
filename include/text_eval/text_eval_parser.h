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
#pragma once
#include <fast_float/fast_float.h>
#include <kicommon.h>
#include <text_eval/text_eval_types.h>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <variant>
#include <concepts>
#include <ranges>
#include <fmt/format.h>
#include <optional>
#include <cassert>
#include <cmath>
#include <chrono>
#include <random>
#include <numeric>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <functional>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace calc_parser
{
    using Value = std::variant<double, std::string>;

    // Simple token type for parser compatibility
    struct TOKEN_TYPE
    {
        char text[256];  // Fixed size buffer for strings
        double dValue;   // Numeric value
        bool isString;   // Flag to indicate if this is a string token
    };

    // Helper functions for TOKEN_TYPE
    inline TOKEN_TYPE MakeStringToken(const std::string& str)
    {
        TOKEN_TYPE token;
        token.dValue = 0.0;
        token.isString = true;
        strncpy(token.text, str.c_str(), sizeof(token.text) - 1);
        token.text[sizeof(token.text) - 1] = '\0';
        return token;
    }

    inline TOKEN_TYPE MakeNumberToken(double val)
    {
        TOKEN_TYPE token;
        token.dValue = val;
        token.isString = false;
        token.text[0] = '\0';
        return token;
    }

    inline std::string GetTokenString(const TOKEN_TYPE& token)
    {
        return std::string(token.text);
    }

    inline double GetTokenDouble(const TOKEN_TYPE& token)
    {
        return token.dValue;
    }

    // Value utilities for type handling
    class VALUE_UTILS
    {
    public:
        // Convert Value to double (for arithmetic operations)
        static auto ToDouble( const Value& aVal ) -> Result<double>
        {
            if( std::holds_alternative<double>( aVal ) )
                return MakeValue( std::get<double>( aVal ) );

            const auto& str = std::get<std::string>( aVal );
            try
            {
                double value;
                auto result = fast_float::from_chars( str.data(), str.data() + str.size(), value );

                if( result.ec != std::errc() || result.ptr != str.data() + str.size() )
                    throw std::invalid_argument( "Invalid number format" );

                return MakeValue( value );
            }
            catch( ... )
            {
                return MakeError<double>( fmt::format( "Cannot convert '{}' to number", str ) );
            }
        }

        // Convert Value to string (for display/concatenation)
        static auto ToString( const Value& aVal ) -> std::string
        {
            if( std::holds_alternative<std::string>( aVal ) )
                return std::get<std::string>( aVal );

            const auto num = std::get<double>( aVal );

            // Smart number formatting with tolerance for floating-point precision
            constexpr double tolerance = 1e-10;
            double rounded = std::round( num );

            // If the number is very close to a whole number, treat it as such
            if( std::abs( num - rounded ) < tolerance && std::abs( rounded ) < 1e15 )
                return fmt::format( "{:.0f}", rounded );

            return fmt::format( "{}", num );
        }

        static auto ToChar( const Value& aVal ) -> char
        {
            std::string str = ToString( aVal );

            if( str.empty() )
                return ' ';
            else
                return str[0];
        }

        // Check if Value represents a "truthy" value for conditionals
        static auto IsTruthy( const Value& aVal ) -> bool
        {
            if( std::holds_alternative<double>( aVal ) )
                return std::get<double>( aVal ) != 0.0;

            return !std::get<std::string>( aVal ).empty();
        }

        // arithmetic operation with type coercion
        static auto ArithmeticOp( const Value& aLeft, const Value& aRight, char aOp ) -> Result<Value>
        {
            auto leftNum = ToDouble( aLeft );
            auto rightNum = ToDouble( aRight );

            if( !leftNum ) return MakeError<Value>( leftNum.GetError() );
            if( !rightNum ) return MakeError<Value>( rightNum.GetError() );

            const auto leftVal = leftNum.GetValue();
            const auto rightVal = rightNum.GetValue();

            switch( aOp )
            {
            case '+': return MakeValue<Value>( leftVal + rightVal );
            case '-': return MakeValue<Value>( leftVal - rightVal );
            case '*': return MakeValue<Value>( leftVal * rightVal );
            case '/':
                if( rightVal == 0.0 )
                    return MakeError<Value>( "Division by zero" );
                return MakeValue<Value>( leftVal / rightVal );
            case '%':
                if( rightVal == 0.0 )
                    return MakeError<Value>( "Modulo by zero" );
                return MakeValue<Value>( std::fmod( leftVal, rightVal ) );
            case '^': return MakeValue<Value>( std::pow( leftVal, rightVal ) );
            case '<': return MakeValue<Value>( leftVal < rightVal ? 1.0 : 0.0 );
            case '>': return MakeValue<Value>( leftVal > rightVal ? 1.0 : 0.0 );
            case 1: return MakeValue<Value>( leftVal <= rightVal ? 1.0 : 0.0 );  // <=
            case 2: return MakeValue<Value>( leftVal >= rightVal ? 1.0 : 0.0 );  // >=
            case 3: return MakeValue<Value>( leftVal == rightVal ? 1.0 : 0.0 );  // ==
            case 4: return MakeValue<Value>( leftVal != rightVal ? 1.0 : 0.0 );  // !=
            default:
                return MakeError<Value>( "Unknown operator" );
            }
        }

        // String concatenation (special case of '+' for strings)
        static auto ConcatStrings( const Value& aLeft, const Value& aRight ) -> Value
        {
            return Value{ ToString( aLeft ) + ToString( aRight ) };
        }
    };

    class NODE;
    class DOC;
    class PARSE_CONTEXT;

    // AST Node types - supporting mixed values
    enum class NodeType { Text, Calc, Var, Number, String, BinOp, Function };

    struct BIN_OP_DATA
    {
        std::unique_ptr<NODE> left;
        std::unique_ptr<NODE> right;
        char op;

        BIN_OP_DATA( std::unique_ptr<NODE> aLeft, char aOperation, std::unique_ptr<NODE> aRight ) :
                left( std::move( aLeft ) ),
                right( std::move( aRight ) ),
                op( aOperation )
        {}
    };

    struct FUNC_DATA
    {
        std::string name;
        std::vector<std::unique_ptr<NODE>> args;

        FUNC_DATA( std::string aName, std::vector<std::unique_ptr<NODE>> aArguments ) :
                name( std::move( aName ) ),
                args( std::move( aArguments ) )
        {}
    };

    class NODE
    {
    public:
        NodeType type;
        std::variant<std::string, double, BIN_OP_DATA, FUNC_DATA> data;

        // Factory methods for type safety
        static auto CreateText( std::string aText ) -> std::unique_ptr<NODE>
        {
            auto node = std::make_unique<NODE>();
            node->type = NodeType::Text;
            node->data = std::move( aText );
            return node;
        }

        static auto CreateCalc( std::unique_ptr<NODE> aExpr ) -> std::unique_ptr<NODE>
        {
            auto node = std::make_unique<NODE>();
            node->type = NodeType::Calc;
            node->data = BIN_OP_DATA( std::move( aExpr ), '=', nullptr );
            return node;
        }

        static auto CreateVar( std::string aName ) -> std::unique_ptr<NODE>
        {
            auto node = std::make_unique<NODE>();
            node->type = NodeType::Var;
            node->data = std::move( aName );
            return node;
        }

        static auto CreateNumber( double aValue ) -> std::unique_ptr<NODE>
        {
            auto node = std::make_unique<NODE>();
            node->type = NodeType::Number;
            node->data = aValue;
            return node;
        }

        static auto CreateString( std::string aValue ) -> std::unique_ptr<NODE>
        {
            auto node = std::make_unique<NODE>();
            node->type = NodeType::String;
            node->data = std::move( aValue );
            return node;
        }

        static auto CreateBinOp( std::unique_ptr<NODE> aLeft, char aOp, std::unique_ptr<NODE> aRight ) -> std::unique_ptr<NODE>
        {
            auto node = std::make_unique<NODE>();
            node->type = NodeType::BinOp;
            node->data = BIN_OP_DATA( std::move( aLeft ), aOp, std::move( aRight ) );
            return node;
        }

        static auto CreateFunction( std::string aName, std::vector<std::unique_ptr<NODE>> aArgs ) -> std::unique_ptr<NODE>
        {
            auto node = std::make_unique<NODE>();
            node->type = NodeType::Function;
            node->data = FUNC_DATA( std::move( aName ), std::move( aArgs ) );
            return node;
        }

        // Raw pointer factory methods for parser use
        static auto CreateTextRaw( std::string aText ) -> NODE*
        {
            auto node = new NODE();
            node->type = NodeType::Text;
            node->data = std::move( aText );
            return node;
        }

        static auto CreateCalcRaw( NODE* aExpr ) -> NODE*
        {
            auto node = new NODE();
            node->type = NodeType::Calc;
            node->data = BIN_OP_DATA( std::unique_ptr<NODE>( aExpr ), '=', nullptr );
            return node;
        }

        static auto CreateVarRaw( std::string aName ) -> NODE*
        {
            auto node = new NODE();
            node->type = NodeType::Var;
            node->data = std::move( aName );
            return node;
        }

        static auto CreateNumberRaw( double aValue ) -> NODE*
        {
            auto node = new NODE();
            node->type = NodeType::Number;
            node->data = aValue;
            return node;
        }

        static auto CreateStringRaw( std::string aValue ) -> NODE*
        {
            auto node = new NODE();
            node->type = NodeType::String;
            node->data = std::move( aValue );
            return node;
        }

        static auto CreateBinOpRaw( NODE* aLeft, char aOp, NODE* aRight ) -> NODE*
        {
            auto node = new NODE();
            node->type = NodeType::BinOp;
            node->data = BIN_OP_DATA( std::unique_ptr<NODE>( aLeft ), aOp, std::unique_ptr<NODE>( aRight ) );
            return node;
        }

        static auto CreateFunctionRaw( std::string aName, std::vector<std::unique_ptr<NODE>>* aArgs ) -> NODE*
        {
            auto node = new NODE();
            node->type = NodeType::Function;
            node->data = FUNC_DATA( std::move( aName ), std::move( *aArgs ) );
            delete aArgs;
            return node;
        }

        // Mixed-type evaluation
        template<typename Visitor>
        auto Accept( Visitor&& aVisitor ) const -> Result<Value>
        {
            return std::forward<Visitor>( aVisitor )( *this );
        }
    };

    class DOC
    {
    public:
        std::vector<std::unique_ptr<NODE>> nodes;
        mutable ERROR_COLLECTOR errors;

        auto AddNode( std::unique_ptr<NODE> aNode ) -> void
        {
            nodes.emplace_back( std::move( aNode ) );
        }

        auto AddNodeRaw( NODE* aNode ) -> void
        {
            nodes.emplace_back( std::unique_ptr<NODE>( aNode ) );
        }

        auto HasErrors() const -> bool { return errors.HasErrors(); }
        auto GetErrors() const -> const std::vector<std::string>& { return errors.GetErrors(); }
        auto GetErrorSummary() const -> std::string { return errors.GetAllMessages(); }

        auto GetNodes() const -> const auto& { return nodes; }
        auto begin() const { return nodes.begin(); }
        auto end() const { return nodes.end(); }
    };

    // Global error collector for parser callbacks
    extern thread_local ERROR_COLLECTOR* g_errorCollector;

    class PARSE_CONTEXT
    {
    public:
        ERROR_COLLECTOR& errors;

        explicit PARSE_CONTEXT( ERROR_COLLECTOR& aErrorCollector ) :
                errors( aErrorCollector )
        {
            g_errorCollector = &aErrorCollector;
        }

        ~PARSE_CONTEXT()
        {
            g_errorCollector = nullptr;
        }

        PARSE_CONTEXT( const PARSE_CONTEXT& ) = delete;
        PARSE_CONTEXT& operator=( const PARSE_CONTEXT& ) = delete;
        PARSE_CONTEXT( PARSE_CONTEXT&& ) = delete;
        PARSE_CONTEXT& operator=( PARSE_CONTEXT&& ) = delete;
    };

    // Enhanced evaluation visitor supporting callback-based variable resolution
    class KICOMMON_API EVAL_VISITOR
    {
    public:
        // Callback function type for variable resolution
        using VariableCallback = std::function<Result<Value>(const std::string& aVariableName)>;

    private:
        VariableCallback m_variableCallback;
        [[maybe_unused]] ERROR_COLLECTOR& m_errors;
        mutable std::random_device m_rd;
        mutable std::mt19937 m_gen;

    public:
        /**
         * @brief Construct evaluator with variable callback function
         * @param aVariableCallback Function to call when resolving variables
         * @param aErrorCollector Error collector for storing errors
         */
        explicit EVAL_VISITOR( VariableCallback aVariableCallback, ERROR_COLLECTOR& aErrorCollector );

        // Visitor methods for evaluating different node types
        auto operator()( const NODE& aNode ) const -> Result<Value>;

    private:
        auto evaluateFunction( const FUNC_DATA& aFunc ) const -> Result<Value>;
    };

    // Enhanced document processor supporting callback-based variable resolution
    class KICOMMON_API DOC_PROCESSOR
    {
    public:
        using VariableCallback = EVAL_VISITOR::VariableCallback;

        /**
         * @brief Process document using callback for variable resolution
         * @param aDoc Document to process
         * @param aVariableCallback Function to resolve variables
         * @return Pair of (result_string, had_errors)
         */
        static auto Process( const DOC& aDoc, VariableCallback aVariableCallback )
                            -> std::pair<std::string, bool>;

        /**
         * @brief Process document with detailed error reporting
         * @param aDoc Document to process
         * @param aVariableCallback Function to resolve variables
         * @return Tuple of (result_string, error_messages, had_errors)
         */
        static auto ProcessWithDetails( const DOC& aDoc, VariableCallback aVariableCallback )
                                       -> std::tuple<std::string, std::vector<std::string>, bool>;
    };


} // namespace calc_parser

