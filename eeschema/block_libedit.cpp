/****************************************************/
/*  block_libedit.cpp                                */
/* Gestion des Operations sur Blocks et Effacements */
/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "block_commande.h"

#include "program.h"
#include "general.h"
#include "class_library.h"
#include "protos.h"
#include "libeditfrm.h"


static void DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC,
                                     bool erase );
static void MirrorMarkedItems( LIB_COMPONENT* LibEntry, wxPoint offset );


/*
 * Return the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
 *  the key (ALT, SHIFT ALT ..)
 */
int WinEDA_LibeditFrame::ReturnBlockCommand( int key )
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
        cmd = BLOCK_MIRROR_Y;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


/*
 * Command BLOCK END (end of block sizing)
 *  return :
 *  0 if command finished (zoom, delete ...)
 *  1 if HandleBlockPlace must follow (items found, and a block place
 * command must follow)
 */
int WinEDA_LibeditFrame::HandleBlockEnd( wxDC* DC )
{
    int ItemCount = 0;
    int MustDoPlace = 0;

    if( GetScreen()->m_BlockLocate.GetCount() )
    {
        BlockState state     = GetScreen()->m_BlockLocate.m_State;
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
        ItemCount = m_component->SelectItems( GetScreen()->m_BlockLocate,
                                              m_unit, m_convert,
                                              g_EditPinByPinIsOn );
        if( ItemCount )
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
        ItemCount = m_component->SelectItems( GetScreen()->m_BlockLocate,
                                              m_unit, m_convert,
                                              g_EditPinByPinIsOn );
        if( ItemCount )
            SaveCopyInUndoList( m_component );
        m_component->DeleteSelectedItems();
        break;

    case BLOCK_SAVE:     /* Save */
    case BLOCK_PASTE:
    case BLOCK_ROTATE:
    case BLOCK_MIRROR_X:
    case BLOCK_FLIP:
        break;


    case BLOCK_MIRROR_Y:
        ItemCount = m_component->SelectItems( GetScreen()->m_BlockLocate,
                                              m_unit, m_convert,
                                              g_EditPinByPinIsOn );
        if( ItemCount )
            SaveCopyInUndoList( m_component );
        MirrorMarkedItems( m_component, GetScreen()->m_BlockLocate.Centre() );
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
            m_component->ClearSelectedItems();

        GetScreen()->m_BlockLocate.m_Flags   = 0;
        GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        GetScreen()->SetCurItem( NULL );
        SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor,
                   wxEmptyString );
        DrawPanel->Refresh( TRUE );
    }


    return MustDoPlace;
}


/*
 * Routine to handle the BLOCK PLACE commande
 *  Last routine for block operation for:
 *  - block move & drag
 *  - block copie & paste
 */
void WinEDA_LibeditFrame::HandleBlockPlace( wxDC* DC )
{
    bool err = FALSE;
    wxPoint offset;

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
        SaveCopyInUndoList( m_component );
        offset = GetScreen()->m_BlockLocate.m_MoveVector;
        offset.y *= -1;
        m_component->MoveSelectedItems( offset );
        DrawPanel->Refresh( TRUE );
        break;

    case BLOCK_COPY:     /* Copy */
        GetScreen()->m_BlockLocate.ClearItemsList();
        SaveCopyInUndoList( m_component );
        offset = GetScreen()->m_BlockLocate.m_MoveVector;
        offset.y *= -1;
        m_component->CopySelectedItems( offset );
        break;

    case BLOCK_PASTE:     /* Paste (recopie du dernier bloc sauve */
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_MIRROR_Y:      /* Invert by popup menu, from block move */
        SaveCopyInUndoList( m_component );
        MirrorMarkedItems( m_component, GetScreen()->m_BlockLocate.Centre() );
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

    DrawPanel->ManageCurseur             = NULL;
    DrawPanel->ForceCloseManageCurseur   = NULL;
    GetScreen()->m_BlockLocate.m_Flags   = 0;
    GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
    GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
    GetScreen()->SetCurItem( NULL );
    DrawPanel->Refresh( TRUE );

    SetToolID( m_ID_current_state, DrawPanel->m_PanelDefaultCursor,
               wxEmptyString );
}


/*
 * Retrace le contour du block de recherche de structures
 *  L'ensemble du block suit le curseur
 */
void DrawMovingBlockOutlines( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    BLOCK_SELECTOR* PtBlock;
    BASE_SCREEN* screen = panel->GetScreen();
    wxPoint move_offset;
    PtBlock = &screen->m_BlockLocate;

    WinEDA_LibeditFrame* parent = ( WinEDA_LibeditFrame* ) panel->GetParent();
    wxASSERT( parent != NULL );

    LIB_COMPONENT* component = parent->GetComponent();

    if( component == NULL )
        return;

    int unit = parent->GetUnit();
    int convert = parent->GetConvert();

    if( erase )
    {
        PtBlock->Draw( panel, DC, PtBlock->m_MoveVector, g_XorMode,
                       PtBlock->m_Color );

        component->Draw( panel, DC, PtBlock->m_MoveVector, unit, convert,
                         g_XorMode, -1, DefaultTransformMatrix,
                         true, true, true );
    }

    /* Redessin nouvel affichage */
    PtBlock->m_MoveVector.x =
        screen->m_Curseur.x - PtBlock->m_BlockLastCursorPosition.x;
    PtBlock->m_MoveVector.y =
        screen->m_Curseur.y - PtBlock->m_BlockLastCursorPosition.y;

    GRSetDrawMode( DC, g_XorMode );
    PtBlock->Draw( panel, DC, PtBlock->m_MoveVector, g_XorMode,
                   PtBlock->m_Color );

    component->Draw( panel, DC, PtBlock->m_MoveVector, unit, convert,
                     g_XorMode, -1, DefaultTransformMatrix,
                     true, true, true );
}


/*
 * Mirror marked items, refer to a Vertical axis at position offset
 */
void MirrorMarkedItems( LIB_COMPONENT* LibEntry, wxPoint offset )
{
#define SETMIRROR( z ) (z) -= offset.x; (z) = -(z); (z) += offset.x;
    LIB_DRAW_ITEM* item;

    if( LibEntry == NULL )
        return;

    offset.y = -offset.y;  // Y axis for lib items is Down to Up: reverse y offset value
    item = LibEntry->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        if( item->m_Selected == 0 )
            continue;

        switch( item->Type() )
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
            EXCHG( ( (LibDrawArc*) item )->m_ArcStart,
                   ( (LibDrawArc*) item )->m_ArcEnd );
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
            unsigned ii, imax = ( (LibDrawPolyline*) item )->GetCornerCount();
            for( ii = 0; ii < imax; ii ++ )
            {
                SETMIRROR( ( (LibDrawPolyline*) item )->m_PolyPoints[ii].x );
            }
        }
            break;

        case COMPONENT_LINE_DRAW_TYPE:
            break;

        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
            SETMIRROR( ( (LibDrawText*) item )->m_Pos.x );
            break;

        default:
            break;
        }

        item->m_Flags = item->m_Selected = 0;
    }
}
