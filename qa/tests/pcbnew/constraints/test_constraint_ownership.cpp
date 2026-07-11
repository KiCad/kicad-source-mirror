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

#include <board.h>
#include <footprint.h>
#include <constraints/pcb_constraint.h>


BOOST_AUTO_TEST_SUITE( ConstraintSolverOwnership )


BOOST_AUTO_TEST_CASE( BoardAddRemoveResolve )
{
    BOARD board;

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::PARALLEL );
    c->AddMember( KIID(), CONSTRAINT_ANCHOR::WHOLE );
    KIID id = c->m_Uuid;

    board.Add( c );

    BOOST_REQUIRE_EQUAL( board.Constraints().size(), 1 );
    BOOST_CHECK( board.Constraints().front() == c );

    // The KIID cache and the linear-scan fallback both resolve the constraint.
    BOOST_CHECK( board.ResolveItem( id, true ) == c );

    board.Remove( c );
    BOOST_CHECK( board.Constraints().empty() );

    delete c;
}


BOOST_AUTO_TEST_CASE( BoardRunOnChildrenVisitsConstraint )
{
    BOARD board;

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::HORIZONTAL );
    board.Add( c );

    bool seen = false;
    board.RunOnChildren(
            [&]( BOARD_ITEM* aItem )
            {
                if( aItem == c )
                    seen = true;
            },
            RECURSE_MODE::NO_RECURSE );

    BOOST_CHECK( seen );
}


BOOST_AUTO_TEST_CASE( FootprintCloneRoundTrip )
{
    BOARD board;

    auto fp = std::make_unique<FOOTPRINT>( &board );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( fp.get(), PCB_CONSTRAINT_TYPE::EQUAL_LENGTH );
    c->AddMember( KIID(), CONSTRAINT_ANCHOR::START );
    c->AddMember( KIID(), CONSTRAINT_ANCHOR::END );
    c->SetValue( 3.0 );
    fp->Add( c );

    BOOST_REQUIRE_EQUAL( fp->Constraints().size(), 1 );

    // Copy constructor must clone the constraint faithfully (uuid preserved).
    std::unique_ptr<FOOTPRINT> clone( static_cast<FOOTPRINT*>( fp->Clone() ) );

    BOOST_REQUIRE_EQUAL( clone->Constraints().size(), 1 );

    PCB_CONSTRAINT* original = fp->Constraints().front();
    PCB_CONSTRAINT* copied = clone->Constraints().front();

    BOOST_CHECK( copied != original );
    BOOST_CHECK( *copied == *original );
}


BOOST_AUTO_TEST_CASE( FootprintTeardownDeletesConstraints )
{
    BOARD board;

    {
        FOOTPRINT fp( &board );
        fp.Add( new PCB_CONSTRAINT( &fp, PCB_CONSTRAINT_TYPE::VERTICAL ) );
        BOOST_CHECK_EQUAL( fp.Constraints().size(), 1 );

        // The constraint is owned by the footprint; its dtor frees it (asan would
        // flag a leak or double-free otherwise).
    }
}


BOOST_AUTO_TEST_SUITE_END()
