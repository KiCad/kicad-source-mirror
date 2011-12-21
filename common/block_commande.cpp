/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file block_commande.cpp
 * @brief Common routines for managing on block commands.
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "wxstruct.h"
#include "common.h"
#include "macros.h"
#include "base_struct.h"
#include "class_base_screen.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "block_commande.h"


BLOCK_SELECTOR::BLOCK_SELECTOR() :
    EDA_ITEM( BLOCK_LOCATE_STRUCT_TYPE ),
    EDA_RECT()
{
    m_State   = STATE_NO_BLOCK; /* State (enum BlockState) of block. */
    m_Command = BLOCK_IDLE;     /* Type (enum CmdBlockType) of operation. */
    m_Color   = BROWN;
}


BLOCK_SELECTOR::~BLOCK_SELECTOR()
{
}


/*
 *  Print block command message (Block move, Block copy ...) in status bar
 */
void BLOCK_SELECTOR::SetMessageBlock( EDA_DRAW_FRAME* frame )
{
    wxString msg;

    switch( m_Command )
    {
    case BLOCK_IDLE:
        break;

    case BLOCK_MOVE:                /* Move */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        msg = _( "Block Move" );
        break;

    case BLOCK_DRAG:     /* Drag */
        msg = _( "Block Drag" );
        break;

    case BLOCK_COPY:     /* Copy */
        msg = _( "Block Copy" );
        break;

    case BLOCK_DELETE:     /* Delete */
        msg = _( "Block Delete" );
        break;

    case BLOCK_SAVE:     /* Save */
        msg = _( "Block Save" );
        break;

    case BLOCK_PASTE:
        msg = _( "Block Paste" );
        break;

    case BLOCK_ZOOM:     /* Window Zoom */
        msg = _( "Win Zoom" );
        break;

    case BLOCK_ROTATE:     /* Rotate 90 deg */
        msg = _( "Block Rotate" );
        break;

    case BLOCK_FLIP:     /* Flip */
        msg = _( "Block Flip" );
        break;

    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:     /* mirror */
        msg = _( "Block Mirror" );
        break;

    case BLOCK_ABORT:
        break;

    default:
        msg = wxT( "????" );
        break;
    }

    frame->DisplayToolMsg( msg );
}


void BLOCK_SELECTOR::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                           int aDrawMode, int aColor )
{

    int w = GetWidth();
    int h = GetHeight();

    GRSetDrawMode( aDC, aDrawMode );

    if(  w == 0 || h == 0 )
        GRLine( &aPanel->m_ClipBox, aDC, GetX() + aOffset.x, GetY() + aOffset.y,
                GetRight() + aOffset.x, GetBottom() + aOffset.y, 0, aColor );
    else
        GRRect( &aPanel->m_ClipBox, aDC, GetX() + aOffset.x, GetY() + aOffset.y,
                GetRight() + aOffset.x, GetBottom() + aOffset.y, 0, aColor );
}


/**
 * Function InitData
 *  Init the initial values of a BLOCK_SELECTOR, before starting a block command
 */
void BLOCK_SELECTOR::InitData( EDA_DRAW_PANEL* aPanel, const wxPoint& startpos )
{
    m_State = STATE_BLOCK_INIT;
    SetOrigin( startpos );
    SetSize( wxSize( 0, 0 ) );
    m_ItemsSelection.ClearItemsList();
    aPanel->SetMouseCapture( DrawAndSizingBlockOutlines, AbortBlockCurrentCommand );
}


/**
 * Function ClearItemsList
 * delete only the list of EDA_ITEM * pointers, NOT the pointed data
 * itself
 */
void BLOCK_SELECTOR::ClearItemsList()
{
    m_ItemsSelection.ClearItemsList();
}

/**
 * Function ClearListAndDeleteItems
 * delete only the list of EDA_ITEM * pointers, AND the data pinted
 * by m_Item
 */
void BLOCK_SELECTOR::ClearListAndDeleteItems()
{
     m_ItemsSelection.ClearListAndDeleteItems();
}

/**
 * Function PushItem
 * Add aItem to the list of items
 * @param aItem = an ITEM_PICKER to add to the list
 */
void BLOCK_SELECTOR::PushItem( ITEM_PICKER& aItem )
{
    m_ItemsSelection.PushItem( aItem );
}


void BLOCK_SELECTOR::Clear()
{
    if( m_Command != BLOCK_IDLE )
    {
        m_Flags   = 0;
        m_Command = BLOCK_IDLE;
        m_State   = STATE_NO_BLOCK;
        ClearItemsList();
    }
}


/*  First command block function:
 *  Init the Block infos: command type, initial position, and other variables..
 */
