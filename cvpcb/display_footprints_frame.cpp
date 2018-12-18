/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2007-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file display_footprints_frame.cpp
 */

#include <fctsys.h>
#include <common.h>
#include <gal/graphics_abstraction_layer.h>
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>
#include <pcb_draw_panel_gal.h>
#include <confirm.h>
#include <macros.h>
#include <bitmaps.h>
#include <msgpanel.h>
#include <wildcards_and_files_ext.h>
#include <lib_id.h>
#include <fp_lib_table.h>
#include <eda_dockart.h>

#include <io_mgr.h>
#include <class_module.h>
#include <class_board.h>
#include <pcb_painter.h>

#include <cvpcb_mainframe.h>
#include <display_footprints_frame.h>
#include <cvpcb_id.h>
#include <listboxes.h>

#include <3d_viewer/eda_3d_viewer.h>
#include <view/view.h>

#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/common_tools.h>
#include "tools/cvpcb_actions.h"

// Colors for layers and items
COLORS_DESIGN_SETTINGS g_ColorsSettings( FRAME_CVPCB_DISPLAY );


BEGIN_EVENT_TABLE( DISPLAY_FOOTPRINTS_FRAME, PCB_BASE_FRAME )
    EVT_CLOSE( DISPLAY_FOOTPRINTS_FRAME::OnCloseWindow )
    EVT_TOOL( ID_OPTIONS_SETUP, DISPLAY_FOOTPRINTS_FRAME::InstallOptionsDisplay )
    EVT_TOOL( ID_CVPCB_SHOW3D_FRAME, DISPLAY_FOOTPRINTS_FRAME::Show3D_Frame )
    EVT_COMBOBOX( ID_ON_ZOOM_SELECT, DISPLAY_FOOTPRINTS_FRAME::OnSelectZoom )
    EVT_COMBOBOX( ID_ON_GRID_SELECT, DISPLAY_FOOTPRINTS_FRAME::OnSelectGrid )

    EVT_UPDATE_UI( ID_NO_TOOL_SELECTED, DISPLAY_FOOTPRINTS_FRAME::OnUIToolSelection )
    EVT_UPDATE_UI( ID_TB_MEASUREMENT_TOOL, DISPLAY_FOOTPRINTS_FRAME::OnUIToolSelection )
    EVT_UPDATE_UI( ID_ZOOM_SELECTION, DISPLAY_FOOTPRINTS_FRAME::OnUIToolSelection )

    /*
    EVT_TOOL  and EVT_UPDATE_UI for:
      ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH,
      ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH,
      ID_TB_OPTIONS_SHOW_PADS_SKETCH
      are managed in PCB_BASE_FRAME
    */
END_EVENT_TABLE()


DISPLAY_FOOTPRINTS_FRAME::DISPLAY_FOOTPRINTS_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    PCB_BASE_FRAME( aKiway, aParent, FRAME_CVPCB_DISPLAY, _( "Footprint Viewer" ),
                    wxDefaultPosition, wxDefaultSize,
                    KICAD_DEFAULT_DRAWFRAME_STYLE, FOOTPRINTVIEWER_FRAME_NAME )
{
    m_showAxis = true;         // true to draw axis.

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

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    // Create GAL canvas
#ifdef __WXMAC__
    // Cairo renderer doesn't handle Retina displays
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
#else
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
#endif
    auto* gal_drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                                  GetGalDisplayOptions(), backend );
    SetGalCanvas( gal_drawPanel );

    m_auimgr.SetManagedWindow( this );
    m_auimgr.SetArtProvider( new EDA_DOCKART( this ) );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" ).Left().Layer(3) );

    m_auimgr.AddPane( m_canvas, EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );
    m_auimgr.AddPane( GetGalCanvas(), EDA_PANE().Canvas().Name( "DrawFrameGal" ).Center().Hide() );

    m_auimgr.Update();

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), gal_drawPanel->GetView(),
                                   gal_drawPanel->GetViewControls(), this );
    m_actions = new CVPCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );
    gal_drawPanel->SetEventDispatcher( m_toolDispatcher );

    m_actions->RegisterAllTools( m_toolManager );
    m_toolManager->InitTools();

    // Run the control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "cvpcb.InteractiveSelection" );

    auto& galOpts = GetGalDisplayOptions();
    galOpts.m_axesEnabled = true;
    UseGalCanvas( true );

    // Restore last zoom.  (If auto-zooming we'll adjust when we load the footprint.)
    GetGalCanvas()->GetView()->SetScale( m_lastZoom );

    updateView();

    Show( true );
}


