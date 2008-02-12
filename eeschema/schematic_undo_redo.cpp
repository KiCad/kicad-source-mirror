/********************************************/
/*  library editor: undo and redo functions */
/********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "id.h"

#include "protos.h"

/* Functions to undo and redo edit commands.
 *  commmands to undo are in CurrentScreen->m_UndoList
 *  commmands to redo are in CurrentScreen->m_RedoList
 * 
 *  m_UndoList and m_RedoList are a linked list of DrawPickedStruct.
 *  each DrawPickedStruct has its .m_Son member pointing to an item to undo or redo,
 *  or to a list of DrawPickedStruct which points (.m_PickedStruct membre)
 *  the items to undo or redo
 * 
 *  there are 3 cases:
 *  - delete item(s) command
 *  - change item(s) command
 *  - add item(s) command
 * 
 *  Undo command
 *  - delete item(s) command:
 *  deleted items are moved in undo list
 * 
 *  - change item(s) command
 *  A copy of item(s) is made (a DrawPickedStruct list of wrappers)
 *  the .m_Image member of each wrapper points the modified item.
 * 
 *  - add item(s) command
 *  A list of item(s) is made
 *  the .m_Image member of each wrapper points the new item.
 * 
 *  Redo command
 *  - delete item(s) old command:
 *  deleted items are moved in EEDrawList list
 * 
 *  - change item(s) command
 *  the copy of item(s) is moved in Undo list
 * 
 *  - add item(s) command
 *  The list of item(s) is used to create a deleted list in undo list
 *  (same as a delete command)
 * 
 *  A problem is the hierarchical sheet handling.
 *  the data associated (subhierarchy, uno/redo list) is deleted only
 *  when the sheet is really deleted (i.e. when deleted from undo or redo list)
 *  and not when it is a copy.
 */


/************************************/
void SwapData( EDA_BaseStruct* Item )
/************************************/

/* Used if undo / redo command:
 *  swap data between Item and its copy, pointed by its .m_Image member
 */
{
    if( Item == NULL )
        return;
    EDA_BaseStruct* image = Item->m_Image;
    if( image == NULL )
        return;

    switch( Item->Type() )
    {
    case DRAW_POLYLINE_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawPolylineStruct*) Item )
        #define DEST   ( (DrawPolylineStruct*) image )
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawJunctionStruct*) Item )
        #define DEST   ( (DrawJunctionStruct*) image )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        break;

    case DRAW_LABEL_STRUCT_TYPE:
    case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
	case DRAW_HIER_LABEL_STRUCT_TYPE:
    case DRAW_TEXT_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawTextStruct*) Item )
        #define DEST   ( (DrawTextStruct*) image )
        DEST->SwapData( SOURCE );
        break;

    case DRAW_LIB_ITEM_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (EDA_SchComponentStruct*) Item )
        #define DEST   ( (EDA_SchComponentStruct*) image )
        DEST->SwapData( SOURCE );
        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (EDA_DrawLineStruct*) Item )
        #define DEST   ( (EDA_DrawLineStruct*) image )
        EXCHG( SOURCE->m_Start, DEST->m_Start );
        EXCHG( SOURCE->m_End, DEST->m_End );
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawBusEntryStruct*) Item )
        #define DEST   ( (DrawBusEntryStruct*) image )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        EXCHG( SOURCE->m_Size, DEST->m_Size );
        break;

    case DRAW_SHEET_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawSheetStruct*) Item )
        #define DEST   ( (DrawSheetStruct*) image )
        DEST->SwapData( SOURCE );
        break;

    case DRAW_MARKER_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawMarkerStruct*) Item )
        #define DEST   ( (DrawMarkerStruct*) image )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        break;

    case DRAW_SHEETLABEL_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawSheetLabelStruct*) Item )
        #define DEST   ( (DrawSheetLabelStruct*) image )
        EXCHG( SOURCE->m_Edge, DEST->m_Edge );
        EXCHG( SOURCE->m_Shape, DEST->m_Shape );
        break;

    case DRAW_NOCONNECT_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawNoConnectStruct*) Item )
        #define DEST   ( (DrawNoConnectStruct*) image )
        EXCHG( SOURCE->m_Pos, DEST->m_Pos );
        break;

    case DRAW_PART_TEXT_STRUCT_TYPE:
        #undef SOURCE
        #undef DEST
        #define SOURCE ( (DrawPolylineStruct*) Item )
        #define DEST   ( (DrawPolylineStruct*) image )
        break;

        // not directly used in schematic:
    default:
        DisplayInfo( NULL, wxT( "SwapData() error: unexpected type" ) );
        break;
    }
}


