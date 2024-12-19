/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2007-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <common.h>
#include <confirm.h>
#include <settings/cvpcb_settings.h>
#include <footprint_editor_settings.h>
#include <fp_lib_table.h>
#include <id.h>
#include <kiface_base.h>
#include <lib_id.h>
#include <macros.h>
#include <widgets/msgpanel.h>
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
#include <tools/pcb_editor_conditions.h>  // Shared conditions with other Pcbnew frames
#include <tools/pcb_viewer_tools.h>       // shared tools with other Pcbnew frames
#include <tools/cvpcb_fpviewer_selection_tool.h>
#include <wx/choice.h>
#include <wx/debug.h>
#include <cvpcb_id.h>
#include <project_pcb.h>

BEGIN_EVENT_TABLE( DISPLAY_FOOTPRINTS_FRAME, PCB_BASE_FRAME )
    EVT_CLOSE( DISPLAY_FOOTPRINTS_FRAME::OnCloseWindow )
    EVT_CHOICE( ID_ON_ZOOM_SELECT, DISPLAY_FOOTPRINTS_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, DISPLAY_FOOTPRINTS_FRAME::OnSelectGrid )
END_EVENT_TABLE()


DISPLAY_FOOTPRINTS_FRAME::DISPLAY_FOOTPRINTS_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        PCB_BASE_FRAME( aKiway, aParent, FRAME_CVPCB_DISPLAY, _( "Footprint Viewer" ),
                        wxDefaultPosition, wxDefaultSize,
                        KICAD_DEFAULT_DRAWFRAME_STYLE, FOOTPRINTVIEWER_FRAME_NAME ),
        m_currentComp( nullptr )
{
    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_cvpcb ) );
    SetIcon( icon );

    SetBoard( new BOARD() );

    // This board will only be used to hold a footprint for viewing
    GetBoard()->SetBoardUse( BOARD_USE::FPHOLDER );

    SetScreen( new PCB_SCREEN( GetPageSizeIU() ) );

    // Create GAL canvas before loading settings
    auto* gal_drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_frameSize,
                                                  GetGalDisplayOptions(),
                                                  EDA_DRAW_PANEL_GAL::GAL_FALLBACK );
    SetCanvas( gal_drawPanel );

    // Don't show the default board solder mask expansion.  Only the footprint or pad expansion
    // settings should be shown.
    GetBoard()->GetDesignSettings().m_SolderMaskExpansion = 0;

    LoadSettings( config() );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), gal_drawPanel->GetView(),
                                   gal_drawPanel->GetViewControls(), config(), this );
    m_actions = new CVPCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    gal_drawPanel->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL );
    m_toolManager->RegisterTool( new PCB_VIEWER_TOOLS );

    m_toolManager->GetTool<PCB_VIEWER_TOOLS>()->SetFootprintFrame( true );

    m_toolManager->InitTools();

    setupUIConditions();

    // Run the control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "cvpcb.FootprintViewerInteractiveSelection" );

    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( wxS( "MainToolbar" ) )
                      .Top().Layer( 6 ) );
    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( wxS( "OptToolbar" ) )
                      .Left().Layer( 3 ) );
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( wxS( "DrawFrame" ) )
                      .Center() );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( wxS( "MsgPanel" ) )
                      .Bottom().Layer( 6 ) );

    FinishAUIInitialization();

    auto& galOpts = GetGalDisplayOptions();
    galOpts.m_axesEnabled = true;

    ActivateGalCanvas();

    setupUnits( config() );

    // Restore last zoom.  (If auto-zooming we'll adjust when we load the footprint.)
    CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( config() );

    if( cfg )
    {
        GetCanvas()->GetView()->SetScale( cfg->m_FootprintViewerZoom );

        wxAuiToolBarItem* toolOpt = m_mainToolBar->FindTool( ID_CVPCB_FPVIEWER_AUTOZOOM_TOOL );

        if( cfg->m_FootprintViewerAutoZoomOnSelect )
            toolOpt->SetState( wxAUI_BUTTON_STATE_CHECKED );
        else
            toolOpt->SetState( 0 );
    }

    updateView();

    Show( true );

    // Register a call to update the toolbar sizes. It can't be done immediately because
    // it seems to require some sizes calculated that aren't yet (at least on GTK).
    CallAfter( [this]()
               {
                   // Ensure the controls on the toolbars all are correctly sized
                    UpdateToolbarControlSizes();
               } );
}


