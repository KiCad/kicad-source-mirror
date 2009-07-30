/****************************************************/
/*	block_module_editor.cpp							*/
/* Handle block commands for the footprint editor	*/
/****************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "block_commande.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "autorout.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"


#define BLOCK_COLOR BROWN
#define IS_SELECTED 1

/* Variables Locales */

/* Fonctions exportees */

/* Fonctions Locales */
static void DrawMovingBlockOutlines( WinEDA_DrawPanel* panel,
                                     wxDC*             DC,
                                     bool              erase );
static int  MarkItemsInBloc( MODULE*   module,
                             EDA_Rect& Rect );

static void ClearMarkItems( MODULE* module );
static void CopyMarkedItems( MODULE* module, wxPoint offset );
static void MoveMarkedItems( MODULE* module, wxPoint offset );
static void MirrorMarkedItems( MODULE* module, wxPoint offset );
static void RotateMarkedItems( MODULE* module, wxPoint offset );
static void DeleteMarkedItems( MODULE* module );


/*************************************************************************/
int WinEDA_ModuleEditFrame::ReturnBlockCommand( int key )
/*************************************************************************/

/* Return the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
 *  the key (ALT, SHIFT ALT ..) pressed when dragging mouse and left or middle button
 *  pressed
 */
{
    int cmd;

    switch( key )
    {
    default:
        cmd = key & 0x255;
        break;

    case - 1:
        cmd = BLOCK_PRESELECT_MOVE;
        break;

    case 0:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_ALT:
        cmd = BLOCK_MIRROR_Y;
        break;

    case GR_KB_SHIFTCTRL:
        cmd = BLOCK_DELETE;
        break;

    case GR_KB_SHIFT:
        cmd = BLOCK_COPY;
        break;

    case GR_KB_CTRL:
        cmd = BLOCK_ROTATE;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


/****************************************************/
int WinEDA_ModuleEditFrame::HandleBlockEnd( wxDC* DC )
/****************************************************/

/* Command BLOCK END (end of block sizing)
 *  return :
 *  0 if command finished (zoom, delete ...)
 *  1 if HandleBlockPlace must follow (items found, and a block place command must follow)
 */
{
    int     ItemsCount    = 0, MustDoPlace = 0;
    MODULE* Currentmodule = GetBoard()->m_Modules;

    if( GetScreen()->m_BlockLocate.GetCount() )
    {
        BlockState   state   = GetScreen()->m_BlockLocate.m_State;
        CmdBlockType command = GetScreen()->m_BlockLocate.m_Command;
        DrawPanel->ForceCloseManageCurseur( DrawPanel, DC );
        GetScreen()->m_BlockLocate.m_State   = state;
        GetScreen()->m_BlockLocate.m_Command = command;
        DrawPanel->ManageCurseur = DrawAndSizingBlockOutlines;
        DrawPanel->ForceCloseManageCurseur = AbortBlockCurrentCommand;
        GetScreen()->m_Curseur.x = GetScreen()->m_BlockLocate.GetRight();
        GetScreen()->m_Curseur.y = GetScreen()->m_BlockLocate.GetBottom();
        DrawPanel->MouseToCursorSchema();
    }

    switch( GetScreen()->m_BlockLocate.m_Command )
    {
    case  BLOCK_IDLE:
        DisplayError( this, wxT( "Error in HandleBlockPLace" ) );
        break;

    case BLOCK_DRAG:        /* Drag */
    case BLOCK_MOVE:        /* Move */
    case BLOCK_COPY:        /* Copy */
        ItemsCount = MarkItemsInBloc( Currentmodule, GetScreen()->m_BlockLocate );
        if( ItemsCount )
        {
            MustDoPlace = 1;
            if( DrawPanel->ManageCurseur != NULL )
            {
                DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
                DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
                DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
            }
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_MOVE;
            DrawPanel->Refresh( TRUE );
        }
        break;

    case BLOCK_PRESELECT_MOVE:     /* Move with preselection list*/
        MustDoPlace = 1;
        DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
        GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_MOVE;
        break;

    case BLOCK_DELETE:     /* Delete */
        ItemsCount = MarkItemsInBloc( Currentmodule, GetScreen()->m_BlockLocate );
        if( ItemsCount )
            SaveCopyInUndoList( Currentmodule );
        DeleteMarkedItems( Currentmodule );
        break;

    case BLOCK_SAVE:     /* Save */
    case BLOCK_PASTE:
        break;

    case BLOCK_ROTATE:
        ItemsCount = MarkItemsInBloc( Currentmodule, GetScreen()->m_BlockLocate );
        if( ItemsCount )
            SaveCopyInUndoList( Currentmodule );
        RotateMarkedItems( Currentmodule, GetScreen()->m_BlockLocate.Centre() );
        break;


    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
    case BLOCK_INVERT:     /* mirror */
        ItemsCount = MarkItemsInBloc( Currentmodule, GetScreen()->m_BlockLocate );
        if( ItemsCount )
            SaveCopyInUndoList( Currentmodule );
        MirrorMarkedItems( Currentmodule, GetScreen()->m_BlockLocate.Centre() );
        break;

    case BLOCK_ZOOM:     /* Window Zoom */
        Window_Zoom( GetScreen()->m_BlockLocate );
        break;

    case BLOCK_ABORT:
        break;

    case BLOCK_SELECT_ITEMS_ONLY:
        break;
    }

    if( MustDoPlace <= 0 )
    {
        if( GetScreen()->m_BlockLocate.m_Command  != BLOCK_SELECT_ITEMS_ONLY )
        {
            ClearMarkItems( Currentmodule );
        }
        GetScreen()->m_BlockLocate.m_Flags   = 0;
        GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        SetCurItem( NULL );
        SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor, wxEmptyString );
        DrawPanel->Refresh( TRUE );
    }


    return MustDoPlace;
}


/******************************************************/
void WinEDA_ModuleEditFrame::HandleBlockPlace( wxDC* DC )
/******************************************************/

/* Routine to handle the BLOCK PLACE commande
 *  Last routine for block operation for:
 *  - block move & drag
 *  - block copie & paste
 */
{
    bool    err = FALSE;
    MODULE* Currentmodule = GetBoard()->m_Modules;

    if( DrawPanel->ManageCurseur == NULL )
    {
        err = TRUE;
        DisplayError( this, wxT( "HandleBlockPLace : ManageCurseur = NULL" ) );
    }

    GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;

    switch( GetScreen()->m_BlockLocate.m_Command )
    {
    case  BLOCK_IDLE:
        err = TRUE;
        break;

    case BLOCK_DRAG:                /* Drag */
    case BLOCK_MOVE:                /* Move */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        GetScreen()->m_BlockLocate.ClearItemsList();
        SaveCopyInUndoList( Currentmodule );
        MoveMarkedItems( Currentmodule, GetScreen()->m_BlockLocate.m_MoveVector );
        DrawPanel->Refresh( TRUE );
        break;

    case BLOCK_COPY:     /* Copy */
        GetScreen()->m_BlockLocate.ClearItemsList();
        SaveCopyInUndoList( Currentmodule );
        CopyMarkedItems( Currentmodule, GetScreen()->m_BlockLocate.m_MoveVector );
        break;

    case BLOCK_PASTE:     /* Paste (recopie du dernier bloc sauve */
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
    case BLOCK_INVERT:      /* Mirror by popup menu, from block move */
        SaveCopyInUndoList( Currentmodule );
        MirrorMarkedItems( Currentmodule, GetScreen()->m_BlockLocate.Centre() );
        break;

    case BLOCK_ROTATE:
        SaveCopyInUndoList( Currentmodule );
        RotateMarkedItems( Currentmodule, GetScreen()->m_BlockLocate.Centre() );
        break;

    case BLOCK_ZOOM:        // Handled by HandleBlockEnd
    case BLOCK_DELETE:
    case BLOCK_SAVE:
    case BLOCK_ABORT:
    default:
        break;
    }

    GetScreen()->SetModify();

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    GetScreen()->m_BlockLocate.m_Flags   = 0;
    GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
    GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
    SetCurItem( NULL );
    DrawPanel->Refresh( TRUE );

    SetToolID( m_ID_current_state,
               DrawPanel->m_PanelDefaultCursor,
               wxEmptyString );
}


/************************************************************************/
static void DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC,
                                     bool erase )
