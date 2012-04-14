/*************************************************************/
/*  board editor: undo and redo functions for board editor  */
/*************************************************************/

#include <fctsys.h>
#include <class_drawpanel.h>
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


/* Functions to undo and redo edit commands.
 *  commmands to undo are stored in CurrentScreen->m_UndoList
 *  commmands to redo are stored in CurrentScreen->m_RedoList
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
 *   Some block operations that change items can be undoed without memorise items, just the
 *   coordiantes of the transform:
 *      move list of items (undo/redo is made by moving with the opposite move vector)
 *      mirror (Y) and flip list of items (undo/redo is made by mirror or flip items)
 *      so they are handled specifically.
 *
 */


/**
 * Function TestForExistingItem
 * test if aItem exists somewhere in lists of items
 * This is a function unsed by PutDataInPreviousState to be sure an item was not deleted
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
    static std::vector<BOARD_ITEM*> itemsList;

    if( aItem == NULL ) // Build list
    {
        // Count items to store in itemsList:
        int icnt = 0;
        BOARD_ITEM* item;

        // Count tracks:
        for( item = aPcb->m_Track; item != NULL; item = item->Next() )
            icnt++;

        // Count modules:
        for( item = aPcb->m_Modules; item != NULL; item = item->Next() )
            icnt++;

        // Count drawings
        for( item = aPcb->m_Drawings; item != NULL; item = item->Next() )
            icnt++;

        // Count zones outlines
        icnt +=  aPcb->GetAreaCount();

        // Count zones segm (now obsolete):
        for( item = aPcb->m_Zone; item != NULL; item = item->Next() )
             icnt++;

        // Build candidate list:
        itemsList.clear();
        itemsList.reserve(icnt);

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

        // Sort list
        std::sort( itemsList.begin(), itemsList.end() );
        return false;
    }

    // search in list:
    return std::binary_search( itemsList.begin(), itemsList.end(), aItem );
}


/**
 * Function SwapData
 * Used in undo / redo command:
 *  swap data between Item and a copy
 *  swapped data is data modified by edition, mainly sizes and texts
 * so ONLY FEW values are swapped
 * @param aItem = the item
 * @param aImage = a copy of the item
 */
