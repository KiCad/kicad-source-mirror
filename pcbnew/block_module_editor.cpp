/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file block_module_editor.cpp
 * @brief Footprint editor block handling implementation.
 */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "block_commande.h"
#include "macros.h"

#include "wxPcbStruct.h"
#include "module_editor_frame.h"
#include "pcbplot.h"
#include "trigo.h"

#include "pcbnew.h"
#include "protos.h"

#include "class_board.h"
#include "class_track.h"
#include "class_drawsegment.h"
#include "class_pcb_text.h"
#include "class_mire.h"
#include "class_module.h"
#include "class_dimension.h"
#include "class_edge_mod.h"


#define BLOCK_COLOR BROWN
#define IS_SELECTED 1


static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase );
static int  MarkItemsInBloc( MODULE* module, EDA_RECT& Rect );

static void ClearMarkItems( MODULE* module );
static void CopyMarkedItems( MODULE* module, wxPoint offset );
static void MoveMarkedItems( MODULE* module, wxPoint offset );
static void MirrorMarkedItems( MODULE* module, wxPoint offset );
static void RotateMarkedItems( MODULE* module, wxPoint offset );
static void DeleteMarkedItems( MODULE* module );


int FOOTPRINT_EDIT_FRAME::ReturnBlockCommand( int key )
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


bool FOOTPRINT_EDIT_FRAME::HandleBlockEnd( wxDC* DC )
{
    int  itemsCount    = 0;
    bool nextcmd = false;
    MODULE* currentModule = GetBoard()->m_Modules;

    if( GetScreen()->m_BlockLocate.GetCount() )
    {
        BlockState   state   = GetScreen()->m_BlockLocate.m_State;
        CmdBlockType command = GetScreen()->m_BlockLocate.m_Command;
        DrawPanel->m_endMouseCaptureCallback( DrawPanel, DC );
        GetScreen()->m_BlockLocate.m_State   = state;
        GetScreen()->m_BlockLocate.m_Command = command;
        DrawPanel->SetMouseCapture( DrawAndSizingBlockOutlines, AbortBlockCurrentCommand );
        GetScreen()->SetCrossHairPosition( wxPoint(  GetScreen()->m_BlockLocate.GetRight(),
                                                     GetScreen()->m_BlockLocate.GetBottom() ) );
        DrawPanel->MoveCursorToCrossHair();
    }

    switch( GetScreen()->m_BlockLocate.m_Command )
    {
    case  BLOCK_IDLE:
        DisplayError( this, wxT( "Error in HandleBlockPLace" ) );
        break;

    case BLOCK_DRAG:        /* Drag */
    case BLOCK_MOVE:        /* Move */
    case BLOCK_COPY:        /* Copy */
        itemsCount = MarkItemsInBloc( currentModule, GetScreen()->m_BlockLocate );

        if( itemsCount )
        {
            nextcmd = true;

            if( DrawPanel->IsMouseCaptured() )
            {
                DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );
                DrawPanel->m_mouseCaptureCallback = DrawMovingBlockOutlines;
                DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );
            }

            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_MOVE;
            DrawPanel->Refresh( true );
        }
        break;

    case BLOCK_PRESELECT_MOVE:     /* Move with preselection list*/
        nextcmd = true;
        DrawPanel->m_mouseCaptureCallback = DrawMovingBlockOutlines;
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

        GetScreen()->ClearBlockCommand();
        SetCurItem( NULL );
        DrawPanel->EndMouseCapture( GetToolId(), DrawPanel->GetCurrentCursor(), wxEmptyString,
                                    false );
        DrawPanel->Refresh( true );
    }

    return nextcmd;
}