DISPLAY_FOOTPRINTS_FRAME::~DISPLAY_FOOTPRINTS_FRAME()
{
    GetGalCanvas()->StopDrawing();
    GetGalCanvas()->GetView()->Clear();
    // Be sure any event cannot be fired after frame deletion:
    GetGalCanvas()->SetEvtHandlerEnabled( false );

    // Be sure a active tool (if exists) is desactivated:
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
    m_optionsToolBar = new wxAuiToolBar( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                         KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    // TODO: these can be moved to the 'proper' right vertical toolbar if and when there are
    // actual tools to put there. That, or I'll get around to implementing configurable
    // toolbars.
    m_optionsToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString,
                               KiScaledBitmap( cursor_xpm, this ),
                               wxEmptyString, wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_MEASUREMENT_TOOL, wxEmptyString,
                                   KiScaledBitmap( measurement_xpm, this ),
                                   _( "Measure distance between two points" ),
                                   wxITEM_CHECK );

    KiScaledSeparator( m_optionsToolBar, this );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, KiScaledBitmap( grid_xpm, this ),
                               _( "Hide grid" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_POLAR_COORD, wxEmptyString,
                               KiScaledBitmap( polar_coord_xpm, this ),
                               _( "Display polar coordinates" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiScaledBitmap( unit_inch_xpm, this ),
                               _( "Set units to inches" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiScaledBitmap( unit_mm_xpm, this ),
                               _( "Set units to millimeters" ), wxITEM_CHECK );

#ifndef __APPLE__
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiScaledBitmap( cursor_shape_xpm, this ),
                               _( "Change cursor shape" ), wxITEM_CHECK  );
#else
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiScaledBitmap( cursor_shape_xpm, this ),
                               _( "Change cursor shape (not supported in Legacy Toolset)" ),
                               wxITEM_CHECK  );
#endif

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

    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    m_mainToolBar->AddTool( ID_OPTIONS_SETUP, wxEmptyString, KiScaledBitmap( config_xpm, this ),
                            _( "Display options" ) );

    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiScaledBitmap( zoom_in_xpm, this ),
                            _( "Zoom in (F1)" ) );

    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiScaledBitmap( zoom_out_xpm, this ),
                            _( "Zoom out (F2)" ) );

    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiScaledBitmap( zoom_redraw_xpm, this ),
                            _( "Redraw view (F3)" ) );

    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiScaledBitmap( zoom_fit_in_page_xpm, this ),
                            _( "Zoom to fit footprint (Home)" ) );

    m_mainToolBar->AddTool( ID_ZOOM_SELECTION, wxEmptyString, KiScaledBitmap( zoom_area_xpm, this ),
                            _( "Zoom to selection" ), wxITEM_CHECK );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_CVPCB_SHOW3D_FRAME, wxEmptyString, KiScaledBitmap( three_d_xpm, this ),
                            _( "3D Display (Alt+3)" ) );

    KiScaledSeparator( m_mainToolBar, this );

    // Grid selection choice box.
    m_gridSelectBox = new wxComboBox( m_mainToolBar, ID_ON_GRID_SELECT, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY );
    UpdateGridSelectBox();
    m_mainToolBar->AddControl( m_gridSelectBox );

    KiScaledSeparator( m_mainToolBar, this );

    // Zoom selection choice box.
    m_zoomSelectBox = new wxComboBox( m_mainToolBar, ID_ON_ZOOM_SELECT, wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY );
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
    aCfg->Write( ConfigBaseName() + ZOOM_KEY, GetGalCanvas()->GetView()->GetScale() );
}


