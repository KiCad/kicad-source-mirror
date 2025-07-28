/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include <wx/popupwin.h>
#include <wx/cmdline.h>
#include <core/profile.h>

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <settings/cvpcb_settings.h>
#include <layer_ids.h>

#include <gal/graphics_abstraction_layer.h>
#include <dpi_scaling_common.h>
#include <class_draw_panel_gal.h>
#include <pcb_draw_panel_gal.h>
#include <view/wx_view_controls.h>
#include <pcb_painter.h>
#include <pcb_actions.h>
#include <functional>

#include <pad.h>
#include <footprint.h>
#include <board.h>
#include <pcb_track.h>
#include <pcb_edit_frame.h>

#include <connectivity/connectivity_data.h>
#include <connectivity/from_to_cache.h>

#include <pcb_io/pcb_io_mgr.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

#include <memory>

#include <trace_helpers.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>

#include "pcb_test_frame.h"
#include "pcb_test_selection_tool.h"

using namespace KIGFX;

void PCB_TEST_FRAME_BASE::SetBoard( std::shared_ptr<BOARD> b )
{
    m_board = b;
    printf("TestFrameBase::SetBoard %p\n", b.get() );

    PROF_TIMER cntConnectivity( "connectivity-build" );
    m_board->BuildListOfNets();
    m_board->BuildConnectivity();
    cntConnectivity.Show();

    PROF_TIMER cntView("view-build");
    m_galPanel->DisplayBoard( m_board.get() );
    cntView.Show();

    PROF_TIMER cntFromTo("fromto-cache-update");
    m_board->GetConnectivity()->GetFromToCache()->Rebuild( m_board.get() );
    cntFromTo.Show();

    m_galPanel->UpdateColors();

    KI_TRACE( traceGalProfile, "%s\n", cntConnectivity.to_string() );
    KI_TRACE( traceGalProfile, "%s\n", cntView.to_string() );

#ifdef USE_TOOL_MANAGER
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    PCBNEW_SETTINGS*  cfg = mgr.RegisterSettings( new PCBNEW_SETTINGS, false );

    m_toolManager = new TOOL_MANAGER;

    m_toolManager->SetEnvironment( m_board.get(), m_galPanel->GetView(),
                                   m_galPanel->GetViewControls(), cfg, this );

    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    m_selectionTool.reset( new PCB_TEST_SELECTION_TOOL );

    m_toolManager->RegisterTool( m_selectionTool.get() );

    createUserTools();


    for( TOOL_BASE* tool : m_toolManager->Tools() )
    {
        if( PCB_TOOL_BASE* pcbTool = dynamic_cast<PCB_TOOL_BASE*>( tool ) )
            pcbTool->SetIsBoardEditor( true );
    }

    m_toolManager->InitTools();

    m_galPanel->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
    m_toolManager->InvokeTool( "common.InteractiveSelection" );
#endif
}


BOARD* PCB_TEST_FRAME_BASE::LoadAndDisplayBoard( const std::string& filename )
{
    IO_RELEASER<PCB_IO> pi( new PCB_IO_KICAD_SEXPR );
    BOARD* brd = nullptr;

    try
    {
        brd = pi->LoadBoard( wxString( filename.c_str() ), nullptr, nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        wxPrintf( "Board Loading Error: '%s'\n", ioe.Problem() );
        return nullptr;
    }

    //SetBoard( brd );

    return brd;
}

void PCB_TEST_FRAME_BASE::SetSelectableItemTypes( const std::vector<KICAD_T> aTypes )
{
    m_selectionTool->SetSelectableItemTypes( aTypes );
}



void PCB_TEST_FRAME_BASE::createView( wxWindow *aParent, PCB_DRAW_PANEL_GAL::GAL_TYPE aGalType )
{
    // SUPERSAMPLING_X4;
    m_displayOptions.antialiasing_mode = KIGFX::GAL_ANTIALIASING_MODE::AA_NONE;

    DPI_SCALING_COMMON dpi( Pgm().GetCommonSettings(), aParent );
    m_displayOptions.m_scaleFactor = dpi.GetScaleFactor();

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



    //SetBoard( std::make_shared<BOARD>( new BOARD ));
}


PCB_TEST_FRAME_BASE::PCB_TEST_FRAME_BASE()
{
    m_mruPath = wxGetCwd();
}


PCB_TEST_FRAME_BASE::~PCB_TEST_FRAME_BASE()
{

}


void PCB_TEST_FRAME_BASE::LoadSettings()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

    mgr.RegisterSettings( new PCBNEW_SETTINGS );
    mgr.RegisterSettings( new CVPCB_SETTINGS );
    mgr.GetColorSettings( DEFAULT_THEME )->Load();
}

void PCB_TEST_FRAME_BASE::SetSelectionHook( std::function<void(PCB_TEST_FRAME_BASE*, PCB_SELECTION*)> aHook )
{
    auto tool = m_toolManager->FindTool( "common.InteractiveSelection" );
    if (!tool)
        return;

    auto casted = static_cast<PCB_TEST_SELECTION_TOOL*>( tool );

    casted->SetSelectionHook( aHook );
}
