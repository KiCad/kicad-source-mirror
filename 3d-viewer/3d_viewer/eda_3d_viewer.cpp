/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  eda_3d_viewer.cpp
 * @brief Implements a 3d viewer windows GUI
 */

#include "eda_3d_viewer.h"
#include "../3d_viewer_id.h"
#include <project.h>
#include <gestfich.h>
#include <wx/colordlg.h>
#include <wx/colourdata.h>
#include <lru_cache.h>
#include  "../common_ogl/cogl_att_list.h"
#include <hotkeys_basic.h>
#include <wx/toolbar.h>
#include <bitmaps.h>

/**
 *  Trace mask used to enable or disable the trace output of this class.
 *  The debug output can be turned on by setting the WXTRACE environment variable to
 *  "KI_TRACE_EDA_3D_VIEWER".  See the wxWidgets documentation on wxLogTrace for
 *  more information.
 */
const wxChar * EDA_3D_VIEWER::m_logTrace = wxT( "KI_TRACE_EDA_3D_VIEWER" );


// Key to store 3D Viewer config

static const wxChar keyBgColor_Red[]            = wxT( "BgColor_Red" );
static const wxChar keyBgColor_Green[]          = wxT( "BgColor_Green" );
static const wxChar keyBgColor_Blue[]           = wxT( "BgColor_Blue" );

static const wxChar keyBgColor_Red_Top[]        = wxT( "BgColor_Red_Top" );
static const wxChar keyBgColor_Green_Top[]      = wxT( "BgColor_Green_Top" );
static const wxChar keyBgColor_Blue_Top[]       = wxT( "BgColor_Blue_Top" );

static const wxChar keySMaskColor_Red[]         = wxT( "SMaskColor_Red" );
static const wxChar keySMaskColor_Green[]       = wxT( "SMaskColor_Green" );
static const wxChar keySMaskColor_Blue[]        = wxT( "SMaskColor_Blue" );

static const wxChar keySPasteColor_Red[]        = wxT( "SPasteColor_Red" );
static const wxChar keySPasteColor_Green[]      = wxT( "SPasteColor_Green" );
static const wxChar keySPasteColor_Blue[]       = wxT( "SPasteColor_Blue" );

static const wxChar keySilkColor_Red[]          = wxT( "SilkColor_Red" );
static const wxChar keySilkColor_Green[]        = wxT( "SilkColor_Green" );
static const wxChar keySilkColor_Blue[]         = wxT( "SilkColor_Blue" );

static const wxChar keyCopperColor_Red[]        = wxT( "CopperColor_Red" );
static const wxChar keyCopperColor_Green[]      = wxT( "CopperColor_Green" );
static const wxChar keyCopperColor_Blue[]       = wxT( "CopperColor_Blue" );

static const wxChar keyBoardBodyColor_Red[]     = wxT( "BoardBodyColor_Red" );
static const wxChar keyBoardBodyColor_Green[]   = wxT( "BoardBodyColor_Green" );
static const wxChar keyBoardBodyColor_Blue[]    = wxT( "BoardBodyColor_Blue" );

static const wxChar keyMousewheelPanning[]      = wxT( "MousewheelPAN3D" );

static const wxChar keyShowRealisticMode[]      = wxT( "ShowRealisticMode" );
static const wxChar keyRenderEngine[]           = wxT( "RenderEngine" );
static const wxChar keyRenderRemoveHoles[]      = wxT( "Render_RemoveHoles" );
//static const wxChar keyRenderTextures[]         = wxT( "Render_Textures" );
static const wxChar keyRenderMaterial[]         = wxT( "Render_Material" );

static const wxChar keyRenderOGL_ShowCopperTck[]= wxT( "Render_OGL_ShowCopperThickness" );
static const wxChar keyRenderOGL_ShowModelBBox[]= wxT( "Render_OGL_ShowModelBoudingBoxes" );

static const wxChar keyRenderRAY_Shadows[]      = wxT( "Render_RAY_Shadows" );
static const wxChar keyRenderRAY_Backfloor[]    = wxT( "Render_RAY_Backfloor" );
static const wxChar keyRenderRAY_Refractions[]  = wxT( "Render_RAY_Refractions" );
static const wxChar keyRenderRAY_Reflections[]  = wxT( "Render_RAY_Reflections" );
static const wxChar keyRenderRAY_PostProcess[]  = wxT( "Render_RAY_PostProcess" );
static const wxChar keyRenderRAY_AAliasing[]    = wxT( "Render_RAY_AntiAliasing" );
static const wxChar keyRenderRAY_ProceduralT[]  = wxT( "Render_RAY_ProceduralTextures" );

static const wxChar keyShowAxis[]               = wxT( "ShowAxis" );
static const wxChar keyShowGrid[]               = wxT( "ShowGrid3D" );
static const wxChar keyShowZones[]              = wxT( "ShowZones" );
static const wxChar keyShowFootprints_Normal[]  = wxT( "ShowFootprints_Normal" );
static const wxChar keyShowFootprints_Insert[]  = wxT( "ShowFootprints_Insert" );
static const wxChar keyShowFootprints_Virtual[] = wxT( "ShowFootprints_Virtual" );
static const wxChar keyShowAdhesiveLayers[]     = wxT( "ShowAdhesiveLayers" );
static const wxChar keyShowSilkScreenLayers[]   = wxT( "ShowSilkScreenLayers" );
static const wxChar keyShowSolderMaskLayers[]   = wxT( "ShowSolderMasLayers" );
static const wxChar keyShowSolderPasteLayers[]  = wxT( "ShowSolderPasteLayers" );
static const wxChar keyShowCommentsLayer[]      = wxT( "ShowCommentsLayers" );
static const wxChar keyShowBoardBody[]          = wxT( "ShowBoardBody" );
static const wxChar keyShowEcoLayers[]          = wxT( "ShowEcoLayers" );


