/**
 * @file gerbview/block.cpp
 * @brief Block operations: displacement.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 <Jean-Pierre Charras>
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
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


#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gr_basic.h"

#include "gerbview.h"
#include "class_gerber_draw_item.h"

#include "wx/debug.h"

#define BLOCK_COLOR BROWN


static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool erase );

/* Return the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
 *  the key (ALT, SHIFT ALT ..)
 * Currently, only block move and block zoom is supported
 */
int GERBVIEW_FRAME::ReturnBlockCommand( int key )
{
    int cmd = 0;

    switch( key )
    {
    default:
        cmd = key & 0x255;
        break;

    case 0:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_SHIFT:
    case GR_KB_CTRL:
    case GR_KB_SHIFTCTRL:
    case GR_KB_ALT:
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


/* Routine to handle the BLOCK PLACE command */
void GERBVIEW_FRAME::HandleBlockPlace( wxDC* DC )
{
    wxASSERT( DrawPanel->IsMouseCaptured() );

    GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;

    switch( GetScreen()->m_BlockLocate.m_Command )
    {
    case BLOCK_MOVE:                /* Move */
        if( DrawPanel->IsMouseCaptured() )
            DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );

        Block_Move( DC );
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_COPY:     /* Copy */
        if( DrawPanel->IsMouseCaptured() )
            DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );

        Block_Duplicate( DC );
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_PASTE:
    case BLOCK_DRAG:
    case BLOCK_PRESELECT_MOVE:
    case BLOCK_ZOOM:
    case BLOCK_ROTATE:
    case BLOCK_FLIP:
    case BLOCK_DELETE:
    case BLOCK_SAVE:
    case BLOCK_ABORT:
    case BLOCK_SELECT_ITEMS_ONLY:
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
    case BLOCK_IDLE:
        wxFAIL_MSG( wxT("HandleBlockPlace: Unexpected block command") );
        break;
    }

    DrawPanel->SetMouseCapture( NULL, NULL );
    DrawPanel->EndMouseCapture( );
    SetToolID( GetToolId(), DrawPanel->GetCurrentCursor(), wxEmptyString );
    GetScreen()->SetModify();
    GetScreen()->ClearBlockCommand();

    wxASSERT( GetScreen()->m_BlockLocate.GetCount() == 0 );

    DisplayToolMsg( wxEmptyString );
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
bool GERBVIEW_FRAME::HandleBlockEnd( wxDC* DC )
{
    bool nextcmd  = false;
    bool zoom_command = false;

    if( DrawPanel->IsMouseCaptured() )

        switch( GetScreen()->m_BlockLocate.m_Command )
        {
        case BLOCK_MOVE:            /* Move */
        case BLOCK_COPY:            /* Copy */
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_MOVE;
            nextcmd = true;
            DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );
            DrawPanel->m_mouseCaptureCallback = DrawMovingBlockOutlines;
            DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );
            break;

        case BLOCK_DELETE: /* Delete */
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );
            Block_Delete( DC );
            break;

        case BLOCK_ZOOM: /* Window Zoom */
            zoom_command = true;
            break;

        case BLOCK_PRESELECT_MOVE:  /* Move with preselection list */
        case BLOCK_DRAG:
        case BLOCK_IDLE:
        case BLOCK_MIRROR_X:    /* Mirror, unused*/
        case BLOCK_ROTATE:      /* Unused */
        case BLOCK_FLIP:        /* Flip, unused */
        case BLOCK_SAVE:        /* Save (not used)*/
        case BLOCK_PASTE:
        case BLOCK_ABORT:
        case BLOCK_SELECT_ITEMS_ONLY:
        case BLOCK_MIRROR_Y:
            wxFAIL_MSG( wxT("HandleBlockEnd: Unexpected block command") );
            break;
        }

    if( ! nextcmd )
    {
        GetScreen()->ClearBlockCommand();
        DrawPanel->SetMouseCapture( NULL, NULL );
        DrawPanel->EndMouseCapture( );
        SetToolID( GetToolId(), DrawPanel->GetCurrentCursor(), wxEmptyString );
        DisplayToolMsg( wxEmptyString );
    }

    if( zoom_command )
        Window_Zoom( GetScreen()->m_BlockLocate );

    return nextcmd ;
}


