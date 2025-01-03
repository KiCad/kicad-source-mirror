/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2023 CERN
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

#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/string.h>
#include <wx/wupdlock.h>
#include <wx/clipbrd.h>
#include <wx/filedlg.h>
#include <wx/dialog.h>
#include "eda_3d_viewer_frame.h"
#include "dialogs/appearance_controls_3D.h"
#include <dialogs/eda_view_switcher.h>
#include <eda_3d_viewer_settings.h>
#include <3d_rendering/raytracing/render_3d_raytrace_ram.h>
#include <3d_viewer_id.h>
#include <3d_viewer/tools/eda_3d_actions.h>
#include <3d_viewer/tools/eda_3d_controller.h>
#include <3d_viewer/tools/eda_3d_conditions.h>
#include <board.h>
#include <advanced_config.h>
#include <bitmaps.h>
#include <board_design_settings.h>
#include <core/arraydim.h>
#include <dpi_scaling_common.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <tool/action_manager.h>
#include <tool/common_control.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <widgets/wx_infobar.h>
#include <widgets/wx_aui_utils.h>
#include <wildcards_and_files_ext.h>
#include <project_pcb.h>
#include <toolbars_3d.h>

#ifdef __linux__
#include <spacenav/libspnav_driver.h>
#include <3d_spacenav/spnav_viewer_plugin.h>
#else
#include <3d_navlib/nl_3d_viewer_plugin.h>
#endif

/**
 * Flag to enable 3D viewer main frame window debug tracing.
 *
 * Use "KI_TRACE_EDA_3D_VIEWER" to enable.
 *
 * @ingroup trace_env_vars
 */
const wxChar* EDA_3D_VIEWER_FRAME::m_logTrace = wxT( "KI_TRACE_EDA_3D_VIEWER" );


BEGIN_EVENT_TABLE( EDA_3D_VIEWER_FRAME, KIWAY_PLAYER )

    EVT_ACTIVATE( EDA_3D_VIEWER_FRAME::OnActivate )
    EVT_SET_FOCUS( EDA_3D_VIEWER_FRAME::OnSetFocus )

    EVT_TOOL_RANGE( ID_START_COMMAND_3D, ID_MENU_COMMAND_END,
                    EDA_3D_VIEWER_FRAME::Process_Special_Functions )

    EVT_MENU( wxID_CLOSE, EDA_3D_VIEWER_FRAME::Exit3DFrame )
    EVT_MENU( ID_DISABLE_RAY_TRACING, EDA_3D_VIEWER_FRAME::onDisableRayTracing )

    EVT_CLOSE( EDA_3D_VIEWER_FRAME::OnCloseWindow )
END_EVENT_TABLE()


