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

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <memory>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_track.h>


BOOST_AUTO_TEST_SUITE( BoardItemIndex )


BOOST_AUTO_TEST_CASE( ResolveItemIdentityCachePurgedOnDestruction )
{
    BOARD board;

    FOOTPRINT* footprint = new FOOTPRINT( &board );
    board.Add( footprint );

    PAD* pad = new PAD( footprint );
    footprint->Pads().push_back( pad );
    board.CacheItemById( *pad );

    const KIID padId = pad->m_Uuid;

    BOOST_REQUIRE( board.IsItemIndexedById( pad ) );
    BOOST_REQUIRE_EQUAL( board.ResolveItem( padId, true ), static_cast<BOARD_ITEM*>( pad ) );

    // Detach the pad from the footprint without FOOTPRINT::Remove()'s surgical eviction, leaving it
    // parented to the still-board-attached footprint, then free it directly.  This stands in for the
    // producer paths that free a board-parented item without touching the identity cache; only the
    // ~BOARD_ITEM safety net can then keep ResolveItem() from returning a freed pointer.
    std::deque<PAD*>& pads = footprint->Pads();
    pads.erase( std::find( pads.begin(), pads.end(), pad ) );
    delete pad;

    BOOST_CHECK( board.ResolveItem( padId, true ) == nullptr );
}


// A never-indexed item parented to a freed board must not touch that board on destruction, the
// crash from the "KiCad master crashes" report (follow-up to ac12a1c820).
BOOST_AUTO_TEST_CASE( UncachedItemSurvivesBoardDestruction )
{
    BOARD*   board = new BOARD();
    PCB_VIA* dummy = new PCB_VIA( board );

    BOOST_REQUIRE( !dummy->IsIndexedInBoard() );

    delete board;

    BOOST_CHECK_NO_THROW( delete dummy );
}


// The indexed counterpart of the case above.  ~BOARD must clear the membership flag as it drops
// the index, or a survivor still believes it is indexed and walks its parent chain into the
// freed board.
BOOST_AUTO_TEST_CASE( IndexedItemSurvivesBoardDestruction )
{
    BOARD*     board = new BOARD();
    FOOTPRINT* footprint = new FOOTPRINT( board );

    board->Add( footprint );

    PAD* pad = new PAD( footprint );
    footprint->Pads().push_back( pad );
    board->CacheItemById( *pad );

    BOOST_REQUIRE( pad->IsIndexedInBoard() );

    // Detach without FOOTPRINT::Remove() so the board frees the footprint while the pad lives on,
    // still parented to it.  This is the ownership hand-off the safety net in ~BOARD_ITEM covers.
    std::deque<PAD*>& pads = footprint->Pads();
    pads.erase( std::find( pads.begin(), pads.end(), pad ) );

    delete board;

    BOOST_CHECK( !pad->IsIndexedInBoard() );
    BOOST_CHECK_NO_THROW( delete pad );
}


BOOST_AUTO_TEST_CASE( SwapItemDataReindexesFootprintChildren )
{
    BOARD* board = new BOARD();

    auto footprint = std::make_unique<FOOTPRINT>( board );
    auto pad = new PAD( footprint.get() );
    pad->SetNumber( "1" );

    PAD* originalPad = pad;
    KIID originalPadId = originalPad->m_Uuid;

    footprint->Add( pad );

    FOOTPRINT* liveFootprint = footprint.get();
    board->Add( footprint.release() );

    auto image = std::unique_ptr<FOOTPRINT>( static_cast<FOOTPRINT*>( liveFootprint->Clone() ) );

    liveFootprint->SwapItemData( image.get() );
    image.reset();

    BOARD_ITEM* resolved = board->ResolveItem( originalPadId, true );

    BOOST_REQUIRE( resolved );
    BOOST_CHECK_EQUAL( resolved->Type(), PCB_PAD_T );
    BOOST_CHECK( resolved != originalPad );

    delete board;
}


BOOST_AUTO_TEST_CASE( AttachedItemUuidRewriteDropsOldAlias )
{
    BOARD* board = new BOARD();

    auto shape = std::make_unique<PCB_SHAPE>( board );
    PCB_SHAPE* liveShape = shape.get();
    const KIID oldId = liveShape->m_Uuid;
    const KIID newId;

    board->Add( shape.release() );

    const_cast<KIID&>( liveShape->m_Uuid ) = newId;

    BOOST_CHECK( board->ResolveItem( oldId, true ) == nullptr );
    BOOST_CHECK_EQUAL( board->ResolveItem( newId, true ), liveShape );

    delete board;
}


