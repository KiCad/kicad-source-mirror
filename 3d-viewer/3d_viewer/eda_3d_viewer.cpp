/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/colordlg.h>
#include <wx/colour.h>
#include <wx/filename.h>
#include <wx/string.h>
#include <wx/wupdlock.h>
#include <wx/clipbrd.h>
#include "eda_3d_viewer.h"
#include <3d_viewer_settings.h>
#include <3d_viewer_id.h>
#include <3d_viewer/tools/3d_actions.h>
#include <3d_viewer/tools/3d_controller.h>
#include <3d_viewer/tools/3d_conditions.h>
#include <bitmaps.h>
#include <board_stackup_manager/board_stackup.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <board.h>
#include <core/arraydim.h>
#include <layers_id_colors_and_visibility.h>
#include <gal/dpi_scaling.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <tool/action_manager.h>
#include <tool/common_control.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <widgets/infobar.h>
#include <wildcards_and_files_ext.h>
#include <wx/filedlg.h>

/**
 * Flag to enable 3D viewer main frame window debug tracing.
 *
 * Use "KI_TRACE_EDA_3D_VIEWER" to enable.
 *
 * @ingroup trace_env_vars
 */
const wxChar* EDA_3D_VIEWER::m_logTrace = wxT( "KI_TRACE_EDA_3D_VIEWER" );


BEGIN_EVENT_TABLE( EDA_3D_VIEWER, EDA_BASE_FRAME )

    EVT_ACTIVATE( EDA_3D_VIEWER::OnActivate )
    EVT_SET_FOCUS( EDA_3D_VIEWER::OnSetFocus )

    EVT_TOOL_RANGE( ID_START_COMMAND_3D, ID_MENU_COMMAND_END,
                    EDA_3D_VIEWER::Process_Special_Functions )

    EVT_TOOL( ID_TOOL_SET_VISIBLE_ITEMS, EDA_3D_VIEWER::Install3DViewOptionDialog )

    EVT_MENU( wxID_CLOSE, EDA_3D_VIEWER::Exit3DFrame )
    EVT_MENU( ID_RENDER_CURRENT_VIEW, EDA_3D_VIEWER::OnRenderEngineSelection )
    EVT_MENU( ID_DISABLE_RAY_TRACING, EDA_3D_VIEWER::OnDisableRayTracing )

    EVT_CLOSE( EDA_3D_VIEWER::OnCloseWindow )
END_EVENT_TABLE()


EDA_3D_VIEWER::EDA_3D_VIEWER( KIWAY *aKiway, PCB_BASE_FRAME *aParent, const wxString &aTitle,
                              long style ) :
        KIWAY_PLAYER( aKiway, aParent, FRAME_PCB_DISPLAY3D, aTitle, wxDefaultPosition,
                      wxDefaultSize, style, QUALIFIED_VIEWER3D_FRAMENAME( aParent ) ),
        m_mainToolBar( nullptr ),
        m_canvas( nullptr ),
        m_currentCamera( m_trackBallCamera ),
        m_trackBallCamera( RANGE_SCALE_3D )
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::EDA_3D_VIEWER %s", aTitle );

    m_disable_ray_tracing = false;
    m_aboutTitle = _( "KiCad 3D Viewer" );

    // Give it an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_3d ) );
    SetIcon( icon );

    // Create the status line
    static const int status_dims[4] = { -1, 170, 130, 130 };

    wxStatusBar *status_bar = CreateStatusBar( arrayDim( status_dims ) );
    SetStatusWidths( arrayDim( status_dims ), status_dims );

    m_canvas = new EDA_3D_CANVAS( this,
                                  OGL_ATT_LIST::GetAttributesList( m_boardAdapter.GetAntiAliasingMode() ),
                                  aParent->GetBoard(), m_boardAdapter, m_currentCamera,
                                  Prj().Get3DCacheManager() );

    auto config = Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>();
    LoadSettings( config );

    // Some settings need the canvas
    loadCommonSettings();

    // Create the manager
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), nullptr, nullptr, config, this );

    m_actions = new EDA_3D_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    m_canvas->SetEventDispatcher( m_toolDispatcher );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new EDA_3D_CONTROLLER );
    m_toolManager->InitTools();

    setupUIConditions();

    if( EDA_3D_CONTROLLER* ctrlTool = GetToolManager()->GetTool<EDA_3D_CONTROLLER>() )
        ctrlTool->SetRotationIncrement( config->m_Camera.rotation_increment );

    // Run the viewer control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "3DViewer.Control" );

    CreateMenuBar();
    ReCreateMainToolbar();

    m_infoBar = new WX_INFOBAR( this, &m_auimgr );

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer( 6 ) );
    m_auimgr.AddPane( m_infoBar, EDA_PANE().InfoBar().Name( "InfoBar" ).Top().Layer(1) );
    m_auimgr.AddPane( m_canvas, EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    // Call Update() to fix all pane default sizes, especially the "InfoBar" pane before
    // hidding it.
    m_auimgr.Update();

    // We don't want the infobar displayed right away
    m_auimgr.GetPane( "InfoBar" ).Hide();
    m_auimgr.Update();

    m_canvas->SetInfoBar( m_infoBar );
    m_canvas->SetStatusBar( status_bar );

    // Fixes bug in Windows (XP and possibly others) where the canvas requires the focus
    // in order to receive mouse events.  Otherwise, the user has to click somewhere on
    // the canvas before it will respond to mouse wheel events.
    m_canvas->SetFocus();
}


