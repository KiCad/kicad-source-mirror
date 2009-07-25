/****************************************************/
/*	BLOCK.CPP										*/
/* Gestion des Operations sur Blocks et Effacements */
/****************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "block_commande.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"
#include "class_marker_sch.h"


/* Variables Locales */

/* Fonctions exportees */
void               DeleteItemsInList( WinEDA_DrawPanel*  panel,
                                             PICKED_ITEMS_LIST& aItemsList );

/* Fonctions Locales */
static void               PlaceItemsInList( SCH_SCREEN* aScreen, PICKED_ITEMS_LIST& aItemsList );
static void               MoveListOfItems( SCH_SCREEN* aScreen, PICKED_ITEMS_LIST& aItemsList );
static void               CopyItemsInList( SCH_SCREEN* screen, PICKED_ITEMS_LIST& aItemsList );
static void               CollectStructsToDrag( SCH_SCREEN* screen );
static void               AddPickedItem( SCH_SCREEN* screen, wxPoint aPosition );
static LibEDA_BaseStruct* GetNextPinPosition( SCH_COMPONENT* aDrawLibItem,
                                              wxPoint&       aPosition );
static void               DrawMovingBlockOutlines( WinEDA_DrawPanel* panel,
                                                   wxDC*             DC,
                                                   bool              erase );
static void               SaveStructListForPaste( PICKED_ITEMS_LIST& aItemsList );

static void               MirrorListOfItems( PICKED_ITEMS_LIST& aItemsList, wxPoint& Center );
static void               MirrorOneStruct( SCH_ITEM* DrawStruct,
                                           wxPoint&  Center );

/*************************************************************************/
int WinEDA_SchematicFrame::ReturnBlockCommand( int key )
/*************************************************************************/

/* Return the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
 *  the key (ALT, SHIFT ALT ..)
 */
{
    int cmd;

    switch( key )
    {
    default:
        cmd = key & 0xFF;
        break;

    case 0:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_ALT:
    case GR_KB_SHIFT:
        cmd = BLOCK_COPY;
        break;

    case GR_KB_CTRL:
        cmd = BLOCK_DRAG;
        break;

    case GR_KB_SHIFTCTRL:
        cmd = BLOCK_DELETE;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


/*************************************************/
void WinEDA_SchematicFrame::InitBlockPasteInfos()
/*************************************************/

/* Init the parameters used by the block paste command
 */
{
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    block->m_ItemsSelection.CopyList( g_BlockSaveDataList.m_ItemsSelection );
    DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
}


/******************************************************/
void WinEDA_SchematicFrame::HandleBlockPlace( wxDC* DC )
/******************************************************/

/* Routine to handle the BLOCK PLACE commande
 *  Last routine for block operation for:
 *  - block move & drag
 *  - block copie & paste
 */
{
    bool            err   = FALSE;
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    if( DrawPanel->ManageCurseur == NULL )
    {
        err = TRUE;
        DisplayError( this, wxT( "HandleBlockPLace() : ManageCurseur = NULL" ) );
    }

    if( block->GetCount() == 0 )
    {
        wxString msg;
        err = TRUE;
        msg.Printf( wxT( "HandleBlockPLace() error : no items to place (cmd %d, state %d)" ),
                    block->m_Command, block->m_State );
        DisplayError( this, msg );
    }

    block->m_State = STATE_BLOCK_STOP;

    switch( block->m_Command )
    {
    case BLOCK_IDLE:
        err = TRUE;
        break;

    case BLOCK_DRAG:        /* Drag */
    case BLOCK_MOVE:        /* Move */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

        SaveCopyInUndoList( block->m_ItemsSelection, IS_CHANGED );

        MoveListOfItems( GetScreen(), block->m_ItemsSelection );
        block->ClearItemsList();
        break;

    case BLOCK_COPY:                /* Copy */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

        CopyItemsInList( GetScreen(), block->m_ItemsSelection );

        SaveCopyInUndoList(  block->m_ItemsSelection,
                             (block->m_Command == BLOCK_PRESELECT_MOVE) ? IS_CHANGED : IS_NEW );

        block->ClearItemsList();
        break;

    case BLOCK_PASTE:     /* Paste (recopie du dernier bloc sauve */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        PasteListOfItems( DC );
        block->ClearItemsList();
        break;

    case BLOCK_ZOOM:        // Handled by HandleBlockEnd()
    case BLOCK_DELETE:
    case BLOCK_SAVE:
    case BLOCK_ROTATE:
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
    case BLOCK_INVERT:
    case BLOCK_ABORT:
    case BLOCK_SELECT_ITEMS_ONLY:
        break;
    }

    GetScreen()->SetModify();

    /* clear struct.m_Flags  */
    SCH_ITEM* Struct;
    for( Struct = GetScreen()->EEDrawList; Struct != NULL; Struct = Struct->Next() )
        Struct->m_Flags = 0;

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    block->m_Flags   = 0;
    block->m_State   = STATE_NO_BLOCK;
    block->m_Command = BLOCK_IDLE;
    GetScreen()->SetCurItem( NULL );

    TestDanglingEnds( GetScreen()->EEDrawList, DC );

    if( block->GetCount() )
    {
        DisplayError( this, wxT( "HandleBlockPLace() error: some items left in buffer" ) );
        block->ClearItemsList();
    }

    SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor,
               wxEmptyString );
    DrawPanel->Refresh( );
}


/****************************************************/
int WinEDA_SchematicFrame::HandleBlockEnd( wxDC* DC )
/****************************************************/

/* Routine de gestion de la commande BLOCK END
 *  retourne :
 *  0 si aucun composant selectionne
 *  1 sinon
 *  -1 si commande terminee et composants trouves (block delete, block save)
 */
{
    int             ii = 0;
    bool            zoom_command = FALSE;
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    if( block->GetCount() )
    {
        BlockState   state   = block->m_State;
        CmdBlockType command = block->m_Command;
        if( DrawPanel->ForceCloseManageCurseur )
            DrawPanel->ForceCloseManageCurseur( DrawPanel, DC );
        block->m_State   = state;
        block->m_Command = command;
        DrawPanel->ManageCurseur = DrawAndSizingBlockOutlines;
        DrawPanel->ForceCloseManageCurseur = AbortBlockCurrentCommand;
        GetScreen()->m_Curseur = block->GetEnd();
        if( block->m_Command != BLOCK_ABORT )
            DrawPanel->MouseToCursorSchema();
    }

    if( DrawPanel->ManageCurseur != NULL )
        switch( block->m_Command )
        {
        case BLOCK_IDLE:
            DisplayError( this, wxT( "Error in HandleBlockPLace()" ) );
            break;

        case BLOCK_DRAG: /* Drag */
            BreakSegmentOnJunction( (SCH_SCREEN*) GetScreen() );

        case BLOCK_MOVE:    /* Move */
        case BLOCK_COPY:    /* Copy */
            PickItemsInBlock( GetScreen()->m_BlockLocate, GetScreen() );

        case BLOCK_PRESELECT_MOVE: /* Move with preselection list*/
            if( block->GetCount() )
            {
                ii = 1;
                CollectStructsToDrag( GetScreen() );
                DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
                DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
                DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
                block->m_State = STATE_BLOCK_MOVE;
            }
            else
            {
                DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
                DrawPanel->ManageCurseur = NULL;
                DrawPanel->ForceCloseManageCurseur = NULL;
            }
            break;

        case BLOCK_DELETE: /* Delete */
            PickItemsInBlock( GetScreen()->m_BlockLocate, GetScreen() );
            DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );
            if( block->GetCount() )
            {
                ii = -1;
                DeleteItemsInList( DrawPanel, block->m_ItemsSelection );
                GetScreen()->SetModify();
            }
            block->ClearItemsList();
            TestDanglingEnds( GetScreen()->EEDrawList, DC );
            DrawPanel->Refresh();
            break;

        case BLOCK_SAVE: /* Save */
            PickItemsInBlock( GetScreen()->m_BlockLocate, GetScreen() );
            DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );
            if( block->GetCount() )
            {
                wxPoint oldpos = GetScreen()->m_Curseur;
                GetScreen()->m_Curseur = wxPoint( 0, 0 );
                SaveStructListForPaste( block->m_ItemsSelection );
                PlaceItemsInList( GetScreen(), g_BlockSaveDataList.m_ItemsSelection );
                GetScreen()->m_Curseur = oldpos;
                ii = -1;
            }
            block->ClearItemsList();
            break;

        case BLOCK_PASTE:
            block->m_State = STATE_BLOCK_MOVE;
            break;

        case BLOCK_INVERT: /* pcbnew only! */
            break;

        case BLOCK_ROTATE:
        case BLOCK_MIRROR_X:
        case BLOCK_MIRROR_Y:
            break;

        case BLOCK_ZOOM: /* Window Zoom */
            zoom_command = TRUE;
            break;

        case BLOCK_SELECT_ITEMS_ONLY:   /* Not used */
        case BLOCK_ABORT:               /* not executed here */
            break;
        }

    if( block->m_Command  == BLOCK_ABORT )
    {
        /* clear struct.m_Flags  */
        EDA_BaseStruct* Struct;
        for( Struct = GetScreen()->EEDrawList; Struct != NULL; Struct = Struct->Next() )
            Struct->m_Flags = 0;
    }

    if( ii <= 0 )
    {
        block->m_Flags   = 0;
        block->m_State   = STATE_NO_BLOCK;
        block->m_Command = BLOCK_IDLE;
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        GetScreen()->SetCurItem( NULL );
        SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor, wxEmptyString );
    }

    if( zoom_command )
        Window_Zoom( GetScreen()->m_BlockLocate );

    return ii;
}


