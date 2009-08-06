/*************************************************************/
/*  board editor: undo and redo functions for board editor  */
/*************************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"

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
 *      => The list of item(s) is used to create a deleted list in undo list(same as a delete command)
 *
 *   Some block operations that change items can be undoed without memorise items, just the coordiantes of the transform:
 *      move list of items (undo/redo is made by moving with the opposite move vector)
 *      mirror (Y) and flip list of items (undo/redo is made by mirror or flip items)
 *      so they are handled specifically.
 *
 */

BOARD_ITEM* DuplicateStruct( BOARD_ITEM* aItem );


/** function TestForExistingItem
 * test if aItem exists somewhere in lists of items
 * This is a function unsed by PutDataInPreviousState to be sure an item was not deleted
 * since an undo or redo.
 * This could be possible:
 *   - if a call to SaveCopyInUndoList was forgotten in pcbnew
 *   - in zones outlines, when a change in one zone merges this zone with an other
 * This function avoids a pcbnew crash
 * @param aBoard = board to test
 * @param aItem = item to find
 */
static bool TestForExistingItem( BOARD* aPcb, BOARD_ITEM* aItem )
{
    BOARD_ITEM* item;

    // search in tracks:
    for( item = aPcb->m_Track; item != NULL; item = item->Next() )
        if( item == aItem )
            return true;

    // search in modules:
    for( item = aPcb->m_Modules; item != NULL; item = item->Next() )
        if( item == aItem )
            return true;

    // Search in drawings
    for( item = aPcb->m_Drawings; item != NULL; item = item->Next() )
        if( item == aItem )
            return true;

    // Search in zones outlines
    for( int ii = 0; ii < aPcb->GetAreaCount(); ii++ )
        if( aPcb->GetArea( ii ) == aItem )
            return true;

    // search in zones segm:
    for( item = aPcb->m_Zone; item != NULL; item = item->Next() )
        if( item == aItem )
            return true;

    return false;
}


/**************************************************************/
void SwapData( BOARD_ITEM* aItem, BOARD_ITEM* aImage )
/***************************************************************/

