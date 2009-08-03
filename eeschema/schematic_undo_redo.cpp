/************************************************************/
/*  eeschema: undo and redo functions for schematic editor  */
/************************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"
#include "class_marker_sch.h"


/* Functions to undo and redo edit commands.
 *  commmands to undo are stored in CurrentScreen->m_UndoList
 *  commmands to redo are stored in CurrentScreen->m_RedoList
 *
 *  m_UndoList and m_RedoList handle a std::vector of PICKED_ITEMS_LIST
 *  Each PICKED_ITEMS_LIST handle a std::vector of pickers (class ITEM_PICKER),
 *  that store the list of schematic items that are concerned by the command to undo or redo
 *  and is created for each command to undo (handle also a command to redo).
 *  each picker has a pointer pointing to an item to undo or redo (in fact: deleted, added or modified),
 * and has a pointer to a copy of this item, when this item has been modified
 * (the old values of parameters are therefore saved)
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
 *      => The list of item(s) is used to create a deleted list in undo list(same as a delete command)
 *
 *   Some block operations that change items can be undoed without memorise items, just the coordiantes of the transform:
 *      move list of items (undo/redo is made by moving with the opposite move vector)
 *      mirror (Y) and flip list of items (undo/redo is made by mirror or flip items)
 *      so they are handled specifically.
 *
 *  A problem is the hierarchical sheet handling.
 *  the data associated (subhierarchy, uno/redo list) is deleted only
 *  when the sheet is really deleted (i.e. when deleted from undo or redo list)
 *  This is handled by its destructor.
 */


/**************************************************************/
void SwapData( EDA_BaseStruct* aItem, EDA_BaseStruct* aImage )
/***************************************************************/

/* Used if undo / redo command:
 *  swap data between Item and its copy, pointed by its .m_Image member
 * swapped data is data modified by edition, so not all values are swapped
 */
{
    if( aItem == NULL || aImage == NULL )
    {
        wxMessageBox( wxT( "SwapData error: NULL pointer" ) );
        return;
    }

    switch( aItem->Type() )
    {
    case DRAW_POLYLINE_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawPolylineStruct*) aItem )
        #define DEST   ( (DrawPolylineStruct*) aImage )
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawJunctionStruct*) aItem )
        #define DEST   ( (DrawJunctionStruct*) aImage )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        break;

    case TYPE_SCH_LABEL:
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_TEXT:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_TEXT*) aItem )
        #define DEST   ( (SCH_TEXT*) aImage )
        DEST->SwapData( SOURCE );
        break;

    case TYPE_SCH_COMPONENT:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (SCH_COMPONENT*) aItem )
        #define DEST   ( (SCH_COMPONENT*) aImage )
        DEST->SwapData( SOURCE );
        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (EDA_DrawLineStruct*) aItem )
        #define DEST   ( (EDA_DrawLineStruct*) aImage )
        EXCHG( SOURCE->m_Start, DEST->m_Start );
        EXCHG( SOURCE->m_End, DEST->m_End );
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawBusEntryStruct*) aItem )
        #define DEST   ( (DrawBusEntryStruct*) aImage )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        EXCHG( SOURCE->m_Size, DEST->m_Size );
        break;

    case DRAW_SHEET_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawSheetStruct*) aItem )
        #define DEST   ( (DrawSheetStruct*) aImage )
        DEST->SwapData( SOURCE );
        break;

    case TYPE_MARKER_SCH:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (MARKER_SCH*) aItem )
        #define DEST   ( (MARKER_SCH*) aImage )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        break;

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (Hierarchical_PIN_Sheet_Struct*) aItem )
        #define DEST   ( (Hierarchical_PIN_Sheet_Struct*) aImage )
        EXCHG( SOURCE->m_Edge, DEST->m_Edge );
        EXCHG( SOURCE->m_Shape, DEST->m_Shape );
        break;

    case DRAW_NOCONNECT_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawNoConnectStruct*) aItem )
        #define DEST   ( (DrawNoConnectStruct*) aImage )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        break;

    case DRAW_PART_TEXT_STRUCT_TYPE:
        break;

    // not directly used in schematic:
    default:
        wxMessageBox(wxT( "SwapData() error: unexpected type" ) );
        break;
    }
}


/***********************************************************************/
void WinEDA_SchematicFrame::SaveCopyInUndoList( SCH_ITEM*      aItem,
                                                UndoRedoOpType aCommandType,
                                                const wxPoint& aTransformPoint )
/***********************************************************************/

