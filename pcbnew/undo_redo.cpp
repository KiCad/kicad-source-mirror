/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <functional>
using namespace std::placeholders;
#include <fctsys.h>
#include <class_draw_panel_gal.h>
#include <macros.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <class_board.h>
#include <class_track.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_pcb_target.h>
#include <class_module.h>
#include <class_dimension.h>
#include <class_zone.h>
#include <class_edge_mod.h>
#include <origin_viewitem.h>
#include <connectivity/connectivity_data.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <tools/selection_tool.h>
#include <tools/pcbnew_control.h>
#include <tools/pcb_editor_control.h>
#include <view/view.h>
#include <ws_proxy_undo_item.h>

/* Functions to undo and redo edit commands.
 *  commands to undo are stored in CurrentScreen->m_UndoList
 *  commands to redo are stored in CurrentScreen->m_RedoList
 *
 *  m_UndoList and m_RedoList handle a std::vector of PICKED_ITEMS_LIST
 *  Each PICKED_ITEMS_LIST handle a std::vector of pickers (class ITEM_PICKER),
 *  that store the list of schematic items that are concerned by the command to undo or redo
 *  and is created for each command to undo (handle also a command to redo).
 *  each picker has a pointer pointing to an item to undo or redo (in fact: deleted, added or
 *  modified),
 * and has a pointer to a copy of this item, when this item has been modified
 * (the old values of parameters are therefore saved)
 *
 *  there are 3 cases:
 *  - delete item(s) command
 *  - change item(s) command
 *  - add item(s) command
 *
 *  Undo command
 *  - delete item(s) command:
 *       =>  deleted items are moved in undo list
 *
 *  - change item(s) command
 *      => A copy of item(s) is made (a DrawPickedStruct list of wrappers)
 *      the .m_Link member of each wrapper points the modified item.
 *      the .m_Item member of each wrapper points the old copy of this item.
 *
 *  - add item(s) command
 *      =>A list of item(s) is made. The .m_Item member of each wrapper points the new item.
 *
 *  Redo command
 *  - delete item(s) old command:
 *      => deleted items are moved in EEDrawList list, and in
 *
 *  - change item(s) command
 *      => the copy of item(s) is moved in Undo list
 *
 *  - add item(s) command
 *      => The list of item(s) is used to create a deleted list in undo list(same as a delete
 *         command)
 *
 *   Some block operations that change items can be undone without memorize items, just the
 *   coordinates of the transform:
 *      move list of items (undo/redo is made by moving with the opposite move vector)
 *      mirror (Y) and flip list of items (undo/redo is made by mirror or flip items)
 *      so they are handled specifically.
 *
 */


/**
 * Function TestForExistingItem
 * test if aItem exists somewhere in lists of items
 * This is a function used by PutDataInPreviousState to be sure an item was not deleted
 * since an undo or redo.
 * This could be possible:
 *   - if a call to SaveCopyInUndoList was forgotten in Pcbnew
 *   - in zones outlines, when a change in one zone merges this zone with an other
 * This function avoids a Pcbnew crash
 * Before using this function to test existence of items,
 * it must be called with aItem = NULL to prepare the list
 * @param aPcb = board to test
 * @param aItem = item to find
 *              = NULL to build the list of existing items
 */

static bool TestForExistingItem( BOARD* aPcb, BOARD_ITEM* aItem )
{
    for( auto item : aPcb->Tracks() )
    {
        if( aItem == static_cast<BOARD_ITEM*>( item ) )
            return true;
    }

    // Append modules:
    for( auto item : aPcb->Modules() )
    {
        if( aItem == static_cast<BOARD_ITEM*>( item ) )
            return true;
    }

    // Append drawings
    for( auto item : aPcb->Drawings() )
    {
        if( aItem == static_cast<BOARD_ITEM*>( item ) )
            return true;
    }

    // Append zones outlines
    for( auto item : aPcb->Zones() )
    {
        if( aItem == static_cast<BOARD_ITEM*>( item ) )
            return true;
    }

    NETINFO_LIST& netInfo = aPcb->GetNetInfo();

    for( NETINFO_LIST::iterator i = netInfo.begin(); i != netInfo.end(); ++i )
    {
        if( aItem == static_cast<BOARD_ITEM*>( *i ) )
            return true;
    }

    return false;
}

static void SwapItemData( BOARD_ITEM* aItem, BOARD_ITEM* aImage )
{
    if( aImage == NULL )
        return;

    wxASSERT( aItem->Type() == aImage->Type() );

    // Remark: to create images of edited items to undo, we are using Clone method
    // which can duplication of items foe copy, but does not clone all members
    // mainly pointers in chain and time stamp, which is set to new, unique value.
    // So we have to use the current values of these parameters.

    timestamp_t timestamp = aItem->GetTimeStamp();
    EDA_ITEM* parent = aItem->GetParent();

    aItem->SwapData( aImage );

    // Restore pointers and time stamp, to be sure they are not broken
    aItem->SetTimeStamp( timestamp );
    aItem->SetParent( parent );
}

