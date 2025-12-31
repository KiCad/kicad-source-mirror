/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#include <sch_actions.h>
#include <sch_edit_frame.h>
#include <tool/tool_manager.h>
#include <schematic.h>
#include <sch_bus_entry.h>
#include <sch_commit.h>
#include <sch_group.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_sheet_pin.h>
#include <sch_table.h>
#include <tools/sch_selection_tool.h>
#include <drawing_sheet/ds_proxy_undo_item.h>
#include <tool/actions.h>
#include <wx/log.h>


/* Functions to undo and redo edit commands.
 *
 *  m_UndoList and m_RedoList handle a std::vector of PICKED_ITEMS_LIST.  Each PICKED_ITEMS_LIST handles
 *  a std::vector of ITEM_PICKER that store the list of schematic items that are concerned by the command
 *  to undo or redo and is created for each command to undo/redo).  Each picker has a pointer pointing to
 *  an item to undo or redo (in fact: deleted, added or modified), and has a pointer to a copy of this
 *  item, when this item has been modified (the old values of parameters are therefore saved).
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
 *  A problem is the hierarchical sheet handling.  The data associated (sub-hierarchy, undo/redo list) is
 *  deleted only when the sheet is really deleted (i.e. when deleted from undo or redo list).  This is
 *  handled by its destructor.
 */

void SCH_EDIT_FRAME::SaveCopyInUndoList( SCH_SCREEN* aScreen, SCH_ITEM* aItem, UNDO_REDO aCommandType,
                                         bool aAppend )
{
    PICKED_ITEMS_LIST* commandToUndo = nullptr;

    wxCHECK( aItem, /* void */ );

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
        itemWrapper.SetLink( aItem->Duplicate( IGNORE_PARENT_GROUP, nullptr, true ) );
        commandToUndo->PushItem( itemWrapper );
        break;

    case UNDO_REDO::NEWITEM:
    case UNDO_REDO::DELETED:
        commandToUndo->PushItem( itemWrapper );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "SaveCopyInUndoList() error (unknown code %X)" ), aCommandType ) );
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


