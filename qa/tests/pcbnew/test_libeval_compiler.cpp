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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/data/test_case.hpp>

#include <wx/wx.h>

#include <layer_ids.h>
#include <pcbnew/pcbexpr_evaluator.h>
#include <drc/drc_rule.h>
#include <pcbnew/board.h>
#include <pcbnew/pcb_track.h>

BOOST_AUTO_TEST_SUITE( Libeval_Compiler )

struct EXPR_TO_TEST
{
    wxString       expression;
    bool           expectError;
    LIBEVAL::VALUE expectedResult;

    friend std::ostream& operator<<( std::ostream& os, const EXPR_TO_TEST& expr )
    {
        os << expr.expression;
        return os;
    }
};

using VAL = LIBEVAL::VALUE;

const static std::vector<EXPR_TO_TEST> simpleExpressions = {
    { "10mm + 20 mm", false, VAL( 30e6 ) },
    { "3*(7+8)", false, VAL( 3 * ( 7 + 8 ) ) },
    { "3*7+8", false, VAL( 3 * 7 + 8 ) },
    { "(3*7)+8", false, VAL( 3 * 7 + 8 ) },
    { "10mm + 20)", true, VAL( 0 ) },

    { "1", false, VAL(1) },
    { "1.5", false, VAL(1.5) },
    { "1,5", false, VAL(1.5) },
    { "1mm", false, VAL(1e6) },
    // Any White-space is OK
    { "   1 +     2    ", false, VAL(3) },
    // Decimals are OK in expressions
    { "1.5 + 0.2 + 0.1", false, VAL(1.8) },
    // Negatives are OK
    { "3 - 10", false, VAL(-7) },
    // Lots of operands
    { "1 + 2 + 10 + 1000.05", false, VAL(1013.05) },
    // Operator precedence
    { "1 + 2 - 4 * 20 / 2", false, VAL(-37) },
    // Parens
    { "(1)", false, VAL(1) },
    // Parens affect precedence
    { "-(1 + (2 - 4)) * 20.8 / 2", false, VAL(10.4) },
    // Unary addition is a sign, not a leading operator
    { "+2 - 1", false, VAL(1) }
};


const static std::vector<EXPR_TO_TEST> introspectionExpressions = {
    { "A.type == 'Pad' && B.type == 'Pad' && (A.existsOnLayer('F.Cu'))", false, VAL( 0.0 ) },
    { "A.Width > B.Width", false, VAL( 0.0 ) },
    { "A.Width + B.Width", false, VAL( pcbIUScale.MilsToIU(10) + pcbIUScale.MilsToIU(20) ) },
    { "A.Netclass", false, VAL( "HV_LINE" ) },
    { "(A.Netclass == 'HV_LINE') && (B.netclass == 'otherClass') && (B.netclass != 'F.Cu')", false,
      VAL( 1.0 ) },
    { "A.Netclass + 1.0", false, VAL( 1.0 ) },
    { "A.hasNetclass('HV_LINE')", false, VAL( 1.0 ) },
    { "A.hasNetclass('HV_*')", false, VAL( 1.0 ) },
    { "A.type == 'Track' && B.type == 'Track' && A.layer == 'F.Cu'", false, VAL( 1.0 ) },
    { "(A.type == 'Track') && (B.type == 'Track') && (A.layer == 'F.Cu')", false, VAL( 1.0 ) },
    { "A.type == 'Via' && A.isMicroVia()", false, VAL(0.0) }
};


static bool testEvalExpr( const wxString& expr, const LIBEVAL::VALUE& expectedResult,
                          bool expectError = false, BOARD_ITEM* itemA = nullptr,
                          BOARD_ITEM* itemB = nullptr )
{
    PCBEXPR_COMPILER  compiler( new PCBEXPR_UNIT_RESOLVER() );
    PCBEXPR_UCODE     ucode;
    PCBEXPR_CONTEXT   context( NULL_CONSTRAINT, UNDEFINED_LAYER );
    PCBEXPR_CONTEXT   preflightContext( NULL_CONSTRAINT, UNDEFINED_LAYER );
    bool              ok = true;

    context.SetItems( itemA, itemB );


    BOOST_TEST_MESSAGE( "Expr: '" << expr.c_str() << "'" );

    bool error = !compiler.Compile( expr, &ucode, &preflightContext );

    BOOST_CHECK_EQUAL( error, expectError );

    if( error != expectError )
    {
        BOOST_TEST_MESSAGE( "Result: FAIL: " << compiler.GetError().message.c_str() <<
                            " (code pos: " << compiler.GetError().srcPos << ")" );

        return false;
    }

    if( error )
        return true;

    LIBEVAL::VALUE* result;

    if( ok )
    {
        result = ucode.Run( &context );
        ok     = ( result->EqualTo( &context, &expectedResult ) );
    }

    if( expectedResult.GetType() == LIBEVAL::VT_NUMERIC )
    {
        BOOST_CHECK_EQUAL( result->AsDouble(), expectedResult.AsDouble() );
    }
    else
    {
        BOOST_CHECK_EQUAL( result->AsString(), expectedResult.AsString() );
    }

    return ok;
}


BOOST_DATA_TEST_CASE( SimpleExpressions, boost::unit_test::data::make( simpleExpressions ), expr )
{
    testEvalExpr( expr.expression, expr.expectedResult, expr.expectError );
}


BOOST_AUTO_TEST_CASE( IntrospectedProperties )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    BOARD brd;

    const NETINFO_LIST& netInfo = brd.GetNetInfo();

    std::shared_ptr<NETCLASS> netclass1( new NETCLASS( "HV_LINE" ) );
    std::shared_ptr<NETCLASS> netclass2( new NETCLASS( "otherClass" ) );

    auto net1info = new NETINFO_ITEM( &brd, "net1", 1 );
    auto net2info = new NETINFO_ITEM( &brd, "net2", 2 );

    net1info->SetNetClass( netclass1 );
    net2info->SetNetClass( netclass2 );

    PCB_TRACK trackA( &brd );
    PCB_TRACK trackB( &brd );

    trackA.SetNet( net1info );
    trackB.SetNet( net2info );

    trackB.SetLayer( F_Cu );

    trackA.SetWidth( pcbIUScale.MilsToIU( 10 ) );
    trackB.SetWidth( pcbIUScale.MilsToIU( 20 ) );

    for( const auto& expr : introspectionExpressions )
    {
        testEvalExpr( expr.expression, expr.expectedResult, expr.expectError, &trackA, &trackB );
    }
}

BOOST_AUTO_TEST_SUITE_END()