DISPLAY_FOOTPRINTS_FRAME::~DISPLAY_FOOTPRINTS_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    GetBoard()->DeleteAllFootprints();
    GetCanvas()->StopDrawing();
    GetCanvas()->GetView()->Clear();
    // Be sure any event cannot be fired after frame deletion:
    GetCanvas()->SetEvtHandlerEnabled( false );

    delete GetScreen();
    SetScreen( nullptr );      // Be sure there is no double deletion
    setFPWatcher( nullptr );
}


void DISPLAY_FOOTPRINTS_FRAME::setupUIConditions()
{
    PCB_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*       mgr = m_toolManager->GetActionManager();
    PCB_EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( ACTIONS::zoomTool,
                        CHECK( cond.CurrentTool( ACTIONS::zoomTool ) ) );
    mgr->SetConditions( ACTIONS::selectionTool,
                        CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );
    mgr->SetConditions( ACTIONS::measureTool,
                        CHECK( cond.CurrentTool( ACTIONS::measureTool ) ) );

    mgr->SetConditions( ACTIONS::toggleGrid,        CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::toggleCursorStyle, CHECK( cond.FullscreenCursor() ) );

    mgr->SetConditions( ACTIONS::millimetersUnits,  CHECK( cond.Units( EDA_UNITS::MILLIMETRES ) ) );
    mgr->SetConditions( ACTIONS::inchesUnits,       CHECK( cond.Units( EDA_UNITS::INCHES ) ) );
    mgr->SetConditions( ACTIONS::milsUnits,         CHECK( cond.Units( EDA_UNITS::MILS ) ) );

    mgr->SetConditions( PCB_ACTIONS::showPadNumbers,   CHECK( cond.PadNumbersDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::padDisplayMode,   CHECK( !cond.PadFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::textOutlines,     CHECK( !cond.TextFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::graphicsOutlines, CHECK( !cond.GraphicsFillDisplay() ) );

#undef CHECK
}


void DISPLAY_FOOTPRINTS_FRAME::ReCreateVToolbar()
{
    // Currently, no vertical right toolbar.
}


void DISPLAY_FOOTPRINTS_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
    {
        m_optionsToolBar->ClearToolbar();
    }
    else
    {
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR, wxDefaultPosition,
                                               wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );
        m_optionsToolBar->SetAuiManager( &m_auimgr );
    }

    m_optionsToolBar->Add( ACTIONS::selectionTool,          ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::measureTool,            ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( ACTIONS::toggleGrid,             ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::togglePolarCoords,      ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::inchesUnits,            ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::milsUnits,              ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::millimetersUnits,       ACTION_TOOLBAR::TOGGLE );
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
    {
        m_mainToolBar->ClearToolbar();
    }
    else
    {
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );
        m_mainToolBar->SetAuiManager( &m_auimgr );
    }

    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE, ACTION_TOOLBAR::CANCEL );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::show3DViewer );

    m_mainToolBar->AddScaledSeparator( this );

    // Grid selection choice box.
    if( !m_gridSelectBox )
        m_gridSelectBox = new wxChoice( m_mainToolBar, ID_ON_GRID_SELECT );

    UpdateGridSelectBox();
    m_mainToolBar->AddControl( m_gridSelectBox );

    m_mainToolBar->AddScaledSeparator( this );

    // Zoom selection choice box.
    if( !m_zoomSelectBox )
        m_zoomSelectBox = new wxChoice( m_mainToolBar, ID_ON_ZOOM_SELECT );

    UpdateZoomSelectBox();
    m_mainToolBar->AddControl( m_zoomSelectBox );

    // Option to run Zoom automatique on footprint selection changge
    m_mainToolBar->AddTool( ID_CVPCB_FPVIEWER_AUTOZOOM_TOOL, wxEmptyString,
                            KiScaledBitmap( BITMAPS::zoom_auto_fit_in_page, this ),
                            _( "Automatic Zoom on footprint change" ),
                            wxITEM_CHECK );

    m_mainToolBar->AddScaledSeparator( this );

    m_mainToolBar->UpdateControlWidth( ID_ON_GRID_SELECT );
    m_mainToolBar->UpdateControlWidth( ID_ON_ZOOM_SELECT );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void DISPLAY_FOOTPRINTS_FRAME::UpdateToolbarControlSizes()
{
    if( m_mainToolBar )
    {
        // Update the item widths
        m_mainToolBar->UpdateControlWidth( ID_ON_GRID_SELECT );
        m_mainToolBar->UpdateControlWidth( ID_ON_ZOOM_SELECT );
    }
}