/***********************************************************************/
void WinEDA_SchematicFrame::HandleBlockEndByPopUp( int Command, wxDC* DC )
/***********************************************************************/

/* Routine de gestion de la commande BLOCK END by PopUp
 *  Appelee apres HandleBlockEnd.
 *  A partir de la commande bloc move, peut executer une commande autre que bloc move.
 */
{
    int             ii    = 0;
    BLOCK_SELECTOR* block = &GetScreen()->m_BlockLocate;

    if( block->m_Command != BLOCK_MOVE )
        return;
    if( Command == BLOCK_MOVE )
        return;

    block->m_Command = (CmdBlockType) Command;
    block->SetMessageBlock( this );

    switch( block->m_Command )
    {
    case BLOCK_COPY:     /* move to copy */
        block->m_State = STATE_BLOCK_MOVE;
        ii = 1;
        break;

    case BLOCK_DRAG:     /* move to Drag */

        /* Effacement de la liste des structures de pointage,
         *  qui est devenue erronnee */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        block->ClearItemsList();

        BreakSegmentOnJunction( GetScreen() );

        PickItemsInBlock( GetScreen()->m_BlockLocate, GetScreen() );
        if( block->GetCount() )
        {
            ii = 1;
            CollectStructsToDrag( (SCH_SCREEN*) GetScreen() );
            if( DrawPanel->ManageCurseur )
                DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
            block->m_State = STATE_BLOCK_MOVE;
        }
        break;

    case BLOCK_DELETE:     /* move to Delete */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        if( block->GetCount() )
        {
            ii = -1;
            DeleteItemsInList( DrawPanel, block->m_ItemsSelection );
            GetScreen()->SetModify();
        }
        TestDanglingEnds( GetScreen()->EEDrawList, DC );
        DrawPanel->Refresh();
        DrawPanel->Refresh();
        break;

    case BLOCK_SAVE:     /* Save */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        if( block->GetCount() )
        {
            wxPoint oldpos = GetScreen()->m_Curseur;
            GetScreen()->m_Curseur = wxPoint( 0, 0 );
            SaveStructListForPaste( block->m_ItemsSelection );
            PlaceItemsInList( GetScreen(), g_BlockSaveDataList.m_ItemsSelection );
            GetScreen()->m_Curseur = oldpos;
            ii = -1;
        }
        break;

    case BLOCK_ZOOM:     /* Window Zoom */
        DrawPanel->ForceCloseManageCurseur( DrawPanel, DC );
        DrawPanel->SetCursor(
            DrawPanel->m_PanelCursor = DrawPanel->m_PanelDefaultCursor );
        Window_Zoom( GetScreen()->m_BlockLocate );
        break;


    case BLOCK_ROTATE:
        break;

    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        if( block->GetCount() )
        {
            SaveCopyInUndoList( block->m_ItemsSelection, IS_CHANGED );

            ii = -1;
            /* Compute the mirror centre and put it on grid */
            wxPoint Center = block->Centre();
            PutOnGrid( &Center );
            MirrorListOfItems( block->m_ItemsSelection, Center );
            GetScreen()->SetModify();
        }
        TestDanglingEnds( GetScreen()->EEDrawList, DC );
        DrawPanel->Refresh();
        break;

    default:
        break;
    }

    if( ii <= 0 )
    {
        block->ClearItemsList();
        block->m_Flags   = 0;
        block->m_State   = STATE_NO_BLOCK;
        block->m_Command = BLOCK_IDLE;
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        GetScreen()->SetCurItem( NULL );
        SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor, wxEmptyString );
    }
}


