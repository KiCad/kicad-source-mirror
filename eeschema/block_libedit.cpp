/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file block_libedit.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>

#include <general.h>
#include <class_library.h>
#include <libeditframe.h>


static void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                     bool aErase );


int LIB_EDIT_FRAME::BlockCommand( int key )
{
    int cmd = BLOCK_IDLE;

    switch( key )
    {
    default:
        cmd = key & 0xFF;
        break;

    case -1:
        cmd = BLOCK_PRESELECT_MOVE;
        break;

    case 0:
        cmd = BLOCK_MOVE;
        break;

    case GR_KB_SHIFT:
        cmd = BLOCK_COPY;
        break;

    case GR_KB_ALT:
        cmd = BLOCK_ROTATE;
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


bool LIB_EDIT_FRAME::HandleBlockEnd( wxDC* DC )
{
    int ItemCount = 0;
    int nextCmd = false;
    wxPoint pt;

    if( GetScreen()->m_BlockLocate.GetCount() )
    {
        BLOCK_STATE_T state     = GetScreen()->m_BlockLocate.GetState();
        BLOCK_COMMAND_T command = GetScreen()->m_BlockLocate.GetCommand();
        m_canvas->CallEndMouseCapture( DC );
        GetScreen()->m_BlockLocate.SetState( state );
        GetScreen()->m_BlockLocate.SetCommand( command );
        m_canvas->SetMouseCapture( DrawAndSizingBlockOutlines, AbortBlockCurrentCommand );
        SetCrossHairPosition( wxPoint( GetScreen()->m_BlockLocate.GetRight(),
                                       GetScreen()->m_BlockLocate.GetBottom() ) );
        m_canvas->MoveCursorToCrossHair();
    }

    switch( GetScreen()->m_BlockLocate.GetCommand() )
    {
    case  BLOCK_IDLE:
        DisplayError( this, wxT( "Error in HandleBlockPLace" ) );
        break;

    case BLOCK_DRAG:        // Drag
    case BLOCK_DRAG_ITEM:
    case BLOCK_MOVE:        // Move
    case BLOCK_COPY:        // Copy
        if( GetCurPart() )
            ItemCount = GetCurPart()->SelectItems( GetScreen()->m_BlockLocate,
                                                  m_unit, m_convert,
                                                  m_editPinsPerPartOrConvert );
        if( ItemCount )
        {
            nextCmd = true;

            if( m_canvas->IsMouseCaptured() )
            {
                m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
                m_canvas->SetMouseCaptureCallback( DrawMovingBlockOutlines );
                m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
            }

            GetScreen()->m_BlockLocate.SetState( STATE_BLOCK_MOVE );
            m_canvas->Refresh( true );
        }
        break;

    case BLOCK_PRESELECT_MOVE:     // Move with preselection list
        nextCmd = true;
        m_canvas->SetMouseCaptureCallback( DrawMovingBlockOutlines );
        GetScreen()->m_BlockLocate.SetState( STATE_BLOCK_MOVE );
        break;

    case BLOCK_DELETE:     // Delete
        if( GetCurPart() )
            ItemCount = GetCurPart()->SelectItems( GetScreen()->m_BlockLocate,
                                                  m_unit, m_convert,
                                                  m_editPinsPerPartOrConvert );
        if( ItemCount )
            SaveCopyInUndoList( GetCurPart() );

        if( GetCurPart() )
        {
            GetCurPart()->DeleteSelectedItems();
            OnModify();
        }
        break;

    case BLOCK_SAVE:     // Save
    case BLOCK_PASTE:
    case BLOCK_FLIP:
        break;

    case BLOCK_ROTATE:
    case BLOCK_MIRROR_X:
    case BLOCK_MIRROR_Y:
        if( GetCurPart() )
            ItemCount = GetCurPart()->SelectItems( GetScreen()->m_BlockLocate,
                                                  m_unit, m_convert,
                                                  m_editPinsPerPartOrConvert );
        if( ItemCount )
            SaveCopyInUndoList( GetCurPart() );

        pt = GetScreen()->m_BlockLocate.Centre();
        pt = GetNearestGridPosition( pt );
        pt.y = -pt.y;

        if( GetCurPart() )
        {
            OnModify();
            int block_cmd = GetScreen()->m_BlockLocate.GetCommand();

            if( block_cmd == BLOCK_MIRROR_Y)
                GetCurPart()->MirrorSelectedItemsH( pt );
            else if( block_cmd == BLOCK_MIRROR_X)
                GetCurPart()->MirrorSelectedItemsV( pt );
            else if( block_cmd == BLOCK_ROTATE )
                GetCurPart()->RotateSelectedItems( pt );
        }

        break;

    case BLOCK_ZOOM:     // Window Zoom
        Window_Zoom( GetScreen()->m_BlockLocate );
        break;

    case BLOCK_ABORT:
        break;

    case BLOCK_SELECT_ITEMS_ONLY:
        break;

    case BLOCK_COPY_AND_INCREMENT:      // not used in Eeschema
    case BLOCK_MOVE_EXACT:              // not used in Eeschema
        break;
    }

    if( !nextCmd )
    {
        if( GetScreen()->m_BlockLocate.GetCommand() != BLOCK_SELECT_ITEMS_ONLY &&  GetCurPart() )
            GetCurPart()->ClearSelectedItems();

        GetScreen()->m_BlockLocate.SetState( STATE_NO_BLOCK );
        GetScreen()->m_BlockLocate.SetCommand( BLOCK_IDLE );
        GetScreen()->SetCurItem( NULL );
        m_canvas->EndMouseCapture( GetToolId(), m_canvas->GetCurrentCursor(), wxEmptyString,
                                   false );
        m_canvas->Refresh( true );
    }

    return nextCmd;
}


void LIB_EDIT_FRAME::HandleBlockPlace( wxDC* DC )
{
    wxPoint pt;

    if( !m_canvas->IsMouseCaptured() )
    {
        DisplayError( this, wxT( "HandleBlockPLace : m_mouseCaptureCallback = NULL" ) );
    }

    GetScreen()->m_BlockLocate.SetState( STATE_BLOCK_STOP );

    switch( GetScreen()->m_BlockLocate.GetCommand() )
    {
    case  BLOCK_IDLE:
        break;

    case BLOCK_DRAG:                // Drag
    case BLOCK_DRAG_ITEM:
    case BLOCK_MOVE:                // Move
    case BLOCK_PRESELECT_MOVE:      // Move with preselection list
        GetScreen()->m_BlockLocate.ClearItemsList();

        if( GetCurPart() )
            SaveCopyInUndoList( GetCurPart() );

        pt = GetScreen()->m_BlockLocate.GetMoveVector();
        pt.y *= -1;

        if( GetCurPart() )
            GetCurPart()->MoveSelectedItems( pt );

        m_canvas->Refresh( true );
        break;

    case BLOCK_COPY:     // Copy
        GetScreen()->m_BlockLocate.ClearItemsList();

        if( GetCurPart() )
            SaveCopyInUndoList( GetCurPart() );

        pt = GetScreen()->m_BlockLocate.GetMoveVector();
        pt.y = -pt.y;

        if( GetCurPart() )
            GetCurPart()->CopySelectedItems( pt );

        break;

    case BLOCK_PASTE:       // Paste (recopy the last block saved)
        GetScreen()->m_BlockLocate.ClearItemsList();
        break;

    case BLOCK_ROTATE:      // Invert by popup menu, from block move
    case BLOCK_MIRROR_X:    // Invert by popup menu, from block move
    case BLOCK_MIRROR_Y:    // Invert by popup menu, from block move
        if( GetCurPart() )
            SaveCopyInUndoList( GetCurPart() );

        pt = GetScreen()->m_BlockLocate.Centre();
        pt = GetNearestGridPosition( pt );
        pt.y = -pt.y;

        if( GetCurPart() )
        {
            int block_cmd = GetScreen()->m_BlockLocate.GetCommand();

            if( block_cmd == BLOCK_MIRROR_Y)
                GetCurPart()->MirrorSelectedItemsH( pt );
            else if( block_cmd == BLOCK_MIRROR_X)
                GetCurPart()->MirrorSelectedItemsV( pt );
            else if( block_cmd == BLOCK_ROTATE )
                GetCurPart()->RotateSelectedItems( pt );
        }

        break;

    case BLOCK_ZOOM:        // Handled by HandleBlockEnd
    case BLOCK_DELETE:
    case BLOCK_SAVE:
    case BLOCK_ABORT:
    default:
        break;
    }

    OnModify();

    GetScreen()->m_BlockLocate.SetState( STATE_NO_BLOCK );
    GetScreen()->m_BlockLocate.SetCommand( BLOCK_IDLE );
    GetScreen()->SetCurItem( NULL );
    m_canvas->EndMouseCapture( GetToolId(), m_canvas->GetCurrentCursor(), wxEmptyString, false );
    m_canvas->Refresh( true );
}


/*
 * Traces the outline of the search block structures
 * The entire block follows the cursor
 */
void DrawMovingBlockOutlines( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                              bool aErase )
{
    BLOCK_SELECTOR* block;
    BASE_SCREEN* screen = aPanel->GetScreen();
    wxPoint move_offset;
    block = &screen->m_BlockLocate;

    LIB_EDIT_FRAME* parent = (LIB_EDIT_FRAME*) aPanel->GetParent();
    wxASSERT( parent != NULL );

    LIB_PART* component = parent->GetCurPart();

    if( component == NULL )
        return;

    int unit = parent->GetUnit();
    int convert = parent->GetConvert();

    if( aErase )
    {
        block->Draw( aPanel, aDC, block->GetMoveVector(), g_XorMode, block->GetColor() );

        component->Draw( aPanel, aDC, block->GetMoveVector(), unit, convert,
                         g_XorMode, UNSPECIFIED_COLOR, DefaultTransform, true, true, true );
    }

    // Repaint new view
    block->SetMoveVector( parent->GetCrossHairPosition() - block->GetLastCursorPosition() );

    GRSetDrawMode( aDC, g_XorMode );
    block->Draw( aPanel, aDC, block->GetMoveVector(), g_XorMode, block->GetColor() );

    component->Draw( aPanel, aDC, block->GetMoveVector(), unit, convert,
                     g_XorMode, UNSPECIFIED_COLOR, DefaultTransform, true, true, true );
}
