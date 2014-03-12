/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file schematic_undo_redo.cpp
 * @brief Eeschema undo and redo functions for schematic editor.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <wxEeschemaStruct.h>

#include <general.h>
#include <protos.h>
#include <sch_bus_entry.h>
#include <sch_marker.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>


/* Functions to undo and redo edit commands.
 *  commands to undo are stored in CurrentScreen->m_UndoList
 *  commands to redo are stored in CurrentScreen->m_RedoList
 *
 *  m_UndoList and m_RedoList handle a std::vector of PICKED_ITEMS_LIST
 *  Each PICKED_ITEMS_LIST handle a std::vector of pickers (class ITEM_PICKER),
 *  that store the list of schematic items that are concerned by the command to
 *  undo or redo and is created for each command to undo (handle also a command
 *  to redo). each picker has a pointer pointing to an item to undo or redo (in
 *  fact: deleted, added or modified), and has a pointer to a copy of this item,
 *  when this item has been modified (the old values of parameters are
 *  therefore saved)
 *
 *  there are 3 cases:
 *  - delete item(s) command
 *  - change item(s) command
 *  - add item(s) command
 *  and 2 cases for block:
 *  - move list of items
 *  - mirror (Y) list of items
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
 *      =>A list of item(s) is made. The .m_Item member of each wrapper points
 *        the new item.
 *
 *  Redo command
 *  - delete item(s) old command:
 *      => deleted items are moved in GetDrawItems() list, and in
 *
 *  - change item(s) command
 *      => the copy of item(s) is moved in Undo list
 *
 *  - add item(s) command
 *      => The list of item(s) is used to create a deleted list in undo
 *         list(same as a delete command)
 *
 *   Some block operations that change items can be undone without memorized
 *   items, just the coordinates of the transform: move list of items (undo/
 *   redo is made by moving with the opposite move vector) mirror (Y) and flip
 *   list of items (undo/redo is made by mirror or flip items) so they are
 *    handled specifically.
 *
 *  A problem is the hierarchical sheet handling.
 *  the data associated (sub-hierarchy, undo/redo list) is deleted only
 *  when the sheet is really deleted (i.e. when deleted from undo or redo list)
 *  This is handled by its destructor.
 */


/* Used if undo / redo command:
 *  swap data between Item and its copy, pointed by its .m_Image member
 * swapped data is data modified by edition, so not all values are swapped
 */

void SCH_EDIT_FRAME::SaveCopyInUndoList( SCH_ITEM*      aItem,
                                         UNDO_REDO_T    aCommandType,
                                         const wxPoint& aTransformPoint )
{
    /* Does not save a null item or a UR_WIRE_IMAGE command type.  UR_WIRE_IMAGE commands
     * are handled by the overloaded version of SaveCopyInUndoList that takes a reference
     * to a PICKED_ITEMS_LIST.
     */
    if( aItem == NULL || aCommandType == UR_WIRE_IMAGE )
        return;

    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();
    commandToUndo->m_TransformPoint = aTransformPoint;

    ITEM_PICKER itemWrapper( aItem, aCommandType );

    if( aItem )
        itemWrapper.SetFlags( aItem->GetFlags() );

    switch( aCommandType )
    {
    case UR_CHANGED:            /* Create a copy of item */
        itemWrapper.SetLink( DuplicateStruct( aItem, true ) );
        commandToUndo->PushItem( itemWrapper );
        break;

    case UR_NEW:
    case UR_DELETED:
    case UR_ROTATED:
    case UR_MOVED:
        commandToUndo->PushItem( itemWrapper );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "SaveCopyInUndoList() error (unknown code %X)" ),
                                      aCommandType ) );
        break;
    }

    if( commandToUndo->GetCount() )
    {
        /* Save the copy in undo list */
        GetScreen()->PushCommandToUndoList( commandToUndo );

        /* Clear redo list, because after new save there is no redo to do */
        GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
    }
    else
    {
        delete commandToUndo;
    }
}


void SCH_EDIT_FRAME::SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
                                         UNDO_REDO_T        aTypeCommand,
                                         const wxPoint&     aTransformPoint )
{
    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();

    commandToUndo->m_TransformPoint = aTransformPoint;
    commandToUndo->m_Status = aTypeCommand;

    // Copy picker list:
    commandToUndo->CopyList( aItemsList );

    // Verify list, and creates data if needed
    for( unsigned ii = 0; ii < commandToUndo->GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) commandToUndo->GetPickedItem( ii );
        wxASSERT( item );

        UNDO_REDO_T command = commandToUndo->GetPickedItemStatus( ii );

        if( command == UR_UNSPECIFIED )
        {
            command = aTypeCommand;
            commandToUndo->SetPickedItemStatus( command, ii );
        }

        switch( command )
        {
        case UR_CHANGED:        /* Create a copy of item */

            /* If needed, create a copy of item, and put in undo list
             * in the picker, as link
             * If this link is not null, the copy is already done
             */
            if( commandToUndo->GetPickedItemLink( ii ) == NULL )
                commandToUndo->SetPickedItemLink( DuplicateStruct( item, true ), ii );

            wxASSERT( commandToUndo->GetPickedItemLink( ii ) );
            break;

        case UR_MOVED:
        case UR_MIRRORED_Y:
        case UR_MIRRORED_X:
        case UR_ROTATED:
        case UR_NEW:
        case UR_DELETED:
        case UR_EXCHANGE_T:
        case UR_WIRE_IMAGE:
            break;

        default:
            wxFAIL_MSG( wxString::Format( wxT( "Unknown undo/redo command %d" ), command ) );
            break;
        }
    }

    if( commandToUndo->GetCount() || aTypeCommand == UR_WIRE_IMAGE )
    {
        /* Save the copy in undo list */
        GetScreen()->PushCommandToUndoList( commandToUndo );

        /* Clear redo list, because after new save there is no redo to do */
        GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
    }
    else    // Should not occur
    {
        delete commandToUndo;
    }
}


