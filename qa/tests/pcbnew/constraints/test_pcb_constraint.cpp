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

#include <memory>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/shape.h>

#include <constraints/pcb_constraint.h>
#include <core/typeinfo.h>


BOOST_AUTO_TEST_SUITE( ConstraintSolverItem )


BOOST_AUTO_TEST_CASE( BasicProperties )
{
    PCB_CONSTRAINT c( nullptr, PCB_CONSTRAINT_TYPE::PARALLEL );

    BOOST_CHECK_EQUAL( c.Type(), PCB_CONSTRAINT_T );
    BOOST_CHECK( c.GetConstraintType() == PCB_CONSTRAINT_TYPE::PARALLEL );

    // A constraint carries no geometry and is on no layer.
    BOOST_CHECK( c.GetLayerSet() == LSET() );
    BOOST_CHECK( !c.HitTest( VECTOR2I( 0, 0 ), 1000000 ) );
    BOOST_CHECK( c.ViewGetLayers().empty() );

    // Driving by default.
    BOOST_CHECK( c.IsDriving() );
    BOOST_CHECK( !c.HasValue() );
}


BOOST_AUTO_TEST_CASE( GeometrySurfaceIsSafe )
{
    PCB_CONSTRAINT c( nullptr, PCB_CONSTRAINT_TYPE::PARALLEL );

    // The base BOARD_ITEM transforms wxFAIL_MSG and GetEffectiveShape is unimplemented;
    // a constraint overrides them so any generic item walk stays safe.
    BOOST_CHECK_NO_THROW( c.Move( VECTOR2I( 1000, 1000 ) ) );
    BOOST_CHECK_NO_THROW( c.Rotate( VECTOR2I( 0, 0 ), ANGLE_90 ) );

    std::shared_ptr<SHAPE> shape = c.GetEffectiveShape();
    BOOST_REQUIRE( shape );
    BOOST_CHECK_EQUAL( shape->Type(), SH_COMPOUND );
}


BOOST_AUTO_TEST_CASE( MembersAndValue )
{
    PCB_CONSTRAINT c( nullptr, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );

    KIID a, b;
    c.AddMember( a, CONSTRAINT_ANCHOR::START );
    c.AddMember( b, CONSTRAINT_ANCHOR::END );
    c.SetValue( 5.0 );
    c.SetDriving( false );

    BOOST_CHECK_EQUAL( c.GetMembers().size(), 2 );
    BOOST_CHECK( c.GetMembers()[0].m_item == a );
    BOOST_CHECK( c.GetMembers()[0].m_anchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK( c.GetMembers()[1].m_anchor == CONSTRAINT_ANCHOR::END );
    BOOST_CHECK( c.HasValue() );
    BOOST_CHECK_CLOSE( *c.GetValue(), 5.0, 1e-9 );
    BOOST_CHECK( !c.IsDriving() );
}


BOOST_AUTO_TEST_CASE( CloneIsAFaithfulCopy )
{
    PCB_CONSTRAINT c( nullptr, PCB_CONSTRAINT_TYPE::EQUAL_RADIUS );
    c.AddMember( KIID(), CONSTRAINT_ANCHOR::RADIUS );
    c.AddMember( KIID(), CONSTRAINT_ANCHOR::RADIUS );
    c.SetValue( 2.5 );

    std::unique_ptr<BOARD_ITEM> clone( static_cast<BOARD_ITEM*>( c.Clone() ) );

    // Clone preserves the uuid and compares equal to its source.
    BOOST_CHECK( clone->m_Uuid == c.m_Uuid );
    BOOST_CHECK( *static_cast<PCB_CONSTRAINT*>( clone.get() ) == c );
    BOOST_CHECK( c == *clone );
}


BOOST_AUTO_TEST_CASE( EqualityAndSimilarity )
{
    KIID a, b;

    PCB_CONSTRAINT c1( nullptr, PCB_CONSTRAINT_TYPE::PARALLEL );
    c1.AddMember( a, CONSTRAINT_ANCHOR::WHOLE );
    c1.AddMember( b, CONSTRAINT_ANCHOR::WHOLE );

    PCB_CONSTRAINT c2( nullptr, PCB_CONSTRAINT_TYPE::PARALLEL );
    c2.AddMember( a, CONSTRAINT_ANCHOR::WHOLE );
    c2.AddMember( b, CONSTRAINT_ANCHOR::WHOLE );

    BOOST_CHECK( c1 == c2 );
    BOOST_CHECK_CLOSE( c1.Similarity( c2 ), 1.0, 1e-9 );

    // A different type is wholly dissimilar.
    PCB_CONSTRAINT cPerp( nullptr, PCB_CONSTRAINT_TYPE::PERPENDICULAR );
    cPerp.AddMember( a, CONSTRAINT_ANCHOR::WHOLE );
    cPerp.AddMember( b, CONSTRAINT_ANCHOR::WHOLE );
    BOOST_CHECK( !( c1 == cPerp ) );
    BOOST_CHECK_EQUAL( c1.Similarity( cPerp ), 0.0 );

    // Same type, different members compares unequal but partially similar.
    PCB_CONSTRAINT c3( nullptr, PCB_CONSTRAINT_TYPE::PARALLEL );
    c3.AddMember( KIID(), CONSTRAINT_ANCHOR::WHOLE );
    BOOST_CHECK( !( c1 == c3 ) );
    BOOST_CHECK_LT( c1.Similarity( c3 ), 1.0 );
    BOOST_CHECK_GT( c1.Similarity( c3 ), 0.0 );
}


BOOST_AUTO_TEST_SUITE_END()
