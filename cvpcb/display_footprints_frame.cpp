/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2007-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <class_board.h>
#include <class_module.h>
#include <common.h>
#include <confirm.h>
#include <eda_dockart.h>
#include <fp_lib_table.h>
#include <id.h>
#include <lib_id.h>
#include <macros.h>
#include <msgpanel.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_painter.h>
#include <tool/action_toolbar.h>
#include <tool/common_tools.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/zoom_tool.h>

#include <cvpcb_mainframe.h>
#include <display_footprints_frame.h>
#include <tools/cvpcb_actions.h>
#include <tools/cvpcb_fpviewer_control.h>
#include <tools/cvpcb_fpviewer_selection_tool.h>

// Colors for layers and items
COLORS_DESIGN_SETTINGS g_ColorsSettings( FRAME_CVPCB_DISPLAY );


BEGIN_EVENT_TABLE( DISPLAY_FOOTPRINTS_FRAME, PCB_BASE_FRAME )
    EVT_CLOSE( DISPLAY_FOOTPRINTS_FRAME::OnCloseWindow )
    EVT_TOOL( ID_OPTIONS_SETUP, DISPLAY_FOOTPRINTS_FRAME::InstallOptionsDisplay )
    EVT_CHOICE( ID_ON_ZOOM_SELECT, DISPLAY_FOOTPRINTS_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, DISPLAY_FOOTPRINTS_FRAME::OnSelectGrid )
END_EVENT_TABLE()


DISPLAY_FOOTPRINTS_FRAME::DISPLAY_FOOTPRINTS_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    PCB_BASE_FRAME( aKiway, aParent, FRAME_CVPCB_DISPLAY, _( "Footprint Viewer" ),
                    wxDefaultPosition, wxDefaultSize,
                    KICAD_DEFAULT_DRAWFRAME_STYLE, FOOTPRINTVIEWER_FRAME_NAME )
{
    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( icon_cvpcb_xpm ) );
    SetIcon( icon );

    SetBoard( new BOARD() );
    SetScreen( new PCB_SCREEN( GetPageSizeIU() ) );

    // Don't show the default board solder mask clearance.  Only the
    // footprint or pad clearance setting should be shown if it is not 0.
    GetBoard()->GetDesignSettings().m_SolderMaskMargin = 0;

    LoadSettings( config() );

    // Initialize grid id to a default value if not found in config or incorrect:
    if( !( GetScreen()->GridExists( m_LastGridSizeId + ID_POPUP_GRID_LEVEL_1000 ) ) )
        m_LastGridSizeId = ID_POPUP_GRID_LEVEL_500 - ID_POPUP_GRID_LEVEL_1000;

    GetScreen()->SetGrid( m_LastGridSizeId + ID_POPUP_GRID_LEVEL_1000 );

    // Initialize some display options
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();
    displ_opts->m_DisplayPadIsol = false;      // Pad clearance has no meaning here

    // Track and via clearance has no meaning here.
    displ_opts->m_ShowTrackClearanceMode = PCB_DISPLAY_OPTIONS::DO_NOT_SHOW_CLEARANCE;

    // Create GAL canvas
#ifdef __WXMAC__
    // Cairo renderer doesn't handle Retina displays
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
#else
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
#endif
    auto* gal_drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                                  GetGalDisplayOptions(), backend );
    SetCanvas( gal_drawPanel );

    // Now all panels are created, set the window size to the latest saved in config:
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), gal_drawPanel->GetView(),
                                   gal_drawPanel->GetViewControls(), this );
    m_actions = new CVPCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );
    gal_drawPanel->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new CVPCB_FOOTPRINT_VIEWER_CONTROL );
    m_toolManager->RegisterTool( new CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL );
    m_toolManager->InitTools();

    // Run the control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "cvpcb.InteractiveSelection" );

    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    m_auimgr.SetManagedWindow( this );
    m_auimgr.SetArtProvider( new EDA_DOCKART() );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" ).Left().Layer(3) );
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );
    m_auimgr.Update();

    auto& galOpts = GetGalDisplayOptions();
    galOpts.m_axesEnabled = true;

    GetCanvas()->GetView()->SetScale( GetZoomLevelCoeff() / GetScreen()->GetZoom() );

    ActivateGalCanvas();

    // Restore last zoom.  (If auto-zooming we'll adjust when we load the footprint.)
    GetCanvas()->GetView()->SetScale( m_lastZoom );

    updateView();

    Show( true );
}


