/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <tool/tool_manager.h>
#include <general.h>
#include <sch_bus_entry.h>
#include <sch_marker.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_bitmap.h>
#include <sch_view.h>
#include <tools/ee_selection_tool.h>
#include <ws_proxy_undo_item.h>
#include <tool/actions.h>

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
 * swap data between Item and its copy, pointed by its picked item link member
 * swapped data is data modified by editing, so not all values are swapped
 */

void SCH_EDIT_FRAME::SaveCopyInUndoList( SCH_ITEM*      aItem,
                                         UNDO_REDO_T    aCommandType,
                                         bool           aAppend,
                                         const wxPoint& aTransformPoint )
{
    PICKED_ITEMS_LIST* commandToUndo = nullptr;

    if( !aItem )
        return;

    // Connectivity may change
    aItem->SetConnectivityDirty();

    if( aAppend )
        commandToUndo = GetScreen()->PopCommandFromUndoList();

    if( !commandToUndo )
    {
        commandToUndo = new PICKED_ITEMS_LIST();
        commandToUndo->m_TransformPoint = aTransformPoint;
    }

    ITEM_PICKER itemWrapper( aItem, aCommandType );
    itemWrapper.SetFlags( aItem->GetFlags() );

    switch( aCommandType )
    {
    case UR_CHANGED:            /* Create a copy of item */
        itemWrapper.SetLink( aItem->Duplicate( true ) );
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
                                         UNDO_REDO_T              aTypeCommand,
                                         bool                     aAppend,
                                         const wxPoint&           aTransformPoint )
{
    PICKED_ITEMS_LIST* commandToUndo = nullptr;

    if( !aItemsList.GetCount() )
        return;

    // Can't append a WIRE IMAGE, so fail to a new undo point
    if( aAppend )
        commandToUndo = GetScreen()->PopCommandFromUndoList();

    if( !commandToUndo )
    {
        commandToUndo = new PICKED_ITEMS_LIST();
        commandToUndo->m_TransformPoint = aTransformPoint;
        commandToUndo->m_Status = aTypeCommand;
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

        // Connectivity may change
        sch_item->SetConnectivityDirty();

        UNDO_REDO_T command = commandToUndo->GetPickedItemStatus( ii );

        if( command == UR_UNSPECIFIED )
        {
            command = aTypeCommand;
            commandToUndo->SetPickedItemStatus( command, ii );
        }

        switch( command )
        {
        case UR_CHANGED:

            /* If needed, create a copy of item, and put in undo list
             * in the picker, as link
             * If this link is not null, the copy is already done
             */
            if( commandToUndo->GetPickedItemLink( ii ) == nullptr )
                commandToUndo->SetPickedItemLink( sch_item->Duplicate( true ), ii );

            wxASSERT( commandToUndo->GetPickedItemLink( ii ) );
            break;

        case UR_MOVED:
        case UR_MIRRORED_Y:
        case UR_MIRRORED_X:
        case UR_ROTATED:
        case UR_NEW:
        case UR_DELETED:
        case UR_EXCHANGE_T:
        case UR_PAGESETTINGS:
            break;

        default:
            wxFAIL_MSG( wxString::Format( wxT( "Unknown undo/redo command %d" ), command ) );
            break;
        }
    }

    if( commandToUndo->GetCount() )
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
    // Undo in the reverse order of list creation: (this can allow stacked changes like the
    // same item can be changed and deleted in the same complex command).
    for( int ii = aList->GetCount() - 1; ii >= 0; ii-- )
    {
        UNDO_REDO_T status = aList->GetPickedItemStatus((unsigned) ii );
        EDA_ITEM*   eda_item = aList->GetPickedItem( (unsigned) ii );

        eda_item->SetFlags( aList->GetPickerFlags( (unsigned) ii ) );
        eda_item->ClearEditFlags();
        eda_item->ClearTempFlags();

        if( status == UR_NEW )
        {
            // new items are deleted on undo
            RemoveFromScreen( eda_item );
            aList->SetPickedItemStatus( UR_DELETED, (unsigned) ii );
        }
        else if( status == UR_DELETED )
        {
            // deleted items are re-inserted on undo
            AddToScreen( eda_item );
            aList->SetPickedItemStatus( UR_NEW, (unsigned) ii );
        }
        else if( status == UR_PAGESETTINGS )
        {
            // swap current settings with stored settings
            WS_PROXY_UNDO_ITEM  alt_item( this );
            WS_PROXY_UNDO_ITEM* item = (WS_PROXY_UNDO_ITEM*) eda_item;
            item->Restore( this );
            *item = alt_item;
            GetToolManager()->RunAction( ACTIONS::zoomFitScreen, true );
        }
        else if( dynamic_cast<SCH_ITEM*>( eda_item ) )
        {
            // everthing else is modified in place

            SCH_ITEM* item = (SCH_ITEM*) eda_item;
            SCH_ITEM* alt_item = (SCH_ITEM*) aList->GetPickedItemLink( (unsigned) ii );
            RemoveFromScreen( item );

            switch( status )
            {
            case UR_CHANGED:
                item->SwapData( alt_item );
                break;

            case UR_MOVED:
                item->Move( aRedoCommand ? aList->m_TransformPoint : -aList->m_TransformPoint );
                break;

            case UR_MIRRORED_Y:
                item->MirrorY( aList->m_TransformPoint.x );
                break;

            case UR_MIRRORED_X:
                item->MirrorX( aList->m_TransformPoint.y );
                break;

            case UR_ROTATED:
                if( aRedoCommand )
                    item->Rotate( aList->m_TransformPoint );
                else
                {
                    // Rotate 270 deg to undo 90-deg rotate
                    item->Rotate( aList->m_TransformPoint );
                    item->Rotate( aList->m_TransformPoint );
                    item->Rotate( aList->m_TransformPoint );
                }
                break;

            case UR_EXCHANGE_T:
                alt_item->SetNext( nullptr );
                alt_item->SetBack( nullptr );
                aList->SetPickedItem( alt_item, (unsigned) ii );
                aList->SetPickedItemLink( item, (unsigned) ii );
                item = alt_item;
                break;

            default:
                wxFAIL_MSG( wxString::Format( wxT( "Unknown undo/redo command %d" ),
                                              aList->GetPickedItemStatus( (unsigned) ii ) ) );
                break;
            }

            AddToScreen( item );
        }
    }

    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    selTool->RebuildSelection();

    // Bitmaps are cached in Opengl: clear the cache, because
    // the cache data can be invalid
    GetCanvas()->GetView()->RecacheAllItems();
    GetCanvas()->GetView()->ClearHiddenFlags();
}


void SCH_EDIT_FRAME::RollbackSchematicFromUndo()
{
    PICKED_ITEMS_LIST* undo = GetScreen()->PopCommandFromUndoList();

    if( undo )
    {
        PutDataInPreviousState( undo, false );
        undo->ClearListAndDeleteItems();
        delete undo;

        SetSheetNumberAndCount();

        TestDanglingEnds();
    }

    SyncView();
    GetCanvas()->Refresh();
}