/************************************************************************/
static void DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC,
                                     bool erase )
/************************************************************************/

/* Retrace le contour du block de recherche de structures
 *  L'ensemble du block suit le curseur
 */
{
    BLOCK_SELECTOR* block = &panel->GetScreen()->m_BlockLocate;;

    BASE_SCREEN*    screen = panel->GetScreen();
    SCH_ITEM*       schitem;

    /* Effacement ancien cadre */
    if( erase )
    {
        block->Draw( panel, DC, block->m_MoveVector, g_XorMode, block->m_Color );
        for( unsigned ii = 0; ii < block->GetCount(); ii++ )
        {
            schitem = (SCH_ITEM*) block->m_ItemsSelection.GetItemData( ii );
            DrawStructsInGhost( panel, DC, schitem,
                                block->m_MoveVector.x,
                                block->m_MoveVector.y );
        }
    }

    /* Redessin nouvel affichage */

    block->m_MoveVector = screen->m_Curseur - block->m_BlockLastCursorPosition;

    block->Draw( panel, DC, block->m_MoveVector, g_XorMode, block->m_Color );

    for( unsigned ii = 0; ii < block->GetCount(); ii++ )
    {
        schitem = (SCH_ITEM*) block->m_ItemsSelection.GetItemData( ii );
        DrawStructsInGhost( panel, DC, schitem,
                            block->m_MoveVector.x,
                            block->m_MoveVector.y );
    }
}


/*****************************************************************************
* Routine to move objects to a new position.							 *
*****************************************************************************/
void MoveListOfItems( SCH_SCREEN* aScreen, PICKED_ITEMS_LIST& aItemsList )
{
    PlaceItemsInList( aScreen, aItemsList );        /* Place it in its new position. */
}


static void MirrorYPoint( wxPoint& point, wxPoint& Center )
{
    point.x -= Center.x;
    NEGATE( point.x );
    point.x += Center.x;
}


/**************************************************************/
void MirrorOneStruct( SCH_ITEM* DrawStruct, wxPoint& Center )
/**************************************************************/

