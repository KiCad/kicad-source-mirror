/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file
 * Unit tests for the constraint-panel display label (issue #2329 UI redesign): a valueless
 * constraint reads as its type name, a driving valued constraint appends its value in the
 * requested display units, and a reference (non-driving) value is parenthesized.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <set>

#include <base_units.h>
#include <eda_units.h>

#include <pcb_shape.h>

#include <constraints/pcb_constraint.h>
#include <constraints/constraint_builder.h>

BOOST_AUTO_TEST_SUITE( ConstraintLabels )


BOOST_AUTO_TEST_CASE( ValuelessConstraintIsTypeNameOnly )
{
    PCB_CONSTRAINT constraint( nullptr, PCB_CONSTRAINT_TYPE::PARALLEL );

    BOOST_TEST( !constraint.HasValue() );
    BOOST_CHECK_EQUAL( ConstraintDisplayLabel( constraint, EDA_UNITS::MM ), wxString( "Parallel" ) );
}


BOOST_AUTO_TEST_CASE( DrivingLengthShowsValueInDisplayUnits )
{
    PCB_CONSTRAINT constraint( nullptr, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    constraint.SetValue( pcbIUScale.mmToIU( 12.0 ) );
    constraint.SetDriving( true );

    wxString mm = ConstraintDisplayLabel( constraint, EDA_UNITS::MM );
    BOOST_TEST( mm.StartsWith( "Fixed length: " ) );
    BOOST_TEST( mm.Contains( "12" ) );
    BOOST_TEST( mm.Contains( "mm" ) );

    // A length follows the requested display units, not a fixed unit.
    wxString in = ConstraintDisplayLabel( constraint, EDA_UNITS::INCH );
    BOOST_TEST( in.Contains( "in" ) );
    BOOST_TEST( !in.Contains( "mm" ) );
}


BOOST_AUTO_TEST_CASE( ReferenceValueIsParenthesized )
{
    PCB_CONSTRAINT constraint( nullptr, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    constraint.SetValue( pcbIUScale.mmToIU( 8.0 ) );
    constraint.SetDriving( false );

    wxString label = ConstraintDisplayLabel( constraint, EDA_UNITS::MM );
    BOOST_TEST( label.Contains( "(" ) );
    BOOST_TEST( label.Contains( ")" ) );
    BOOST_TEST( label.Contains( "8" ) );
}


BOOST_AUTO_TEST_CASE( AngularValueShownInDegrees )
{
    PCB_CONSTRAINT constraint( nullptr, PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION );
    constraint.SetValue( 45.0 );
    constraint.SetDriving( true );

    // An angle ignores the length display units and reads in degrees.
    wxString label = ConstraintDisplayLabel( constraint, EDA_UNITS::MM );
    BOOST_TEST( label.StartsWith( "Angular dimension: " ) );
    BOOST_TEST( label.Contains( "45" ) );
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_CASE( TypeGlyphIsSingleCodepointAndDistinct )
{
    // Each badge is one codepoint carried by both the newstroke stroke font and the OpenGL glyph
    // atlas, and unique so a badge is unambiguous. A multi-character mark would smear across the
    // fixed-size badge and read poorly at any zoom.
    std::set<wxString> seen;

    for( int i = 1; i <= static_cast<int>( PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION ); ++i )
    {
        wxString glyph = ConstraintTypeGlyph( static_cast<PCB_CONSTRAINT_TYPE>( i ) );

        BOOST_TEST( !glyph.IsEmpty() );
        BOOST_TEST( glyph.length() == 1, "multi-codepoint glyph: " << glyph.ToStdString() );
        BOOST_TEST( seen.insert( glyph ).second, "duplicate glyph: " << glyph.ToStdString() );
    }
}


BOOST_AUTO_TEST_SUITE( ConstraintShapeAnchorsTests )


BOOST_AUTO_TEST_CASE( SegmentExposesStartAndEnd )
{
    PCB_SHAPE seg( nullptr, SHAPE_T::SEGMENT );
    seg.SetStart( VECTOR2I( 0, 0 ) );
    seg.SetEnd( VECTOR2I( 1000, 2000 ) );

    std::vector<CONSTRAINT_ANCHOR_POINT> anchors = ConstraintShapeAnchors( &seg );

    BOOST_REQUIRE_EQUAL( anchors.size(), 2 );
    BOOST_CHECK( anchors[0].anchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK( anchors[0].pos == VECTOR2I( 0, 0 ) );
    BOOST_CHECK( anchors[1].anchor == CONSTRAINT_ANCHOR::END );
    BOOST_CHECK( anchors[1].pos == VECTOR2I( 1000, 2000 ) );
}


BOOST_AUTO_TEST_CASE( CircleExposesOnlyCenter )
{
    PCB_SHAPE circle( nullptr, SHAPE_T::CIRCLE );
    circle.SetCenter( VECTOR2I( 500, 500 ) );
    circle.SetEnd( VECTOR2I( 500, 1500 ) );   // radius handle

    std::vector<CONSTRAINT_ANCHOR_POINT> anchors = ConstraintShapeAnchors( &circle );

    BOOST_REQUIRE_EQUAL( anchors.size(), 1 );
    BOOST_CHECK( anchors[0].anchor == CONSTRAINT_ANCHOR::CENTER );
    BOOST_CHECK( anchors[0].pos == VECTOR2I( 500, 500 ) );
}


BOOST_AUTO_TEST_CASE( NonAnchoredShapeYieldsNothing )
{
    PCB_SHAPE rect( nullptr, SHAPE_T::RECTANGLE );
    rect.SetStart( VECTOR2I( 0, 0 ) );
    rect.SetEnd( VECTOR2I( 1000, 1000 ) );

    BOOST_CHECK( ConstraintShapeAnchors( &rect ).empty() );
}


BOOST_AUTO_TEST_SUITE_END()