void SwapData( BOARD_ITEM* aItem, BOARD_ITEM* aImage )
{
    if( aItem == NULL || aImage == NULL )
    {
        wxMessageBox( wxT( "SwapData error: NULL pointer" ) );
        return;
    }

    // Swap layers:
    if( aItem->Type() != PCB_MODULE_T && aItem->Type() != PCB_ZONE_AREA_T )
    {
        // These items have a global swap function.
        int layer, layerimg;
        layer    = aItem->GetLayer();
        layerimg = aImage->GetLayer();
        aItem->SetLayer( layerimg );
        aImage->SetLayer( layer );
    }

    switch( aItem->Type() )
    {
    case PCB_MODULE_T:
        {
            MODULE* tmp = (MODULE*) aImage->Clone();
            ( (MODULE*) aImage )->Copy( (MODULE*) aItem );
            ( (MODULE*) aItem )->Copy( tmp );
            delete tmp;
        }
        break;

    case PCB_ZONE_AREA_T:
        {
            ZONE_CONTAINER* tmp = (ZONE_CONTAINER*) aImage->Clone();
            ( (ZONE_CONTAINER*) aImage )->Copy( (ZONE_CONTAINER*) aItem );
            ( (ZONE_CONTAINER*) aItem )->Copy( tmp );
            delete tmp;
        }
        break;

    case PCB_LINE_T:
#if 0
        EXCHG( ( (DRAWSEGMENT*) aItem )->m_Start, ( (DRAWSEGMENT*) aImage )->m_Start );
        EXCHG( ( (DRAWSEGMENT*) aItem )->m_End, ( (DRAWSEGMENT*) aImage )->m_End );
        EXCHG( ( (DRAWSEGMENT*) aItem )->m_Width, ( (DRAWSEGMENT*) aImage )->m_Width );
        EXCHG( ( (DRAWSEGMENT*) aItem )->m_Shape, ( (DRAWSEGMENT*) aImage )->m_Shape );
#else
        {
            DRAWSEGMENT tmp = *(DRAWSEGMENT*) aImage;
            *aImage = *aItem;
            *aItem  = tmp;
        }
#endif
        break;

    case PCB_TRACE_T:
    case PCB_VIA_T:
        {
            TRACK* track = (TRACK*) aItem;
            TRACK* image = (TRACK*) aImage;
            EXCHG( track->m_Start, image->m_Start );
            EXCHG( track->m_End, image->m_End );
            EXCHG( track->m_Width, image->m_Width );
            EXCHG( track->m_Shape, image->m_Shape );
            int atmp = track->GetDrillValue();

            if( track->IsDrillDefault() )
                atmp = -1;

            int itmp = image->GetDrillValue();

            if( image->IsDrillDefault() )
                itmp = -1;

            EXCHG(itmp, atmp );

            if( atmp > 0 )
                track->SetDrill( atmp );
            else
                track->SetDrillDefault();

            if( itmp > 0 )
                image->SetDrill( itmp );
            else
                image->SetDrillDefault();
        }
        break;

    case PCB_TEXT_T:
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Mirror, ( (TEXTE_PCB*) aImage )->m_Mirror );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Size, ( (TEXTE_PCB*) aImage )->m_Size );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Pos, ( (TEXTE_PCB*) aImage )->m_Pos );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Thickness, ( (TEXTE_PCB*) aImage )->m_Thickness );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Orient, ( (TEXTE_PCB*) aImage )->m_Orient );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Text, ( (TEXTE_PCB*) aImage )->m_Text );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Italic, ( (TEXTE_PCB*) aImage )->m_Italic );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_Bold, ( (TEXTE_PCB*) aImage )->m_Bold );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_HJustify, ( (TEXTE_PCB*) aImage )->m_HJustify );
        EXCHG( ( (TEXTE_PCB*) aItem )->m_VJustify, ( (TEXTE_PCB*) aImage )->m_VJustify );
        break;

    case PCB_TARGET_T:
        ( (PCB_TARGET*) aItem )->Exchg( (PCB_TARGET*) aImage );
        break;

    case PCB_DIMENSION_T:
        {
            wxString txt = ( (DIMENSION*) aItem )->GetText();
            ( (DIMENSION*) aItem )->SetText( ( (DIMENSION*) aImage )->GetText() );
            ( (DIMENSION*) aImage )->SetText( txt );
            EXCHG( ( (DIMENSION*) aItem )->m_Width, ( (DIMENSION*) aImage )->m_Width );
            EXCHG( ( (DIMENSION*) aItem )->m_Text.m_Size, ( (DIMENSION*) aImage )->m_Text.m_Size );
            EXCHG( ( (DIMENSION*) aItem )->m_Text.m_Pos, ( (DIMENSION*) aImage )->m_Text.m_Pos );
            EXCHG( ( (DIMENSION*) aItem )->m_Text.m_Thickness,
                   ( (DIMENSION*) aImage )->m_Text.m_Thickness );
            EXCHG( ( (DIMENSION*) aItem )->m_Text.m_Mirror,
                   ( (DIMENSION*) aImage )->m_Text.m_Mirror );
        }
        break;

    case PCB_ZONE_T:
    default:
        wxMessageBox( wxT( "SwapData() error: unexpected type" ) );
        break;
    }
}


/*
 * Function SaveCopyInUndoList
 * Create a copy of the current board item, and put it in the undo list.
 *
 *  aCommandType =
 *      UR_CHANGED
 *      UR_NEW
 *      UR_DELETED
 *      UR_MOVED
 *      UR_FLIPPED
 *      UR_ROTATED
 */
