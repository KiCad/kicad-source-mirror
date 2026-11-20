/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_commit.h>
#include <footprint.h>
#include <pcb_group.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <undo_group_state.h>
#include <undo_redo_container.h>

// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/26111
//
// Update Footprints from Library, undo, redo dropped the grouped footprints from their group


// The board side of PCB_BASE_EDIT_FRAME::PutDataInPreviousState(), which needs a frame
static void applyUndoList( BOARD& aBoard, PICKED_ITEMS_LIST& aList )
{
    UNDO_GROUP_STATE groupState( aBoard, aList );

    for( int ii = (int) aList.GetCount() - 1; ii >= 0; ii-- )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( aList.GetPickedItem( ii ) );

        switch( aList.GetPickedItemStatus( ii ) )
        {
        case UNDO_REDO::CHANGED:
            aBoard.Remove( item, REMOVE_MODE::BULK );
            item->SwapItemData( static_cast<BOARD_ITEM*>( aList.GetPickedItemLink( ii ) ) );
            aBoard.Add( item, ADD_MODE::BULK_INSERT );
            break;

        case UNDO_REDO::NEWITEM:
            aBoard.Remove( item, REMOVE_MODE::BULK );
            aList.SetPickedItemStatus( UNDO_REDO::DELETED, ii );
            break;

        case UNDO_REDO::DELETED:
            aBoard.Add( item, ADD_MODE::BULK_APPEND );
            aList.SetPickedItemStatus( UNDO_REDO::NEWITEM, ii );
            break;

        default:
            break;
        }
    }

    groupState.Restore( aList );
    aList.ReversePickersListOrder();
}


BOOST_AUTO_TEST_CASE( Issue26111_UpdatedFootprintStaysGroupedThroughUndoRedo )
{
    SETTINGS_MANAGER       settingsManager;
    std::unique_ptr<BOARD> board;

    KI_TEST::LoadBoard( settingsManager, "issue26111", board );

    TOOL_MANAGER         toolMgr;
    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();

    toolMgr.SetEnvironment( board.get(), nullptr, nullptr, nullptr, nullptr );
    toolMgr.RegisterTool( dummyTool );

    PCB_GROUP*   group = board->Groups().front();
    FOOTPRINT*   existing = board->Footprints().front();
    FOOTPRINT*   updated = static_cast<FOOTPRINT*>( existing->Clone() );
    const size_t memberCount = group->GetItems().size();

    // Push() records this undo list, but keeps none without a frame
    PICKED_ITEMS_LIST undoList;
    ITEM_PICKER       groupPicker( nullptr, group, UNDO_REDO::CHANGED );

    groupPicker.SetLink( BOARD_COMMIT::MakeImage( group ) );

    BOARD_COMMIT commit( dummyTool );

    board->ExchangeFootprint( existing, updated, commit, FLIP_DIRECTION::TOP_BOTTOM, true );
    commit.Push( wxT( "Update Footprint" ) );

    ITEM_PICKER existingPicker( nullptr, existing, UNDO_REDO::DELETED );

    existingPicker.SetLink( BOARD_COMMIT::MakeImage( existing ) );

    undoList.PushItem( groupPicker );
    undoList.PushItem( existingPicker );
    undoList.PushItem( ITEM_PICKER( nullptr, updated, UNDO_REDO::NEWITEM ) );

    applyUndoList( *board, undoList );

    BOOST_CHECK_EQUAL( group->GetItems().size(), memberCount );
    BOOST_CHECK( group->GetItems().contains( existing ) );
    BOOST_CHECK( existing->GetParentGroup() == group );

    applyUndoList( *board, undoList );

    BOOST_CHECK_EQUAL( group->GetItems().size(), memberCount );
    BOOST_CHECK( group->GetItems().contains( updated ) );
    BOOST_CHECK( updated->GetParentGroup() == group );

    undoList.ClearListAndDeleteItems( []( EDA_ITEM* aItem ) { delete aItem; } );
}