/** Function SwapData
 * Used in undo / redo command:
 *  swap data between Item and a copy
 *  swapped data is data modified by edition, mainly sizes and texts
 * so ONLY FEW values are swapped
 * @param aItem = the item
 * @param aImage = a copy of the item
 */
{
    if( aItem == NULL || aImage == NULL )
    {
        wxMessageBox( wxT( "SwapData error: NULL pointer" ) );
        return;
    }

    // Swap layers:
    int layer, layerimg;
    layer    = aItem->GetLayer();
    layerimg = aImage->GetLayer();
    aItem->SetLayer( layerimg );
    aImage->SetLayer( layer );

    switch( aItem->Type() )
    {
    case TYPE_MODULE:
    {
        MODULE* m_tmp = (MODULE*) DuplicateStruct( aImage );
        ( (MODULE*) aImage )->Copy( (MODULE*) aItem );
        ( (MODULE*) aItem )->Copy( m_tmp );
        delete m_tmp;
    }
    break;

    case TYPE_ZONE_CONTAINER:
        wxMessageBox( wxT( "SwapData(): TYPE_ZONE_CONTAINER not handled" ) );
        break;

    case TYPE_DRAWSEGMENT:
        EXCHG( ( (TRACK*) aItem )->m_Start, ( (TRACK*) aImage )->m_Start );
        EXCHG( ( (TRACK*) aItem )->m_End, ( (TRACK*) aImage )->m_End );
        EXCHG( ( (TRACK*) aItem )->m_Width, ( (TRACK*) aImage )->m_Width );
        EXCHG( ( (TRACK*) aItem )->m_Shape, ( (TRACK*) aImage )->m_Shape );
        break;

    case TYPE_TRACK:
    case TYPE_VIA:
    case TYPE_ZONE:
        EXCHG( ( (TRACK*) aItem )->m_Start, ( (TRACK*) aImage )->m_Start );
        EXCHG( ( (TRACK*) aItem )->m_End, ( (TRACK*) aImage )->m_End );
        EXCHG( ( (TRACK*) aItem )->m_Width, ( (TRACK*) aImage )->m_Width );
        EXCHG( ( (TRACK*) aItem )->m_Shape, ( (TRACK*) aImage )->m_Shape );
        break;

    case TYPE_TEXTE:
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Mirror, ( (TEXTE_PCB*) aImage )->m_Mirror );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Size, ( (TEXTE_PCB*) aImage )->m_Size );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Pos, ( (TEXTE_PCB*) aImage )->m_Pos );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Width, ( (TEXTE_PCB*) aImage )->m_Width );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Orient, ( (TEXTE_PCB*) aImage )->m_Orient );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Text, ( (TEXTE_PCB*) aImage )->m_Text );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Italic, ( (TEXTE_PCB*) aImage )->m_Italic );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Bold, ( (TEXTE_PCB*) aImage )->m_Bold );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_HJustify, ( (TEXTE_PCB*) aImage )->m_HJustify );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_VJustify, ( (TEXTE_PCB*) aImage )->m_VJustify );
        break;

    case TYPE_MIRE:
        EXCHG( ( (MIREPCB*) aItem )->m_Pos, ( (MIREPCB*) aImage )->m_Pos );
        EXCHG( ( (MIREPCB*) aItem )->m_Width, ( (MIREPCB*) aImage )->m_Width );
        EXCHG( ( (MIREPCB*) aItem )->m_Size, ( (MIREPCB*) aImage )->m_Size );
        EXCHG( ( (MIREPCB*) aItem )->m_Shape, ( (MIREPCB*) aImage )->m_Shape );
        break;

    case TYPE_COTATION:
    {
        wxString txt = ( (COTATION*) aItem )->GetText();
        ( (COTATION*) aItem )->SetText( ((COTATION*) aImage )->GetText() );
        ( (COTATION*) aImage )->SetText( txt );
        EXCHG( ( (COTATION*) aItem )->m_Text->m_Size, ( (COTATION*) aImage )->m_Text->m_Size );
        EXCHG( ( (COTATION*) aItem )->m_Text->m_Width, ( (COTATION*) aImage )->m_Text->m_Width );
        EXCHG( ( (COTATION*) aItem )->m_Text->m_Mirror, ( (COTATION*) aImage )->m_Text->m_Mirror );
    }
        break;

    default:
        wxMessageBox( wxT( "SwapData() error: unexpected type" ) );
        break;
    }
}


/************************************************************/
BOARD_ITEM* DuplicateStruct( BOARD_ITEM* aItem )
/************************************************************/

/* Routine to create a new copy of given struct.
 *  The new object is not put in list (not linked)
 */
{
    if( aItem == NULL )
    {
        wxMessageBox( wxT( "DuplicateStruct error: NULL struct" ) );
        return NULL;
    }

    switch( aItem->Type() )
    {
    case TYPE_MODULE:
    {
        MODULE* new_module;
        new_module = new MODULE( (BOARD*) aItem->GetParent() );
        new_module->Copy( (MODULE*) aItem );
        return new_module;
    }

    case TYPE_TRACK:
    {
        TRACK* new_track = ( (TRACK*) aItem )->Copy();
        return new_track;
    }

    case TYPE_VIA:
    {
        SEGVIA* new_via = (SEGVIA*)( (SEGVIA*) aItem )->Copy();
        return new_via;
    }

    case TYPE_ZONE:
    {
        SEGZONE* new_segzone = (SEGZONE*)( (SEGZONE*) aItem )->Copy();
        return new_segzone;
    }

    case TYPE_ZONE_CONTAINER:
    {
        ZONE_CONTAINER* new_zone = new ZONE_CONTAINER( (BOARD*) aItem->GetParent() );
        new_zone->Copy( (ZONE_CONTAINER*) aItem );
        return new_zone;
    }

    case TYPE_DRAWSEGMENT:
    {
        DRAWSEGMENT* new_drawsegment = new DRAWSEGMENT( aItem->GetParent() );
        new_drawsegment->Copy( (DRAWSEGMENT*) aItem );
        return new_drawsegment;
    }
    break;

    case TYPE_TEXTE:
    {
        TEXTE_PCB* new_pcbtext = new TEXTE_PCB( aItem->GetParent() );
        new_pcbtext->Copy( (TEXTE_PCB*) aItem );
        return new_pcbtext;
    }
    break;

    case TYPE_MIRE:
    {
        MIREPCB* new_mire = new MIREPCB( aItem->GetParent() );
        new_mire->Copy( (MIREPCB*) aItem );
        return new_mire;
    }
    break;

    case TYPE_COTATION:
    {
        COTATION* new_cotation = new COTATION( aItem->GetParent() );
        new_cotation->Copy( (COTATION*) aItem );
        return new_cotation;
    }
    break;

    default:
    {
        wxString msg;
        msg << wxT( "DuplicateStruct error: unexpected StructType " ) <<
        aItem->Type() << wxT( " " ) << aItem->GetClass();
        wxMessageBox( msg );
    }
    break;
    }

    return NULL;
}


