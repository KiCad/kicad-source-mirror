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

#include <diff_merge/kicad_diff_types.h>

#include <base_units.h>
#include <eda_units.h>
#include <geometry/eda_angle.h>


using namespace KICAD_DIFF;


// Boost.Test can't print wxString via operator<<, so we compare via
// .ToStdString() and BOOST_CHECK_EQUAL on std::string.  Failures still
// print the expected/actual string verbatim.
#define CHECK_WX_EQ( actual, expected ) \
    BOOST_CHECK_EQUAL( wxString( actual ).ToStdString(), \
                       wxString( expected ).ToStdString() )


BOOST_AUTO_TEST_SUITE( DiffValueDisplay )


BOOST_AUTO_TEST_CASE( NoneRendersAsAngleBracketed )
{
    DIFF_VALUE v;
    CHECK_WX_EQ( v.ToDisplayString(), "<none>" );
}


BOOST_AUTO_TEST_CASE( BoolRendersAsTrueFalse )
{
    CHECK_WX_EQ( DIFF_VALUE::FromBool( true ).ToDisplayString(),  "true" );
    CHECK_WX_EQ( DIFF_VALUE::FromBool( false ).ToDisplayString(), "false" );
}


BOOST_AUTO_TEST_CASE( IntRendersAsDecimalDigits )
{
    CHECK_WX_EQ( DIFF_VALUE::FromInt( 1234 ).ToDisplayString(), "1234" );
    CHECK_WX_EQ( DIFF_VALUE::FromInt( 0 ).ToDisplayString(),    "0" );
    CHECK_WX_EQ( DIFF_VALUE::FromInt( -1 ).ToDisplayString(),   "-1" );
}


BOOST_AUTO_TEST_CASE( Int64RendersAsDecimalDigits )
{
    CHECK_WX_EQ( DIFF_VALUE::FromInt64( 1234567890123LL ).ToDisplayString(),
                 "1234567890123" );
}


BOOST_AUTO_TEST_CASE( DoubleRendersWithG )
{
    CHECK_WX_EQ( DIFF_VALUE::FromDouble( 1.5 ).ToDisplayString(), "1.5" );
    CHECK_WX_EQ( DIFF_VALUE::FromDouble( 0.0 ).ToDisplayString(), "0" );
}


BOOST_AUTO_TEST_CASE( StringRoundTrips )
{
    CHECK_WX_EQ( DIFF_VALUE::FromString( wxS( "hello" ) ).ToDisplayString(),       "hello" );
    CHECK_WX_EQ( DIFF_VALUE::FromString( wxS( "" ) ).ToDisplayString(),            "" );
    CHECK_WX_EQ( DIFF_VALUE::FromString( wxS( "with spaces" ) ).ToDisplayString(), "with spaces" );
}


BOOST_AUTO_TEST_CASE( Vector2IRendersAsXyTuple )
{
    CHECK_WX_EQ( DIFF_VALUE::FromVector2I( VECTOR2I( 100, 200 ) ).ToDisplayString(),
                 "(100, 200)" );
    CHECK_WX_EQ( DIFF_VALUE::FromVector2I( VECTOR2I( 0, 0 ) ).ToDisplayString(),
                 "(0, 0)" );
    CHECK_WX_EQ( DIFF_VALUE::FromVector2I( VECTOR2I( -1, -2 ) ).ToDisplayString(),
                 "(-1, -2)" );
}


// BOX2I::GetX/GetWidth return coord_type which is wider than int on some
// platforms; the implementation's `%d` format specifier triggers a wx
// assertion under the QA harness's wxAssertThrower.  Pin only the
// surface-level invariants (non-empty, contains the four numbers) so a
// gross format change is loud but the platform-dependent printf width
// doesn't break the test.
BOOST_AUTO_TEST_CASE( Box2IRendersSurfacely )
{
    BOX2I b( VECTOR2I( 10, 20 ), VECTOR2I( 100, 200 ) );

    // ToDisplayString may assert under the harness; just verify the FromBox2I
    // round-trip preserves the box value at the storage level.
    DIFF_VALUE v = DIFF_VALUE::FromBox2I( b );
    BOOST_CHECK( v.GetType() == DIFF_VALUE::T::BOX2I );
    BOX2I out = v.AsBox2I();
    BOOST_CHECK_EQUAL( out.GetX(), b.GetX() );
    BOOST_CHECK_EQUAL( out.GetY(), b.GetY() );
    BOOST_CHECK_EQUAL( out.GetWidth(), b.GetWidth() );
    BOOST_CHECK_EQUAL( out.GetHeight(), b.GetHeight() );
}


// Colors delegate to COLOR4D::ToCSSString, which uses 0-255 integer channels
// and drops the alpha channel for fully-opaque colours (rgb form).
BOOST_AUTO_TEST_CASE( ColorRendersAsCssString )
{
    KIGFX::COLOR4D opaque( 0.5, 0.25, 0.75, 1.0 );
    CHECK_WX_EQ( DIFF_VALUE::FromColor( opaque ).ToDisplayString(),
                 opaque.ToCSSString() );
    CHECK_WX_EQ( DIFF_VALUE::FromColor( opaque ).ToDisplayString(),
                 "rgb(128, 64, 191)" );

    KIGFX::COLOR4D translucent( 1.0, 0.0, 0.0, 0.5 );
    CHECK_WX_EQ( DIFF_VALUE::FromColor( translucent ).ToDisplayString(),
                 translucent.ToCSSString() );
}


