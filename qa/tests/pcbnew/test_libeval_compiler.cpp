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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/data/test_case.hpp>

#include <wx/wx.h>

#include <layer_ids.h>
#include <gal/color4d.h>
#include <geometry/eda_angle.h>
#include <pcbnew/pcbexpr_evaluator.h>
#include <drc/drc_rule.h>
#include <pcbnew/board.h>
#include <board_design_settings.h>
#include <pcbnew/pcb_shape.h>
#include <pcbnew/pcb_table.h>
#include <pcbnew/pcb_track.h>
#include <pcbnew/footprint.h>
#include <pcbnew/pcb_text.h>
#include <pcbnew/zone.h>
#include <project/net_settings.h>
#include <properties/property.h>
#include <properties/property_mgr.h>

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
    { "+2 - 1", false, VAL(1) },
    // A short-circuited || must yield a normalized 1, not the raw (nonzero) left operand, so a
    // boolean feeding a further operator behaves the same as the non-short-circuited path.
    { "(2 || 0) == 1", false, VAL(1) },
    { "(7 || 0) + 5", false, VAL(6) }
};


const static std::vector<EXPR_TO_TEST> introspectionExpressions = {
    { "A.type == 'Pad' && B.type == 'Pad' && (A.existsOnLayer('F.Cu'))", false, VAL( 0.0 ) },
    { "A.Width > B.Width", false, VAL( 0.0 ) },
    { "A.Width + B.Width", false, VAL( pcbIUScale.MilsToIU(10) + pcbIUScale.MilsToIU(20) ) },
    { "A.Netclass", false, VAL( "HV_LINE" ) },
    { "A.Net_Class == 'HV_LINE'", false, VAL( 1.0 ) },
    { "(A.Netclass == 'HV_LINE') && (B.netclass == 'otherClass') && (B.netclass != 'F.Cu')", false, VAL( 1.0 ) },
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


static void expectCompileError( const wxString& aExpr )
{
    PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER() );
    PCBEXPR_UCODE    ucode;
    PCBEXPR_CONTEXT  preflight( NULL_CONSTRAINT, UNDEFINED_LAYER );

    compiler.Compile( aExpr, &ucode, &preflight );

    BOOST_CHECK_MESSAGE( compiler.IsErrorPending(), "Expected a compile error for: " << aExpr.mb_str() );
}


static void expectCompileSuccess( const wxString& aExpr )
{
    PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER() );
    PCBEXPR_UCODE    ucode;
    PCBEXPR_CONTEXT  preflight( NULL_CONSTRAINT, UNDEFINED_LAYER );

    compiler.Compile( aExpr, &ucode, &preflight );

    BOOST_CHECK_MESSAGE( !compiler.IsErrorPending(), "Expected expression to compile: " << aExpr.mb_str() );
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

BOOST_AUTO_TEST_CASE( IntrospectedExtendedNumericProperties )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    BOARD brd;
    ZONE  zone( &brd );

    zone.SetLayer( F_Cu );
    zone.SetAssignedPriority( 7 );
    zone.SetHatchOrientation( EDA_ANGLE( 45.0, DEGREES_T ) );
    zone.SetMinIslandArea( 123456789LL );

    testEvalExpr( wxT( "A.Priority == 7" ), VAL( 1.0 ), false, &zone );
    testEvalExpr( wxT( "A.Hatch_Orientation == 45deg" ), VAL( 1.0 ), false, &zone );
    testEvalExpr( wxT( "A.Minimum_Island_Area == 123456789" ), VAL( 1.0 ), false, &zone );

    PCB_SHAPE ellipse( &brd, SHAPE_T::ELLIPSE );
    ellipse.SetEllipseRotation( EDA_ANGLE( 30.0, DEGREES_T ) );

    testEvalExpr( wxT( "A.Ellipse_Rotation == 30deg" ), VAL( 1.0 ), false, &ellipse );

    FOOTPRINT footprint( &brd );
    footprint.SetLocalSolderPasteMarginRatio( 0.125 );

    testEvalExpr( wxT( "A.Solderpaste_Margin_Ratio_Override == 0.125" ), VAL( 1.0 ), false,
                  &footprint );

    footprint.SetLocalSolderPasteMarginRatio( std::nullopt );

    testEvalExpr( wxT( "A.Solderpaste_Margin_Ratio_Override == null" ), VAL( 1.0 ), false,
                  &footprint );
}