void DISPLAY_FOOTPRINTS_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( aCfg );
    wxCHECK( cfg, /* void */ );

    // We don't allow people to change this right now, so make sure it's on
    GetWindowSettings( cfg )->cursor.always_show_cursor = true;

    PCB_BASE_FRAME::LoadSettings( cfg );

    SetDisplayOptions( cfg->m_FootprintViewerDisplayOptions );
}


void DISPLAY_FOOTPRINTS_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( aCfg );
    wxCHECK( cfg, /* void */ );

    PCB_BASE_FRAME::SaveSettings( cfg );

    cfg->m_FootprintViewerDisplayOptions = GetDisplayOptions();

    cfg->m_FootprintViewerZoom = GetCanvas()->GetView()->GetScale();

    wxAuiToolBarItem* toolOpt = m_mainToolBar->FindTool( ID_CVPCB_FPVIEWER_AUTOZOOM_TOOL );
    cfg->m_FootprintViewerAutoZoomOnSelect = ( toolOpt->GetState() & wxAUI_BUTTON_STATE_CHECKED );
}


WINDOW_SETTINGS* DISPLAY_FOOTPRINTS_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    CVPCB_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<CVPCB_SETTINGS>( "cvpcb" );
    return &cfg->m_FootprintViewer;
}


PCB_VIEWERS_SETTINGS_BASE* DISPLAY_FOOTPRINTS_FRAME::GetViewerSettingsBase() const
{
    return Pgm().GetSettingsManager().GetAppSettings<CVPCB_SETTINGS>( "cvpcb" );
}


MAGNETIC_SETTINGS* DISPLAY_FOOTPRINTS_FRAME::GetMagneticItemsSettings()
{
    CVPCB_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<CVPCB_SETTINGS>( "cvpcb" );
    return &cfg->m_FootprintViewerMagneticSettings;
}


COLOR4D DISPLAY_FOOTPRINTS_FRAME::GetGridColor()
{
    return COLOR4D( DARKGRAY );
}


FOOTPRINT* DISPLAY_FOOTPRINTS_FRAME::GetFootprint( const wxString& aFootprintName,
                                                   REPORTER& aReporter )
{
    FOOTPRINT* footprint = nullptr;
    LIB_ID     fpid;

    if( fpid.Parse( aFootprintName ) >= 0 )
    {
        aReporter.Report( wxString::Format( _( "Footprint ID '%s' is not valid." ),
                                            aFootprintName ),
                          RPT_SEVERITY_ERROR );
        return nullptr;
    }

    wxString libNickname = From_UTF8( fpid.GetLibNickname().c_str() );
    wxString fpName      = From_UTF8( fpid.GetLibItemName().c_str() );

    FP_LIB_TABLE* fpTable = PROJECT_PCB::PcbFootprintLibs( &Prj() );
    wxASSERT( fpTable );

    // See if the library requested is in the library table
    if( !fpTable->HasLibrary( libNickname ) )
    {
        aReporter.Report( wxString::Format( _( "Library '%s' is not in the footprint library table." ),
                                            libNickname ),
                          RPT_SEVERITY_ERROR );
        return nullptr;
    }

    // See if the footprint requested is in the library
    if( !fpTable->FootprintExists( libNickname, fpName ) )
    {
        aReporter.Report( wxString::Format( _( "Footprint '%s' not found." ), aFootprintName ),
                          RPT_SEVERITY_ERROR );
        return nullptr;
    }

    try
    {
        const FOOTPRINT* fp = fpTable->GetEnumeratedFootprint( libNickname, fpName );

        if( fp )
            footprint = static_cast<FOOTPRINT*>( fp->Duplicate() );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return nullptr;
    }

    if( footprint )
    {
        footprint->SetFPID( fpid );
        footprint->SetParent( (EDA_ITEM*) GetBoard() );
        footprint->SetPosition( VECTOR2I( 0, 0 ) );
        return footprint;
    }

    aReporter.Report( wxString::Format( _( "Footprint '%s' not found." ), aFootprintName ),
                      RPT_SEVERITY_ERROR );
    return nullptr;
}


