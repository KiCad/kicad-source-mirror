/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <diff_merge/property_value_converter.h>

#include <geometry/eda_angle.h>
#include <gal/color4d.h>
#include <kiid.h>
#include <layer_ids.h>
#include <math/box2.h>
#include <math/vector2d.h>

#include <wx/any.h>
#include <wx/string.h>

#include <optional>


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( PropertyValueConverter )


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_Bool )
{
    DIFF_VALUE d = WxAnyToDiffValue( wxAny( true ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::BOOL );
    BOOST_CHECK_EQUAL( d.AsBool(), true );

    d = WxAnyToDiffValue( wxAny( false ) );
    BOOST_CHECK_EQUAL( d.AsBool(), false );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_Int )
{
    DIFF_VALUE d = WxAnyToDiffValue( wxAny( 42 ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::INT );
    BOOST_CHECK_EQUAL( d.AsInt(), 42 );

    d = WxAnyToDiffValue( wxAny( -7 ) );
    BOOST_CHECK_EQUAL( d.AsInt(), -7 );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_OptionalIntPresent )
{
    std::optional<int> opt = 100;
    DIFF_VALUE         d   = WxAnyToDiffValue( wxAny( opt ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::INT );
    BOOST_CHECK_EQUAL( d.AsInt(), 100 );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_OptionalIntEmpty )
{
    // Empty optional<int> distinguishes "unset" from 0 — should become T::NONE.
    std::optional<int> opt;
    DIFF_VALUE         d = WxAnyToDiffValue( wxAny( opt ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::NONE );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_OptionalDouble )
{
    std::optional<double> opt = 3.14;
    DIFF_VALUE            d   = WxAnyToDiffValue( wxAny( opt ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::DOUBLE );
    BOOST_CHECK_CLOSE( d.AsDouble(), 3.14, 0.001 );

    std::optional<double> empty;
    d = WxAnyToDiffValue( wxAny( empty ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::NONE );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_Double )
{
    DIFF_VALUE d = WxAnyToDiffValue( wxAny( 1.5 ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::DOUBLE );
    BOOST_CHECK_CLOSE( d.AsDouble(), 1.5, 0.001 );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_Float )
{
    float v = 2.5f;
    DIFF_VALUE d = WxAnyToDiffValue( wxAny( v ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::DOUBLE );
    BOOST_CHECK_CLOSE( d.AsDouble(), 2.5, 0.001 );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_WxString )
{
    DIFF_VALUE d = WxAnyToDiffValue( wxAny( wxString( "hello" ) ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::STRING );
    BOOST_CHECK( d.AsString() == wxS( "hello" ) );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_StdString )
{
    std::string s = "world";
    DIFF_VALUE  d = WxAnyToDiffValue( wxAny( s ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::STRING );
    BOOST_CHECK( d.AsString() == wxS( "world" ) );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_Kiid )
{
    KIID       k;
    DIFF_VALUE d = WxAnyToDiffValue( wxAny( k ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::KIID );
    BOOST_CHECK( d.AsKiid() == k );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_EdaAngle )
{
    EDA_ANGLE  ang( 45.0, DEGREES_T );
    DIFF_VALUE d = WxAnyToDiffValue( wxAny( ang ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::DOUBLE );
    BOOST_CHECK_CLOSE( d.AsDouble(), 45.0, 0.001 );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_Vector2I )
{
    VECTOR2I   v( 10, 20 );
    DIFF_VALUE d = WxAnyToDiffValue( wxAny( v ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::VECTOR2I );
    BOOST_CHECK( d.AsVector2I() == v );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_Box2I )
{
    // All four fields participate — y and height regressions previously
    // would have slipped past a partial-field assertion.
    BOX2I      b( VECTOR2I( 3, 7 ), VECTOR2I( 100, 200 ) );
    DIFF_VALUE d = WxAnyToDiffValue( wxAny( b ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::BOX2I );
    BOOST_CHECK( d.AsBox2I() == b );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_Color )
{
    KIGFX::COLOR4D c( 0.5, 0.25, 0.125, 0.75 );
    DIFF_VALUE     d = WxAnyToDiffValue( wxAny( c ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::COLOR );
    KIGFX::COLOR4D out = d.AsColor();
    BOOST_CHECK_CLOSE( out.r, 0.5, 0.001 );
    BOOST_CHECK_CLOSE( out.g, 0.25, 0.001 );
    BOOST_CHECK_CLOSE( out.b, 0.125, 0.001 );
    BOOST_CHECK_CLOSE( out.a, 0.75, 0.001 );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_PcbLayerId )
{
    PCB_LAYER_ID lyr = F_Cu;
    DIFF_VALUE   d   = WxAnyToDiffValue( wxAny( lyr ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::LAYER );
    BOOST_CHECK( d.AsLayer() == F_Cu );
}


BOOST_AUTO_TEST_CASE( WxAnyToDiffValue_UnsupportedReturnsNone )
{
    // wxAny holding an unsupported user type (no PROPERTY_BASE for enum
    // fallback) returns DIFF_VALUE::T::NONE.
    struct Custom { int n; };
    Custom     c{ 5 };
    DIFF_VALUE d = WxAnyToDiffValue( wxAny( c ) );
    BOOST_CHECK( d.GetType() == DIFF_VALUE::T::NONE );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_None )
{
    // A NONE value carries no payload; the converter must report failure and
    // leave the output untouched so the merge applier counts it as failed.
    wxAny out( wxString( "untouched" ) );
    BOOST_CHECK( !DiffValueToWxAny( DIFF_VALUE(), out ) );
    BOOST_CHECK( out.As<wxString>() == wxS( "untouched" ) );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_Bool )
{
    wxAny out;
    BOOST_CHECK( DiffValueToWxAny( DIFF_VALUE::FromBool( true ), out ) );
    BOOST_CHECK_EQUAL( out.As<bool>(), true );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_Int )
{
    wxAny out;
    BOOST_CHECK( DiffValueToWxAny( DIFF_VALUE::FromInt( -42 ), out ) );
    BOOST_CHECK_EQUAL( out.As<int>(), -42 );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_Int64 )
{
    wxAny out;
    BOOST_CHECK( DiffValueToWxAny( DIFF_VALUE::FromInt64( 1234567890123LL ), out ) );
    BOOST_CHECK( out.As<long long>() == 1234567890123LL );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_Double )
{
    wxAny out;
    BOOST_CHECK( DiffValueToWxAny( DIFF_VALUE::FromDouble( 2.5 ), out ) );
    BOOST_CHECK_CLOSE( out.As<double>(), 2.5, 0.001 );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_String )
{
    wxAny out;
    BOOST_CHECK( DiffValueToWxAny( DIFF_VALUE::FromString( wxString( "hi" ) ), out ) );
    BOOST_CHECK( out.As<wxString>() == wxS( "hi" ) );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_Kiid )
{
    KIID  k;
    wxAny out;
    BOOST_CHECK( DiffValueToWxAny( DIFF_VALUE::FromKiid( k ), out ) );
    BOOST_CHECK( out.As<KIID>() == k );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_Vector2I )
{
    VECTOR2I v( 11, 22 );
    wxAny    out;
    BOOST_CHECK( DiffValueToWxAny( DIFF_VALUE::FromVector2I( v ), out ) );
    BOOST_CHECK( out.As<VECTOR2I>() == v );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_Box2I )
{
    BOX2I b( VECTOR2I( 1, 2 ), VECTOR2I( 30, 40 ) );
    wxAny out;
    BOOST_CHECK( DiffValueToWxAny( DIFF_VALUE::FromBox2I( b ), out ) );
    BOOST_CHECK( out.As<BOX2I>() == b );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_Color )
{
    KIGFX::COLOR4D c( 0.1, 0.2, 0.3, 0.4 );
    wxAny          out;
    BOOST_CHECK( DiffValueToWxAny( DIFF_VALUE::FromColor( c ), out ) );
    BOOST_CHECK( out.As<KIGFX::COLOR4D>() == c );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_Layer )
{
    wxAny out;
    BOOST_CHECK( DiffValueToWxAny( DIFF_VALUE::FromLayer( B_Cu ), out ) );
    BOOST_CHECK( out.As<PCB_LAYER_ID>() == B_Cu );
}


BOOST_AUTO_TEST_CASE( DiffValueToWxAny_Enum )
{
    // The enum payload's integer is the canonical value a PROPERTY_ENUM setter
    // consumes; the label is dropped on the way back to wxAny.
    wxAny out;
    BOOST_CHECK( DiffValueToWxAny( DIFF_VALUE::FromEnum( 7, "Seven" ), out ) );
    BOOST_CHECK_EQUAL( out.As<int>(), 7 );
}


BOOST_AUTO_TEST_SUITE_END()