BOOST_AUTO_TEST_CASE( IntrospectedColorProperties )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    BOARD     brd;
    PCB_TABLE table( &brd, 0 );

    const auto checkColor = [&]( const COLOR4D& aColor, const wxString& aExpectedCss )
    {
        table.SetBorderColor( aColor );
        BOOST_CHECK_EQUAL( aColor.ToCSSString(), aExpectedCss );

        wxString expression = wxT( "A.Border_Color == '" ) + aExpectedCss + wxT( "'" );
        testEvalExpr( expression, VAL( 1.0 ), false, &table );
    };

    checkColor( COLOR4D( 0.25, 0.5, 0.75, 0.5 ), wxT( "rgba(64, 128, 191, 0.502)" ) );
    checkColor( COLOR4D( wxString( wxT( "red" ) ) ), wxT( "rgb(255, 0, 0)" ) );
}

BOOST_AUTO_TEST_CASE( ExpressionPropertyTypesAreSupported )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    std::set<std::pair<TYPE_ID, PROPERTY_BASE*>> seen;
    std::map<wxString, std::set<LIBEVAL::VAR_TYPE_T>> fieldTypes;

    for( const PROPERTY_MANAGER::CLASS_INFO& cls : propMgr.GetAllClasses() )
    {
        if( !propMgr.IsOfType( cls.type, TYPE_HASH( BOARD_ITEM ) ) )
            continue;

        for( PROPERTY_BASE* prop : cls.properties )
        {
            if( !seen.emplace( cls.type, prop ).second )
                continue;

            PCBEXPR_PROPERTY_KIND kind = PCBEXPR_VAR_REF::ClassifyProperty( prop );

            BOOST_CHECK_MESSAGE( kind != PCBEXPR_PROPERTY_KIND::UNSUPPORTED,
                                 "Unsupported expression property: class=" << cls.name.mb_str()
                                                                           << ", property=" << prop->Name().mb_str()
                                                                           << ", type_hash=" << prop->TypeHash() );

            if( kind != PCBEXPR_PROPERTY_KIND::UNSUPPORTED )
                fieldTypes[prop->Name()].insert( PCBEXPR_VAR_REF::ExpressionType( kind ) );
        }
    }

    // PCB_TARGET::Shape is numeric while EDA_SHAPE::Shape is an enum, so a global
    // expression reference cannot have one stable public type.
    auto shapeTypes = fieldTypes.find( wxT( "Shape" ) );
    BOOST_REQUIRE( shapeTypes != fieldTypes.end() );
    BOOST_CHECK_EQUAL( shapeTypes->second.size(), 2u );
    fieldTypes.erase( shapeTypes );

    for( const auto& [field, types] : fieldTypes )
    {
        BOOST_CHECK_MESSAGE( types.size() == 1,
                             "Incompatible expression types for property: " << field.mb_str() );
    }

    expectCompileError( wxT( "A.Shape" ) );
}

BOOST_AUTO_TEST_CASE( IntrospectedPropertyAvailability )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    BOARD brd;
    ZONE  zone( &brd );

    zone.SetLayer( F_Cu );
    zone.SetAssignedPriority( 7 );
    zone.SetIsRuleArea( true );

    testEvalExpr( wxT( "A.Priority == 7" ), VAL( 0.0 ), false, &zone );

    zone.SetIsRuleArea( false );

    testEvalExpr( wxT( "A.Priority == 7" ), VAL( 1.0 ), false, &zone );
}

BOOST_AUTO_TEST_CASE( InNetChainClassWildcard )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    BOARD brd;

    std::shared_ptr<NET_SETTINGS> netSettings = brd.GetDesignSettings().m_NetSettings;
    netSettings->SetNetChainClass( wxT( "ChainHS" ), wxT( "HighSpeed" ) );

    auto netUnclassified = new NETINFO_ITEM( &brd, "netA", 1 );
    auto netClassified   = new NETINFO_ITEM( &brd, "netB", 2 );
    auto netNoChain      = new NETINFO_ITEM( &brd, "netC", 3 );

    netUnclassified->SetNetChain( wxT( "ChainOrphan" ) );
    netClassified->SetNetChain( wxT( "ChainHS" ) );

    PCB_TRACK trackUnclassified( &brd );
    PCB_TRACK trackClassified( &brd );
    PCB_TRACK trackNoChain( &brd );

    trackUnclassified.SetNet( netUnclassified );
    trackClassified.SetNet( netClassified );
    trackNoChain.SetNet( netNoChain );

    // A chain with no class assignment must not match any inNetChainClass() pattern,
    // including the '*' wildcard.
    testEvalExpr( wxT( "A.inNetChainClass('*')" ), VAL( 0.0 ), false, &trackUnclassified,
                  &trackUnclassified );
    testEvalExpr( wxT( "A.inNetChainClass('HighSpeed')" ), VAL( 0.0 ), false, &trackUnclassified,
                  &trackUnclassified );

    // Net with no chain at all must not match either.
    testEvalExpr( wxT( "A.inNetChainClass('*')" ), VAL( 0.0 ), false, &trackNoChain,
                  &trackNoChain );

    // Properly classified chain must match both wildcard and exact patterns.
    testEvalExpr( wxT( "A.inNetChainClass('*')" ), VAL( 1.0 ), false, &trackClassified,
                  &trackClassified );
    testEvalExpr( wxT( "A.inNetChainClass('HighSpeed')" ), VAL( 1.0 ), false, &trackClassified,
                  &trackClassified );
    testEvalExpr( wxT( "A.inNetChainClass('High*')" ), VAL( 1.0 ), false, &trackClassified,
                  &trackClassified );
    testEvalExpr( wxT( "A.inNetChainClass('LowSpeed')" ), VAL( 0.0 ), false, &trackClassified,
                  &trackClassified );
}

