/*****************************************************/
/* muwave_command.cpp: micro wave functions commands */
/*****************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "pcbnew_id.h"
#include "kicad_device_context.h"

#include "protos.h"


/* Handle microwave commands.
 */
void WinEDA_PcbFrame::ProcessMuWaveFunctions( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxPoint    pos;
    INSTALL_UNBUFFERED_DC( dc, DrawPanel );

    wxGetMousePosition( &pos.x, &pos.y );

    pos.y += 20;

    switch( id )    // End any command in progress.
    {
    case ID_POPUP_COPY_BLOCK:
        break;

    default:        // End block command in progress.
        DrawPanel->EndMouseCapture( );
        break;
    }

    switch( id )
    {
    case ID_PCB_MUWAVE_TOOL_SELF_CMD:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Line" ) );
        break;

    case ID_PCB_MUWAVE_TOOL_GAP_CMD:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Gap" ) );
        break;

    case ID_PCB_MUWAVE_TOOL_STUB_CMD:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Stub" ) );
        break;

    case ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Arc Stub" ) );
        break;

    case ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add Polynomial Shape" ) );
        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_PcbFrame::ProcessMuWaveFunctions() id error" ) );
        break;
    }
}


void WinEDA_PcbFrame::MuWaveCommand( wxDC* DC, const wxPoint& MousePos )
{
    MODULE* module = NULL;

    switch( m_ID_current_state )
    {
    case ID_PCB_MUWAVE_TOOL_SELF_CMD:
        Begin_Self( DC );
        break;

    case ID_PCB_MUWAVE_TOOL_GAP_CMD:
        module = Create_MuWaveComponent( 0 );
        break;

    case ID_PCB_MUWAVE_TOOL_STUB_CMD:
        module = Create_MuWaveComponent( 1 );
        break;

    case ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD:
        module = Create_MuWaveComponent( 2 );
        break;

    case ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD:
        module = Create_MuWavePolygonShape();
        break;

    default:
        DrawPanel->SetCursor( wxCURSOR_ARROW );
        DisplayError( this, wxT( "WinEDA_PcbFrame::MuWaveCommand() id error" ) );
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;
    }

    if( module )
    {
        StartMove_Module( module, DC );
    }

    DrawPanel->MoveCursorToCrossHair();
}