/************************************************************************/

/* Retrace le contour du block de recherche de structures
 *  L'ensemble du block suit le curseur
 */
{
    BLOCK_SELECTOR* PtBlock;
    BASE_SCREEN*     screen = panel->GetScreen();
    BOARD_ITEM*      item;
    wxPoint          move_offset;
    MODULE*          Currentmodule =
        ( (WinEDA_BasePcbFrame*) wxGetApp().GetTopWindow() )->m_ModuleEditFrame->GetBoard()->m_Modules;

    PtBlock = &screen->m_BlockLocate;
    GRSetDrawMode( DC, g_XorMode );

    /* Effacement ancien cadre */
    if( erase )
    {
        PtBlock->Draw( panel, DC, PtBlock->m_MoveVector, g_XorMode, PtBlock->m_Color );

        if( Currentmodule )
        {
            move_offset.x = -PtBlock->m_MoveVector.x;
            move_offset.y = -PtBlock->m_MoveVector.y;
            item = Currentmodule->m_Drawings;
            for( ; item != NULL; item = item->Next() )
            {
                if( item->m_Selected == 0 )
                    continue;

                switch( item->Type() )
                {
                case TYPE_TEXTE_MODULE:
                case TYPE_EDGE_MODULE:
                    item->Draw( panel, DC, g_XorMode, move_offset );
                    break;

                default:
                    break;
                }
            }

            D_PAD* pad = Currentmodule->m_Pads;
            for( ; pad != NULL; pad = pad->Next() )
            {
                if( pad->m_Selected == 0 )
                    continue;
                pad->Draw( panel, DC, g_XorMode, move_offset );
            }
        }
    }

    /* Redessin nouvel affichage */
    PtBlock->m_MoveVector.x = screen->m_Curseur.x -
                              PtBlock->m_BlockLastCursorPosition.x;
    PtBlock->m_MoveVector.y = screen->m_Curseur.y -
                              PtBlock->m_BlockLastCursorPosition.y;

    PtBlock->Draw( panel, DC, PtBlock->m_MoveVector, g_XorMode, PtBlock->m_Color );


    if( Currentmodule )
    {
        item = Currentmodule->m_Drawings;
        move_offset.x = -PtBlock->m_MoveVector.x;
        move_offset.y = -PtBlock->m_MoveVector.y;
        for( ; item != NULL; item = item->Next() )
        {
            if( item->m_Selected == 0 )
                continue;

            switch( item->Type() )
            {
            case TYPE_TEXTE_MODULE:
            case TYPE_EDGE_MODULE:
                item->Draw( panel, DC, g_XorMode, move_offset );
                break;

            default:
                break;
            }
        }

        D_PAD* pad = Currentmodule->m_Pads;
        for( ; pad != NULL; pad = pad->Next() )
        {
            if( pad->m_Selected == 0 )
                continue;
            pad->Draw( panel, DC, g_XorMode, move_offset );
        }
    }
}


