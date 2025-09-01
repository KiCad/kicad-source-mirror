/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file
 * Test suite for low-level text_eval_parser functionality
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <text_eval/text_eval_parser.h>

#include <memory>
#include <unordered_map>

using namespace calc_parser;

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( TextEvalParserLowLevel )

/**
 * Helper function to create a simple variable resolver for testing
 */
auto CreateTestVariableResolver()
{
    auto variables = std::make_shared<std::unordered_map<std::string, Value>>();

    // Set up some test variables
    (*variables)["x"] = 10.0;
    (*variables)["y"] = 5.0;
    (*variables)["name"] = std::string("KiCad");
    (*variables)["pi"] = 3.14159;

    return [variables]( const std::string& varName ) -> Result<Value>
    {
        auto it = variables->find( varName );
        if( it != variables->end() )
            return MakeValue( it->second );

        return MakeError<Value>( "Variable not found: " + varName );
    };
}

/**
 * Test VALUE_UTILS functionality
 */
BOOST_AUTO_TEST_CASE( ValueUtils )
{
    // Test ToDouble conversion
    {
        Value numVal = 42.5;
        auto result = calc_parser::VALUE_UTILS::ToDouble( numVal );
        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK_CLOSE( result.GetValue(), 42.5, 0.001 );
    }

    {
        Value strVal = std::string("123.45");
        auto result = calc_parser::VALUE_UTILS::ToDouble( strVal );
        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK_CLOSE( result.GetValue(), 123.45, 0.001 );
    }

    {
        Value invalidStr = std::string("not_a_number");
        auto result = calc_parser::VALUE_UTILS::ToDouble( invalidStr );
        BOOST_CHECK( result.HasError() );
    }

    // Test ToString conversion
    {
        Value numVal = 42.0;
        auto result = calc_parser::VALUE_UTILS::ToString( numVal );
        BOOST_CHECK_EQUAL( result, "42" );
    }

    {
        Value strVal = std::string("Hello");
        auto result = calc_parser::VALUE_UTILS::ToString( strVal );
        BOOST_CHECK_EQUAL( result, "Hello" );
    }

    // Test IsTruthy
    {
        BOOST_CHECK( calc_parser::VALUE_UTILS::IsTruthy( Value{1.0} ) );
        BOOST_CHECK( !calc_parser::VALUE_UTILS::IsTruthy( Value{0.0} ) );
        BOOST_CHECK( calc_parser::VALUE_UTILS::IsTruthy( Value{std::string("non-empty")} ) );
        BOOST_CHECK( !calc_parser::VALUE_UTILS::IsTruthy( Value{std::string("")} ) );
    }

    // Test ArithmeticOp
    {
        Value left = 10.0;
        Value right = 3.0;

        auto addResult = calc_parser::VALUE_UTILS::ArithmeticOp( left, right, '+' );
        BOOST_CHECK( addResult.HasValue() );
        BOOST_CHECK_CLOSE( std::get<double>( addResult.GetValue() ), 13.0, 0.001 );

        auto subResult = calc_parser::VALUE_UTILS::ArithmeticOp( left, right, '-' );
        BOOST_CHECK( subResult.HasValue() );
        BOOST_CHECK_CLOSE( std::get<double>( subResult.GetValue() ), 7.0, 0.001 );

        auto mulResult = calc_parser::VALUE_UTILS::ArithmeticOp( left, right, '*' );
        BOOST_CHECK( mulResult.HasValue() );
        BOOST_CHECK_CLOSE( std::get<double>( mulResult.GetValue() ), 30.0, 0.001 );

        auto divResult = calc_parser::VALUE_UTILS::ArithmeticOp( left, right, '/' );
        BOOST_CHECK( divResult.HasValue() );
        BOOST_CHECK_CLOSE( std::get<double>( divResult.GetValue() ), 3.333, 0.1 );

        auto modResult = calc_parser::VALUE_UTILS::ArithmeticOp( left, right, '%' );
        BOOST_CHECK( modResult.HasValue() );
        BOOST_CHECK_CLOSE( std::get<double>( modResult.GetValue() ), 1.0, 0.001 );

        auto powResult = calc_parser::VALUE_UTILS::ArithmeticOp( left, right, '^' );
        BOOST_CHECK( powResult.HasValue() );
        BOOST_CHECK_CLOSE( std::get<double>( powResult.GetValue() ), 1000.0, 0.001 );
    }

    // Test division by zero
    {
        Value left = 10.0;
        Value right = 0.0;

        auto divResult = calc_parser::VALUE_UTILS::ArithmeticOp( left, right, '/' );
        BOOST_CHECK( divResult.HasError() );

        auto modResult = calc_parser::VALUE_UTILS::ArithmeticOp( left, right, '%' );
        BOOST_CHECK( modResult.HasError() );
    }

    // Test ConcatStrings
    {
        Value left = std::string("Hello ");
        Value right = std::string("World");
        auto result = calc_parser::VALUE_UTILS::ConcatStrings( left, right );
        BOOST_CHECK_EQUAL( std::get<std::string>( result ), "Hello World" );
    }

    {
        Value left = 42.0;
        Value right = std::string(" items");
        auto result = calc_parser::VALUE_UTILS::ConcatStrings( left, right );
        BOOST_CHECK_EQUAL( std::get<std::string>( result ), "42 items" );
    }
}