/* Given a structure rotate it to 90 degrees refer to the Center point.
 */
{
    int dx;
    DrawPolylineStruct*            DrawPoly;
    DrawJunctionStruct*            DrawConnect;
    EDA_DrawLineStruct*            DrawSegment;
    DrawBusEntryStruct*            DrawRaccord;
    SCH_COMPONENT*                 DrawLibItem;
    DrawSheetStruct*               DrawSheet;
    Hierarchical_PIN_Sheet_Struct* DrawSheetLabel;
    MARKER_SCH* DrawMarker;
    DrawNoConnectStruct*           DrawNoConnect;
    SCH_TEXT* DrawText;
    wxPoint px;
    WinEDA_SchematicFrame*         frame;

    if( !DrawStruct )
        return;

    frame = (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();

    switch( DrawStruct->Type() )
    {
    case TYPE_NOT_INIT:
        break;

    case DRAW_POLYLINE_STRUCT_TYPE:
        DrawPoly = (DrawPolylineStruct*) DrawStruct;
        for( unsigned ii = 0; ii < DrawPoly->GetCornerCount(); ii++ )
        {
            wxPoint point;
            point = DrawPoly->m_PolyPoints[ii];
            MirrorYPoint( point, Center );
            DrawPoly->m_PolyPoints[ii] = point;
        }

        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        DrawSegment = (EDA_DrawLineStruct*) DrawStruct;
        if( (DrawSegment->m_Flags & STARTPOINT) == 0 )
        {
            MirrorYPoint( DrawSegment->m_Start, Center );
        }
        if( (DrawSegment->m_Flags & ENDPOINT) == 0 )
        {
            MirrorYPoint( DrawSegment->m_End, Center );
        }
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        DrawRaccord = (DrawBusEntryStruct*) DrawStruct;
        MirrorYPoint( DrawRaccord->m_Pos, Center );
        NEGATE( DrawRaccord->m_Size.x );
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        DrawConnect = (DrawJunctionStruct*) DrawStruct;
        MirrorYPoint( DrawConnect->m_Pos, Center );
        break;

    case DRAW_MARKER_STRUCT_TYPE:
        DrawMarker = (MARKER_SCH*) DrawStruct;
        MirrorYPoint( DrawMarker->m_Pos, Center );
        break;

    case DRAW_NOCONNECT_STRUCT_TYPE:
        DrawNoConnect = (DrawNoConnectStruct*) DrawStruct;
        MirrorYPoint( DrawNoConnect->m_Pos, Center );
        break;

    case TYPE_SCH_TEXT:
    case TYPE_SCH_LABEL:

        // Text is not really mirrored; it is moved to a suitable position
        // which is the closest position for a true mirrored text
        // The center position is mirrored and the text is moved for half horizontal len
        DrawText = (SCH_TEXT*) DrawStruct;
        px = DrawText->m_Pos;
        if( DrawText->m_Orient == 0 )       /* horizontal text */
            dx = DrawText->LenSize( DrawText->m_Text ) / 2;
        else if( DrawText->m_Orient == 2 )  /* invert horizontal text*/
            dx = -DrawText->LenSize( DrawText->m_Text ) / 2;
        else
            dx = 0;
        px.x += dx;
        MirrorYPoint( px, Center );
        px.x -= dx;

        frame->PutOnGrid( &px );
        DrawText->m_Pos.x = px.x;
        break;

    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_GLOBALLABEL:

        // Text is not really mirrored: Orientation is changed
        DrawText = (SCH_LABEL*) DrawStruct;
        if( DrawText->m_Orient == 0 )       /* horizontal text */
            DrawText->m_Orient = 2;
        else if( DrawText->m_Orient == 2 )  /* invert horizontal text*/
            DrawText->m_Orient = 0;

        px = DrawText->m_Pos;
        MirrorYPoint( px, Center );
        frame->PutOnGrid( &px );
        DrawText->m_Pos.x = px.x;
        break;

    case TYPE_SCH_COMPONENT:
        DrawLibItem = (SCH_COMPONENT*) DrawStruct;
        dx = DrawLibItem->m_Pos.x;
        frame->CmpRotationMiroir( DrawLibItem, NULL, CMP_MIROIR_Y );
        MirrorYPoint( DrawLibItem->m_Pos, Center );
        dx -= DrawLibItem->m_Pos.x;

        for( int ii = 0; ii < DrawLibItem->GetFieldCount(); ii++ )
        {
            /* move the fields to the new position because the component itself has moved */
            DrawLibItem->GetField( ii )->m_Pos.x -= dx;
        }

        break;

    case DRAW_SHEET_STRUCT_TYPE:
        DrawSheet = (DrawSheetStruct*) DrawStruct;
        MirrorYPoint( DrawSheet->m_Pos, Center );
        DrawSheet->m_Pos.x -= DrawSheet->m_Size.x;

        DrawSheetLabel = DrawSheet->m_Label;
        while( DrawSheetLabel != NULL )
        {
            MirrorYPoint( DrawSheetLabel->m_Pos, Center );
            DrawSheetLabel->m_Edge = DrawSheetLabel->m_Edge ? 0 : 1;
            DrawSheetLabel =
                (Hierarchical_PIN_Sheet_Struct*) DrawSheetLabel->Next();
        }

        break;

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        DrawSheetLabel = (Hierarchical_PIN_Sheet_Struct*) DrawStruct;
        MirrorYPoint( DrawSheetLabel->m_Pos, Center );
        break;

    default:
        break;
    }
}


/*****************************************************************************
* Routine to Mirror objects.							 *
*****************************************************************************/
void MirrorListOfItems( PICKED_ITEMS_LIST& aItemsList, wxPoint& Center )
{
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetItemData( ii );
        MirrorOneStruct( item, Center );      // Place it in its new position.
        item->m_Flags = 0;
    }
}


/*****************************************************************************/
void CopyItemsInList( SCH_SCREEN* screen, PICKED_ITEMS_LIST& aItemsList )
/*****************************************************************************/

/* Routine to copy a new entity of an object for each object in list and reposition it.
 * Return the new created object list in aItemsList
 */
{
    SCH_ITEM* newitem;

    if( aItemsList.GetCount() == 0 )
        return;

    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        newitem = DuplicateStruct( (SCH_ITEM*) aItemsList.GetItemData( ii ) );
        aItemsList.SetItem( newitem, ii );
        aItemsList.SetItemStatus( IS_NEW, ii );
        {
            switch( newitem->Type() )
            {
            case DRAW_POLYLINE_STRUCT_TYPE:
            case DRAW_JUNCTION_STRUCT_TYPE:
            case DRAW_SEGMENT_STRUCT_TYPE:
            case DRAW_BUSENTRY_STRUCT_TYPE:
            case TYPE_SCH_TEXT:
            case TYPE_SCH_LABEL:
            case TYPE_SCH_GLOBALLABEL:
            case TYPE_SCH_HIERLABEL:
            case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
            case DRAW_MARKER_STRUCT_TYPE:
            case DRAW_NOCONNECT_STRUCT_TYPE:
            default:
                break;

            case DRAW_SHEET_STRUCT_TYPE:
            {
                DrawSheetStruct* sheet = (DrawSheetStruct*) newitem;
                sheet->m_TimeStamp = GetTimeStamp();
                sheet->SetSon( NULL );
                break;
            }

            case TYPE_SCH_COMPONENT:
                ( (SCH_COMPONENT*) newitem )->m_TimeStamp = GetTimeStamp();
                ( (SCH_COMPONENT*) newitem )->ClearAnnotation( NULL );
                break;
            }

            SetaParent( newitem, screen );
            newitem->SetNext( screen->EEDrawList );
            screen->EEDrawList = newitem;
        }
    }

    PlaceItemsInList( screen, aItemsList );
}


/*********************************************************************************/
void DeleteStruct( WinEDA_DrawPanel* panel, wxDC* DC, SCH_ITEM* DrawStruct )
/*********************************************************************************/

