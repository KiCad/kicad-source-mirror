/****************************************************/
/*	BLOCK.CPP										*/
/* Gestion des Operations sur Blocks et Effacements */
/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/* Variables Locales */

/* Fonctions exportees */

/* Fonctions Locales */
static SCH_ITEM*          CopyStruct( WinEDA_DrawPanel* panel,
                                      wxDC*             DC,
                                      BASE_SCREEN*      screen,
                                      SCH_ITEM*         DrawStruct );
static void               CollectStructsToDrag( SCH_SCREEN* screen );
static void               AddPickedItem( SCH_SCREEN* screen, wxPoint aPosition );
static LibEDA_BaseStruct* GetNextPinPosition( SCH_COMPONENT* aDrawLibItem,
                                              wxPoint&       aPosition );
static void               DrawMovingBlockOutlines( WinEDA_DrawPanel* panel,
                                                   wxDC*             DC,
                                                   bool              erase );
static SCH_ITEM*          SaveStructListForPaste( SCH_ITEM* DrawStruct );
static bool               MirrorStruct( WinEDA_DrawPanel* panel, wxDC* DC,
                                        SCH_ITEM* DrawStruct, wxPoint& Center );
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
    DrawBlockStruct* block = &GetScreen()->BlockLocate;

    block->m_BlockDrawStruct = g_BlockSaveDataList;
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
    bool             err   = FALSE;
    DrawBlockStruct* block = &GetScreen()->BlockLocate;

    SCH_ITEM*        NewStruct = NULL;

    if( DrawPanel->ManageCurseur == NULL )
    {
        err = TRUE;
        DisplayError( this, wxT( "HandleBlockPLace() : ManageCurseur = NULL" ) );
    }

    if( block->m_BlockDrawStruct == NULL )
    {
        wxString msg;
        err = TRUE;
        msg.Printf( wxT( "HandleBlockPLace() : m_BlockDrawStruct = " \
                         "NULL (cmd %d, state %d)" ),
                    block->m_Command, block->m_State );
        DisplayError( this, msg );
    }

    block->m_State = STATE_BLOCK_STOP;

    switch( block->m_Command )
    {
    case  BLOCK_IDLE:
        err = TRUE;
        break;

    case BLOCK_DRAG:        /* Drag */
    case BLOCK_MOVE:        /* Move */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

        SaveCopyInUndoList( (SCH_ITEM*) block->m_BlockDrawStruct, IS_CHANGED );

        MoveStruct( DrawPanel, DC, (SCH_ITEM*) block->m_BlockDrawStruct );
        block->m_BlockDrawStruct = NULL;
        DrawPanel->Refresh( TRUE );
        break;

    case BLOCK_COPY:                /* Copy */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

        NewStruct = CopyStruct( DrawPanel, DC,
                                GetScreen(),
                                (SCH_ITEM*) block->m_BlockDrawStruct );

        SaveCopyInUndoList(
            NewStruct,
            (block->m_Command ==
             BLOCK_PRESELECT_MOVE) ? IS_CHANGED : IS_NEW );

        block->m_BlockDrawStruct = NULL;
        break;

    case BLOCK_PASTE:     /* Paste (recopie du dernier bloc sauve */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        PasteStruct( DC );
        block->m_BlockDrawStruct = NULL;
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

    if( block->m_BlockDrawStruct )
    {
        DisplayError( this,
                     wxT( "HandleBlockPLace() error: DrawStruct != Null" ) );
        block->m_BlockDrawStruct = NULL;
    }

    SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor, wxEmptyString );
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
    int ii = 0;
    bool             zoom_command = FALSE;
    DrawBlockStruct* block = &GetScreen()->BlockLocate;

    if( block->m_BlockDrawStruct )
    {
        BlockState   state   = block->m_State;
        CmdBlockType command = block->m_Command;
        if( DrawPanel->ForceCloseManageCurseur )
            DrawPanel->ForceCloseManageCurseur( DrawPanel, DC );
        block->m_State   = state;
        block->m_Command = command;
        DrawPanel->ManageCurseur = DrawAndSizingBlockOutlines;
        DrawPanel->ForceCloseManageCurseur = AbortBlockCurrentCommand;
        GetScreen()->m_Curseur.x = block->GetRight();
        GetScreen()->m_Curseur.y = block->GetBottom();
        if( block->m_Command != BLOCK_ABORT )
            DrawPanel->MouseToCursorSchema();
    }

    if( DrawPanel->ManageCurseur != NULL )
        switch( block->m_Command )
        {
        case  BLOCK_IDLE:
            DisplayError( this, wxT( "Error in HandleBlockPLace()" ) );
            break;

        case BLOCK_DRAG: /* Drag */
            BreakSegmentOnJunction( (SCH_SCREEN*) GetScreen() );

        case BLOCK_MOVE:    /* Move */
        case BLOCK_COPY:    /* Copy */
            block->m_BlockDrawStruct =
                PickStruct( GetScreen()->BlockLocate, GetScreen(), SEARCHALL );

        case BLOCK_PRESELECT_MOVE: /* Move with preselection list*/
            if( block->m_BlockDrawStruct != NULL )
            {
                ii = 1;
                CollectStructsToDrag( (SCH_SCREEN*) GetScreen() );
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
            block->m_BlockDrawStruct =
                PickStruct( GetScreen()->BlockLocate,
                            GetScreen(), SEARCHALL );
            DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );
            if( block->m_BlockDrawStruct != NULL )
            {
                ii = -1;
                DeleteStruct( DrawPanel,
                              DC,
                              (SCH_ITEM*) block->m_BlockDrawStruct );
                GetScreen()->SetModify();
            }
            block->m_BlockDrawStruct = NULL;
            TestDanglingEnds( GetScreen()->EEDrawList, DC );
            break;

        case BLOCK_SAVE: /* Save */
            block->m_BlockDrawStruct =
                PickStruct( GetScreen()->BlockLocate,
                            GetScreen(), SEARCHALL );
            DrawAndSizingBlockOutlines( DrawPanel, DC, FALSE );
            if( block->m_BlockDrawStruct != NULL )
            {
                wxPoint   oldpos = GetScreen()->m_Curseur;
                GetScreen()->m_Curseur = wxPoint( 0, 0 );
                SCH_ITEM* DrawStructCopy =
                    SaveStructListForPaste(
                         (SCH_ITEM*) block->m_BlockDrawStruct );
                PlaceStruct( GetScreen(), DrawStructCopy );
                GetScreen()->m_Curseur = oldpos;
                ii = -1;
            }
            block->m_BlockDrawStruct = NULL;
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
    {   /* clear struct.m_Flags  */
        EDA_BaseStruct* Struct;
        for( Struct = GetScreen()->EEDrawList;
            Struct != NULL;
            Struct = Struct->Next() )
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
        SetToolID( m_ID_current_state,
                   DrawPanel->m_PanelDefaultCursor,
                   wxEmptyString );
    }

    if( zoom_command )
        Window_Zoom( GetScreen()->BlockLocate );

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
    int ii = 0;
    DrawBlockStruct* block = &GetScreen()->BlockLocate;

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
        if( block->m_BlockDrawStruct )
        {
            if( block->m_BlockDrawStruct->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
            {       /* Delete the picked wrapper if this is a picked list. */
                DrawPickedStruct* PickedList;
                PickedList = (DrawPickedStruct*) block->m_BlockDrawStruct;
                PickedList->DeleteWrapperList();
            }
            block->m_BlockDrawStruct = NULL;
        }
        BreakSegmentOnJunction( (SCH_SCREEN*) GetScreen() );
        block->m_BlockDrawStruct =
            PickStruct( GetScreen()->BlockLocate,
                        GetScreen(), SEARCHALL );
        if( block->m_BlockDrawStruct != NULL )
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
        if( block->m_BlockDrawStruct != NULL )
        {
            ii = -1;
            DeleteStruct( DrawPanel, DC, (SCH_ITEM*) block->m_BlockDrawStruct );
            GetScreen()->SetModify();
        }
        TestDanglingEnds( GetScreen()->EEDrawList, DC );
        break;

    case BLOCK_SAVE:     /* Save */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        if( block->m_BlockDrawStruct != NULL )
        {
            wxPoint   oldpos = GetScreen()->m_Curseur;
            GetScreen()->m_Curseur = wxPoint( 0, 0 );
            SCH_ITEM* DrawStructCopy =
                SaveStructListForPaste( (SCH_ITEM*) block->m_BlockDrawStruct );
            PlaceStruct( GetScreen(), DrawStructCopy );
            GetScreen()->m_Curseur = oldpos;
            ii = -1;
        }
        break;

    case BLOCK_ZOOM:     /* Window Zoom */
        DrawPanel->ForceCloseManageCurseur( DrawPanel, DC );
        DrawPanel->SetCursor(
            DrawPanel->m_PanelCursor = DrawPanel->m_PanelDefaultCursor );
        Window_Zoom( GetScreen()->BlockLocate );
        break;


    case BLOCK_ROTATE:
        break;

    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
        if( block->m_BlockDrawStruct != NULL )
        {
            SaveCopyInUndoList( (SCH_ITEM*) block->m_BlockDrawStruct,
                               IS_CHANGED );

            ii = -1;
            /* Compute the mirror centre and put it on grid */
            wxPoint Center = block->Centre();
            PutOnGrid( &Center );
            MirrorStruct( DrawPanel,
                          DC,
                          (SCH_ITEM*) block->m_BlockDrawStruct,
                          Center );
            GetScreen()->SetModify();
        }
        TestDanglingEnds( GetScreen()->EEDrawList, DC );
        break;

    default:
        break;
    }

    if( ii <= 0 )
    {
        block->m_BlockDrawStruct = NULL;
        block->m_Flags   = 0;
        block->m_State   = STATE_NO_BLOCK;
        block->m_Command = BLOCK_IDLE;
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        GetScreen()->SetCurItem( NULL );
        SetToolID( m_ID_current_state,
                   DrawPanel->m_PanelDefaultCursor,
                   wxEmptyString );
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
    DrawBlockStruct*  PtBlock;
    DrawPickedStruct* PickedList;
    BASE_SCREEN*      screen = panel->GetScreen();

    PtBlock = &panel->GetScreen()->BlockLocate;
    GRSetDrawMode( DC, g_XorMode );

    /* Effacement ancien cadre */
    if( erase && PtBlock->m_BlockDrawStruct )
    {
        PtBlock->Offset( PtBlock->m_MoveVector );
        PtBlock->Draw( panel, DC );
        PtBlock->Offset( -PtBlock->m_MoveVector.x, -PtBlock->m_MoveVector.y );

        /* Effacement ancien affichage */
        if( PtBlock->m_BlockDrawStruct->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
        {
            PickedList = (DrawPickedStruct*) PtBlock->m_BlockDrawStruct;
            while( PickedList )
            {
                DrawStructsInGhost( panel,
                                    DC,
                                    (SCH_ITEM*) PickedList->m_PickedStruct,
                                    PtBlock->m_MoveVector.x,
                                    PtBlock->m_MoveVector.y );
                PickedList = (DrawPickedStruct*) PickedList->Next();
            }
        }
        else
            DrawStructsInGhost( panel,
                                DC,
                                (SCH_ITEM*) PtBlock->m_BlockDrawStruct,
                                PtBlock->m_MoveVector.x,
                                PtBlock->m_MoveVector.y );
    }

    /* Redessin nouvel affichage */

    PtBlock->m_MoveVector.x = screen->m_Curseur.x -
                              PtBlock->m_BlockLastCursorPosition.x;
    PtBlock->m_MoveVector.y = screen->m_Curseur.y -
                              PtBlock->m_BlockLastCursorPosition.y;

    GRSetDrawMode( DC, g_XorMode );
    PtBlock->Offset( PtBlock->m_MoveVector );
    PtBlock->Draw( panel, DC );
    PtBlock->Offset( -PtBlock->m_MoveVector.x, -PtBlock->m_MoveVector.y );

    if( PtBlock->m_BlockDrawStruct )
    {
        if( PtBlock->m_BlockDrawStruct->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
        {
            PickedList = (DrawPickedStruct*) PtBlock->m_BlockDrawStruct;
            while( PickedList )
            {
                DrawStructsInGhost( panel,
                                    DC,
                                    (SCH_ITEM*) PickedList->m_PickedStruct,
                                    PtBlock->m_MoveVector.x,
                                    PtBlock->m_MoveVector.y );
                PickedList = (DrawPickedStruct*) PickedList->Next();
            }
        }
        else
            DrawStructsInGhost( panel,
                                DC,
                                (SCH_ITEM*) PtBlock->m_BlockDrawStruct,
                                PtBlock->m_MoveVector.x,
                                PtBlock->m_MoveVector.y );
    }
}


/*****************************************************************************
* Routine to move an object(s) to a new position.							 *
* If DrawStruct is of type DrawPickedStruct, a list of objects picked is	 *
* assumed, otherwise exactly one structure is assumed been picked.			 *
*****************************************************************************/
bool MoveStruct( WinEDA_DrawPanel* panel, wxDC* DC, SCH_ITEM* DrawStruct )
{
    if( !DrawStruct )
        return FALSE;

    if( DrawStruct->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
    {
        DrawPickedStruct* pickedList = (DrawPickedStruct*) DrawStruct;

        if( DC )
            panel->PostDirtyRect( pickedList->GetBoundingBoxUnion() );

        PlaceStruct( panel->GetScreen(), pickedList );    // Place it in its new position.

        if( DC )
            RedrawStructList( panel, DC, pickedList, GR_DEFAULT_DRAWMODE );

        // Free the wrapper DrawPickedStruct chain
        pickedList->DeleteWrapperList();
    }
    else
    {
        if( DC )
            panel->PostDirtyRect( DrawStruct->GetBoundingBox() );
        PlaceStruct( panel->GetScreen(), DrawStruct );        /* Place it in its new position. */
        if( DC )
            RedrawOneStruct( panel, DC, DrawStruct, GR_DEFAULT_DRAWMODE );
    }
    return TRUE;
}


static void MirrorYPoint( wxPoint& point, wxPoint& Center )
{
    point.x -= Center.x;
    point.x  = -point.x;
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
    DrawMarkerStruct*              DrawMarker;
    DrawNoConnectStruct*           DrawNoConnect;
    SCH_TEXT*                      DrawText;
    wxPoint                        px;
    WinEDA_SchematicFrame*         frame;

    if( !DrawStruct )
        return;

    frame = (WinEDA_SchematicFrame*)wxGetApp().GetTopWindow();

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
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
        DrawConnect = (DrawJunctionStruct*) DrawStruct;
        MirrorYPoint( DrawConnect->m_Pos, Center );
        break;

    case DRAW_MARKER_STRUCT_TYPE:
        DrawMarker = (DrawMarkerStruct*) DrawStruct;
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
            dx = DrawText->Len_Size() / 2;
        else if( DrawText->m_Orient == 2 )  /* invert horizontal text*/
            dx = -DrawText->Len_Size() / 2;
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

    case DRAW_PICK_ITEM_STRUCT_TYPE:
        break;

    default:
        break;
    }
}


/*****************************************************************************
* Routine to Mirror an object(s).							 *
* If DrawStruct is of type DrawPickedStruct, a list of objects picked is	 *
* assumed, otherwise exactly one structure is assumed been picked.			 *
*****************************************************************************/
bool MirrorStruct( WinEDA_DrawPanel* panel,
                   wxDC*             DC,
                   SCH_ITEM*         DrawStruct,
                   wxPoint&          Center )
{
    if( !DrawStruct )
        return FALSE;

    if( DrawStruct->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
    {
        DrawPickedStruct* pickedList = (DrawPickedStruct*) DrawStruct;

        if( DC )
            panel->PostDirtyRect( pickedList->GetBoundingBoxUnion() );

        for( DrawPickedStruct* cur = pickedList;  cur;  cur = cur->Next() )
        {
            MirrorOneStruct( (SCH_ITEM*) cur->m_PickedStruct, Center );
            cur->m_PickedStruct->m_Flags = 0;
        }

        if( DC )
            RedrawStructList( panel, DC, pickedList, GR_DEFAULT_DRAWMODE );

        // Free the wrapper DrawPickedStruct chain
        pickedList->DeleteWrapperList();
    }
    else
    {
        if( DC )
            panel->PostDirtyRect( DrawStruct->GetBoundingBox() );

        MirrorOneStruct( DrawStruct, Center );      // Place it in its new position.

        if( DC )
            RedrawOneStruct( panel, DC, DrawStruct, GR_DEFAULT_DRAWMODE );

        DrawStruct->m_Flags = 0;
    }

    return true;
}


/*****************************************************************************/
static SCH_ITEM* CopyStruct( WinEDA_DrawPanel* panel,
                             wxDC*             DC,
                             BASE_SCREEN*      screen,
                             SCH_ITEM*         DrawStruct )
/*****************************************************************************/

/* Routine to copy a new entity of an object and reposition it.
 *  If DrawStruct is of type DrawPickedStruct, a list of objects picked is
 *  assumed, otherwise exactly one structure is assumed been picked.
 *  Return the new created struct
 */
{
    SCH_ITEM*         NewDrawStruct;
    DrawPickedStruct* PickedList = NULL;

    if( !DrawStruct )
        return FALSE;

    NewDrawStruct = DuplicateStruct( DrawStruct );
    if( NewDrawStruct == NULL )
        return NULL;

    PlaceStruct( screen, NewDrawStruct );
    /* Draw the new structure and chain it in: */
    if( NewDrawStruct->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
    {
        PickedList = (DrawPickedStruct*) NewDrawStruct;
        while( PickedList )  // Clear annotation for new components
        {
            EDA_BaseStruct* Struct = PickedList->m_PickedStruct;

            switch( Struct->Type() )
            {
            case TYPE_SCH_COMPONENT:
            {
                ( (SCH_COMPONENT*) Struct )->m_TimeStamp = GetTimeStamp();
                ( (SCH_COMPONENT*) Struct )->ClearAnnotation( NULL );
            }
            break;

            case DRAW_SHEET_STRUCT_TYPE:
            {
                //DuplicateStruct calls GenCopy, which should handle
                //m_AssociatedScreen and m_sRefCount properly.
                DrawSheetStruct* sheet = (DrawSheetStruct*) Struct;
                sheet->m_TimeStamp = GetTimeStamp();

                //sheet->m_AssociatedScreen->m_UndoList  = NULL;
                //sheet->m_AssociatedScreen->m_RedoList  = NULL;
                //keep m_AssociatedScreen pointer & associated.
                //sheet->m_Son = NULL; m_son is involved in undo and redo.
                break;
            }

            default:
                ;
            }

            SetaParent( Struct, screen );
            PickedList = (DrawPickedStruct*) PickedList->Next();
        }

        RedrawStructList( panel, DC, NewDrawStruct, GR_DEFAULT_DRAWMODE );
        /* Chain the new items */
        PickedList = (DrawPickedStruct*) NewDrawStruct;
        while( PickedList )
        {
            PickedList->m_PickedStruct->SetNext( screen->EEDrawList );
            screen->EEDrawList = PickedList->m_PickedStruct;
            PickedList = PickedList->Next();
        }
    }
    else
    {
        switch( NewDrawStruct->Type() )
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
        case DRAW_PICK_ITEM_STRUCT_TYPE:
        case DRAW_MARKER_STRUCT_TYPE:
        case DRAW_NOCONNECT_STRUCT_TYPE:
        default:
            break;

        case DRAW_SHEET_STRUCT_TYPE:
        {
            DrawSheetStruct* sheet = (DrawSheetStruct*) NewDrawStruct;
            sheet->m_TimeStamp = GetTimeStamp();
            sheet->SetSon( NULL );
            break;
        }

        case TYPE_SCH_COMPONENT:
            ( (SCH_COMPONENT*) NewDrawStruct )->m_TimeStamp = GetTimeStamp();
            ( (SCH_COMPONENT*) NewDrawStruct )->ClearAnnotation( NULL );
            break;
        }

        RedrawOneStruct( panel, DC, NewDrawStruct, GR_DEFAULT_DRAWMODE );

        SetaParent( NewDrawStruct, screen );
        NewDrawStruct->SetNext( screen->EEDrawList );
        screen->EEDrawList = NewDrawStruct;
    }

    /* Free the original DrawPickedStruct chain: */
    if( DrawStruct->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
    {
        PickedList = (DrawPickedStruct*) DrawStruct;
        PickedList->DeleteWrapperList();
    }

    return NewDrawStruct;
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

    if( DrawStruct->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
    {
        // Unlink all picked structs from current EEDrawList

        for( DrawPickedStruct* cur = (DrawPickedStruct*) DrawStruct;
            cur;
            cur = cur->Next() )
        {
            SCH_ITEM* item = cur->m_PickedStruct;
            screen->RemoveFromDrawList( item );
            panel->PostDirtyRect( item->GetBoundingBox() );
            item->SetNext( 0 );
            item->SetBack( 0 );
            item->m_Flags = IS_DELETED;
        }

        // Removed items are put onto the Undo list
        frame->SaveCopyInUndoList( DrawStruct, IS_DELETED );
    }
    else    /* structure classique */
    {
        screen->RemoveFromDrawList( DrawStruct );

        panel->PostDirtyRect( DrawStruct->GetBoundingBox() );

        /* Unlink the structure */
        DrawStruct->SetNext( 0 );
        DrawStruct->SetBack( 0 );  // Only one struct -> no link

        if( DrawStruct->Type() == DRAW_SHEET_STRUCT_TYPE )
        {
            frame->SaveCopyInUndoList( DrawStruct, IS_DELETED );    // Currently In TEST
        }
        else
            frame->SaveCopyInUndoList( DrawStruct, IS_DELETED );
    }
}


/*****************************************************************/
SCH_ITEM* SaveStructListForPaste( SCH_ITEM* DrawStruct )
/*****************************************************************/

/* Routine to Save an object from global drawing object list.
 *  This routine is the same as delete but:
 *  - the original list is NOT removed.
 *  - List is saved in g_BlockSaveDataList
 */
{
    DrawPickedStruct* PickedList;
    SCH_ITEM*         DrawStructCopy;

    if( !DrawStruct )
        return NULL;

    /* Make a copy of the original picked item. */
    DrawStructCopy = DuplicateStruct( DrawStruct );

    if( DrawStruct->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
    {
        /* Delete the picked wrapper if this is a picked list. */
        PickedList = (DrawPickedStruct*) DrawStruct;
        PickedList->DeleteWrapperList();
    }

    /* And delete old list and save the new list: */
    if( g_BlockSaveDataList ) /* Delete last deleted item or item list */
    {
        EDA_BaseStruct* item = g_BlockSaveDataList, * next_item;
        while( item )
        {
            next_item = item->Next();
            delete item;
            item = next_item;
        }
    }

    g_BlockSaveDataList = DrawStructCopy;
    DrawStructCopy->SetParent( NULL );

    return DrawStructCopy;
}


/*****************************************************************************
* Routine to paste a structure from the g_BlockSaveDataList stack.						 *
*	This routine is the same as undelete but original list is NOT removed.	 *
*****************************************************************************/
void WinEDA_SchematicFrame::PasteStruct( wxDC* DC )
{
    SCH_ITEM*         DrawStruct;
    DrawPickedStruct* PickedList = NULL;

    if( g_BlockSaveDataList == NULL )
    {
        DisplayError( this, wxT( "No struct to paste" ) );
        return;
    }

    DrawStruct = DuplicateStruct( g_BlockSaveDataList );

    PlaceStruct( GetScreen(), DrawStruct );

    RedrawStructList( DrawPanel, DC, DrawStruct, GR_DEFAULT_DRAWMODE );

    // Clear annotation and init new time stamp for the new components:
    if( DrawStruct->Type() == DRAW_PICK_ITEM_STRUCT_TYPE )
    {
        for( PickedList = (DrawPickedStruct*) DrawStruct; PickedList != NULL; ) // Clear annotation for new components
        {
            EDA_BaseStruct* Struct = PickedList->m_PickedStruct;
            if( Struct->Type() == TYPE_SCH_COMPONENT )
            {
                ( (SCH_COMPONENT*) Struct )->m_TimeStamp = GetTimeStamp();
                ( (SCH_COMPONENT*) Struct )->ClearAnnotation( NULL );
                SetaParent( Struct, GetScreen() );
            }
            PickedList = (DrawPickedStruct*) PickedList->Next();
        }

        RedrawStructList( DrawPanel, DC, DrawStruct, GR_DEFAULT_DRAWMODE );
        for( PickedList = (DrawPickedStruct*) DrawStruct; PickedList != NULL; )
        {
            SCH_ITEM* Struct = PickedList->m_PickedStruct;
            Struct->SetNext( GetScreen()->EEDrawList );
            SetaParent( Struct, GetScreen() );
            GetScreen()->EEDrawList = Struct;
            PickedList = PickedList->Next();
        }

        /* Save wrapper list in undo stack */
        SaveCopyInUndoList( DrawStruct, IS_NEW );
    }
    else
    {
        if( DrawStruct->Type() == TYPE_SCH_COMPONENT )
        {
            ( (SCH_COMPONENT*) DrawStruct )->m_TimeStamp = GetTimeStamp();
            ( (SCH_COMPONENT*) DrawStruct )->ClearAnnotation( NULL );
        }
        SetaParent( DrawStruct, GetScreen() );
        RedrawOneStruct( DrawPanel, DC, DrawStruct, GR_DEFAULT_DRAWMODE );
        DrawStruct->SetNext( GetScreen()->EEDrawList );
        GetScreen()->EEDrawList = DrawStruct;
        SaveCopyInUndoList( DrawStruct, IS_NEW );
    }

    /* clear .m_Flags member for all items */
    SCH_ITEM* Struct;
    for( Struct = GetScreen()->EEDrawList;
        Struct != NULL;
        Struct = Struct->Next() )
        Struct->m_Flags = 0;

    GetScreen()->SetModify();

    return;
}


/*****************************************************************************
* Routine to place a given object.											 *
*****************************************************************************/
bool PlaceStruct( BASE_SCREEN* screen, SCH_ITEM* DrawStruct )
{
    DrawPickedStruct* DrawStructs;
    wxPoint           move_vector;

    if( !DrawStruct )
        return FALSE;

    move_vector = screen->m_Curseur - screen->BlockLocate.m_BlockLastCursorPosition;

    switch( DrawStruct->Type() )
    {
    default:
    case TYPE_NOT_INIT:
        return FALSE;

    case DRAW_POLYLINE_STRUCT_TYPE:
    case DRAW_JUNCTION_STRUCT_TYPE:
    case DRAW_SEGMENT_STRUCT_TYPE:
    case DRAW_BUSENTRY_STRUCT_TYPE:
    case TYPE_SCH_TEXT:
    case TYPE_SCH_LABEL:
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_COMPONENT:
    case DRAW_SHEET_STRUCT_TYPE:
    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
    case DRAW_MARKER_STRUCT_TYPE:
    case DRAW_NOCONNECT_STRUCT_TYPE:
        MoveOneStruct( DrawStruct, move_vector );
        break;

    case DRAW_PICK_ITEM_STRUCT_TYPE:
        DrawStructs = (DrawPickedStruct*) DrawStruct;
        while( DrawStructs )
        {
            MoveOneStruct( DrawStructs->m_PickedStruct, move_vector );
            DrawStructs = DrawStructs->Next();
        }

        break;
    }

    return TRUE;
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
    DrawMarkerStruct*              DrawMarker;
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
        DrawMarker = (DrawMarkerStruct*) DrawStruct;
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

    case DRAW_PICK_ITEM_STRUCT_TYPE:
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
        NewDrawStruct = ( (DrawMarkerStruct*) DrawStruct )->GenCopy();
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

    case DRAW_PICK_ITEM_STRUCT_TYPE:
    {
        DrawPickedStruct* NewPickedItem, * PickedList = NULL,
        * LastPickedItem = NULL;
        PickedList = (DrawPickedStruct*) DrawStruct;
        while( PickedList )
        {
            NewPickedItem = new DrawPickedStruct();
            if( NewDrawStruct == NULL )
                NewDrawStruct = NewPickedItem;
            if( LastPickedItem )
                LastPickedItem->SetNext( NewPickedItem );
            LastPickedItem = NewPickedItem;
            NewPickedItem->m_PickedStruct =
                DuplicateStruct( PickedList->m_PickedStruct );
            PickedList = PickedList->Next();
        }

        break;
    }

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
{
    DrawPickedStruct*   DrawStructs, * FirstPicked;
    SCH_ITEM*           Struct;
    EDA_DrawLineStruct* SegmStruct;
    int ox, oy, fx, fy;

    /* Set membre .m_Flags des segments */
    for( Struct = screen->EEDrawList; Struct != NULL; Struct = Struct->Next() )
        Struct->m_Flags = 0;

    if( screen->BlockLocate.m_BlockDrawStruct->Type() ==
        DRAW_SEGMENT_STRUCT_TYPE )
        screen->BlockLocate.m_BlockDrawStruct->m_Flags = SELECTED;

    else if( screen->BlockLocate.m_BlockDrawStruct->Type() ==
             DRAW_PICK_ITEM_STRUCT_TYPE )
    {
        DrawStructs =
            (DrawPickedStruct*) screen->BlockLocate.m_BlockDrawStruct;
        while( DrawStructs )
        {
            Struct = DrawStructs->m_PickedStruct;
            DrawStructs     = DrawStructs->Next();
            Struct->m_Flags = SELECTED;
        }
    }

    if( screen->BlockLocate.m_Command != BLOCK_DRAG )
        return;

    ox = screen->BlockLocate.GetX();
    oy = screen->BlockLocate.GetY();
    fx = screen->BlockLocate.GetRight();
    fy = screen->BlockLocate.GetBottom();

    if( fx < ox )
        EXCHG( fx, ox );
    if( fy < oy )
        EXCHG( fy, oy );

    /* Pour Drag Block: remise sous forme de liste de structure, s'il n'y
     *  a qu'un seul element ( pour homogeneiser les traitements ulterieurs */
    if( screen->BlockLocate.m_BlockDrawStruct->Type() !=
        DRAW_PICK_ITEM_STRUCT_TYPE )
    {
        DrawStructs = new DrawPickedStruct(
             (SCH_ITEM*) screen->BlockLocate.m_BlockDrawStruct );
        screen->BlockLocate.m_BlockDrawStruct = DrawStructs;
    }

    /* Suppression du deplacement des extremites de segments hors cadre
     *  de selection */
    DrawStructs = (DrawPickedStruct*) screen->BlockLocate.m_BlockDrawStruct;
    while( DrawStructs )
    {
        Struct = DrawStructs->m_PickedStruct;
        DrawStructs = DrawStructs->Next();
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

    /* Recherche des elements complementaires a "dragger", c'est a dire les
     *  fils et connexions hors bloc relies a des pins ou entries elles meme
     *  draggees */

    FirstPicked = DrawStructs =
                      (DrawPickedStruct*) screen->BlockLocate.
                      m_BlockDrawStruct;
    while( DrawStructs )
    {
        Struct = DrawStructs->m_PickedStruct;
        DrawStructs = DrawStructs->Next();
        if( Struct->Type() == TYPE_SCH_COMPONENT )
        {
            LibEDA_BaseStruct* DrawItem;
            wxPoint            pos;
            DrawItem = GetNextPinPosition( (SCH_COMPONENT*) Struct, pos );
            while( DrawItem )
            {
                if( (pos.x < ox) || (pos.x > fx) || (pos.y < oy)
                   || (pos.y > fy) )
                    AddPickedItem( screen, pos );

                DrawItem = GetNextPinPosition( NULL, pos );
            }
        }

        if( Struct->Type() == DRAW_SHEET_STRUCT_TYPE )
        {
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
{
    DrawPickedStruct* DrawStructs;
    SCH_ITEM*         Struct;

    /* Examen de la liste des elements deja selectionnes */
    DrawStructs = (DrawPickedStruct*) screen->BlockLocate.m_BlockDrawStruct;
    while( DrawStructs )
    {
        Struct = DrawStructs->m_PickedStruct;
        DrawStructs = (DrawPickedStruct*) DrawStructs->Next();

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
            DrawStructs = new DrawPickedStruct( Struct );
            DrawStructs->SetNext( screen->BlockLocate.m_BlockDrawStruct );
            screen->BlockLocate.m_BlockDrawStruct =
                (EDA_BaseStruct*) DrawStructs;
            break;

        case DRAW_SEGMENT_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (EDA_DrawLineStruct*) Struct )
            if( Struct->m_Flags & SELECTED )
                break; /* Deja en liste */
            if( STRUCT->m_Start == position )
            {
                DrawStructs = new DrawPickedStruct( Struct );
                DrawStructs->SetNext( screen->BlockLocate.m_BlockDrawStruct );
                screen->BlockLocate.m_BlockDrawStruct =
                    (EDA_BaseStruct*) DrawStructs;
                Struct->m_Flags  = SELECTED | ENDPOINT | STARTPOINT;
                Struct->m_Flags &= ~STARTPOINT;
            }
            else if( STRUCT->m_End == position )
            {
                DrawStructs = new DrawPickedStruct( Struct );
                DrawStructs->SetNext( screen->BlockLocate.m_BlockDrawStruct );
                screen->BlockLocate.m_BlockDrawStruct =
                    (EDA_BaseStruct*) DrawStructs;
                Struct->m_Flags  = SELECTED | ENDPOINT | STARTPOINT;
                Struct->m_Flags &= ~ENDPOINT;
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
            DrawStructs = new DrawPickedStruct( Struct );
            DrawStructs->SetNext( screen->BlockLocate.m_BlockDrawStruct );
            screen->BlockLocate.m_BlockDrawStruct =
                (EDA_BaseStruct*) DrawStructs;
            Struct->m_Flags |= SELECTED;
            break;

        case TYPE_SCH_HIERLABEL:
        case TYPE_SCH_GLOBALLABEL:
                #undef STRUCT
                #define STRUCT ( (SCH_LABEL*) Struct )
            if( Struct->m_Flags & SELECTED )
                break; /* Already in list */
            if( STRUCT->m_Pos != position )
                break;
            DrawStructs = new DrawPickedStruct( Struct );
            DrawStructs->SetNext( screen->BlockLocate.m_BlockDrawStruct );
            screen->BlockLocate.m_BlockDrawStruct =
                (EDA_BaseStruct*) DrawStructs;
            Struct->m_Flags |= SELECTED;
            break;

        case TYPE_SCH_COMPONENT:
            break;

        case DRAW_SHEET_STRUCT_TYPE:
            break;

        case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
            break;

        case DRAW_PICK_ITEM_STRUCT_TYPE:
            break;

        case DRAW_MARKER_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (DrawMarkerStruct*) Struct )
            if( Struct->m_Flags & SELECTED )
                break; /* Already in list */
            if( STRUCT->m_Pos != position )
                break;
            DrawStructs = new DrawPickedStruct( Struct );
            DrawStructs->SetNext( screen->BlockLocate.m_BlockDrawStruct );
            screen->BlockLocate.m_BlockDrawStruct =
                (EDA_BaseStruct*) DrawStructs;
            Struct->m_Flags |= SELECTED;
            break;

        case DRAW_NOCONNECT_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (DrawNoConnectStruct*) Struct )
            if( Struct->m_Flags & SELECTED )
                break; /* Already in list */
            if( STRUCT->m_Pos != position )
                break;
            DrawStructs = new DrawPickedStruct( Struct );
            DrawStructs->SetNext( screen->BlockLocate.m_BlockDrawStruct );
            screen->BlockLocate.m_BlockDrawStruct =
                (EDA_BaseStruct*) DrawStructs;
            Struct->m_Flags |= SELECTED;
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
{
    EDA_LibComponentStruct* Entry;
    static LibEDA_BaseStruct* NextItem;
    static int Multi, convert, PartX, PartY, TransMat[2][2];
    LibEDA_BaseStruct* DEntry;
    int orient;
    LibDrawPin* Pin;

    if( aDrawLibItem )
    {
        NextItem = NULL;
        if( ( Entry =
                 FindLibPart( aDrawLibItem->m_ChipName.GetData(), wxEmptyString,
                              FIND_ROOT ) ) == NULL )
            return NULL;
        DEntry  = Entry->m_Drawings;
        Multi   = aDrawLibItem->m_Multi;
        convert = aDrawLibItem->m_Convert;
        PartX   = aDrawLibItem->m_Pos.x;
        PartY   = aDrawLibItem->m_Pos.y;
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
        aPosition = TransformCoordinate( TransMat, Pin->m_Pos);
        NextItem = DEntry->Next();
        return DEntry;
    }

    NextItem = NULL;
    return NULL;
}