BEGIN_EVENT_TABLE( EDA_3D_VIEWER, EDA_BASE_FRAME )

    EVT_ACTIVATE( EDA_3D_VIEWER::OnActivate )
    EVT_SET_FOCUS( EDA_3D_VIEWER::OnSetFocus )

    EVT_TOOL_RANGE( ID_ZOOM_IN,
                    ID_ZOOM_REDRAW,
                    EDA_3D_VIEWER::ProcessZoom )

    EVT_TOOL_RANGE( ID_START_COMMAND_3D,
                    ID_MENU_COMMAND_END,
                    EDA_3D_VIEWER::Process_Special_Functions )

    EVT_MENU( wxID_EXIT,
              EDA_3D_VIEWER::Exit3DFrame )

    EVT_MENU_RANGE( ID_MENU3D_GRID,
                    ID_MENU3D_GRID_END,
                    EDA_3D_VIEWER::On3DGridSelection )

    EVT_MENU_RANGE( ID_MENU3D_ENGINE,
                    ID_MENU3D_ENGINE_END,
                    EDA_3D_VIEWER::OnRenderEngineSelection )

    EVT_CLOSE( EDA_3D_VIEWER::OnCloseWindow )

    EVT_UPDATE_UI_RANGE( ID_START_COMMAND_3D,
                         ID_MENU_COMMAND_END,
                         EDA_3D_VIEWER::OnUpdateMenus )

END_EVENT_TABLE()


EDA_3D_VIEWER::EDA_3D_VIEWER( KIWAY *aKiway,
                              PCB_BASE_FRAME *aParent,
                              const wxString &aTitle,
                              long style ) :

                KIWAY_PLAYER( aKiway,
                              aParent,
                              FRAME_PCB_DISPLAY3D,
                              aTitle,
                              wxDefaultPosition,
                              wxDefaultSize,
                              style,
                              VIEWER3D_FRAMENAME )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::EDA_3D_VIEWER %s" ), aTitle );

    m_canvas = NULL;
    m_defaultFileName = "";

    // Give it an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_3d_xpm ) );
    SetIcon( icon );

    LoadSettings( config() );
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Create the status line
    static const int status_dims[4] = { -1, 130, 130, 170 };

    wxStatusBar *status_bar = CreateStatusBar( DIM( status_dims ) );
    SetStatusWidths( DIM( status_dims ), status_dims );

    CreateMenuBar();
    ReCreateMainToolbar();

    m_canvas = new EDA_3D_CANVAS( this,
                                  COGL_ATT_LIST::GetAttributesList( true ),
                                  aParent->GetBoard(),
                                  m_settings,
                                  Prj().Get3DCacheManager() );

    if( m_canvas )
        m_canvas->SetStatusBar( status_bar );

    m_auimgr.SetManagedWindow( this );

    EDA_PANEINFO horiztb;
    horiztb.HorizontalToolbarPane();

    m_auimgr.AddPane( m_mainToolBar,
                      wxAuiPaneInfo( horiztb ).Name( wxT( "m_mainToolBar" ) ).Top() );

    if( m_canvas )
        m_auimgr.AddPane( m_canvas,
                          wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    m_auimgr.Update();

    m_mainToolBar->EnableTool( ID_RENDER_CURRENT_VIEW,
                               (m_settings.RenderEngineGet() == RENDER_ENGINE_OPENGL_LEGACY) );

    // Fixes bug in Windows (XP and possibly others) where the canvas requires the focus
    // in order to receive mouse events.  Otherwise, the user has to click somewhere on
    // the canvas before it will respond to mouse wheel events.
    if( m_canvas )
        m_canvas->SetFocus();
}


EDA_3D_VIEWER::~EDA_3D_VIEWER()
{
    m_auimgr.UnInit();

    // m_canvas delete will be called by wxWidget manager
    //delete m_canvas;
    //m_canvas = 0;
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


void EDA_3D_VIEWER::Exit3DFrame( wxCommandEvent &event )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::Exit3DFrame" ) );

    Close( true );
}


void EDA_3D_VIEWER::OnCloseWindow( wxCloseEvent &event )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::OnCloseWindow" ) );

    if( m_canvas )
        m_canvas->Close();

    // m_canvas delete will be called by wxWidget manager
    //delete m_canvas;
    //m_canvas = 0;

    Destroy();
    event.Skip( true );
}

#define ROT_ANGLE 10.0

