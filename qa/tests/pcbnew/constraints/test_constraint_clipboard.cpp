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
#include <kicad_clipboard.h>
#include <lset.h>
#include <pcb_shape.h>
#include <tools/pcb_selection.h>
#include <constraints/pcb_constraint.h>


BOOST_AUTO_TEST_SUITE( ConstraintSolverClipboard )


namespace
{
constexpr int MM = 1000000;


PCB_SHAPE* addSegment( BOARD& aBoard, const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    PCB_SHAPE* seg = new PCB_SHAPE( &aBoard, SHAPE_T::SEGMENT );
    seg->SetStart( aStart );
    seg->SetEnd( aEnd );
    seg->SetLayer( F_SilkS );
    seg->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    aBoard.Add( seg );
    return seg;
}


std::unique_ptr<BOARD> roundTrip( BOARD* aBoard, const PCB_SELECTION& aSelection )
{
    wxString data;
    CLIPBOARD_IO io;
    io.SetBoard( aBoard );
    io.SetWriter( [&]( const wxString& aData ) { data = aData; } );
    io.SetReader( [&]() { return data; } );

    io.SaveSelection( aSelection, false );

    BOARD_ITEM* parsed = io.Parse();
    BOOST_REQUIRE( parsed );

    return std::unique_ptr<BOARD>( static_cast<BOARD*>( parsed ) );
}
}


