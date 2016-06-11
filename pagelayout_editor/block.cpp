/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pagelayout_editor/block.cpp
 * @brief Block operations
 */


#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>
#include <pl_editor_frame.h>
#include <worksheet_shape_builder.h>
#include <class_worksheet_dataitem.h>


static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool erase );

static void DrawMovingItems( EDA_DRAW_PANEL* aPanel, wxDC* aDC );

static void ConfigureDrawList( WS_DRAW_ITEM_LIST* aDrawList,
        PL_EDITOR_SCREEN* aScreen, PL_EDITOR_FRAME* aFrame );


static void ConfigureDrawList( WS_DRAW_ITEM_LIST* aDrawList,
        PL_EDITOR_SCREEN* aScreen, PL_EDITOR_FRAME* aFrame )
{
    aDrawList->SetPenSize( 0 );
    aDrawList->SetMilsToIUfactor( IU_PER_MILS );
    aDrawList->SetSheetNumber( aScreen->m_ScreenNumber );
    aDrawList->SetSheetCount( aScreen->m_NumberOfScreens );
    aDrawList->SetFileName( aFrame->GetCurrFileName() );
    aDrawList->SetSheetName( aFrame->GetScreenDesc() );
    aDrawList->BuildWorkSheetGraphicList( aFrame->GetPageSettings(),
            aFrame->GetTitleBlock(), RED, RED );
}


int PL_EDITOR_FRAME::BlockCommand( EDA_KEY key )
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


void PL_EDITOR_FRAME::HandleBlockPlace( wxDC* DC )
{
    wxASSERT( m_canvas->IsMouseCaptured() );

    GetScreen()->m_BlockLocate.SetState( STATE_BLOCK_STOP );

    switch( GetScreen()->m_BlockLocate.GetCommand() )
    {
    case BLOCK_MOVE:                /* Move */
        if( m_canvas->IsMouseCaptured() )
            m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

        Block_Move( DC );
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    default:
        wxFAIL_MSG( wxT("HandleBlockPlace: Unexpected block command") );
        break;
    }

    m_canvas->EndMouseCapture( GetToolId(), m_canvas->GetCurrentCursor(), wxEmptyString, false );
    GetScreen()->SetModify();
    GetScreen()->ClearBlockCommand();

    wxASSERT( GetScreen()->m_BlockLocate.GetCount() == 0 );

    DisplayToolMsg( wxEmptyString );
}


bool PL_EDITOR_FRAME::HandleBlockEnd( wxDC* DC )
{
    bool nextcmd  = false;
    bool zoom_command = false;

    if( m_canvas->IsMouseCaptured() )

        switch( GetScreen()->m_BlockLocate.GetCommand() )
        {
        case BLOCK_MOVE:            /* Move */
            GetScreen()->m_BlockLocate.SetState( STATE_BLOCK_MOVE );
            nextcmd = true;
            m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
            m_canvas->SetMouseCaptureCallback( DrawMovingBlockOutlines );
            m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
            break;

        case BLOCK_ZOOM: /* Window Zoom */
            zoom_command = true;
            break;

        default:
            break;
        }

    if( ! nextcmd )
    {
        GetScreen()->ClearBlockCommand();
        m_canvas->EndMouseCapture( GetToolId(), m_canvas->GetCurrentCursor(), wxEmptyString,
                                   false );
    }

    if( zoom_command )
        Window_Zoom( GetScreen()->m_BlockLocate );

    return nextcmd ;
}


static void DrawMovingItems( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    auto screen = static_cast<PL_EDITOR_SCREEN*>( aPanel->GetScreen() );
    auto frame = static_cast<PL_EDITOR_FRAME*>( aPanel->GetParent() );

    // Get items
    std::vector<WS_DRAW_ITEM_BASE*> items;
    WS_DRAW_ITEM_LIST drawList;
    ConfigureDrawList( &drawList, screen, frame );
    drawList.GetAllItems( &items );

    // Draw items
    for( auto item: items )
    {
        if( item->HitTest( screen->m_BlockLocate ) )
        {
            item->DrawWsItem( NULL, aDC, screen->m_BlockLocate.GetMoveVector(),
                    g_XorMode, g_GhostColor );
        }
    }
}


/* Traces the outline of the block structures of a repositioning move
 */
static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPositon,
                                     bool aErase )
{
    auto screen = aPanel->GetScreen();
    auto block = &screen->m_BlockLocate;

    if( aErase )
    {
        block->Draw( aPanel, aDC, block->GetMoveVector(), g_XorMode, block->GetColor() );
        DrawMovingItems( aPanel, aDC );
    }

    block->SetMoveVector( aPanel->GetParent()->GetCrossHairPosition() - block->GetLastCursorPosition() );
    block->Draw( aPanel, aDC, block->GetMoveVector(), g_XorMode, block->GetColor() );
    DrawMovingItems( aPanel, aDC );
}


void PL_EDITOR_FRAME::Block_Move( wxDC* DC )
{
    auto screen = static_cast<PL_EDITOR_SCREEN*>( GetScreen() );

    wxPoint delta;
    wxPoint oldpos;

    oldpos = GetCrossHairPosition();
    m_canvas->SetMouseCaptureCallback( NULL );

    SetCrossHairPosition( oldpos );
    m_canvas->MoveCursorToCrossHair();
    GetScreen()->SetModify();
    GetScreen()->m_BlockLocate.Normalize();

    // Calculate displacement vectors.
    delta = GetScreen()->m_BlockLocate.GetMoveVector();

    // Get the items
    std::vector<WS_DRAW_ITEM_BASE*> items;
    WS_DRAW_ITEM_LIST drawList;
    ConfigureDrawList( &drawList, screen, this );
    drawList.GetAllItems( &items );

    // Move items in block
    SaveCopyInUndoList();
    for( auto item: items )
    {
        if( item->HitTest( screen->m_BlockLocate ) )
        {
            auto data_item = item->GetParent();
            data_item->MoveToUi( data_item->GetStartPosUi() + delta );
        }
    }

    m_canvas->Refresh( true );
}