/**
 * Test Node creation and basic structure
 */
BOOST_AUTO_TEST_CASE( NodeCreation )
{
    // Test number node
    {
        auto node = NODE::CreateNumber( 42.5 );
        BOOST_CHECK( node->type == NodeType::Number );
        BOOST_CHECK_CLOSE( std::get<double>( node->data ), 42.5, 0.001 );
    }

    // Test string node
    {
        auto node = NODE::CreateString( "Hello World" );
        BOOST_CHECK( node->type == NodeType::String );
        BOOST_CHECK_EQUAL( std::get<std::string>( node->data ), "Hello World" );
    }

    // Test variable node
    {
        auto node = NODE::CreateVar( "testVar" );
        BOOST_CHECK( node->type == NodeType::Var );
        BOOST_CHECK_EQUAL( std::get<std::string>( node->data ), "testVar" );
    }

    // Test binary operation node
    {
        auto left = NODE::CreateNumber( 10.0 );
        auto right = NODE::CreateNumber( 5.0 );
        auto binOp = NODE::CreateBinOp( std::move( left ), '+', std::move( right ) );

        BOOST_CHECK( binOp->type == NodeType::BinOp );
        const auto& binOpData = std::get<BIN_OP_DATA>( binOp->data );
        BOOST_CHECK( binOpData.op == '+' );
        BOOST_CHECK( binOpData.left != nullptr );
        BOOST_CHECK( binOpData.right != nullptr );
    }

    // Test function node
    {
        std::vector<std::unique_ptr<NODE>> args;
        args.push_back( NODE::CreateNumber( 5.0 ) );

        auto funcNode = NODE::CreateFunction( "abs", std::move( args ) );
        BOOST_CHECK( funcNode->type == NodeType::Function );

        const auto& funcData = std::get<FUNC_DATA>( funcNode->data );
        BOOST_CHECK_EQUAL( funcData.name, "abs" );
        BOOST_CHECK_EQUAL( funcData.args.size(), 1 );
    }
}

/**
 * Test evaluation visitor with simple expressions
 */
