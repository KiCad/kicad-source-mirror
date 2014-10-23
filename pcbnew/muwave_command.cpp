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

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>

#include <pcbnew_id.h>
#include <kicad_device_context.h>


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
        StartMoveModule( module, DC, false );
    }

    m_canvas->MoveCursorToCrossHair();
}
