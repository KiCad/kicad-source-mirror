/**********************************************************/
/* Block operations: displacement, rotation, deletion ... */
/**********************************************************/

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 <Jean-Pierre Charras>
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "trigo.h"
#include "confirm.h"

#include "gerbview.h"
#include "class_gerber_draw_item.h"


#define BLOCK_COLOR BROWN


static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* panel, wxDC* DC, bool erase );

/* Return the block command (BLOCK_MOVE, BLOCK_COPY...) corresponding to
 *  the key (ALT, SHIFT ALT ..)
 */
int WinEDA_GerberFrame::ReturnBlockCommand( int key )
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
        break;

    case GR_KB_CTRL:
        break;

    case GR_KB_SHIFTCTRL:
        cmd = BLOCK_DELETE;
        break;

    case GR_KB_ALT:
        cmd = BLOCK_COPY;
        break;

    case MOUSE_MIDDLE:
        cmd = BLOCK_ZOOM;
        break;
    }

    return cmd;
}


/* Routine to handle the BLOCK PLACE command */
void WinEDA_GerberFrame::HandleBlockPlace( wxDC* DC )
{
    bool err = false;

    if( DrawPanel->ManageCurseur == NULL )
    {
        err = true;
        DisplayError( this,
                     wxT( "Error in HandleBlockPLace : ManageCurseur = NULL" ) );
    }
    GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;

    switch( GetScreen()->m_BlockLocate.m_Command )
    {
    case BLOCK_IDLE:
        err = true;
        break;

    case BLOCK_DRAG:                /* Drag */
    case BLOCK_MOVE:                /* Move */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );
        Block_Move( DC );
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_COPY:     /* Copy */
        if( DrawPanel->ManageCurseur )
            DrawPanel->ManageCurseur( DrawPanel, DC, false );
        Block_Duplicate( DC );
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_PASTE:
        break;

    case BLOCK_ZOOM:        // Handle by HandleBlockEnd()
    case BLOCK_ROTATE:
    case BLOCK_FLIP:
    case BLOCK_DELETE:
    case BLOCK_SAVE:
    case BLOCK_ABORT:
    case BLOCK_SELECT_ITEMS_ONLY:
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
        break;
    }

    GetScreen()->SetModify();

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur   = NULL;
    GetScreen()->m_BlockLocate.m_Flags   = 0;
    GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
    GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
    if( GetScreen()->m_BlockLocate.GetCount() )
    {
        DisplayError( this, wxT( "HandleBlockPLace error: some items left" ) );
        GetScreen()->m_BlockLocate.ClearItemsList();
    }

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
bool WinEDA_GerberFrame::HandleBlockEnd( wxDC* DC )
{
    bool nextcmd  = false;
    bool zoom_command = false;

    if( DrawPanel->ManageCurseur )

        switch( GetScreen()->m_BlockLocate.m_Command )
        {
        case BLOCK_IDLE:
            DisplayError( this, wxT( "Error in HandleBlockPLace" ) );
            break;

        case BLOCK_DRAG:            /* Drag (not used, for future
                                     * enhancements) */
        case BLOCK_MOVE:            /* Move */
        case BLOCK_COPY:            /* Copy */
        case BLOCK_PRESELECT_MOVE:  /* Move with preselection list */
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_MOVE;
            nextcmd = true;
            DrawPanel->ManageCurseur( DrawPanel, DC, false );
            DrawPanel->ManageCurseur = DrawMovingBlockOutlines;
            DrawPanel->ManageCurseur( DrawPanel, DC, false );
            break;

        case BLOCK_DELETE: /* Delete */
            GetScreen()->m_BlockLocate.m_State = STATE_BLOCK_STOP;
            DrawPanel->ManageCurseur( DrawPanel, DC, false );
            Block_Delete( DC );
            break;

        case BLOCK_MIRROR_X:    /* Mirror, unused*/
        case BLOCK_ROTATE:      /* Unused */
        case BLOCK_FLIP:        /* Flip, unused */
        case BLOCK_SAVE:        /* Save (not used)*/
        case BLOCK_PASTE:
            break;

        case BLOCK_ZOOM: /* Window Zoom */
            zoom_command = true;
            break;

        case BLOCK_ABORT:
        case BLOCK_SELECT_ITEMS_ONLY:
        case BLOCK_MIRROR_Y:
            break;
        }

    if( ! nextcmd )
    {
        GetScreen()->m_BlockLocate.m_Flags   = 0;
        GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
        GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
        GetScreen()->m_BlockLocate.ClearItemsList();
        DrawPanel->ManageCurseur = NULL;
        DrawPanel->ForceCloseManageCurseur = NULL;
        DisplayToolMsg( wxEmptyString );
    }

    if( zoom_command )
        Window_Zoom( GetScreen()->m_BlockLocate );

    return nextcmd ;
}


/* Traces the outline of the block structures of a repositioning move
 */
static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* panel, wxDC* DC, bool erase )
{
    int          Color;
    BASE_SCREEN* screen = panel->GetScreen();

    Color = YELLOW;

    if( erase )
    {
        screen->m_BlockLocate.Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode,
                                    Color );
        if( screen->m_BlockLocate.m_MoveVector.x
            || screen->m_BlockLocate.m_MoveVector.y )
        {
            screen->m_BlockLocate.Draw( panel,
                                        DC,
                                        screen->m_BlockLocate.m_MoveVector,
                                        g_XorMode,
                                        Color );
        }
    }

    if( panel->GetScreen()->m_BlockLocate.m_State != STATE_BLOCK_STOP )
    {
        screen->m_BlockLocate.m_MoveVector.x = screen->m_Curseur.x -
                                               screen->m_BlockLocate.GetRight();
        screen->m_BlockLocate.m_MoveVector.y = screen->m_Curseur.y -
                                               screen->m_BlockLocate.GetBottom();
    }

    screen->m_BlockLocate.Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode, Color );
    if( screen->m_BlockLocate.m_MoveVector.x
        || screen->m_BlockLocate.m_MoveVector.y )
    {
        screen->m_BlockLocate.Draw( panel,
                                    DC,
                                    screen->m_BlockLocate.m_MoveVector,
                                    g_XorMode,
                                    Color );
    }
}


/*
 * Erase the selected block.
 */
void WinEDA_GerberFrame::Block_Delete( wxDC* DC )
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
void WinEDA_GerberFrame::Block_Move( wxDC* DC )
{
    wxPoint delta;
    wxPoint oldpos;

    oldpos = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = NULL;

    GetScreen()->m_Curseur = oldpos;
    DrawPanel->MouseToCursorSchema();
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
void WinEDA_GerberFrame::Block_Duplicate( wxDC* DC )
{
    wxPoint delta;
    wxPoint oldpos;

    oldpos = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = NULL;

    GetScreen()->m_Curseur = oldpos;
    DrawPanel->MouseToCursorSchema();
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