/***********************************************************************/
void WinEDA_SchematicFrame::SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy,
                                                int             flag_type_command )
/***********************************************************************/

/* Create a copy of the current schematic draw list, and put it in the undo list.
 *  A DrawPickedStruct wrapper is created to handle the draw list.
 *  the .m_Son of this wrapper points the list of items
 * 
 *  flag_type_command =
 *      0 (unspecified)
 *      IS_CHANGED
 *      IS_NEW
 *      IS_DELETED
 *      IS_WIRE_IMAGE
 * 
 *  for 0: only a wrapper is created. The used must init the .Flags member of the
 *  wrapper, and add the item list to the wrapper
 *  If it is a delete command, items are put on list with the .Flags member set to IS_DELETED.
 *  When it will be really deleted, the EEDrawList and the subhierarchy will be deleted.
 *  If it is only a copy, the EEDrawList and the subhierarchy must NOT be deleted.
 * 
 *  Note:
 *  Edit wires and busses is a bit complex.
 *  because when a new wire is added, modifications in wire list
 *  (wire concatenation) there are modified items, deleted items and new items
 *  so flag_type_command is IS_WIRE_IMAGE: the struct ItemToCopy is a list of wires
 *  saved in Undo List (for Undo or Redo commands, saved wires will be exchanged with current wire list
 */
{
    EDA_BaseStruct*   CopyItem;

    DrawPickedStruct* NewList = new DrawPickedStruct( NULL );

    NewList->m_Flags = flag_type_command;

    if( ItemToCopy )
    {
        switch( flag_type_command )
        {
        case 0:
            break;

        case IS_CHANGED:        /* Create a copy of schematic */
            NewList->m_Son = CopyItem = DuplicateStruct( ItemToCopy );
            if( ItemToCopy->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
            {
                DrawPickedStruct* PickedList = (DrawPickedStruct*) CopyItem;
                while( PickedList )
                {
                    CopyItem = PickedList->m_PickedStruct;
                    CopyItem->m_Flags   = flag_type_command;
                    PickedList->m_Image = CopyItem->m_Image;
                    PickedList = (DrawPickedStruct*) PickedList->Pnext;
                }
            }
            else
            {
                CopyItem->m_Flags = flag_type_command;
                CopyItem->m_Image = ItemToCopy;
            }
            break;

        case IS_NEW:
            if( ItemToCopy->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
            {
                NewList->m_Son = ItemToCopy;
                DrawPickedStruct* PickedList = (DrawPickedStruct*) ItemToCopy;
                while( PickedList )
                {
                    CopyItem = PickedList->m_PickedStruct;
                    PickedList->m_Image = CopyItem;
                    PickedList->m_PickedStruct = NULL;
                    PickedList->m_Flags = flag_type_command;
                    PickedList = (DrawPickedStruct*) PickedList->Pnext;
                }
            }
            else
            {
                NewList->m_Image = ItemToCopy;
            }
            break;

        case IS_NEW | IS_CHANGED:
        case IS_WIRE_IMAGE:
            NewList->m_Son = ItemToCopy;
            break;

        case IS_DELETED:
            NewList->m_Son      = ItemToCopy;
            ItemToCopy->m_Flags = flag_type_command;
            if( ItemToCopy->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
            {
                DrawPickedStruct* PickedList = (DrawPickedStruct*) ItemToCopy;
                while( PickedList )
                {
                    CopyItem = PickedList->m_PickedStruct;
                    CopyItem->m_Flags   = flag_type_command;
                    PickedList->m_Flags = flag_type_command;
                    PickedList = (DrawPickedStruct*) PickedList->Pnext;
                }
            }
            break;

        default:
            DisplayError( this, wxT( "SaveCopyInUndoList() error" ) );
            break;
        }
    }
    /* Save the copy in undo list */
    GetScreen()->AddItemToUndoList( NewList );

    /* Clear redo list, because after new save there is no redo to do */
	((SCH_SCREEN*)GetScreen())->ClearUndoORRedoList( GetScreen()->m_RedoList );
    GetScreen()->m_RedoList = NULL;
}


/**********************************************************/
bool  WinEDA_SchematicFrame::GetSchematicFromRedoList()
/**********************************************************/

/* Redo the last edition:
 *  - Save the current schematic in undo list
 *  - Get the old version
 *  @return FALSE if nothing done, else TRUE
 */
{
    if( GetScreen()->m_RedoList == NULL )
        return FALSE;

    /* Get the old wrapper and put it in UndoList */
    DrawPickedStruct* List = (DrawPickedStruct*) GetScreen()->GetItemFromRedoList();
    GetScreen()->AddItemToUndoList( List );
    /* Redo the command: */
    PutDataInPreviousState( List );

    CurrentDrawItem = NULL;
    GetScreen()->SetModify();
    ReCreateHToolbar();
    SetToolbars();
	
	return TRUE;
}


/***************************************************************************/
void WinEDA_SchematicFrame::PutDataInPreviousState( DrawPickedStruct* List )
/***************************************************************************/

/* Used in undo or redo command.
 *  Put data pointed by List in the previous state, i.e. the state memorised by List
 */
{
    EDA_BaseStruct*   FirstItem = List->m_Son;
    EDA_BaseStruct*   item;
    DrawPickedStruct* PickedList;

    switch( List->m_Flags )
    {
    case IS_CHANGED:        /* Exchange old and new data for each item */
        if( FirstItem == NULL )
        {
            DisplayError( this, wxT( "PutDataInPreviousState() error : Null item" ) );
            break;
        }
        if( FirstItem->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
        {
            DrawPickedStruct* PickedList = (DrawPickedStruct*) FirstItem;
            while( PickedList )
            {
                SwapData( PickedList->m_PickedStruct );
                PickedList = PickedList->Next();
            }
        }
        else
        {
            SwapData( FirstItem );
        }
        break;

    case IS_NEW:            /* new items are deleted */
        List->m_Flags = IS_DELETED;
        if( FirstItem && FirstItem->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
        {
            PickedList = (DrawPickedStruct*) FirstItem;
            while( PickedList )
            {
                item = PickedList->m_Image;
				((SCH_SCREEN*)GetScreen())->RemoveFromDrawList( item );
                item->m_Flags = IS_DELETED;
                PickedList->m_PickedStruct = item;
                PickedList->m_Flags = IS_DELETED;
                PickedList = PickedList->Next();
            }
        }
        else
        {
            FirstItem = List->m_Image;
			((SCH_SCREEN*)GetScreen())->RemoveFromDrawList( FirstItem );
            FirstItem->m_Flags = IS_DELETED;
            List->m_Son = FirstItem;
        }
        break;

    case IS_DELETED:        /* deleted items are put in EEdrawList, as new items */
        List->m_Flags = IS_NEW;
        if( FirstItem->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
        {
            PickedList = (DrawPickedStruct*) FirstItem;
            while( PickedList )
            {
                item = PickedList->m_PickedStruct;
                item->Pnext = GetScreen()->EEDrawList;
                GetScreen()->EEDrawList = item;
                item->m_Flags = 0;
                PickedList->m_PickedStruct = NULL;
                PickedList->m_Image = item;
                PickedList->m_Flags = IS_NEW;
                PickedList = PickedList->Next();
            }
        }
        else
        {
            FirstItem->Pnext = GetScreen()->EEDrawList;
            GetScreen()->EEDrawList = FirstItem;
            FirstItem->m_Flags = 0;
            List->m_Image = List->m_Son;
            List->m_Son   = NULL;
        }
        break;

    case IS_WIRE_IMAGE:
        /* Exchange the current wires and the oild wires */
		List->m_Son = ((SCH_SCREEN*)GetScreen())->ExtractWires( FALSE );
        while( FirstItem )
        {
            EDA_BaseStruct* nextitem = FirstItem->Pnext;
            FirstItem->Pnext = GetScreen()->EEDrawList;
            GetScreen()->EEDrawList = FirstItem;
            FirstItem->m_Flags = 0;
            FirstItem = nextitem;
        }

        break;

    case IS_NEW | IS_CHANGED:
    case IS_DELETED | IS_CHANGED:
        if( !FirstItem || FirstItem->Type() != DRAW_PICK_ITEM_STRUCT_TYPE )
        {
            DisplayError( this, wxT( "GetSchematicFromUndoList() error: Pickstruct expected" ) );
            break;
        }
        PickedList = (DrawPickedStruct*) FirstItem;
        while( PickedList )
        {
            switch( PickedList->m_Flags )
            {
            case IS_CHANGED:
                SwapData( PickedList->m_PickedStruct );
                break;

            case IS_NEW:
                item = PickedList->m_Image;
                ((SCH_SCREEN*)GetScreen())->RemoveFromDrawList( item );
                item->m_Flags = IS_DELETED;
                PickedList->m_PickedStruct = item;
                PickedList->m_Flags = IS_DELETED;
                break;

            case IS_DELETED:
                item = PickedList->m_PickedStruct;
                item->Pnext = GetScreen()->EEDrawList;
                GetScreen()->EEDrawList = item;
                item->m_Flags = 0;
                PickedList->m_PickedStruct = NULL;
                PickedList->m_Image = item;
                PickedList->m_Flags = IS_NEW;
                break;
            }

            PickedList = PickedList->Next();
        }

        break;

    default:
        DisplayError( this, wxT( "GetSchematicFromUndoList() error: Unknown cmd" ) );
        break;
    }
}


/**********************************************************/
bool WinEDA_SchematicFrame::GetSchematicFromUndoList()
/**********************************************************/

/* Undo the last edition:
 *  - Save the current schematic in Redo list
 *  - Get an old version of the schematic
 *  @return FALSE if nothing done, else TRUE
 */
{
    if( GetScreen()->m_UndoList == NULL )
        return FALSE;

    /* Get the old wrapper and put it in RedoList (the real data list is the m_Son member) */
    DrawPickedStruct* List = (DrawPickedStruct*) GetScreen()->GetItemFromUndoList();
    GetScreen()->AddItemToRedoList( List );
    /* Undo the command */
    PutDataInPreviousState( List );

    CurrentDrawItem = NULL;
    GetScreen()->SetModify();
    ReCreateHToolbar();
    SetToolbars();
	
	return TRUE;
}


/*********************************************************/
void SCH_SCREEN::ClearUndoORRedoList( EDA_BaseStruct* List )
/*********************************************************/

/* free the undo or redo list from List element
 *  Wrappers are deleted.
 *  datas pointed by wrappers are deleted if not flagged IS_NEW
 *  because they are copy of used data or they are not in use (DELETED)
 */
{
    EDA_BaseStruct* nextitem;
    EDA_BaseStruct* FirstItem;
    int             CmdType;

    if( List == NULL )
        return;

    for( ; List != NULL; List = nextitem )
    {
        nextitem  = List->Pnext;
        FirstItem = List->m_Son;
        CmdType   = List->m_Flags;

		SAFE_DELETE( List );

        if( FirstItem == NULL )
            continue;

        if( FirstItem->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
        {
            EDA_BaseStruct*   item;
            DrawPickedStruct* PickedList = (DrawPickedStruct*) FirstItem;
            while( PickedList )    // delete wrapper list and copies
            {
                item = PickedList->m_PickedStruct;
                if( item )
                {
                    if( item->Type() == DRAW_SHEET_STRUCT_TYPE )
                    {
                        DrawSheetStruct* sheet = (DrawSheetStruct*) item;
                        /* Delete sub hierarchy if the sheet must be deleted */
                        if( (sheet->m_Flags & IS_DELETED) != 0 )
                            DeleteSubHierarchy( sheet, FALSE );
                        else    /* EEDrawList, UndoList and Redo list are shared, and are deleted
                                 *  only if sheet is deleted */
                        {
                            if( (item->m_Flags & IS_NEW) == 0 )
                            {
								printf("schematic undo_redo.cpp: undo_redo with a DRAW_SHEET_STRUCT_TYPE, checkme!!\n"); 
								/*
                                sheet->EEDrawList = NULL;
                                sheet->m_UndoList = NULL;
                                sheet->m_RedoList = NULL;
								*/
                            }
                        }
                    }
                    if( (item->m_Flags & IS_NEW) == 0 ){
						SAFE_DELETE( item );
					}
                }
                DrawPickedStruct* wrapper = PickedList;
                PickedList = PickedList->Next();
				SAFE_DELETE( wrapper );
            }
        }
        else    // This is a single item: deleted copy
        {
            if( CmdType == IS_WIRE_IMAGE )
            {
                while( FirstItem )
                {
                    EDA_BaseStruct* nextitem = FirstItem->Pnext;
                    delete          FirstItem;
                    FirstItem = nextitem;
                }
            }
            else
            {
                if( FirstItem->Type() == DRAW_SHEET_STRUCT_TYPE )
                {
                    DrawSheetStruct* sheet = (DrawSheetStruct*) FirstItem;
                    /* Delete hierarchy if the sheet must be deleted */
                    if( (sheet->m_Flags & IS_DELETED) == IS_DELETED )
                        DeleteSubHierarchy( sheet, FALSE );
                    else
                    {
                        if( (FirstItem->m_Flags & IS_NEW) == 0 )
                        {
							printf("schematic undo_redo.cpp undo_redo with a DRAW_SHEET_STRUCT_TYPE, checkme!!\n"); 
							/*
                            sheet->EEDrawList = NULL;
                            sheet->m_UndoList = NULL;
                            sheet->m_RedoList = NULL;
							*/
                        }
                    }
                }
                if( (FirstItem->m_Flags & IS_NEW) == 0 ){
					SAFE_DELETE( FirstItem );
				}
            }
        }
    }
}


/*****************************************/
void SCH_SCREEN::ClearUndoRedoList()
/*****************************************/

/* free the undo and the redo lists
 */
{
    ClearUndoORRedoList( m_UndoList ); m_UndoList = NULL;
    ClearUndoORRedoList( m_RedoList ); m_RedoList = NULL;
}


/***********************************************************/
void SCH_SCREEN::AddItemToUndoList( EDA_BaseStruct* newitem )
/************************************************************/

/* Put newitem in head of undo list
 *  Deletes olds items if > count max.
 */
{
    int             ii;
    EDA_BaseStruct* item, * nextitem;

    if( newitem == NULL )
        return;

    if( m_UndoList )
        m_UndoList->Pback = newitem;
    newitem->Pnext = m_UndoList;
    newitem->Pback = NULL;
    m_UndoList = newitem;

    /* Free oldest items, if count max reached */
    for( ii = 0, item = m_UndoList; ii < m_UndoRedoCountMax; ii++ )
    {
        if( item->Pnext == NULL )
            return;
        item = item->Pnext;
    }

    if( item == NULL )
        return;

    nextitem    = item->Pnext;
    item->Pnext = NULL; // Set end of chain

    // Delete the extra  items
    ClearUndoORRedoList( nextitem );
}


/***********************************************************/
void SCH_SCREEN::AddItemToRedoList( EDA_BaseStruct* newitem )
/***********************************************************/
{
    int             ii;
    EDA_BaseStruct* item, * nextitem;

    if( newitem == NULL )
        return;

    newitem->Pback = NULL;
    newitem->Pnext = m_RedoList;
    m_RedoList = newitem;
    /* Free first items, if count max reached */
    for( ii = 0, item = m_RedoList; ii < m_UndoRedoCountMax; ii++ )
    {
        if( item->Pnext == NULL )
            break;
        item = item->Pnext;
    }

    if( item == NULL )
        return;

    nextitem    = item->Pnext;
    item->Pnext = NULL; // Set end of chain

    // Delete the extra items
    ClearUndoORRedoList( nextitem );
}