void EDA_3D_VIEWER::Process_Special_Functions( wxCommandEvent &event )
{
    int     id = event.GetId();
    bool    isChecked = event.IsChecked();

    wxLogTrace( m_logTrace,
                wxT( "EDA_3D_VIEWER::Process_Special_Functions id:%d isChecked:%d" ),
                id, isChecked );

    if( m_canvas == NULL )
        return;

    switch( id )
    {
    case ID_RENDER_CURRENT_VIEW:
        m_canvas->RenderRaytracingRequest();
        break;

    case ID_RELOAD3D_BOARD:
        ReloadRequest();
        m_canvas->Request_refresh();
        break;

    case ID_ROTATE3D_X_POS:
        m_settings.CameraGet().RotateX( glm::radians(ROT_ANGLE) );
        m_canvas->Request_refresh();
        break;

    case ID_ROTATE3D_X_NEG:
        m_settings.CameraGet().RotateX( -glm::radians(ROT_ANGLE) );
        m_canvas->Request_refresh();
        break;

    case ID_ROTATE3D_Y_POS:
        m_settings.CameraGet().RotateY( glm::radians(ROT_ANGLE) );
        m_canvas->Request_refresh();
        break;

    case ID_ROTATE3D_Y_NEG:
        m_settings.CameraGet().RotateY( -glm::radians(ROT_ANGLE) );
        m_canvas->Request_refresh();
        break;

    case ID_ROTATE3D_Z_POS:
        m_settings.CameraGet().RotateZ( glm::radians(ROT_ANGLE) );
        m_canvas->Request_refresh();
        break;

    case ID_ROTATE3D_Z_NEG:
        m_settings.CameraGet().RotateZ( -glm::radians(ROT_ANGLE) );
        m_canvas->Request_refresh();
        break;

    case ID_MOVE3D_LEFT:
        m_canvas->SetView3D( WXK_LEFT );
        return;

    case ID_MOVE3D_RIGHT:
        m_canvas->SetView3D( WXK_RIGHT );
        return;

    case ID_MOVE3D_UP:
        m_canvas->SetView3D( WXK_UP );
        return;

    case ID_MOVE3D_DOWN:
        m_canvas->SetView3D( WXK_DOWN );
        return;

    case ID_ORTHO:
        m_settings.CameraGet().ToggleProjection();
        m_canvas->Request_refresh();
        return;

    case ID_TOOL_SCREENCOPY_TOCLIBBOARD:
    case ID_MENU_SCREENCOPY_PNG:
    case ID_MENU_SCREENCOPY_JPEG:
        takeScreenshot( event );
        return;

    case ID_MENU3D_BGCOLOR_BOTTOM_SELECTION:
        if( Set3DColorFromUser( m_settings.m_BgColorBot, _( "Background Color, Bottom" ) ) )
        {
            if( m_settings.RenderEngineGet() == RENDER_ENGINE_OPENGL_LEGACY )
                m_canvas->Request_refresh();
            else
                ReloadRequest();
        }
        return;

    case ID_MENU3D_BGCOLOR_TOP_SELECTION:
        if( Set3DColorFromUser( m_settings.m_BgColorTop, _( "Background Color, Top" ) ) )
        {
            if( m_settings.RenderEngineGet() == RENDER_ENGINE_OPENGL_LEGACY )
                m_canvas->Request_refresh();
            else
                ReloadRequest();
        }
        return;

    case ID_MENU3D_SILKSCREEN_COLOR_SELECTION:
        Set3DSilkScreenColorFromUser();
        return;

    case ID_MENU3D_SOLDERMASK_COLOR_SELECTION:
        Set3DSolderMaskColorFromUser();
        return;

    case ID_MENU3D_SOLDERPASTE_COLOR_SELECTION:
        Set3DSolderPasteColorFromUser();
        return;

    case ID_MENU3D_COPPER_COLOR_SELECTION:
        Set3DCopperColorFromUser();
        break;

    case ID_MENU3D_PCB_BODY_COLOR_SELECTION:
        Set3DBoardBodyColorFromUser();
        break;

    case ID_MENU3D_MOUSEWHEEL_PANNING:
        m_settings.SetFlag( FL_MOUSEWHEEL_PANNING, isChecked );
        break;

    case ID_MENU3D_REALISTIC_MODE:
        m_settings.SetFlag( FL_USE_REALISTIC_MODE, isChecked );
        SetMenuBarOptionsState();
        ReloadRequest( );
        return;

    case ID_MENU3D_FL_RENDER_SHOW_HOLES_IN_ZONES:
        m_settings.SetFlag( FL_RENDER_SHOW_HOLES_IN_ZONES, isChecked );
        ReloadRequest();
        return;

    case ID_MENU3D_FL_RENDER_MATERIAL_MODE_NORMAL:
        m_settings.MaterialModeSet( MATERIAL_MODE_NORMAL );
        ReloadRequest( );
        return;

    case ID_MENU3D_FL_RENDER_MATERIAL_MODE_DIFFUSE_ONLY:
        m_settings.MaterialModeSet( MATERIAL_MODE_DIFFUSE_ONLY );
        ReloadRequest( );
        return;

    case ID_MENU3D_FL_RENDER_MATERIAL_MODE_CAD_MODE:
        m_settings.MaterialModeSet( MATERIAL_MODE_CAD_MODE );
        ReloadRequest( );
        return;

    case ID_MENU3D_FL_OPENGL_RENDER_COPPER_THICKNESS:
        m_settings.SetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS, isChecked );
        ReloadRequest();
        return;

    case ID_MENU3D_FL_OPENGL_RENDER_SHOW_MODEL_BBOX:
        m_settings.SetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX, isChecked );
        m_canvas->Request_refresh();
        return;

    case ID_MENU3D_FL_RAYTRACING_RENDER_SHADOWS:
        m_settings.SetFlag( FL_RENDER_RAYTRACING_SHADOWS, isChecked );
        m_canvas->Request_refresh();
        return;

    case ID_MENU3D_FL_RAYTRACING_PROCEDURAL_TEXTURES:
        m_settings.SetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_FL_RAYTRACING_BACKFLOOR:
        m_settings.SetFlag( FL_RENDER_RAYTRACING_BACKFLOOR, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_FL_RAYTRACING_REFRACTIONS:
        m_settings.SetFlag( FL_RENDER_RAYTRACING_REFRACTIONS, isChecked );
        m_canvas->Request_refresh();
        return;

    case ID_MENU3D_FL_RAYTRACING_REFLECTIONS:
        m_settings.SetFlag( FL_RENDER_RAYTRACING_REFLECTIONS, isChecked );
        m_canvas->Request_refresh();
        return;

    case ID_MENU3D_FL_RAYTRACING_POST_PROCESSING:
        m_settings.SetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_FL_RAYTRACING_ANTI_ALIASING:
        m_settings.SetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING, isChecked );
        m_canvas->Request_refresh();
        return;

    case ID_MENU3D_SHOW_BOARD_BODY:
        m_settings.SetFlag( FL_SHOW_BOARD_BODY, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_AXIS_ONOFF:
        m_settings.SetFlag( FL_AXIS, isChecked );
        m_canvas->Request_refresh();
        return;

    case ID_MENU3D_MODULE_ONOFF_ATTRIBUTES_NORMAL:
        m_settings.SetFlag( FL_MODULE_ATTRIBUTES_NORMAL, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_MODULE_ONOFF_ATTRIBUTES_NORMAL_INSERT:
        m_settings.SetFlag( FL_MODULE_ATTRIBUTES_NORMAL_INSERT, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_MODULE_ONOFF_ATTRIBUTES_VIRTUAL:
        m_settings.SetFlag( FL_MODULE_ATTRIBUTES_VIRTUAL, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_ZONE_ONOFF:
        m_settings.SetFlag( FL_ZONE, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_ADHESIVE_ONOFF:
        m_settings.SetFlag( FL_ADHESIVE, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_SILKSCREEN_ONOFF:
        m_settings.SetFlag( FL_SILKSCREEN, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_SOLDER_MASK_ONOFF:
        m_settings.SetFlag( FL_SOLDERMASK, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_SOLDER_PASTE_ONOFF:
        m_settings.SetFlag( FL_SOLDERPASTE, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_COMMENTS_ONOFF:
        m_settings.SetFlag( FL_COMMENTS, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_ECO_ONOFF:
        m_settings.SetFlag( FL_ECO, isChecked );
        ReloadRequest( );
        return;

    case ID_MENU3D_RESET_DEFAULTS:
    {
        // Reload settings with a dummy config, so it will load the defaults
        wxConfig *fooconfig = new wxConfig("FooBarApp");
        LoadSettings( fooconfig );
        delete fooconfig;

        // Refresh menu option state
        SetMenuBarOptionsState();

        // Tell canvas that we (may) changed the render engine
        RenderEngineChanged();

        ReloadRequest();
    }
        return;

    case ID_MENU3D_HELP_HOTKEY_SHOW_CURRENT_LIST:
    {
        DisplayHotKeys();
    }
        return;

    default:
        wxLogMessage( wxT( "EDA_3D_VIEWER::Process_Special_Functions() error: unknown command %d" ), id );
        return;
    }
}


void EDA_3D_VIEWER::On3DGridSelection( wxCommandEvent &event )
{
    int id = event.GetId();

    wxASSERT( id < ID_MENU3D_GRID_END );
    wxASSERT( id > ID_MENU3D_GRID );

    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::On3DGridSelection id:%d" ), id );

    switch( id )
    {
    case ID_MENU3D_GRID_NOGRID:
        m_settings.GridSet( GRID3D_NONE );
        break;

    case ID_MENU3D_GRID_10_MM:
        m_settings.GridSet( GRID3D_10MM );
        break;

    case ID_MENU3D_GRID_5_MM:
        m_settings.GridSet( GRID3D_5MM );
        break;

    case ID_MENU3D_GRID_2P5_MM:
        m_settings.GridSet( GRID3D_2P5MM );
        break;

    case ID_MENU3D_GRID_1_MM:
        m_settings.GridSet( GRID3D_1MM );
        break;

    default:
        wxLogMessage( wxT( "EDA_3D_VIEWER::On3DGridSelection() error: unknown command %d" ), id );
        return;
    }

    if( m_canvas )
        m_canvas->Request_refresh();
}


void EDA_3D_VIEWER::OnRenderEngineSelection( wxCommandEvent &event )
{
    int id = event.GetId();

    wxASSERT( id < ID_MENU3D_ENGINE_END );
    wxASSERT( id > ID_MENU3D_ENGINE );

    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::OnRenderEngineSelection id:%d" ), id );

    const RENDER_ENGINE old_engine = m_settings.RenderEngineGet();

    switch( id )
    {
    case ID_MENU3D_ENGINE_OPENGL_LEGACY:
        if( old_engine != RENDER_ENGINE_OPENGL_LEGACY )
            m_settings.RenderEngineSet( RENDER_ENGINE_OPENGL_LEGACY );
        break;

    case ID_MENU3D_ENGINE_RAYTRACING:
        if( old_engine != RENDER_ENGINE_RAYTRACING )
            m_settings.RenderEngineSet( RENDER_ENGINE_RAYTRACING );
        break;

    default:
        wxLogMessage( wxT( "EDA_3D_VIEWER::OnRenderEngineSelection() error: unknown command %d" ), id );
        return;
    }

    if( old_engine != m_settings.RenderEngineGet() )
    {
        RenderEngineChanged();
    }
}


void EDA_3D_VIEWER::OnUpdateMenus(wxUpdateUIEvent &event)
{
    //!TODO: verify how many times this event is called and check if that is OK
    // to have it working this way
    SetMenuBarOptionsState();
}


void EDA_3D_VIEWER::ProcessZoom( wxCommandEvent &event )
{
    int id = event.GetId();

    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::ProcessZoom id:%d" ), id );

    if( m_canvas == NULL )
        return;

    switch( id )
    {
    case ID_ZOOM_PAGE:
        m_canvas->SetView3D( WXK_HOME );
        break;

    case ID_ZOOM_IN:
        m_canvas->SetView3D( WXK_F1 );
        break;

    case ID_ZOOM_OUT:
        m_canvas->SetView3D( WXK_F2 );
        break;

    case ID_ZOOM_REDRAW:
        m_canvas->Request_refresh();
        break;

    default:
        wxLogMessage( wxT( "EDA_3D_VIEWER::ProcessZoom() error: unknown command %d" ), id );
        return;
    }

    m_canvas->DisplayStatus();
}


void EDA_3D_VIEWER::OnActivate( wxActivateEvent &event )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::OnActivate" ) );

    if( m_canvas )
    {
        // Reload data if 3D frame shows a board,
        // because it can be changed since last frame activation
        if( m_canvas->IsReloadRequestPending() )
            m_canvas->Request_refresh();

        // Activates again the focus of the canvas so it will catch mouse and key events
        m_canvas->SetFocus();
    }

    event.Skip();    // required under wxMAC
}


void EDA_3D_VIEWER::OnSetFocus(wxFocusEvent &event)
{
    // Activates again the focus of the canvas so it will catch mouse and key events
    if( m_canvas )
        m_canvas->SetFocus();

    event.Skip();
}


void EDA_3D_VIEWER::LoadSettings( wxConfigBase *aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::LoadSettings" ) );

    aCfg->Read( keyBgColor_Red,   &m_settings.m_BgColorBot.r, 0.4 );
    aCfg->Read( keyBgColor_Green, &m_settings.m_BgColorBot.g, 0.4 );
    aCfg->Read( keyBgColor_Blue,  &m_settings.m_BgColorBot.b, 0.5 );

    aCfg->Read( keyBgColor_Red_Top,   &m_settings.m_BgColorTop.r, 0.8 );
    aCfg->Read( keyBgColor_Green_Top, &m_settings.m_BgColorTop.g, 0.8 );
    aCfg->Read( keyBgColor_Blue_Top,  &m_settings.m_BgColorTop.b, 0.9 );

    // m_SolderMaskColor default value = dark grey-green
    aCfg->Read( keySMaskColor_Red,   &m_settings.m_SolderMaskColor.r, 100.0 * 0.2 / 255.0 );
    aCfg->Read( keySMaskColor_Green, &m_settings.m_SolderMaskColor.g, 255.0 * 0.2 / 255.0 );
    aCfg->Read( keySMaskColor_Blue,  &m_settings.m_SolderMaskColor.b, 180.0 * 0.2 / 255.0 );

    // m_SolderPasteColor default value = light grey
    aCfg->Read( keySPasteColor_Red,   &m_settings.m_SolderPasteColor.r, 128.0 /255.0 );
    aCfg->Read( keySPasteColor_Green, &m_settings.m_SolderPasteColor.g, 128.0 /255.0 );
    aCfg->Read( keySPasteColor_Blue,  &m_settings.m_SolderPasteColor.b, 128.0 /255.0 );

    // m_SilkScreenColor default value = white
    aCfg->Read( keySilkColor_Red,   &m_settings.m_SilkScreenColor.r, 0.9 );
    aCfg->Read( keySilkColor_Green, &m_settings.m_SilkScreenColor.g, 0.9 );
    aCfg->Read( keySilkColor_Blue,  &m_settings.m_SilkScreenColor.b, 0.9 );

    // m_CopperColor default value = gold
    aCfg->Read( keyCopperColor_Red,  &m_settings.m_CopperColor.r, 255.0 * 0.7 / 255.0 );
    aCfg->Read( keyCopperColor_Green, &m_settings.m_CopperColor.g, 223.0 * 0.7 / 255.0 );
    aCfg->Read( keyCopperColor_Blue,  &m_settings.m_CopperColor.b, 0.0 /255.0 );

    // m_BoardBodyColor default value = FR4, in realistic mode
    aCfg->Read( keyBoardBodyColor_Red,  &m_settings.m_BoardBodyColor.r, 51.0 / 255.0 );
    aCfg->Read( keyBoardBodyColor_Green, &m_settings.m_BoardBodyColor.g, 43.0 / 255.0 );
    aCfg->Read( keyBoardBodyColor_Blue,  &m_settings.m_BoardBodyColor.b, 22.0 /255.0 );


    bool tmp;
    aCfg->Read( keyMousewheelPanning, &tmp, false );
    m_settings.SetFlag( FL_MOUSEWHEEL_PANNING, tmp );

    aCfg->Read( keyShowRealisticMode, &tmp, true );
    m_settings.SetFlag( FL_USE_REALISTIC_MODE, tmp );

    aCfg->Read( keyRenderRemoveHoles, &tmp, true );
    m_settings.SetFlag( FL_RENDER_SHOW_HOLES_IN_ZONES, tmp );

    // OpenGL options
    aCfg->Read( keyRenderOGL_ShowCopperTck, &tmp, true );
    m_settings.SetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS, tmp );

    aCfg->Read( keyRenderOGL_ShowModelBBox, &tmp, false );
    m_settings.SetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX, tmp );

    // Raytracing options
    aCfg->Read( keyRenderRAY_Shadows, &tmp, true );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_SHADOWS, tmp );

    aCfg->Read( keyRenderRAY_Backfloor, &tmp, true );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_BACKFLOOR, tmp );

    aCfg->Read( keyRenderRAY_Refractions, &tmp, true );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_REFRACTIONS, tmp );

    aCfg->Read( keyRenderRAY_Reflections, &tmp, true );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_REFLECTIONS, tmp );

    aCfg->Read( keyRenderRAY_PostProcess, &tmp, true );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING, tmp );

    aCfg->Read( keyRenderRAY_AAliasing, &tmp, true );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING, tmp );

    aCfg->Read( keyRenderRAY_ProceduralT, &tmp, true );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES, tmp );

    aCfg->Read( keyShowAxis, &tmp, true );
    m_settings.SetFlag( FL_AXIS, tmp );

    aCfg->Read( keyShowFootprints_Normal, &tmp, true );
    m_settings.SetFlag( FL_MODULE_ATTRIBUTES_NORMAL, tmp );

    aCfg->Read( keyShowFootprints_Insert, &tmp, true );
    m_settings.SetFlag( FL_MODULE_ATTRIBUTES_NORMAL_INSERT, tmp );

    aCfg->Read( keyShowFootprints_Virtual, &tmp, true );
    m_settings.SetFlag( FL_MODULE_ATTRIBUTES_VIRTUAL, tmp );

    aCfg->Read( keyShowZones, &tmp, true );
    m_settings.SetFlag( FL_ZONE, tmp );

    aCfg->Read( keyShowAdhesiveLayers, &tmp, true );
    m_settings.SetFlag( FL_ADHESIVE, tmp );

    aCfg->Read( keyShowSilkScreenLayers, &tmp, true );
    m_settings.SetFlag( FL_SILKSCREEN, tmp );

    aCfg->Read( keyShowSolderMaskLayers, &tmp, true );
    m_settings.SetFlag( FL_SOLDERMASK, tmp );

    aCfg->Read( keyShowSolderPasteLayers, &tmp, true );
    m_settings.SetFlag( FL_SOLDERPASTE, tmp );

    aCfg->Read( keyShowCommentsLayer, &tmp, true );
    m_settings.SetFlag( FL_COMMENTS, tmp );

    aCfg->Read( keyShowEcoLayers, &tmp, true );
    m_settings.SetFlag( FL_ECO, tmp );

    aCfg->Read( keyShowBoardBody, &tmp, true );
    m_settings.SetFlag( FL_SHOW_BOARD_BODY, tmp );

    int tmpi;
    aCfg->Read( keyShowGrid, &tmpi, (int)GRID3D_NONE );
    m_settings.GridSet( (GRID3D_TYPE)tmpi );

    aCfg->Read( keyRenderEngine, &tmpi, (int)RENDER_ENGINE_OPENGL_LEGACY );
    m_settings.RenderEngineSet( (RENDER_ENGINE)tmpi );

    aCfg->Read( keyRenderMaterial, &tmpi, (int)MATERIAL_MODE_NORMAL );
    m_settings.MaterialModeSet( (MATERIAL_MODE)tmpi );
}