EDA_3D_VIEWER_FRAME::EDA_3D_VIEWER_FRAME( KIWAY* aKiway, PCB_BASE_FRAME* aParent,
                                          const wxString& aTitle, long style ) :
        KIWAY_PLAYER( aKiway, aParent, FRAME_PCB_DISPLAY3D, aTitle, wxDefaultPosition,
                      wxDefaultSize, style, QUALIFIED_VIEWER3D_FRAMENAME( aParent ), unityScale ),
        m_canvas( nullptr ),
        m_currentCamera( m_trackBallCamera ),
        m_trackBallCamera( 2 * RANGE_SCALE_3D )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::EDA_3D_VIEWER_FRAME %s" ), aTitle );

    m_disable_ray_tracing = false;
    m_aboutTitle = _HKI( "KiCad 3D Viewer" );

    // Give it an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_3d ) );
    SetIcon( icon );

    // Create the status line
    static const int status_dims[5] = { -1, 170, 130, 130, 130 };

    wxStatusBar *status_bar = CreateStatusBar( arrayDim( status_dims ) );
    SetStatusWidths( arrayDim( status_dims ), status_dims );

    ANTIALIASING_MODE       aaMode = ANTIALIASING_MODE::AA_NONE;
    EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );

    if( cfg )
        aaMode = static_cast<ANTIALIASING_MODE>( cfg->m_Render.opengl_AA_mode );

    m_canvas = new EDA_3D_CANVAS( this, OGL_ATT_LIST::GetAttributesList( aaMode, true ), m_boardAdapter,
                                  m_currentCamera, PROJECT_PCB::Get3DCacheManager( &Prj() ) );

    m_appearancePanel = new APPEARANCE_CONTROLS_3D( this, GetCanvas() );

    LoadSettings( GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) );
    loadCommonSettings();

    m_appearancePanel->SetUserViewports( Prj().GetProjectFile().m_Viewports3D );

    // Create the manager
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), nullptr, nullptr,
                                   GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ), this );

    m_actions = new EDA_3D_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    m_canvas->SetEventDispatcher( m_toolDispatcher );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new EDA_3D_CONTROLLER );
    m_toolManager->InitTools();

    setupUIConditions();

    if( EDA_3D_CONTROLLER* ctrlTool = GetToolManager()->GetTool<EDA_3D_CONTROLLER>() )
        ctrlTool->SetRotationIncrement( cfg ? cfg->m_Camera.rotation_increment : 10.0 );

    // Run the viewer control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "3DViewer.Control" );

    ReCreateMenuBar();

    m_toolbarSettings = Pgm().GetSettingsManager().GetToolbarSettings<EDA_3D_VIEWER_TOOLBAR_SETTINGS>( "3d_viewer-toolbars" );
    configureToolbars();
    RecreateToolbars();

    m_infoBar = new WX_INFOBAR( this, &m_auimgr );

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( wxS( "TopMainToolbar" ) )
                      .Top().Layer( 6 ) );
    m_auimgr.AddPane( m_infoBar, EDA_PANE().InfoBar().Name( wxS( "InfoBar" ) )
                      .Top().Layer( 1 ) );
    m_auimgr.AddPane( m_appearancePanel, EDA_PANE().Name( "LayersManager" )
                      .Right().Layer( 3 )
                      .Caption( _( "Appearance" ) ).PaneBorder( false )
                      .MinSize( 180, -1 ).BestSize( 190, -1 ) );
    m_auimgr.AddPane( m_canvas, EDA_PANE().Canvas().Name( wxS( "DrawFrame" ) )
                      .Center() );

    wxAuiPaneInfo& layersManager = m_auimgr.GetPane( "LayersManager" );

    if( cfg && cfg->m_AuiPanels.right_panel_width > 0 )
        SetAuiPaneSize( m_auimgr, layersManager, cfg->m_AuiPanels.right_panel_width, -1 );

    if( cfg )
        layersManager.Show( cfg->m_AuiPanels.show_layer_manager );

    // Call Update() to fix all pane default sizes, especially the "InfoBar" pane before
    // hiding it.
    m_auimgr.Update();

    // We don't want the infobar displayed right away
    m_auimgr.GetPane( wxS( "InfoBar" ) ).Hide();
    m_auimgr.Update();

    m_canvas->SetInfoBar( m_infoBar );
    m_canvas->SetStatusBar( status_bar );

    try
    {
#ifdef __linux__
        m_spaceMouse = std::make_unique<SPNAV_VIEWER_PLUGIN>( m_canvas );
#else
        m_spaceMouse = std::make_unique<NL_3D_VIEWER_PLUGIN>( m_canvas );
#endif
    }
    catch( const std::system_error& e )
    {
        wxLogTrace( wxT( "KI_TRACE_NAVLIB" ), e.what() );
    }

    // Fixes bug in Windows (XP and possibly others) where the canvas requires the focus
    // in order to receive mouse events.  Otherwise, the user has to click somewhere on
    // the canvas before it will respond to mouse wheel events.
    m_canvas->SetFocus();
}


EDA_3D_VIEWER_FRAME::~EDA_3D_VIEWER_FRAME()
{
    Prj().GetProjectFile().m_Viewports3D = m_appearancePanel->GetUserViewports();

    m_canvas->SetEventDispatcher( nullptr );

    m_auimgr.UnInit();
}


