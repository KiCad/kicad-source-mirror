/**
 * @file pcbnew/initpcb.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <pcb_painter.h>
#include <confirm.h>
#include <wxPcbStruct.h>

#include <class_board.h>

#include <pcbnew.h>
#include <module_editor_frame.h>


bool PCB_EDIT_FRAME::Clear_Pcb( bool aQuery )
{
    if( GetBoard() == NULL )
        return false;

    if( aQuery )
    {
        if( GetBoard()->m_Drawings || GetBoard()->m_Modules
            || GetBoard()->m_Track || GetBoard()->m_Zone )
        {
            if( !IsOK( this,
                       _( "Current Board will be lost and this operation cannot be undone. Continue ?" ) ) )
                return false;
        }
    }

    // Clear undo and redo lists because we want a full deletion
    GetScreen()->ClearUndoRedoList();

    // Items visibility flags will be set becuse a new board will be created.
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

    // preserve grid size accross call to InitDataPoints()

//  wxRealPoint gridsize = GetScreen()->GetGridSize();
    GetScreen()->InitDataPoints( GetPageSizeIU() );
//  GetScreen()->SetGrid( gridsize );

    GetBoard()->ResetHighLight();

    // Enable all layers (SetCopperLayerCount() will adjust the copper layers enabled)
    GetBoard()->SetEnabledLayers( ALL_LAYERS );

    // Default copper layers count set to 2: double layer board
    GetBoard()->SetCopperLayerCount( 2 );

    // Update display
    GetBoard()->SetVisibleLayers( ALL_LAYERS );

    // Set currently selected layer to be shown in high contrast mode, when enabled`
    SetHighContrastLayer( GetScreen()->m_Active_Layer );

    ReFillLayerWidget();

    Zoom_Automatique( false );

    return true;
}


bool FOOTPRINT_EDIT_FRAME::Clear_Pcb( bool aQuery )
{
    if( GetBoard() == NULL )
        return false;

    if( aQuery && GetScreen()->IsModify() )
    {
        if( GetBoard()->m_Modules )
        {
            if( !IsOK( this,
                       _( "Current Footprint will be lost and this operation cannot be undone. Continue ?" ) ) )
                return false;
        }
    }

    // Clear undo and redo lists
    GetScreen()->ClearUndoRedoList();

    // Delete the current footprint
    GetBoard()->m_Modules.DeleteAll();

    // init pointeurs  et variables
    GetBoard()->SetFileName( wxEmptyString );

    SetCurItem( NULL );

    // preserve grid size accross call to InitDataPoints()
//    wxRealPoint gridsize = GetScreen()->GetGridSize();
    GetScreen()->InitDataPoints( GetPageSizeIU() );
//    GetScreen()->SetGrid( gridsize );

    Zoom_Automatique( false );

    return true;
}