void EDA_3D_VIEWER::SaveSettings( wxConfigBase *aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::SaveSettings" ) );

    aCfg->Write( keyBgColor_Red,            m_settings.m_BgColorBot.r );
    aCfg->Write( keyBgColor_Green,          m_settings.m_BgColorBot.g );
    aCfg->Write( keyBgColor_Blue,           m_settings.m_BgColorBot.b );

    aCfg->Write( keyBgColor_Red_Top,        m_settings.m_BgColorTop.r );
    aCfg->Write( keyBgColor_Green_Top,      m_settings.m_BgColorTop.g );
    aCfg->Write( keyBgColor_Blue_Top,       m_settings.m_BgColorTop.b );

    aCfg->Write( keySMaskColor_Red,         m_settings.m_SolderMaskColor.r );
    aCfg->Write( keySMaskColor_Green,       m_settings.m_SolderMaskColor.g );
    aCfg->Write( keySMaskColor_Blue,        m_settings.m_SolderMaskColor.b );

    aCfg->Write( keySPasteColor_Red,        m_settings.m_SolderPasteColor.r );
    aCfg->Write( keySPasteColor_Green,      m_settings.m_SolderPasteColor.g );
    aCfg->Write( keySPasteColor_Blue,       m_settings.m_SolderPasteColor.b );

    aCfg->Write( keySilkColor_Red,          m_settings.m_SilkScreenColor.r );
    aCfg->Write( keySilkColor_Green,        m_settings.m_SilkScreenColor.g );
    aCfg->Write( keySilkColor_Blue,         m_settings.m_SilkScreenColor.b );

    aCfg->Write( keyCopperColor_Red,        m_settings.m_CopperColor.r );
    aCfg->Write( keyCopperColor_Green,      m_settings.m_CopperColor.g );
    aCfg->Write( keyCopperColor_Blue,       m_settings.m_CopperColor.b );

    aCfg->Write( keyBoardBodyColor_Red,     m_settings.m_BoardBodyColor.r );
    aCfg->Write( keyBoardBodyColor_Green,   m_settings.m_BoardBodyColor.g );
    aCfg->Write( keyBoardBodyColor_Blue,    m_settings.m_BoardBodyColor.b );

    aCfg->Write( keyShowRealisticMode,      m_settings.GetFlag( FL_USE_REALISTIC_MODE ) );

    aCfg->Write( keyMousewheelPanning,      m_settings.GetFlag( FL_MOUSEWHEEL_PANNING ) );

    aCfg->Write( keyRenderEngine,           (int)m_settings.RenderEngineGet() );

    aCfg->Write( keyRenderRemoveHoles,      m_settings.GetFlag( FL_RENDER_SHOW_HOLES_IN_ZONES ) );

    aCfg->Write( keyRenderMaterial,         (int)m_settings.MaterialModeGet() );

    // OpenGL options
    aCfg->Write( keyRenderOGL_ShowCopperTck,m_settings.GetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS ) );
    aCfg->Write( keyRenderOGL_ShowModelBBox,m_settings.GetFlag( FL_RENDER_OPENGL_SHOW_MODEL_BBOX ) );

    // Raytracing options
    aCfg->Write( keyRenderRAY_Shadows,      m_settings.GetFlag( FL_RENDER_RAYTRACING_SHADOWS ) );
    aCfg->Write( keyRenderRAY_Backfloor,    m_settings.GetFlag( FL_RENDER_RAYTRACING_BACKFLOOR ) );
    aCfg->Write( keyRenderRAY_Refractions,  m_settings.GetFlag( FL_RENDER_RAYTRACING_REFRACTIONS ) );
    aCfg->Write( keyRenderRAY_Reflections,  m_settings.GetFlag( FL_RENDER_RAYTRACING_REFLECTIONS ) );
    aCfg->Write( keyRenderRAY_PostProcess,  m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) );
    aCfg->Write( keyRenderRAY_AAliasing,    m_settings.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING ) );
    aCfg->Write( keyRenderRAY_ProceduralT,  m_settings.GetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES ) );

    aCfg->Write( keyShowAxis,               m_settings.GetFlag( FL_AXIS ) );
    aCfg->Write( keyShowGrid,               (int)m_settings.GridGet() );

    aCfg->Write( keyShowFootprints_Normal,  m_settings.GetFlag( FL_MODULE_ATTRIBUTES_NORMAL ) );
    aCfg->Write( keyShowFootprints_Insert,  m_settings.GetFlag( FL_MODULE_ATTRIBUTES_NORMAL_INSERT ) );
    aCfg->Write( keyShowFootprints_Virtual, m_settings.GetFlag( FL_MODULE_ATTRIBUTES_VIRTUAL ) );

    aCfg->Write( keyShowZones,              m_settings.GetFlag( FL_ZONE ) );
    aCfg->Write( keyShowAdhesiveLayers,     m_settings.GetFlag( FL_ADHESIVE ) );
    aCfg->Write( keyShowSilkScreenLayers,   m_settings.GetFlag( FL_SILKSCREEN ) );
    aCfg->Write( keyShowSolderMaskLayers,   m_settings.GetFlag( FL_SOLDERMASK ) );
    aCfg->Write( keyShowSolderPasteLayers,  m_settings.GetFlag( FL_SOLDERPASTE ) );
    aCfg->Write( keyShowCommentsLayer,      m_settings.GetFlag( FL_COMMENTS ) );
    aCfg->Write( keyShowEcoLayers,          m_settings.GetFlag( FL_ECO ) );
    aCfg->Write( keyShowBoardBody,          m_settings.GetFlag( FL_SHOW_BOARD_BODY ) );
}


