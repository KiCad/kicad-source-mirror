/*********************************************/
/******* file initpcb.cpp ********************/
/*********************************************/


#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"
#include "class_board_design_settings.h"

//#include "protos.h"


/**
 * Function Clear_Pcb
 * delete all and reinitialize the current board
 * @param aQuery = true to prompt user for confirmation, false to initialize silently
 */
bool PCB_EDIT_FRAME::Clear_Pcb( bool aQuery )
{
    if( GetBoard() == NULL )
        return FALSE;

    if( aQuery )
    {
        if( GetBoard()->m_Drawings || GetBoard()->m_Modules
            || GetBoard()->m_Track || GetBoard()->m_Zone )
        {
            if( !IsOK( this,
                _( "Current Board will be lost and this operation cannot be undone. Continue ?" ) ) )
                return FALSE;
        }
    }

    // Clear undo and redo lists because we want a full deletion
    GetScreen()->ClearUndoRedoList();

    // delete the old BOARD and create a new BOARD so that the default
    // layer names are put into the BOARD.
    SetBoard( new BOARD( NULL, this ) );
    m_TrackAndViasSizesList_Changed = true;
    SetCurItem( NULL );

    /* clear filename, to avoid overwriting an old file */
    GetScreen()->GetFileName().Empty();

    /* Init new grid size */
    wxRealPoint gridsize = GetScreen()->GetGridSize();
    GetScreen()->Init();
    GetScreen()->SetGrid( gridsize );

    g_HighLight_Status = 0;

    // Enable all layers (SetCopperLayerCount() will adjust the copper layers enabled)
    GetBoard()->SetEnabledLayers(ALL_LAYERS);

    // Default copper layers count set to 2: double layer board
    GetBoard()->SetCopperLayerCount(2);

    // Update display:
    GetBoard()->SetVisibleLayers( ALL_LAYERS );

    ReFillLayerWidget();

    Zoom_Automatique( false );

    return true;
}



/**
 * Function Clear_Pcb
 * delete all and reinitialize the current board
 * @param aQuery = true to prompt user for confirmation, false to initialize silently
 */
bool WinEDA_ModuleEditFrame::Clear_Pcb( bool aQuery )
{
    if( GetBoard() == NULL )
        return FALSE;

    if( aQuery && GetScreen()->IsModify() )
    {
        if( GetBoard()->m_Modules )
        {
            if( !IsOK( this,
                _( "Current Footprint will be lost and this operation cannot be undone. Continue ?" ) ) )
                return FALSE;
        }
    }

    // Clear undo and redo lists
    GetScreen()->ClearUndoRedoList();

    // Delete the current footprint
    GetBoard()->m_Modules.DeleteAll();

    /* init pointeurs  et variables */
    GetScreen()->GetFileName().Empty();

    SetCurItem( NULL );

    /* Init parametres de gestion */
    wxRealPoint gridsize = GetScreen()->GetGridSize();
    GetScreen()->Init();
    GetScreen()->SetGrid( gridsize );

    Zoom_Automatique( false );

    return true;
}
