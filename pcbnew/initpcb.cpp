/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew/initpcb.cpp
 */

#include <confirm.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>

#include <board.h>
#include <board_design_settings.h>

#include <pcbnew.h>
#include <footprint_edit_frame.h>
#include <widgets/appearance_controls.h>


bool PCB_EDIT_FRAME::Clear_Pcb( bool aQuery, bool aFinal )
{
    if( GetBoard() == nullptr )
        return false;

    if( aQuery && !GetBoard()->IsEmpty() )
    {
        if( !IsOK( this, _( "Current Board will be lost and this operation cannot be undone. "
                            "Continue?" ) ) )
        {
            return false;
        }
    }

    // Release the lock file, if exists
    ReleaseFile();

    // Clear undo and redo lists because we want a full deletion
    ClearUndoRedoList();
    GetScreen()->SetContentModified( false );

    if( !aFinal )
    {
        // delete the old BOARD and create a new BOARD so that the default
        // layer names are put into the BOARD.
        SetBoard( new BOARD() );

        // clear filename, to avoid overwriting an old file
        GetBoard()->SetFileName( wxEmptyString );

        GetScreen()->InitDataPoints( GetPageSizeIU() );

        GetBoard()->ResetNetHighLight();

        // Enable all layers (SetCopperLayerCount() will adjust the copper layers enabled)
        GetBoard()->SetEnabledLayers( LSET().set() );

        // Default copper layers count set to 2: double layer board
        GetBoard()->SetCopperLayerCount( 2 );

        // Update display (some options depend on the board setup)
        GetBoard()->SetVisibleLayers( LSET().set() );
        ReCreateLayerBox();
        ReCreateAuxiliaryToolbar();
        m_appearancePanel->OnBoardChanged();
        UpdateTitle();

        Zoom_Automatique( false );
    }

    return true;
}


bool FOOTPRINT_EDIT_FRAME::Clear_Pcb( bool aQuery )
{
    if( GetBoard() == nullptr )
        return false;

    bool is_last_fp_from_brd = IsCurrentFPFromBoard();

    if( aQuery && IsContentModified() )
    {
        wxSafeYield( this, true );      // Allow frame to come to front before showing warning.

        if( !HandleUnsavedChanges(
                    this, _( "The current footprint has been modified.  Save changes?" ),
                    [&]() -> bool
                    {
                        return SaveFootprint( GetBoard()->Footprints().front() );
                    } ) )
        {
            return false;
        }
    }

    if( is_last_fp_from_brd )
        m_boardFootprintUuids.clear();

    // Clear undo and redo lists because we want a full deletion
    ClearUndoRedoList();
    GetScreen()->SetContentModified( false );

    BOARD* board = new BOARD;

    board->GetDesignSettings() = GetDesignSettings();
    board->SynchronizeNetsAndNetClasses();
    SetBoard( board );

    // This board will only be used to hold a footprint for editing
    GetBoard()->SetBoardUse( BOARD_USE::FPHOLDER );

    // clear filename, to avoid overwriting an old file
    GetBoard()->SetFileName( wxEmptyString );

    GetScreen()->InitDataPoints( GetPageSizeIU() );

    return true;
}