DISPLAY_FOOTPRINTS_FRAME::~DISPLAY_FOOTPRINTS_FRAME()
{
    GetBoard()->DeleteAllModules();
    GetCanvas()->StopDrawing();
    GetCanvas()->GetView()->Clear();
    // Be sure any event cannot be fired after frame deletion:
    GetCanvas()->SetEvtHandlerEnabled( false );

    // Be sure a active tool (if exists) is deactivated:
    if( m_toolManager )
        m_toolManager->DeactivateTool();

    delete GetScreen();
    SetScreen( NULL );      // Be sure there is no double deletion
}


void DISPLAY_FOOTPRINTS_FRAME::OnCloseWindow( wxCloseEvent& event )
{
    Destroy();
}


void DISPLAY_FOOTPRINTS_FRAME::ReCreateVToolbar()
{
    // Currently, no vertical right toolbar.
    // So do nothing
}


void DISPLAY_FOOTPRINTS_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        return;

    // Create options tool bar.
    m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                           KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    // TODO: these can be moved to the 'proper' right vertical toolbar if and when there are
    // actual tools to put there. That, or I'll get around to implementing configurable
    // toolbars.
    m_optionsToolBar->Add( ACTIONS::selectionTool,       ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::measureTool,         ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->Add( ACTIONS::toggleGrid,          ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::togglePolarCoords,   ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::imperialUnits,       ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::metricUnits,         ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::toggleCursorStyle,   ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH, wxEmptyString,
                               KiScaledBitmap( pad_sketch_xpm, this ),
                               _( "Show pads in outline mode" ), wxITEM_CHECK  );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH, wxEmptyString,
                               KiScaledBitmap( text_sketch_xpm, this ),
                               _( "Show texts in line mode" ), wxITEM_CHECK  );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH, wxEmptyString,
                               KiScaledBitmap( show_mod_edge_xpm, this ),
                               _( "Show outlines in line mode" ), wxITEM_CHECK  );

    m_optionsToolBar->Realize();
}


void DISPLAY_FOOTPRINTS_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar != NULL )
        return;

    m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                        KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    m_mainToolBar->AddTool( ID_OPTIONS_SETUP, wxEmptyString, KiScaledBitmap( config_xpm, this ),
                            _( "Display options" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::show3DViewer );

    KiScaledSeparator( m_mainToolBar, this );

    // Grid selection choice box.
    m_gridSelectBox = new wxChoice( m_mainToolBar, ID_ON_GRID_SELECT,
                                    wxDefaultPosition, wxDefaultSize, 0, NULL );
    UpdateGridSelectBox();
    m_mainToolBar->AddControl( m_gridSelectBox );

    KiScaledSeparator( m_mainToolBar, this );

    // Zoom selection choice box.
    m_zoomSelectBox = new wxChoice( m_mainToolBar, ID_ON_ZOOM_SELECT,
                                    wxDefaultPosition, wxDefaultSize, 0, NULL );
    updateZoomSelectBox();
    m_mainToolBar->AddControl( m_zoomSelectBox );

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_mainToolBar->Realize();
}


void DISPLAY_FOOTPRINTS_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    PCB_BASE_FRAME::LoadSettings( aCfg );

    m_configSettings.Load( aCfg );  // mainly, load the color config

    aCfg->Read( ConfigBaseName() + AUTO_ZOOM_KEY, &m_autoZoom, true );
    aCfg->Read( ConfigBaseName() + ZOOM_KEY, &m_lastZoom, 10.0 );
}


void DISPLAY_FOOTPRINTS_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    PCB_BASE_FRAME::SaveSettings( aCfg );

    aCfg->Write( ConfigBaseName() + AUTO_ZOOM_KEY, m_autoZoom );
    aCfg->Write( ConfigBaseName() + ZOOM_KEY, GetCanvas()->GetView()->GetScale() );
}


void DISPLAY_FOOTPRINTS_FRAME::ApplyDisplaySettingsToGAL()
{
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( GetCanvas()->GetView()->GetPainter() );

    painter->GetSettings()->LoadDisplayOptions( &m_DisplayOptions, false );

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
    GetCanvas()->Refresh();
}


/**
 * Virtual function needed by the PCB_SCREEN class derived from BASE_SCREEN
 * this is a virtual pure function in BASE_SCREEN
 * do nothing in Cvpcb
 * could be removed later
 */
void PCB_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER&, int )
{
}


bool DISPLAY_FOOTPRINTS_FRAME::IsGridVisible() const
{
    return m_drawGrid;
}


void DISPLAY_FOOTPRINTS_FRAME::SetGridVisibility(bool aVisible)
{
    m_drawGrid = aVisible;
}


COLOR4D DISPLAY_FOOTPRINTS_FRAME::GetGridColor()
{
    return COLOR4D( DARKGRAY );
}


