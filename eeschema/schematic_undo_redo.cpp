/************************************************************/
/*  eeschema: undo and redo functions for schematic editor  */
/************************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_bus_entry.h"
#include "sch_marker.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_component.h"
#include "sch_polyline.h"
#include "sch_sheet.h"


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
void SwapData( EDA_ITEM* aItem, EDA_ITEM* aImage )
{
    if( aItem == NULL || aImage == NULL )
    {
        wxMessageBox( wxT( "SwapData error: NULL pointer" ) );
        return;
    }

    switch( aItem->Type() )
    {
    case SCH_POLYLINE_T:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_POLYLINE*) aItem )
        #define DEST   ( (SCH_POLYLINE*) aImage )
        break;

    case SCH_JUNCTION_T:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_JUNCTION*) aItem )
        #define DEST   ( (SCH_JUNCTION*) aImage )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        break;

    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_TEXT_T:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_TEXT*) aItem )
        #define DEST   ( (SCH_TEXT*) aImage )
        DEST->SwapData( SOURCE );
        break;

    case SCH_COMPONENT_T:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_COMPONENT*) aItem )
        #define DEST   ( (SCH_COMPONENT*) aImage )
        DEST->SwapData( SOURCE );
        break;

    case SCH_LINE_T:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_LINE*) aItem )
        #define DEST   ( (SCH_LINE*) aImage )
        EXCHG( SOURCE->m_Start, DEST->m_Start );
        EXCHG( SOURCE->m_End, DEST->m_End );
        break;

    case SCH_BUS_ENTRY_T:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_BUS_ENTRY*) aItem )
        #define DEST   ( (SCH_BUS_ENTRY*) aImage )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        EXCHG( SOURCE->m_Size, DEST->m_Size );
        break;

    case SCH_SHEET_T:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_SHEET*) aItem )
        #define DEST   ( (SCH_SHEET*) aImage )
        DEST->SwapData( SOURCE );
        break;

    case SCH_MARKER_T:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_MARKER*) aItem )
        #define DEST   ( (SCH_MARKER*) aImage )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        break;

    case SCH_SHEET_PIN_T:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_SHEET_PIN*) aItem )
        #define DEST   ( (SCH_SHEET_PIN*) aImage )
        DEST->SwapData( SOURCE );
        break;

    case SCH_NO_CONNECT_T:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_NO_CONNECT*) aItem )
        #define DEST   ( (SCH_NO_CONNECT*) aImage )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        break;

    case SCH_FIELD_T:
        break;

    // not directly used in schematic:
    default:
        wxMessageBox( wxT( "SwapData() error: unexpected type" ) );
        break;
    }
}


void SCH_EDIT_FRAME::SaveCopyInUndoList( SCH_ITEM*      aItem,
                                         UndoRedoOpType aCommandType,
                                         const wxPoint& aTransformPoint )
{
    /* Does not save a null item.
     * but if aCommandType == UR_WIRE_IMAGE, we must save null item.
     * It happens for the first wire entered in schematic:
     * To undo this first command, the previous state is a NULL item,
     * and we accept this
     */
    if( aItem == NULL && ( aCommandType != UR_WIRE_IMAGE ) )
        return;

    SCH_ITEM*          CopyOfItem;
    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();
    commandToUndo->m_TransformPoint = aTransformPoint;

    ITEM_PICKER        itemWrapper( aItem, aCommandType );

    if( aItem )
    {
        itemWrapper.m_PickedItemType = aItem->Type();
        itemWrapper.m_PickerFlags    = aItem->GetFlags();
    }

    switch( aCommandType )
    {
    case UR_CHANGED:            /* Create a copy of item */
        CopyOfItem = DuplicateStruct( aItem, true );
        itemWrapper.m_Link = CopyOfItem;
        if( CopyOfItem )
            commandToUndo->PushItem( itemWrapper );
        break;

    case UR_NEW:
    case UR_WIRE_IMAGE:
    case UR_DELETED:
    case UR_MOVED:
        commandToUndo->PushItem( itemWrapper );
        break;

    default:
    {
        wxString msg;
        msg.Printf( wxT( "SaveCopyInUndoList() error (unknown code %X)" ),
                    aCommandType );
        wxMessageBox( msg );
    }
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
        delete commandToUndo;
}