void EDA_3D_VIEWER_FRAME::setupUIConditions()
{
    EDA_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    EDA_3D_CONDITIONS cond( &m_boardAdapter );

// Helper to define check conditions
#define GridSizeCheck( x ) ACTION_CONDITIONS().Check( cond.GridSize( x ) )

    auto raytracing =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.engine != RENDER_ENGINE::OPENGL;
            };
    auto showTH =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_footprints_normal;
            };
    auto showSMD =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_footprints_insert;
            };
    auto showVirtual =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_footprints_virtual;
            };
    auto show_NotInPosfile =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_footprints_not_in_posfile;
            };
    auto show_DNP =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_footprints_dnp;
            };
    auto showBBoxes =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_model_bbox;
            };
    auto showNavig = [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_navigator;
            };
    auto ortho =
            [this]( const SELECTION& )
            {
                return m_currentCamera.GetProjection() == PROJECTION_TYPE::ORTHO;
            };

    auto appearances =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_AuiPanels.show_layer_manager;
            };

    mgr->SetConditions( EDA_3D_ACTIONS::toggleRaytacing, ACTION_CONDITIONS().Check( raytracing ) );

    mgr->SetConditions( EDA_3D_ACTIONS::showTHT, ACTION_CONDITIONS().Check( showTH ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showSMD, ACTION_CONDITIONS().Check( showSMD ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showVirtual, ACTION_CONDITIONS().Check( showVirtual ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showNotInPosFile,
                        ACTION_CONDITIONS().Check( show_NotInPosfile ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showDNP, ACTION_CONDITIONS().Check( show_DNP ) );

    mgr->SetConditions( EDA_3D_ACTIONS::showBBoxes, ACTION_CONDITIONS().Check( showBBoxes ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showNavigator, ACTION_CONDITIONS().Check( showNavig ) );

    mgr->SetConditions( EDA_3D_ACTIONS::noGrid, GridSizeCheck( GRID3D_TYPE::NONE ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show10mmGrid, GridSizeCheck( GRID3D_TYPE::GRID_10MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show5mmGrid, GridSizeCheck( GRID3D_TYPE::GRID_5MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show2_5mmGrid, GridSizeCheck( GRID3D_TYPE::GRID_2P5MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show1mmGrid, GridSizeCheck( GRID3D_TYPE::GRID_1MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::toggleOrtho, ACTION_CONDITIONS().Check( ortho ) );

    mgr->SetConditions( EDA_3D_ACTIONS::showLayersManager,
                        ACTION_CONDITIONS().Check( appearances ) );

#undef GridSizeCheck
}


bool EDA_3D_VIEWER_FRAME::TryBefore( wxEvent& aEvent )
{
    static bool s_presetSwitcherShown = false;
    static bool s_viewportSwitcherShown = false;

    // wxWidgets generates no key events for the tab key when the ctrl key is held down.  One
    // way around this is to look at all events and inspect the keyboard state of the tab key.
    // However, this runs into issues on some linux VMs where querying the keyboard state is
    // very slow.  Fortunately we only use ctrl-tab on Mac, so we implement this lovely hack:
#ifdef __WXMAC__
    if( wxGetKeyState( WXK_TAB ) )
#else
    if( ( aEvent.GetEventType() == wxEVT_CHAR || aEvent.GetEventType() == wxEVT_CHAR_HOOK )
            && static_cast<wxKeyEvent&>( aEvent ).GetKeyCode() == WXK_TAB )
#endif
    {
        if( !s_presetSwitcherShown && wxGetKeyState( PRESET_SWITCH_KEY ) )
        {
            if( m_appearancePanel && this->IsActive() )
            {
                wxArrayString mru = m_appearancePanel->GetLayerPresetsMRU();

                if( mru.size() > 0 )
                {
                    for( wxString& str : mru )
                    {
                        if( str == FOLLOW_PCB )
                            str = _( "Follow PCB Editor" );
                        else if( str == FOLLOW_PLOT_SETTINGS )
                            str = _( "Follow PCB Plot Settings" );
                    }

                    EDA_VIEW_SWITCHER switcher( this, mru, PRESET_SWITCH_KEY );

                    s_presetSwitcherShown = true;
                    switcher.ShowModal();
                    s_presetSwitcherShown = false;

                    int idx = switcher.GetSelection();

                    if( idx >= 0 && idx < (int) mru.size() )
                    {
                        wxString internalName = m_appearancePanel->GetLayerPresetsMRU()[idx];
                        m_appearancePanel->ApplyLayerPreset( internalName );
                    }

                    return true;
                }
            }
        }
        else if( !s_viewportSwitcherShown && wxGetKeyState( VIEWPORT_SWITCH_KEY ) )
        {
            if( this->IsActive() )
            {
                const wxArrayString& viewportMRU = m_appearancePanel->GetViewportsMRU();

                if( viewportMRU.size() > 0 )
                {
                    EDA_VIEW_SWITCHER switcher( this, viewportMRU, VIEWPORT_SWITCH_KEY );

                    s_viewportSwitcherShown = true;
                    switcher.ShowModal();
                    s_viewportSwitcherShown = false;

                    int idx = switcher.GetSelection();

                    if( idx >= 0 && idx < (int) viewportMRU.size() )
                        m_appearancePanel->ApplyViewport( viewportMRU[idx] );

                    return true;
                }
            }
        }
    }

    return wxFrame::TryBefore( aEvent );
}


void EDA_3D_VIEWER_FRAME::handleIconizeEvent( wxIconizeEvent& aEvent )
{
    KIWAY_PLAYER::handleIconizeEvent( aEvent );

    if( m_spaceMouse && aEvent.IsIconized() )
        m_spaceMouse->SetFocus( false );
}


void EDA_3D_VIEWER_FRAME::ReloadRequest()
{
    // This will schedule a request to load later
    // ReloadRequest also updates the board pointer so always call it first
    if( m_canvas )
        m_canvas->ReloadRequest( GetBoard(), PROJECT_PCB::Get3DCacheManager( &Prj() ) );

    if( m_appearancePanel )
        m_appearancePanel->UpdateLayerCtls();
}


void EDA_3D_VIEWER_FRAME::NewDisplay( bool aForceImmediateRedraw )
{
    if( m_canvas )
        m_canvas->ReloadRequest( GetBoard(), PROJECT_PCB::Get3DCacheManager( &Prj() ) );

    // After the ReloadRequest call, the refresh often takes a bit of time,
    // and it is made here only on request.
    if( m_canvas && aForceImmediateRedraw )
        m_canvas->Refresh();
}


void EDA_3D_VIEWER_FRAME::Redraw()
{
    // Only update in OpenGL for an interactive interaction
    if( m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
        m_canvas->Request_refresh( true );
}


void EDA_3D_VIEWER_FRAME::refreshRender()
{
    if( m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
        m_canvas->Request_refresh();
    else
        NewDisplay( true );
}


void EDA_3D_VIEWER_FRAME::Exit3DFrame( wxCommandEvent &event )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::Exit3DFrame" ) );

    Close( true );
}


void EDA_3D_VIEWER_FRAME::OnCloseWindow( wxCloseEvent &event )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::OnCloseWindow" ) );

    // Do not show the layer manager during closing to avoid flicker on some platforms (Windows)
    // that generate useless redraw of items in the Layer Manager
    if( m_auimgr.GetPane( wxS( "LayersManager" ) ).IsShown() )
        m_auimgr.GetPane( wxS( "LayersManager" ) ).Show( false );

    if( m_canvas )
        m_canvas->Close();

    Destroy();
    event.Skip( true );
}


void EDA_3D_VIEWER_FRAME::Process_Special_Functions( wxCommandEvent &event )
{
    if( m_canvas == nullptr )
        return;

    switch( event.GetId() )
    {
    case ID_MENU3D_RESET_DEFAULTS:
    {
        m_boardAdapter.SetLayerColors( m_boardAdapter.GetDefaultColors() );

        if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
            cfg->ResetToDefaults();

        LoadSettings( GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) );

        // Tell canvas that we (may have) changed the render engine
        RenderEngineChanged();
        NewDisplay( true );
        return;
    }

    default:
        wxFAIL_MSG( wxT( "Invalid event in EDA_3D_VIEWER_FRAME::Process_Special_Functions()" ) );
        return;
    }
}


void EDA_3D_VIEWER_FRAME::onDisableRayTracing( wxCommandEvent& aEvent )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::%s disabling ray tracing." ), __WXFUNCTION__ );

    m_disable_ray_tracing = true;
    m_boardAdapter.m_Cfg->m_Render.engine = RENDER_ENGINE::OPENGL;
}


void EDA_3D_VIEWER_FRAME::OnActivate( wxActivateEvent &aEvent )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::OnActivate" ) );

    if( aEvent.GetActive() && m_canvas )
    {
        // Reload data if 3D frame shows a board,
        // because it can be changed since last frame activation
        if( m_canvas->IsReloadRequestPending() )
            m_canvas->Request_refresh();

        // Activates again the focus of the canvas so it will catch mouse and key events
        m_canvas->SetFocus();
    }

    if( m_spaceMouse )
        m_spaceMouse->SetFocus( aEvent.GetActive() );

    aEvent.Skip(); // required under wxMAC
}


void EDA_3D_VIEWER_FRAME::OnSetFocus( wxFocusEvent& aEvent )
{
    // Activates again the focus of the canvas so it will catch mouse and key events
    if( m_canvas )
        m_canvas->SetFocus();

    aEvent.Skip();
}


void EDA_3D_VIEWER_FRAME::LoadSettings( APP_SETTINGS_BASE *aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    // Dynamic_cast here will fail on Mac when called from CvPCB.
    EDA_3D_VIEWER_SETTINGS* cfg = static_cast<EDA_3D_VIEWER_SETTINGS*>( aCfg );

    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::LoadSettings" ) );

    if( cfg )
    {
        applySettings( cfg );

        m_boardAdapter.SetBoard( GetBoard() );

        // When opening the 3D viewer, we use the OpenGL mode, never the ray tracing engine
        // because the ray tracing is very time consuming, and can be seen as not working
        // (freeze window) with large boards.
        m_boardAdapter.m_Cfg->m_Render.engine = RENDER_ENGINE::OPENGL;

        if( cfg->m_CurrentPreset == LEGACY_PRESET_FLAG )
        {
            wxString legacyColorsPresetName = _( "legacy colors" );

            cfg->m_UseStackupColors = false;

            if( !cfg->FindPreset( legacyColorsPresetName ) )
            {
                cfg->m_LayerPresets.emplace_back( legacyColorsPresetName,
                                                  GetAdapter().GetDefaultVisibleLayers(),
                                                  GetAdapter().GetDefaultColors() );
            }

            cfg->m_CurrentPreset = FOLLOW_PLOT_SETTINGS;
        }

        m_boardAdapter.InitSettings( nullptr, nullptr );

        if( m_appearancePanel )
            m_appearancePanel->CommonSettingsChanged();
    }
}


void EDA_3D_VIEWER_FRAME::SaveSettings( APP_SETTINGS_BASE *aCfg )
{
    EDA_BASE_FRAME::SaveSettings( GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) );

    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        cfg->m_AuiPanels.right_panel_width    = m_appearancePanel->GetSize().x;

        cfg->m_Camera.animation_enabled       = m_canvas->GetAnimationEnabled();
        cfg->m_Camera.moving_speed_multiplier = m_canvas->GetMovingSpeedMultiplier();
        cfg->m_Camera.projection_mode         = m_canvas->GetProjectionMode();

        if( EDA_3D_CONTROLLER* ctrlTool = GetToolManager()->GetTool<EDA_3D_CONTROLLER>() )
            cfg->m_Camera.rotation_increment = ctrlTool->GetRotationIncrement();
    }
}


void EDA_3D_VIEWER_FRAME::CommonSettingsChanged( int aFlags )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::CommonSettingsChanged" ) );

    // Regen menu bars, etc
    EDA_BASE_FRAME::CommonSettingsChanged( aFlags );

    loadCommonSettings();
    applySettings( GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) );

    m_appearancePanel->CommonSettingsChanged();

    NewDisplay( true );
}


void EDA_3D_VIEWER_FRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();

    SetTitle( _( "3D Viewer" ) );
    RecreateToolbars();

    if( m_appearancePanel )
    {
        wxAuiPaneInfo& lm_pane_info = m_auimgr.GetPane( m_appearancePanel );
        lm_pane_info.Caption( _( "Appearance" ) );
    }

    SetStatusText( wxEmptyString, ACTIVITY );
    SetStatusText( wxEmptyString, HOVERED_ITEM );
}


void EDA_3D_VIEWER_FRAME::ToggleAppearanceManager()
{
    wxAuiPaneInfo& layersManager = m_auimgr.GetPane( "LayersManager" );

    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        // show auxiliary Vertical layers and visibility manager toolbar
        cfg->m_AuiPanels.show_layer_manager = !cfg->m_AuiPanels.show_layer_manager;

        layersManager.Show( cfg->m_AuiPanels.show_layer_manager );

        if( cfg->m_AuiPanels.show_layer_manager )
        {
            SetAuiPaneSize( m_auimgr, layersManager, cfg->m_AuiPanels.right_panel_width, -1 );
        }
        else
        {
            cfg->m_AuiPanels.right_panel_width = m_appearancePanel->GetSize().x;
            m_auimgr.Update();
        }
    }
}


