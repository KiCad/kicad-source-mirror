/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <footprint_library_adapter.h>
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
#include <string_utils.h>
#include <tools/cvpcb_actions.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_editor_conditions.h>  // Shared conditions with other Pcbnew frames
#include <tools/pcb_viewer_tools.h>       // shared tools with other Pcbnew frames
#include <tools/cvpcb_fpviewer_selection_tool.h>
#include <toolbars_display_footprints.h>
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

    m_toolbarSettings = GetToolbarSettings<DISPLAY_FOOTPRINTS_TOOLBAR_SETTINGS>( "display_footprints-toolbars" );
    configureToolbars();
    RecreateToolbars();

    // Run the control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "common.InteractiveSelection" );

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( wxS( "TopMainToolbar" ) )
                      .Top().Layer( 6 ) );
    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( wxS( "LeftToolbar" ) )
                      .Left().Layer( 3 ) );
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( wxS( "DrawFrame" ) )
                      .Center() );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( wxS( "MsgPanel" ) )
                      .Bottom().Layer( 6 ) );

    RestoreAuiLayout();
    FinishAUIInitialization();

    auto& galOpts = GetGalDisplayOptions();
    galOpts.m_axesEnabled = true;

    ActivateGalCanvas();

    setupUnits( config() );

    // Restore last zoom.  (If auto-zooming we'll adjust when we load the footprint.)
    CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( config() );

    if( cfg )
        GetCanvas()->GetView()->SetScale( cfg->m_FootprintViewerZoom );

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
    mgr->SetConditions( ACTIONS::zoomTool,             CHECK( cond.CurrentTool( ACTIONS::zoomTool ) ) );
    mgr->SetConditions( ACTIONS::selectionTool,        CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );
    mgr->SetConditions( ACTIONS::measureTool,          CHECK( cond.CurrentTool( ACTIONS::measureTool ) ) );

    mgr->SetConditions( ACTIONS::toggleGrid,           CHECK( cond.GridVisible() ) );

    mgr->SetConditions( PCB_ACTIONS::showPadNumbers,   CHECK( cond.PadNumbersDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::padDisplayMode,   CHECK( !cond.PadFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::textOutlines,     CHECK( !cond.TextFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::graphicsOutlines, CHECK( !cond.GraphicsFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::fpAutoZoom,       CHECK( cond.FootprintViewerAutoZoom() ) );
#undef CHECK
}


void DISPLAY_FOOTPRINTS_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    // We don't allow people to change this right now, so make sure it's on
    GetWindowSettings( aCfg )->cursor.always_show_cursor = true;

    if( CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( aCfg ) )
    {
        PCB_BASE_FRAME::LoadSettings( cfg );
        SetDisplayOptions( cfg->m_FootprintViewerDisplayOptions );
    }
}


void DISPLAY_FOOTPRINTS_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    PCB_BASE_FRAME::SaveSettings( aCfg );

    if( CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( aCfg ) )
    {
        cfg->m_FootprintViewerDisplayOptions = GetDisplayOptions();
        cfg->m_FootprintViewerZoom = GetCanvas()->GetView()->GetScale();
    }
}


WINDOW_SETTINGS* DISPLAY_FOOTPRINTS_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    if( CVPCB_SETTINGS* cfg = GetAppSettings<CVPCB_SETTINGS>( "cvpcb" ) )
        return &cfg->m_FootprintViewer;

    wxFAIL_MSG( wxT( "DISPLAY_FOOTPRINTS_FRAME not running with CVPCB_SETTINGS" ) );
    return &aCfg->m_Window;     // non-null fail-safe
}


PCB_VIEWERS_SETTINGS_BASE* DISPLAY_FOOTPRINTS_FRAME::GetViewerSettingsBase() const
{
    return GetAppSettings<CVPCB_SETTINGS>( "cvpcb" );
}


MAGNETIC_SETTINGS* DISPLAY_FOOTPRINTS_FRAME::GetMagneticItemsSettings()
{
    static MAGNETIC_SETTINGS fallback;

    if( CVPCB_SETTINGS* cfg = GetAppSettings<CVPCB_SETTINGS>( "cvpcb" ) )
        return &cfg->m_FootprintViewerMagneticSettings;

    wxFAIL_MSG( wxT( "DISPLAY_FOOTPRINTS_FRAME not running with CVPCB_SETTINGS" ) );
    return &fallback;
}


COLOR4D DISPLAY_FOOTPRINTS_FRAME::GetGridColor()
{
    return COLOR4D( DARKGRAY );
}


FOOTPRINT* DISPLAY_FOOTPRINTS_FRAME::GetFootprint( const wxString& aFootprintName, REPORTER& aReporter )
{
    FOOTPRINT* footprint = nullptr;
    LIB_ID     fpid;

    if( fpid.Parse( aFootprintName ) >= 0 )
    {
        aReporter.Report( wxString::Format( _( "Footprint ID '%s' is not valid." ), aFootprintName ),
                          RPT_SEVERITY_ERROR );
        return nullptr;
    }

    wxString libNickname = From_UTF8( fpid.GetLibNickname().c_str() );
    wxString fpName      = From_UTF8( fpid.GetLibItemName().c_str() );

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );

    // See if the library requested is in the library table
    if( !adapter->HasLibrary( libNickname ) )
    {
        aReporter.Report( wxString::Format( _( "Library '%s' is not in the footprint library table." ),
                                            libNickname ),
                          RPT_SEVERITY_ERROR );
        return nullptr;
    }

    // See if the footprint requested is in the library
    if( !adapter->FootprintExists( libNickname, fpName ) )
    {
        aReporter.Report( wxString::Format( _( "Footprint '%s' not found." ), aFootprintName ),
                          RPT_SEVERITY_ERROR );
        return nullptr;
    }

    try
    {
        footprint = adapter->LoadFootprint( libNickname, fpName, false );
    }
    catch( const IO_ERROR& ioe )
    {
        aReporter.Report( wxString::Format( _( "Error loading footprint: %s" ), ioe.What() ),
                          RPT_SEVERITY_ERROR );
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

    m_toolManager->RunAction( PCB_ACTIONS::rehatchShapes );

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

    CVPCB_SETTINGS* cfg = dynamic_cast<CVPCB_SETTINGS*>( config() );
    wxCHECK( cfg, /* void */ );

    if( cfg->m_FootprintViewerAutoZoomOnSelect )
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
    FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
    return ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );
}


BOARD_ITEM_CONTAINER* DISPLAY_FOOTPRINTS_FRAME::GetModel() const
{
    return GetBoard()->GetFirstFootprint();
}


SELECTION& DISPLAY_FOOTPRINTS_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL>()->GetSelection();
}
