/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

//#define USE_TOOL_MANAGER

#include <wx/timer.h>
#include <wx/math.h>
#include <wx/log.h>
#include <wx/popupwin.h>
#include <wx/cmdline.h>


#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <layer_ids.h>

#include <gal/graphics_abstraction_layer.h>
#include <view/view.h>
#include <class_draw_panel_gal.h>
#include <pcb_draw_panel_gal.h>
#include <view/wx_view_controls.h>
#include <pcb_painter.h>

#include <pad.h>
#include <footprint.h>
#include <board.h>

#include <pcb_track.h>
#include <pcb_painter.h>
#include <pcb_edit_frame.h>

#include <connectivity/connectivity_data.h>

#include <io_mgr.h>
#include <memory>
#include <set>

#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/pcb_tool_base.h>
#include <tools/pcb_actions.h>
#include <pcbnew/tools/pcb_selection_tool.h>
#include <plugins/kicad/kicad_plugin.h>

#include "pcb_test_frame.h"


using namespace KIGFX;


void PCB_TEST_FRAME_BASE::SetBoard( std::shared_ptr<BOARD> b )
{
    m_board = b;

    m_board->GetConnectivity()->Build( m_board.get() );
    m_galPanel->DisplayBoard( m_board.get() );
    m_galPanel->UpdateColors();

#ifdef USE_TOOL_MANAGER
    m_toolManager->SetEnvironment( m_board.get(), m_galPanel->GetView(),
                                   m_galPanel->GetViewControls(), nullptr );

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
#endif
}


BOARD* PCB_TEST_FRAME_BASE::LoadAndDisplayBoard( const std::string& filename )
{
    PLUGIN::RELEASER pi( new PCB_IO );
    BOARD* brd = nullptr;

    try
    {
        brd = pi->Load( wxString( filename.c_str() ), nullptr, nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        wxPrintf( "Board Loading Error: '%s'\n", ioe.Problem() );
        return nullptr;
    }

    //SetBoard( brd );

    return brd;
}


class TEST_ACTIONS : public ACTIONS
{
};


void PCB_TEST_FRAME_BASE::createView( wxWindow *aParent, PCB_DRAW_PANEL_GAL::GAL_TYPE aGalType )
{
    // SUPERSAMPLING_X4;
    m_displayOptions.gl_antialiasing_mode = KIGFX::OPENGL_ANTIALIASING_MODE::NONE;

    m_galPanel = std::make_shared<PCB_DRAW_PANEL_GAL>( aParent, -1, wxPoint( 0, 0 ),
                                                       wxDefaultSize, m_displayOptions, aGalType );
    m_galPanel->UpdateColors();

    m_galPanel->SetEvtHandlerEnabled( true );
    m_galPanel->SetFocus();
    m_galPanel->Show( true );
    m_galPanel->Raise();
    m_galPanel->StartDrawing();

    auto gal = m_galPanel->GetGAL();

    gal->SetGridVisibility( true );
    gal->SetGridSize( VECTOR2D( 100000.0, 100000.0 ) );
    gal->SetGridOrigin( VECTOR2D( 0.0, 0.0 ) );

    //m_galPanel->Connect( wxEVT_MOTION,
    //wxMouseEventHandler( PCB_TEST_FRAME::OnMotion ), nullptr, this );

    m_galPanel->GetViewControls()->ShowCursor( true );

#ifdef USE_TOOL_MANAGER
    m_toolManager = std::make_unique<TOOL_MANAGER>( );
    m_toolManager->SetEnvironment( m_board.get(), m_galPanel->GetView(),
                                   m_galPanel->GetViewControls(), nullptr );

    m_pcbActions = std::make_unique<TEST_ACTIONS>( );
    m_toolDispatcher = std::make_unique<TOOL_DISPATCHER>( m_toolManager.get() );

    //m_toolManager->RegisterTool( new PCB_SELECTION_TOOL );
    createUserTools();

    m_toolManager->InitTools();
    m_toolManager->InvokeTool( "test.DefaultTool" );
#endif

    //SetBoard( std::make_shared<BOARD>( new BOARD ));
}


PCB_TEST_FRAME_BASE::PCB_TEST_FRAME_BASE()
{
}


PCB_TEST_FRAME_BASE::~PCB_TEST_FRAME_BASE()
{

}


void PCB_TEST_FRAME_BASE::LoadSettings()
{
    auto cs = Pgm().GetSettingsManager().GetColorSettings();
    //cs->SetColorContext( COLOR_CONTEXT::PCB );
    cs->Load();
}
