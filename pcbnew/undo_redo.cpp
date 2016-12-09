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
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>
#include <macros.h>

#include <pcbnew.h>
#include <wxPcbStruct.h>

#include <class_board.h>
#include <class_track.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_mire.h>
#include <class_module.h>
#include <class_dimension.h>
#include <class_zone.h>
#include <class_edge_mod.h>

#include <ratsnest_data.h>

#include <tools/selection_tool.h>
#include <tool/tool_manager.h>

#include <view/view.h>

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
 *  and 3 cases for block:
 *  - move list of items
 *  - mirror (Y) list of items
 *  - Flip list of items
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
    static std::list<BOARD_ITEM*> itemsList;

    if( aItem == NULL ) // Build list
    {
        // Count items to store in itemsList:
        BOARD_ITEM* item;
        itemsList.clear();

        // Store items in list:
        // Append tracks:
        for( item = aPcb->m_Track; item != NULL; item = item->Next() )
            itemsList.push_back( item );

        // Append modules:
        for( item = aPcb->m_Modules; item != NULL; item = item->Next() )
            itemsList.push_back( item );

        // Append drawings
        for( item = aPcb->m_Drawings; item != NULL; item = item->Next() )
            itemsList.push_back( item );

        // Append zones outlines
        for( int ii = 0; ii < aPcb->GetAreaCount(); ii++ )
            itemsList.push_back( aPcb->GetArea( ii ) );

        // Append zones segm:
        for( item = aPcb->m_Zone; item != NULL; item = item->Next() )
            itemsList.push_back( item );

        NETINFO_LIST& netInfo = aPcb->GetNetInfo();

        for( NETINFO_LIST::iterator i = netInfo.begin(); i != netInfo.end(); ++i )
            itemsList.push_back( *i );

        // Sort list
        itemsList.sort();

        return false;
    }

    // search in list:
    return std::binary_search( itemsList.begin(), itemsList.end(), aItem );
}


void PCB_BASE_EDIT_FRAME::SaveCopyInUndoList( BOARD_ITEM* aItem, UNDO_REDO_T aCommandType,
                                              const wxPoint& aTransformPoint )
{
    PICKED_ITEMS_LIST commandToUndo;
    commandToUndo.PushItem( ITEM_PICKER( aItem, aCommandType ) );
    SaveCopyInUndoList( commandToUndo, aCommandType, aTransformPoint );
}