/* Traces the outline of the block structures of a repositioning move
 */
static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPositon,
                                     bool aErase )
{
    int          Color;
    BASE_SCREEN* screen = aPanel->GetScreen();

    Color = YELLOW;

    if( aErase )
    {
        screen->m_BlockLocate.Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, Color );

        if( screen->m_BlockLocate.m_MoveVector.x|| screen->m_BlockLocate.m_MoveVector.y )
        {
            screen->m_BlockLocate.Draw( aPanel,
                                        aDC,
                                        screen->m_BlockLocate.m_MoveVector,
                                        g_XorMode,
                                        Color );
        }
    }

    if( screen->m_BlockLocate.m_State != STATE_BLOCK_STOP )
    {
        screen->m_BlockLocate.m_MoveVector.x = screen->GetCrossHairPosition().x -
                                               screen->m_BlockLocate.GetRight();
        screen->m_BlockLocate.m_MoveVector.y = screen->GetCrossHairPosition().y -
                                               screen->m_BlockLocate.GetBottom();
    }

    screen->m_BlockLocate.Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, Color );

    if( screen->m_BlockLocate.m_MoveVector.x || screen->m_BlockLocate.m_MoveVector.y )
    {
        screen->m_BlockLocate.Draw( aPanel,
                                    aDC,
                                    screen->m_BlockLocate.m_MoveVector,
                                    g_XorMode,
                                    Color );
    }
}


/*
 * Erase the selected block.
 */
void GERBVIEW_FRAME::Block_Delete( wxDC* DC )
{
    if( !IsOK( this, _( "Ok to delete block ?" ) ) )
        return;

    GetScreen()->SetModify();
    GetScreen()->m_BlockLocate.Normalize();
    GetScreen()->SetCurItem( NULL );

    BOARD_ITEM* item = GetBoard()->m_Drawings;
    BOARD_ITEM* nextitem;
    for( ; item; item = nextitem )
    {
        nextitem = item->Next();
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;
        if( gerb_item->HitTest( GetScreen()->m_BlockLocate ) )
            gerb_item->DeleteStructure();
    }

    Refresh();
}


/*
 *  Function to move items in the current selected block
 */
void GERBVIEW_FRAME::Block_Move( wxDC* DC )
{
    wxPoint delta;
    wxPoint oldpos;

    oldpos = GetScreen()->GetCrossHairPosition();
    DrawPanel->m_mouseCaptureCallback = NULL;

    GetScreen()->SetCrossHairPosition( oldpos );
    DrawPanel->MoveCursorToCrossHair();
    GetScreen()->SetModify();
    GetScreen()->m_BlockLocate.Normalize();

    /* Calculate displacement vectors. */
    delta = GetScreen()->m_BlockLocate.m_MoveVector;

    /* Move items in block */
    BOARD_ITEM* item = GetBoard()->m_Drawings;
    for( ; item; item = item->Next() )
    {
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;
        if( gerb_item->HitTest( GetScreen()->m_BlockLocate ) )
            gerb_item->MoveAB( delta );
    }

    DrawPanel->Refresh( true );
}


/*
 *  Function to duplicate items in the current selected block
 */
void GERBVIEW_FRAME::Block_Duplicate( wxDC* DC )
{
    wxPoint delta;
    wxPoint oldpos;

    oldpos = GetScreen()->GetCrossHairPosition();
    DrawPanel->m_mouseCaptureCallback = NULL;

    GetScreen()->SetCrossHairPosition( oldpos );
    DrawPanel->MoveCursorToCrossHair();
    GetScreen()->SetModify();
    GetScreen()->m_BlockLocate.Normalize();

    delta = GetScreen()->m_BlockLocate.m_MoveVector;

    /* Copy items in block */
    BOARD_ITEM* item = GetBoard()->m_Drawings;
    for( ; item; item = item->Next() )
    {
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;
        if( gerb_item->HitTest( GetScreen()->m_BlockLocate ) )
        {
            /* this item must be duplicated */
            GERBER_DRAW_ITEM* new_item = gerb_item->Copy();
            new_item->MoveAB( delta );
            GetBoard()->m_Drawings.PushFront( new_item );
        }
    }

    DrawPanel->Refresh();
}