/* Routine to delete an object from global drawing object list.
 *  Object is put in Undo list
 */
{
    SCH_SCREEN*            screen = (SCH_SCREEN*) panel->GetScreen();
    WinEDA_SchematicFrame* frame  = (WinEDA_SchematicFrame*) panel->m_Parent;

    if( !DrawStruct )
        return;

    if( DrawStruct->Type() == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
    {
        /* Cette stucture est rattachee a une feuille, et n'est pas
         *  accessible par la liste globale directement */
        frame->SaveCopyInUndoList( (SCH_ITEM*)( (Hierarchical_PIN_Sheet_Struct
                                                 *) DrawStruct )->GetParent(),
                                  IS_CHANGED );
        frame->DeleteSheetLabel( DC ? true : false,
                                 (Hierarchical_PIN_Sheet_Struct*) DrawStruct );
        return;
    }

    else    /* structure classique */
    {
        screen->RemoveFromDrawList( DrawStruct );

        panel->PostDirtyRect( DrawStruct->GetBoundingBox() );

        /* Unlink the structure */
        DrawStruct->SetNext( 0 );
        DrawStruct->SetBack( 0 );  // Only one struct -> no link

        frame->SaveCopyInUndoList( DrawStruct, IS_DELETED );
    }
}


/*****************************************************************/
void SaveStructListForPaste( PICKED_ITEMS_LIST& aItemsList )
/*****************************************************************/

/* Routine to Save an object from global drawing object list.
 *  This routine is the same as delete but:
 *  - the original list is NOT removed.
 *  - List is saved in g_BlockSaveDataList
 */
{
    g_BlockSaveDataList.ClearListAndDeleteItems();      // delete previous saved list, if exists

    /* save the new list: */
    ITEM_PICKER item;
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        /* Make a copy of the original picked item. */
        SCH_ITEM* DrawStructCopy = DuplicateStruct( (SCH_ITEM*) aItemsList.GetItemData( ii ) );
        DrawStructCopy->SetParent( NULL );
        item.m_Item = DrawStructCopy;
        g_BlockSaveDataList.PushItem( item );
    }
}


/*****************************************************************************
* Routine to paste a structure from the g_BlockSaveDataList stack.
*	This routine is the same as undelete but original list is NOT removed.
*****************************************************************************/
void WinEDA_SchematicFrame::PasteListOfItems( wxDC* DC )
{
    SCH_ITEM* Struct;

    if( g_BlockSaveDataList.GetCount() == 0 )
    {
        DisplayError( this, wxT( "No struct to paste" ) );
        return;
    }

    PICKED_ITEMS_LIST picklist;
    picklist.m_UndoRedoType = IS_NEW;

    // Creates data, and push it as new data in undo item list buffer
    ITEM_PICKER picker( NULL, IS_NEW );
    for( unsigned ii = 0; ii < g_BlockSaveDataList.GetCount(); ii++ )
    {
        Struct = DuplicateStruct( (SCH_ITEM*) g_BlockSaveDataList.m_ItemsSelection.GetItemData( ii ) );
        picker.m_Item = Struct;
        picklist.PushItem( picker );

        // Clear annotation and init new time stamp for the new components:
        if( Struct->Type() == TYPE_SCH_COMPONENT )
        {
            ( (SCH_COMPONENT*) Struct )->m_TimeStamp = GetTimeStamp();
            ( (SCH_COMPONENT*) Struct )->ClearAnnotation( NULL );
        }
        SetaParent( Struct, GetScreen() );
        RedrawOneStruct( DrawPanel, DC, Struct, GR_DEFAULT_DRAWMODE );
        Struct->SetNext( GetScreen()->EEDrawList );
        GetScreen()->EEDrawList = Struct;
    }

    SaveCopyInUndoList( picklist, IS_NEW );

    PlaceItemsInList( GetScreen(), picklist );

    /* clear .m_Flags member for all items */
    for( Struct = GetScreen()->EEDrawList; Struct != NULL; Struct = Struct->Next() )
        Struct->m_Flags = 0;

    GetScreen()->SetModify();

    return;
}


/** function DeleteItemsInList
 * delete schematic items in aItemsList
 * deleted items are put in undo list
 */
void DeleteItemsInList( WinEDA_DrawPanel* panel, PICKED_ITEMS_LIST& aItemsList )
{
    SCH_SCREEN*            screen = (SCH_SCREEN*) panel->GetScreen();
    WinEDA_SchematicFrame* frame  = (WinEDA_SchematicFrame*) panel->m_Parent;
    PICKED_ITEMS_LIST      itemsList;
    ITEM_PICKER            itemWrapper;

    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetItemData( ii );
        itemWrapper.m_Item = item;
        itemWrapper.m_UndoRedoStatus = IS_DELETED;
        if( item->Type() == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
        {
            /* this item is depending on a sheet, and is not in global list */
            wxMessageBox( wxT(
                             "DeleteItemsInList() err: unexpected DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE" ) );
#if 0
            Hierarchical_PIN_Sheet_Struct* pinlabel = (Hierarchical_PIN_Sheet_Struct*) item;
            frame->DeleteSheetLabel( false, pinlabel->m_Parent );
            itemWrapper.m_Item = pinlabel->m_Parent;
            itemWrapper.m_UndoRedoStatus = IS_CHANGED;
            itemsList.PushItem( itemWrapper );
#endif
        }
        else
        {
            screen->RemoveFromDrawList( item );

            /* Unlink the structure */
            item->SetNext( 0 );
            item->SetBack( 0 );
            itemsList.PushItem( itemWrapper );
        }
    }

    frame->SaveCopyInUndoList( itemsList, IS_DELETED );
}


/*****************************************************************************
* Routine to place a given object.											 *
*****************************************************************************/
void PlaceItemsInList( SCH_SCREEN* aScreen, PICKED_ITEMS_LIST& aItemsList )
{
    wxPoint move_vector;

    move_vector = aScreen->m_Curseur - aScreen->m_BlockLocate.m_BlockLastCursorPosition;
    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) aItemsList.GetItemData( ii );
        MoveOneStruct( item, move_vector );
    }
}


/**************************************************************************/
void MoveOneStruct( SCH_ITEM* DrawStruct, const wxPoint& move_vector )
/*************************************************************************/