void SCH_EDIT_FRAME::SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                         UndoRedoOpType     aTypeCommand,
                                         const wxPoint&     aTransformPoint )
{
    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();

    commandToUndo->m_TransformPoint = aTransformPoint;

    // Copy picker list:
    commandToUndo->CopyList( aItemsList );

    // Verify list, and creates data if needed
    for( unsigned ii = 0; ii < commandToUndo->GetCount(); ii++ )
    {
        SCH_ITEM*      item = (SCH_ITEM*) commandToUndo->GetPickedItem( ii );
        wxASSERT( item );

        UndoRedoOpType command = commandToUndo->GetPickedItemStatus( ii );
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

        /* Clear redo list, because after new save there is no redo to do */
        GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
    }
    else    // Should not occur
        delete commandToUndo;
}


void SCH_EDIT_FRAME::PutDataInPreviousState( PICKED_ITEMS_LIST* aList, bool aRedoCommand )
{
    SCH_ITEM* item;
    SCH_ITEM* alt_item;

    // Undo in the reverse order of list creation: (this can allow stacked
    // changes like the same item can be changes and deleted in the same
    // complex command
    for( int ii = aList->GetCount() - 1; ii >= 0; ii--  )
    {
        item = (SCH_ITEM*) aList->GetPickedItem( ii );

        if( item )
            item->ClearFlags();

        SCH_ITEM* image = (SCH_ITEM*) aList->GetPickedItemLink( ii );

        switch( aList->GetPickedItemStatus( ii ) )
        {
        case UR_CHANGED: /* Exchange old and new data for each item */
            SwapData( item, image );
            break;

        case UR_NEW:     /* new items are deleted */
            aList->SetPickedItemStatus( UR_DELETED, ii );
            GetScreen()->RemoveFromDrawList( item );
            break;

        case UR_DELETED: /* deleted items are put in EEdrawList, as new items */
            aList->SetPickedItemStatus( UR_NEW, ii );
            item->SetNext( GetScreen()->GetDrawItems() );
            GetScreen()->SetDrawItems( item );
            break;

        case UR_MOVED:
            item->ClearFlags();
            item->SetFlags( aList->GetPickerFlags( ii ) );
            item->Move( aRedoCommand ? aList->m_TransformPoint : -aList->m_TransformPoint );
            item->ClearFlags();
            break;

        case UR_MIRRORED_Y:
        {
            wxPoint mirrorPoint = aList->m_TransformPoint;
            item->Mirror_Y( mirrorPoint.x );
        }
        break;

        case UR_MIRRORED_X:
        {
            wxPoint mirrorPoint = aList->m_TransformPoint;
            item->Mirror_X( mirrorPoint.y );
        }
        break;

        case UR_ROTATED:
        {
            wxPoint RotationPoint = aList->m_TransformPoint;
            item->Rotate( RotationPoint );
            item->Rotate( RotationPoint );
            item->Rotate( RotationPoint );
        }
        break;

        case UR_WIRE_IMAGE:
            /* Exchange the current wires and the old wires */
            alt_item = GetScreen()->ExtractWires( false );
            aList->SetPickedItem( alt_item, ii );

            while( item )
            {
                SCH_ITEM* nextitem = item->Next();
                item->SetNext( GetScreen()->GetDrawItems() );
                GetScreen()->SetDrawItems( item );
                item->ClearFlags();
                item = nextitem;
            }

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
    ReCreateHToolbar();

    GetScreen()->TestDanglingEnds();
    DrawPanel->Refresh();
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
    ReCreateHToolbar();

    GetScreen()->TestDanglingEnds();
    DrawPanel->Refresh();
}
