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

#include <api/board/board_types.pb.h>
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


// Every constraint type survives the proto round-trip, so a divergence between the C++ enum and
// the proto ConstraintType (which Serialize bridges by static_cast) fails here in CI instead of
// silently corrupting API traffic.
BOOST_AUTO_TEST_CASE( RoundTripEveryType )
{
    for( int t = static_cast<int>( PCB_CONSTRAINT_TYPE::COINCIDENT );
         t <= static_cast<int>( PCB_CONSTRAINT_TYPE::ARC_ANGLE ); ++t )
    {
        PCB_CONSTRAINT_TYPE type = static_cast<PCB_CONSTRAINT_TYPE>( t );

        PCB_CONSTRAINT original( nullptr, type );
        original.AddMember( KIID(), CONSTRAINT_ANCHOR::WHOLE );
        original.AddMember( KIID(), CONSTRAINT_ANCHOR::WHOLE );

        google::protobuf::Any container;
        original.Serialize( container );

        PCB_CONSTRAINT restored( nullptr );
        BOOST_REQUIRE_MESSAGE( restored.Deserialize( container ),
                               "type " << t << " failed to deserialize" );
        BOOST_CHECK_MESSAGE( restored.GetConstraintType() == type,
                             "type " << t << " came back as "
                                     << static_cast<int>( restored.GetConstraintType() ) );
    }
}


// VERTEX members keep their index across the round trip non vertex members come back with the
// negative one sentinel not proto3 scalar default of zero
BOOST_AUTO_TEST_CASE( RoundTripVertexIndex )
{
    PCB_CONSTRAINT original( nullptr, PCB_CONSTRAINT_TYPE::COINCIDENT );

    KIID a, b;
    original.AddMember( a, CONSTRAINT_ANCHOR::VERTEX, 2 );
    original.AddMember( b, CONSTRAINT_ANCHOR::START );

    google::protobuf::Any container;
    original.Serialize( container );

    // Index is presence tracked on the wire so only the vertex member carries it
    kiapi::board::types::Constraint wire;
    BOOST_REQUIRE( container.UnpackTo( &wire ) );
    BOOST_REQUIRE_EQUAL( wire.members_size(), 2 );
    BOOST_CHECK( wire.members( 0 ).has_index() );
    BOOST_CHECK( !wire.members( 1 ).has_index() );

    PCB_CONSTRAINT restored( nullptr );
    BOOST_REQUIRE( restored.Deserialize( container ) );

    BOOST_REQUIRE_EQUAL( restored.GetMembers().size(), 2 );
    BOOST_CHECK( restored.GetMembers()[0].m_anchor == CONSTRAINT_ANCHOR::VERTEX );
    BOOST_CHECK_EQUAL( restored.GetMembers()[0].m_index, 2 );
    BOOST_CHECK( restored.GetMembers()[1].m_anchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK_EQUAL( restored.GetMembers()[1].m_index, -1 );

    BOOST_CHECK( restored == original );
}


// A hand built message with an index on a non vertex anchor must not leak it into the
// deserialized member only VERTEX anchors may carry one
BOOST_AUTO_TEST_CASE( DeserializeIgnoresIndexOnNonVertexAnchor )
{
    using namespace kiapi::board::types;

    Constraint constraint;
    constraint.mutable_id()->set_value( KIID().AsStdString() );
    constraint.set_type( ConstraintType::CT_COINCIDENT );
    constraint.set_driving( true );

    ConstraintMember* m = constraint.add_members();
    m->mutable_item()->set_value( KIID().AsStdString() );
    m->set_anchor( ConstraintAnchor::CA_START );
    m->set_index( 5 );

    google::protobuf::Any container;
    container.PackFrom( constraint );

    PCB_CONSTRAINT restored( nullptr );
    BOOST_REQUIRE( restored.Deserialize( container ) );

    BOOST_REQUIRE_EQUAL( restored.GetMembers().size(), 1 );
    BOOST_CHECK( restored.GetMembers()[0].m_anchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK_EQUAL( restored.GetMembers()[0].m_index, -1 );
}


// A VERTEX anchor with no index gets the negative one sentinel not a silent vertex zero
// presence tracking distinguishes this from an explicit index of zero
BOOST_AUTO_TEST_CASE( DeserializeVertexWithoutIndexGetsSentinel )
{
    using namespace kiapi::board::types;

    Constraint constraint;
    constraint.mutable_id()->set_value( KIID().AsStdString() );
    constraint.set_type( ConstraintType::CT_COINCIDENT );
    constraint.set_driving( true );

    ConstraintMember* m = constraint.add_members();
    m->mutable_item()->set_value( KIID().AsStdString() );
    m->set_anchor( ConstraintAnchor::CA_VERTEX );

    google::protobuf::Any container;
    container.PackFrom( constraint );

    PCB_CONSTRAINT restored( nullptr );
    BOOST_REQUIRE( restored.Deserialize( container ) );

    BOOST_REQUIRE_EQUAL( restored.GetMembers().size(), 1 );
    BOOST_CHECK( restored.GetMembers()[0].m_anchor == CONSTRAINT_ANCHOR::VERTEX );
    BOOST_CHECK_EQUAL( restored.GetMembers()[0].m_index, -1 );
}


BOOST_AUTO_TEST_SUITE_END()