void SCH_EDIT_FRAME::PutDataInPreviousState( PICKED_ITEMS_LIST* aList, bool aRedoCommand )
{
    SCH_ITEM* item;
    SCH_ITEM* alt_item;

    // Exchange the current wires, buses, and junctions with the copy save by the last edit.
    if( aList->m_Status == UR_WIRE_IMAGE )
    {
        DLIST< SCH_ITEM > oldWires;

        // Prevent items from being deleted when the DLIST goes out of scope.
        oldWires.SetOwnership( false );

        // Remove all of the wires, buses, and junctions from the current screen.
        GetScreen()->ExtractWires( oldWires, false );

        // Copy the saved wires, buses, and junctions to the current screen.
        for( unsigned int i = 0;  i < aList->GetCount();  i++ )
            GetScreen()->Append( (SCH_ITEM*) aList->GetPickedItem( i ) );

        aList->ClearItemsList();

        // Copy the previous wires, buses, and junctions to the picked item list for the
        // redo operation.
        while( oldWires.GetCount() != 0 )
        {
            ITEM_PICKER picker = ITEM_PICKER( oldWires.PopFront(), UR_WIRE_IMAGE );
            aList->PushItem( picker );
        }

        return;
    }

    // Undo in the reverse order of list creation: (this can allow stacked changes like the
    // same item can be changes and deleted in the same complex command.
    for( int ii = aList->GetCount() - 1; ii >= 0; ii--  )
    {
        item = (SCH_ITEM*) aList->GetPickedItem( ii );
        wxASSERT( item );

        item->ClearFlags();

        SCH_ITEM* image = (SCH_ITEM*) aList->GetPickedItemLink( ii );

        switch( aList->GetPickedItemStatus( ii ) )
        {
        case UR_CHANGED: /* Exchange old and new data for each item */
            item->SwapData( image );
            break;

        case UR_NEW:     /* new items are deleted */
            aList->SetPickedItemStatus( UR_DELETED, ii );
            GetScreen()->Remove( item );
            break;

        case UR_DELETED: /* deleted items are put in the draw item list, as new items */
            aList->SetPickedItemStatus( UR_NEW, ii );
            GetScreen()->Append( item );
            break;

        case UR_MOVED:
            item->ClearFlags();
            item->SetFlags( aList->GetPickerFlags( ii ) );
            item->Move( aRedoCommand ? aList->m_TransformPoint : -aList->m_TransformPoint );
            item->ClearFlags();
            break;

        case UR_MIRRORED_Y:
            item->MirrorY( aList->m_TransformPoint.x );
            break;

        case UR_MIRRORED_X:
            item->MirrorX( aList->m_TransformPoint.y );
            break;

        case UR_ROTATED:
            // To undo a rotate 90 deg transform we must rotate 270 deg to undo
            // and 90 deg to redo:
            item->Rotate( aList->m_TransformPoint );

            if( aRedoCommand )
                break;  // A only one rotate transform is OK

            // Make 3 rotate 90 deg transforms is this is actually an undo command
            item->Rotate( aList->m_TransformPoint );
            item->Rotate( aList->m_TransformPoint );
            break;

        case UR_EXCHANGE_T:
            alt_item = (SCH_ITEM*) aList->GetPickedItemLink( ii );
            alt_item->SetNext( NULL );
            alt_item->SetBack( NULL );
            GetScreen()->Remove( item );
            GetScreen()->Append( alt_item );
            aList->SetPickedItem( alt_item, ii );
            aList->SetPickedItemLink( item, ii );
            break;

        default:
            wxFAIL_MSG( wxString::Format( wxT( "Unknown undo/redo command %d" ),
                                          aList->GetPickedItemStatus( ii ) ) );
            break;
        }
    }
}


void SCH_EDIT_FRAME::GetSchematicFromUndoList( wxCommandEvent& event )
{
    if( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    /* Get the old list */
    PICKED_ITEMS_LIST* List = GetScreen()->PopCommandFromUndoList();

    /* Undo the command */
    PutDataInPreviousState( List, false );

    /* Put the old list in RedoList */
    List->ReversePickersListOrder();
    GetScreen()->PushCommandToRedoList( List );

    OnModify();
    SetSheetNumberAndCount();

    GetScreen()->TestDanglingEnds();
    m_canvas->Refresh();
}


void SCH_EDIT_FRAME::GetSchematicFromRedoList( wxCommandEvent& event )
{
    if( GetScreen()->GetRedoCommandCount() == 0 )
        return;

    /* Get the old list */
    PICKED_ITEMS_LIST* List = GetScreen()->PopCommandFromRedoList();

    /* Redo the command: */
    PutDataInPreviousState( List, true );

    /* Put the old list in UndoList */
    List->ReversePickersListOrder();
    GetScreen()->PushCommandToUndoList( List );

    OnModify();
    SetSheetNumberAndCount();

    GetScreen()->TestDanglingEnds();
    m_canvas->Refresh();
}
