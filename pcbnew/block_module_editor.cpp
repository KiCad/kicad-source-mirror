/****************************************************/
/* block_module_editor.cpp                          */
/* Handle block commands for the footprint editor   */
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
#include "module_editor_frame.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"


#define BLOCK_COLOR BROWN
#define IS_SELECTED 1


static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase );
static int  MarkItemsInBloc( MODULE* module, EDA_Rect& Rect );

static void ClearMarkItems( MODULE* module );
static void CopyMarkedItems( MODULE* module, wxPoint offset );
static void MoveMarkedItems( MODULE* module, wxPoint offset );
static void MirrorMarkedItems( MODULE* module, wxPoint offset );
static void RotateMarkedItems( MODULE* module, wxPoint offset );
static void DeleteMarkedItems( MODULE* module );


/* Return the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
 * the key (ALT, SHIFT ALT ..) pressed when dragging mouse and left or
 * middle button pressed
 */
int WinEDA_ModuleEditFrame::ReturnBlockCommand( int key )
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


/**
 * Function HandleBlockEnd( )
 * Handle the "end"  of a block command,
 * i.e. is called at the end of the definition of the area of a block.
 * depending on the current block command, this command is executed
 * or parameters are initialized to prepare a call to HandleBlockPlace
 * in GetScreen()->m_BlockLocate
 * @return false if no item selected, or command finished,
 * true if some items found and HandleBlockPlace must be called later
 */
bool WinEDA_ModuleEditFrame::HandleBlockEnd( wxDC* DC )
{
    int  itemsCount    = 0;
    bool nextcmd = false;
    MODULE* currentModule = GetBoard()->m_Modules;

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
        itemsCount = MarkItemsInBloc( currentModule,
                                      GetScreen()->m_BlockLocate );
        if( itemsCount )
        {
            nextcmd = true;
            if( DrawPanel->ManageCurseur != NULL )
            {
                DrawPanel->ManageCurseur( DrawPanel, DC, wxDefaultPosition, FALSE );
                DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
                DrawPanel->ManageCurseur( DrawPanel, DC, wxDefaultPosition, FALSE );
            }
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_MOVE;
            DrawPanel->Refresh( TRUE );
        }
        break;

    case BLOCK_PRESELECT_MOVE:     /* Move with preselection list*/
        nextcmd = true;
        DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
        GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_MOVE;
        break;

    case BLOCK_DELETE:     /* Delete */
        itemsCount = MarkItemsInBloc( currentModule, GetScreen()->m_BlockLocate );
        if( itemsCount )
            SaveCopyInUndoList( currentModule, UR_MODEDIT );
        DeleteMarkedItems( currentModule );
        break;

    case BLOCK_SAVE:     /* Save */
    case BLOCK_PASTE:
        break;

    case BLOCK_ROTATE:
        itemsCount = MarkItemsInBloc( currentModule, GetScreen()->m_BlockLocate );
        if( itemsCount )
            SaveCopyInUndoList( currentModule, UR_MODEDIT );
        RotateMarkedItems( currentModule, GetScreen()->m_BlockLocate.Centre() );
        break;


    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
    case BLOCK_FLIP:     /* mirror */
        itemsCount = MarkItemsInBloc( currentModule, GetScreen()->m_BlockLocate );
        if( itemsCount )
            SaveCopyInUndoList( currentModule, UR_MODEDIT );
        MirrorMarkedItems( currentModule, GetScreen()->m_BlockLocate.Centre() );
        break;

    case BLOCK_ZOOM:     /* Window Zoom */
        Window_Zoom( GetScreen()->m_BlockLocate );
        break;

    case BLOCK_ABORT:
        break;

    case BLOCK_SELECT_ITEMS_ONLY:
        break;
    }

    if( !nextcmd )
    {
        if( GetScreen()->m_BlockLocate.m_Command  != BLOCK_SELECT_ITEMS_ONLY )
        {
            ClearMarkItems( currentModule );
        }
        GetScreen()->m_BlockLocate.m_Flags   = 0;
        GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        SetCurItem( NULL );
        SetToolID( m_ID_current_state, DrawPanel->GetDefaultCursor(), wxEmptyString );
        DrawPanel->Refresh( TRUE );
    }


    return nextcmd;
}