/* Given a structure move it by Dx, Dy.
 */
{
    DrawPolylineStruct*            DrawPoly;
    DrawJunctionStruct*            DrawConnect;
    EDA_DrawLineStruct*            DrawSegment;
    DrawBusEntryStruct*            DrawRaccord;
    SCH_COMPONENT*                 DrawLibItem;
    DrawSheetStruct*               DrawSheet;
    Hierarchical_PIN_Sheet_Struct* DrawSheetLabel;
    MARKER_SCH* DrawMarker;
    DrawNoConnectStruct*           DrawNoConnect;

    if( !DrawStruct )
        return;

    switch( DrawStruct->Type() )
    {
    case TYPE_NOT_INIT:
        break;

    case DRAW_POLYLINE_STRUCT_TYPE:
        DrawPoly = (DrawPolylineStruct*) DrawStruct;
        for( unsigned ii = 0; ii < DrawPoly->GetCornerCount(); ii++ )
        {
            DrawPoly->m_PolyPoints[ii] += move_vector;
        }

        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        DrawSegment = (EDA_DrawLineStruct*) DrawStruct;
        if( (DrawSegment->m_Flags & STARTPOINT) == 0 )
        {
            DrawSegment->m_Start += move_vector;
        }
        if( (DrawSegment->m_Flags & ENDPOINT) == 0 )
        {
            DrawSegment->m_End += move_vector;
        }
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        DrawRaccord = (DrawBusEntryStruct*) DrawStruct;
        DrawRaccord->m_Pos += move_vector;
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        DrawConnect = (DrawJunctionStruct*) DrawStruct;
        DrawConnect->m_Pos += move_vector;
        break;

    case DRAW_MARKER_STRUCT_TYPE:
        DrawMarker = (MARKER_SCH*) DrawStruct;
        DrawMarker->m_Pos += move_vector;
        break;

    case DRAW_NOCONNECT_STRUCT_TYPE:
        DrawNoConnect = (DrawNoConnectStruct*) DrawStruct;
        DrawNoConnect->m_Pos += move_vector;
        break;

    case TYPE_SCH_TEXT:
             #define DrawText ( (SCH_TEXT*) DrawStruct )
        DrawText->m_Pos += move_vector;
        break;

    case TYPE_SCH_LABEL:
             #define DrawLabel ( (SCH_LABEL*) DrawStruct )
        DrawLabel->m_Pos += move_vector;
        break;

    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_GLOBALLABEL:
             #define DrawGHLabel ( (SCH_LABEL*) DrawStruct )
        DrawGHLabel->m_Pos += move_vector;
        break;

    case TYPE_SCH_COMPONENT:
        DrawLibItem = (SCH_COMPONENT*) DrawStruct;
        DrawLibItem->m_Pos += move_vector;
        for( int ii = 0; ii < DrawLibItem->GetFieldCount(); ii++ )
        {
            DrawLibItem->GetField( ii )->m_Pos += move_vector;
        }

        break;

    case DRAW_SHEET_STRUCT_TYPE:
        DrawSheet = (DrawSheetStruct*) DrawStruct;
        DrawSheet->m_Pos += move_vector;
        DrawSheetLabel    = DrawSheet->m_Label;
        while( DrawSheetLabel != NULL )
        {
            DrawSheetLabel->m_Pos += move_vector;
            DrawSheetLabel = DrawSheetLabel->Next();
        }

        break;

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        DrawSheetLabel = (Hierarchical_PIN_Sheet_Struct*) DrawStruct;
        DrawSheetLabel->m_Pos += move_vector;
        break;

    default:
        break;
    }
}


/************************************************************/
SCH_ITEM* DuplicateStruct( SCH_ITEM* DrawStruct )
/************************************************************/

/* Routine to create a new copy of given struct.
 *  The new object is not put in draw list (not linked)
 */
{
    SCH_ITEM* NewDrawStruct = NULL;

    if( DrawStruct == NULL )
    {
        DisplayError( NULL, wxT( "DuplicateStruct error: NULL struct" ) );
        return NULL;
    }

    switch( DrawStruct->Type() )
    {
    case DRAW_POLYLINE_STRUCT_TYPE:
        NewDrawStruct = ( (DrawPolylineStruct*) DrawStruct )->GenCopy();
        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        NewDrawStruct = ( (EDA_DrawLineStruct*) DrawStruct )->GenCopy();
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        NewDrawStruct = ( (DrawBusEntryStruct*) DrawStruct )->GenCopy();
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        NewDrawStruct = ( (DrawJunctionStruct*) DrawStruct )->GenCopy();
        break;

    case DRAW_MARKER_STRUCT_TYPE:
        NewDrawStruct = ( (MARKER_SCH*) DrawStruct )->GenCopy();
        break;

    case DRAW_NOCONNECT_STRUCT_TYPE:
        NewDrawStruct = ( (DrawNoConnectStruct*) DrawStruct )->GenCopy();
        break;

    case TYPE_SCH_TEXT:
        NewDrawStruct = ( (SCH_TEXT*) DrawStruct )->GenCopy();
        break;

    case TYPE_SCH_LABEL:
        NewDrawStruct = ( (SCH_LABEL*) DrawStruct )->GenCopy();
        break;

    case TYPE_SCH_HIERLABEL:
        NewDrawStruct = ( (SCH_HIERLABEL*) DrawStruct )->GenCopy();
        break;

    case TYPE_SCH_GLOBALLABEL:
        NewDrawStruct = ( (SCH_GLOBALLABEL*) DrawStruct )->GenCopy();
        break;

    case TYPE_SCH_COMPONENT:
        NewDrawStruct = ( (SCH_COMPONENT*) DrawStruct )->GenCopy();
        break;

    case DRAW_SHEET_STRUCT_TYPE:
        NewDrawStruct = ( (DrawSheetStruct*) DrawStruct )->GenCopy();
        break;

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
    case DRAW_PART_TEXT_STRUCT_TYPE:
    case SCREEN_STRUCT_TYPE:
    default:
    {
        wxString msg;
        msg << wxT( "DuplicateStruct error: unexpected StructType " ) <<
        DrawStruct->Type() << wxT( " " ) << DrawStruct->GetClass();
        DisplayError( NULL, msg );
    }
    break;
    }

    NewDrawStruct->m_Image = DrawStruct;
    return NewDrawStruct;
}