EDA_3D_VIEWER::~EDA_3D_VIEWER()
{
    m_canvas->SetEventDispatcher( nullptr );

    m_auimgr.UnInit();

    // m_canvas delete will be called by wxWidget manager
    //delete m_canvas;
    //m_canvas = nullptr;
}


void EDA_3D_VIEWER::setupUIConditions()
{
    EDA_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    EDA_3D_CONDITIONS cond( &m_boardAdapter );

// Helper to define check conditions
#define MaterialCheck( x ) ACTION_CONDITIONS().Check( cond.MaterialMode( x ) )
#define FlagCheck( x )     ACTION_CONDITIONS().Check( cond.Flag( x ) )
#define GridSizeCheck( x ) ACTION_CONDITIONS().Check( cond.GridSize( x ) )

    auto raytracingCondition = [this]( const SELECTION& aSel )
    {
        return m_boardAdapter.GetRenderEngine() != RENDER_ENGINE::OPENGL_LEGACY;
    };

    RegisterUIUpdateHandler( ID_RENDER_CURRENT_VIEW,
                             ACTION_CONDITIONS().Check( raytracingCondition ) );

    mgr->SetConditions( EDA_3D_ACTIONS::materialNormal,
                        MaterialCheck( MATERIAL_MODE::NORMAL ) );
    mgr->SetConditions( EDA_3D_ACTIONS::materialDiffuse,
                        MaterialCheck( MATERIAL_MODE::DIFFUSE_ONLY ) );
    mgr->SetConditions( EDA_3D_ACTIONS::materialCAD,
                        MaterialCheck( MATERIAL_MODE::CAD_MODE ) );

    mgr->SetConditions( EDA_3D_ACTIONS::renderShadows,
                        FlagCheck( FL_RENDER_RAYTRACING_SHADOWS ) );
    mgr->SetConditions( EDA_3D_ACTIONS::proceduralTextures,
                        FlagCheck( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES ) );
    mgr->SetConditions( EDA_3D_ACTIONS::addFloor,
                        FlagCheck( FL_RENDER_RAYTRACING_BACKFLOOR ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showRefractions,
                        FlagCheck( FL_RENDER_RAYTRACING_REFRACTIONS ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showReflections,
                        FlagCheck( FL_RENDER_RAYTRACING_REFLECTIONS ) );
    mgr->SetConditions( EDA_3D_ACTIONS::antiAliasing,
                        FlagCheck( FL_RENDER_RAYTRACING_ANTI_ALIASING ) );
    mgr->SetConditions( EDA_3D_ACTIONS::postProcessing,
                        FlagCheck( FL_RENDER_RAYTRACING_POST_PROCESSING ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showBoundingBoxes,
                        FlagCheck( FL_RENDER_OPENGL_SHOW_MODEL_BBOX ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showAxis,
                        FlagCheck( FL_AXIS ) );

    mgr->SetConditions( EDA_3D_ACTIONS::noGrid,        GridSizeCheck( GRID3D_TYPE::NONE ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show10mmGrid,  GridSizeCheck( GRID3D_TYPE::GRID_10MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show5mmGrid,   GridSizeCheck( GRID3D_TYPE::GRID_5MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show2_5mmGrid, GridSizeCheck( GRID3D_TYPE::GRID_2P5MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show1mmGrid,   GridSizeCheck( GRID3D_TYPE::GRID_1MM ) );


    auto orthoCondition =
        [this] ( const SELECTION& )
        {
            return m_currentCamera.GetProjection() == PROJECTION_TYPE::ORTHO;
        };

    mgr->SetConditions( EDA_3D_ACTIONS::toggleOrtho, ACTION_CONDITIONS().Check( orthoCondition ) );

#undef MaterialCheck
#undef FlagCheck
#undef GridSizeCheck
}


void EDA_3D_VIEWER::ReloadRequest()
{
    // This will schedule a request to load later
    if( m_canvas )
        m_canvas->ReloadRequest( GetBoard(), Prj().Get3DCacheManager() );
}


void EDA_3D_VIEWER::NewDisplay( bool aForceImmediateRedraw )
{
    ReloadRequest();

    // After the ReloadRequest call, the refresh often takes a bit of time,
    // and it is made here only on request.
    if( aForceImmediateRedraw )
        m_canvas->Refresh();
}


void EDA_3D_VIEWER::Redraw()
{
    // Only update in OpenGL for an interactive interaction
    if( m_boardAdapter.GetRenderEngine() == RENDER_ENGINE::OPENGL_LEGACY )
        m_canvas->Request_refresh( true );
}


void EDA_3D_VIEWER::refreshRender()
{
    if( m_boardAdapter.GetRenderEngine() == RENDER_ENGINE::OPENGL_LEGACY )
        m_canvas->Request_refresh();
    else
        NewDisplay( true );
}


void EDA_3D_VIEWER::Exit3DFrame( wxCommandEvent &event )
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::Exit3DFrame" );

    Close( true );
}


void EDA_3D_VIEWER::OnCloseWindow( wxCloseEvent &event )
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::OnCloseWindow" );

    if( m_canvas )
        m_canvas->Close();

    // m_canvas delete will be called by wxWidget manager
    //delete m_canvas;
    //m_canvas = nullptr;

    Destroy();
    event.Skip( true );
}


void EDA_3D_VIEWER::Process_Special_Functions( wxCommandEvent &event )
{
    int     id = event.GetId();
    bool    isChecked = event.IsChecked();

    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::Process_Special_Functions id %d isChecked %d",
                id, isChecked );

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

    case ID_MENU3D_BGCOLOR_BOTTOM:
        if( Set3DColorFromUser( m_boardAdapter.m_BgColorBot, _( "Background Color, Bottom" ),
                                nullptr ) )
            refreshRender();
        return;

    case ID_MENU3D_BGCOLOR_TOP:
        if( Set3DColorFromUser( m_boardAdapter.m_BgColorTop, _( "Background Color, Top" ),
                                nullptr ) )
            refreshRender();
        return;

    case ID_MENU3D_SILKSCREEN_COLOR:
        Set3DSilkScreenColorFromUser();
        return;

    case ID_MENU3D_SOLDERMASK_COLOR:
        Set3DSolderMaskColorFromUser();
        return;

    case ID_MENU3D_SOLDERPASTE_COLOR:
        Set3DSolderPasteColorFromUser();
        return;

    case ID_MENU3D_COPPER_COLOR:
        Set3DCopperColorFromUser();
        break;

    case ID_MENU3D_PCB_BODY_COLOR:
        Set3DBoardBodyColorFromUser();
        break;

    case ID_MENU3D_STACKUP_COLORS:
        SynchroniseColoursWithBoard();
        refreshRender();
        break;

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
        wxFAIL_MSG( "Invalid event in EDA_3D_VIEWER::Process_Special_Functions()" );
        return;
    }
}


void EDA_3D_VIEWER::OnRenderEngineSelection( wxCommandEvent &event )
{
    const RENDER_ENGINE old_engine = m_boardAdapter.GetRenderEngine();

    if( old_engine == RENDER_ENGINE::OPENGL_LEGACY )
        m_boardAdapter.SetRenderEngine( RENDER_ENGINE::RAYTRACING );
    else
        m_boardAdapter.SetRenderEngine( RENDER_ENGINE::OPENGL_LEGACY );

    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::OnRenderEngineSelection type %s ",
            ( m_boardAdapter.GetRenderEngine() == RENDER_ENGINE::RAYTRACING ) ? "Ray Trace" :
                                                                                "OpenGL Legacy" );

    if( old_engine != m_boardAdapter.GetRenderEngine() )
        RenderEngineChanged();
}


void EDA_3D_VIEWER::OnDisableRayTracing( wxCommandEvent& aEvent )
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::%s disabling ray tracing.", __WXFUNCTION__ );

    m_disable_ray_tracing = true;
    m_boardAdapter.SetRenderEngine( RENDER_ENGINE::OPENGL_LEGACY );
}


void EDA_3D_VIEWER::OnActivate( wxActivateEvent &aEvent )
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::OnActivate" );

    if( aEvent.GetActive() && m_canvas )
    {
        // Reload data if 3D frame shows a board,
        // because it can be changed since last frame activation
        if( m_canvas->IsReloadRequestPending() )
            m_canvas->Request_refresh();

        // Activates again the focus of the canvas so it will catch mouse and key events
        m_canvas->SetFocus();
    }

    aEvent.Skip(); // required under wxMAC
}


void EDA_3D_VIEWER::OnSetFocus( wxFocusEvent& aEvent )
{
    // Activates again the focus of the canvas so it will catch mouse and key events
    if( m_canvas )
        m_canvas->SetFocus();

    aEvent.Skip();
}


void EDA_3D_VIEWER::LoadSettings( APP_SETTINGS_BASE *aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    EDA_3D_VIEWER_SETTINGS* cfg = dynamic_cast<EDA_3D_VIEWER_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::LoadSettings" );

    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();

    auto set_color =
            [] ( const COLOR4D& aColor, SFVEC4F& aTarget )
            {
                aTarget.r = aColor.r;
                aTarget.g = aColor.g;
                aTarget.b = aColor.b;
                aTarget.a = aColor.a;
            };

    set_color( colors->GetColor( LAYER_3D_BACKGROUND_BOTTOM ), m_boardAdapter.m_BgColorBot );
    set_color( colors->GetColor( LAYER_3D_BACKGROUND_TOP ), m_boardAdapter.m_BgColorTop );
    set_color( colors->GetColor( LAYER_3D_BOARD ), m_boardAdapter.m_BoardBodyColor );
    set_color( colors->GetColor( LAYER_3D_COPPER ), m_boardAdapter.m_CopperColor );
    set_color( colors->GetColor( LAYER_3D_SILKSCREEN_BOTTOM ),
               m_boardAdapter.m_SilkScreenColorBot );
    set_color( colors->GetColor( LAYER_3D_SILKSCREEN_TOP ), m_boardAdapter.m_SilkScreenColorTop );
    set_color( colors->GetColor( LAYER_3D_SOLDERMASK ), m_boardAdapter.m_SolderMaskColorBot );
    set_color( colors->GetColor( LAYER_3D_SOLDERMASK ), m_boardAdapter.m_SolderMaskColorTop );
    set_color( colors->GetColor( LAYER_3D_SOLDERPASTE ), m_boardAdapter.m_SolderPasteColor );

    if( cfg )
    {
        m_boardAdapter.m_RtCameraLightColor =
                m_boardAdapter.GetColor( cfg->m_Render.raytrace_lightColorCamera );
        m_boardAdapter.m_RtLightColorTop =
                m_boardAdapter.GetColor( cfg->m_Render.raytrace_lightColorTop );
        m_boardAdapter.m_RtLightColorBottom =
                m_boardAdapter.GetColor( cfg->m_Render.raytrace_lightColorBottom );

        m_boardAdapter.m_RtLightColor.resize( cfg->m_Render.raytrace_lightColor.size() );
        m_boardAdapter.m_RtLightSphericalCoords.resize( cfg->m_Render.raytrace_lightColor.size() );

        for( size_t i = 0; i < cfg->m_Render.raytrace_lightColor.size(); ++i )
        {
            m_boardAdapter.m_RtLightColor[i] =
                    m_boardAdapter.GetColor( cfg->m_Render.raytrace_lightColor[i] );

            SFVEC2F sphericalCoord =
                    SFVEC2F( ( cfg->m_Render.raytrace_lightElevation[i] + 90.0f ) / 180.0f,
                             cfg->m_Render.raytrace_lightAzimuth[i] / 180.0f );

            sphericalCoord.x = glm::clamp( sphericalCoord.x, 0.0f, 1.0f );
            sphericalCoord.y = glm::clamp( sphericalCoord.y, 0.0f, 2.0f );

            m_boardAdapter.m_RtLightSphericalCoords[i] = sphericalCoord;
        }

#define TRANSFER_SETTING( flag, field ) m_boardAdapter.SetFlag( flag, cfg->m_Render.field )

        TRANSFER_SETTING( FL_USE_REALISTIC_MODE,      realistic );
        TRANSFER_SETTING( FL_SUBTRACT_MASK_FROM_SILK, subtract_mask_from_silk );

        // OpenGL options
        TRANSFER_SETTING( FL_RENDER_OPENGL_COPPER_THICKNESS,          opengl_copper_thickness );
        TRANSFER_SETTING( FL_RENDER_OPENGL_SHOW_MODEL_BBOX,           opengl_show_model_bbox );
        TRANSFER_SETTING( FL_RENDER_OPENGL_AA_DISABLE_ON_MOVE,        opengl_AA_disableOnMove );
        TRANSFER_SETTING( FL_RENDER_OPENGL_THICKNESS_DISABLE_ON_MOVE,
                          opengl_thickness_disableOnMove );
        TRANSFER_SETTING( FL_RENDER_OPENGL_VIAS_DISABLE_ON_MOVE,      opengl_vias_disableOnMove );
        TRANSFER_SETTING( FL_RENDER_OPENGL_HOLES_DISABLE_ON_MOVE,     opengl_holes_disableOnMove );

        // Raytracing options
        TRANSFER_SETTING( FL_RENDER_RAYTRACING_SHADOWS,             raytrace_shadows );
        TRANSFER_SETTING( FL_RENDER_RAYTRACING_BACKFLOOR,           raytrace_backfloor );
        TRANSFER_SETTING( FL_RENDER_RAYTRACING_REFRACTIONS,         raytrace_refractions );
        TRANSFER_SETTING( FL_RENDER_RAYTRACING_REFLECTIONS,         raytrace_reflections );
        TRANSFER_SETTING( FL_RENDER_RAYTRACING_POST_PROCESSING,     raytrace_post_processing );
        TRANSFER_SETTING( FL_RENDER_RAYTRACING_ANTI_ALIASING,       raytrace_anti_aliasing );
        TRANSFER_SETTING( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES, raytrace_procedural_textures );

        TRANSFER_SETTING( FL_AXIS,                            show_axis );
        TRANSFER_SETTING( FL_FP_ATTRIBUTES_NORMAL,            show_footprints_normal );
        TRANSFER_SETTING( FL_FP_ATTRIBUTES_NORMAL_INSERT,     show_footprints_insert );
        TRANSFER_SETTING( FL_FP_ATTRIBUTES_VIRTUAL,           show_footprints_virtual );
        TRANSFER_SETTING( FL_ZONE,                            show_zones );
        TRANSFER_SETTING( FL_ADHESIVE,                        show_adhesive );
        TRANSFER_SETTING( FL_SILKSCREEN,                      show_silkscreen );
        TRANSFER_SETTING( FL_SOLDERMASK,                      show_soldermask );
        TRANSFER_SETTING( FL_SOLDERPASTE,                     show_solderpaste );
        TRANSFER_SETTING( FL_COMMENTS,                        show_comments );
        TRANSFER_SETTING( FL_ECO,                             show_eco );
        TRANSFER_SETTING( FL_SHOW_BOARD_BODY,                 show_board_body );
        TRANSFER_SETTING( FL_CLIP_SILK_ON_VIA_ANNULUS,        clip_silk_on_via_annulus );
        TRANSFER_SETTING( FL_RENDER_PLATED_PADS_AS_PLATED,    renderPlatedPadsAsPlated );

        m_boardAdapter.SetGridType( static_cast<GRID3D_TYPE>( cfg->m_Render.grid_type ) );
        m_boardAdapter.SetAntiAliasingMode(
                static_cast<ANTIALIASING_MODE>( cfg->m_Render.opengl_AA_mode ) );

        m_boardAdapter.m_OpenGlSelectionColor =
                m_boardAdapter.GetColor( cfg->m_Render.opengl_selection_color );

        m_boardAdapter.m_RtShadowSampleCount = cfg->m_Render.raytrace_nrsamples_shadows;
        m_boardAdapter.m_RtReflectionSampleCount = cfg->m_Render.raytrace_nrsamples_reflections;
        m_boardAdapter.m_RtRefractionSampleCount = cfg->m_Render.raytrace_nrsamples_refractions;

        m_boardAdapter.m_RtSpreadShadows = cfg->m_Render.raytrace_spread_shadows;
        m_boardAdapter.m_RtSpreadReflections = cfg->m_Render.raytrace_spread_reflections;
        m_boardAdapter.m_RtSpreadRefractions = cfg->m_Render.raytrace_spread_refractions;

        m_boardAdapter.m_RtRecursiveRefractionCount =
                cfg->m_Render.raytrace_recursivelevel_refractions;
        m_boardAdapter.m_RtRecursiveReflectionCount =
                cfg->m_Render.raytrace_recursivelevel_reflections;

        // When opening the 3D viewer, we use the opengl mode, not the ray tracing engine
        // because the ray tracing is very time consumming, and can be seen as not working
        // (freeze window) with large boards.
#if 0
        RENDER_ENGINE engine = static_cast<RENDER_ENGINE>( cfg->m_Render.engine );
        wxLogTrace( m_logTrace, engine == RENDER_ENGINE::RAYTRACING ?
                                "EDA_3D_VIEWER::LoadSettings render setting Ray Trace" :
                                "EDA_3D_VIEWER::LoadSettings render setting OpenGL" );
#else
        m_boardAdapter.SetRenderEngine( RENDER_ENGINE::OPENGL_LEGACY );
#endif

        m_boardAdapter.SetMaterialMode( static_cast<MATERIAL_MODE>( cfg->m_Render.material_mode ) );

        m_canvas->AnimationEnabledSet( cfg->m_Camera.animation_enabled );
        m_canvas->MovingSpeedMultiplierSet( cfg->m_Camera.moving_speed_multiplier );

#undef TRANSFER_SETTING
    }
}


void EDA_3D_VIEWER::SaveSettings( APP_SETTINGS_BASE *aCfg )
{
    auto cfg = Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>();

    EDA_BASE_FRAME::SaveSettings( cfg );

    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::SaveSettings" );

    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();

    auto save_color =
            [colors] ( SFVEC4F& aSource, LAYER_3D_ID aTarget )
            {
                // You could think just copy the new color in config is enough.
                // unfortunately, SFVEC4F uses floats, and COLOR4D uses doubles,
                // and the conversion SFVEC4F from/to COLOR4D creates small diffs.
                //
                // This has no matter to draw colors, but creates slight differences
                // in config file, that appears always modified.
                // So we must compare the SFVEC4F old and new values and update only
                // actual changes.
                SFVEC4F newSFVEC4Fcolor( float( colors->GetColor( aTarget ).r ),
                                         float( colors->GetColor( aTarget ).g ),
                                         float( colors->GetColor( aTarget ).b ),
                                         float( colors->GetColor( aTarget ).a ) );

                if( aSource != newSFVEC4Fcolor )
                    colors->SetColor( aTarget, COLOR4D( aSource.r, aSource.g, aSource.b,
                                                        aSource.a ) );
            };

    save_color( m_boardAdapter.m_BgColorBot,         LAYER_3D_BACKGROUND_BOTTOM );
    save_color( m_boardAdapter.m_BgColorTop,         LAYER_3D_BACKGROUND_TOP );
    save_color( m_boardAdapter.m_BoardBodyColor,     LAYER_3D_BOARD );
    save_color( m_boardAdapter.m_CopperColor,        LAYER_3D_COPPER );
    save_color( m_boardAdapter.m_SilkScreenColorBot, LAYER_3D_SILKSCREEN_BOTTOM );
    save_color( m_boardAdapter.m_SilkScreenColorTop, LAYER_3D_SILKSCREEN_TOP );
    save_color( m_boardAdapter.m_SolderMaskColorTop, LAYER_3D_SOLDERMASK );
    save_color( m_boardAdapter.m_SolderPasteColor,   LAYER_3D_SOLDERPASTE );

    Pgm().GetSettingsManager().SaveColorSettings( colors, "3d_viewer" );

    wxLogTrace( m_logTrace, m_boardAdapter.GetRenderEngine() == RENDER_ENGINE::RAYTRACING ?
                            "EDA_3D_VIEWER::SaveSettings render setting Ray Trace" :
                            "EDA_3D_VIEWER::SaveSettings render setting OpenGL" );

    if( cfg )
    {

        auto save_color =
                [] ( const SFVEC3F& aSource, COLOR4D& aTarget )
                {
                    aTarget = COLOR4D( aSource.r, aSource.g, aSource.b, 1.0 );
                };

        save_color( m_boardAdapter.m_RtCameraLightColor, cfg->m_Render.raytrace_lightColorCamera );
        save_color( m_boardAdapter.m_RtLightColorTop, cfg->m_Render.raytrace_lightColorTop );
        save_color( m_boardAdapter.m_RtLightColorBottom, cfg->m_Render.raytrace_lightColorBottom );

        for( size_t i = 0; i < cfg->m_Render.raytrace_lightColor.size(); ++i )
        {
            save_color( m_boardAdapter.m_RtLightColor[i], cfg->m_Render.raytrace_lightColor[i] );

            cfg->m_Render.raytrace_lightElevation[i] =
                    (int)( m_boardAdapter.m_RtLightSphericalCoords[i].x * 180.0f - 90.0f );
            cfg->m_Render.raytrace_lightAzimuth[i] =
                    (int)( m_boardAdapter.m_RtLightSphericalCoords[i].y * 180.0f );
        }

        cfg->m_Render.raytrace_nrsamples_shadows = m_boardAdapter.m_RtShadowSampleCount;
        cfg->m_Render.raytrace_nrsamples_reflections = m_boardAdapter.m_RtReflectionSampleCount;
        cfg->m_Render.raytrace_nrsamples_refractions = m_boardAdapter.m_RtRefractionSampleCount;

        cfg->m_Render.raytrace_spread_shadows = m_boardAdapter.m_RtSpreadShadows;
        cfg->m_Render.raytrace_spread_reflections = m_boardAdapter.m_RtSpreadReflections;
        cfg->m_Render.raytrace_spread_refractions = m_boardAdapter.m_RtSpreadRefractions;

        cfg->m_Render.raytrace_recursivelevel_refractions =
                m_boardAdapter.m_RtRecursiveRefractionCount;
        cfg->m_Render.raytrace_recursivelevel_reflections =
                m_boardAdapter.m_RtRecursiveReflectionCount;

#define TRANSFER_SETTING( field, flag ) cfg->m_Render.field = m_boardAdapter.GetFlag( flag )

        cfg->m_Render.engine         = static_cast<int>( m_boardAdapter.GetRenderEngine() );
        cfg->m_Render.grid_type      = static_cast<int>( m_boardAdapter.GetGridType() );
        cfg->m_Render.material_mode  = static_cast<int>( m_boardAdapter.GetMaterialMode() );
        cfg->m_Render.opengl_AA_mode = static_cast<int>( m_boardAdapter.GetAntiAliasingMode() );

        save_color( m_boardAdapter.m_OpenGlSelectionColor, cfg->m_Render.opengl_selection_color );

        cfg->m_Camera.animation_enabled       = m_canvas->AnimationEnabledGet();
        cfg->m_Camera.moving_speed_multiplier = m_canvas->MovingSpeedMultiplierGet();

        if( EDA_3D_CONTROLLER* ctrlTool = GetToolManager()->GetTool<EDA_3D_CONTROLLER>() )
            cfg->m_Camera.rotation_increment = ctrlTool->GetRotationIncrement();

        TRANSFER_SETTING( opengl_AA_disableOnMove,        FL_RENDER_OPENGL_AA_DISABLE_ON_MOVE );
        TRANSFER_SETTING( opengl_copper_thickness,        FL_RENDER_OPENGL_COPPER_THICKNESS );
        TRANSFER_SETTING( opengl_show_model_bbox,         FL_RENDER_OPENGL_SHOW_MODEL_BBOX );
        TRANSFER_SETTING( opengl_thickness_disableOnMove,
                          FL_RENDER_OPENGL_THICKNESS_DISABLE_ON_MOVE );
        TRANSFER_SETTING( opengl_vias_disableOnMove,      FL_RENDER_OPENGL_VIAS_DISABLE_ON_MOVE );
        TRANSFER_SETTING( opengl_holes_disableOnMove,     FL_RENDER_OPENGL_HOLES_DISABLE_ON_MOVE );

        TRANSFER_SETTING( raytrace_anti_aliasing,       FL_RENDER_RAYTRACING_ANTI_ALIASING );
        TRANSFER_SETTING( raytrace_backfloor,           FL_RENDER_RAYTRACING_BACKFLOOR );
        TRANSFER_SETTING( raytrace_post_processing,     FL_RENDER_RAYTRACING_POST_PROCESSING );
        TRANSFER_SETTING( raytrace_procedural_textures, FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES );
        TRANSFER_SETTING( raytrace_reflections,         FL_RENDER_RAYTRACING_REFLECTIONS );
        TRANSFER_SETTING( raytrace_refractions,         FL_RENDER_RAYTRACING_REFRACTIONS );
        TRANSFER_SETTING( raytrace_shadows,             FL_RENDER_RAYTRACING_SHADOWS );

        TRANSFER_SETTING( realistic,                FL_USE_REALISTIC_MODE );
        TRANSFER_SETTING( show_adhesive,            FL_ADHESIVE );
        TRANSFER_SETTING( show_axis,                FL_AXIS );
        TRANSFER_SETTING( show_board_body,          FL_SHOW_BOARD_BODY );
        TRANSFER_SETTING( clip_silk_on_via_annulus, FL_CLIP_SILK_ON_VIA_ANNULUS );
        TRANSFER_SETTING( renderPlatedPadsAsPlated, FL_RENDER_PLATED_PADS_AS_PLATED );
        TRANSFER_SETTING( show_comments,            FL_COMMENTS );
        TRANSFER_SETTING( show_eco,                 FL_ECO );
        TRANSFER_SETTING( show_footprints_insert,   FL_FP_ATTRIBUTES_NORMAL_INSERT );
        TRANSFER_SETTING( show_footprints_normal,   FL_FP_ATTRIBUTES_NORMAL );
        TRANSFER_SETTING( show_footprints_virtual,  FL_FP_ATTRIBUTES_VIRTUAL );
        TRANSFER_SETTING( show_silkscreen,          FL_SILKSCREEN );
        TRANSFER_SETTING( show_soldermask,          FL_SOLDERMASK );
        TRANSFER_SETTING( show_solderpaste,         FL_SOLDERPASTE );
        TRANSFER_SETTING( show_zones,               FL_ZONE );
        TRANSFER_SETTING( subtract_mask_from_silk,  FL_SUBTRACT_MASK_FROM_SILK );

#undef TRANSFER_SETTING
    }
}


void EDA_3D_VIEWER::SynchroniseColoursWithBoard()
{
    BOARD*                 brd       = GetBoard();
    const FAB_LAYER_COLOR* stdColors = GetColorStandardList();
    wxColour               color;

    if( brd )
    {
        const BOARD_STACKUP& stckp = brd->GetDesignSettings().GetStackupDescriptor();

        for( const BOARD_STACKUP_ITEM* stckpItem : stckp.GetList() )
        {
            wxString colorName = stckpItem->GetColor();

            if( colorName.StartsWith( "#" ) ) // This is a user defined color.
            {
                color.Set( colorName );
            }
            else
            {
                for( int i = 0; i < GetColorStandardListCount(); i++ )
                {
                    if( stdColors[i].m_ColorName == colorName )
                    {
                        color = stdColors[i].m_Color;
                        break;
                    }
                }
            }

            if( color.IsOk() )
            {
                switch( stckpItem->GetBrdLayerId() )
                {
                case F_SilkS:
                    m_boardAdapter.m_SilkScreenColorTop.r = color.Red() / 255.0;
                    m_boardAdapter.m_SilkScreenColorTop.g = color.Green() / 255.0;
                    m_boardAdapter.m_SilkScreenColorTop.b = color.Blue() / 255.0;
                    break;
                case B_SilkS:
                    m_boardAdapter.m_SilkScreenColorBot.r = color.Red() / 255.0;
                    m_boardAdapter.m_SilkScreenColorBot.g = color.Green() / 255.0;
                    m_boardAdapter.m_SilkScreenColorBot.b = color.Blue() / 255.0;
                    break;
                case F_Mask:
                    m_boardAdapter.m_SolderMaskColorTop.r = color.Red() / 255.0;
                    m_boardAdapter.m_SolderMaskColorTop.g = color.Green() / 255.0;
                    m_boardAdapter.m_SolderMaskColorTop.b = color.Blue() / 255.0;
                    // Keep the previous alpha value
                    //m_boardAdapter.m_SolderMaskColorTop.a = color.Alpha() / 255.0;
                    break;
                case B_Mask:
                    m_boardAdapter.m_SolderMaskColorBot.r = color.Red() / 255.0;
                    m_boardAdapter.m_SolderMaskColorBot.g = color.Green() / 255.0;
                    m_boardAdapter.m_SolderMaskColorBot.b = color.Blue() / 255.0;
                    // Keep the previous alpha value
                    //m_boardAdapter.m_SolderMaskColorBot.a = color.Alpha() / 255.0;
                    break;
                default:
                    break;
                }
            }
        }
    }
}


void EDA_3D_VIEWER::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::CommonSettingsChanged" );

    // Regen menu bars, etc
    EDA_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );

    // There is no base class that handles toolbars for this frame
    ReCreateMainToolbar();

    loadCommonSettings();

    NewDisplay( true );
}