void FOOTPRINT_EDIT_FRAME::HandleBlockPlace( wxDC* DC )
{
    MODULE* currentModule = GetBoard()->m_Modules;

    if( !DrawPanel->IsMouseCaptured() )
    {
        DisplayError( this, wxT( "HandleBlockPLace : m_mouseCaptureCallback = NULL" ) );
    }

    GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;

    switch( GetScreen()->m_BlockLocate.m_Command )
    {
    case  BLOCK_IDLE:
        break;

    case BLOCK_DRAG:                /* Drag */
    case BLOCK_MOVE:                /* Move */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        GetScreen()->m_BlockLocate.ClearItemsList();
        SaveCopyInUndoList( currentModule, UR_MODEDIT );
        MoveMarkedItems( currentModule, GetScreen()->m_BlockLocate.m_MoveVector );
        DrawPanel->Refresh( true );
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

    GetScreen()->m_BlockLocate.m_Flags   = 0;
    GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
    GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
    SetCurItem( NULL );
    DrawPanel->EndMouseCapture( GetToolId(), DrawPanel->GetCurrentCursor(), wxEmptyString, false );
    DrawPanel->Refresh( true );
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
        ( (PCB_BASE_FRAME*) wxGetApp().GetTopWindow() )->m_ModuleEditFrame->GetBoard()->m_Modules;

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
                case PCB_MODULE_TEXT_T:
                case PCB_MODULE_EDGE_T:
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
    PtBlock->m_MoveVector = screen->GetCrossHairPosition() - PtBlock->m_BlockLastCursorPosition;

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
            case PCB_MODULE_TEXT_T:
            case PCB_MODULE_EDGE_T:
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
        case PCB_MODULE_TEXT_T:
            TEXTE_MODULE * textm;
            textm = new TEXTE_MODULE( module );
            textm->Copy( (TEXTE_MODULE*) item );
            textm->m_Selected = IS_SELECTED;
            module->m_Drawings.PushFront( textm );
            break;

        case PCB_MODULE_EDGE_T:
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
        pad->m_Pos0 += FROM_LEGACY_LU_VEC( offset );
    }

    item = module->m_Drawings;

    for( ; item != NULL; item = item->Next() )
    {
        if( item->m_Selected == 0 )
            continue;

        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
            ( (TEXTE_MODULE*) item )->m_Pos += offset;
            ( (TEXTE_MODULE*) item )->m_Pos0 += offset;
            break;

        case PCB_MODULE_EDGE_T:
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
        pad->m_Pos0.x() = FROM_LEGACY_LU( pad->GetPosition().x );
        NEGATE( pad->m_Offset.x() );
        NEGATE( pad->m_DeltaSize.x() );
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
        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE * edge =  (EDGE_MODULE*) item;
            SETMIRROR( edge->m_Start.x );
            edge->m_Start0.x = edge->m_Start.x;
            SETMIRROR( edge->m_End.x );
            edge->m_End0.x = edge->m_End.x;
            NEGATE( edge->m_Angle );
        }
            break;

        case PCB_MODULE_TEXT_T:
            SETMIRROR( ( (TEXTE_MODULE*) item )->GetPosition().x );
            ( (TEXTE_MODULE*) item )->m_Pos0.x = ( (TEXTE_MODULE*) item )->GetPosition().x;
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
        pad->m_Pos0    = FROM_LEGACY_LU_VEC( pad->GetPosition() );
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
        case PCB_MODULE_EDGE_T:
            ROTATE( ( (EDGE_MODULE*) item )->m_Start );
            ( (EDGE_MODULE*) item )->m_Start0 = ( (EDGE_MODULE*) item )->m_Start;
            ROTATE( ( (EDGE_MODULE*) item )->m_End );
            ( (EDGE_MODULE*) item )->m_End0 = ( (EDGE_MODULE*) item )->m_End;
            break;

        case PCB_MODULE_TEXT_T:
            ROTATE( ( (TEXTE_MODULE*) item )->GetPosition() );
            ( (TEXTE_MODULE*) item )->m_Pos0 = ( (TEXTE_MODULE*) item )->GetPosition();
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
int MarkItemsInBloc( MODULE* module, EDA_RECT& Rect )
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
        case PCB_MODULE_EDGE_T:
            if( ((EDGE_MODULE*)item )->HitTest( Rect ) )
            {
                item->m_Selected = IS_SELECTED;
                ItemsCount++;
            }

            break;

        case PCB_MODULE_TEXT_T:
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