/** function SaveCopyInUndoList
 * Create a copy of the current schematic item, and put it in the undo list.
 *
 *  flag_type_command =
 *      UR_CHANGED
 *      UR_NEW
 *      UR_DELETED
 *      UR_WIRE_IMAGE
 *      UR_MOVED
 *
 *  If it is a delete command, items are put on list with the .Flags member set to UR_DELETED.
 *  When it will be really deleted, the EEDrawList and the subhierarchy will be deleted.
 *  If it is only a copy, the EEDrawList and the subhierarchy must NOT be deleted.
 *
 *  Note:
 *  Edit wires and busses is a bit complex.
 *  because when a new wire is added, modifications in wire list
 *  (wire concatenation) there are modified items, deleted items and new items
 *  so flag_type_command is UR_WIRE_IMAGE: the struct ItemToCopy is a list of wires
 *  saved in Undo List (for Undo or Redo commands, saved wires will be exchanged with current wire list
 */
{
    SCH_ITEM*          CopyOfItem;
    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();
    commandToUndo->m_TransformPoint = aTransformPoint;

    ITEM_PICKER        itemWrapper( aItem, aCommandType );
    itemWrapper.m_PickedItemType = aItem->Type();

    switch( aCommandType )
    {
    case UR_CHANGED:            /* Create a copy of item */
        CopyOfItem = DuplicateStruct( aItem );
        itemWrapper.m_Link = CopyOfItem;
        if ( CopyOfItem )
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
        msg.Printf( wxT( "SaveCopyInUndoList() error (unknown code %X)" ), aCommandType );
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


/** function SaveCopyInUndoList
 * @param aItemsList = a PICKED_ITEMS_LIST of items to save
 * @param aTypeCommand = type of comand ( UR_CHANGED, UR_NEW, UR_DELETED ...
 */
void WinEDA_SchematicFrame::SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                                UndoRedoOpType     aTypeCommand,
                                                const wxPoint& aTransformPoint )
{
    SCH_ITEM*          CopyOfItem;
    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();
    commandToUndo->m_TransformPoint = aTransformPoint;

    ITEM_PICKER        itemWrapper;

    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM*      item = (SCH_ITEM*) aItemsList.GetPickedItem( ii );
        UndoRedoOpType command    = aItemsList.GetPickedItemStatus( ii );
        if( command == UR_UNSPECIFIED )
        {
            command = aTypeCommand;
        }
        itemWrapper.m_PickedItem = item;
        itemWrapper.m_PickedItemType = item->Type();
        itemWrapper.m_UndoRedoStatus = command;
        switch( command )
        {
        case UR_CHANGED:        /* Create a copy of item */
            CopyOfItem = DuplicateStruct( item );
            itemWrapper.m_Link = CopyOfItem;
            if ( CopyOfItem )
                commandToUndo->PushItem( itemWrapper );
            break;

        case UR_MOVED:
        case UR_MIRRORED_Y:
        case UR_NEW:
        case UR_DELETED:
            commandToUndo->PushItem( itemWrapper );
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
    else
        delete commandToUndo;
}


/** Function PutDataInPreviousState()
 * Used in undo or redo command.
 * Put data pointed by List in the previous state, i.e. the state memorised by List
 * @param aList = a PICKED_ITEMS_LIST pointer to the list of items to undo/redo
 * @param aRedoCommand = a bool: true for redo, false for undo
 */
void WinEDA_SchematicFrame::PutDataInPreviousState( PICKED_ITEMS_LIST* aList, bool aRedoCommand )
{
    SCH_ITEM* item;
    SCH_ITEM* alt_item;

    for( unsigned ii = 0; ii < aList->GetCount(); ii++  )
    {
        ITEM_PICKER itemWrapper = aList->GetItemWrapper( ii );
        item = (SCH_ITEM*) itemWrapper.m_PickedItem;
        wxASSERT( item  );
        item->m_Flags = 0;
        SCH_ITEM*   image = (SCH_ITEM*) itemWrapper.m_Link;
        switch( itemWrapper.m_UndoRedoStatus )
        {
        case UR_CHANGED:    /* Exchange old and new data for each item */
            SwapData( item, image );
            break;

        case UR_NEW:        /* new items are deleted */
            aList->SetPickedItemStatus( UR_DELETED, ii );
            GetScreen()->RemoveFromDrawList( item );
            break;

        case UR_DELETED:    /* deleted items are put in EEdrawList, as new items */
            aList->SetPickedItemStatus( UR_NEW, ii );
            item->SetNext( GetScreen()->EEDrawList );
            GetScreen()->EEDrawList = item;
            break;

        case UR_MOVED:
            item->Move( aRedoCommand ? aList->m_TransformPoint : - aList->m_TransformPoint );
            break;

        case UR_MIRRORED_Y:
        {
            wxPoint mirrorPoint = aList->m_TransformPoint;
            item->Mirror_Y( mirrorPoint.x );
        }
            break;

        case UR_WIRE_IMAGE:
            /* Exchange the current wires and the old wires */
            alt_item = GetScreen()->ExtractWires( false );
            aList->SetPickedItem( alt_item, ii );
            while( item )
            {
                SCH_ITEM* nextitem = item->Next();
                item->SetNext( GetScreen()->EEDrawList );
                GetScreen()->EEDrawList = item;
                item->m_Flags = 0;
                item = nextitem;
            }

            break;

        default:
        {
            wxString msg;
            msg.Printf( wxT(
                            "PutDataInPreviousState() error (unknown code %X)" ),
                        itemWrapper.m_UndoRedoStatus );
            wxMessageBox( msg );
        }
        break;
        }
    }
}


/**********************************************************/
void WinEDA_SchematicFrame::GetSchematicFromUndoList(wxCommandEvent& event)
/**********************************************************/

/** Function GetSchematicFromUndoList
 *  Undo the last edition:
 *  - Save the current schematic in Redo list
 *  - Get the previous version of the schematic from Unodo list
 *  @return none
 */
{
    if( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    /* Get the old wrapper and put it in RedoList */
    PICKED_ITEMS_LIST* List = GetScreen()->PopCommandFromUndoList();
    GetScreen()->PushCommandToRedoList( List );
    /* Undo the command */
    PutDataInPreviousState( List, false );

    CurrentDrawItem = NULL;
    GetScreen()->SetModify();
    SetSheetNumberAndCount();
    ReCreateHToolbar();
    SetToolbars();

    TestDanglingEnds( GetScreen()->EEDrawList, NULL );
    DrawPanel->Refresh( );
}

/**********************************************************/
void WinEDA_SchematicFrame::GetSchematicFromRedoList(wxCommandEvent& event)
/**********************************************************/

/** Function GetSchematicFromRedoList
 * Redo the last edition:
 *  - Save the current schematic in undo list
 *  - Get the previous version from Redo list
 *  @return none
 */
{
    if( GetScreen()->GetRedoCommandCount() == 0 )
        return;


    /* Get the old wrapper and put it in UndoList */
    PICKED_ITEMS_LIST* List = GetScreen()->PopCommandFromRedoList();
    GetScreen()->PushCommandToUndoList( List );

    /* Redo the command: */
    PutDataInPreviousState( List, true );

    CurrentDrawItem = NULL;
    GetScreen()->SetModify();
    SetSheetNumberAndCount();
    ReCreateHToolbar();
    SetToolbars();

    TestDanglingEnds( GetScreen()->EEDrawList, NULL );
    DrawPanel->Refresh( );
}


/***********************************************************************************/
void SCH_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount )
/**********************************************************************************/

/** Function ClearUndoORRedoList
 * free the undo or redo list from List element
 *  Wrappers are deleted.
 *  datas pointed by wrappers are deleted if not in use in schematic
 *  i.e. when they are copy of a schematic item or they are no more in use (DELETED)
 * @param aList = the UNDO_REDO_CONTAINER to clear
 * @param aItemCount = the count of items to remove. < 0 for all items
 * items (commands stored in list) are removed from the beginning of the list.
 * So this function can be called to remove old commands
 */
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

        // Delete items is they are not flagged UR_NEW, or if this is a block operation
        while( 1 )
        {
            ITEM_PICKER     wrapper = curr_cmd->PopItem();
            EDA_BaseStruct* item    = wrapper.m_PickedItem;
            if( item == NULL ) // No more item in list.
                break;
            switch( wrapper.m_UndoRedoStatus )
            {
            case UR_WIRE_IMAGE:
                while( item )
                {   // Delete old copy of wires
                    EDA_BaseStruct* nextitem = item->Next();
                    delete          item;
                    item = nextitem;
                }

                break;

            case UR_MOVED:
            case UR_MIRRORED_X:
            case UR_MIRRORED_Y:
            case UR_ROTATED:
            case UR_NEW:        // Do nothing, items are in use
                break;

            case UR_DELETED:
                delete item;    // Delete the picked item, because it was deleted from schematic
                break;

            case UR_CHANGED:
                delete wrapper.m_Link;  // Delete the copy of item (the item is itself in use)
                break;

            default:
            {
                wxString msg;
                msg.Printf(
                    wxT("ClearUndoORRedoList() error: unexpected undo/redo type %d"),
                    wrapper.m_UndoRedoStatus );
                wxMessageBox( msg );
                break;
            }
            }
        }

        delete curr_cmd;    // Delete command
    }
}