void EDA_3D_VIEWER_FRAME::OnDarkModeToggle()
{
    if( m_appearancePanel )
        m_appearancePanel->OnDarkModeToggle();
}


void EDA_3D_VIEWER_FRAME::TakeScreenshot( EDA_3D_VIEWER_EXPORT_FORMAT aFormat )
{
    wxString fullFileName;

    if( aFormat != EDA_3D_VIEWER_EXPORT_FORMAT::CLIPBOARD )
    {
        if( !getExportFileName( aFormat, fullFileName ) )
            return;
    }

    wxImage screenshotImage = captureCurrentViewScreenshot();

    if( screenshotImage.IsOk() )
    {
        saveOrCopyImage( screenshotImage, aFormat, fullFileName );
    }
}


wxImage EDA_3D_VIEWER_FRAME::captureCurrentViewScreenshot()
{
    // Ensure we have the latest 3D view (remember 3D view is buffered)
    // Also ensure any highlighted item is not highlighted when creating screen shot
    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS& cfg = m_boardAdapter.m_Cfg->m_Render;
    bool original_highlight = cfg.highlight_on_rollover;
    cfg.highlight_on_rollover = false;

    m_canvas->DoRePaint();      // init first buffer
    m_canvas->DoRePaint();      // init second buffer

    wxImage screenshotImage;

    if( m_canvas )
    {
        // Build image from the 3D buffer
        wxWindowUpdateLocker noUpdates( this );
        m_canvas->GetScreenshot( screenshotImage );
    }

    // Restore highlight setting
    cfg.highlight_on_rollover = original_highlight;

    return screenshotImage;
}