void EDA_3D_VIEWER::takeScreenshot( wxCommandEvent& event )
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

            msg.Printf( _( "Insufficient permissions required to save file\n%s" ), fullFileName );
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


void EDA_3D_VIEWER::RenderEngineChanged()
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::RenderEngineChanged()" );

    if( m_canvas )
        m_canvas->RenderEngineChanged();
}


bool EDA_3D_VIEWER::Set3DColorFromUser( SFVEC4F &aColor, const wxString& aTitle,
                                        CUSTOM_COLORS_LIST* aPredefinedColors,
                                        bool aAllowOpacityControl,
                                        KIGFX::COLOR4D aDefaultColor )
{
    KIGFX::COLOR4D newcolor;
    KIGFX::COLOR4D oldcolor( aColor.r,aColor.g, aColor.b, aColor.a );

    DIALOG_COLOR_PICKER picker( this, oldcolor, aAllowOpacityControl, aPredefinedColors,
                                aDefaultColor );

    if( picker.ShowModal() != wxID_OK )
        return false;

    newcolor = picker.GetColor();

    if( newcolor == oldcolor )
        return false;

    aColor.r = newcolor.r;
    aColor.g = newcolor.g;
    aColor.b = newcolor.b;
    aColor.a = newcolor.a;

    return true;
}