void EDA_3D_VIEWER::OnLeftClick( wxDC *DC, const wxPoint &MousePos )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::OnLeftClick" ) );
    // Do nothing
}


void EDA_3D_VIEWER::OnRightClick( const wxPoint &MousePos, wxMenu *PopMenu )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::OnRightClick" ) );
    // Do nothing
}


void EDA_3D_VIEWER::RedrawActiveWindow( wxDC *DC, bool EraseBg )
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER::RedrawActiveWindow" ) );
    // Do nothing
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
        static wxFileName fn;
        const wxString file_ext = fmt_is_jpeg ? wxT( "jpg" ) : wxT( "png" );
        const wxString mask     = wxT( "*." ) + file_ext;

        // First time path is set to the project path.
        if( !fn.IsOk() )
            fn = Parent()->Prj().GetProjectFullName();

        fn.SetExt( file_ext );

        fullFileName = EDA_FILE_SELECTOR( _( "3D Image File Name:" ), fn.GetPath(),
                                          m_defaultFileName, file_ext, mask, this,
                                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT, true );

        if( fullFileName.IsEmpty() )
            return;

        fn = fullFileName;

        // Be sure the screen area destroyed by the file dialog is redrawn
        // before making a screen copy.
        // Without this call, under Linux the screen refresh is made to late.
        wxYield();
    }

    // Be sure we have the latest 3D view (remember 3D view is buffered)
    Refresh();
    wxYield();

    // Build image from the 3D buffer
    wxWindowUpdateLocker noUpdates( this );

    wxImage screenshotImage;

    if( m_canvas )
        m_canvas->GetScreenshot( screenshotImage );

    if( event.GetId() == ID_TOOL_SCREENCOPY_TOCLIBBOARD )
    {
        wxBitmap bitmap( screenshotImage );

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
    if( m_canvas )
        m_canvas->RenderEngineChanged();

    m_mainToolBar->EnableTool( ID_RENDER_CURRENT_VIEW,
                               (m_settings.RenderEngineGet() == RENDER_ENGINE_OPENGL_LEGACY) );

    m_mainToolBar->Refresh();
}