void EDA_3D_VIEWER_FRAME::ExportImage( EDA_3D_VIEWER_EXPORT_FORMAT aFormat, const wxSize& aSize )
{
    wxString fullFileName;

    if( aFormat != EDA_3D_VIEWER_EXPORT_FORMAT::CLIPBOARD )
    {
        if( !getExportFileName( aFormat, fullFileName ) )
            return;
    }

    wxImage screenshotImage = captureScreenshot( aSize );

    if( screenshotImage.IsOk() )
    {
        saveOrCopyImage( screenshotImage, aFormat, fullFileName );
    }
}


bool EDA_3D_VIEWER_FRAME::getExportFileName( EDA_3D_VIEWER_EXPORT_FORMAT& aFormat, wxString& fullFileName )
{
    // Create combined wildcard for both formats
    const wxString wildcard = FILEEXT::JpegFileWildcard() + "|" + FILEEXT::PngFileWildcard();

    if( !m_defaultSaveScreenshotFileName.IsOk() )
        m_defaultSaveScreenshotFileName = Parent()->Prj().GetProjectFullName();

    // Set default extension based on current format
    const wxString defaultExt = ( aFormat == EDA_3D_VIEWER_EXPORT_FORMAT::JPEG ) ?
                                FILEEXT::JpegFileExtension : FILEEXT::PngFileExtension;
    m_defaultSaveScreenshotFileName.SetExt( defaultExt );

    wxFileDialog dlg( this, _( "3D Image File Name" ),
                      m_defaultSaveScreenshotFileName.GetPath(),
                      m_defaultSaveScreenshotFileName.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    // Set initial filter index based on current format
    dlg.SetFilterIndex( ( aFormat == EDA_3D_VIEWER_EXPORT_FORMAT::JPEG ) ? 0 : 1 );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    m_defaultSaveScreenshotFileName = dlg.GetPath();

    // Determine format based on file extension first
    wxString fileExt = m_defaultSaveScreenshotFileName.GetExt().Lower();
    EDA_3D_VIEWER_EXPORT_FORMAT detectedFormat;
    bool formatDetected = false;

    if( fileExt == wxT("jpg") || fileExt == wxT("jpeg") )
    {
        detectedFormat = EDA_3D_VIEWER_EXPORT_FORMAT::JPEG;
        formatDetected = true;
    }
    else if( fileExt == wxT("png") )
    {
        detectedFormat = EDA_3D_VIEWER_EXPORT_FORMAT::PNG;
        formatDetected = true;
    }

    // If format can't be determined from extension, use dropdown selection
    if( !formatDetected )
    {
        int filterIndex = dlg.GetFilterIndex();
        detectedFormat = ( filterIndex == 0 ) ? EDA_3D_VIEWER_EXPORT_FORMAT::JPEG :
                                               EDA_3D_VIEWER_EXPORT_FORMAT::PNG;

        // Append appropriate extension
        const wxString ext = ( detectedFormat == EDA_3D_VIEWER_EXPORT_FORMAT::JPEG ) ?
                            FILEEXT::JpegFileExtension : FILEEXT::PngFileExtension;
        m_defaultSaveScreenshotFileName.SetExt( ext );
    }

    // Update the format parameter
    aFormat = detectedFormat;

    // Check directory permissions using the updated filename
    wxFileName fn = m_defaultSaveScreenshotFileName;

    if( !fn.IsDirWritable() )
    {
        wxString msg;
        msg.Printf( _( "Insufficient permissions to save file '%s'." ),
                   m_defaultSaveScreenshotFileName.GetFullPath() );
        wxMessageBox( msg, _( "Error" ), wxOK | wxICON_ERROR, this );
        return false;
    }

    fullFileName = m_defaultSaveScreenshotFileName.GetFullPath();
    return true;
}


wxImage EDA_3D_VIEWER_FRAME::captureScreenshot( const wxSize& aSize )
{
    TRACK_BALL    camera  = m_trackBallCamera;
    camera.SetCurWindowSize( aSize );

    EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );
    EDA_3D_VIEWER_SETTINGS* backupCfg = m_boardAdapter.m_Cfg;

    auto configRestorer = std::unique_ptr<void, std::function<void(void*)>>(
        reinterpret_cast<void*>(1),
        [&](void*) { m_boardAdapter.m_Cfg = backupCfg; }
    );

    if( cfg )
        m_boardAdapter.m_Cfg = cfg;

    if( cfg && cfg->m_Render.engine == RENDER_ENGINE::RAYTRACING )
        return captureRaytracingScreenshot( m_boardAdapter, camera, aSize );
    else
        return captureOpenGLScreenshot( m_boardAdapter, camera, aSize );
}


