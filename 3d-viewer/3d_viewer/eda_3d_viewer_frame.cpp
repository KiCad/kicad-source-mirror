/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/choice.h>
#include <wx/dialog.h>
#include "eda_3d_viewer_frame.h"
#include "eda_list_dialog.h"
#include "wx/generic/textdlgg.h"
#include <dialogs/eda_view_switcher.h>
#include <eda_3d_viewer_settings.h>
#include <3d_viewer_id.h>
#include <3d_viewer/tools/eda_3d_actions.h>
#include <3d_viewer/tools/eda_3d_controller.h>
#include <3d_viewer/tools/eda_3d_conditions.h>
#include <advanced_config.h>
#include <bitmaps.h>
#include <board_design_settings.h>
#include <core/arraydim.h>
#include <gal/dpi_scaling.h>
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
#include <wildcards_and_files_ext.h>

#include <3d_navlib/nl_3d_viewer_plugin.h>

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
    EVT_MENU( ID_RENDER_CURRENT_VIEW, EDA_3D_VIEWER_FRAME::onRenderEngineSelection )
    EVT_MENU( ID_DISABLE_RAY_TRACING, EDA_3D_VIEWER_FRAME::onDisableRayTracing )

    EVT_CLOSE( EDA_3D_VIEWER_FRAME::OnCloseWindow )
END_EVENT_TABLE()


EDA_3D_VIEWER_FRAME::EDA_3D_VIEWER_FRAME( KIWAY* aKiway, PCB_BASE_FRAME* aParent,
                                          const wxString& aTitle, long style ) :
        KIWAY_PLAYER( aKiway, aParent, FRAME_PCB_DISPLAY3D, aTitle, wxDefaultPosition,
                      wxDefaultSize, style, QUALIFIED_VIEWER3D_FRAMENAME( aParent ), unityScale ),
        m_mainToolBar( nullptr ), m_canvas( nullptr ), m_currentCamera( m_trackBallCamera ),
        m_viewportsLabel( nullptr ),
        m_cbViewports( nullptr ),
        m_trackBallCamera( 2 * RANGE_SCALE_3D ), m_spaceMouse( nullptr )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::EDA_3D_VIEWER_FRAME %s" ), aTitle );

    m_disable_ray_tracing = false;
    m_aboutTitle = _HKI( "KiCad 3D Viewer" );

    // Give it an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_3d ) );
    SetIcon( icon );

    // Create the status line
    static const int status_dims[4] = { -1, 170, 130, 130 };

    wxStatusBar *status_bar = CreateStatusBar( arrayDim( status_dims ) );
    SetStatusWidths( arrayDim( status_dims ), status_dims );

    SETTINGS_MANAGER&       mgr = Pgm().GetSettingsManager();
    EDA_3D_VIEWER_SETTINGS* cfg = mgr.GetAppSettings<EDA_3D_VIEWER_SETTINGS>();
    ANTIALIASING_MODE       aaMode = static_cast<ANTIALIASING_MODE>( cfg->m_Render.opengl_AA_mode );

    m_canvas = new EDA_3D_CANVAS( this, OGL_ATT_LIST::GetAttributesList( aaMode ),
                                  m_boardAdapter, m_currentCamera, Prj().Get3DCacheManager() );

    LoadSettings( cfg );
    loadCommonSettings();

    SetUserViewports( Prj().GetProjectFile().m_Viewports3D );

    // Create the manager
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), nullptr, nullptr, cfg, this );

    m_actions = new EDA_3D_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    m_canvas->SetEventDispatcher( m_toolDispatcher );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new EDA_3D_CONTROLLER );
    m_toolManager->InitTools();

    setupUIConditions();

    if( EDA_3D_CONTROLLER* ctrlTool = GetToolManager()->GetTool<EDA_3D_CONTROLLER>() )
        ctrlTool->SetRotationIncrement( cfg->m_Camera.rotation_increment );

    // Run the viewer control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "3DViewer.Control" );

    CreateMenuBar();
    ReCreateMainToolbar();

    m_infoBar = new WX_INFOBAR( this, &m_auimgr );

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( wxS( "MainToolbar" ) ).Top().Layer( 6 ) );
    m_auimgr.AddPane( m_infoBar, EDA_PANE().InfoBar().Name( wxS( "InfoBar" ) ).Top().Layer( 1 ) );
    m_auimgr.AddPane( m_canvas, EDA_PANE().Canvas().Name( wxS( "DrawFrame" ) ).Center() );

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
        m_spaceMouse = new NL_3D_VIEWER_PLUGIN( m_canvas );
    }
    catch( const std::system_error& e )
    {
        wxLogTrace( wxT( "KI_TRACE_NAVLIB" ), e.what() );
    }

    // Fixes bug in Windows (XP and possibly others) where the canvas requires the focus
    // in order to receive mouse events.  Otherwise, the user has to click somewhere on
    // the canvas before it will respond to mouse wheel events.
    m_canvas->SetFocus();

    m_cbViewports->Connect( wxEVT_COMMAND_CHOICE_SELECTED,
                            wxCommandEventHandler( EDA_3D_VIEWER_FRAME::onViewportChanged ),
                            nullptr, this );
    m_cbViewports->Connect( wxEVT_UPDATE_UI,
                            wxUpdateUIEventHandler( EDA_3D_VIEWER_FRAME::onUpdateViewportsCb ),
                            nullptr, this );
}


