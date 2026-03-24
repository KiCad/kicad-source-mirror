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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <memory>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>


BOOST_AUTO_TEST_SUITE( BoardItemIndex )


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
    BOOST_CHECK( board->GetItemByIdCache().contains( newId ) );
    BOOST_CHECK( !board->GetItemByIdCache().contains( oldId ) );

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
    board->RebindItemUuid( liveShape, newId );

    BOOST_CHECK( liveShape->m_Uuid == newId );
    BOOST_CHECK( !board->GetItemByIdCache().contains( oldId ) );
    BOOST_CHECK( board->GetItemByIdCache().contains( newId ) );
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
    BOOST_CHECK( !board->GetItemByIdCache().contains( oldId ) );
    BOOST_CHECK( board->GetItemByIdCache().contains( newId ) );
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
    board->CacheItemById( liveShape );

    BOOST_CHECK( !board->GetItemByIdCache().contains( oldId ) );
    BOOST_CHECK( board->GetItemByIdCache().contains( newId ) );
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


BOOST_AUTO_TEST_SUITE_END()