void EDA_3D_VIEWER_FRAME::setupRenderingConfig( BOARD_ADAPTER& aAdapter )
{
    EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );

    if( cfg )
        aAdapter.m_Cfg = cfg;
}


wxImage EDA_3D_VIEWER_FRAME::captureRaytracingScreenshot( BOARD_ADAPTER& aAdapter, TRACK_BALL& aCamera, const wxSize& aSize )
{
    BOARD_ADAPTER tempadapter;
    tempadapter.SetBoard( GetBoard() );
    tempadapter.m_Cfg = aAdapter.m_Cfg;
    tempadapter.InitSettings( nullptr, nullptr );
    tempadapter.Set3dCacheManager( aAdapter.Get3dCacheManager() );

    RENDER_3D_RAYTRACE_RAM raytrace( tempadapter, aCamera );
    raytrace.SetCurWindowSize( aSize );

    while( raytrace.Redraw( false, nullptr, nullptr ) );

    uint8_t* rgbaBuffer = raytrace.GetBuffer();
    wxSize   realSize   = raytrace.GetRealBufferSize();

    if( !rgbaBuffer )
        return wxImage();

    return convertRGBAToImage( rgbaBuffer, realSize );
}


wxImage EDA_3D_VIEWER_FRAME::convertRGBAToImage( uint8_t* aRGBABuffer, const wxSize& aRealSize )
{
    const unsigned int wxh = aRealSize.x * aRealSize.y;

    unsigned char* rgbBuffer   = (unsigned char*) malloc( wxh * 3 );
    unsigned char* alphaBuffer = (unsigned char*) malloc( wxh );

    unsigned char* rgbaPtr  = aRGBABuffer;
    unsigned char* rgbPtr   = rgbBuffer;
    unsigned char* alphaPtr = alphaBuffer;

    for( int y = 0; y < aRealSize.y; y++ )
    {
        for( int x = 0; x < aRealSize.x; x++ )
        {
            rgbPtr[0]   = rgbaPtr[0];
            rgbPtr[1]   = rgbaPtr[1];
            rgbPtr[2]   = rgbaPtr[2];
            alphaPtr[0] = rgbaPtr[3];

            rgbaPtr  += 4;
            rgbPtr   += 3;
            alphaPtr += 1;
        }
    }

    wxImage screenshotImage;
    screenshotImage.Create( aRealSize );
    screenshotImage.SetData( rgbBuffer );
    screenshotImage.SetAlpha( alphaBuffer );
    return screenshotImage.Mirror( false );
}