/****************************************************************************/
void CopyMarkedItems( MODULE* module, wxPoint offset )
/****************************************************************************/

/* Copy marked items, at new position = old position + offset
 */
{
    if( module == NULL )
        return;

    for( D_PAD* pad = module->m_Pads;  pad;  pad = pad->Next() )
    {
        if( pad->m_Selected == 0 )
            continue;
        pad->m_Selected = 0;
        D_PAD* NewPad = new D_PAD( module );
        NewPad->Copy( pad );
        NewPad->m_Selected = IS_SELECTED;
        module->m_Pads.PushFront( NewPad );
    }

    for( BOARD_ITEM* item = module->m_Drawings;  item; item = item->Next() )
    {
        if( item->m_Selected == 0 )
            continue;

        item->m_Selected = 0;

        switch( item->Type() )
        {
        case TYPE_TEXTE_MODULE:
            TEXTE_MODULE * textm;
            textm = new TEXTE_MODULE( module );
            textm->Copy( (TEXTE_MODULE*) item );
            textm->m_Selected = IS_SELECTED;
            module->m_Drawings.PushFront( textm );
            break;

        case TYPE_EDGE_MODULE:
            EDGE_MODULE * edge;
            edge = new EDGE_MODULE( module );
            edge->Copy( (EDGE_MODULE*) item );
            edge->m_Selected = IS_SELECTED;
            module->m_Drawings.PushFront( edge );
            break;

        default:
            DisplayError( NULL,
                          wxT( "Internal Err: CopyMarkedItems: type indefini" ) );
            break;
        }
    }

    MoveMarkedItems( module, offset );
}