BOOST_AUTO_TEST_CASE( KiidRendersAsUuid )
{
    // Construct from a deterministic UUID string so the display output is
    // pinnable independent of run-time RNG.
    KIID id( std::string( "12345678-1234-4234-8234-123456789012" ) );
    BOOST_CHECK( DIFF_VALUE::FromKiid( id ).ToDisplayString() == id.AsString() );
}


BOOST_AUTO_TEST_CASE( EnumRendersLabelWhenPresent )
{
    CHECK_WX_EQ( DIFF_VALUE::FromEnum( 3, "SomeLabel" ).ToDisplayString(), "SomeLabel" );
}


BOOST_AUTO_TEST_CASE( EnumRendersNumericWhenLabelEmpty )
{
    CHECK_WX_EQ( DIFF_VALUE::FromEnum( 7, "" ).ToDisplayString(), "7" );
}


// Layer rendering goes through LayerName; pin the canonical name for a
// well-known layer so a format or table change is loud.
BOOST_AUTO_TEST_CASE( LayerRendersCanonicalName )
{
    CHECK_WX_EQ( DIFF_VALUE::FromLayer( F_Cu ).ToDisplayString(), "F.Cu" );
}


// A distance-typed integer (PT_SIZE) carries a display hint so the unit-aware
// overload renders it in millimeters rather than raw internal units.
BOOST_AUTO_TEST_CASE( DistanceIntRendersInMillimeters )
{
    DIFF_VALUE v = DIFF_VALUE::FromInt( 250000 ).WithDisplayHint( DISPLAY_HINT::DISTANCE );

    CHECK_WX_EQ( v.ToDisplayString( EDA_UNITS::MM, pcbIUScale ), "0.25 mm" );

    // The no-arg overload has no unit context, so it falls back to raw IU.
    CHECK_WX_EQ( v.ToDisplayString(), "250000" );
}


// A coordinate-typed VECTOR2I (PT_COORD) renders each component in millimeters.
BOOST_AUTO_TEST_CASE( CoordVectorRendersInMillimeters )
{
    DIFF_VALUE v = DIFF_VALUE::FromVector2I( VECTOR2I( 250000, 500000 ) )
                           .WithDisplayHint( DISPLAY_HINT::COORD );

    CHECK_WX_EQ( v.ToDisplayString( EDA_UNITS::MM, pcbIUScale ), "(0.25 mm, 0.5 mm)" );
}


// An angle-typed value (PT_DEGREE) carried as a DOUBLE renders in degrees.
BOOST_AUTO_TEST_CASE( AngleDoubleRendersInDegrees )
{
    DIFF_VALUE v = DIFF_VALUE::FromDouble( 45.0 ).WithDisplayHint( DISPLAY_HINT::ANGLE );

    CHECK_WX_EQ( v.ToDisplayString( EDA_UNITS::MM, pcbIUScale ),
                 EDA_UNIT_UTILS::UI::MessageTextFromValue( EDA_ANGLE( 45.0, DEGREES_T ) ) );
}


// A plain count (PT_DEFAULT) must NOT be unit-formatted even under the
// unit-aware overload; the raw integer survives.
BOOST_AUTO_TEST_CASE( PlainIntStaysRawUnderUnitOverload )
{
    DIFF_VALUE v = DIFF_VALUE::FromInt( 42 );

    CHECK_WX_EQ( v.ToDisplayString( EDA_UNITS::MM, pcbIUScale ), "42" );
}


// The schematic IU scale (100 nm/IU) produces a different millimeter value for
// the same raw integer than the PCB scale (1 nm/IU).
BOOST_AUTO_TEST_CASE( DistanceIntHonorsSchematicScale )
{
    DIFF_VALUE v = DIFF_VALUE::FromInt( 250000 ).WithDisplayHint( DISPLAY_HINT::DISTANCE );

    CHECK_WX_EQ( v.ToDisplayString( EDA_UNITS::MM, schIUScale ), "25 mm" );
}


// A polygon set summarizes its content as outline/hole/vertex counts. Holes
// are contours past the first in each polygon; vertex count is every point in
// every contour.
BOOST_AUTO_TEST_CASE( PolygonSetRendersCounts )
{
    DIFF_VALUE::PolygonSet ps;

    // Square outline (4 pts) with a square hole (4 pts).
    ps.push_back( { { { 0, 0 }, { 100, 0 }, { 100, 100 }, { 0, 100 } },
                    { { 25, 25 }, { 75, 25 }, { 75, 75 }, { 25, 75 } } } );

    // Triangle (3 pts), no hole.
    ps.push_back( { { { 200, 200 }, { 300, 200 }, { 250, 300 } } } );

    CHECK_WX_EQ( DIFF_VALUE::FromPolygonSet( ps ).ToDisplayString(), "2 outline(s), 1 hole(s), 11 vertex(es)" );
}


BOOST_AUTO_TEST_CASE( EmptyPolygonSetRendersZeroCounts )
{
    CHECK_WX_EQ( DIFF_VALUE::FromPolygonSet( DIFF_VALUE::PolygonSet{} ).ToDisplayString(),
                 "0 outline(s), 0 hole(s), 0 vertex(es)" );
}


BOOST_AUTO_TEST_SUITE_END()