/****************************************************/
static void CollectStructsToDrag( SCH_SCREEN* screen )
/****************************************************/

/* creates the list of items found when a drag block is initiated.
 * items are those selected in window block an some items outside this area but connected
 * to a selected item (connected wires to a component or an entry )
 */
{
    SCH_ITEM*           Struct;
    EDA_DrawLineStruct* SegmStruct;
    int ox, oy, fx, fy;

    PICKED_ITEMS_LIST*  pickedlist = &screen->m_BlockLocate.m_ItemsSelection;

    if( pickedlist->GetCount() == 0 )
        return;

    /* .m_Flags member is used to handle how a wire is exactly selected
     * (fully selected, or partially selected by an end point )
     */
    for( Struct = screen->EEDrawList; Struct != NULL; Struct = Struct->Next() )
        Struct->m_Flags = 0;

    // Sel .m_Flags to selected for a wire or bus in selected area if there is only one item:
    if( pickedlist->GetCount() == 1 )
    {
        Struct = (SCH_ITEM*) pickedlist->GetItemData( 0 );
        if( Struct->Type() == DRAW_SEGMENT_STRUCT_TYPE )
            Struct->m_Flags = SELECTED;
    }
    // Sel .m_Flags to selected for a wire or bus in selected area for a list of items:
    else
    {
        for( unsigned ii = 0; ii < pickedlist->GetCount(); ii++ )
        {
            Struct = (SCH_ITEM*)(SCH_ITEM*) pickedlist->GetItemData( ii );
            Struct->m_Flags = SELECTED;
        }
    }

    if( screen->m_BlockLocate.m_Command != BLOCK_DRAG )
        return;

    ox = screen->m_BlockLocate.GetX();
    oy = screen->m_BlockLocate.GetY();
    fx = screen->m_BlockLocate.GetRight();
    fy = screen->m_BlockLocate.GetBottom();

    if( fx < ox )
        EXCHG( fx, ox );
    if( fy < oy )
        EXCHG( fy, oy );


    /* Suppression du deplacement des extremites de segments hors cadre
     *  de selection */
    for( unsigned ii = 0; ii < pickedlist->GetCount(); ii++ )
    {
        Struct = (SCH_ITEM*)(SCH_ITEM*) pickedlist->GetItemData( ii );
        if( Struct->Type() == DRAW_SEGMENT_STRUCT_TYPE )
        {
            SegmStruct = (EDA_DrawLineStruct*) Struct;
            if( (SegmStruct->m_Start.x < ox) || (SegmStruct->m_Start.x > fx)
               || (SegmStruct->m_Start.y < oy) || (SegmStruct->m_Start.y > fy) )
                SegmStruct->m_Flags |= STARTPOINT;

            if( (SegmStruct->m_End.x < ox) || (SegmStruct->m_End.x > fx)
               || (SegmStruct->m_End.y < oy) || (SegmStruct->m_End.y > fy) )
                SegmStruct->m_Flags |= ENDPOINT;
        }
    }

    /* Search for other items to drag. They are end wires connected to selected items
     */

    for( unsigned ii = 0; ii < pickedlist->GetCount(); ii++ )
    {
        Struct = (SCH_ITEM*)(SCH_ITEM*) pickedlist->GetItemData( ii );
        if( Struct->Type() == TYPE_SCH_COMPONENT )
        {
            // Add all pins of the selected component to list
            LibEDA_BaseStruct* DrawItem;
            wxPoint            pos;
            DrawItem = GetNextPinPosition( (SCH_COMPONENT*) Struct, pos );
            while( DrawItem )
            {
                if( (pos.x < ox) || (pos.x > fx) || (pos.y < oy) || (pos.y > fy) )
                {
                    // This pin is outside area,
                    // but because it it the pin of a selected component
                    // we must also select connected items to this pin
                    AddPickedItem( screen, pos );
                }

                DrawItem = GetNextPinPosition( NULL, pos );
            }
        }

        if( Struct->Type() == DRAW_SHEET_STRUCT_TYPE )
        {
            // Add all pins sheets of a selected hierarchical sheet to the list
            Hierarchical_PIN_Sheet_Struct* SLabel =
                ( (DrawSheetStruct*) Struct )->m_Label;
            while( SLabel )
            {
                if( SLabel->Type() == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
                    AddPickedItem( screen, SLabel->m_Pos );
                SLabel = (Hierarchical_PIN_Sheet_Struct*) SLabel->Next();
            }
        }

        if( Struct->Type() == DRAW_BUSENTRY_STRUCT_TYPE )
        {
            DrawBusEntryStruct* item = (DrawBusEntryStruct*) Struct;
            AddPickedItem( screen, item->m_Pos );
            AddPickedItem( screen, item->m_End() );
        }
    }
}


/******************************************************************/
static void AddPickedItem( SCH_SCREEN* screen, wxPoint position )
/******************************************************************/

/** AddPickedItem
 * add to the picked list in screen->m_BlockLocate items found at location position
 * @param screen = the screen to consider
 * @param position = the wxPoint where items must be located to be select
 */
{
    SCH_ITEM*          Struct;

    /* Examen de la liste des elements deja selectionnes */
    PICKED_ITEMS_LIST* pickedlist = &screen->m_BlockLocate.m_ItemsSelection;

    for( unsigned ii = 0; ii < pickedlist->GetCount(); ii++ )
    {
        Struct = (SCH_ITEM*) pickedlist->GetItemData( ii );

        switch( Struct->Type() )
        {
        case DRAW_SEGMENT_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (EDA_DrawLineStruct*) Struct )
            if( STRUCT->m_Start == position )
                STRUCT->m_Flags &= ~STARTPOINT;

            if( STRUCT->m_End == position )
                STRUCT->m_Flags &= ~ENDPOINT;
            break;

        default:
            break;
        }
    }

    /* Examen de la liste des elements non selectionnes */

    ITEM_PICKER picker;
    Struct = screen->EEDrawList;
    while( Struct )
    {
        switch( Struct->Type() )
        {
        case TYPE_NOT_INIT:
            break;

        case DRAW_POLYLINE_STRUCT_TYPE:
            if( Struct->m_Flags & SELECTED )
                break; /* Deja en liste */
            break;

        case DRAW_JUNCTION_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (DrawJunctionStruct*) Struct )
            if( Struct->m_Flags & SELECTED )
                break; /* Deja en liste */
            if( STRUCT->m_Pos != position )
                break;
            picker.m_Item = Struct;
            pickedlist->PushItem( picker );
            break;

        case DRAW_SEGMENT_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (EDA_DrawLineStruct*) Struct )
            if( Struct->m_Flags & SELECTED )
                break; /* Deja en liste */
            if( STRUCT->m_Start == position )
            {
                Struct->m_Flags  = SELECTED | ENDPOINT | STARTPOINT;
                Struct->m_Flags &= ~STARTPOINT;
                picker.m_Item    = Struct;
                pickedlist->PushItem( picker );
            }
            else if( STRUCT->m_End == position )
            {
                Struct->m_Flags  = SELECTED | ENDPOINT | STARTPOINT;
                Struct->m_Flags &= ~ENDPOINT;
                picker.m_Item    = Struct;
                pickedlist->PushItem( picker );
            }
            break;

        case DRAW_BUSENTRY_STRUCT_TYPE:
            break;

        case TYPE_SCH_TEXT:
            break;

        case TYPE_SCH_LABEL:
                #undef STRUCT
                #define STRUCT ( (SCH_LABEL*) Struct )
            if( Struct->m_Flags & SELECTED )
                break; /* Already in list */
            if( STRUCT->m_Pos != position )
                break;
            Struct->m_Flags |= SELECTED;
            picker.m_Item    = Struct;
            pickedlist->PushItem( picker );
            break;

        case TYPE_SCH_HIERLABEL:
        case TYPE_SCH_GLOBALLABEL:
                #undef STRUCT
                #define STRUCT ( (SCH_LABEL*) Struct )
            if( Struct->m_Flags & SELECTED )
                break; /* Already in list */
            if( STRUCT->m_Pos != position )
                break;
            Struct->m_Flags |= SELECTED;
            picker.m_Item    = Struct;
            pickedlist->PushItem( picker );
            break;

        case TYPE_SCH_COMPONENT:
        case DRAW_SHEET_STRUCT_TYPE:
        case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
            break;

        case DRAW_MARKER_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (MARKER_SCH*) Struct )
            if( Struct->m_Flags & SELECTED )
                break; /* Already in list */
            if( STRUCT->m_Pos != position )
                break;
            Struct->m_Flags |= SELECTED;
            picker.m_Item    = Struct;
            pickedlist->PushItem( picker );
            break;

        case DRAW_NOCONNECT_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (DrawNoConnectStruct*) Struct )
            if( Struct->m_Flags & SELECTED )
                break; /* Already in list */
            if( STRUCT->m_Pos != position )
                break;
            Struct->m_Flags |= SELECTED;
            picker.m_Item    = Struct;
            pickedlist->PushItem( picker );
            break;

        default:
            break;
        }
        Struct = Struct->Next();
    }
}


