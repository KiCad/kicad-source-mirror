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

#include <filesystem>
#include <memory>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <richio.h>

#include <board.h>
#include <pcb_shape.h>
#include <constraints/pcb_constraint.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.h>
#include <pcbnew_utils/board_file_utils.h>

#include "constraint_test_utils.h"

using namespace KI_TEST;


namespace
{
PCB_CONSTRAINT* findConstraint( BOARD& aBoard, PCB_CONSTRAINT_TYPE aType )
{
    for( PCB_CONSTRAINT* c : aBoard.Constraints() )
    {
        if( c->GetConstraintType() == aType )
            return c;
    }

    return nullptr;
}
}


BOOST_AUTO_TEST_SUITE( ConstraintSolverSerialization )


// A board with two segments and a parallel + fixed-length constraint round-trips: the
// constraints, their members (uuid + anchor), value and driving flag survive write/reload.
BOOST_AUTO_TEST_CASE( RoundTrip )
{
    auto board1 = std::make_unique<BOARD>();

    PCB_SHAPE* a = addSegment( *board1, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( *board1, { 0, 5 * MM }, { 8 * MM, 6 * MM } );

    PCB_CONSTRAINT* parallel = new PCB_CONSTRAINT( board1.get(), PCB_CONSTRAINT_TYPE::PARALLEL );
    parallel->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    parallel->AddMember( b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board1->Add( parallel );

    // A fixed-length constraint carries a length value (in IU, written in mm) and the reference
    // (non-driving) flag; its two START/END members exercise non-WHOLE anchor round-trip.
    PCB_CONSTRAINT* dim = new PCB_CONSTRAINT( board1.get(), PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    dim->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::START );
    dim->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::END );
    dim->SetValue( 10.0 * MM );
    dim->SetDriving( false );
    board1->Add( dim );

    KIID aId = a->m_Uuid;
    KIID bId = b->m_Uuid;

    auto path = std::filesystem::temp_directory_path() / "constraint_roundtrip_tst.kicad_pcb";
    ::KI_TEST::DumpBoardToFile( *board1, path.string() );

    std::unique_ptr<BOARD> board2 = ::KI_TEST::ReadBoardFromFileOrStream( path.string() );

    BOOST_REQUIRE_EQUAL( board2->Constraints().size(), 2 );

    PCB_CONSTRAINT* par2 = findConstraint( *board2, PCB_CONSTRAINT_TYPE::PARALLEL );
    BOOST_REQUIRE( par2 );
    BOOST_REQUIRE_EQUAL( par2->GetMembers().size(), 2 );
    BOOST_CHECK( par2->GetMembers()[0].m_item == aId );
    BOOST_CHECK( par2->GetMembers()[0].m_anchor == CONSTRAINT_ANCHOR::WHOLE );
    BOOST_CHECK( par2->GetMembers()[1].m_item == bId );
    BOOST_CHECK( par2->IsDriving() );
    BOOST_CHECK( !par2->HasValue() );

    PCB_CONSTRAINT* dim2 = findConstraint( *board2, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
    BOOST_REQUIRE( dim2 );
    BOOST_REQUIRE_EQUAL( dim2->GetMembers().size(), 2 );
    BOOST_CHECK( dim2->GetMembers()[0].m_anchor == CONSTRAINT_ANCHOR::START );
    BOOST_CHECK( dim2->GetMembers()[1].m_anchor == CONSTRAINT_ANCHOR::END );
    BOOST_CHECK( !dim2->IsDriving() );
    BOOST_REQUIRE( dim2->HasValue() );
    BOOST_CHECK_CLOSE( *dim2->GetValue(), 10.0 * MM, 1e-6 );   // length survives the mm round-trip
}


// A constraint member referencing a missing shape is preserved through a round-trip as a dangling
// reference (error state), not silently dropped (Zulip "Geometry Constraint Solver", 2026-06-18:
// a constraint persists in an error state rather than losing the reference).
BOOST_AUTO_TEST_CASE( MissingShapeMemberPreservedAsError )
{
    auto board1 = std::make_unique<BOARD>();

    PCB_SHAPE* a = addSegment( *board1, { 0, 0 }, { 10 * MM, 0 } );
    KIID       danglingId;   // a reference to an item that is not on the board

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( board1.get(), PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::END );
    c->AddMember( danglingId, CONSTRAINT_ANCHOR::START );
    board1->Add( c );

    auto path = std::filesystem::temp_directory_path() / "constraint_missing_tst.kicad_pcb";
    ::KI_TEST::DumpBoardToFile( *board1, path.string() );

    std::unique_ptr<BOARD> board2 = ::KI_TEST::ReadBoardFromFileOrStream( path.string() );

    BOOST_REQUIRE_EQUAL( board2->Constraints().size(), 1 );

    // Both members survive: the resolvable one and the dangling one (error state).
    PCB_CONSTRAINT* c2 = board2->Constraints().front();
    BOOST_REQUIRE_EQUAL( c2->GetMembers().size(), 2 );

    bool hasValid = false, hasDangling = false;

    for( const CONSTRAINT_MEMBER& m : c2->GetMembers() )
    {
        if( m.m_item == a->m_Uuid )
            hasValid = true;
        else if( m.m_item == danglingId )
            hasDangling = true;
    }

    BOOST_CHECK( hasValid );
    BOOST_CHECK( hasDangling );
}


// Appending a board file into an existing board remaps every uuid; a constraint's members must
// follow the remap and resolve to the appended copies, not dangle on the originals' old uuids.
BOOST_AUTO_TEST_CASE( AppendRemapsConstraintMembers )
{
    auto board1 = std::make_unique<BOARD>();

    PCB_SHAPE* a = addSegment( *board1, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( *board1, { 10 * MM, 0 }, { 10 * MM, 10 * MM } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( board1.get(), PCB_CONSTRAINT_TYPE::COINCIDENT );
    c->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::END );
    c->AddMember( b->m_Uuid, CONSTRAINT_ANCHOR::START );
    board1->Add( c );

    KIID aOrigId = a->m_Uuid;

    auto path = std::filesystem::temp_directory_path() / "constraint_append_tst.kicad_pcb";
    ::KI_TEST::DumpBoardToFile( *board1, path.string() );

    // Append the file into a fresh board, which forces uuid remapping.
    auto board2 = std::make_unique<BOARD>();

    FILE_LINE_READER reader( path.string() );
    PCB_IO_KICAD_SEXPR_PARSER parser( &reader, board2.get(),
                                      []( wxString, int, wxString, wxString ) { return true; } );
    parser.Parse();

    BOOST_REQUIRE_EQUAL( board2->Constraints().size(), 1 );

    PCB_CONSTRAINT* c2 = board2->Constraints().front();
    BOOST_REQUIRE_EQUAL( c2->GetMembers().size(), 2 );

    // Every member resolves to an item that actually lives in the appended board...
    for( const CONSTRAINT_MEMBER& member : c2->GetMembers() )
        BOOST_CHECK( board2->ResolveItem( member.m_item, true ) != nullptr );

    // ...and the references were remapped off the originals' uuids.
    BOOST_CHECK( c2->GetMembers()[0].m_item != aOrigId );
}


BOOST_AUTO_TEST_SUITE_END()
