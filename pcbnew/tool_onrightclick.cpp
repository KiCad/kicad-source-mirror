/*************************/
/* tool_onrightclick.cpp */
/*************************/

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"
#include "dialog_design_rules.h"

#include "pcbnew_id.h"


void WinEDA_PcbFrame::ToolOnRightClick( wxCommandEvent& event )
{
    wxPoint pos;
    int     id = event.GetSelection();

    wxGetMousePosition( &pos.x, &pos.y );

    switch( id )
    {
    case ID_TRACK_BUTT:
    {
        DIALOG_DESIGN_RULES dlg( this );
        dlg.ShowModal();
        break;
    }

    case ID_COMPONENT_BUTT:
        break;

    case ID_PCB_CIRCLE_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_PCB_ADD_LINE_BUTT:
    case ID_PCB_DIMENSION_BUTT:
    case ID_PCB_ADD_TEXT_BUTT:
        OnConfigurePcbOptions( event );
        break;

    case ID_PCB_PLACE_GRID_COORD_BUTT:
        InstallGridFrame( wxDefaultPosition );
        break;

    default:
        break;
    }
}


void WinEDA_ModuleEditFrame::ToolOnRightClick( wxCommandEvent& event )
{
    wxPoint pos;
    int     id = event.GetSelection();

    wxGetMousePosition( &pos.x, &pos.y );
    pos.x -= 400;
    pos.y -= 30;

    switch( id )
    {
    case ID_MODEDIT_ADD_PAD:
        InstallPadOptionsFrame( NULL );
        break;

    case ID_PCB_CIRCLE_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_PCB_ADD_LINE_BUTT:
    case ID_PCB_DIMENSION_BUTT:
    case ID_PCB_ADD_TEXT_BUTT:
        InstallOptionsFrame( pos );
        break;

    default:
        DisplayError( this, wxT( "ToolOnRightClick() error" ) );
        break;
    }
}