/******************************************************/
void WinEDA_ModuleEditFrame::HandleBlockPlace( wxDC* DC )
/******************************************************/

/* Routine to handle the BLOCK PLACE command
 *  Last routine for block operation for:
 *  - block move & drag
 *  - block copy & paste
 */
{
    bool    err = FALSE;
    MODULE* currentModule = GetBoard()->m_Modules;

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
        SaveCopyInUndoList( currentModule, UR_MODEDIT );
        MoveMarkedItems( currentModule, GetScreen()->m_BlockLocate.m_MoveVector );
        DrawPanel->Refresh( TRUE );
        break;

    case BLOCK_COPY:     /* Copy */
        GetScreen()->m_BlockLocate.ClearItemsList();
        SaveCopyInUndoList( currentModule, UR_MODEDIT );
        CopyMarkedItems( currentModule, GetScreen()->m_BlockLocate.m_MoveVector );
        break;

    case BLOCK_PASTE:     /* Paste */
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
    case BLOCK_FLIP:      /* Mirror by popup menu, from block move */
        SaveCopyInUndoList( currentModule, UR_MODEDIT );
        MirrorMarkedItems( currentModule, GetScreen()->m_BlockLocate.Centre() );
        break;

    case BLOCK_ROTATE:
        SaveCopyInUndoList( currentModule, UR_MODEDIT );
        RotateMarkedItems( currentModule, GetScreen()->m_BlockLocate.Centre() );
        break;

    case BLOCK_ZOOM:        // Handled by HandleBlockEnd
    case BLOCK_DELETE:
    case BLOCK_SAVE:
    case BLOCK_ABORT:
    default:
        break;
    }

    OnModify();

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    GetScreen()->m_BlockLocate.m_Flags   = 0;
    GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
    GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
    SetCurItem( NULL );
    DrawPanel->Refresh( TRUE );

    SetToolID( m_ID_current_state, DrawPanel->GetDefaultCursor(), wxEmptyString );
}


/* Traces the outline of the search block structures
 * The entire block follows the cursor
 */
static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase )
{
    BLOCK_SELECTOR*  PtBlock;
    BASE_SCREEN*     screen = aPanel->GetScreen();
    BOARD_ITEM*      item;
    wxPoint          move_offset;
    MODULE*          currentModule =
        ( (WinEDA_BasePcbFrame*) wxGetApp().GetTopWindow() )->m_ModuleEditFrame->GetBoard()->m_Modules;

    PtBlock = &screen->m_BlockLocate;
    GRSetDrawMode( aDC, g_XorMode );

    if( aErase )
    {
        PtBlock->Draw( aPanel, aDC, PtBlock->m_MoveVector, g_XorMode, PtBlock->m_Color );

        if( currentModule )
        {
            move_offset.x = -PtBlock->m_MoveVector.x;
            move_offset.y = -PtBlock->m_MoveVector.y;
            item = currentModule->m_Drawings;

            for( ; item != NULL; item = item->Next() )
            {
                if( item->m_Selected == 0 )
                    continue;

                switch( item->Type() )
                {
                case TYPE_TEXTE_MODULE:
                case TYPE_EDGE_MODULE:
                    item->Draw( aPanel, aDC, g_XorMode, move_offset );
                    break;

                default:
                    break;
                }
            }

            D_PAD* pad = currentModule->m_Pads;

            for( ; pad != NULL; pad = pad->Next() )
            {
                if( pad->m_Selected == 0 )
                    continue;
                pad->Draw( aPanel, aDC, g_XorMode, move_offset );
            }
        }
    }

    /* Repaint new view. */
    PtBlock->m_MoveVector = screen->m_Curseur - PtBlock->m_BlockLastCursorPosition;

    PtBlock->Draw( aPanel, aDC, PtBlock->m_MoveVector, g_XorMode, PtBlock->m_Color );

    if( currentModule )
    {
        item = currentModule->m_Drawings;
        move_offset = - PtBlock->m_MoveVector;

        for( ; item != NULL; item = item->Next() )
        {
            if( item->m_Selected == 0 )
                continue;

            switch( item->Type() )
            {
            case TYPE_TEXTE_MODULE:
            case TYPE_EDGE_MODULE:
                item->Draw( aPanel, aDC, g_XorMode, move_offset );
                break;

            default:
                break;
            }
        }

        D_PAD* pad = currentModule->m_Pads;

        for( ; pad != NULL; pad = pad->Next() )
        {
            if( pad->m_Selected == 0 )
                continue;

            pad->Draw( aPanel, aDC, g_XorMode, move_offset );
        }
    }
}