bool EDA_3D_VIEWER::Set3DColorFromUser( SFVEC3D &aColor, const wxString& aTitle,
                                       wxColourData* aPredefinedColors )
{
    wxColour newcolor, oldcolor;

    oldcolor.Set( KiROUND( aColor.r * 255 ),
                  KiROUND( aColor.g * 255 ),
                  KiROUND( aColor.b * 255 ) );

    wxColourData emptyColorSet; // Provides a empty predefined set of colors
                                // if no color set available to avoid use of an
                                // old color set

    if( aPredefinedColors == NULL )
        aPredefinedColors = &emptyColorSet;

    newcolor = wxGetColourFromUser( this, oldcolor, aTitle, aPredefinedColors );

    if( !newcolor.IsOk() )     // Cancel command
        return false;

    if( newcolor != oldcolor )
    {
        aColor.r = (double) newcolor.Red()   / 255.0;
        aColor.g = (double) newcolor.Green() / 255.0;
        aColor.b = (double) newcolor.Blue()  / 255.0;
    }

    return true;
}


bool EDA_3D_VIEWER::Set3DSilkScreenColorFromUser()
{
    wxColourData definedColors;
    unsigned int i = 0;

    definedColors.SetCustomColour( i++, wxColour( 241, 241, 241 ) );    // White
    definedColors.SetCustomColour( i++, wxColour(   4,  18,  21 ) );    // Dark

    for(; i < wxColourData::NUM_CUSTOM;)
    {
        definedColors.SetCustomColour( i++, wxColour(   0,   0,  0 ) );
    }

    bool change = Set3DColorFromUser( m_settings.m_SilkScreenColor,
                                      _( "Silk Screen Color" ),
                                      &definedColors );

    if( change )
        NewDisplay();

    return change;
}