/****************************************************/
void MoveMarkedItems( MODULE* module, wxPoint offset )
/****************************************************/

/* Move marked items, at new position = old position + offset
 */
{
    EDA_BaseStruct* item;

    if( module == NULL )
        return;

    D_PAD* pad = module->m_Pads;
    for( ; pad != NULL; pad = pad->Next() )
    {
        if( pad->m_Selected == 0 )
            continue;
        pad->SetPosition( pad->GetPosition() + offset );
        pad->m_Pos0 += offset;
    }

    item = module->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        if( item->m_Selected == 0 )
            continue;

        switch( item->Type() )
        {
        case TYPE_TEXTE_MODULE:
            ( (TEXTE_MODULE*) item )->m_Pos += offset;
            ( (TEXTE_MODULE*) item )->m_Pos0 += offset;
            break;

        case TYPE_EDGE_MODULE:
            ( (EDGE_MODULE*) item )->m_Start += offset;
            ( (EDGE_MODULE*) item )->m_End += offset;

            ( (EDGE_MODULE*) item )->m_Start0 += offset;
            ( (EDGE_MODULE*) item )->m_End0 += offset;
            break;

        default:
            ;
        }

        item->m_Flags = item->m_Selected = 0;
    }
}


/******************************************************/
void DeleteMarkedItems( MODULE* module )
/******************************************************/

/* Delete marked items
 */
{
    BOARD_ITEM* item;
    BOARD_ITEM* next_item;
    D_PAD*      pad;
    D_PAD*      next_pad;

    if( module == NULL )
        return;

    pad = module->m_Pads;
    for( ; pad != NULL; pad = next_pad )
    {
        next_pad = pad->Next();
        if( pad->m_Selected == 0 )
            continue;
        pad->DeleteStructure();
    }

    item = module->m_Drawings;
    for( ; item != NULL; item = next_item )
    {
        next_item = item->Next();
        if( item->m_Selected == 0 )
            continue;
        item->DeleteStructure();
    }
}


/******************************************************/
void MirrorMarkedItems( MODULE* module, wxPoint offset )
/******************************************************/

/* Mirror marked items, refer to a Vertical axis at position offset
 */
{
#define SETMIRROR( z ) (z) -= offset.x; (z) = -(z); (z) += offset.x;
    EDA_BaseStruct* item;

    if( module == NULL )
        return;

    D_PAD* pad = module->m_Pads;
    for( ; pad != NULL; pad = pad->Next() )
    {
        if( pad->m_Selected == 0 )
            continue;
        SETMIRROR( pad->GetPosition().x );
        pad->m_Pos0.x = pad->GetPosition().x;
        pad->m_Offset.x    = -pad->m_Offset.x;
        pad->m_DeltaSize.x = -pad->m_DeltaSize.x;
        pad->m_Orient      = 1800 - pad->m_Orient;
        NORMALIZE_ANGLE( pad->m_Orient );
    }

    item = module->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        if( item->m_Selected == 0 )
            continue;

        switch( item->Type() )
        {
        case TYPE_EDGE_MODULE:
            SETMIRROR( ( (EDGE_MODULE*) item )->m_Start.x );
            ( (EDGE_MODULE*) item )->m_Start0.x =
                ( (EDGE_MODULE*) item )->m_Start.x;
            SETMIRROR( ( (EDGE_MODULE*) item )->m_End.x );
            ( (EDGE_MODULE*) item )->m_End0.x =
                ( (EDGE_MODULE*) item )->m_End.x;
            ( (EDGE_MODULE*) item )->m_Angle =
                -( (EDGE_MODULE*) item )->m_Angle;
            break;

        case TYPE_TEXTE_MODULE:
            SETMIRROR( ( (TEXTE_MODULE*) item )->GetPosition().x );
            ( (TEXTE_MODULE*) item )->m_Pos0.x =
                ( (TEXTE_MODULE*) item )->GetPosition().x;
            break;

        default:
            ;
        }

        item->m_Flags = item->m_Selected = 0;
    }
}


