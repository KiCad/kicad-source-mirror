/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#include <fctsys.h>
#include <gr_basic.h>
#include <draw_frame.h>
#include <common.h>
#include <macros.h>
#include <base_struct.h>
#include <class_base_screen.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <block_commande.h>


BLOCK_SELECTOR::BLOCK_SELECTOR() :
    EDA_RECT()
{
    m_state   = STATE_NO_BLOCK; // State (enum BLOCK_STATE_T) of block.
    m_command = BLOCK_IDLE;     // Type (enum BLOCK_COMMAND_T) of operation.
    m_color   = BROWN;
}


BLOCK_SELECTOR::~BLOCK_SELECTOR()
{
}


void BLOCK_SELECTOR::SetMessageBlock( EDA_DRAW_FRAME* frame )
{
    wxString msg;

    switch( m_command )
    {
    case BLOCK_IDLE:
        break;

    case BLOCK_MOVE:                // Move
    case BLOCK_PRESELECT_MOVE:      // Move with preselection list
    case BLOCK_MOVE_EXACT:
        msg = _( "Block Move" );
        break;

    case BLOCK_DRAG:     // Drag
        msg = _( "Block Drag" );
        break;

    case BLOCK_DRAG_ITEM:     // Drag
        msg = _( "Drag item" );
        break;

    case BLOCK_COPY:     // Copy
        msg = _( "Block Copy" );
        break;

    case BLOCK_DELETE:     // Delete
        msg = _( "Block Delete" );
        break;

    case BLOCK_SAVE:     // Save
        msg = _( "Block Save" );
        break;

    case BLOCK_PASTE:
        msg = _( "Block Paste" );
        break;

    case BLOCK_ZOOM:     // Window Zoom
        msg = _( "Win Zoom" );
        break;

    case BLOCK_ROTATE:     // Rotate 90 deg
        msg = _( "Block Rotate" );
        break;

    case BLOCK_FLIP:     // Flip
        msg = _( "Block Flip" );
        break;

    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:     // mirror
        msg = _( "Block Mirror" );
        break;

    case BLOCK_ABORT:
        break;

    default:
        msg = wxT( "???" );
        break;
    }

    frame->DisplayToolMsg( msg );
}


void BLOCK_SELECTOR::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                           GR_DRAWMODE aDrawMode, EDA_COLOR_T aColor )
{
    if( !aDC )
        return;

    int w = GetWidth();
    int h = GetHeight();

    GRSetDrawMode( aDC, aDrawMode );

    if(  w == 0 || h == 0 )
        GRLine( aPanel->GetClipBox(), aDC, GetX() + aOffset.x, GetY() + aOffset.y,
                GetRight() + aOffset.x, GetBottom() + aOffset.y, 0, aColor );
    else
        GRRect( aPanel->GetClipBox(), aDC, GetX() + aOffset.x, GetY() + aOffset.y,
                GetRight() + aOffset.x, GetBottom() + aOffset.y, 0, aColor );
}


void BLOCK_SELECTOR::InitData( EDA_DRAW_PANEL* aPanel, const wxPoint& startpos )
{
    m_state = STATE_BLOCK_INIT;
    SetOrigin( startpos );
    SetSize( wxSize( 0, 0 ) );
    m_items.ClearItemsList();
    aPanel->SetMouseCapture( DrawAndSizingBlockOutlines, AbortBlockCurrentCommand );
}


void BLOCK_SELECTOR::ClearItemsList()
{
    m_items.ClearItemsList();
}


void BLOCK_SELECTOR::ClearListAndDeleteItems()
{
     m_items.ClearListAndDeleteItems();
}


void BLOCK_SELECTOR::PushItem( ITEM_PICKER& aItem )
{
    m_items.PushItem( aItem );
}


void BLOCK_SELECTOR::Clear()
{
    if( m_command != BLOCK_IDLE )
    {
        m_command = BLOCK_IDLE;
        m_state   = STATE_NO_BLOCK;
        ClearItemsList();
    }
}


void DrawAndSizingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                 bool aErase )
{
    BLOCK_SELECTOR* block;

    block = &aPanel->GetScreen()->m_BlockLocate;

    block->SetMoveVector( wxPoint( 0, 0 ) );

    if( aErase && aDC )
        block->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, block->GetColor() );

    block->SetLastCursorPosition( aPanel->GetParent()->GetCrossHairPosition() );
    block->SetEnd( aPanel->GetParent()->GetCrossHairPosition() );

    if( aDC )
        block->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, block->GetColor() );

    if( block->GetState() == STATE_BLOCK_INIT )
    {
        if( block->GetWidth() || block->GetHeight() )
            // 2nd point exists: the rectangle is not surface anywhere
            block->SetState( STATE_BLOCK_END );
    }
}


void AbortBlockCurrentCommand( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    BASE_SCREEN* screen = aPanel->GetScreen();

    if( aPanel->IsMouseCaptured() )      // Erase current drawing on screen
    {
        // Clear block outline.
        aPanel->CallMouseCapture( aDC, wxDefaultPosition, false );
        aPanel->SetMouseCapture( NULL, NULL );
        screen->SetCurItem( NULL );

        // Delete the picked wrapper if this is a picked list.
        if( screen->m_BlockLocate.GetCommand() != BLOCK_PASTE )
            screen->m_BlockLocate.ClearItemsList();
    }

    screen->m_BlockLocate.SetState( STATE_NO_BLOCK );
    screen->m_BlockLocate.SetCommand( BLOCK_ABORT );
    aPanel->GetParent()->HandleBlockEnd( aDC );

    screen->m_BlockLocate.SetCommand( BLOCK_IDLE );
    aPanel->GetParent()->DisplayToolMsg( wxEmptyString );
    aPanel->SetCursor( (wxStockCursor) aPanel->GetCurrentCursor() );
}
