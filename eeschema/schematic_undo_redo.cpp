/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2023 KiCad Developers, see change_log.txt for contributors.
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

#include <ee_actions.h>
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

void SCH_EDIT_FRAME::SaveCopyInUndoList( SCH_SCREEN* aScreen, SCH_ITEM* aItem,
                                         UNDO_REDO aCommandType, bool aAppend,
                                         bool aDirtyConnectivity )
{
    PICKED_ITEMS_LIST* commandToUndo = nullptr;

    wxCHECK( aItem, /* void */ );

    if( aDirtyConnectivity )
    {
        if( !aItem->IsConnectivityDirty() && aItem->Connection()
          && ( aItem->Connection()->Name() == m_highlightedConn ) )
            m_highlightedConnChanged = true;

        aItem->SetConnectivityDirty();
    }

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
                                         UNDO_REDO aTypeCommand, bool aAppend,
                                         bool aDirtyConnectivity )
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
    {
        commandToUndo = new PICKED_ITEMS_LIST();
        commandToUndo->SetDescription( aItemsList.GetDescription() );
    }

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
    // After hitting 0, subtracting 1 will roll the value over to its max representation
    for( unsigned ii = aList->GetCount() - 1; ii < std::numeric_limits<unsigned>::max(); ii-- )
    {
        UNDO_REDO   status = aList->GetPickedItemStatus( ii );
        EDA_ITEM*   eda_item = aList->GetPickedItem( ii );
        SCH_SCREEN* screen = dynamic_cast<SCH_SCREEN*>( aList->GetScreenForItem( ii ) );

        wxCHECK( screen, /* void */ );

        eda_item->SetFlags( aList->GetPickerFlags( ii ) );
        eda_item->ClearEditFlags();
        eda_item->ClearTempFlags();

        if( status == UNDO_REDO::NOP )
        {
            continue;
        }

        if( status == UNDO_REDO::NEWITEM )
        {
            // If we are removing the current sheet, get out first
            if( eda_item->Type() == SCH_SHEET_T )
            {
                if( static_cast<SCH_SHEET*>( eda_item )->GetScreen() == GetScreen() )
                    GetToolManager()->PostAction( EE_ACTIONS::leaveSheet );
            }

            SCH_ITEM* schItem = static_cast<SCH_ITEM*>( eda_item );

            if( schItem && schItem->IsConnectable() )
                m_highlightedConnChanged = true;

            RemoveFromScreen( eda_item, screen );
            aList->SetPickedItemStatus( UNDO_REDO::DELETED, ii );
        }
        else if( status == UNDO_REDO::DELETED )
        {
            SCH_ITEM* schItem = static_cast<SCH_ITEM*>( eda_item );

            if( schItem && schItem->IsConnectable() )
                m_highlightedConnChanged = true;

            // deleted items are re-inserted on undo
            AddToScreen( eda_item, screen );
            aList->SetPickedItemStatus( UNDO_REDO::NEWITEM, ii );
        }
        else if( status == UNDO_REDO::PAGESETTINGS )
        {
            SCH_SHEET_PATH& undoSheet = *m_schematic->GetSheets().FindSheetForScreen( screen );

            if( GetCurrentSheet() != undoSheet )
            {
                SetCurrentSheet( undoSheet );
                DisplayCurrentSheet();
            }

            // swap current settings with stored settings
            DS_PROXY_UNDO_ITEM  alt_item( this );
            DS_PROXY_UNDO_ITEM* item = static_cast<DS_PROXY_UNDO_ITEM*>( eda_item );
            item->Restore( this );
            *item = alt_item;
        }
        else if( SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( eda_item ) )
        {
            if( item->IsConnectable() )
                m_highlightedConnChanged = true;

            // everything else is modified in place
            SCH_ITEM* alt_item = static_cast<SCH_ITEM*>( aList->GetPickedItemLink( ii ) );

            // The root sheet is a pseudo object that owns the root screen object but is not on
            // the root screen so do not attempt to remove it from the screen it owns.
            if( item != &Schematic().Root() )
                RemoveFromScreen( item, screen );

            switch( status )
            {
            case UNDO_REDO::CHANGED:
                item->SwapData( alt_item );

                // Special cases for items which have instance data
                if( item->GetParent() && item->GetParent()->Type() == SCH_SYMBOL_T
                        && item->Type() == SCH_FIELD_T )
                {
                    SCH_FIELD*  field = static_cast<SCH_FIELD*>( item );
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item->GetParent() );

                    if( field->GetId() == REFERENCE_FIELD )
                    {
                        symbol->SetRef( m_schematic->GetSheets().FindSheetForScreen( screen ),
                                        field->GetText() );
                    }
                }

                break;

            case UNDO_REDO::EXCHANGE_T:
                aList->SetPickedItem( alt_item, ii );
                aList->SetPickedItemLink( item, ii );
                item = alt_item;
                break;

            default:
                wxFAIL_MSG( wxString::Format( wxT( "Unknown undo/redo command %d" ),
                                              aList->GetPickedItemStatus( ii ) ) );
                break;
            }

            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
                sym->UpdatePins();
            }

            if( item != &Schematic().Root() )
                AddToScreen( item, screen );
        }
    }

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
        undo->ClearListAndDeleteItems( []( EDA_ITEM* aItem )
                                       {
                                           delete aItem;
                                       } );
        delete undo;

        SetSheetNumberAndCount();
        UpdateHierarchyNavigator();

        TestDanglingEnds();

        m_toolManager->GetTool<EE_SELECTION_TOOL>()->RebuildSelection();
    }

    GetCanvas()->Refresh();
}


void SCH_EDIT_FRAME::ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    UNDO_REDO_CONTAINER& list = whichList == UNDO_LIST ? m_undoList : m_redoList;

    for( PICKED_ITEMS_LIST* command : list.m_CommandsList )
    {
        command->ClearListAndDeleteItems( []( EDA_ITEM* aItem )
                                          {
                                              delete aItem;
                                          } );
        delete command;
    }

    list.m_CommandsList.clear();
}