bool EDA_3D_VIEWER::Set3DSilkScreenColorFromUser()
{
    CUSTOM_COLORS_LIST colors;

    colors.push_back( CUSTOM_COLOR_ITEM( 241.0/255.0, 241.0/255.0, 241.0/255.0, "White" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 4.0/255.0, 18.0/255.0, 21.0/255.0, "Dark" ) );

    if( Set3DColorFromUser( m_boardAdapter.m_SilkScreenColorTop, _( "Silkscreen Color" ),
                            &colors, false, colors[0].m_Color ) )
    {
        m_boardAdapter.m_SilkScreenColorBot = m_boardAdapter.m_SilkScreenColorTop;

        refreshRender();

        return true;
    }

    return false;
}


bool EDA_3D_VIEWER::Set3DSolderMaskColorFromUser()
{
    CUSTOM_COLORS_LIST colors;

    colors.push_back( CUSTOM_COLOR_ITEM(  20/255.0,  51/255.0,  36/255.0, 0.83, "Green" ) );
    colors.push_back( CUSTOM_COLOR_ITEM(  91/255.0, 168/255.0,  12/255.0, 0.83, "Light Green" ) );
    colors.push_back( CUSTOM_COLOR_ITEM(  13/255.0, 104/255.0,  11/255.0, 0.83,
                                          "Saturated Green" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 181/255.0,  19/255.0,  21/255.0, 0.83, "Red" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 239/255.0,  53/255.0,  41/255.0, 0.83,
                                         "Red Light Orange" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 210/255.0,  40/255.0,  14/255.0, 0.83, "Red 2" ) );
    colors.push_back( CUSTOM_COLOR_ITEM(   2/255.0,  59/255.0, 162/255.0, 0.83, "Blue" ) );
    colors.push_back( CUSTOM_COLOR_ITEM(  54/255.0,  79/255.0, 116/255.0, 0.83, "Light blue 1" ) );
    colors.push_back( CUSTOM_COLOR_ITEM(  61/255.0,  85/255.0, 130/255.0, 0.83, "Light blue 2" ) );
    colors.push_back( CUSTOM_COLOR_ITEM(  21/255.0,  70/255.0,  80/255.0, 0.83,
                                          "Green blue (dark)" ) );
    colors.push_back( CUSTOM_COLOR_ITEM(  11/255.0,  11/255.0,  11/255.0, 0.83, "Black" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 245/255.0, 245/255.0, 245/255.0, 0.83, "White" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 119/255.0,  31/255.0,  91/255.0, 0.83, "Purple" ) );
    colors.push_back( CUSTOM_COLOR_ITEM(  32/255.0,   2/255.0,  53/255.0, 0.83, "Purple Dark" ) );

    if( Set3DColorFromUser( m_boardAdapter.m_SolderMaskColorTop, _( "Solder Mask Color" ),
                            &colors, true, colors[0].m_Color ) )
    {
        m_boardAdapter.m_SolderMaskColorBot = m_boardAdapter.m_SolderMaskColorTop;

        refreshRender();

        return true;
    }

    return false;
}