/***********************************************************************/
void WinEDA_PcbFrame::SaveCopyInUndoList( BOARD_ITEM*    aItem,
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
 *      UR_MOVED
 *      UR_FLIPPED
 *      UR_ROTATED
 *
 *  If it is a delete command, items are put on list with the .Flags member set to UR_DELETED.
 *  When it will be really deleted, the EEDrawList and the subhierarchy will be deleted.
 *  If it is only a copy, the EEDrawList and the subhierarchy must NOT be deleted.
 *
 */
{
    if( aItem == NULL )     // Nothing to save
        return;

    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();

    commandToUndo->m_TransformPoint = aTransformPoint;

    ITEM_PICKER itemWrapper( aItem, aCommandType );
    itemWrapper.m_PickedItemType = aItem->Type();

    switch( aCommandType )
    {
    case UR_CHANGED:            /* Create a copy of schematic */
        if( itemWrapper.m_Link == NULL )    // When not null, the copy is already done
            itemWrapper.m_Link = DuplicateStruct( aItem );;
        if( itemWrapper.m_Link )
            commandToUndo->PushItem( itemWrapper );
        break;

    case UR_NEW:
    case UR_MOVED:
    case UR_FLIPPED:
    case UR_ROTATED:
    case UR_ROTATED_CLOCKWISE:
    case UR_DELETED:
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
void WinEDA_PcbFrame::SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                          UndoRedoOpType     aTypeCommand,
                                          const wxPoint&     aTransformPoint )
{
    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();

    commandToUndo->m_TransformPoint = aTransformPoint;

    ITEM_PICKER itemWrapper;

    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        BOARD_ITEM*    item    = (BOARD_ITEM*) aItemsList.GetPickedItem( ii );
        UndoRedoOpType command = aItemsList.GetPickedItemStatus( ii );
        if( command == UR_UNSPECIFIED )
            command = aTypeCommand;

        wxASSERT( item );
        itemWrapper.m_PickedItem     = item;
        itemWrapper.m_PickedItemType = item->Type();
        itemWrapper.m_UndoRedoStatus = command;
        itemWrapper.m_Link = aItemsList.GetPickedItemLink( ii );
        switch( command )
        {
        case UR_CHANGED:        /* If needed, create a copy of item, and put in undo list */
            if( itemWrapper.m_Link == NULL )    // When not null, the copy is already done
                itemWrapper.m_Link = DuplicateStruct( item );
            if( itemWrapper.m_Link )
                commandToUndo->PushItem( itemWrapper );
            break;

        case UR_MOVED:
        case UR_ROTATED:
        case UR_ROTATED_CLOCKWISE:
        case UR_FLIPPED:
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
void WinEDA_PcbFrame::PutDataInPreviousState( PICKED_ITEMS_LIST* aList, bool aRedoCommand )
{
    BOARD_ITEM* item;
    bool        not_found = false;
    bool        reBuild_ratsnest = false;

    for( unsigned ii = 0; ii < aList->GetCount(); ii++  )
    {
        item = (BOARD_ITEM*) aList->GetPickedItem( ii );
        wxASSERT( item );
        if( aList->GetPickedItemStatus( ii ) != UR_DELETED )
        {
            if( !TestForExistingItem( GetBoard(), item ) )
            {
                // Remove this non existant item
                aList->RemovePickedItem( ii );
                ii--;       // the current item was removed, ii points now the next item
                            // whe must decrement it because it will be incremented
                not_found = true;
                continue;
            }
        }
        item->m_Flags = 0;

        // see if one must rebuild ratsnets and pointers lists
        switch( item->Type() )
        {
        case TYPE_MODULE:
        case TYPE_ZONE_CONTAINER:
        case TYPE_TRACK:
        case TYPE_VIA:
            reBuild_ratsnest = true;
            break;

        default:
            break;
        }

        switch( aList->GetPickedItemStatus( ii ) )
        {
        case UR_CHANGED:    /* Exchange old and new data for each item */
        {
            BOARD_ITEM* image = (BOARD_ITEM*) aList->GetPickedItemLink( ii );
            SwapData( item, image );
        }
        break;

        case UR_NEW:        /* new items are deleted */
            aList->SetPickedItemStatus( UR_DELETED, ii );
            GetBoard()->Remove( item );
            break;

        case UR_DELETED:    /* deleted items are put in List, as new items */
            aList->SetPickedItemStatus( UR_NEW, ii );
            GetBoard()->Add( item );
            break;

        case UR_MOVED:
            item->Move( aRedoCommand ? aList->m_TransformPoint : -aList->m_TransformPoint );
            break;

        case UR_ROTATED:
            item->Rotate( aList->m_TransformPoint, aRedoCommand ? 900 : -900 );
            break;

        case UR_ROTATED_CLOCKWISE:
            item->Rotate( aList->m_TransformPoint, aRedoCommand ? -900 : 900 );
            break;

        case UR_FLIPPED:
            item->Flip( aList->m_TransformPoint );
            break;

        default:
        {
            wxString msg;
            msg.Printf( wxT(
                           "PutDataInPreviousState() error (unknown code %X)" ),
                       aList->GetPickedItemStatus( ii ) );
            wxMessageBox( msg );
        }
        break;
        }
    }

    if( not_found )
        wxMessageBox( wxT( "Incomplete undo/redo command: item not found" ) );

    // Rebuild pointers and rastnest
    if( reBuild_ratsnest )
        Compile_Ratsnest( NULL, true );
}


/**********************************************************/
void WinEDA_PcbFrame::GetBoardFromUndoList( wxCommandEvent& event )
/**********************************************************/

/** Function GetBoardFromUndoList
 *  Undo the last edition:
 *  - Save the current board in Redo list
 *  - Get an old version of the board from Undo list
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

    GetScreen()->SetModify();
    ReCreateHToolbar();
    SetToolbars();

    DrawPanel->Refresh();
}


/** Function GetBoardFromRedoList
 *  Redo the last edition:
 *  - Save the current board in Undo list
 *  - Get an old version of the board from Redo list
 *  @return none
 */
void WinEDA_PcbFrame::GetBoardFromRedoList( wxCommandEvent& event )
{
    if( GetScreen()->GetRedoCommandCount() == 0 )
        return;


    /* Get the old wrapper and put it in UndoList */
    PICKED_ITEMS_LIST* List = GetScreen()->PopCommandFromRedoList();
    GetScreen()->PushCommandToUndoList( List );

    /* Redo the command: */
    PutDataInPreviousState( List, true );

    GetScreen()->SetModify();
    ReCreateHToolbar();
    SetToolbars();

    DrawPanel->Refresh();
}


/***********************************************************************************/
void PCB_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount )
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
    bool     displ_error = true;
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
