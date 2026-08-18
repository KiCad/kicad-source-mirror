/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <boost/test/unit_test.hpp>

#include <drc/drc_rtree.h>
#include <pcb_shape.h>
#include <geometry/shape_segment.h>
#include <properties/property_mgr.h>
#include <properties/property.h>
#include <i18n_utility.h>

BOOST_AUTO_TEST_CASE( PCBShapeCornerRadius )
{
    PCB_SHAPE shape( nullptr, SHAPE_T::RECTANGLE );
    shape.SetStart( VECTOR2I( 0, 0 ) );
    shape.SetEnd( VECTOR2I( 1000, 600 ) );
    shape.SetCornerRadius( 200 );
    BOOST_CHECK_EQUAL( shape.GetCornerRadius(), 200 );

    shape.SetCornerRadius( 400 );
    BOOST_CHECK_EQUAL( shape.GetCornerRadius(), 300 );

    shape.SetCornerRadius( -10 );
    BOOST_CHECK_EQUAL( shape.GetCornerRadius(), 0 );
}

BOOST_AUTO_TEST_CASE( PCBShapeCornerRadiusValidation )
{
    PCB_SHAPE shape( nullptr, SHAPE_T::RECTANGLE );
    shape.SetStart( VECTOR2I( 0, 0 ) );
    shape.SetEnd( VECTOR2I( 1000, 500 ) );

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    PROPERTY_BASE* prop = propMgr.GetProperty( TYPE_HASH( EDA_SHAPE ), _HKI( "Corner Radius" ) );

    auto resultTooLarge = prop->Validate( wxAny( 400 ), &shape );
    BOOST_CHECK( resultTooLarge.has_value() );

    auto resultOK = prop->Validate( wxAny( 200 ), &shape );
    BOOST_CHECK( !resultOK.has_value() );
}


BOOST_AUTO_TEST_CASE( ShapeEffectiveGeometryHitsLineEnding )
{
    PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
    shape.SetLayer( F_Cu );
    shape.SetStart( VECTOR2I( 0, 0 ) );
    shape.SetEnd( VECTOR2I( 1000, 0 ) );
    shape.SetWidth( 100 );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::SQUARE );
    shape.SetStartEndingLength( 400 );
    shape.SetStartEndingWidth( 400 );

    std::shared_ptr<SHAPE> effectiveShape = shape.GetEffectiveShape( F_Cu );
    SHAPE_SEGMENT          probe( VECTOR2I( 0, -150 ), VECTOR2I( 0, 150 ), 20 );

    BOOST_REQUIRE( effectiveShape );
    BOOST_CHECK( effectiveShape->Collide( probe.GetSeg(), 0 ) );
}


BOOST_AUTO_TEST_CASE( PCBShapeLineEndingRTreeQuery )
{
    PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
    shape.SetLayer( F_Cu );
    shape.SetStart( VECTOR2I( 0, 0 ) );
    shape.SetEnd( VECTOR2I( 1000, 0 ) );
    shape.SetWidth( 100 );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::SQUARE );
    shape.SetStartEndingLength( 400 );
    shape.SetStartEndingWidth( 400 );

    PCB_SHAPE probe( nullptr, SHAPE_T::SEGMENT );
    probe.SetLayer( F_Cu );
    probe.SetStart( VECTOR2I( 0, -150 ) );
    probe.SetEnd( VECTOR2I( 0, 150 ) );
    probe.SetWidth( 20 );

    DRC_RTREE rtree;
    rtree.Insert( &shape, F_Cu, CLEARANCE_CONSTRAINT );
    rtree.Build();

    BOOST_CHECK_EQUAL( rtree.QueryColliding( &probe, F_Cu, F_Cu ), 1 );
}


BOOST_AUTO_TEST_CASE( PCBShapeHitTestHitsLineEnding )
{
    PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
    shape.SetLayer( F_Cu );
    shape.SetStart( VECTOR2I( 0, 0 ) );
    shape.SetEnd( VECTOR2I( 1000, 0 ) );
    shape.SetWidth( 100 );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::SQUARE );
    shape.SetStartEndingLength( 400 );
    shape.SetStartEndingWidth( 400 );

    BOOST_CHECK( shape.HitTest( VECTOR2I( 0, 150 ), 0 ) );

    BOX2I selection( VECTOR2I( -50, 100 ), VECTOR2I( 100, 80 ) );
    BOOST_CHECK( shape.HitTest( selection, false, 0 ) );
}


BOOST_AUTO_TEST_CASE( PCBShapeShortLineHitTestHitsConsumedBodyEnding )
{
    PCB_SHAPE shape( nullptr, SHAPE_T::SEGMENT );
    shape.SetLayer( F_Cu );
    shape.SetStart( VECTOR2I( 0, 0 ) );
    shape.SetEnd( VECTOR2I( 100, 0 ) );
    shape.SetWidth( 20 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 400 );
    shape.SetEndEndingWidth( 400 );

    BOOST_CHECK( shape.HitTest( VECTOR2I( -200, 100 ), 0 ) );
}