bool EDA_3D_VIEWER::Set3DCopperColorFromUser()
{
    CUSTOM_COLORS_LIST colors;

    colors.push_back( CUSTOM_COLOR_ITEM( 184/255.0, 115/255.0,  50/255.0, "Copper" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 178/255.0, 156/255.0,       0.0, "Gold" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 213/255.0, 213/255.0, 213/255.0, "Silver" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 160/255.0, 160/255.0, 160/255.0, "Tin" ) );

    if( Set3DColorFromUser( m_boardAdapter.m_CopperColor, _( "Copper Color" ), &colors, false,
                            colors[0].m_Color ) )
    {
        refreshRender();

        return true;
    }

    return false;
}


bool EDA_3D_VIEWER::Set3DBoardBodyColorFromUser()
{
    CUSTOM_COLORS_LIST colors;

    colors.push_back( CUSTOM_COLOR_ITEM(  51/255.0,  43/255.0, 22/255.0, 0.9,
                                          "FR4 natural, dark" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 109/255.0, 116/255.0, 75/255.0, 0.9, "FR4 natural" ) );
    colors.push_back( CUSTOM_COLOR_ITEM(  78/255.0,  14/255.0,  5/255.0, 0.9, "brown/red" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 146/255.0,  99/255.0, 47/255.0, 0.9, "brown 1" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 160/255.0, 123/255.0, 54/255.0, 0.9, "brown 2" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 146/255.0,  99/255.0, 47/255.0, 0.9, "brown 3" ) );
    colors.push_back( CUSTOM_COLOR_ITEM(  63/255.0, 126/255.0, 71/255.0, 0.9, "green 1" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 117/255.0, 122/255.0, 90/255.0, 0.9, "green 2" ) );

    if( Set3DColorFromUser( m_boardAdapter.m_BoardBodyColor, _( "Board Body Color" ), &colors,
                            true, colors[0].m_Color ) )
    {
        refreshRender();

        return true;
    }

    return false;
}


bool EDA_3D_VIEWER::Set3DSolderPasteColorFromUser()
{
    CUSTOM_COLORS_LIST colors;

    colors.push_back( CUSTOM_COLOR_ITEM( 128/255.0, 128/255.0, 128/255.0, "grey" ) );
    colors.push_back( CUSTOM_COLOR_ITEM( 213/255.0, 213/255.0, 213/255.0, "Silver" ) );
    colors.push_back( CUSTOM_COLOR_ITEM(  90/255.0,  90/255.0,  90/255.0, "grey 2" ) );

    if( Set3DColorFromUser( m_boardAdapter.m_SolderPasteColor, _( "Solder Paste Color" ), &colors,
                            false, colors[0].m_Color ) )
    {
        refreshRender();

        return true;
    }

    return false;
}


void EDA_3D_VIEWER::loadCommonSettings()
{
    wxCHECK_RET( m_canvas, "Cannot load settings to null canvas" );

    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    const DPI_SCALING dpi{ settings, this };
    m_canvas->SetScaleFactor( dpi.GetScaleFactor() );
    // TODO(JE) use all control options
    m_boardAdapter.SetFlag( FL_MOUSEWHEEL_PANNING, settings->m_Input.scroll_modifier_zoom != 0 );
}
