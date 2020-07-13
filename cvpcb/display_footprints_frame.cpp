/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2007-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <cvpcb_settings.h>
#include <footprint_editor_settings.h>
#include <fp_lib_table.h>
#include <id.h>
#include <kiface_i.h>
#include <lib_id.h>
#include <macros.h>
#include <msgpanel.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_painter.h>
#include <pgm_base.h>
#include <reporter.h>
#include <settings/settings_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_tools.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/zoom_tool.h>
#include <cvpcb_mainframe.h>
#include <display_footprints_frame.h>
#include <tools/cvpcb_actions.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_viewer_tools.h>       // shared tools with other pcbnew frames
#include <tools/cvpcb_fpviewer_selection_tool.h>
#include <widgets/infobar.h>


BEGIN_EVENT_TABLE( DISPLAY_FOOTPRINTS_FRAME, PCB_BASE_FRAME )
    EVT_CLOSE( DISPLAY_FOOTPRINTS_FRAME::OnCloseWindow )
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

    // Create GAL canvas before loading settings
#ifdef __WXMAC__
    // Cairo renderer doesn't handle Retina displays
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
#else
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
#endif
    auto* gal_drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                                  GetGalDisplayOptions(), backend );
    SetCanvas( gal_drawPanel );

    // Don't show the default board solder mask clearance.  Only the
    // footprint or pad clearance setting should be shown if it is not 0.
    GetBoard()->GetDesignSettings().m_SolderMaskMargin = 0;

    LoadSettings( config() );

    // Initialize some display options
    auto displ_opts = GetDisplayOptions();
    displ_opts.m_DisplayPadIsol = false;      // Pad clearance has no meaning here

    // Track and via clearance has no meaning here.
    displ_opts.m_ShowTrackClearanceMode = PCB_DISPLAY_OPTIONS::DO_NOT_SHOW_CLEARANCE;
    SetDisplayOptions( displ_opts );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), gal_drawPanel->GetView(),
                                   gal_drawPanel->GetViewControls(), config(), this );
    m_actions = new CVPCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );
    gal_drawPanel->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL );
    m_toolManager->RegisterTool( new PCB_VIEWER_TOOLS );

    m_toolManager->GetTool<PCB_VIEWER_TOOLS>()->SetFootprintFrame( true );

    m_toolManager->InitTools();

    // Run the control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "cvpcb.FootprintViewerInteractiveSelection" );

    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    // Create the infobar
    m_infoBar = new WX_INFOBAR( this, &m_auimgr );

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" ).Left().Layer(3) );
    m_auimgr.AddPane( m_infoBar, EDA_PANE().InfoBar().Name( "InfoBar" ).Top().Layer(1) );
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    // Call Update() to fix all pane default sizes, especially the "InfoBar" pane before
    // hidding it.
    m_auimgr.Update();

    // We don't want the infobar displayed right away
    m_auimgr.GetPane( "InfoBar" ).Hide();
    m_auimgr.Update();

    auto& galOpts = GetGalDisplayOptions();
    galOpts.m_axesEnabled = true;

    ActivateGalCanvas();

    // Restore last zoom.  (If auto-zooming we'll adjust when we load the footprint.)
    CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( config() );

    if( cfg )
        GetCanvas()->GetView()->SetScale( cfg->m_FootprintViewerZoom );

    updateView();

    Show( true );
}


DISPLAY_FOOTPRINTS_FRAME::~DISPLAY_FOOTPRINTS_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    GetBoard()->DeleteAllModules();
    GetCanvas()->StopDrawing();
    GetCanvas()->GetView()->Clear();
    // Be sure any event cannot be fired after frame deletion:
    GetCanvas()->SetEvtHandlerEnabled( false );

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
}