bool EDA_3D_VIEWER::Set3DSolderMaskColorFromUser()
{
    wxColourData definedColors;
    unsigned int i = 0;

    definedColors.SetCustomColour( i++, wxColour( 20,  51,  36 ) ); // Green
    definedColors.SetCustomColour( i++, wxColour( 91, 168,  12 ) ); // Light Green
    definedColors.SetCustomColour( i++, wxColour( 13, 104,  11 ) ); // Saturated Green
    definedColors.SetCustomColour( i++, wxColour(181,  19,  21 ) ); // Red
    definedColors.SetCustomColour( i++, wxColour(239,  53,  41 ) ); // Red Light Orange
    definedColors.SetCustomColour( i++, wxColour(210,  40,  14 ) ); // Red 2
    definedColors.SetCustomColour( i++, wxColour(  2,  59, 162 ) ); // Blue
    definedColors.SetCustomColour( i++, wxColour( 54,  79, 116 ) ); // Light blue 1
    definedColors.SetCustomColour( i++, wxColour( 61,  85, 130 ) ); // Light blue 2
    definedColors.SetCustomColour( i++, wxColour( 21,  70,  80 ) ); // Green blue (dark)
    definedColors.SetCustomColour( i++, wxColour( 11,  11,  11 ) ); // Black
    definedColors.SetCustomColour( i++, wxColour( 245, 245,245 ) ); // White
    definedColors.SetCustomColour( i++, wxColour(119,  31,  91 ) ); // Purple
    definedColors.SetCustomColour( i++, wxColour( 32,   2,  53 ) ); // Purple Dark

    for(; i < wxColourData::NUM_CUSTOM;)
    {
        definedColors.SetCustomColour( i++, wxColour(   0,   0,  0 ) );
    }

    bool change = Set3DColorFromUser( m_settings.m_SolderMaskColor,
                                      _( "Solder Mask Color" ),
                                      &definedColors );

    if( change )
        NewDisplay();

    return change;
}


bool EDA_3D_VIEWER::Set3DCopperColorFromUser()
{
    wxColourData definedColors;
    unsigned int i = 0;

    definedColors.SetCustomColour( i++, wxColour( 184, 115,  50) );   // Copper
    definedColors.SetCustomColour( i++, wxColour( 191, 155,  58) );   // Gold
    definedColors.SetCustomColour( i++, wxColour( 213, 213, 213) );   // Silver
    definedColors.SetCustomColour( i++, wxColour( 160, 160, 160) );   // tin

    for(; i < wxColourData::NUM_CUSTOM;)
    {
        definedColors.SetCustomColour( i++, wxColour(   0,   0,  0 ) );
    }

    bool change = Set3DColorFromUser( m_settings.m_CopperColor,
                                      _( "Copper Color" ),
                                      &definedColors );

    if( change )
        NewDisplay();

    return change;
}