EDA_3D_VIEWER_FRAME::~EDA_3D_VIEWER_FRAME()
{
    delete m_spaceMouse;

    Prj().GetProjectFile().m_Viewports3D = GetUserViewports();

    m_cbViewports->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED,
                               wxCommandEventHandler( EDA_3D_VIEWER_FRAME::onViewportChanged ),
                               nullptr, this );
    m_cbViewports->Disconnect( wxEVT_UPDATE_UI,
                               wxUpdateUIEventHandler( EDA_3D_VIEWER_FRAME::onUpdateViewportsCb ),
                               nullptr, this );

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
                return m_boardAdapter.m_Cfg->m_Render.opengl_show_model_bbox;
            };
    auto showAxes =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_axis;
            };
    auto ortho =
            [this]( const SELECTION& )
            {
                return m_currentCamera.GetProjection() == PROJECTION_TYPE::ORTHO;
            };

    RegisterUIUpdateHandler( ID_RENDER_CURRENT_VIEW,   ACTION_CONDITIONS().Check( raytracing ) );

    mgr->SetConditions( EDA_3D_ACTIONS::showTHT,       ACTION_CONDITIONS().Check( showTH ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showSMD,       ACTION_CONDITIONS().Check( showSMD ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showVirtual,   ACTION_CONDITIONS().Check( showVirtual ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showNotInPosFile,   ACTION_CONDITIONS().Check( show_NotInPosfile ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showDNP,       ACTION_CONDITIONS().Check( show_DNP ) );


    mgr->SetConditions( EDA_3D_ACTIONS::showBBoxes,    ACTION_CONDITIONS().Check( showBBoxes ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showAxis,      ACTION_CONDITIONS().Check( showAxes ) );

    mgr->SetConditions( EDA_3D_ACTIONS::noGrid,        GridSizeCheck( GRID3D_TYPE::NONE ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show10mmGrid,  GridSizeCheck( GRID3D_TYPE::GRID_10MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show5mmGrid,   GridSizeCheck( GRID3D_TYPE::GRID_5MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show2_5mmGrid, GridSizeCheck( GRID3D_TYPE::GRID_2P5MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show1mmGrid,   GridSizeCheck( GRID3D_TYPE::GRID_1MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::toggleOrtho,   ACTION_CONDITIONS().Check( ortho ) );

#undef GridSizeCheck
}


bool EDA_3D_VIEWER_FRAME::TryBefore( wxEvent& aEvent )
{
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
        if( !s_viewportSwitcherShown && wxGetKeyState( VIEWPORT_SWITCH_KEY ) )
        {
            if( this->IsActive() )
            {
                if( m_viewportMRU.size() > 0 )
                {
                    EDA_VIEW_SWITCHER switcher( this, m_viewportMRU, VIEWPORT_SWITCH_KEY );

                    s_viewportSwitcherShown = true;
                    switcher.ShowModal();
                    s_viewportSwitcherShown = false;

                    int idx = switcher.GetSelection();

                    if( idx >= 0 && idx < (int) m_viewportMRU.size() )
                        applyViewport( m_viewportMRU[idx] );

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
    if( m_canvas )
        m_canvas->ReloadRequest( GetBoard(), Prj().Get3DCacheManager() );
}


void EDA_3D_VIEWER_FRAME::NewDisplay( bool aForceImmediateRedraw )
{
    ReloadRequest();

    // After the ReloadRequest call, the refresh often takes a bit of time,
    // and it is made here only on request.
    if( aForceImmediateRedraw )
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

    if( m_canvas )
        m_canvas->Close();

    Destroy();
    event.Skip( true );
}


void EDA_3D_VIEWER_FRAME::Process_Special_Functions( wxCommandEvent &event )
{
    int     id = event.GetId();
    bool    isChecked = event.IsChecked();

    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::Process_Special_Functions id %d "
                                 "isChecked %d" ),
                id,
                isChecked );

    if( m_canvas == nullptr )
        return;

    switch( id )
    {
    case ID_RELOAD3D_BOARD:
        NewDisplay( true );
        break;

    case ID_TOOL_SCREENCOPY_TOCLIBBOARD:
    case ID_MENU_SCREENCOPY_PNG:
    case ID_MENU_SCREENCOPY_JPEG:
        takeScreenshot( event );
        return;

    case ID_MENU3D_RESET_DEFAULTS:
    {
        auto cfg = Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>();
        cfg->ResetToDefaults();
        LoadSettings( cfg );

        // Tell canvas that we (may have) changed the render engine
        RenderEngineChanged();

        NewDisplay( true );
    }
        return;

    default:
        wxFAIL_MSG( wxT( "Invalid event in EDA_3D_VIEWER_FRAME::Process_Special_Functions()" ) );
        return;
    }
}


std::vector<VIEWPORT3D> EDA_3D_VIEWER_FRAME::GetUserViewports() const
{
    std::vector<VIEWPORT3D> ret;

    for( const std::pair<const wxString, VIEWPORT3D>& pair : m_viewports )
        ret.emplace_back( pair.second );

    return ret;
}


void EDA_3D_VIEWER_FRAME::SetUserViewports( std::vector<VIEWPORT3D>& aViewportList )
{
    m_viewports.clear();

    for( const VIEWPORT3D& viewport : aViewportList )
    {
        if( m_viewports.count( viewport.name ) )
            continue;

        m_viewports[viewport.name] = viewport;

        m_viewportMRU.Add( viewport.name );
    }
}


void EDA_3D_VIEWER_FRAME::applyViewport( const wxString& aViewportName )
{
    int idx = m_cbViewports->FindString( aViewportName );

    if( idx >= 0 && idx < m_cbViewports->GetCount() - 3 /* separator */ )
    {
        m_cbViewports->SetSelection( idx );
        m_lastSelectedViewport = static_cast<VIEWPORT3D*>( m_cbViewports->GetClientData( idx ) );

        m_currentCamera.SetViewMatrix( m_lastSelectedViewport->matrix );

        if( m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
            m_canvas->Request_refresh();
        else
            m_canvas->RenderRaytracingRequest();

        if( !m_lastSelectedViewport->name.IsEmpty() )
        {
            m_viewportMRU.Remove( m_lastSelectedViewport->name );
            m_viewportMRU.Insert( m_lastSelectedViewport->name, 0 );
        }
    }
    else
    {
        m_cbViewports->SetSelection( -1 ); // separator
        m_lastSelectedViewport = nullptr;
    }
}


void EDA_3D_VIEWER_FRAME::onViewportChanged( wxCommandEvent& aEvent )
{
    int count = m_cbViewports->GetCount();
    int index = m_cbViewports->GetSelection();

    if( index >= 0 && index < count - 3 /* separator */ )
    {
        VIEWPORT3D* viewport = static_cast<VIEWPORT3D*>( m_cbViewports->GetClientData( index ) );

        wxCHECK( viewport, /* void */ );

        applyViewport( viewport->name );
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        wxTextEntryDialog dlg( this, _( "Viewport name:" ), _( "Save Viewport" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            if( m_lastSelectedViewport )
                m_cbViewports->SetStringSelection( m_lastSelectedViewport->name );
            else
                m_cbViewports->SetSelection( -1 );

            return;
        }

        name = dlg.GetValue();
        bool exists = m_viewports.count( name );

        if( !exists )
        {
            m_viewports[name] = VIEWPORT3D( name, m_currentCamera.GetViewMatrix() );

            index = m_cbViewports->Insert( name, index-1, static_cast<void*>( &m_viewports[name] ) );
        }
        else
        {
            index = m_cbViewports->FindString( name );
            m_viewports[name].matrix = m_currentCamera.GetViewMatrix();
            m_viewportMRU.Remove( name );
        }

        m_cbViewports->SetSelection( index );
        m_viewportMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete an existing viewport
        wxArrayString headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Viewports" ) );

        for( std::pair<const wxString, VIEWPORT3D>& pair : m_viewports )
        {
            wxArrayString item;
            item.Add( pair.first );
            items.emplace_back( item );
        }

        EDA_LIST_DIALOG dlg( this, _( "Delete Viewport" ), headers, items );
        dlg.SetListLabel( _( "Select viewport:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString viewportName = dlg.GetTextSelection();
            int idx = m_cbViewports->FindString( viewportName );

            if( idx != wxNOT_FOUND )
            {
                m_viewports.erase( viewportName );
                m_cbViewports->Delete( idx );
                m_viewportMRU.Remove( viewportName );
            }
        }

        if( m_lastSelectedViewport )
            m_cbViewports->SetStringSelection( m_lastSelectedViewport->name );
        else
            m_cbViewports->SetSelection( -1 );

        return;
    }

    passOnFocus();
}


void EDA_3D_VIEWER_FRAME::onUpdateViewportsCb( wxUpdateUIEvent& aEvent )
{
    int count = m_cbViewports->GetCount();
    int index = m_cbViewports->GetSelection();

    if( index >= 0 && index < count - 3 )
    {
        VIEWPORT3D* viewport = static_cast<VIEWPORT3D*>( m_cbViewports->GetClientData( index ) );

        wxCHECK( viewport, /* void */ );

        if( m_currentCamera.GetViewMatrix() != viewport->matrix )
            m_cbViewports->SetSelection( -1 );
    }
}


void EDA_3D_VIEWER_FRAME::onRenderEngineSelection( wxCommandEvent &event )
{
    RENDER_ENGINE& engine = m_boardAdapter.m_Cfg->m_Render.engine;
    RENDER_ENGINE  old_engine = engine;

    if( old_engine == RENDER_ENGINE::OPENGL )
        engine = RENDER_ENGINE::RAYTRACING;
    else
        engine = RENDER_ENGINE::OPENGL;

    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::OnRenderEngineSelection type %s " ),
                engine == RENDER_ENGINE::RAYTRACING ? wxT( "raytracing" ) : wxT( "realtime" ) );

    if( old_engine != engine )
        RenderEngineChanged();
}


void EDA_3D_VIEWER_FRAME::onDisableRayTracing( wxCommandEvent& aEvent )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::%s disabling ray tracing." ),
                __WXFUNCTION__ );

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


void EDA_3D_VIEWER_FRAME::passOnFocus()
{
    if( m_canvas )
        m_canvas->SetFocus();
}


void EDA_3D_VIEWER_FRAME::LoadSettings( APP_SETTINGS_BASE *aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    EDA_3D_VIEWER_SETTINGS* cfg = dynamic_cast<EDA_3D_VIEWER_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::LoadSettings" ) );

    if( cfg )
    {
        m_boardAdapter.m_Cfg = cfg;

        // When opening the 3D viewer, we use the OpenGL mode, never the ray tracing engine
        // because the ray tracing is very time consuming, and can be seen as not working
        // (freeze window) with large boards.
        m_boardAdapter.m_Cfg->m_Render.engine = RENDER_ENGINE::OPENGL;

        m_canvas->SetAnimationEnabled( cfg->m_Camera.animation_enabled );
        m_canvas->SetMovingSpeedMultiplier( cfg->m_Camera.moving_speed_multiplier );
        m_canvas->SetProjectionMode( cfg->m_Camera.projection_mode );
    }
}


void EDA_3D_VIEWER_FRAME::SaveSettings( APP_SETTINGS_BASE *aCfg )
{
    auto cfg = Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>();

    EDA_BASE_FRAME::SaveSettings( cfg );

    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::SaveSettings" ) );

    wxLogTrace( m_logTrace, m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::RAYTRACING ?
                               wxT( "EDA_3D_VIEWER_FRAME::SaveSettings render setting Ray Trace" )
                               :
                               wxT( "EDA_3D_VIEWER_FRAME::SaveSettings render setting OpenGL" ) );

    if( cfg )
    {
        cfg->m_Camera.animation_enabled       = m_canvas->GetAnimationEnabled();
        cfg->m_Camera.moving_speed_multiplier = m_canvas->GetMovingSpeedMultiplier();
        cfg->m_Camera.projection_mode         = m_canvas->GetProjectionMode();

        if( EDA_3D_CONTROLLER* ctrlTool = GetToolManager()->GetTool<EDA_3D_CONTROLLER>() )
            cfg->m_Camera.rotation_increment = ctrlTool->GetRotationIncrement();
    }
}


void EDA_3D_VIEWER_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::CommonSettingsChanged" ) );

    // Regen menu bars, etc
    EDA_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    // There is no base class that handles toolbars for this frame
    ReCreateMainToolbar();

    loadCommonSettings();
    LoadSettings( Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>() );

    NewDisplay( true );
}


void EDA_3D_VIEWER_FRAME::takeScreenshot( wxCommandEvent& event )
{
    wxString   fullFileName;
    bool       fmt_is_jpeg = false;

    if( event.GetId() == ID_MENU_SCREENCOPY_JPEG )
        fmt_is_jpeg = true;

    if( event.GetId() != ID_TOOL_SCREENCOPY_TOCLIBBOARD )
    {
        // Remember path between saves during this session only.
        const wxString wildcard = fmt_is_jpeg ? JpegFileWildcard() : PngFileWildcard();
        const wxString ext = fmt_is_jpeg ? JpegFileExtension : PngFileExtension;

        // First time path is set to the project path.
        if( !m_defaultSaveScreenshotFileName.IsOk() )
            m_defaultSaveScreenshotFileName = Parent()->Prj().GetProjectFullName();

        m_defaultSaveScreenshotFileName.SetExt( ext );

        wxFileDialog dlg( this, _( "3D Image File Name" ),
                          m_defaultSaveScreenshotFileName.GetPath(),
                          m_defaultSaveScreenshotFileName.GetFullName(), wildcard,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        m_defaultSaveScreenshotFileName = dlg.GetPath();

        if( m_defaultSaveScreenshotFileName.GetExt().IsEmpty() )
            m_defaultSaveScreenshotFileName.SetExt( ext );

        fullFileName = m_defaultSaveScreenshotFileName.GetFullPath();

        wxFileName fn = fullFileName;

        if( !fn.IsDirWritable() )
        {
            wxString msg;

            msg.Printf( _( "Insufficient permissions to save file '%s'." ), fullFileName );
            wxMessageBox( msg, _( "Error" ), wxOK | wxICON_ERROR, this );
            return;
        }

        // Be sure the screen area destroyed by the file dialog is redrawn
        // before making a screen copy.
        // Without this call, under Linux the screen refresh is made to late.
        wxYield();
    }

    // Be sure we have the latest 3D view (remember 3D view is buffered)
    m_canvas->Request_refresh( true );
    wxYield();

    // Build image from the 3D buffer
    wxWindowUpdateLocker noUpdates( this );

    wxImage screenshotImage;

    if( m_canvas )
        m_canvas->GetScreenshot( screenshotImage );

    if( event.GetId() == ID_TOOL_SCREENCOPY_TOCLIBBOARD )
    {
        wxBitmap bitmap( screenshotImage );

        wxLogNull doNotLog; // disable logging of failed clipboard actions

        if( wxTheClipboard->Open() )
        {
            wxBitmapDataObject* dobjBmp = new wxBitmapDataObject( bitmap );

            if( !wxTheClipboard->SetData( dobjBmp ) )
                wxMessageBox( _( "Failed to copy image to clipboard" ) );

            wxTheClipboard->Flush();    /* the data in clipboard will stay
                                         * available after the application exits */
            wxTheClipboard->Close();
        }
    }
    else
    {
        if( !screenshotImage.SaveFile( fullFileName,
                                       fmt_is_jpeg ? wxBITMAP_TYPE_JPEG : wxBITMAP_TYPE_PNG ) )
            wxMessageBox( _( "Can't save file" ) );

        screenshotImage.Destroy();
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

    const DPI_SCALING dpi{ settings, this };
    m_canvas->SetScaleFactor( dpi.GetScaleFactor() );

    // TODO(JE) use all control options
    m_boardAdapter.m_MousewheelPanning = settings->m_Input.scroll_modifier_zoom != 0;
}
