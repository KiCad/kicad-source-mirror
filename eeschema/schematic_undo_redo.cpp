/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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

#include <sch_edit_frame.h>
#include <tool/tool_manager.h>
#include <schematic.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_bitmap.h>
#include <tools/ee_selection_tool.h>
#include <drawing_sheet/ds_proxy_undo_item.h>
#include <tool/actions.h>


/* Functions to undo and redo edit commands.
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
 *      => deleted items are moved into m_tree
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
 * swap data between Item and its copy, pointed by its picked item link member
 * swapped data is data modified by editing, so not all values are swapped
 */

void SCH_EDIT_FRAME::StartNewUndo()
{
    PICKED_ITEMS_LIST* blank = new PICKED_ITEMS_LIST();
    PushCommandToUndoList( blank );
}


void SCH_EDIT_FRAME::SaveCopyInUndoList( SCH_SCREEN*    aScreen,
                                         SCH_ITEM*      aItem,
                                         UNDO_REDO      aCommandType,
                                         bool           aAppend )
{
    PICKED_ITEMS_LIST* commandToUndo = nullptr;

    wxCHECK( aItem, /* void */ );

    // Connectivity may change
    aItem->SetConnectivityDirty();

    PICKED_ITEMS_LIST* lastUndo = PopCommandFromUndoList();

    // If the last stack was empty, use that one instead of creating a new stack
    if( lastUndo )
    {
        if( aAppend || !lastUndo->GetCount() )
            commandToUndo = lastUndo;
        else
            PushCommandToUndoList( lastUndo );
    }

    if( !commandToUndo )
    {
        commandToUndo = new PICKED_ITEMS_LIST();
    }

    ITEM_PICKER itemWrapper( aScreen, aItem, aCommandType );
    itemWrapper.SetFlags( aItem->GetFlags() );

    switch( aCommandType )
    {
    case UNDO_REDO::CHANGED: /* Create a copy of item */
        itemWrapper.SetLink( aItem->Duplicate( true ) );
        commandToUndo->PushItem( itemWrapper );
        break;

    case UNDO_REDO::NEWITEM:
    case UNDO_REDO::DELETED:
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
        PushCommandToUndoList( commandToUndo );

        /* Clear redo list, because after new save there is no redo to do */
        ClearUndoORRedoList( REDO_LIST );
    }
    else
    {
        delete commandToUndo;
    }
}


void SCH_EDIT_FRAME::SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
                                         UNDO_REDO                aTypeCommand,
                                         bool                     aAppend )
{
    PICKED_ITEMS_LIST* commandToUndo = nullptr;

    if( !aItemsList.GetCount() )
        return;

    PICKED_ITEMS_LIST* lastUndo = PopCommandFromUndoList();

    // If the last stack was empty, use that one instead of creating a new stack
    if( lastUndo )
    {
        if( aAppend || !lastUndo->GetCount() )
            commandToUndo = lastUndo;
        else
            PushCommandToUndoList( lastUndo );
    }

    if( !commandToUndo )
        commandToUndo = new PICKED_ITEMS_LIST();

    // Copy picker list:
    if( !commandToUndo->GetCount() )
        commandToUndo->CopyList( aItemsList );
    else
    {
        // Unless we are appending, in which case, get the picker items
        for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
            commandToUndo->PushItem( aItemsList.GetItemWrapper( ii) );
    }

    // Verify list, and creates data if needed
    for( unsigned ii = 0; ii < commandToUndo->GetCount(); ii++ )
    {
        SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( commandToUndo->GetPickedItem( ii ) );

        // Common items implemented in EDA_DRAW_FRAME will not be SCH_ITEMs.
        if( !sch_item )
            continue;

        // Connectivity may change
        sch_item->SetConnectivityDirty();

        UNDO_REDO command = commandToUndo->GetPickedItemStatus( ii );

        if( command == UNDO_REDO::UNSPECIFIED )
        {
            command = aTypeCommand;
            commandToUndo->SetPickedItemStatus( command, ii );
        }

        switch( command )
        {
        case UNDO_REDO::CHANGED:

            /* If needed, create a copy of item, and put in undo list
             * in the picker, as link
             * If this link is not null, the copy is already done
             */
            if( commandToUndo->GetPickedItemLink( ii ) == nullptr )
                commandToUndo->SetPickedItemLink( sch_item->Duplicate( true ), ii );

            wxASSERT( commandToUndo->GetPickedItemLink( ii ) );
            break;

        case UNDO_REDO::NEWITEM:
        case UNDO_REDO::DELETED:
        case UNDO_REDO::EXCHANGE_T:
        case UNDO_REDO::PAGESETTINGS:
            break;

        default:
            wxFAIL_MSG( wxString::Format( wxT( "Unknown undo/redo command %d" ), command ) );
            break;
        }
    }

    if( commandToUndo->GetCount() )
    {
        /* Save the copy in undo list */
        PushCommandToUndoList( commandToUndo );

        /* Clear redo list, because after new save there is no redo to do */
        ClearUndoORRedoList( REDO_LIST );
    }
    else    // Should not occur
    {
        delete commandToUndo;
    }
}


