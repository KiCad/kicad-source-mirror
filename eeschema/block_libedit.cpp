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
static void DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static int  MarkItemsInBloc( EDA_LibComponentStruct* LibComponent,
                             EDA_Rect&                Rect );

static void ClearMarkItems( EDA_LibComponentStruct* LibComponent );
static void CopyMarkedItems( EDA_LibComponentStruct* LibEntry, wxPoint offset );
static void MoveMarkedItems( EDA_LibComponentStruct* LibEntry, wxPoint offset );
static void MirrorMarkedItems( EDA_LibComponentStruct* LibEntry, wxPoint offset );
static void DeleteMarkedItems( EDA_LibComponentStruct* LibEntry );

/*********************************************************/
void ClearMarkItems( EDA_LibComponentStruct* LibComponent )
/*********************************************************/
{
    LibEDA_BaseStruct* item;

    if( LibComponent == NULL )
        return;

    item = LibComponent->m_Drawings;
    for( ; item != NULL; item = item->Next() )
        item->m_Flags = item->m_Selected = 0;
}


/***************************************************************/
int MarkItemsInBloc( EDA_LibComponentStruct* LibComponent,
                     EDA_Rect&               Rect )
/***************************************************************/

/* Mark items inside rect.
 *  Items are inside rect when an end point is inside rect
 * 
 *  Rules for convert drawings and other parts ( for multi part per package):
 *  - Commons are always marked
 *  - Specific graphic shapes must agree with the current displayed part and convert
 *  - Because most of pins are specific to current part and current convert:
 *      - if g_EditPinByPinIsOn == TRUE, or flag .m_UnitSelectionLocked == TRUE,
 *          only the pins specific to current part and current convert are marked
 *      - all specific to current convert pins are marked;
 */
{
    LibEDA_BaseStruct* item;
    int ItemsCount = 0;
    wxPoint            pos;
    bool ItemIsInOtherPart, ItemIsInOtherConvert;

    if( LibComponent == NULL )
        return 0;

    item = LibComponent->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        item->m_Selected = 0;

        // Do not consider other units or other convert items:
        ItemIsInOtherPart = ItemIsInOtherConvert = FALSE;
        if( item->m_Unit && (item->m_Unit != CurrentUnit) )
            ItemIsInOtherPart = TRUE;
        if( item->m_Convert && (item->m_Convert != CurrentConvert) )
            ItemIsInOtherConvert = TRUE;
        if( ItemIsInOtherPart || ItemIsInOtherConvert )
        {
            if( item->m_StructType == COMPONENT_PIN_DRAW_TYPE )
            { // Specific rules for pins:
                if( g_EditPinByPinIsOn )
                    continue;
                if( LibComponent->m_UnitSelectionLocked )
                    continue;
                if( ItemIsInOtherConvert )
                    continue;
            }
            else
                continue;
        }

        switch( item->m_StructType )
        {
        case COMPONENT_ARC_DRAW_TYPE:
        {
            pos = ( (LibDrawArc*) item )->m_ArcStart; pos.y = -pos.y;
            if( Rect.Inside( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            pos = ( (LibDrawArc*) item )->m_ArcEnd; pos.y = -pos.y;
            if( Rect.Inside( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            break;
        }

        case COMPONENT_CIRCLE_DRAW_TYPE:
            pos = ( (LibDrawCircle*) item )->m_Pos; pos.y = -pos.y;
            if( Rect.Inside( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            break;

        case COMPONENT_RECT_DRAW_TYPE:
            pos = ( (LibDrawSquare*) item )->m_Pos; pos.y = -pos.y;
            if( Rect.Inside( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            pos = ( (LibDrawSquare*) item )->m_End; pos.y = -pos.y;
            if( Rect.Inside( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
        {
            int  ii, imax = ( (LibDrawPolyline*) item )->n * 2;
            int* ptpoly = ( (LibDrawPolyline*) item )->PolyList;
            for( ii = 0; ii < imax; ii += 2 )
            {
                pos.x = ptpoly[ii]; pos.y = -ptpoly[ii + 1];
                if( Rect.Inside( pos ) )
                {
                    item->m_Selected = IS_SELECTED;
                    ItemsCount++;
                    break;
                }
            }
        }
            break;

        case COMPONENT_LINE_DRAW_TYPE:
            break;

        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
            pos = ( (LibDrawText*) item )->m_Pos; pos.y = -pos.y;
            if( Rect.Inside( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            break;

        case COMPONENT_PIN_DRAW_TYPE:
                #undef STRUCT
                #define STRUCT ( (LibDrawPin*) item )
            pos = STRUCT->m_Pos; pos.y = -pos.y;
            if( Rect.Inside( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            pos = STRUCT->ReturnPinEndPoint(); pos.y = -pos.y;
            if( Rect.Inside( pos ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }
            break;

        case COMPONENT_FIELD_DRAW_TYPE:
            break;

        default:
            break;
        }
    }

    return ItemsCount;
}


/*************************************************************************/
int WinEDA_LibeditFrame::ReturnBlockCommand( int key )
/*************************************************************************/

/* Return the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
 *  the key (ALT, SHIFT ALT ..)
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
    case GR_KB_SHIFT:
        cmd = BLOCK_COPY;
        break;

    case GR_KB_SHIFTCTRL:
        cmd = BLOCK_DELETE;
        break;

    case GR_KB_CTRL:
        cmd = BLOCK_INVERT;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


/****************************************************/
int WinEDA_LibeditFrame::HandleBlockEnd( wxDC* DC )
/****************************************************/

/* Command BLOCK END (end of block sizing)
 *  return :
 *  0 if command finished (zoom, delete ...)
 *  1 if HandleBlockPlace must follow (items found, and a block place command must follow)
 */
{
    int ItemsCount = 0, MustDoPlace = 0;

    if( GetScreen()->BlockLocate.m_BlockDrawStruct )
    {
        BlockState state     = GetScreen()->BlockLocate.m_State;
        CmdBlockType command = GetScreen()->BlockLocate.m_Command;
        DrawPanel->ForceCloseManageCurseur( DrawPanel, DC );
        GetScreen()->BlockLocate.m_State   = state;
        GetScreen()->BlockLocate.m_Command = command;
        DrawPanel->ManageCurseur = DrawAndSizingBlockOutlines;
        DrawPanel->ForceCloseManageCurseur = AbortBlockCurrentCommand;
        GetScreen()->m_Curseur.x = GetScreen()->BlockLocate.GetRight();
        GetScreen()->m_Curseur.y = GetScreen()->BlockLocate.GetBottom();
        DrawPanel->MouseToCursorSchema();
    }

    switch( GetScreen()->BlockLocate.m_Command )
    {
    case  BLOCK_IDLE:
        DisplayError( this, wxT( "Error in HandleBlockPLace" ) );
        break;

    case BLOCK_DRAG:        /* Drag */
    case BLOCK_MOVE:        /* Move */
    case BLOCK_COPY:        /* Copy */
        ItemsCount = MarkItemsInBloc( CurrentLibEntry, GetScreen()->BlockLocate );
        if( ItemsCount )
        {
            MustDoPlace = 1;
            if( DrawPanel->ManageCurseur != NULL )
            {
                DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
                DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
                DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
            }
            GetScreen()->BlockLocate.m_State = STATE_BLOCK_MOVE;
            DrawPanel->Refresh( TRUE );
        }
        break;

    case BLOCK_PRESELECT_MOVE:     /* Move with preselection list*/
        MustDoPlace = 1;
        DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
        GetScreen()->BlockLocate.m_State = STATE_BLOCK_MOVE;
        break;

    case BLOCK_DELETE:     /* Delete */
        ItemsCount = MarkItemsInBloc( CurrentLibEntry, GetScreen()->BlockLocate );
        if( ItemsCount )
            SaveCopyInUndoList( CurrentLibEntry );
        DeleteMarkedItems( CurrentLibEntry );
        break;

    case BLOCK_SAVE:     /* Save */
    case BLOCK_PASTE:
    case BLOCK_ROTATE:
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
        break;


    case BLOCK_INVERT:
        ItemsCount = MarkItemsInBloc( CurrentLibEntry, GetScreen()->BlockLocate );
        if( ItemsCount )
            SaveCopyInUndoList( CurrentLibEntry );
        MirrorMarkedItems( CurrentLibEntry, GetScreen()->BlockLocate.Centre() );
        break;

    case BLOCK_ZOOM:     /* Window Zoom */
        Window_Zoom( GetScreen()->BlockLocate );
        break;

    case BLOCK_ABORT:
        break;

    case BLOCK_SELECT_ITEMS_ONLY:
        break;
    }

    if( MustDoPlace <= 0 )
    {
        if( GetScreen()->BlockLocate.m_Command  != BLOCK_SELECT_ITEMS_ONLY )
        {
            ClearMarkItems( CurrentLibEntry );
        }
        GetScreen()->BlockLocate.m_Flags   = 0;
        GetScreen()->BlockLocate.m_State   = STATE_NO_BLOCK;
        GetScreen()->BlockLocate.m_Command = BLOCK_IDLE;
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        GetScreen()->SetCurItem( NULL );
        SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor, wxEmptyString );
        DrawPanel->Refresh( TRUE );
    }


    return MustDoPlace;
}


/******************************************************/
void WinEDA_LibeditFrame::HandleBlockPlace( wxDC* DC )
/******************************************************/

/* Routine to handle the BLOCK PLACE commande
 *  Last routine for block operation for:
 *  - block move & drag
 *  - block copie & paste
 */
{
    bool err = FALSE;

    if( DrawPanel->ManageCurseur == NULL )
    {
        err = TRUE;
        DisplayError( this, wxT( "HandleBlockPLace : ManageCurseur = NULL" ) );
    }

    GetScreen()->BlockLocate.m_State = STATE_BLOCK_STOP;

    switch( GetScreen()->BlockLocate.m_Command )
    {
    case  BLOCK_IDLE:
        err = TRUE;
        break;

    case BLOCK_DRAG:                /* Drag */
    case BLOCK_MOVE:                /* Move */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        GetScreen()->BlockLocate.m_BlockDrawStruct = NULL;
        SaveCopyInUndoList( CurrentLibEntry );
        MoveMarkedItems( CurrentLibEntry, GetScreen()->BlockLocate.m_MoveVector );
        DrawPanel->Refresh( TRUE );
        break;

    case BLOCK_COPY:     /* Copy */
        GetScreen()->BlockLocate.m_BlockDrawStruct = NULL;
        SaveCopyInUndoList( CurrentLibEntry );
        CopyMarkedItems( CurrentLibEntry, GetScreen()->BlockLocate.m_MoveVector );
        break;

    case BLOCK_PASTE:     /* Paste (recopie du dernier bloc sauve */
        GetScreen()->BlockLocate.m_BlockDrawStruct = NULL;
        break;

    case BLOCK_INVERT:      /* Invert by popup menu, from block move */
        SaveCopyInUndoList( CurrentLibEntry );
        MirrorMarkedItems( CurrentLibEntry, GetScreen()->BlockLocate.Centre() );
        break;

    case BLOCK_ZOOM:        // Handled by HandleBlockEnd
    case BLOCK_DELETE:
    case BLOCK_SAVE:
    case BLOCK_ROTATE:
    case BLOCK_ABORT:
    default:
        break;
    }

    GetScreen()->SetModify();


    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    GetScreen()->BlockLocate.m_Flags   = 0;
    GetScreen()->BlockLocate.m_State   = STATE_NO_BLOCK;
    GetScreen()->BlockLocate.m_Command = BLOCK_IDLE;
    GetScreen()->SetCurItem( NULL );
    DrawPanel->Refresh( TRUE );

    SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor, wxEmptyString );
}


/************************************************************************/
static void DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC,
                                     bool erase )
/************************************************************************/

/* Retrace le contour du block de recherche de structures
 *  L'ensemble du block suit le curseur
 */
{
    DrawBlockStruct* PtBlock;
    BASE_SCREEN* screen = panel->m_Parent->GetScreen();
    LibEDA_BaseStruct* item;
    wxPoint move_offset;

    PtBlock = &panel->GetScreen()->BlockLocate;
    GRSetDrawMode( DC, g_XorMode );

    /* Effacement ancien cadre */
    if( erase )
    {
        PtBlock->Offset( PtBlock->m_MoveVector );
        PtBlock->Draw( panel, DC );
        PtBlock->Offset( -PtBlock->m_MoveVector.x, -PtBlock->m_MoveVector.y );

        if( CurrentLibEntry )
        {
            item = CurrentLibEntry->m_Drawings;
            for( ; item != NULL; item = item->Next() )
            {
                if( item->m_Selected == 0 )
                    continue;
                /* Do not draw items for other units */
                if( CurrentUnit && item->m_Unit && (item->m_Unit != CurrentUnit) )
                    continue;
                if( CurrentConvert && item->m_Convert && (item->m_Convert != CurrentConvert) )
                    continue;
                DrawLibraryDrawStruct( panel, DC, CurrentLibEntry,
                                       PtBlock->m_MoveVector.x, PtBlock->m_MoveVector.y,
                                       item, CurrentUnit, g_XorMode );
            }
        }
    }

    /* Redessin nouvel affichage */
    PtBlock->m_MoveVector.x = screen->m_Curseur.x - PtBlock->m_BlockLastCursorPosition.x;
    PtBlock->m_MoveVector.y = screen->m_Curseur.y - PtBlock->m_BlockLastCursorPosition.y;

    GRSetDrawMode( DC, g_XorMode );
    PtBlock->Offset( PtBlock->m_MoveVector );
    PtBlock->Draw( panel, DC );
    PtBlock->Offset( -PtBlock->m_MoveVector.x, -PtBlock->m_MoveVector.y );


    if( CurrentLibEntry )
    {
        item = CurrentLibEntry->m_Drawings;
        for( ; item != NULL; item = item->Next() )
        {
            if( item->m_Selected == 0 )
                continue;
            /* Do not draw items for other units */
            if( CurrentUnit && item->m_Unit && (item->m_Unit != CurrentUnit) )
                continue;
            if( CurrentConvert && item->m_Convert && (item->m_Convert != CurrentConvert) )
                continue;
            DrawLibraryDrawStruct( panel, DC, CurrentLibEntry,
                                   PtBlock->m_MoveVector.x, PtBlock->m_MoveVector.y,
                                   item, CurrentUnit, g_XorMode );
        }
    }
}


/****************************************************************************/
void CopyMarkedItems( EDA_LibComponentStruct* LibEntry, wxPoint offset )
/****************************************************************************/

/* Copy marked items, at new position = old position + offset
 */
{
    LibEDA_BaseStruct* item;

    if( LibEntry == NULL )
        return;

    item = LibEntry->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        if( item->m_Selected == 0 )
            continue;
        item->m_Selected = 0;
        LibEDA_BaseStruct* newitem = CopyDrawEntryStruct( NULL, item );
        newitem->m_Selected = IS_SELECTED;
        newitem->Pnext = LibEntry->m_Drawings;
        LibEntry->m_Drawings = newitem;
    }

    MoveMarkedItems( LibEntry, offset );
}


/****************************************************************************/
void MoveMarkedItems( EDA_LibComponentStruct* LibEntry, wxPoint offset )
/****************************************************************************/

/* Move marked items, at new position = old position + offset
 */
{
    LibEDA_BaseStruct* item;

    if( LibEntry == NULL )
        return;

    offset.y = -offset.y;  // Y axis for lib items is Down to Up: reverse y offset value
    item = LibEntry->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        if( item->m_Selected == 0 )
            continue;

        switch( item->m_StructType )
        {
        case COMPONENT_PIN_DRAW_TYPE:
            ( (LibDrawPin*) item )->m_Pos.x += offset.x;
            ( (LibDrawPin*) item )->m_Pos.y += offset.y;
            break;

        case COMPONENT_ARC_DRAW_TYPE:
        {
            ( (LibDrawArc*) item )->m_Pos.x      += offset.x;
            ( (LibDrawArc*) item )->m_Pos.y      += offset.y;
            ( (LibDrawArc*) item )->m_ArcStart.x += offset.x;
            ( (LibDrawArc*) item )->m_ArcStart.y += offset.y;
            ( (LibDrawArc*) item )->m_ArcEnd.x   += offset.x;
            ( (LibDrawArc*) item )->m_ArcEnd.y   += offset.y;
            break;
        }

        case COMPONENT_CIRCLE_DRAW_TYPE:
            ( (LibDrawCircle*) item )->m_Pos.x += offset.x;
            ( (LibDrawCircle*) item )->m_Pos.y += offset.y;
            break;

        case COMPONENT_RECT_DRAW_TYPE:
            ( (LibDrawSquare*) item )->m_Pos.x += offset.x;
            ( (LibDrawSquare*) item )->m_Pos.y += offset.y;
            ( (LibDrawSquare*) item )->m_End.x += offset.x;
            ( (LibDrawSquare*) item )->m_End.y += offset.y;
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
        {
            int ii, imax = ( (LibDrawPolyline*) item )->n * 2;
            int* ptpoly = ( (LibDrawPolyline*) item )->PolyList;
            for( ii = 0; ii < imax; ii += 2 )
            {
                ptpoly[ii]     += offset.x;
                ptpoly[ii + 1] += offset.y;
            }
        }
            break;

        case COMPONENT_LINE_DRAW_TYPE:
            break;

        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
            ( (LibDrawText*) item )->m_Pos.x += offset.x;
            ( (LibDrawText*) item )->m_Pos.y += offset.y;
            break;
        }

        item->m_Flags = item->m_Selected = 0;
    }
}


/******************************************************/
void DeleteMarkedItems( EDA_LibComponentStruct* LibEntry )
/******************************************************/

/* Delete marked items
 */
{
    LibEDA_BaseStruct* item, * next_item;

    if( LibEntry == NULL )
        return;

    item = LibEntry->m_Drawings;
    for( ; item != NULL; item = next_item )
    {
        next_item = item->Next();
        if( item->m_Selected == 0 )
            continue;
        DeleteOneLibraryDrawStruct( NULL, NULL, LibEntry, item, 0 );
    }
}


/****************************************************************************/
void MirrorMarkedItems( EDA_LibComponentStruct* LibEntry, wxPoint offset )
/****************************************************************************/

/* Mirror marked items, refer to a Vertical axis at position offset
 */
{
#define SETMIRROR( z ) (z) -= offset.x; (z) = -(z); (z) += offset.x;
    LibEDA_BaseStruct* item;

    if( LibEntry == NULL )
        return;

    offset.y = -offset.y;  // Y axis for lib items is Down to Up: reverse y offset value
    item = LibEntry->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        if( item->m_Selected == 0 )
            continue;

        switch( item->m_StructType )
        {
        case COMPONENT_PIN_DRAW_TYPE:
            SETMIRROR( ( (LibDrawPin*) item )->m_Pos.x );

            switch( ( (LibDrawPin*) item )->m_Orient )
            {
            case PIN_RIGHT:
                ( (LibDrawPin*) item )->m_Orient = PIN_LEFT;
                break;

            case PIN_LEFT:
                ( (LibDrawPin*) item )->m_Orient = PIN_RIGHT;
                break;

            case PIN_UP:
            case PIN_DOWN:
                break;
            }

            break;

        case COMPONENT_ARC_DRAW_TYPE:
        {
            SETMIRROR( ( (LibDrawArc*) item )->m_Pos.x );
            SETMIRROR( ( (LibDrawArc*) item )->m_ArcStart.x );
            SETMIRROR( ( (LibDrawArc*) item )->m_ArcEnd.x );
            EXCHG( ( (LibDrawArc*) item )->m_ArcStart, ( (LibDrawArc*) item )->m_ArcEnd );
            break;
        }

        case COMPONENT_CIRCLE_DRAW_TYPE:
            SETMIRROR( ( (LibDrawCircle*) item )->m_Pos.x );
            break;

        case COMPONENT_RECT_DRAW_TYPE:
            SETMIRROR( ( (LibDrawSquare*) item )->m_Pos.x );
            SETMIRROR( ( (LibDrawSquare*) item )->m_End.x );
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
        {
            int ii, imax = ( (LibDrawPolyline*) item )->n * 2;
            int* ptpoly = ( (LibDrawPolyline*) item )->PolyList;
            for( ii = 0; ii < imax; ii += 2 )
            {
                SETMIRROR( ptpoly[ii] );
            }
        }
            break;

        case COMPONENT_LINE_DRAW_TYPE:
            break;

        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
            SETMIRROR( ( (LibDrawText*) item )->m_Pos.x );
            break;
        }

        item->m_Flags = item->m_Selected = 0;
    }
}