BOOST_AUTO_TEST_CASE( EvaluationVisitor )
{
    ERROR_COLLECTOR errors;
    auto varResolver = CreateTestVariableResolver();
    calc_parser::EVAL_VISITOR evaluator( varResolver, errors );

    // Test number evaluation
    {
        auto node = NODE::CreateNumber( 42.5 );
        auto result = node->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK( std::holds_alternative<double>( result.GetValue() ) );
        BOOST_CHECK_CLOSE( std::get<double>( result.GetValue() ), 42.5, 0.001 );
    }

    // Test string evaluation
    {
        auto node = NODE::CreateString( "Hello" );
        auto result = node->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK( std::holds_alternative<std::string>( result.GetValue() ) );
        BOOST_CHECK_EQUAL( std::get<std::string>( result.GetValue() ), "Hello" );
    }

    // Test variable evaluation
    {
        auto node = NODE::CreateVar( "x" );
        auto result = node->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK( std::holds_alternative<double>( result.GetValue() ) );
        BOOST_CHECK_CLOSE( std::get<double>( result.GetValue() ), 10.0, 0.001 );
    }

    // Test undefined variable
    {
        auto node = NODE::CreateVar( "undefined" );
        auto result = node->Accept( evaluator );

        BOOST_CHECK( result.HasError() );
    }

    // Test binary operation
    {
        auto left = NODE::CreateNumber( 10.0 );
        auto right = NODE::CreateNumber( 5.0 );
        auto binOp = NODE::CreateBinOp( std::move( left ), '+', std::move( right ) );

        auto result = binOp->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK_CLOSE( std::get<double>( result.GetValue() ), 15.0, 0.001 );
    }

    // Test string concatenation with +
    {
        auto left = NODE::CreateString( "Hello " );
        auto right = NODE::CreateString( "World" );
        auto binOp = NODE::CreateBinOp( std::move( left ), '+', std::move( right ) );

        auto result = binOp->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK( std::holds_alternative<std::string>( result.GetValue() ) );
        BOOST_CHECK_EQUAL( std::get<std::string>( result.GetValue() ), "Hello World" );
    }
}

/**
 * Test function evaluation
 */
BOOST_AUTO_TEST_CASE( FunctionEvaluation )
{
    ERROR_COLLECTOR errors;
    auto varResolver = CreateTestVariableResolver();
    calc_parser::EVAL_VISITOR evaluator( varResolver, errors );

    // Test abs function
    {
        std::vector<std::unique_ptr<NODE>> args;
        args.push_back( NODE::CreateNumber( -5.0 ) );

        auto funcNode = NODE::CreateFunction( "abs", std::move( args ) );
        auto result = funcNode->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK_CLOSE( std::get<double>( result.GetValue() ), 5.0, 0.001 );
    }

    // Test sqrt function
    {
        std::vector<std::unique_ptr<NODE>> args;
        args.push_back( NODE::CreateNumber( 16.0 ) );

        auto funcNode = NODE::CreateFunction( "sqrt", std::move( args ) );
        auto result = funcNode->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK_CLOSE( std::get<double>( result.GetValue() ), 4.0, 0.001 );
    }

    // Test sqrt with negative number (should error)
    {
        std::vector<std::unique_ptr<NODE>> args;
        args.push_back( NODE::CreateNumber( -1.0 ) );

        auto funcNode = NODE::CreateFunction( "sqrt", std::move( args ) );
        auto result = funcNode->Accept( evaluator );

        BOOST_CHECK( result.HasError() );
    }

    // Test max function
    {
        std::vector<std::unique_ptr<NODE>> args;
        args.push_back( NODE::CreateNumber( 3.0 ) );
        args.push_back( NODE::CreateNumber( 7.0 ) );
        args.push_back( NODE::CreateNumber( 1.0 ) );

        auto funcNode = NODE::CreateFunction( "max", std::move( args ) );
        auto result = funcNode->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK_CLOSE( std::get<double>( result.GetValue() ), 7.0, 0.001 );
    }

    // Test string function - upper
    {
        std::vector<std::unique_ptr<NODE>> args;
        args.push_back( NODE::CreateString( "hello" ) );

        auto funcNode = NODE::CreateFunction( "upper", std::move( args ) );
        auto result = funcNode->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK( std::holds_alternative<std::string>( result.GetValue() ) );
        BOOST_CHECK_EQUAL( std::get<std::string>( result.GetValue() ), "HELLO" );
    }

    // Test format function
    {
        std::vector<std::unique_ptr<NODE>> args;
        args.push_back( NODE::CreateNumber( 3.14159 ) );
        args.push_back( NODE::CreateNumber( 2.0 ) );

        auto funcNode = NODE::CreateFunction( "format", std::move( args ) );
        auto result = funcNode->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK( std::holds_alternative<std::string>( result.GetValue() ) );
        BOOST_CHECK_EQUAL( std::get<std::string>( result.GetValue() ), "3.14" );
    }

    // Test unknown function
    {
        std::vector<std::unique_ptr<NODE>> args;
        args.push_back( NODE::CreateNumber( 1.0 ) );

        auto funcNode = NODE::CreateFunction( "unknownfunc", std::move( args ) );
        auto result = funcNode->Accept( evaluator );

        BOOST_CHECK( result.HasError() );
    }

    // Test zero-argument functions
    {
        std::vector<std::unique_ptr<NODE>> args;  // Empty args

        auto todayNode = NODE::CreateFunction( "today", std::move( args ) );
        auto result = todayNode->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK( std::holds_alternative<double>( result.GetValue() ) );
        // Should return a reasonable number of days since epoch
        auto days = std::get<double>( result.GetValue() );
        BOOST_CHECK_GT( days, 18000 );  // Should be after year 2019
    }

    {
        std::vector<std::unique_ptr<NODE>> args;  // Empty args

        auto randomNode = NODE::CreateFunction( "random", std::move( args ) );
        auto result = randomNode->Accept( evaluator );

        BOOST_CHECK( result.HasValue() );
        BOOST_CHECK( std::holds_alternative<double>( result.GetValue() ) );
        auto randomVal = std::get<double>( result.GetValue() );
        BOOST_CHECK_GE( randomVal, 0.0 );
        BOOST_CHECK_LT( randomVal, 1.0 );
    }
}