void PCB_EDIT_FRAME::SaveCopyInUndoList( BOARD_ITEM*    aItem,
                                         UNDO_REDO_T    aCommandType,
                                         const wxPoint& aTransformPoint )
{
    if( aItem == NULL )     // Nothing to save
        return;

    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();

    commandToUndo->m_TransformPoint = aTransformPoint;

    ITEM_PICKER itemWrapper( aItem, aCommandType );

    switch( aCommandType )
    {
    case UR_CHANGED:                        // Create a copy of item
        if( itemWrapper.GetLink() == NULL ) // When not null, the copy is already done
            itemWrapper.SetLink( aItem->Clone() );
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
    {
        delete commandToUndo;
    }
}


/**
 * Function SaveCopyInUndoList
 * @param aItemsList = a PICKED_ITEMS_LIST of items to save
 * @param aTypeCommand = type of comand ( UR_CHANGED, UR_NEW, UR_DELETED ...
 * @param aTransformPoint - Transform items around this point.
 */
void PCB_EDIT_FRAME::SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                         UNDO_REDO_T        aTypeCommand,
                                         const wxPoint&     aTransformPoint )
{
    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();

    commandToUndo->m_TransformPoint = aTransformPoint;

    // Copy picker list:
    commandToUndo->CopyList( aItemsList );

    // Verify list, and creates data if needed
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
                commandToUndo->SetPickedItemLink( item->Clone(), ii );
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
    else    // Should not occur
    {
        delete commandToUndo;
    }
}


/**
 * Function PutDataInPreviousState
 * Used in undo or redo command.
 * Put data pointed by List in the previous state, i.e. the state memorised by List
 * @param aList = a PICKED_ITEMS_LIST pointer to the list of items to undo/redo
 * @param aRedoCommand = a bool: true for redo, false for undo
 * @param aRebuildRatsnet = a bool: true to rebuid ratsnet (normal use, and default), false
 * to just retrieve las state (used in abort commands that do not need to rebuild ratsnest)
 */
void PCB_EDIT_FRAME::PutDataInPreviousState( PICKED_ITEMS_LIST* aList, bool aRedoCommand,
                                             bool aRebuildRatsnet )
{
    BOARD_ITEM* item;
    bool        not_found = false;
    bool        reBuild_ratsnest = false;

    // Undo in the reverse order of list creation: (this can allow stacked changes
    // like the same item can be changes and deleted in the same complex command

    bool build_item_list = true;    // if true the list of esiting items must be rebuilt
    for( int ii = aList->GetCount()-1; ii >= 0 ; ii--  )
    {
        item = (BOARD_ITEM*) aList->GetPickedItem( ii );
        wxASSERT( item );

        /* Test for existence of item on board.
         * It could be deleted, and no more on board:
         *   - if a call to SaveCopyInUndoList was forgotten in Pcbnew
         *   - in zones outlines, when a change in one zone merges this zone with an other
         * This test avoids a Pcbnew crash
         * Obviouly, this test is not made for deleted items
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
                // Remove this non existant item
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
        case PCB_ZONE_AREA_T:
        case PCB_TRACE_T:
        case PCB_VIA_T:
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
            build_item_list = true;
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
            msg.Printf( wxT( "PutDataInPreviousState() error (unknown code %X)" ),
                        aList->GetPickedItemStatus( ii ) );
            wxMessageBox( msg );
        }
        break;
        }
    }

    if( not_found )
        wxMessageBox( wxT( "Incomplete undo/redo operation: some items not found" ) );

    // Rebuild pointers and rastnest that can be changed.
    if( reBuild_ratsnest && aRebuildRatsnet )
        Compile_Ratsnest( NULL, true );
}


/**
 * Function GetBoardFromUndoList
 * Undo the last edition:
 *     - Save the current board state in Redo list
 *     - Get an old version of the board state from Undo list
 */
void PCB_EDIT_FRAME::GetBoardFromUndoList( wxCommandEvent& event )
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
    m_canvas->Refresh();
}


/**
 * Function GetBoardFromRedoList
 *  Redo the last edition:
 *  - Save the current board in Undo list
 *  - Get an old version of the board from Redo list
 *  @return none
 */
void PCB_EDIT_FRAME::GetBoardFromRedoList( wxCommandEvent& event )
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
    m_canvas->Refresh();
}


/**
 * Function ClearUndoORRedoList
 * free the undo or redo list from List element
 *  Wrappers are deleted.
 *  datas pointed by wrappers are deleted if not in use in schematic
 *  i.e. when they are copy of a schematic item or they are no more in use (DELETED)
 * @param aList = the UNDO_REDO_CONTAINER to clear
 * @param aItemCount = the count of items to remove. < 0 for all items
 * items (commands stored in list) are removed from the beginning of the list.
 * So this function can be called to remove old commands
 */
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
