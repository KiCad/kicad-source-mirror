/***********************************************/
/* PCBNEW - Footprint (module) editor options. */
/***********************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"

#include "pcbnew_id.h"

#include "protos.h"


void WinEDA_ModuleEditFrame::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int        id = event.GetId();

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_GRID:
        SetGridVisibility( m_OptionsToolBar->GetToolState( id ) );
        DrawPanel->Refresh( );
        break;

    case ID_TB_OPTIONS_SELECT_UNIT_MM:
        g_UserUnit = MILLIMETRES;

    case ID_TB_OPTIONS_SELECT_UNIT_INCH:
        if( id == ID_TB_OPTIONS_SELECT_UNIT_INCH )
            g_UserUnit = INCHES;
        UpdateStatusBar();
        ReCreateAuxiliaryToolbar();
        break;

    case ID_TB_OPTIONS_SHOW_POLAR_COORD:
        SetStatusText( wxEmptyString );
        DisplayOpt.DisplayPolarCood = m_OptionsToolBar->GetToolState( id );
        UpdateStatusBar();
        break;

    case ID_TB_OPTIONS_SELECT_CURSOR:
        m_CursorShape = m_OptionsToolBar->GetToolState( id );
        break;

    case ID_TB_OPTIONS_SHOW_PADS_SKETCH:
        m_DisplayPadFill = !m_OptionsToolBar->GetToolState( id );
        DrawPanel->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_VIAS_SKETCH:
        m_DisplayViaFill = !m_OptionsToolBar->GetToolState( id );
        DrawPanel->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH:
        m_DisplayModText =
            m_OptionsToolBar->GetToolState( id ) ? SKETCH : FILLED;
        DrawPanel->Refresh( );
        break;

    case ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH:
        m_DisplayModEdge =
            m_OptionsToolBar->GetToolState( id ) ? SKETCH : FILLED;
        DrawPanel->Refresh( );
        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_ModuleEditFrame::OnSelectOptionToolbar error" ) );
        break;
    }

    SetToolbars();
}