wxImage EDA_3D_VIEWER_FRAME::captureOpenGLScreenshot( BOARD_ADAPTER& aAdapter, TRACK_BALL& aCamera, const wxSize& aSize )
{
    EDA_3D_VIEWER_SETTINGS* cfg = aAdapter.m_Cfg;
    ANTIALIASING_MODE aaMode = cfg ? static_cast<ANTIALIASING_MODE>( cfg->m_Render.opengl_AA_mode )
                                   : ANTIALIASING_MODE::AA_NONE;

    wxFrame temp( this, wxID_ANY, wxEmptyString, wxDefaultPosition, aSize, wxFRAME_NO_TASKBAR );
    temp.Hide();
    BOARD_ADAPTER tempadapter;
    tempadapter.SetBoard( GetBoard() );
    tempadapter.m_Cfg = aAdapter.m_Cfg;
    tempadapter.InitSettings( nullptr, nullptr );
    tempadapter.Set3dCacheManager( aAdapter.Get3dCacheManager() );

    auto canvas = std::make_unique<EDA_3D_CANVAS>( &temp,
                                               OGL_ATT_LIST::GetAttributesList( aaMode, true ),
                                               tempadapter, aCamera,
                                               aAdapter.Get3dCacheManager() );

    canvas->SetSize( aSize );
    configureCanvas( canvas, cfg );
    wxWindowUpdateLocker noUpdates( this );

    // Temporarily disable highlight during screenshot
    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS& renderCfg = aAdapter.m_Cfg->m_Render;
    bool original_highlight = renderCfg.highlight_on_rollover;
    bool original_navigator = renderCfg.show_navigator;
    renderCfg.show_navigator = false;
    renderCfg.highlight_on_rollover = false;

    std::vector<unsigned char> buffer(aSize.x * aSize.y * 4); // RGBA format
    canvas->RenderToFrameBuffer( buffer.data(), aSize.x, aSize.y );
    wxImage result = convertRGBAToImage( buffer.data(), aSize );

    // Restore highlight setting
    renderCfg.highlight_on_rollover = original_highlight;
    renderCfg.show_navigator = original_navigator;

    return result;
}


