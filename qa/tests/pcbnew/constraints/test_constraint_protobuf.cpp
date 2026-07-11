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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <google/protobuf/any.pb.h>

#include <constraints/pcb_constraint.h>


BOOST_AUTO_TEST_SUITE( ConstraintSolverProtobuf )


// A constraint round-trips through its protobuf Any representation: type, members (uuid + anchor),
// value and driving flag are all preserved.
BOOST_AUTO_TEST_CASE( RoundTrip )
{
    PCB_CONSTRAINT original( nullptr, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );

    KIID a, b;
    original.AddMember( a, CONSTRAINT_ANCHOR::START );
    original.AddMember( b, CONSTRAINT_ANCHOR::END );
    original.SetValue( 12.5 );
    original.SetDriving( false );

    google::protobuf::Any container;
    original.Serialize( container );

    PCB_CONSTRAINT restored( nullptr );
    BOOST_REQUIRE( restored.Deserialize( container ) );

    BOOST_CHECK( restored.m_Uuid == original.m_Uuid );
    BOOST_CHECK( restored.GetConstraintType() == PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    BOOST_REQUIRE_EQUAL( restored.GetMembers().size(), 2 );
    BOOST_CHECK( restored.GetMembers()[0].m_item == a );
    BOOST_CHECK( restored.GetMembers()[0].m_anchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK( restored.GetMembers()[1].m_item == b );
    BOOST_CHECK( restored.GetMembers()[1].m_anchor == CONSTRAINT_ANCHOR::END );
    BOOST_CHECK( !restored.IsDriving() );
    BOOST_REQUIRE( restored.HasValue() );
    BOOST_CHECK_CLOSE( *restored.GetValue(), 12.5, 1e-9 );

    BOOST_CHECK( restored == original );
}


// A valueless, driving constraint round-trips with no value set.
BOOST_AUTO_TEST_CASE( RoundTripNoValue )
{
    PCB_CONSTRAINT original( nullptr, PCB_CONSTRAINT_TYPE::PARALLEL );
    original.AddMember( KIID(), CONSTRAINT_ANCHOR::WHOLE );
    original.AddMember( KIID(), CONSTRAINT_ANCHOR::WHOLE );

    google::protobuf::Any container;
    original.Serialize( container );

    PCB_CONSTRAINT restored( nullptr );
    BOOST_REQUIRE( restored.Deserialize( container ) );

    BOOST_CHECK( restored.GetConstraintType() == PCB_CONSTRAINT_TYPE::PARALLEL );
    BOOST_CHECK( !restored.HasValue() );
    BOOST_CHECK( restored.IsDriving() );
    BOOST_CHECK( restored == original );
}


BOOST_AUTO_TEST_SUITE_END()
