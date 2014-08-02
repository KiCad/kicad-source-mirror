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

/**
 * @file gerbview/block.cpp
 * @brief Block operations: displacement.
 */


#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gr_basic.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_gerber_draw_item.h>

#include <wx/debug.h>

#define BLOCK_COLOR BROWN


static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool erase );


int GERBVIEW_FRAME::BlockCommand( int key )
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


void GERBVIEW_FRAME::HandleBlockPlace( wxDC* DC )
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


bool GERBVIEW_FRAME::HandleBlockEnd( wxDC* DC )
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
            wxFAIL_MSG( wxT("HandleBlockEnd: Unexpected block command") );
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


/* Traces the outline of the block structures of a repositioning move
 */
static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPositon,
                                     bool aErase )
{
    BASE_SCREEN* screen = aPanel->GetScreen();

    EDA_COLOR_T Color = YELLOW;

    if( aErase )
    {
        screen->m_BlockLocate.Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, Color );

        if( screen->m_BlockLocate.GetMoveVector().x|| screen->m_BlockLocate.GetMoveVector().y )
        {
            screen->m_BlockLocate.Draw( aPanel,
                                        aDC,
                                        screen->m_BlockLocate.GetMoveVector(),
                                        g_XorMode,
                                        Color );
        }
    }

    if( screen->m_BlockLocate.GetState() != STATE_BLOCK_STOP )
    {
        const wxPoint& cross_hair = aPanel->GetParent()->GetCrossHairPosition();

        screen->m_BlockLocate.SetMoveVector(
            wxPoint( cross_hair.x - screen->m_BlockLocate.GetRight(),
                     cross_hair.y - screen->m_BlockLocate.GetBottom() ) );
    }

    screen->m_BlockLocate.Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode, Color );

    if( screen->m_BlockLocate.GetMoveVector().x || screen->m_BlockLocate.GetMoveVector().y )
    {
        screen->m_BlockLocate.Draw( aPanel,
                                    aDC,
                                    screen->m_BlockLocate.GetMoveVector(),
                                    g_XorMode,
                                    Color );
    }
}


void GERBVIEW_FRAME::Block_Move( wxDC* DC )
{
    wxPoint delta;
    wxPoint oldpos;

    oldpos = GetCrossHairPosition();
    m_canvas->SetMouseCaptureCallback( NULL );

    SetCrossHairPosition( oldpos );
    m_canvas->MoveCursorToCrossHair();
    GetScreen()->SetModify();
    GetScreen()->m_BlockLocate.Normalize();

    /* Calculate displacement vectors. */
    delta = GetScreen()->m_BlockLocate.GetMoveVector();

    /* Move items in block */
    for( GERBER_DRAW_ITEM* item = GetItemsList(); item; item = item->Next() )
    {
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;

        if( gerb_item->HitTest( GetScreen()->m_BlockLocate ) )
            gerb_item->MoveAB( delta );
    }

    m_canvas->Refresh( true );
}