void DISPLAY_FOOTPRINTS_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        m_optionsToolBar->ClearToolbar();
    else
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    m_optionsToolBar->Add( ACTIONS::selectionTool,          ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::measureTool,            ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( ACTIONS::toggleGrid,             ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::togglePolarCoords,      ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::imperialUnits,          ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::metricUnits,            ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::toggleCursorStyle,      ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( PCB_ACTIONS::showPadNumbers,     ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::padDisplayMode,     ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::textOutlines,       ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::graphicsOutlines,   ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->Realize();
}


void DISPLAY_FOOTPRINTS_FRAME::ReCreateHToolbar()
{
    // Note:
    // To rebuild the aui toolbar, the more easy way is to clear ( calling m_mainToolBar.Clear() )
    // all wxAuiToolBarItems.
    // However the wxAuiToolBarItems are not the owners of controls managed by
    // them ( m_zoomSelectBox and m_gridSelectBox ), and therefore do not delete them
    // So we do not recreate them after clearing the tools.

    if( m_mainToolBar )
        m_mainToolBar->ClearToolbar();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool,                       ACTION_TOOLBAR::TOGGLE );
    m_mainToolBar->Add( PCB_ACTIONS::zoomFootprintAutomatically, ACTION_TOOLBAR::TOGGLE );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::show3DViewer );

    m_mainToolBar->AddScaledSeparator( this );

    // Grid selection choice box.
    if( !m_gridSelectBox )
        m_gridSelectBox = new wxChoice( m_mainToolBar, ID_ON_GRID_SELECT,
                                        wxDefaultPosition, wxDefaultSize, 0, NULL );

    UpdateGridSelectBox();
    m_mainToolBar->AddControl( m_gridSelectBox );

    m_mainToolBar->AddScaledSeparator( this );

    // Zoom selection choice box.
    if( !m_zoomSelectBox )
        m_zoomSelectBox = new wxChoice( m_mainToolBar, ID_ON_ZOOM_SELECT,
                                        wxDefaultPosition, wxDefaultSize, 0, NULL );

    UpdateZoomSelectBox();
    m_mainToolBar->AddControl( m_zoomSelectBox );

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_mainToolBar->Realize();
}


void DISPLAY_FOOTPRINTS_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    auto cfg = dynamic_cast<CVPCB_SETTINGS*>( aCfg );
    wxCHECK( cfg, /* void */ );

    // We don't allow people to change this right now, so make sure it's on
    GetWindowSettings( cfg )->cursor.always_show_cursor = true;

    PCB_BASE_FRAME::LoadSettings( cfg );

    SetDisplayOptions( cfg->m_FootprintViewerDisplayOptions );
}


void DISPLAY_FOOTPRINTS_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    auto cfg = dynamic_cast<CVPCB_SETTINGS*>( aCfg );
    wxCHECK( cfg, /* void */ );

    PCB_BASE_FRAME::SaveSettings( cfg );

    cfg->m_FootprintViewerDisplayOptions = GetDisplayOptions();

    cfg->m_FootprintViewerZoom = GetCanvas()->GetView()->GetScale();
}


WINDOW_SETTINGS* DISPLAY_FOOTPRINTS_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    auto cfg = dynamic_cast<CVPCB_SETTINGS*>( aCfg );
    wxCHECK( cfg, nullptr );
    return &cfg->m_FootprintViewer;
}


MAGNETIC_SETTINGS* DISPLAY_FOOTPRINTS_FRAME::GetMagneticItemsSettings()
{
    auto cfg = dynamic_cast<CVPCB_SETTINGS*>( Kiface().KifaceSettings() );
    wxCHECK( cfg, nullptr );
    return &cfg->m_FootprintViewerMagneticSettings;
}


COLOR4D DISPLAY_FOOTPRINTS_FRAME::GetGridColor()
{
    return COLOR4D( DARKGRAY );
}


