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
#include <wxPcbStruct.h>

#include <class_board.h>

#include <pcbnew.h>
#include <module_editor_frame.h>


bool PCB_EDIT_FRAME::Clear_Pcb( bool aQuery )
{
    if( GetBoard() == NULL )
        return false;

    if( aQuery && GetScreen()->IsModify() && !GetBoard()->IsEmpty() )
    {
        if( !IsOK( this,
                   _( "Current Board will be lost and this operation cannot be undone. Continue ?" ) ) )
            return false;
    }

    // Clear undo and redo lists because we want a full deletion
    GetScreen()->ClearUndoRedoList();
    GetScreen()->ClrModify();

    // Items visibility flags will be set because a new board will be created.
    // Grid and ratsnest can be left to their previous state
    bool showGrid = IsElementVisible( GRID_VISIBLE );
    bool showRats = IsElementVisible( RATSNEST_VISIBLE );

    // delete the old BOARD and create a new BOARD so that the default
    // layer names are put into the BOARD.
    SetBoard( new BOARD() );
    SetElementVisibility( GRID_VISIBLE, showGrid );
    SetElementVisibility( RATSNEST_VISIBLE, showRats );

    SetCurItem( NULL );

    // clear filename, to avoid overwriting an old file
    GetBoard()->SetFileName( wxEmptyString );

    GetScreen()->InitDataPoints( GetPageSizeIU() );

    GetBoard()->ResetHighLight();

    // Enable all layers (SetCopperLayerCount() will adjust the copper layers enabled)
    GetBoard()->SetEnabledLayers( LSET().set() );

    // Default copper layers count set to 2: double layer board
    GetBoard()->SetCopperLayerCount( 2 );

    // Update display (some options depend on the board setup)
    GetBoard()->SetVisibleLayers( LSET().set() );
    ReCreateLayerBox();
    ReCreateAuxiliaryToolbar();
    ReFillLayerWidget();

    Zoom_Automatique( false );

    return true;
}


bool FOOTPRINT_EDIT_FRAME::Clear_Pcb( bool aQuery )
{
    if( GetBoard() == NULL )
        return false;

    if( aQuery && GetScreen()->IsModify() && !GetBoard()->IsEmpty() )
    {
        if( !IsOK( this,
                   _( "Current Footprint will be lost and this operation cannot be undone. Continue ?" ) ) )
            return false;
    }

    // Clear undo and redo lists
    GetScreen()->ClearUndoRedoList();

    // Delete the current footprint
    GetBoard()->m_Modules.DeleteAll();

    // init pointeurs  et variables
    GetBoard()->SetFileName( wxEmptyString );

    SetCurItem( NULL );

    GetScreen()->InitDataPoints( GetPageSizeIU() );

    Zoom_Automatique( false );

    return true;
}