BOOST_AUTO_TEST_CASE( ResolvingNewUuidPurgesStaleOldAlias )
{
    BOARD* board = new BOARD();

    auto shape = std::make_unique<PCB_SHAPE>( board );
    PCB_SHAPE* liveShape = shape.get();
    const KIID oldId = liveShape->m_Uuid;
    const KIID newId;

    board->Add( shape.release() );
    const_cast<KIID&>( liveShape->m_Uuid ) = newId;

    BOOST_CHECK_EQUAL( board->ResolveItem( newId, true ), liveShape );
    BOOST_CHECK( board->IsItemIndexedById( newId ) );
    BOOST_CHECK( !board->IsItemIndexedById( oldId ) );

    delete board;
}


BOOST_AUTO_TEST_CASE( RebindItemUuidUpdatesCacheAtomically )
{
    BOARD* board = new BOARD();

    auto shape = std::make_unique<PCB_SHAPE>( board );
    PCB_SHAPE* liveShape = shape.get();
    const KIID oldId = liveShape->m_Uuid;
    const KIID newId;

    board->Add( shape.release() );
    board->RebindItemUuid( *liveShape, newId );

    BOOST_CHECK( liveShape->m_Uuid == newId );
    BOOST_CHECK( !board->IsItemIndexedById( oldId ) );
    BOOST_CHECK( board->IsItemIndexedById( newId ) );
    BOOST_CHECK_EQUAL( board->ResolveItem( newId, true ), liveShape );

    delete board;
}


BOOST_AUTO_TEST_CASE( AttachedChildSetUuidRebindsCache )
{
    BOARD* board = new BOARD();

    auto footprint = std::make_unique<FOOTPRINT>( board );
    auto pad = new PAD( footprint.get() );
    PAD* livePad = pad;
    const KIID oldId = livePad->m_Uuid;
    const KIID newId;

    footprint->Add( pad );
    board->Add( footprint.release() );

    livePad->SetUuid( newId );

    BOOST_CHECK( livePad->m_Uuid == newId );
    BOOST_CHECK( !board->IsItemIndexedById( oldId ) );
    BOOST_CHECK( board->IsItemIndexedById( newId ) );
    BOOST_CHECK_EQUAL( board->ResolveItem( newId, true ), livePad );

    delete board;
}


BOOST_AUTO_TEST_CASE( CacheItemByIdCanonicalizesRewrittenUuid )
{
    BOARD* board = new BOARD();

    auto shape = std::make_unique<PCB_SHAPE>( board );
    PCB_SHAPE* liveShape = shape.get();
    const KIID oldId = liveShape->m_Uuid;
    const KIID newId;

    board->Add( shape.release() );

    const_cast<KIID&>( liveShape->m_Uuid ) = newId;
    board->CacheItemById( *liveShape );

    BOOST_CHECK( !board->IsItemIndexedById( oldId ) );
    BOOST_CHECK( board->IsItemIndexedById( newId ) );
    BOOST_CHECK_EQUAL( board->ResolveItem( newId, true ), liveShape );

    delete board;
}


BOOST_AUTO_TEST_CASE( RepairDuplicateItemUuidsKeepsEarlierTraversalWinner )
{
    BOARD* board = new BOARD();

    auto footprint = std::make_unique<FOOTPRINT>( board );
    FOOTPRINT* liveFootprint = footprint.get();
    const KIID claimedId = liveFootprint->m_Uuid;

    auto shape = std::make_unique<PCB_SHAPE>( board );
    PCB_SHAPE* liveShape = shape.get();
    const KIID originalShapeId = liveShape->m_Uuid;

    board->Add( footprint.release() );
    board->Add( shape.release() );

    const_cast<KIID&>( liveShape->m_Uuid ) = claimedId;

    BOOST_CHECK_EQUAL( board->RepairDuplicateItemUuids(), 1 );
    BOOST_CHECK( liveFootprint->m_Uuid == claimedId );
    BOOST_CHECK( liveShape->m_Uuid != claimedId );
    BOOST_CHECK( liveShape->m_Uuid != originalShapeId );
    BOOST_CHECK_EQUAL( board->ResolveItem( claimedId, true ), liveFootprint );
    BOOST_CHECK( board->ResolveItem( originalShapeId, true ) == nullptr );
    BOOST_CHECK_EQUAL( board->ResolveItem( liveShape->m_Uuid, true ), liveShape );

    delete board;
}