MODULE* DISPLAY_FOOTPRINTS_FRAME::Get_Module( const wxString& aFootprintName )
{
    MODULE* footprint = NULL;

    try
    {
        LIB_ID fpid;

        if( fpid.Parse( aFootprintName, LIB_ID::ID_PCB ) >= 0 )
        {
            DisplayInfoMessage( this, wxString::Format( _( "Footprint ID \"%s\" is not valid." ),
                                                        GetChars( aFootprintName ) ) );
            return NULL;
        }

        std::string nickname = fpid.GetLibNickname();
        std::string fpname   = fpid.GetLibItemName();

        wxLogDebug( wxT( "Load footprint \"%s\" from library \"%s\"." ),
                    fpname.c_str(), nickname.c_str()  );

        footprint = Prj().PcbFootprintLibs( Kiway() )->FootprintLoad(
                FROM_UTF8( nickname.c_str() ), FROM_UTF8( fpname.c_str() ) );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return NULL;
    }

    if( footprint )
    {
        footprint->SetParent( (EDA_ITEM*) GetBoard() );
        footprint->SetPosition( wxPoint( 0, 0 ) );
        return footprint;
    }

    wxString msg = wxString::Format( _( "Footprint \"%s\" not found" ), aFootprintName.GetData() );
    DisplayError( this, msg );
    return NULL;
}


void DISPLAY_FOOTPRINTS_FRAME::InitDisplay()
{
    CVPCB_MAINFRAME*      parentframe = (CVPCB_MAINFRAME *) GetParent();
    MODULE*               module = nullptr;
    const FOOTPRINT_INFO* module_info = nullptr;

    GetBoard()->DeleteAllModules();
    GetCanvas()->GetView()->Clear();

    wxString footprintName = parentframe->GetSelectedFootprint();

    if( footprintName.IsEmpty() )
    {
        COMPONENT* comp = parentframe->GetSelectedComponent();

        if( comp )
            footprintName = comp->GetFPID().GetUniStringLibId();
    }

    if( !footprintName.IsEmpty() )
    {
        SetTitle( wxString::Format( _( "Footprint: %s" ), footprintName ) );

        module = Get_Module( footprintName );

        module_info = parentframe->m_FootprintsList->GetModuleInfo( footprintName );
    }

    if( module )
        GetBoard()->Add( module );

    if( module_info )
        SetStatusText( wxString::Format( _( "Lib: %s" ), module_info->GetLibNickname() ), 0 );
    else
        SetStatusText( wxEmptyString, 0 );

    updateView();

    UpdateStatusBar();

    GetCanvas()->Refresh();
    Update3DView( true );
}


void DISPLAY_FOOTPRINTS_FRAME::updateView()
{
    PCB_DRAW_PANEL_GAL* dp = static_cast<PCB_DRAW_PANEL_GAL*>( GetCanvas() );
    dp->UseColorScheme( &Settings().Colors() );
    dp->DisplayBoard( GetBoard() );

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    if( m_autoZoom )
        m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    else
        m_toolManager->RunAction( ACTIONS::centerContents, true );

    UpdateMsgPanel();
}


void DISPLAY_FOOTPRINTS_FRAME::UpdateMsgPanel()
{
    MODULE*         footprint = GetBoard()->GetFirstModule();
    MSG_PANEL_ITEMS items;

    if( footprint )
        footprint->GetMsgPanelInfo( m_userUnits, items );

    SetMsgPanel( items );
}


void DISPLAY_FOOTPRINTS_FRAME::SyncToolbars()
{
    m_mainToolBar->Toggle( ACTIONS::zoomTool, IsCurrentTool( ACTIONS::zoomTool ) );
    m_mainToolBar->Refresh();

    m_optionsToolBar->Toggle( ACTIONS::selectionTool, IsCurrentTool( ACTIONS::selectionTool ) );
    m_optionsToolBar->Toggle( ACTIONS::measureTool, IsCurrentTool( ACTIONS::measureTool ) );
    m_optionsToolBar->Toggle( ACTIONS::metricUnits, GetUserUnits() != INCHES );
    m_optionsToolBar->Toggle( ACTIONS::imperialUnits, GetUserUnits() == INCHES );
    m_optionsToolBar->Refresh();
}


/*
 * Redraw the BOARD items but not cursors, axis or grid.
 */
void BOARD::Print( PCB_BASE_FRAME* aFrame, wxDC* aDC, const wxPoint& aOffset )
{
    if( !m_modules.empty() )
        GetFirstModule()->Print( aFrame, aDC );
}