BOOST_AUTO_TEST_CASE( ParentNavigation )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    BOARD brd;

    FOOTPRINT fp( &brd );
    fp.SetReference( wxT( "J1" ) );

    // A text item living inside the footprint.  Its direct parent is the footprint, so
    // "A.Parent" navigates to J1.
    PCB_TEXT* text = new PCB_TEXT( &fp );
    text->SetText( wxT( "J1" ) );
    fp.Add( text );

    // The headline capability: getField() only returns a value when its receiver is a
    // footprint, so this passes solely because navigation steps from the text to its parent.
    testEvalExpr( wxT( "A.Parent.getField('Reference') == 'J1'" ), VAL( 1.0 ), false, text, text );

    // The exact pattern from the issue (parent field compared to the text's own value).
    testEvalExpr( wxT( "A.Parent.getField('Reference') == A.Text" ), VAL( 1.0 ), false, text, text );

    // The parent resolves to a footprint object that can be type-queried.
    testEvalExpr( wxT( "A.Parent.Type == 'Footprint'" ), VAL( 1.0 ), false, text, text );

    // Regression: the terminal "Parent" string still yields the parent footprint reference,
    // i.e. the object and the string refer to the same parent.
    testEvalExpr( wxT( "A.Parent == 'J1'" ), VAL( 1.0 ), false, text, text );

    // Without navigation getField() has a non-footprint receiver and returns "", so this must
    // NOT match - it guards against the navigation silently applying to the base item.
    testEvalExpr( wxT( "A.getField('Reference') == 'J1'" ), VAL( 0.0 ), false, text, text );

    // Error cases.  These are code-generation errors (not parse errors), which the shared
    // testEvalExpr does not observe through Compile()'s return value, so check the pending-error
    // status directly.  None of these expressions contain a bare number, so the only error that
    // can be raised is the unrecognized item/property we are testing for.
    // An unknown property on the parent, an unknown navigation step, and navigation on the
    // layer pseudo-item (which has no parent).
    expectCompileError( wxT( "A.Parent.bogusProperty" ) );
    expectCompileError( wxT( "A.bogus.Reference == 'J1'" ) );
    expectCompileError( wxT( "L.Parent.Type == 'Footprint'" ) );
}


BOOST_AUTO_TEST_CASE( ReceiverValidation )
{
    expectCompileSuccess( wxT( "L == 'F.Cu'" ) );
    expectCompileSuccess( wxT( "AB.isCoupledDiffPair()" ) );
    expectCompileSuccess( wxT( "A.Parent.Reference == 'J1'" ) );
    expectCompileSuccess( wxT( "A.Width == 1mm" ) );

    expectCompileError( wxT( "L.Width == 1mm" ) );
    expectCompileError( wxT( "AB.Width == A.Width" ) );
    expectCompileError( wxT( "AB.Parent.Reference == 'J1'" ) );
}


BOOST_AUTO_TEST_CASE( PairItemDependency )
{
    auto requiresPairItems =
            []( const wxString& aExpr )
            {
                PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER() );
                PCBEXPR_UCODE    ucode;
                PCBEXPR_CONTEXT  preflight( NULL_CONSTRAINT, UNDEFINED_LAYER );

                BOOST_REQUIRE( compiler.Compile( aExpr, &ucode, &preflight ) );
                return ucode.RequiresPairItems();
            };

    BOOST_CHECK( !requiresPairItems( wxT( "A.Width == 1mm" ) ) );
    BOOST_CHECK( requiresPairItems( wxT( "B.Width == 1mm" ) ) );
    BOOST_CHECK( requiresPairItems( wxT( "AB.isCoupledDiffPair()" ) ) );
    BOOST_CHECK( !requiresPairItems( wxT( "A.Reference == 'B.Width'" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