void SCH_EDIT_FRAME::PutDataInPreviousState( PICKED_ITEMS_LIST* aList )
{
    // Undo in the reverse order of list creation: (this can allow stacked changes like the
    // same item can be changed and deleted in the same complex command).
    for( int ii = aList->GetCount() - 1; ii >= 0; ii-- )
    {
        UNDO_REDO status = aList->GetPickedItemStatus((unsigned) ii );
        EDA_ITEM*   eda_item = aList->GetPickedItem( (unsigned) ii );
        SCH_SCREEN* screen =
                dynamic_cast< SCH_SCREEN* >( aList->GetScreenForItem( (unsigned) ii ) );

        wxCHECK( screen, /* void */ );

        eda_item->SetFlags( aList->GetPickerFlags( (unsigned) ii ) );
        eda_item->ClearEditFlags();
        eda_item->ClearTempFlags();

        if( status == UNDO_REDO::NOP )
        {
            continue;
        }
        if( status == UNDO_REDO::NEWITEM )
        {
            // new items are deleted on undo
            RemoveFromScreen( eda_item, screen );
            aList->SetPickedItemStatus( UNDO_REDO::DELETED, (unsigned) ii );
        }
        else if( status == UNDO_REDO::DELETED )
        {
            // deleted items are re-inserted on undo
            AddToScreen( eda_item, screen );
            aList->SetPickedItemStatus( UNDO_REDO::NEWITEM, (unsigned) ii );
        }
        else if( status == UNDO_REDO::PAGESETTINGS )
        {
            // swap current settings with stored settings
            DS_PROXY_UNDO_ITEM  alt_item( this );
            DS_PROXY_UNDO_ITEM* item = static_cast<DS_PROXY_UNDO_ITEM*>( eda_item );
            item->Restore( this );
            *item = alt_item;
            GetToolManager()->RunAction( ACTIONS::zoomFitScreen, true );
        }
        else if( dynamic_cast<SCH_ITEM*>( eda_item ) )
        {
            // everything else is modified in place
            SCH_ITEM* item = (SCH_ITEM*) eda_item;
            SCH_ITEM* alt_item = (SCH_ITEM*) aList->GetPickedItemLink( (unsigned) ii );

            // The root sheet is a pseudo object that owns the root screen object but is not on
            // the root screen so do not attempt to remove it from the screen it owns.
            if( item != &Schematic().Root() )
                RemoveFromScreen( item, screen );

            switch( status )
            {
            case UNDO_REDO::CHANGED:
                if( item->Type() == SCH_SYMBOL_T )
                {
                    // Update the schematic library cache in case that was the change.
                    SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item );
                    SCH_SYMBOL* altSymbol = dynamic_cast<SCH_SYMBOL*>( alt_item );

                    wxCHECK( symbol && altSymbol, /* void */ );

                    screen->SwapSymbolLinks( symbol, altSymbol );
                }

                item->SwapData( alt_item );

                if( item->Type() == SCH_SYMBOL_T )
                    static_cast<SCH_SYMBOL*>( item )->UpdatePins();

                break;

            case UNDO_REDO::EXCHANGE_T:
                aList->SetPickedItem( alt_item, (unsigned) ii );
                aList->SetPickedItemLink( item, (unsigned) ii );
                item = alt_item;
                break;

            default:
                wxFAIL_MSG( wxString::Format( wxT( "Unknown undo/redo command %d" ),
                                              aList->GetPickedItemStatus( (unsigned) ii ) ) );
                break;
            }

            if( item != &Schematic().Root() )
                AddToScreen( item, screen );
        }
    }

    // Bitmaps are cached in Opengl: clear the cache, because
    // the cache data can be invalid
    GetCanvas()->GetView()->RecacheAllItems();
    GetCanvas()->GetView()->ClearHiddenFlags();
}


void SCH_EDIT_FRAME::RollbackSchematicFromUndo()
{
    PICKED_ITEMS_LIST* undo = PopCommandFromUndoList();

    // Skip empty frames
    while( undo && ( !undo->GetCount()
            || ( undo->GetCount() == 1 && undo->GetPickedItemStatus( 0 ) == UNDO_REDO::NOP ) ) )
    {
        delete undo;
        undo = PopCommandFromUndoList();
    }

    if( undo )
    {
        PutDataInPreviousState( undo );
        undo->ClearListAndDeleteItems();
        delete undo;

        SetSheetNumberAndCount();
        UpdateHierarchyNavigator();

        TestDanglingEnds();

        m_toolManager->GetTool<EE_SELECTION_TOOL>()->RebuildSelection();
    }

    SyncView();
    GetCanvas()->Refresh();
}


void SCH_EDIT_FRAME::ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    UNDO_REDO_CONTAINER& list = whichList == UNDO_LIST ? m_undoList : m_redoList;

    for( PICKED_ITEMS_LIST* command : list.m_CommandsList )
    {
        command->ClearListAndDeleteItems();
        delete command;
    }

    list.m_CommandsList.clear();
}