void PCB_BASE_EDIT_FRAME::SaveCopyInUndoList( BOARD_ITEM* aItem, UNDO_REDO_T aCommandType,
                                              const wxPoint& aTransformPoint )
{
    PICKED_ITEMS_LIST commandToUndo;
    commandToUndo.PushItem( ITEM_PICKER( aItem, aCommandType ) );
    SaveCopyInUndoList( commandToUndo, aCommandType, aTransformPoint );
}


void PCB_BASE_EDIT_FRAME::SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
                                              UNDO_REDO_T aTypeCommand,
                                              const wxPoint& aTransformPoint )
{
    static KICAD_T moduleChildren[] = { PCB_MODULE_TEXT_T, PCB_MODULE_EDGE_T, PCB_PAD_T, EOT };

    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();

    commandToUndo->m_TransformPoint = aTransformPoint;

    // First, filter unnecessary stuff from the list (i.e. for multiple pads / labels modified),
    // take the first occurence of the module (we save copies of modules when one of its subitems
    // is changed).
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        ITEM_PICKER curr_picker = aItemsList.GetItemWrapper(ii);
        BOARD_ITEM* item        = dynamic_cast<BOARD_ITEM*>( aItemsList.GetPickedItem( ii ) );

        // For items belonging to modules, we need to save state of the parent module
        if( item && item->IsType( moduleChildren ) )
        {
            // Item to be stored in the undo buffer is the parent module
            item = item->GetParent();
            wxASSERT( item && item->Type() == PCB_MODULE_T );

            if( item == NULL )
                continue;

            // Check if the parent module has already been saved in another entry
            bool found = false;

            for( unsigned j = 0; j < commandToUndo->GetCount(); j++ )
            {
                if( commandToUndo->GetPickedItem( j ) == item
                        && commandToUndo->GetPickedItemStatus( j ) == UR_CHANGED )
                {
                    found = true;
                    break;
                }
            }

            if( !found )
            {
                // Create a clean copy of the parent module
                MODULE* orig = static_cast<MODULE*>( item );
                MODULE* clone = new MODULE( *orig );
                clone->SetParent( GetBoard() );

                // Clear current flags (which can be temporary set by a current edit command)
                for( auto child : clone->GraphicalItems() )
                    child->ClearEditFlags();

                for( auto pad : clone->Pads() )
                    pad->ClearEditFlags();

                clone->Reference().ClearEditFlags();
                clone->Value().ClearEditFlags();

                ITEM_PICKER picker( item, UR_CHANGED );
                picker.SetLink( clone );
                commandToUndo->PushItem( picker );

                orig->SetLastEditTime();
            }
            else
            {
                continue;
            }
        }
        else
        {
            // Normal case: all other BOARD_ITEMs, are simply copied to the new list
            commandToUndo->PushItem( curr_picker );
        }
    }

    for( unsigned ii = 0; ii < commandToUndo->GetCount(); ii++ )
    {
        EDA_ITEM*   item    = aItemsList.GetPickedItem( ii );
        UNDO_REDO_T command = commandToUndo->GetPickedItemStatus( ii );

        if( command == UR_UNSPECIFIED )
        {
            command = aTypeCommand;
            commandToUndo->SetPickedItemStatus( command, ii );
        }

        wxASSERT( item );

        switch( command )
        {
        case UR_CHANGED:
        case UR_DRILLORIGIN:
        case UR_GRIDORIGIN:

            /* If needed, create a copy of item, and put in undo list
             * in the picker, as link
             * If this link is not null, the copy is already done
             */
            if( commandToUndo->GetPickedItemLink( ii ) == NULL )
            {
                EDA_ITEM* cloned = item->Clone();
                commandToUndo->SetPickedItemLink( cloned, ii );
            }
            break;

        case UR_MOVED:
        case UR_ROTATED:
        case UR_ROTATED_CLOCKWISE:
        case UR_FLIPPED:
        case UR_NEW:
        case UR_DELETED:
        case UR_PAGESETTINGS:
            break;

        default:
        {
            wxLogDebug( wxT( "SaveCopyInUndoList() error (unknown code %X)" ), command );
        }
        break;

        }
    }

    if( commandToUndo->GetCount() )
    {
        /* Save the copy in undo list */
        GetScreen()->PushCommandToUndoList( commandToUndo );

        /* Clear redo list, because after a new command one cannot redo a command */
        GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
    }
    else
    {
        // Should not occur
        wxASSERT( false );
        delete commandToUndo;
    }
}