BOOST_AUTO_TEST_CASE( RemovingResolvedChildEvictsCacheOnFootprintHolder )
{
    // Regression for the RC_TREE_MODEL::GetValue use-after-free: ResolveItem() caches a
    // footprint child on demand, but the cache eviction in FOOTPRINT::Remove is gated on the
    // parent footprint being indexed.  A footprint-holder board never indexes anything (
    // CacheItemById early-returns), yet ResolveItem still caches children, so removing and
    // freeing a resolved child leaves a dangling id in the cache.  A later resolve then hands
    // back the freed pointer.
    BOARD* board = new BOARD();
    board->SetBoardUse( BOARD_USE::FPHOLDER );

    auto footprint = std::make_unique<FOOTPRINT>( board );
    auto pad = new PAD( footprint.get() );
    pad->SetNumber( "1" );

    const KIID padId = pad->m_Uuid;
    footprint->Add( pad );

    FOOTPRINT* liveFootprint = footprint.get();
    board->Add( footprint.release() );

    // Resolve the pad the way a results-tree row does.  On the buggy code this caches the pad
    // even though the holder board never indexed its parent footprint.
    BOOST_REQUIRE_EQUAL( board->ResolveItem( padId, true ), pad );

    // Remove and destroy the pad through the footprint, as a footprint edit does.
    liveFootprint->Remove( pad );
    const bool staleEntrySurvived = board->IsItemIndexedById( padId );

    delete pad;
    delete board;

    BOOST_CHECK( !staleEntrySurvived );
}


BOOST_AUTO_TEST_CASE( TableCellResolvesToTheCellNotItsTable )
{
    // A cell carries its own KIID and is an editable item in its own right, so handing back the
    // parent table forced every caller that can select a cell to special-case it.  The cache is
    // cleared to exercise the linear scan, which is the path that got this wrong.
    BOARD board;
    auto* table = new PCB_TABLE( &board, 0 );
    auto* cell = new PCB_TABLECELL( table );

    table->AddCell( cell );
    board.Add( table );
    board.ClearItemByIdCache();

    const KIID tableId = table->m_Uuid;
    const KIID cellId = cell->m_Uuid;

    BOOST_CHECK_EQUAL( board.ResolveItem( cellId, true ), cell );
    BOOST_CHECK_EQUAL( board.ResolveItem( tableId, true ), table );

    // Caching the table under the cell's id also made the two lookups evict each other.
    BOOST_CHECK_EQUAL( board.ResolveItem( cellId, true ), cell );
    BOOST_CHECK( board.IsItemIndexedById( tableId ) );
    BOOST_CHECK( board.IsItemIndexedById( cellId ) );
}


BOOST_AUTO_TEST_CASE( FootprintTableCellResolvesByItsOwnId )
{
    // The footprint branch walked GraphicalItems() without ever descending into a table, so a
    // cell inside a footprint table could not be resolved at all.
    BOARD board;
    auto* footprint = new FOOTPRINT( &board );
    auto* table = new PCB_TABLE( footprint, 0 );
    auto* cell = new PCB_TABLECELL( table );

    table->AddCell( cell );
    footprint->Add( table, ADD_MODE::APPEND );
    board.Add( footprint );
    board.ClearItemByIdCache();

    BOOST_CHECK_EQUAL( board.ResolveItem( cell->m_Uuid, true ), cell );
}


// Unbinding an item the index never held must not scan the whole index, or comparing a few footprints
// against a large board costs as much as the board each time
BOOST_AUTO_TEST_CASE( UnbindingForeignSubtreesDoesNotScanTheIndex )
{
    BOARD board;

    for( int i = 0; i < 20000; i++ )
    {
        FOOTPRINT* footprint = new FOOTPRINT( &board );

        for( int j = 0; j < 4; j++ )
            footprint->Add( new PAD( footprint ), ADD_MODE::APPEND );

        board.Add( footprint );
    }

    FOOTPRINT foreign( &board );

    for( int j = 0; j < 20; j++ )
        foreign.Add( new PAD( &foreign ), ADD_MODE::APPEND );

    auto start = std::chrono::steady_clock::now();

    for( int i = 0; i < 300; i++ )
        board.UncacheItemSubtreeById( foreign );

    std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

    BOOST_CHECK_LT( elapsed.count(), 5.0 );
}


// Unbinding through one board must leave another board's claim on the item intact, or the item
// is never evicted from the owner when it is destroyed
BOOST_AUTO_TEST_CASE( UnbindingThroughAnotherBoardKeepsTheOwner )
{
    BOARD owner;
    BOARD other;

    auto* pad = new PAD( nullptr );
    KIID  id = pad->m_Uuid;

    owner.CacheItemById( *pad );
    other.UncacheItemSubtreeById( *pad );

    BOOST_CHECK( owner.IsItemIndexedById( id ) );

    delete pad;

    BOOST_CHECK( !owner.IsItemIndexedById( id ) );
}


BOOST_AUTO_TEST_SUITE_END()