bool EDA_DRAW_FRAME::HandleBlockBegin( wxDC* DC, int key, const wxPoint& startpos )
{
    BLOCK_SELECTOR* Block = &GetScreen()->m_BlockLocate;

    if( ( Block->m_Command != BLOCK_IDLE ) || ( Block->m_State != STATE_NO_BLOCK ) )
        return false;

    Block->ClearFlags();
    Block->m_Command = (CmdBlockType) ReturnBlockCommand( key );

    if( Block->m_Command == 0 )
        return false;

    switch( Block->m_Command )
    {
    case BLOCK_IDLE:
        break;

    case BLOCK_MOVE:                /* Move */
    case BLOCK_DRAG:                /* Drag */
    case BLOCK_COPY:                /* Copy */
    case BLOCK_DELETE:              /* Delete */
    case BLOCK_SAVE:                /* Save */
    case BLOCK_ROTATE:              /* Rotate 90 deg */
    case BLOCK_FLIP:                /* Flip */
    case BLOCK_ZOOM:                /* Window Zoom */
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:            /* mirror */
    case BLOCK_PRESELECT_MOVE:      /* Move with preselection list*/
        Block->InitData( DrawPanel, startpos );
        break;

    case BLOCK_PASTE:
        Block->InitData( DrawPanel, startpos );
        Block->m_BlockLastCursorPosition.x = 0;
        Block->m_BlockLastCursorPosition.y = 0;
        InitBlockPasteInfos();

        if( Block->m_ItemsSelection.GetCount() == 0 )      /* No data to paste */
        {
            DisplayError( this, wxT( "No Block to paste" ), 20 );
            GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
            DrawPanel->m_mouseCaptureCallback = NULL;
            return true;
        }

        if( !DrawPanel->IsMouseCaptured() )
        {
            Block->m_ItemsSelection.ClearItemsList();
            DisplayError( this,
                          wxT( "EDA_DRAW_FRAME::HandleBlockBegin() Err: m_mouseCaptureCallback NULL" ) );
            return true;
        }

        Block->m_State = STATE_BLOCK_MOVE;
        DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, startpos, false );
        break;

    default:
    {
        wxString msg;
        msg << wxT( "EDA_DRAW_FRAME::HandleBlockBegin() error: Unknown command " ) <<
        Block->m_Command;
        DisplayError( this, msg );
    }
    break;
    }

    Block->SetMessageBlock( this );
    return true;
}


/* Redraw the outlines of the block which shows the search area for block
 * commands
 *  The first point of the rectangle showing the area is initialised
 *  by Initm_BlockLocateDatas().
 *  The other point of the rectangle is the mouse cursor
 */
void DrawAndSizingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                 bool aErase )
{
    BLOCK_SELECTOR* PtBlock;

    PtBlock = &aPanel->GetScreen()->m_BlockLocate;

    PtBlock->m_MoveVector = wxPoint( 0, 0 );

    if( aErase )
        PtBlock->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, PtBlock->m_Color );

    PtBlock->m_BlockLastCursorPosition = aPanel->GetScreen()->GetCrossHairPosition();
    PtBlock->SetEnd( aPanel->GetScreen()->GetCrossHairPosition() );

    PtBlock->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, PtBlock->m_Color );

    if( PtBlock->m_State == STATE_BLOCK_INIT )
    {
        if( PtBlock->GetWidth() || PtBlock->GetHeight() )
            /* 2nd point exists: the rectangle is not surface anywhere */
            PtBlock->m_State = STATE_BLOCK_END;
    }
}


/*
 *  Cancel Current block operation.
 */
void AbortBlockCurrentCommand( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    BASE_SCREEN* screen = Panel->GetScreen();

    if( Panel->IsMouseCaptured() )      /* Erase current drawing on screen */
    {
        /* Clear block outline. */
        Panel->m_mouseCaptureCallback( Panel, DC, wxDefaultPosition, false );
        Panel->SetMouseCapture( NULL, NULL );
        screen->SetCurItem( NULL );

        /* Delete the picked wrapper if this is a picked list. */
        if( screen->m_BlockLocate.m_Command != BLOCK_PASTE )
            screen->m_BlockLocate.ClearItemsList();
    }

    screen->m_BlockLocate.ClearFlags();
    screen->m_BlockLocate.m_State = STATE_NO_BLOCK;
    screen->m_BlockLocate.m_Command = BLOCK_ABORT;
    Panel->GetParent()->HandleBlockEnd( DC );

    screen->m_BlockLocate.m_Command = BLOCK_IDLE;
    Panel->GetParent()->DisplayToolMsg( wxEmptyString );
    Panel->SetCursor( Panel->GetCurrentCursor() );
}
