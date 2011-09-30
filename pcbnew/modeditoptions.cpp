/**
 * @file modeditoptions.cpp
 * @brief Pcbnew footprint (module) editor options.
 */

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"

#include "pcbnew_id.h"

#include "protos.h"


void FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int        id = event.GetId();

    switch( id )
    {
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
                      wxT( "FOOTPRINT_EDIT_FRAME::OnSelectOptionToolbar error" ) );
        break;
    }
}