void DISPLAY_FOOTPRINTS_FRAME::ReloadFootprint( FOOTPRINT* aFootprint )
{
    if( !aFootprint || !m_currentComp )
        return;

    GetBoard()->DeleteAllFootprints();
    GetCanvas()->GetView()->Clear();

    for( PAD* pad : aFootprint->Pads() )
    {
        const COMPONENT_NET& net = m_currentComp->GetNet( pad->GetNumber() );

        if( !net.GetPinFunction().IsEmpty() )
            pad->SetPinFunction( net.GetPinFunction() );
    }

    GetBoard()->Add( aFootprint );
    updateView();
    GetCanvas()->Refresh();
}


void DISPLAY_FOOTPRINTS_FRAME::InitDisplay()
{
    CVPCB_MAINFRAME*      parentframe = (CVPCB_MAINFRAME *) GetParent();
    COMPONENT*            comp = parentframe->GetSelectedComponent();
    FOOTPRINT*            footprint = nullptr;
    const FOOTPRINT_INFO* fpInfo = nullptr;

    wxString footprintName = parentframe->GetSelectedFootprint();

    if( footprintName.IsEmpty() && comp )
        footprintName = comp->GetFPID().Format().wx_str();

    if( m_currentFootprint == footprintName && m_currentComp == comp )
        return;

    GetBoard()->DeleteAllFootprints();
    GetCanvas()->GetView()->Clear();

    INFOBAR_REPORTER infoReporter( m_infoBar );
    m_infoBar->Dismiss();

    if( !footprintName.IsEmpty() )
    {
        SetTitle( wxString::Format( _( "Footprint: %s" ), footprintName ) );

        footprint = GetFootprint( footprintName, infoReporter );

        fpInfo = parentframe->m_FootprintsList->GetFootprintInfo( footprintName );
    }

    if( footprint )
    {
        if( comp )
        {
            for( PAD* pad : footprint->Pads() )
            {
                const COMPONENT_NET& net = comp->GetNet( pad->GetNumber() );

                if( !net.GetPinFunction().IsEmpty() )
                    pad->SetPinFunction( net.GetPinFunction() );
            }
        }

        GetBoard()->Add( footprint );
        GetCanvas()->GetView()->Update( footprint );
        m_currentFootprint = footprintName;
        m_currentComp = comp;
        setFPWatcher( footprint );
    }

    if( fpInfo )
        SetStatusText( wxString::Format( _( "Lib: %s" ), fpInfo->GetLibNickname() ), 0 );
    else
        SetStatusText( wxEmptyString, 0 );

    infoReporter.Finalize();

    updateView();

    UpdateStatusBar();

    GetCanvas()->Refresh();
    Update3DView( true, true );
}


void DISPLAY_FOOTPRINTS_FRAME::updateView()
{
    PCB_DRAW_PANEL_GAL* dp = static_cast<PCB_DRAW_PANEL_GAL*>( GetCanvas() );
    dp->UpdateColors();
    dp->DisplayBoard( GetBoard() );

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    wxAuiToolBarItem* toolOpt = m_mainToolBar->FindTool( ID_CVPCB_FPVIEWER_AUTOZOOM_TOOL );

    if( toolOpt->GetState() & wxAUI_BUTTON_STATE_CHECKED )
        m_toolManager->RunAction( ACTIONS::zoomFitScreen );
    else
        m_toolManager->RunAction( ACTIONS::centerContents );

    UpdateMsgPanel();
}


void DISPLAY_FOOTPRINTS_FRAME::UpdateMsgPanel()
{
    FOOTPRINT*                  footprint = GetBoard()->GetFirstFootprint();
    std::vector<MSG_PANEL_ITEM> items;

    if( footprint )
        footprint->GetMsgPanelInfo( this, items );

    SetMsgPanel( items );
}


COLOR_SETTINGS* DISPLAY_FOOTPRINTS_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    auto* cfg = Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );

    if( cfg )
        return Pgm().GetSettingsManager().GetColorSettings( cfg->m_ColorTheme );
    else
        return Pgm().GetSettingsManager().GetColorSettings();
}


BOARD_ITEM_CONTAINER* DISPLAY_FOOTPRINTS_FRAME::GetModel() const
{
    return GetBoard()->GetFirstFootprint();
}


SELECTION& DISPLAY_FOOTPRINTS_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL>()->GetSelection();
}