void SCH_EDIT_FRAME::SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList, UNDO_REDO aTypeCommand,
                                         bool aAppend )
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
    {
        commandToUndo->CopyList( aItemsList );

        for( const std::unique_ptr<SCH_ITEM>& item : GetRepeatItems() )
        {
            EDA_ITEM* repeatItemClone = item->Clone();
            repeatItemClone->SetFlags( UR_TRANSIENT );

            ITEM_PICKER repeatItemPicker( nullptr, repeatItemClone, UNDO_REDO::REPEAT_ITEM );
            commandToUndo->PushItem( repeatItemPicker );
        }
    }
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
                commandToUndo->SetPickedItemLink( sch_item->Duplicate( IGNORE_PARENT_GROUP, nullptr, true ), ii );

            wxASSERT( commandToUndo->GetPickedItemLink( ii ) );
            break;

        case UNDO_REDO::NEWITEM:
        case UNDO_REDO::DELETED:
        case UNDO_REDO::PAGESETTINGS:
        case UNDO_REDO::REPEAT_ITEM:
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
    bool                   updateVariantCtrl = false;
    bool                   dirtyConnectivity = false;
    bool                   rebuildHierarchyNavigator = false;
    bool                   refreshHierarchy = false;
    SCH_CLEANUP_FLAGS      connectivityCleanUp = NO_CLEANUP;
    SCH_SHEET_LIST         sheets= m_schematic->Hierarchy();
    bool                   clearedRepeatItems = false;

    // Undo in the reverse order of list creation: (this can allow stacked changes like the
    // same item can be changed and deleted in the same complex command).
    // After hitting 0, subtracting 1 will roll the value over to its max representation
    for( unsigned ii = aList->GetCount() - 1; ii < std::numeric_limits<unsigned>::max(); ii-- )
    {
        UNDO_REDO      status = aList->GetPickedItemStatus( ii );
        EDA_ITEM*      eda_item = aList->GetPickedItem( ii );
        SCH_SCREEN*    screen = dynamic_cast<SCH_SCREEN*>( aList->GetScreenForItem( ii ) );
        SCH_SHEET_PATH undoSheet = sheets.FindSheetForScreen( screen );

        eda_item->SetFlags( aList->GetPickerFlags( ii ) );
        eda_item->ClearEditFlags();
        eda_item->ClearTempFlags();

        // Set connectable object connectivity status.
        auto propagateConnectivityDamage =
                [&]( SCH_ITEM* schItem )
                {
                    if( schItem->IsConnectable() )
                    {
                        schItem->SetConnectivityDirty();

                        if( schItem->Type() == SCH_SYMBOL_T )
                        {
                            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( schItem );

                            for( SCH_PIN* pin : symbol->GetPins( &undoSheet ) )
                                pin->SetConnectivityDirty();
                        }
                        else if( schItem->Type() == SCH_SHEET_T )
                        {
                            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( schItem );

                            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                                pin->SetConnectivityDirty();
                        }

                        m_highlightedConnChanged = true;
                        dirtyConnectivity = true;

                        // Do a local clean up if there are any connectable objects in the commit
                        if( connectivityCleanUp == NO_CLEANUP )
                            connectivityCleanUp = LOCAL_CLEANUP;

                        // Do a full rebauild of the connectivity if there is a sheet in the commit
                        if( schItem->Type() == SCH_SHEET_T )
                            connectivityCleanUp = GLOBAL_CLEANUP;
                    }
                    else if( schItem->Type() == SCH_RULE_AREA_T )
                    {
                        dirtyConnectivity = true;
                    }
                };

        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( eda_item );

        if( status == UNDO_REDO::NEWITEM )
        {
            if( schItem )
                propagateConnectivityDamage( schItem );

            // If we are removing the current sheet, get out first
            if( eda_item->Type() == SCH_SHEET_T )
            {
                rebuildHierarchyNavigator = true;
                refreshHierarchy = true;

                if( static_cast<SCH_SHEET*>( eda_item )->GetScreen() == GetScreen() )
                    GetToolManager()->PostAction( SCH_ACTIONS::leaveSheet );
            }

            RemoveFromScreen( eda_item, screen );
            aList->SetPickedItemStatus( UNDO_REDO::DELETED, ii );

            bulkRemovedItems.emplace_back( schItem );
        }
        else if( status == UNDO_REDO::DELETED )
        {
            if( eda_item->Type() == SCH_SHEET_T )
            {
                rebuildHierarchyNavigator = true;
                refreshHierarchy = true;
            }

            if( schItem )
                propagateConnectivityDamage( schItem );

            // deleted items are re-inserted on undo
            AddToScreen( eda_item, screen );
            aList->SetPickedItemStatus( UNDO_REDO::NEWITEM, ii );

            bulkAddedItems.emplace_back( schItem );
        }
        else if( status == UNDO_REDO::PAGESETTINGS )
        {
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
        else if( status == UNDO_REDO::REPEAT_ITEM )
        {
            if( !clearedRepeatItems )
            {
                ClearRepeatItemsList();
                clearedRepeatItems = true;
            }

            if( schItem )
            {
                propagateConnectivityDamage( schItem );
                AddCopyForRepeatItem( schItem );

                if( schItem->Type() == SCH_SHEET_T )
                {
                    rebuildHierarchyNavigator = true;
                    refreshHierarchy = true;
                }
            }
        }
        else if( schItem )
        {
            SCH_ITEM* itemCopy = dynamic_cast<SCH_ITEM*>( aList->GetPickedItemLink( ii ) );

            wxCHECK2( itemCopy, continue );

            if( schItem->HasConnectivityChanges( itemCopy, &GetCurrentSheet() ) )
                propagateConnectivityDamage( schItem );

            // The root sheet is a pseudo object that owns the root screen object but is not on
            // the root screen so do not attempt to remove it from the screen it owns.
            if( schItem != &Schematic().Root() )
                RemoveFromScreen( schItem, screen );

            switch( status )
            {
            case UNDO_REDO::CHANGED:
                if( schItem->Type() == SCH_SHEET_T )
                {
                    const SCH_SHEET* origSheet = static_cast<const SCH_SHEET*>( schItem );
                    const SCH_SHEET* copySheet = static_cast<const SCH_SHEET*>( itemCopy );

                    wxCHECK2( origSheet && copySheet, continue );

                    if( origSheet->GetName() != copySheet->GetName()
                            || origSheet->GetFileName() != copySheet->GetFileName()
                            || origSheet->HasPageNumberChanges( *copySheet ) )
                    {
                        rebuildHierarchyNavigator = true;
                    }

                    // Sheet name changes do not require rebuilding the hiearchy.
                    if( origSheet->GetFileName() != copySheet->GetFileName()
                            || origSheet->HasPageNumberChanges( *copySheet ) )
                    {
                        refreshHierarchy = true;
                    }

                    updateVariantCtrl = true;
                }

                if( schItem->Type() == SCH_SYMBOL_T )
                    updateVariantCtrl = true;

                schItem->SwapItemData( itemCopy );

                bulkChangedItems.emplace_back( schItem );

                // Special cases for items which have instance data
                if( schItem->GetParent() && schItem->GetParent()->Type() == SCH_SYMBOL_T
                  && schItem->Type() == SCH_FIELD_T )
                {
                    SCH_FIELD*  field = static_cast<SCH_FIELD*>( schItem );
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( schItem->GetParent() );

                    if( field->GetId() == FIELD_T::REFERENCE )
                    {
                        // Lazy eval of sheet list; this is expensive even when unsorted
                        if( sheets.empty() )
                            sheets = m_schematic->Hierarchy();

                        SCH_SHEET_PATH sheet = sheets.FindSheetForScreen( screen );
                        symbol->SetRef( &sheet, field->GetText() );
                    }

                    bulkChangedItems.emplace_back( symbol );
                }

                break;

            default:
                wxFAIL_MSG( wxString::Format( wxT( "Unknown undo/redo command %d" ), status ) );
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

    // We have now swapped all the group parent and group member pointers.  But it is a
    // risky proposition to bet on the pointers being invariant, so validate them all.
    for( int ii = 0; ii < (int) aList->GetCount(); ++ii )
    {
        ITEM_PICKER& wrapper = aList->GetItemWrapper( ii );

        if( wrapper.GetStatus() == UNDO_REDO::DELETED )
            continue;

        SCH_ITEM* parentGroup = Schematic().ResolveItem( wrapper.GetGroupId(), nullptr, true );
        wrapper.GetItem()->SetParentGroup( dynamic_cast<SCH_GROUP*>( parentGroup ) );

        if( EDA_GROUP* group = dynamic_cast<SCH_GROUP*>( wrapper.GetItem() ) )
        {
            // Items list may contain dodgy pointers, so don't use RemoveAll()
            group->GetItems().clear();

            for( const KIID& member : wrapper.GetGroupMembers() )
            {
                if( SCH_ITEM* memberItem = Schematic().ResolveItem( member, nullptr, true ) )
                    group->AddItem( memberItem );
            }
        }

        // And prepare for a redo by updating group info based on current image
        if( EDA_ITEM* item = wrapper.GetLink() )
            wrapper.SetLink( item );
    }

    GetCanvas()->GetView()->ClearHiddenFlags();

    // Notify our listeners
    if( bulkAddedItems.size() > 0 )
        Schematic().OnItemsAdded( bulkAddedItems );

    if( bulkRemovedItems.size() > 0 )
        Schematic().OnItemsRemoved( bulkRemovedItems );

    if( bulkChangedItems.size() > 0 )
        Schematic().OnItemsChanged( bulkChangedItems );

    if( refreshHierarchy )
        Schematic().RefreshHierarchy();

    if( dirtyConnectivity )
    {
        wxLogTrace( wxS( "CONN_PROFILE" ), wxS( "Undo/redo %s clean up connectivity rebuild." ),
                    connectivityCleanUp == LOCAL_CLEANUP ? wxS( "local" ) : wxS( "global" ) );

        SCH_COMMIT localCommit( m_toolManager );

        RecalculateConnections( &localCommit, connectivityCleanUp );

        if( connectivityCleanUp == GLOBAL_CLEANUP )
            SetSheetNumberAndCount();

        // Restore hop over shapes of wires, if any
        if( m_schematic->Settings().m_HopOverScale > 0.0 )
        {
            for( SCH_ITEM* item : GetScreen()->Items() )
            {
                if( item->Type() != SCH_LINE_T )
                    continue;

                SCH_LINE* line = static_cast<SCH_LINE*>( item );

                if( line->IsWire() )
                    UpdateHopOveredWires( line );
            }
        }
    }

    // Update the hierarchy navigator when there are sheet changes.
    if( rebuildHierarchyNavigator )
        UpdateHierarchyNavigator();

    if( updateVariantCtrl && m_schematic )
    {
        m_schematic->LoadVariants();
        UpdateVariantSelectionCtrl( m_schematic->GetVariantNamesForUI() );
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

        m_toolManager->GetTool<SCH_SELECTION_TOOL>()->RebuildSelection();
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