void PCB_BASE_EDIT_FRAME::SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
                                         UNDO_REDO_T aTypeCommand, const wxPoint& aTransformPoint )
{
    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();

    commandToUndo->m_TransformPoint = aTransformPoint;

    // First, filter unnecessary stuff from the list (i.e. for multiple pads / labels modified),
    // take the first occurence of the module (we save copies of modules when one of its subitems
    // is changed).
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        ITEM_PICKER curr_picker = aItemsList.GetItemWrapper(ii);
        BOARD_ITEM* item    = (BOARD_ITEM*) aItemsList.GetPickedItem( ii );

        // For items belonging to modules, we need to save state of the parent module
        if( item->Type() == PCB_MODULE_TEXT_T || item->Type() == PCB_MODULE_EDGE_T
                || item->Type() == PCB_PAD_T )
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
                if( commandToUndo->GetPickedItem( j ) == item && commandToUndo->GetPickedItemStatus( j ) == UR_CHANGED )
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
                for( EDA_ITEM* loc_item = clone->GraphicalItems(); loc_item;
                        loc_item = loc_item->Next() )
                    loc_item->ClearFlags();

                for( D_PAD* pad = clone->Pads(); pad; pad = pad->Next() )
                    pad->ClearFlags();

                clone->Reference().ClearFlags();
                clone->Value().ClearFlags();

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
        BOARD_ITEM* item    = (BOARD_ITEM*) commandToUndo->GetPickedItem( ii );
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
            break;

        default:
        {
            wxString msg;
            msg.Printf( wxT( "SaveCopyInUndoList() error (unknown code %X)" ), command );
            wxMessageBox( msg );
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

    m_canvas->Refresh();
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

    m_canvas->Refresh();
}


void PCB_BASE_EDIT_FRAME::PutDataInPreviousState( PICKED_ITEMS_LIST* aList, bool aRedoCommand,
                                             bool aRebuildRatsnet )
{
    BOARD_ITEM* item;
    bool        not_found = false;
    bool        reBuild_ratsnest = false;
    bool        deep_reBuild_ratsnest = false;  // true later if pointers must be rebuilt

    KIGFX::VIEW* view = GetGalCanvas()->GetView();
    RN_DATA* ratsnest = GetBoard()->GetRatsnest();

    // Undo in the reverse order of list creation: (this can allow stacked changes
    // like the same item can be changes and deleted in the same complex command

    bool build_item_list = true;    // if true the list of existing items must be rebuilt

    // Restore changes in reverse order
    for( int ii = aList->GetCount() - 1; ii >= 0 ; ii-- )
    {
        item = (BOARD_ITEM*) aList->GetPickedItem( ii );
        wxASSERT( item );

        /* Test for existence of item on board.
         * It could be deleted, and no more on board:
         *   - if a call to SaveCopyInUndoList was forgotten in Pcbnew
         *   - in zones outlines, when a change in one zone merges this zone with an other
         * This test avoids a Pcbnew crash
         * Obviously, this test is not made for deleted items
         */
        UNDO_REDO_T status = aList->GetPickedItemStatus( ii );

        if( status != UR_DELETED )
        {
            if( build_item_list )
                // Build list of existing items, for integrity test
                TestForExistingItem( GetBoard(), NULL );

            build_item_list = false;

            if( !TestForExistingItem( GetBoard(), item ) )
            {
                // Checking if it ever happens
                wxASSERT_MSG( false, "Item in the undo buffer does not exist" );

                // Remove this non existent item
                aList->RemovePicker( ii );
                ii++;       // the current item was removed, ii points now the next item
                            // decrement it because it will be incremented later
                not_found = true;
                continue;
            }
        }

        item->ClearFlags();

        // see if we must rebuild ratsnets and pointers lists
        switch( item->Type() )
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

        // It is possible that we are going to replace the selected item, so clear it
        SetCurItem( NULL );

        switch( aList->GetPickedItemStatus( ii ) )
        {
        case UR_CHANGED:    /* Exchange old and new data for each item */
        {
            BOARD_ITEM* image = (BOARD_ITEM*) aList->GetPickedItemLink( ii );

            // Remove all pads/drawings/texts, as they become invalid
            // for the VIEW after SwapData() called for modules
            if( item->Type() == PCB_MODULE_T )
            {
                MODULE* oldModule = static_cast<MODULE*>( item );
                oldModule->RunOnChildren( std::bind( &KIGFX::VIEW::Remove, view, _1 ) );
            }

            view->Remove( item );
            ratsnest->Remove( item );

            item->SwapData( image );

            // Update all pads/drawings/texts, as they become invalid
            // for the VIEW after SwapData() called for modules
            if( item->Type() == PCB_MODULE_T )
            {
                MODULE* newModule = static_cast<MODULE*>( item );
                newModule->RunOnChildren( std::bind( &KIGFX::VIEW::Add, view, _1 ) );
                newModule->RunOnChildren( std::bind( &BOARD_ITEM::ClearFlags, _1, EDA_ITEM_ALL_FLAGS ));
            }

            view->Add( item );
            ratsnest->Add( item );
            item->ClearFlags();

        }
        break;

        case UR_NEW:        /* new items are deleted */
            aList->SetPickedItemStatus( UR_DELETED, ii );
            GetModel()->Remove( item );

            if( item->Type() == PCB_MODULE_T )
            {
                MODULE* module = static_cast<MODULE*>( item );
                module->RunOnChildren( std::bind( &KIGFX::VIEW::Remove, view, _1 ) );
            }

            view->Remove( item );
            break;

        case UR_DELETED:    /* deleted items are put in List, as new items */
            aList->SetPickedItemStatus( UR_NEW, ii );
            GetModel()->Add( item );

            if( item->Type() == PCB_MODULE_T )
            {
                MODULE* module = static_cast<MODULE*>( item );
                module->RunOnChildren( std::bind( &KIGFX::VIEW::Add, view, _1) );
            }

            view->Add( item );
            build_item_list = true;
            break;

        case UR_MOVED:
            item->Move( aRedoCommand ? aList->m_TransformPoint : -aList->m_TransformPoint );
            view->Update( item, KIGFX::GEOMETRY );
            ratsnest->Update( item );
            break;

        case UR_ROTATED:
            item->Rotate( aList->m_TransformPoint,
                          aRedoCommand ? m_rotationAngle : -m_rotationAngle );
            view->Update( item, KIGFX::GEOMETRY );
            ratsnest->Update( item );
            break;

        case UR_ROTATED_CLOCKWISE:
            item->Rotate( aList->m_TransformPoint,
                          aRedoCommand ? -m_rotationAngle : m_rotationAngle );
            view->Update( item, KIGFX::GEOMETRY );
            ratsnest->Update( item );
            break;

        case UR_FLIPPED:
            item->Flip( aList->m_TransformPoint );
            view->Update( item, KIGFX::LAYERS );
            ratsnest->Update( item );
            break;

        default:
        {
            wxString msg;
            msg.Printf( wxT( "PutDataInPreviousState() error (unknown code %X)" ),
                        aList->GetPickedItemStatus( ii ) );
            wxMessageBox( msg );
        }
        break;
        }
    }

    if( not_found )
        wxMessageBox( wxT( "Incomplete undo/redo operation: some items not found" ) );

    // Rebuild pointers and ratsnest that can be changed.
    if( reBuild_ratsnest )
    {
        // Compile ratsnest propagates nets from pads to tracks
        /// @todo LEGACY Compile_Ratsnest() has to be rewritten and moved to RN_DATA
        if( deep_reBuild_ratsnest )
            Compile_Ratsnest( NULL, false );

        if( IsGalCanvasActive() )
        {
            if( deep_reBuild_ratsnest )
                ratsnest->ProcessBoard();
            else
                ratsnest->Recalculate();
        }
    }
}


void BOARD_ITEM::SwapData( BOARD_ITEM* aImage )
{
    if( aImage == NULL )
        return;

    wxASSERT( Type() == aImage->Type() );

    // Remark: to create images of edited items to undo, we are using Clone method
    // which can duplication of items foe copy, but does not clone all members
    // mainly pointers in chain and time stamp, which is set to new, unique value.
    // So we have to use the current values of these parameters.

    EDA_ITEM* pnext = Next();
    EDA_ITEM* pback = Back();
    DHEAD* mylist    = m_List;
    time_t timestamp = GetTimeStamp();
    EDA_ITEM* parent = GetParent();

    switch( Type() )
    {
    case PCB_MODULE_T:
        std::swap( *((MODULE*) this), *((MODULE*) aImage) );
        break;

    case PCB_ZONE_AREA_T:
        std::swap( *((ZONE_CONTAINER*) this), *((ZONE_CONTAINER*) aImage) );
        break;

    case PCB_LINE_T:
        std::swap( *((DRAWSEGMENT*) this), *((DRAWSEGMENT*) aImage) );
        break;

    case PCB_TRACE_T:
        std::swap( *((TRACK*) this), *((TRACK*) aImage) );
        break;

    case PCB_VIA_T:
        std::swap( *((VIA*) this), *((VIA*) aImage) );
        break;

    case PCB_TEXT_T:
        std::swap( *((TEXTE_PCB*)this), *((TEXTE_PCB*)aImage) );
        break;

    case PCB_TARGET_T:
        std::swap( *((PCB_TARGET*)this), *((PCB_TARGET*)aImage) );
        break;

    case PCB_DIMENSION_T:
        std::swap( *((DIMENSION*)this), *((DIMENSION*)aImage) );
        break;

    case PCB_ZONE_T:
    default:
        wxLogMessage( wxT( "SwapData() error: unexpected type %d" ), Type() );
        break;
    }

    // Restore pointers and time stamp, to be sure they are not broken
    Pnext = pnext;
    Pback = pback;
    m_List = mylist;
    SetTimeStamp( timestamp );
    SetParent( parent );
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
