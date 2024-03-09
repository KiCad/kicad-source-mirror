/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2024 KiCad Developers, see change_log.txt for contributors.
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
#include <sch_commit.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_bitmap.h>
#include <sch_sheet_pin.h>
#include <sch_table.h>
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
        if( !aItem->IsConnectivityDirty()
                && aItem->Connection()
                && ( aItem->Connection()->Name() == m_highlightedConn ) )
        {
            m_highlightedConnChanged = true;
        }

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
    std::vector<SCH_ITEM*> bulkAddedItems;
    std::vector<SCH_ITEM*> bulkRemovedItems;
    std::vector<SCH_ITEM*> bulkChangedItems;
    std::set<SCH_TABLE*>   changedTables;
    bool                   dirtyConnectivity = false;
    SCH_CLEANUP_FLAGS      connectivityCleanUp = NO_CLEANUP;

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

        SCH_ITEM*   schItem = dynamic_cast<SCH_ITEM*>( eda_item );

        // Set connectable object connectivity status.
        auto updateConnectivityFlag = [&, this]()
        {
            if( schItem && schItem->IsConnectable() )
            {
                schItem->SetConnectivityDirty();

                if( schItem->Type() == SCH_SYMBOL_T )
                {
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( schItem );

                    wxCHECK( symbol, /* void */ );

                    for( SCH_PIN* pin : symbol->GetPins() )
                        pin->SetConnectivityDirty();
                }
                else if( schItem->Type() == SCH_SHEET_T )
                {
                    SCH_SHEET* sheet = static_cast<SCH_SHEET*>( schItem );

                    wxCHECK( sheet, /* void */ );

                    for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                        pin->SetConnectivityDirty();
                }

                m_highlightedConnChanged = true;
                dirtyConnectivity = true;

                // Do a local clean up if there are any connectable objects in the commit.
                if( connectivityCleanUp == NO_CLEANUP )
                    connectivityCleanUp = LOCAL_CLEANUP;

                // Do a full rebauild of the connectivity if there is a sheet in the commit.
                if( schItem->Type() == SCH_SHEET_T )
                    connectivityCleanUp = GLOBAL_CLEANUP;
            }
        };

        if( status == UNDO_REDO::NEWITEM )
        {
            updateConnectivityFlag();

            // If we are removing the current sheet, get out first
            if( eda_item->Type() == SCH_SHEET_T )
            {
                if( static_cast<SCH_SHEET*>( eda_item )->GetScreen() == GetScreen() )
                    GetToolManager()->PostAction( EE_ACTIONS::leaveSheet );
            }

            RemoveFromScreen( eda_item, screen );
            aList->SetPickedItemStatus( UNDO_REDO::DELETED, ii );

            bulkRemovedItems.emplace_back( schItem );
        }
        else if( status == UNDO_REDO::DELETED )
        {
            updateConnectivityFlag();

            // deleted items are re-inserted on undo
            AddToScreen( eda_item, screen );
            aList->SetPickedItemStatus( UNDO_REDO::NEWITEM, ii );

            bulkAddedItems.emplace_back( schItem );
        }
        else if( status == UNDO_REDO::PAGESETTINGS )
        {
            SCH_SHEET_PATH undoSheet = m_schematic->GetSheets().FindSheetForScreen( screen );

            if( GetCurrentSheet() != undoSheet )
            {
                SetCurrentSheet( undoSheet );
                DisplayCurrentSheet();
            }

            // swap current settings with stored settings
            DS_PROXY_UNDO_ITEM  alt_item( this );
            DS_PROXY_UNDO_ITEM* item = static_cast<DS_PROXY_UNDO_ITEM*>( eda_item );
            item->Restore( this );
            *item = std::move( alt_item );
        }
        else if( schItem )
        {
            SCH_ITEM* itemCopy = dynamic_cast<SCH_ITEM*>( aList->GetPickedItemLink( ii ) );

            wxCHECK2( itemCopy, continue );

            if( schItem->HasConnectivityChanges( itemCopy, &GetCurrentSheet() ) )
                updateConnectivityFlag();

            // The root sheet is a pseudo object that owns the root screen object but is not on
            // the root screen so do not attempt to remove it from the screen it owns.
            if( schItem != &Schematic().Root() )
                RemoveFromScreen( schItem, screen );

            switch( status )
            {
            case UNDO_REDO::CHANGED:
                schItem->SwapData( itemCopy );
                bulkChangedItems.emplace_back( schItem );

                // Special cases for items which have instance data
                if( schItem->GetParent() && schItem->GetParent()->Type() == SCH_SYMBOL_T
                  && schItem->Type() == SCH_FIELD_T )
                {
                    SCH_FIELD*  field = static_cast<SCH_FIELD*>( schItem );
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( schItem->GetParent() );

                    if( field->GetId() == REFERENCE_FIELD )
                    {
                        SCH_SHEET_PATH sheet = m_schematic->GetSheets().FindSheetForScreen( screen );
                        symbol->SetRef( &sheet, field->GetText() );
                    }

                    bulkChangedItems.emplace_back( symbol );
                }

                break;

            default:
                wxFAIL_MSG( wxString::Format( wxT( "Unknown undo/redo command %d" ),
                                              aList->GetPickedItemStatus( ii ) ) );
                break;
            }

            if( schItem->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( schItem );
                sym->UpdatePins();
            }

            if( schItem != &Schematic().Root() )
                AddToScreen( schItem, screen );
        }
    }

    GetCanvas()->GetView()->ClearHiddenFlags();

    // Notify our listeners
    if( bulkAddedItems.size() > 0 )
        Schematic().OnItemsAdded( bulkAddedItems );

    if( bulkRemovedItems.size() > 0 )
        Schematic().OnItemsRemoved( bulkRemovedItems );

    if( bulkChangedItems.size() > 0 )
        Schematic().OnItemsChanged( bulkChangedItems );

    if( dirtyConnectivity )
    {
        wxLogTrace( wxS( "CONN_PROFILE" ),
                    wxS( "Undo/redo %s clean up connectivity rebuild." ),
                    ( connectivityCleanUp == LOCAL_CLEANUP ) ? wxS( "local" ) : wxS( "global" ) );

        SCH_COMMIT localCommit( m_toolManager );

        RecalculateConnections( &localCommit, connectivityCleanUp );

        // Update the hierarchy navigator when there are sheet changes.
        if( connectivityCleanUp == GLOBAL_CLEANUP )
        {
            SetSheetNumberAndCount();
            UpdateHierarchyNavigator();
        }
    }
}


void SCH_EDIT_FRAME::RollbackSchematicFromUndo()
{
    PICKED_ITEMS_LIST* undo = PopCommandFromUndoList();

    // Skip empty frames
    while( undo && !undo->GetCount() )
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

        m_toolManager->GetTool<EE_SELECTION_TOOL>()->RebuildSelection();
    }

    GetCanvas()->Refresh();
}


void SCH_EDIT_FRAME::ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    UNDO_REDO_CONTAINER& list = ( whichList == UNDO_LIST ) ? m_undoList : m_redoList;

    if( aItemCount < 0 )
    {
        list.ClearCommandList();
    }
    else
    {
        for( int ii = 0; ii < aItemCount; ii++ )
        {
            if( list.m_CommandsList.size() == 0 )
                break;

            PICKED_ITEMS_LIST* curr_cmd = list.m_CommandsList[0];
            list.m_CommandsList.erase( list.m_CommandsList.begin() );

            curr_cmd->ClearListAndDeleteItems( []( EDA_ITEM* aItem )
                                               {
                                                   delete aItem;
                                               } );
            delete curr_cmd;    // Delete command
        }
    }
}


