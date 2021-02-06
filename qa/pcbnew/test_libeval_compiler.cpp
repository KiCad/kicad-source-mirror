/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <wx/wx.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <layers_id_colors_and_visibility.h>
#include <pcbnew/pcb_expr_evaluator.h>

#include <pcbnew/board.h>
#include <pcbnew/track.h>

BOOST_AUTO_TEST_SUITE( Libeval_Compiler )

struct EXPR_TO_TEST
{
    wxString       expression;
    bool           expectError;
    LIBEVAL::VALUE expectedResult;
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
    { "A.Width + B.Width", false, VAL( Mils2iu(10) + Mils2iu(20) ) },
    { "A.Netclass", false, VAL( "HV" ) },
    { "(A.Netclass == 'HV') && (B.netclass == 'otherClass') && (B.netclass != 'F.Cu')", false, VAL( 1.0 ) },
    { "A.Netclass + 1.0", false, VAL( 1.0 ) },
    { "A.type == 'Track' && B.type == 'Track' && A.layer == 'F.Cu'", false, VAL( 1.0 ) },
    { "(A.type == 'Track') && (B.type == 'Track') && (A.layer == 'F.Cu')", false, VAL( 1.0 ) },
    { "A.type == 'Via' && A.isMicroVia()", false, VAL(0.0) }
};


static bool testEvalExpr( const wxString& expr, LIBEVAL::VALUE expectedResult,
                          bool expectError = false, BOARD_ITEM* itemA = nullptr,
                          BOARD_ITEM* itemB = nullptr )
{
    PCB_EXPR_COMPILER compiler;
    PCB_EXPR_UCODE    ucode;
    PCB_EXPR_CONTEXT  context, preflightContext;
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

    LIBEVAL::VALUE result;

    if( ok )
    {
        result = *ucode.Run( &context );
        ok     = ( result.EqualTo( &expectedResult ) );
    }


    if( expectedResult.GetType() == LIBEVAL::VT_NUMERIC )
    {
        BOOST_CHECK_EQUAL( result.AsDouble(), expectedResult.AsDouble() );
    }
    else
    {
        BOOST_CHECK_EQUAL( result.AsString(), expectedResult.AsString() );
    }


    return ok;
}

BOOST_AUTO_TEST_CASE( SimpleExpressions )
{
    for( const auto& expr : simpleExpressions )
    {
        testEvalExpr( expr.expression, expr.expectedResult, expr.expectError );
    }
}

BOOST_AUTO_TEST_CASE( IntrospectedProperties )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    BOARD brd;

    NETINFO_LIST& netInfo = brd.GetNetInfo();

    NETCLASSPTR netclass1( new NETCLASS( "HV" ) );
    NETCLASSPTR netclass2( new NETCLASS( "otherClass" ) );

    auto net1info = new NETINFO_ITEM( &brd, "net1", 1 );
    auto net2info = new NETINFO_ITEM( &brd, "net2", 2 );

    net1info->SetNetClass( netclass1 );
    net2info->SetNetClass( netclass2 );

    TRACK trackA( &brd );
    TRACK trackB( &brd );

    trackA.SetNet( net1info );
    trackB.SetNet( net2info );

    trackB.SetLayer( F_Cu );

    trackA.SetWidth( Mils2iu( 10 ) );
    trackB.SetWidth( Mils2iu( 20 ) );

    for( const auto& expr : introspectionExpressions )
    {
        testEvalExpr( expr.expression, expr.expectedResult, expr.expectError, &trackA, &trackB );
    }
}

BOOST_AUTO_TEST_SUITE_END()