/* Copy marked items, at new position = old position + offset
 */
void CopyMarkedItems( MODULE* module, wxPoint offset )
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
            DisplayError( NULL, wxT( "CopyMarkedItems: type undefined" ) );
            break;
        }
    }

    MoveMarkedItems( module, offset );
}


/* Move marked items, at new position = old position + offset
 */
void MoveMarkedItems( MODULE* module, wxPoint offset )
{
    EDA_ITEM* item;

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


/* Delete marked items
 */
void DeleteMarkedItems( MODULE* module )
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


/* Mirror marked items, refer to a Vertical axis at position offset
 */
void MirrorMarkedItems( MODULE* module, wxPoint offset )
{
#define SETMIRROR( z ) (z) -= offset.x; (z) = -(z); (z) += offset.x;
    EDA_ITEM* item;

    if( module == NULL )
        return;

    D_PAD* pad = module->m_Pads;
    for( ; pad != NULL; pad = pad->Next() )
    {
        if( pad->m_Selected == 0 )
            continue;
        SETMIRROR( pad->GetPosition().x );
        pad->m_Pos0.x = pad->GetPosition().x;
        NEGATE( pad->m_Offset.x );
        NEGATE( pad->m_DeltaSize.x );
        pad->m_Orient      = 1800 - pad->m_Orient;
        NORMALIZE_ANGLE_POS( pad->m_Orient );
    }

    item = module->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        if( item->m_Selected == 0 )
            continue;

        switch( item->Type() )
        {
        case TYPE_EDGE_MODULE:
        {  EDGE_MODULE * edge =  (EDGE_MODULE*) item;
            SETMIRROR( edge->m_Start.x );
            edge->m_Start0.x = edge->m_Start.x;
            SETMIRROR( edge->m_End.x );
            edge->m_End0.x = edge->m_End.x;
            NEGATE( edge->m_Angle );
        }
            break;

        case TYPE_TEXTE_MODULE:
            SETMIRROR( ( (TEXTE_MODULE*) item )->GetPosition().x );
            ( (TEXTE_MODULE*) item )->m_Pos0.x =
                ( (TEXTE_MODULE*) item )->GetPosition().x;
            break;

        default:
            break;
        }

        item->m_Flags = 0;
        item->m_Selected = 0;
    }
}


/* Rotate marked items, refer to a Vertical axis at position offset
 */
void RotateMarkedItems( MODULE* module, wxPoint offset )
{
#define ROTATE( z ) RotatePoint( (&z), offset, 900 )
    EDA_ITEM* item;

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
        NORMALIZE_ANGLE_POS( pad->m_Orient );
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


void ClearMarkItems( MODULE* module )
{
    EDA_ITEM* item;

    if( module == NULL )
        return;

    item = module->m_Drawings;
    for( ; item != NULL; item = item->Next() )
        item->m_Flags = item->m_Selected = 0;

    item = module->m_Pads;
    for( ; item != NULL; item = item->Next() )
        item->m_Flags = item->m_Selected = 0;
}


/* Mark items inside rect.
 *  Items are inside rect when an end point is inside rect
 */
int MarkItemsInBloc( MODULE* module, EDA_Rect& Rect )
{
    EDA_ITEM* item;
    int       ItemsCount = 0;
    wxPoint   pos;
    D_PAD*    pad;

    if( module == NULL )
        return 0;

    pad = module->m_Pads;
    for( ; pad != NULL; pad = pad->Next() )
    {
        pad->m_Selected = 0;
        pos = pad->GetPosition();
        if( Rect.Contains( pos ) )
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
            if( Rect.Contains( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            pos = ( (EDGE_MODULE*) item )->m_End;
            if( Rect.Contains( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            break;

        case TYPE_TEXTE_MODULE:
            pos = ( (TEXTE_MODULE*) item )->GetPosition();
            if( Rect.Contains( pos ) )
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