MODULE* DISPLAY_FOOTPRINTS_FRAME::GetModule( const wxString& aFootprintName, REPORTER& aReporter )
{
    MODULE* footprint = NULL;

    LIB_ID fpid;

    if( fpid.Parse( aFootprintName, LIB_ID::ID_PCB ) >= 0 )
    {
        aReporter.Report( wxString::Format( _( "Footprint ID \"%s\" is not valid." ),
                                            aFootprintName ),
                          RPT_SEVERITY_ERROR );
        return NULL;
    }

    wxString libNickname = FROM_UTF8( fpid.GetLibNickname().c_str() );
    wxString fpName      = FROM_UTF8( fpid.GetLibItemName().c_str() );

    FP_LIB_TABLE* fpTable = Prj().PcbFootprintLibs( Kiway() );
    wxASSERT( fpTable );

    // See if the library requested is in the library table
    if( !fpTable->HasLibrary( libNickname ) )
    {
        aReporter.Report( wxString::Format( _( "Library \"%s\" is not in the footprint library table." ),
                                            libNickname ),
                          RPT_SEVERITY_ERROR );
        return NULL;
    }

    // See if the footprint requested is in the library
    if( !fpTable->FootprintExists( libNickname, fpName ) )
    {
        aReporter.Report( wxString::Format( _( "Footprint \"%s\" not found." ), aFootprintName ),
                          RPT_SEVERITY_ERROR );
        return NULL;
    }

    try
    {
        footprint = fpTable->FootprintLoad( libNickname, fpName );
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

    aReporter.Report( wxString::Format( _( "Footprint \"%s\" not found." ), aFootprintName ),
                      RPT_SEVERITY_ERROR );
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

    INFOBAR_REPORTER infoReporter( m_infoBar );
    m_infoBar->Dismiss();

    if( !footprintName.IsEmpty() )
    {
        SetTitle( wxString::Format( _( "Footprint: %s" ), footprintName ) );

        module = GetModule( footprintName, infoReporter );

        module_info = parentframe->m_FootprintsList->GetModuleInfo( footprintName );
    }

    if( module )
        GetBoard()->Add( module );

    if( module_info )
        SetStatusText( wxString::Format( _( "Lib: %s" ), module_info->GetLibNickname() ), 0 );
    else
        SetStatusText( wxEmptyString, 0 );

    infoReporter.Finalize();

    updateView();

    UpdateStatusBar();

    GetCanvas()->Refresh();
    Update3DView( true );
}


void DISPLAY_FOOTPRINTS_FRAME::updateView()
{
    PCB_DRAW_PANEL_GAL* dp = static_cast<PCB_DRAW_PANEL_GAL*>( GetCanvas() );
    dp->UpdateColors();
    dp->DisplayBoard( GetBoard() );

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    if( GetAutoZoom() )
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
        footprint->GetMsgPanelInfo( this, items );

    SetMsgPanel( items );
}


void DISPLAY_FOOTPRINTS_FRAME::SyncToolbars()
{
    m_mainToolBar->Toggle( ACTIONS::zoomTool, IsCurrentTool( ACTIONS::zoomTool ) );
    m_mainToolBar->Toggle( PCB_ACTIONS::zoomFootprintAutomatically, GetAutoZoom() );
    m_mainToolBar->Refresh();

    m_optionsToolBar->Toggle( ACTIONS::toggleGrid,    IsGridVisible() );
    m_optionsToolBar->Toggle( ACTIONS::selectionTool, IsCurrentTool( ACTIONS::selectionTool ) );
    m_optionsToolBar->Toggle( ACTIONS::measureTool,   IsCurrentTool( ACTIONS::measureTool ) );
    m_optionsToolBar->Toggle( ACTIONS::metricUnits,   GetUserUnits() != EDA_UNITS::INCHES );
    m_optionsToolBar->Toggle( ACTIONS::imperialUnits, GetUserUnits() == EDA_UNITS::INCHES );

    const PCB_DISPLAY_OPTIONS& opts = GetDisplayOptions();

    m_optionsToolBar->Toggle( PCB_ACTIONS::showPadNumbers,     opts.m_DisplayPadNum );
    m_optionsToolBar->Toggle( PCB_ACTIONS::padDisplayMode,     !opts.m_DisplayPadFill );
    m_optionsToolBar->Toggle( PCB_ACTIONS::textOutlines,       !opts.m_DisplayTextFill );
    m_optionsToolBar->Toggle( PCB_ACTIONS::graphicsOutlines,   !opts.m_DisplayGraphicsFill );

    m_optionsToolBar->Refresh();
}


COLOR_SETTINGS* DISPLAY_FOOTPRINTS_FRAME::GetColorSettings()
{
    auto* settings = Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>();

    if( settings )
        return Pgm().GetSettingsManager().GetColorSettings( settings->m_ColorTheme );
    else
        return Pgm().GetSettingsManager().GetColorSettings();
}


BOARD_ITEM_CONTAINER* DISPLAY_FOOTPRINTS_FRAME::GetModel() const
{
    return GetBoard()->GetFirstModule();
}


void DISPLAY_FOOTPRINTS_FRAME::SetAutoZoom( bool aAutoZoom )
{
    CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( config() );
    wxCHECK( cfg, /* void */ );
    cfg->m_FootprintViewerAutoZoom = aAutoZoom;
}


bool DISPLAY_FOOTPRINTS_FRAME::GetAutoZoom()
{
    CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( config() );
    wxCHECK( cfg, false );
    return cfg->m_FootprintViewerAutoZoom;
}