void DISPLAY_FOOTPRINTS_FRAME::ApplyDisplaySettingsToGAL()
{
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( GetGalCanvas()->GetView()->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings = painter->GetSettings();
    settings->LoadDisplayOptions( &m_DisplayOptions, false );

    GetGalCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
    GetGalCanvas()->Refresh();
}


void DISPLAY_FOOTPRINTS_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


void DISPLAY_FOOTPRINTS_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
{
}


bool DISPLAY_FOOTPRINTS_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    return true;
}


bool DISPLAY_FOOTPRINTS_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition,
        EDA_KEY aHotKey )
{
    bool eventHandled = true;

    // Filter out the 'fake' mouse motion after a keyboard movement
    if( !aHotKey && m_movingCursorWithKeyboard )
    {
        m_movingCursorWithKeyboard = false;
        return false;
    }

    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    wxPoint pos = aPosition;
    wxPoint oldpos = GetCrossHairPosition();
    GeneralControlKeyMovement( aHotKey, &pos, true );

    switch( aHotKey )
    {
    case WXK_F1:
        cmd.SetId( ID_KEY_ZOOM_IN );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F2:
        cmd.SetId( ID_KEY_ZOOM_OUT );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F3:
        cmd.SetId( ID_ZOOM_REDRAW );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_F4:
        cmd.SetId( ID_POPUP_ZOOM_CENTER );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case WXK_HOME:
        cmd.SetId( ID_ZOOM_PAGE );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case ' ':
        GetScreen()->m_O_Curseur = GetCrossHairPosition();
        break;

    case GR_KB_ALT + '3':
        cmd.SetId( ID_CVPCB_SHOW3D_FRAME );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    default:
        eventHandled = false;
    }

    SetCrossHairPosition( pos );
    RefreshCrossHair( oldpos, aPosition, aDC );

    UpdateStatusBar();    /* Display new cursor coordinates */

    return eventHandled;
}


void DISPLAY_FOOTPRINTS_FRAME::Show3D_Frame( wxCommandEvent& event )
{
    bool forceRecreateIfNotOwner = true;
    CreateAndShow3D_Frame( forceRecreateIfNotOwner );
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

    if( GetBoard()->m_Modules.GetCount() )
        GetBoard()->m_Modules.DeleteAll();

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
        GetBoard()->m_Modules.PushBack( module );

    if( module_info )
        SetStatusText( wxString::Format( _( "Lib: %s" ), module_info->GetLibNickname() ), 0 );
    else
        SetStatusText( wxEmptyString, 0 );

    updateView();

    UpdateStatusBar();

    GetCanvas()->Refresh();
    Update3DView();
}


void DISPLAY_FOOTPRINTS_FRAME::updateView()
{
    PCB_DRAW_PANEL_GAL* dp = static_cast<PCB_DRAW_PANEL_GAL*>( GetGalCanvas() );
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
    MODULE* footprint = GetBoard()->m_Modules;
    MSG_PANEL_ITEMS items;

    if( footprint )
        footprint->GetMsgPanelInfo( m_UserUnits, items );

    SetMsgPanel( items );
}


void DISPLAY_FOOTPRINTS_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( GetBoard() )
        UpdateMsgPanel();
}


void DISPLAY_FOOTPRINTS_FRAME::OnUIToolSelection( wxUpdateUIEvent& aEvent )
{
    switch( aEvent.GetId() )
    {
    case ID_TB_MEASUREMENT_TOOL:
        aEvent.Check( GetToolId() == ID_TB_MEASUREMENT_TOOL );
        break;

    case ID_NO_TOOL_SELECTED:
        aEvent.Check( GetToolId() == ID_NO_TOOL_SELECTED );
        break;

    case ID_ZOOM_SELECTION:
        aEvent.Check( GetToolId() == ID_ZOOM_SELECTION );
        break;

    default:
        break;
    }
}


/*
 * Redraw the BOARD items but not cursors, axis or grid.
 */
void BOARD::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                  GR_DRAWMODE aDrawMode, const wxPoint& aOffset )
{
    if( m_Modules )
    {
        m_Modules->Draw( aPanel, aDC, GR_COPY );
    }
}
