/*****************************************************/
/* muwave_command.cpp: micro wave functions commands */
/*****************************************************/

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>

#include <pcbnew_id.h>
#include <kicad_device_context.h>

#include <protos.h>


/* Handle microwave commands.
 */
void PCB_EDIT_FRAME::ProcessMuWaveFunctions( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxPoint    pos;
    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    wxGetMousePosition( &pos.x, &pos.y );

    pos.y += 20;

    switch( id )    // End any command in progress.
    {
    case ID_POPUP_COPY_BLOCK:
        break;

    default:        // End block command in progress.
        m_canvas->EndMouseCapture( );
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
                      wxT( "PCB_EDIT_FRAME::ProcessMuWaveFunctions() id error" ) );
        break;
    }
}


void PCB_EDIT_FRAME::MuWaveCommand( wxDC* DC, const wxPoint& MousePos )
{
    MODULE* module = NULL;

    switch( GetToolId() )
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
        m_canvas->SetCursor( wxCURSOR_ARROW );
        DisplayError( this, wxT( "PCB_EDIT_FRAME::MuWaveCommand() id error" ) );
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        break;
    }

    if( module )
    {
        StartMove_Module( module, DC );
    }

    m_canvas->MoveCursorToCrossHair();
}