void EDA_3D_VIEWER_FRAME::configureCanvas( std::unique_ptr<EDA_3D_CANVAS>& aCanvas, EDA_3D_VIEWER_SETTINGS* aCfg )
{
    if( aCfg )
    {
        aCanvas->SetAnimationEnabled( aCfg->m_Camera.animation_enabled );
        aCanvas->SetMovingSpeedMultiplier( aCfg->m_Camera.moving_speed_multiplier );
        aCanvas->SetProjectionMode( aCfg->m_Camera.projection_mode );
    }

    aCanvas->SetVcSettings( EDA_DRAW_PANEL_GAL::GetVcSettings() );
}


void EDA_3D_VIEWER_FRAME::saveOrCopyImage( const wxImage& aScreenshotImage, EDA_3D_VIEWER_EXPORT_FORMAT aFormat, const wxString& aFullFileName )
{
    if( aFormat == EDA_3D_VIEWER_EXPORT_FORMAT::CLIPBOARD )
    {
        copyImageToClipboard( aScreenshotImage );
    }
    else
    {
        saveImageToFile( aScreenshotImage, aFormat, aFullFileName );
    }
}


void EDA_3D_VIEWER_FRAME::copyImageToClipboard( const wxImage& aScreenshotImage )
{
    wxBitmap bitmap( aScreenshotImage );
    wxLogNull doNotLog;

    if( wxTheClipboard->Open() )
    {
        wxBitmapDataObject* dobjBmp = new wxBitmapDataObject( bitmap );

        if( !wxTheClipboard->SetData( dobjBmp ) )
            wxMessageBox( _( "Failed to copy image to clipboard" ) );

        wxTheClipboard->Flush();
        wxTheClipboard->Close();
    }
}


void EDA_3D_VIEWER_FRAME::saveImageToFile( const wxImage& aScreenshotImage, EDA_3D_VIEWER_EXPORT_FORMAT aFormat, const wxString& aFullFileName )
{
    bool fmt_is_jpeg = ( aFormat == EDA_3D_VIEWER_EXPORT_FORMAT::JPEG );

    if( !aScreenshotImage.SaveFile( aFullFileName, fmt_is_jpeg ? wxBITMAP_TYPE_JPEG : wxBITMAP_TYPE_PNG ) )
    {
        wxMessageBox( _( "Can't save file" ) );
    }
}


void EDA_3D_VIEWER_FRAME::RenderEngineChanged()
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::RenderEngineChanged()" ) );

    if( m_canvas )
        m_canvas->RenderEngineChanged();
}


void EDA_3D_VIEWER_FRAME::loadCommonSettings()
{
    wxCHECK_RET( m_canvas, wxT( "Cannot load settings to null canvas" ) );

    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    // TODO(JE) use all control options
    m_boardAdapter.m_MousewheelPanning = settings->m_Input.scroll_modifier_zoom != 0;
}


void EDA_3D_VIEWER_FRAME::applySettings( EDA_3D_VIEWER_SETTINGS* cfg )
{
    m_boardAdapter.m_Cfg = cfg;

    m_canvas->SetAnimationEnabled( cfg->m_Camera.animation_enabled );
    m_canvas->SetMovingSpeedMultiplier( cfg->m_Camera.moving_speed_multiplier );
    m_canvas->SetProjectionMode( cfg->m_Camera.projection_mode );

    m_canvas->SetVcSettings( EDA_DRAW_PANEL_GAL::GetVcSettings() );
}