/******************************************************/
void RotateMarkedItems( MODULE* module, wxPoint offset )
/******************************************************/

/* Rotate marked items, refer to a Vertical axis at position offset
 */
{
#define ROTATE( z ) RotatePoint( (&z), offset, 900 )
    EDA_BaseStruct* item;

    if( module == NULL )
        return;

    D_PAD* pad = module->m_Pads;
    for( ; pad != NULL; pad = pad->Next() )
    {
        if( pad->m_Selected == 0 )
            continue;
        ROTATE( pad->GetPosition() );
        pad->m_Pos0    = pad->GetPosition();
        pad->m_Orient += 900;
        NORMALIZE_ANGLE( pad->m_Orient );
    }

    item = module->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        if( item->m_Selected == 0 )
            continue;

        switch( item->Type() )
        {
        case TYPE_EDGE_MODULE:
            ROTATE( ( (EDGE_MODULE*) item )->m_Start );
            ( (EDGE_MODULE*) item )->m_Start0 =
                ( (EDGE_MODULE*) item )->m_Start;
            ROTATE( ( (EDGE_MODULE*) item )->m_End );
            ( (EDGE_MODULE*) item )->m_End0 = ( (EDGE_MODULE*) item )->m_End;
            break;

        case TYPE_TEXTE_MODULE:
            ROTATE( ( (TEXTE_MODULE*) item )->GetPosition() );
            ( (TEXTE_MODULE*) item )->m_Pos0 =
                ( (TEXTE_MODULE*) item )->GetPosition();
            ( (TEXTE_MODULE*) item )->m_Orient += 900;
            break;

        default:
            ;
        }

        item->m_Flags = item->m_Selected = 0;
    }
}


/*********************************************************/
void ClearMarkItems( MODULE* module )
/*********************************************************/
{
    EDA_BaseStruct* item;

    if( module == NULL )
        return;

    item = module->m_Drawings;
    for( ; item != NULL; item = item->Next() )
        item->m_Flags = item->m_Selected = 0;

    item = module->m_Pads;
    for( ; item != NULL; item = item->Next() )
        item->m_Flags = item->m_Selected = 0;
}


/***************************************************************/
int MarkItemsInBloc( MODULE* module, EDA_Rect& Rect )
/***************************************************************/

/* Mark items inside rect.
 *  Items are inside rect when an end point is inside rect
 */
{
    EDA_BaseStruct* item;
    int             ItemsCount = 0;
    wxPoint         pos;
    D_PAD*          pad;

    if( module == NULL )
        return 0;

    pad = module->m_Pads;
    for( ; pad != NULL; pad = pad->Next() )
    {
        pad->m_Selected = 0;
        pos = pad->GetPosition();
        if( Rect.Inside( pos ) )
        {
            pad->m_Selected = IS_SELECTED;
            ItemsCount++;
        }
    }

    item = module->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        item->m_Selected = 0;

        switch( item->Type() )
        {
        case TYPE_EDGE_MODULE:
            pos = ( (EDGE_MODULE*) item )->m_Start;
            if( Rect.Inside( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            pos = ( (EDGE_MODULE*) item )->m_End;
            if( Rect.Inside( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            break;

        case TYPE_TEXTE_MODULE:
            pos = ( (TEXTE_MODULE*) item )->GetPosition();
            if( Rect.Inside( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            break;

        default:
            break;
        }
    }

    return ItemsCount;
}