/**
 * Test DOC_PROCESSOR functionality
 */
BOOST_AUTO_TEST_CASE( DocumentProcessor )
{
    auto varResolver = CreateTestVariableResolver();

    // Create a simple document with text and calculations
    auto doc = std::make_unique<DOC>();

    // Add text node
    doc->AddNode( NODE::CreateText( "Value is " ) );

    // Add calculation node
    auto calcExpr = NODE::CreateBinOp(
        NODE::CreateNumber( 2.0 ),
        '+',
        NODE::CreateNumber( 3.0 )
    );
    doc->AddNode( NODE::CreateCalc( std::move( calcExpr ) ) );

    // Add more text
    doc->AddNode( NODE::CreateText( " units" ) );

    // Process the document
    auto [result, hadErrors] = calc_parser::DOC_PROCESSOR::Process( *doc, varResolver );

    BOOST_CHECK( !hadErrors );
    BOOST_CHECK_EQUAL( result, "Value is 5 units" );
}

/**
 * Test error collection and reporting
 */
BOOST_AUTO_TEST_CASE( ErrorHandling )
{
    ERROR_COLLECTOR errors;

    // Test adding errors
    errors.AddError( "Test error 1" );
    errors.AddWarning( "Test warning 1" );
    errors.AddError( "Test error 2" );

    BOOST_CHECK( errors.HasErrors() );
    BOOST_CHECK( errors.HasWarnings() );

    const auto& errorList = errors.GetErrors();
    BOOST_CHECK_EQUAL( errorList.size(), 2 );
    BOOST_CHECK_EQUAL( errorList[0], "Test error 1" );
    BOOST_CHECK_EQUAL( errorList[1], "Test error 2" );

    const auto& warningList = errors.GetWarnings();
    BOOST_CHECK_EQUAL( warningList.size(), 1 );
    BOOST_CHECK_EQUAL( warningList[0], "Test warning 1" );

    // Test error message formatting
    auto allMessages = errors.GetAllMessages();
    BOOST_CHECK( allMessages.find( "Error: Test error 1" ) != std::string::npos );
    BOOST_CHECK( allMessages.find( "Warning: Test warning 1" ) != std::string::npos );

    // Test clearing
    errors.Clear();
    BOOST_CHECK( !errors.HasErrors() );
    BOOST_CHECK( !errors.HasWarnings() );
}

/**
 * Test TOKEN_TYPE utilities
 */
BOOST_AUTO_TEST_CASE( TokenTypes )
{
    // Test string token
    {
        auto token = MakeStringToken( "Hello World" );
        BOOST_CHECK( token.isString );
        BOOST_CHECK_EQUAL( GetTokenString( token ), "Hello World" );
    }

    // Test number token
    {
        auto token = MakeNumberToken( 42.5 );
        BOOST_CHECK( !token.isString );
        BOOST_CHECK_CLOSE( GetTokenDouble( token ), 42.5, 0.001 );
    }
}

BOOST_AUTO_TEST_SUITE_END()