// A constraint copies along with its objects when every participant is in the selection, and the
// pasted constraint's members resolve to the pasted (remapped) copies.
BOOST_AUTO_TEST_CASE( ConstraintCopiedWhenAllMembersSelected )
{
    auto board = std::make_unique<BOARD>();
    board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    PCB_SHAPE* a = addSegment( *board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( *board, { 0, 5 * MM }, { 10 * MM, 6 * MM } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( board.get(), PCB_CONSTRAINT_TYPE::PARALLEL );
    c->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    c->AddMember( b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board->Add( c );

    PCB_SELECTION selection;
    selection.Add( a );
    selection.Add( b );

    std::unique_ptr<BOARD> pasted = roundTrip( board.get(), selection );

    BOOST_REQUIRE_EQUAL( pasted->Constraints().size(), 1 );

    PCB_CONSTRAINT* pastedConstraint = pasted->Constraints().front();
    BOOST_CHECK( pastedConstraint->GetConstraintType() == PCB_CONSTRAINT_TYPE::PARALLEL );
    BOOST_REQUIRE_EQUAL( pastedConstraint->GetMembers().size(), 2 );

    // Every member resolves to an item actually in the pasted board (remapped, not dangling).
    for( const CONSTRAINT_MEMBER& member : pastedConstraint->GetMembers() )
        BOOST_CHECK( pasted->ResolveItem( member.m_item, true ) != nullptr );
}


// Pasting re-UUIDs every item (placeBoardItems), which would orphan a constraint's KIID members.
// RemapKIIDs, fed the old -> new map built during the re-UUID, must re-point them at the copies.
BOOST_AUTO_TEST_CASE( ConstraintMembersRemapAfterReUuid )
{
    auto board = std::make_unique<BOARD>();

    PCB_SHAPE* a = addSegment( *board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( *board, { 0, 5 * MM }, { 10 * MM, 6 * MM } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( board.get(), PCB_CONSTRAINT_TYPE::PARALLEL );
    c->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    c->AddMember( b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board->Add( c );

    // Mimic the re-UUID a paste/duplicate performs, recording old -> new exactly as placeBoardItems.
    std::map<KIID, KIID> idMap;

    for( PCB_SHAPE* shape : { a, b } )
    {
        KIID oldUuid = shape->m_Uuid;
        shape->ResetUuid();
        idMap[oldUuid] = shape->m_Uuid;
    }

    // Without the remap the members still hold the pre-paste KIIDs and no longer resolve.
    BOOST_CHECK( board->ResolveItem( c->GetMembers()[0].m_item, true ) == nullptr );

    c->RemapKIIDs( idMap );

    BOOST_CHECK( c->GetMembers()[0].m_item == a->m_Uuid );
    BOOST_CHECK( c->GetMembers()[1].m_item == b->m_Uuid );
    BOOST_CHECK( board->ResolveItem( c->GetMembers()[0].m_item, true ) == a );
    BOOST_CHECK( board->ResolveItem( c->GetMembers()[1].m_item, true ) == b );

    // A KIID absent from the map is left untouched.
    KIID            stranger;
    PCB_CONSTRAINT* d = new PCB_CONSTRAINT( board.get(), PCB_CONSTRAINT_TYPE::PARALLEL );
    d->AddMember( stranger, CONSTRAINT_ANCHOR::WHOLE );
    d->RemapKIIDs( idMap );
    BOOST_CHECK( d->GetMembers()[0].m_item == stranger );
    delete d;
}


// A constraint is NOT copied when only some of its participants are selected.
BOOST_AUTO_TEST_CASE( ConstraintNotCopiedWhenPartialSelection )
{
    auto board = std::make_unique<BOARD>();
    board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    PCB_SHAPE* a = addSegment( *board, { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addSegment( *board, { 0, 5 * MM }, { 10 * MM, 6 * MM } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( board.get(), PCB_CONSTRAINT_TYPE::PARALLEL );
    c->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    c->AddMember( b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    board->Add( c );

    PCB_SELECTION selection;
    selection.Add( a );   // only one of the two participants

    std::unique_ptr<BOARD> pasted = roundTrip( board.get(), selection );

    BOOST_CHECK( pasted->Constraints().empty() );
}


// The footprint editor's loose-shape copy path carries footprint constraints whose members are all
// selected, matching the board branch.
BOOST_AUTO_TEST_CASE( FootprintEditorCopyCarriesConstraints )
{
    auto board = std::make_unique<BOARD>();
    board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    board->Add( fp );

    auto addFpSegment =
            [&]( const VECTOR2I& aStart, const VECTOR2I& aEnd ) -> PCB_SHAPE*
            {
                PCB_SHAPE* seg = new PCB_SHAPE( fp, SHAPE_T::SEGMENT );
                seg->SetStart( aStart );
                seg->SetEnd( aEnd );
                seg->SetLayer( F_SilkS );
                seg->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
                fp->Add( seg );
                return seg;
            };

    PCB_SHAPE* a = addFpSegment( { 0, 0 }, { 10 * MM, 0 } );
    PCB_SHAPE* b = addFpSegment( { 0, 5 * MM }, { 10 * MM, 6 * MM } );

    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( fp, PCB_CONSTRAINT_TYPE::PARALLEL );
    c->AddMember( a->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    c->AddMember( b->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    fp->Add( c );

    // Each save gets a fresh CLIPBOARD_IO; its formatter accumulates across SaveSelection calls.
    auto fpRoundTrip =
            [&]( const PCB_SELECTION& aSelection ) -> std::unique_ptr<FOOTPRINT>
            {
                wxString     data;
                CLIPBOARD_IO io;
                io.SetBoard( board.get() );
                io.SetWriter( [&]( const wxString& aData ) { data = aData; } );
                io.SetReader( [&]() { return data; } );

                io.SaveSelection( aSelection, true );

                BOARD_ITEM* parsed = io.Parse();
                BOOST_REQUIRE( parsed );
                BOOST_REQUIRE( parsed->Type() == PCB_FOOTPRINT_T );

                return std::unique_ptr<FOOTPRINT>( static_cast<FOOTPRINT*>( parsed ) );
            };

    PCB_SELECTION selection;
    selection.Add( a );
    selection.Add( b );

    std::unique_ptr<FOOTPRINT> clip = fpRoundTrip( selection );

    BOOST_REQUIRE_EQUAL( clip->Constraints().size(), 1 );
    BOOST_CHECK( clip->Constraints().front()->GetConstraintType() == PCB_CONSTRAINT_TYPE::PARALLEL );

    // A partial selection leaves the constraint behind, like the board branch.
    PCB_SELECTION partial;
    partial.Add( a );

    BOOST_CHECK( fpRoundTrip( partial )->Constraints().empty() );
}


BOOST_AUTO_TEST_SUITE_END()
