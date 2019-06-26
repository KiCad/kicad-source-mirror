/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <confirm.h>
#include <pcb_edit_frame.h>

#include <class_board.h>

#include <pcbnew.h>
#include <footprint_edit_frame.h>


bool PCB_EDIT_FRAME::Clear_Pcb( bool aQuery, bool aFinal )
{
    if( GetBoard() == NULL )
        return false;

    if( aQuery && !GetBoard()->IsEmpty() )
    {
        if( !IsOK( this,
                   _( "Current Board will be lost and this operation cannot be undone. Continue ?" ) ) )
            return false;
    }

    // Release the lock file, if exists
    ReleaseFile();

    // Clear undo and redo lists because we want a full deletion
    GetScreen()->ClearUndoRedoList();
    GetScreen()->ClrModify();

    // Items visibility flags will be set because a new board will be created.
    // Grid and ratsnest can be left to their previous state
    bool showGrid = IsElementVisible( LAYER_GRID );
    bool showRats = m_DisplayOptions.m_ShowGlobalRatsnest;

    if( !aFinal )
    {
        // delete the old BOARD and create a new BOARD so that the default
        // layer names are put into the BOARD.
        SetBoard( new BOARD() );
        SetElementVisibility( LAYER_GRID, showGrid );
        SetElementVisibility( LAYER_RATSNEST, showRats );

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
        ReFillLayerWidget();
        UpdateTitle();

        Zoom_Automatique( false );
    }

    return true;
}


bool FOOTPRINT_EDIT_FRAME::Clear_Pcb( bool aQuery )
{
    if( GetBoard() == NULL )
        return false;

    if( aQuery && GetScreen()->IsModify() && !GetBoard()->IsEmpty() )
    {
        wxSafeYield( this, true );      // Allow frame to come to front before showing warning.

        if( !HandleUnsavedChanges( this, _( "The current footprint has been modified.  "
                                            "Save changes?" ),
                    [&]() -> bool { return SaveFootprint( GetBoard()->Modules().front() ); } ) )
        {
            return false;
        }
    }

    // Clear undo and redo lists because we want a full deletion
    GetScreen()->ClearUndoRedoList();
    GetScreen()->ClrModify();

    BOARD* board = new BOARD;

    // Transfer current design settings
    if( GetBoard() )
        board->SetDesignSettings( GetBoard()->GetDesignSettings() );

    board->SynchronizeNetsAndNetClasses();
    SetBoard( board );

    // clear filename, to avoid overwriting an old file
    GetBoard()->SetFileName( wxEmptyString );

    GetScreen()->InitDataPoints( GetPageSizeIU() );

    Zoom_Automatique( false );

    return true;
}