void PCB_BASE_EDIT_FRAME::RestoreCopyFromUndoList( wxCommandEvent& aEvent )
{
    if( UndoRedoBlocked() )
        return;

    if( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    // Inform tools that undo command was issued
    m_toolManager->ProcessEvent( { TC_MESSAGE, TA_UNDO_REDO_PRE, AS_GLOBAL } );

    // Get the old list
    PICKED_ITEMS_LIST* List = GetScreen()->PopCommandFromUndoList();

    // Undo the command
    PutDataInPreviousState( List, false );

    // Put the old list in RedoList
    List->ReversePickersListOrder();
    GetScreen()->PushCommandToRedoList( List );

    OnModify();

    m_toolManager->ProcessEvent( { TC_MESSAGE, TA_UNDO_REDO_POST, AS_GLOBAL } );

    GetCanvas()->Refresh();
}


void PCB_BASE_EDIT_FRAME::RestoreCopyFromRedoList( wxCommandEvent& aEvent )
{
    if( UndoRedoBlocked() )
        return;

    if( GetScreen()->GetRedoCommandCount() == 0 )
        return;

    // Inform tools that redo command was issued
    m_toolManager->ProcessEvent( { TC_MESSAGE, TA_UNDO_REDO_PRE, AS_GLOBAL } );

    // Get the old list
    PICKED_ITEMS_LIST* List = GetScreen()->PopCommandFromRedoList();

    // Redo the command
    PutDataInPreviousState( List, true );

    // Put the old list in UndoList
    List->ReversePickersListOrder();
    GetScreen()->PushCommandToUndoList( List );

    OnModify();

    m_toolManager->ProcessEvent( { TC_MESSAGE, TA_UNDO_REDO_POST, AS_GLOBAL } );

    GetCanvas()->Refresh();
}


void PCB_BASE_EDIT_FRAME::PutDataInPreviousState( PICKED_ITEMS_LIST* aList, bool aRedoCommand,
                                                  bool aRebuildRatsnet )
{
    bool not_found = false;
    bool reBuild_ratsnest = false;
    bool deep_reBuild_ratsnest = false;  // true later if pointers must be rebuilt

    auto view = GetCanvas()->GetView();
    auto connectivity = GetBoard()->GetConnectivity();

    // Undo in the reverse order of list creation: (this can allow stacked changes
    // like the same item can be changes and deleted in the same complex command

    // Restore changes in reverse order
    for( int ii = aList->GetCount() - 1; ii >= 0 ; ii-- )
    {
        EDA_ITEM* eda_item = aList->GetPickedItem( (unsigned) ii );

        /* Test for existence of item on board.
         * It could be deleted, and no more on board:
         *   - if a call to SaveCopyInUndoList was forgotten in Pcbnew
         *   - in zones outlines, when a change in one zone merges this zone with an other
         * This test avoids a Pcbnew crash
         * Obviously, this test is not made for deleted items
         */
        UNDO_REDO_T status = aList->GetPickedItemStatus( ii );

        if( status != UR_DELETED
                && status != UR_DRILLORIGIN     // origin markers never on board
                && status != UR_GRIDORIGIN      // origin markers never on board
                && status != UR_PAGESETTINGS )  // nor are page settings proxy items
        {
            if( !TestForExistingItem( GetBoard(), (BOARD_ITEM*) eda_item ) )
            {
                // Checking if it ever happens
                wxASSERT_MSG( false, "Item in the undo buffer does not exist" );

                // Remove this non existent item
                aList->RemovePicker( ii );
                ii++;       // the current item was removed, ii points now the next item
                            // decrement it because it will be incremented later
                not_found = true;

                if( aList->GetCount() == 0 )
                    break;

                continue;
            }
        }

        // see if we must rebuild ratsnets and pointers lists
        switch( eda_item->Type() )
        {
        case PCB_MODULE_T:
            deep_reBuild_ratsnest = true;   // Pointers on pads can be invalid
            // Fall through
        case PCB_ZONE_AREA_T:
        case PCB_TRACE_T:
        case PCB_VIA_T:
            reBuild_ratsnest = true;
            break;

        case PCB_NETINFO_T:
            reBuild_ratsnest = true;
            deep_reBuild_ratsnest = true;
            break;

        default:
            break;
        }

        switch( aList->GetPickedItemStatus( ii ) )
        {
        case UR_CHANGED:    /* Exchange old and new data for each item */
        {
            BOARD_ITEM* item = (BOARD_ITEM*) eda_item;
            BOARD_ITEM* image = (BOARD_ITEM*) aList->GetPickedItemLink( ii );

            // Remove all pads/drawings/texts, as they become invalid
            // for the VIEW after SwapData() called for modules
            view->Remove( eda_item );
            connectivity->Remove( item );

            SwapItemData( item, image );

            view->Add( eda_item );
            connectivity->Add( item );
        }
        break;

        case UR_NEW:        /* new items are deleted */
            aList->SetPickedItemStatus( UR_DELETED, ii );
            GetModel()->Remove( (BOARD_ITEM*) eda_item );
            view->Remove( eda_item );
            break;

        case UR_DELETED:    /* deleted items are put in List, as new items */
            aList->SetPickedItemStatus( UR_NEW, ii );
            GetModel()->Add( (BOARD_ITEM*) eda_item );
            view->Add( eda_item );
            break;

        case UR_MOVED:
        {
            BOARD_ITEM* item = (BOARD_ITEM*) eda_item;
            item->Move( aRedoCommand ? aList->m_TransformPoint : -aList->m_TransformPoint );
            view->Update( item, KIGFX::GEOMETRY );
            connectivity->Update( item );
        }
            break;

        case UR_ROTATED:
        {
            BOARD_ITEM* item = (BOARD_ITEM*) eda_item;
            item->Rotate( aList->m_TransformPoint,
                          aRedoCommand ? m_rotationAngle : -m_rotationAngle );
            view->Update( item, KIGFX::GEOMETRY );
            connectivity->Update( item );
        }
            break;

        case UR_ROTATED_CLOCKWISE:
        {
            BOARD_ITEM* item = (BOARD_ITEM*) eda_item;
            item->Rotate( aList->m_TransformPoint,
                          aRedoCommand ? -m_rotationAngle : m_rotationAngle );
            view->Update( item, KIGFX::GEOMETRY );
            connectivity->Update( item );
        }
            break;

        case UR_FLIPPED:
        {
            BOARD_ITEM* item = (BOARD_ITEM*) eda_item;
            item->Flip( aList->m_TransformPoint, m_configSettings.m_FlipLeftRight );
            view->Update( item, KIGFX::LAYERS );
            connectivity->Update( item );
        }
            break;

        case UR_DRILLORIGIN:
        case UR_GRIDORIGIN:
        {
            BOARD_ITEM* item = (BOARD_ITEM*) eda_item;
            BOARD_ITEM* image = (BOARD_ITEM*) aList->GetPickedItemLink( ii );
            VECTOR2D origin = image->GetPosition();
            image->SetPosition( item->GetPosition() );

            if( aList->GetPickedItemStatus( ii ) == UR_DRILLORIGIN )
                PCB_EDITOR_CONTROL::DoSetDrillOrigin( view, this, item, origin );
            else
                PCBNEW_CONTROL::DoSetGridOrigin( view, this, item, origin );
        }
        break;

        case UR_PAGESETTINGS:
        {
            // swap current settings with stored settings
            WS_PROXY_UNDO_ITEM  alt_item( this );
            WS_PROXY_UNDO_ITEM* item = (WS_PROXY_UNDO_ITEM*) eda_item;
            item->Restore( this );
            *item = alt_item;
            GetToolManager()->RunAction( ACTIONS::zoomFitScreen, true );
        }
        break;

        default:
            wxLogDebug( wxT( "PutDataInPreviousState() error (unknown code %X)" ),
                        aList->GetPickedItemStatus( ii ) );
            break;
        }
    }

    if( not_found )
        wxMessageBox( _( "Incomplete undo/redo operation: some items not found" ) );
    
    // Rebuild pointers and connectivity that can be changed.
    // connectivity can be rebuilt only in the board editor frame
    if( IsType( FRAME_PCB ) && ( reBuild_ratsnest || deep_reBuild_ratsnest ) )
    {
        Compile_Ratsnest( false );
    }

    SELECTION_TOOL* selTool = m_toolManager->GetTool<SELECTION_TOOL>();
    selTool->RebuildSelection();

    GetBoard()->SanitizeNetcodes();
}




void PCB_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    unsigned icnt = aList.m_CommandsList.size();

    if( aItemCount > 0 )
        icnt = aItemCount;

    for( unsigned ii = 0; ii < icnt; ii++ )
    {
        if( aList.m_CommandsList.size() == 0 )
            break;

        PICKED_ITEMS_LIST* curr_cmd = aList.m_CommandsList[0];
        aList.m_CommandsList.erase( aList.m_CommandsList.begin() );

        curr_cmd->ClearListAndDeleteItems();
        delete curr_cmd;    // Delete command
    }
}


void PCB_BASE_EDIT_FRAME::RollbackFromUndo()
{
    PICKED_ITEMS_LIST* undo = GetScreen()->PopCommandFromUndoList();
    PutDataInPreviousState( undo, false );

    undo->ClearListAndDeleteItems();
    delete undo;

    GetCanvas()->Refresh();
}