/*********************************************************************************/
static LibEDA_BaseStruct* GetNextPinPosition( SCH_COMPONENT* aDrawLibItem,
                                              wxPoint&       aPosition )
/*********************************************************************************/

/** GetNextPinPosition()
 * calculate position of the "next" pin of the aDrawLibItem component
 * if aDrawLibItem is non null : search for the first pin
 * if aDrawLibItem == NULL, search the next pin
 * returns its position
 * @param aDrawLibItem = component test. search for the first pin
 *      if NULL, serach for the next pin for each call
 * @param aPosition = the calculated pin position, according to the component orientation and position
 * @return a pointer to the pin
 */
{
    EDA_LibComponentStruct* Entry;
    static LibEDA_BaseStruct* NextItem;
    static int Multi, convert, TransMat[2][2];
    LibEDA_BaseStruct* DEntry;
    int orient;
    LibDrawPin* Pin;
    static wxPoint CmpPosition;

    if( aDrawLibItem )
    {
        NextItem = NULL;
        if( ( Entry =
                 FindLibPart( aDrawLibItem->m_ChipName.GetData(), wxEmptyString,
                              FIND_ROOT ) ) == NULL )
            return NULL;
        DEntry      = Entry->m_Drawings;
        Multi       = aDrawLibItem->m_Multi;
        convert     = aDrawLibItem->m_Convert;
        CmpPosition = aDrawLibItem->m_Pos;
        memcpy( TransMat, aDrawLibItem->m_Transform, sizeof(TransMat) );
    }
    else
        DEntry = NextItem;

    for( ; DEntry != NULL; DEntry = DEntry->Next() )
    {
        /* Elimination des elements non relatifs a l'unite */
        if( Multi && DEntry->m_Unit && (DEntry->m_Unit != Multi) )
            continue;
        if( convert && DEntry->m_Convert && (DEntry->m_Convert != convert) )
            continue;
        if( DEntry->Type() != COMPONENT_PIN_DRAW_TYPE )
            continue;

        Pin = (LibDrawPin*) DEntry;

        /* Calcul de l'orientation reelle de la Pin */
        orient = Pin->ReturnPinDrawOrient( TransMat );

        /* Calcul de la position du point de reference */
        aPosition = TransformCoordinate( TransMat, Pin->m_Pos ) + CmpPosition;
        NextItem  = DEntry->Next();
        return DEntry;
    }

    NextItem = NULL;
    return NULL;
}
