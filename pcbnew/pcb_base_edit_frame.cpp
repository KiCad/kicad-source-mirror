/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <pcb_base_edit_frame.h>
#include <tool/tool_manager.h>
#include <pcb_draw_panel_gal.h>
#include <gal/graphics_abstraction_layer.h>
#include <class_board.h>
#include <view/view.h>


void PCB_BASE_EDIT_FRAME::SetRotationAngle( int aRotationAngle )
{
    wxCHECK2_MSG( aRotationAngle > 0 && aRotationAngle <= 900, aRotationAngle = 900,
                  wxT( "Invalid rotation angle, defaulting to 90." ) );

    m_rotationAngle = aRotationAngle;
}


void PCB_BASE_EDIT_FRAME::UseGalCanvas( bool aEnable )
{
    PCB_BASE_FRAME::UseGalCanvas( aEnable );

    // No matter what, reenable undo/redo on switching to the legacy canvas
    if( !aEnable )
        UndoRedoBlock( false );
    else
        static_cast<PCB_DRAW_PANEL_GAL*>( GetGalCanvas() )->SyncLayersVisibility( m_Pcb );
}


void PCB_BASE_EDIT_FRAME::SetBoard( BOARD* aBoard )
{
    bool new_board = ( aBoard != m_Pcb );

    // The active tool might store a reference to the BOARD that is about to be deleted.
    if( m_toolManager )
        m_toolManager->DeactivateTool();

    // It has to be done before the previous board is destroyed by SetBoard()
    if( new_board )
        GetGalCanvas()->GetView()->Clear();

    PCB_BASE_FRAME::SetBoard( aBoard );

    GetGalCanvas()->GetGAL()->SetGridOrigin( VECTOR2D( aBoard->GetGridOrigin() ) );

    // update the tool manager with the new board and its view.
    if( m_toolManager )
    {
        PCB_DRAW_PANEL_GAL* drawPanel = static_cast<PCB_DRAW_PANEL_GAL*>( GetGalCanvas() );

        drawPanel->DisplayBoard( aBoard );
        m_toolManager->SetEnvironment( aBoard, drawPanel->GetView(),
                                       drawPanel->GetViewControls(), this );

        if( new_board )
            m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
    }
}