bool EDA_3D_VIEWER::Set3DBoardBodyColorFromUser()
{
    wxColourData definedColors;
    unsigned int i = 0;

    definedColors.SetCustomColour( i++, wxColour(  51,  43, 22 ) ); // FR4 natural, dark
    definedColors.SetCustomColour( i++, wxColour( 109, 116, 75 ) ); // FR4 natural
    definedColors.SetCustomColour( i++, wxColour(  78,  14,  5 ) ); // brown/red
    definedColors.SetCustomColour( i++, wxColour( 146,  99, 47 ) ); // brown 1
    definedColors.SetCustomColour( i++, wxColour( 160, 123, 54 ) ); // brown 2
    definedColors.SetCustomColour( i++, wxColour( 146,  99, 47 ) ); // brown 3
    definedColors.SetCustomColour( i++, wxColour(  63, 126, 71 ) ); // green 1
    definedColors.SetCustomColour( i++, wxColour( 117, 122, 90 ) ); // green 2

    for(; i < wxColourData::NUM_CUSTOM;)
    {
        definedColors.SetCustomColour( i++, wxColour(   0,   0,  0 ) );
    }

    bool change = Set3DColorFromUser( m_settings.m_BoardBodyColor,
                                      _( "Board Body Color" ),
                                      &definedColors );

    if( change )
        NewDisplay();

    return change;
}


bool EDA_3D_VIEWER::Set3DSolderPasteColorFromUser()
{
    wxColourData definedColors;
    unsigned int i = 0;

    definedColors.SetCustomColour( i++, wxColour( 128, 128, 128 ) );    // grey
    definedColors.SetCustomColour( i++, wxColour( 213, 213, 213 ) );    // Silver
    definedColors.SetCustomColour( i++, wxColour( 90,  90,  90  ) );    // grey 2

    for(; i < wxColourData::NUM_CUSTOM;)
    {
        definedColors.SetCustomColour( i++, wxColour(   0,   0,  0 ) );
    }

    bool change = Set3DColorFromUser( m_settings.m_SolderPasteColor,
                                      _( "Solder Paste Color" ),
                                      &definedColors );

    if( change )
        NewDisplay();

    return change;
}


// Define 3D Viewer Hotkeys
static EDA_HOTKEY Hk3D_PivotCenter( _HKI( "Center pivot rotation (Middle mouse click)" ), 0, WXK_SPACE );
static EDA_HOTKEY Hk3D_MoveLeft( _HKI( "Move board Left" ), ID_POPUP_MOVE3D_LEFT, WXK_LEFT );
static EDA_HOTKEY Hk3D_MoveRight( _HKI( "Move board Right" ), ID_POPUP_MOVE3D_RIGHT, WXK_RIGHT );
static EDA_HOTKEY Hk3D_MoveUp( _HKI( "Move board Up" ), ID_POPUP_MOVE3D_UP, WXK_UP );
static EDA_HOTKEY Hk3D_MoveDown( _HKI( "Move board Down" ), ID_POPUP_MOVE3D_DOWN, WXK_DOWN );
static EDA_HOTKEY Hk3D_HomeView( _HKI( "Home view" ), 0, WXK_HOME );
static EDA_HOTKEY Hk3D_ResetView( _HKI( "Reset view" ), 0, 'R' );

static EDA_HOTKEY Hk3D_ViewFront( _HKI( "View Front" ), ID_POPUP_VIEW_YPOS, 'y' );
static EDA_HOTKEY Hk3D_ViewBack( _HKI( "View Back" ), ID_POPUP_VIEW_YNEG, 'Y' );
static EDA_HOTKEY Hk3D_ViewLeft( _HKI( "View Left" ), ID_POPUP_VIEW_XNEG, 'X' );
static EDA_HOTKEY Hk3D_ViewRight( _HKI( "View Right" ), ID_POPUP_VIEW_XPOS, 'x' );
static EDA_HOTKEY Hk3D_ViewTop( _HKI( "View Top" ), ID_POPUP_VIEW_ZPOS, 'z' );
static EDA_HOTKEY Hk3D_ViewBot( _HKI( "View Bot" ), ID_POPUP_VIEW_ZNEG, 'Z' );

static EDA_HOTKEY Hk3D_Rotate45axisZ( _HKI( "Rotate 45 degrees over Z axis" ), 0, WXK_TAB );
static EDA_HOTKEY Hk3D_ZoomIn( _HKI( "Zoom in " ), ID_POPUP_ZOOMIN, WXK_F1 );
static EDA_HOTKEY Hk3D_ZoomOut( _HKI( "Zoom out" ), ID_POPUP_ZOOMOUT, WXK_F2 );
static EDA_HOTKEY Hk3D_AttributesTHT( _HKI( "Toggle 3D models with type Through Hole" ), 0, 'T' );
static EDA_HOTKEY Hk3D_AttributesSMD( _HKI( "Toggle 3D models with type Surface Mount" ), 0, 'S' );
static EDA_HOTKEY Hk3D_AttributesVirtual( _HKI( "Toggle 3D models with type Virtual" ), 0, 'V' );

static wxString viewer3DSectionTitle( _HKI( "Viewer 3D" ) );

// List of hotkey descriptors for the 3D Viewer only
// !TODO: this is used just for help menu, the structured are not used yet in the viewer
static EDA_HOTKEY* viewer3d_Hotkey_List[] =
{
    &Hk3D_PivotCenter,
    &Hk3D_MoveLeft,
    &Hk3D_MoveRight,
    &Hk3D_MoveUp,
    &Hk3D_MoveDown,
    &Hk3D_HomeView,
    &Hk3D_ResetView,
    &Hk3D_ViewFront,
    &Hk3D_ViewBack,
    &Hk3D_ViewLeft,
    &Hk3D_ViewRight,
    &Hk3D_ViewTop,
    &Hk3D_ViewBot,
    &Hk3D_Rotate45axisZ,
    &Hk3D_ZoomIn,
    &Hk3D_ZoomOut,
    &Hk3D_AttributesTHT,
    &Hk3D_AttributesSMD,
    &Hk3D_AttributesVirtual,
    NULL
};


// list of sections and corresponding hotkey list for the 3D Viewer
// (used to list current hotkeys)
struct EDA_HOTKEY_CONFIG g_3DViewer_Hokeys_Descr[] =
{
    { &g_CommonSectionTag, viewer3d_Hotkey_List, &viewer3DSectionTitle },
    { NULL,                NULL,                 NULL }
};


void EDA_3D_VIEWER::DisplayHotKeys()
{
    DisplayHotkeyList( this, g_3DViewer_Hokeys_Descr );
}
