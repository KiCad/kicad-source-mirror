/**
 * @file 3d_frame.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <macros.h>

#include <3d_viewer.h>
#include <3d_canvas.h>
#include <info3d_visu.h>
#include <trackball.h>

#include <wx/colordlg.h>
#include <3d_viewer_id.h>
#include <wxBasePcbFrame.h>

INFO3D_VISU             g_Parm_3D_Visu;

// Key to store 3D Viewer config:
static const wxChar keyBgColor_Red[] =          wxT( "BgColor_Red" );
static const wxChar keyBgColor_Green[] =        wxT( "BgColor_Green" );
static const wxChar keyBgColor_Blue[] =         wxT( "BgColor_Blue" );

static const wxChar keyBgColor_Red_Top[]    = wxT( "BgColor_Red_Top" );
static const wxChar keyBgColor_Green_Top[]  = wxT( "BgColor_Green_Top" );
static const wxChar keyBgColor_Blue_Top[]   = wxT( "BgColor_Blue_Top" );

static const wxChar keySMaskColor_Red[]     = wxT( "SMaskColor_Red" );
static const wxChar keySMaskColor_Green[]   = wxT( "SMaskColor_Green" );
static const wxChar keySMaskColor_Blue[]    = wxT( "SMaskColor_Blue" );

static const wxChar keySPasteColor_Red[]     = wxT( "SPasteColor_Red" );
static const wxChar keySPasteColor_Green[]   = wxT( "SPasteColor_Green" );
static const wxChar keySPasteColor_Blue[]    = wxT( "SPasteColor_Blue" );

static const wxChar keySilkColor_Red[]      = wxT( "SilkColor_Red" );
static const wxChar keySilkColor_Green[]    = wxT( "SilkColor_Green" );
static const wxChar keySilkColor_Blue[]     = wxT( "SilkColor_Blue" );

static const wxChar keyCopperColor_Red[]    = wxT( "CopperColor_Red" );
static const wxChar keyCopperColor_Green[]  = wxT( "CopperColor_Green" );
static const wxChar keyCopperColor_Blue[]   = wxT( "CopperColor_Blue" );

static const wxChar keyBoardBodyColor_Red[] = wxT( "BoardBodyColor_Red" );
static const wxChar keyBoardBodyColor_Green[]    = wxT( "BoardBodyColor_Green" );
static const wxChar keyBoardBodyColor_Blue[]= wxT( "BoardBodyColor_Blue" );

static const wxChar keyShowRealisticMode[] =    wxT( "ShowRealisticMode" );
static const wxChar keyRenderShadows[] =        wxT( "Render_Shadows" );
static const wxChar keyRenderRemoveHoles[] =    wxT( "Render_RemoveHoles" );
static const wxChar keyRenderTextures[] =       wxT( "Render_Textures" );
static const wxChar keyRenderSmoothNormals[] =  wxT( "Render_Smooth_Normals" );
static const wxChar keyRenderUseModelNormals[] =wxT( "Render_Use_Model_Normals" );
static const wxChar keyRenderMaterial[] =       wxT( "Render_Material" );
static const wxChar keyRenderShowModelBBox[] =  wxT( "Render_ShowModelBoudingBoxes" );

static const wxChar keyShowAxis[] =             wxT( "ShowAxis" );
static const wxChar keyShowGrid[] =             wxT( "ShowGrid3D" );
static const wxChar keyShowGridSize[] =         wxT( "Grid3DSize" );
static const wxChar keyShowZones[] =            wxT( "ShowZones" );
static const wxChar keyShowFootprints[] =       wxT( "ShowFootprints" );
static const wxChar keyShowCopperThickness[] =  wxT( "ShowCopperThickness" );
static const wxChar keyShowAdhesiveLayers[] =   wxT( "ShowAdhesiveLayers" );
static const wxChar keyShowSilkScreenLayers[] = wxT( "ShowSilkScreenLayers" );
static const wxChar keyShowSolderMaskLayers[] = wxT( "ShowSolderMasLayers" );
static const wxChar keyShowSolderPasteLayers[] =wxT( "ShowSolderPasteLayers" );
static const wxChar keyShowCommentsLayer[] =    wxT( "ShowCommentsLayers" );
static const wxChar keyShowBoardBody[] =        wxT( "ShowBoardBody" );
static const wxChar keyShowEcoLayers[] =        wxT( "ShowEcoLayers" );


BEGIN_EVENT_TABLE( EDA_3D_FRAME, EDA_BASE_FRAME )
EVT_ACTIVATE( EDA_3D_FRAME::OnActivate )

EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, EDA_3D_FRAME::Process_Zoom )
EVT_TOOL_RANGE( ID_START_COMMAND_3D, ID_END_COMMAND_3D,
                EDA_3D_FRAME::Process_Special_Functions )
EVT_TOOL( ID_TOOL_SET_VISIBLE_ITEMS, EDA_3D_FRAME::Process_Special_Functions )
EVT_MENU( wxID_EXIT, EDA_3D_FRAME::Exit3DFrame )
EVT_MENU( ID_MENU_SCREENCOPY_PNG, EDA_3D_FRAME::Process_Special_Functions )
EVT_MENU( ID_MENU_SCREENCOPY_JPEG, EDA_3D_FRAME::Process_Special_Functions )

EVT_MENU_RANGE( ID_MENU3D_GRID, ID_MENU3D_GRID_END,
                EDA_3D_FRAME::On3DGridSelection )

EVT_CLOSE( EDA_3D_FRAME::OnCloseWindow )

END_EVENT_TABLE()


EDA_3D_FRAME::EDA_3D_FRAME( KIWAY* aKiway, PCB_BASE_FRAME* aParent,
        const wxString& aTitle, long style ) :
    KIWAY_PLAYER( aKiway, aParent, FRAME_PCB_DISPLAY3D, aTitle,
            wxDefaultPosition, wxDefaultSize, style, VIEWER3D_FRAMENAME )
{
    m_canvas        = NULL;
    m_reloadRequest = false;
    m_ortho         = false;

    // Give it an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_3d_xpm ) );
    SetIcon( icon );

    LoadSettings( config() );
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Create the status line
    static const int status_dims[4] = { -1, 130, 130, 170 };

    CreateStatusBar( DIM( status_dims ) );
    SetStatusWidths( DIM( status_dims ), status_dims );

    CreateMenuBar();
    ReCreateMainToolbar();

    // Make a EDA_3D_CANVAS
    // Note: We try to use anti aliasing if the graphic card allows that,
    // but only on wxWidgets >= 3.0.0 (this option does not exist on wxWidgets 2.8)
    int attrs[] = { // This array should be 2*n+1
                    // Sadly wxwidgets / glx < 13 allowed
                    // a thing named "boolean attributes" that don't take a value.
                    // (See src/unix/glx11.cpp -> wxGLCanvasX11::ConvertWXAttrsToGL() ).
                    // To avoid problems due to this, just specify those attributes twice.
                    // Only WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_STEREO are such boolean
                    // attributes.

                    // Boolean attributes (using itself at padding):
                    WX_GL_RGBA, WX_GL_RGBA,
                    WX_GL_DOUBLEBUFFER, WX_GL_DOUBLEBUFFER,

                    // Normal attributes with values:
                    WX_GL_DEPTH_SIZE, 16,
                    WX_GL_STENCIL_SIZE, 1,
                    WX_GL_SAMPLE_BUFFERS, 1,    // Enable multisampling support (antialiasing).
                    WX_GL_SAMPLES, 0,           // Disable AA for the start.
                    0 };                        // NULL termination


    // Check if the canvas supports multisampling.
    if( EDA_3D_CANVAS::IsDisplaySupported( attrs ) )
    {
        // Check for possible sample sizes, start form the top.
        int maxSamples = 8; // Any higher doesn't change anything.
        int samplesOffset = 0;

        for( unsigned int ii = 0; ii < DIM( attrs ); ii += 2 )
        {
            if( attrs[ii] == WX_GL_SAMPLES )
            {
                samplesOffset = ii+1;
                break;
            }
        }

        attrs[samplesOffset] = maxSamples;

        for( ; maxSamples > 0 && !EDA_3D_CANVAS::IsDisplaySupported( attrs );
            maxSamples = maxSamples>>1 )
        {
            attrs[samplesOffset] = maxSamples;
        }
    }
    else
    {
        // Disable multisampling
        for( unsigned int ii = 0; ii < DIM( attrs ); ii += 2 )
        {
            if( attrs[ii] == WX_GL_SAMPLE_BUFFERS )
            {
                attrs[ii+1] = 0;
                break;
            }
        }
    }

    m_canvas = new EDA_3D_CANVAS( this, attrs );

    m_auimgr.SetManagedWindow( this );


    EDA_PANEINFO horiztb;
    horiztb.HorizontalToolbarPane();

    m_auimgr.AddPane( m_mainToolBar,
                      wxAuiPaneInfo( horiztb ).Name( wxT( "m_mainToolBar" ) ).Top() );

    m_auimgr.AddPane( m_canvas,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    m_auimgr.Update();

    // Fixes bug in Windows (XP and possibly others) where the canvas requires the focus
    // in order to receive mouse events.  Otherwise, the user has to click somewhere on
    // the canvas before it will respond to mouse wheel events.
    m_canvas->SetFocus();
}


EDA_3D_FRAME::~EDA_3D_FRAME()
{
    m_auimgr.UnInit();
}

void EDA_3D_FRAME::Exit3DFrame( wxCommandEvent& event )
{
    Close( true );
}


void EDA_3D_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    Destroy();
}


void EDA_3D_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    INFO3D_VISU& prms = GetPrm3DVisu();

    aCfg->Read( keyBgColor_Red, &GetPrm3DVisu().m_BgColor.m_Red, 0.4 );
    aCfg->Read( keyBgColor_Green, &GetPrm3DVisu().m_BgColor.m_Green, 0.4 );
    aCfg->Read( keyBgColor_Blue, &GetPrm3DVisu().m_BgColor.m_Blue, 0.5 );

    aCfg->Read( keyBgColor_Red_Top, &GetPrm3DVisu().m_BgColor_Top.m_Red, 0.8 );
    aCfg->Read( keyBgColor_Green_Top, &GetPrm3DVisu().m_BgColor_Top.m_Green, 0.8 );
    aCfg->Read( keyBgColor_Blue_Top, &GetPrm3DVisu().m_BgColor_Top.m_Blue, 0.9 );

    // m_SolderMaskColor default value = dark grey-green
    aCfg->Read( keySMaskColor_Red, &GetPrm3DVisu().m_SolderMaskColor.m_Red, 100.0 * 0.2 / 255.0 );
    aCfg->Read( keySMaskColor_Green, &GetPrm3DVisu().m_SolderMaskColor.m_Green, 255.0 * 0.2 / 255.0 );
    aCfg->Read( keySMaskColor_Blue, &GetPrm3DVisu().m_SolderMaskColor.m_Blue, 180.0 * 0.2 / 255.0 );

    // m_SolderPasteColor default value = light grey
    aCfg->Read( keySPasteColor_Red, &GetPrm3DVisu().m_SolderPasteColor.m_Red, 128.0 /255.0 );
    aCfg->Read( keySPasteColor_Green, &GetPrm3DVisu().m_SolderPasteColor.m_Green, 128.0 /255.0 );
    aCfg->Read( keySPasteColor_Blue, &GetPrm3DVisu().m_SolderPasteColor.m_Blue, 128.0 /255.0 );

    // m_SilkScreenColor default value = white
    aCfg->Read( keySilkColor_Red, &GetPrm3DVisu().m_SilkScreenColor.m_Red, 0.9 );
    aCfg->Read( keySilkColor_Green, &GetPrm3DVisu().m_SilkScreenColor.m_Green, 0.9 );
    aCfg->Read( keySilkColor_Blue, &GetPrm3DVisu().m_SilkScreenColor.m_Blue, 0.9 );

    // m_CopperColor default value = gold
    aCfg->Read( keyCopperColor_Red, &GetPrm3DVisu().m_CopperColor.m_Red, 255.0 * 0.7 / 255.0 );
    aCfg->Read( keyCopperColor_Green, &GetPrm3DVisu().m_CopperColor.m_Green, 223.0 * 0.7 / 255.0 );
    aCfg->Read( keyCopperColor_Blue, &GetPrm3DVisu().m_CopperColor.m_Blue, 0.0 /255.0 );

    // m_BoardBodyColor default value = FR4, in realistic mode
    aCfg->Read( keyBoardBodyColor_Red, &GetPrm3DVisu().m_BoardBodyColor.m_Red, 51.0 / 255.0 );
    aCfg->Read( keyBoardBodyColor_Green, &GetPrm3DVisu().m_BoardBodyColor.m_Green, 43.0 / 255.0 );
    aCfg->Read( keyBoardBodyColor_Blue, &GetPrm3DVisu().m_BoardBodyColor.m_Blue, 22.0 /255.0 );

    bool tmp;
    aCfg->Read( keyShowRealisticMode, &tmp, false );
    prms.SetFlag( FL_USE_REALISTIC_MODE, tmp );

    aCfg->Read( keyRenderShadows, &tmp, false );
    prms.SetFlag( FL_RENDER_SHADOWS, tmp );

    aCfg->Read( keyRenderRemoveHoles, &tmp, false );
    prms.SetFlag( FL_RENDER_SHOW_HOLES_IN_ZONES, tmp );

    aCfg->Read( keyRenderTextures, &tmp, false );
    prms.SetFlag( FL_RENDER_TEXTURES, tmp );

    aCfg->Read( keyRenderSmoothNormals, &tmp, false );
    prms.SetFlag( FL_RENDER_SMOOTH_NORMALS, tmp );

    aCfg->Read( keyRenderUseModelNormals, &tmp, false );
    prms.SetFlag( FL_RENDER_USE_MODEL_NORMALS, tmp );

    aCfg->Read( keyRenderMaterial, &tmp, false );
    prms.SetFlag( FL_RENDER_MATERIAL, tmp );

    aCfg->Read( keyRenderShowModelBBox, &tmp, false );
    prms.SetFlag( FL_RENDER_SHOW_MODEL_BBOX, tmp );

    aCfg->Read( keyShowAxis, &tmp, true );
    prms.SetFlag( FL_AXIS, tmp );

    aCfg->Read( keyShowGrid, &tmp, true );
    prms.SetFlag( FL_GRID, tmp );

    aCfg->Read( keyShowGridSize, &prms.m_3D_Grid, 10.0 );

    aCfg->Read( keyShowFootprints, &tmp, true );
    prms.SetFlag( FL_MODULE, tmp );

    aCfg->Read( keyShowCopperThickness, &tmp, false );
    prms.SetFlag( FL_USE_COPPER_THICKNESS, tmp );

    aCfg->Read( keyShowZones, &tmp, true );
    prms.SetFlag( FL_ZONE, tmp );

    aCfg->Read( keyShowAdhesiveLayers, &tmp, true );
    prms.SetFlag( FL_ADHESIVE, tmp );

    aCfg->Read( keyShowSilkScreenLayers, &tmp, true );
    prms.SetFlag( FL_SILKSCREEN, tmp );

    aCfg->Read( keyShowSolderMaskLayers, &tmp, true );
    prms.SetFlag( FL_SOLDERMASK, tmp );

    aCfg->Read( keyShowSolderPasteLayers, &tmp, true );
    prms.SetFlag( FL_SOLDERPASTE, tmp );

    aCfg->Read( keyShowCommentsLayer, &tmp, true );
    prms.SetFlag( FL_COMMENTS, tmp );

    aCfg->Read( keyShowEcoLayers, &tmp, true );
    prms.SetFlag( FL_ECO, tmp );

    aCfg->Read( keyShowBoardBody, &tmp, true );
    prms.SetFlag( FL_SHOW_BOARD_BODY, tmp );
}


void EDA_3D_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    INFO3D_VISU& prms = GetPrm3DVisu();

    aCfg->Write( keyBgColor_Red, GetPrm3DVisu().m_BgColor.m_Red );
    aCfg->Write( keyBgColor_Green, GetPrm3DVisu().m_BgColor.m_Green );
    aCfg->Write( keyBgColor_Blue, GetPrm3DVisu().m_BgColor.m_Blue );

    aCfg->Write( keyBgColor_Red_Top, GetPrm3DVisu().m_BgColor_Top.m_Red );
    aCfg->Write( keyBgColor_Green_Top, GetPrm3DVisu().m_BgColor_Top.m_Green );
    aCfg->Write( keyBgColor_Blue_Top, GetPrm3DVisu().m_BgColor_Top.m_Blue );

    aCfg->Write( keySMaskColor_Red, GetPrm3DVisu().m_SolderMaskColor.m_Red );
    aCfg->Write( keySMaskColor_Green, GetPrm3DVisu().m_SolderMaskColor.m_Green );
    aCfg->Write( keySMaskColor_Blue, GetPrm3DVisu().m_SolderMaskColor.m_Blue );

    aCfg->Write( keySPasteColor_Red, GetPrm3DVisu().m_SolderPasteColor.m_Red );
    aCfg->Write( keySPasteColor_Green, GetPrm3DVisu().m_SolderPasteColor.m_Green );
    aCfg->Write( keySPasteColor_Blue, GetPrm3DVisu().m_SolderPasteColor.m_Blue );

    aCfg->Write( keySilkColor_Red, GetPrm3DVisu().m_SilkScreenColor.m_Red );
    aCfg->Write( keySilkColor_Green, GetPrm3DVisu().m_SilkScreenColor.m_Green );
    aCfg->Write( keySilkColor_Blue, GetPrm3DVisu().m_SilkScreenColor.m_Blue );

    aCfg->Write( keyCopperColor_Red, GetPrm3DVisu().m_CopperColor.m_Red );
    aCfg->Write( keyCopperColor_Green, GetPrm3DVisu().m_CopperColor.m_Green );
    aCfg->Write( keyCopperColor_Blue, GetPrm3DVisu().m_CopperColor.m_Blue );

    aCfg->Write( keyBoardBodyColor_Red, GetPrm3DVisu().m_BoardBodyColor.m_Red );
    aCfg->Write( keyBoardBodyColor_Green, GetPrm3DVisu().m_BoardBodyColor.m_Green );
    aCfg->Write( keyBoardBodyColor_Blue, GetPrm3DVisu().m_BoardBodyColor.m_Blue );

    aCfg->Write( keyShowRealisticMode, prms.GetFlag( FL_USE_REALISTIC_MODE ) );

    aCfg->Write( keyRenderShadows, prms.GetFlag( FL_RENDER_SHADOWS ) );
    aCfg->Write( keyRenderRemoveHoles, prms.GetFlag( FL_RENDER_SHOW_HOLES_IN_ZONES ) );
    aCfg->Write( keyRenderTextures, prms.GetFlag( FL_RENDER_TEXTURES ) );
    aCfg->Write( keyRenderSmoothNormals, prms.GetFlag( FL_RENDER_SMOOTH_NORMALS ) );
    aCfg->Write( keyRenderUseModelNormals, prms.GetFlag( FL_RENDER_USE_MODEL_NORMALS ) );
    aCfg->Write( keyRenderMaterial, prms.GetFlag( FL_RENDER_MATERIAL ) );
    aCfg->Write( keyRenderShowModelBBox, prms.GetFlag( FL_RENDER_SHOW_MODEL_BBOX ) );

    aCfg->Write( keyShowAxis, prms.GetFlag( FL_AXIS ) );
    aCfg->Write( keyShowGrid, prms.GetFlag( FL_GRID ) );
    aCfg->Write( keyShowGridSize, prms.m_3D_Grid );
    aCfg->Write( keyShowFootprints, prms.GetFlag( FL_MODULE ) );
    aCfg->Write( keyShowCopperThickness, prms.GetFlag( FL_USE_COPPER_THICKNESS ) );
    aCfg->Write( keyShowZones, prms.GetFlag( FL_ZONE ) );
    aCfg->Write( keyShowAdhesiveLayers, prms.GetFlag( FL_ADHESIVE ) );
    aCfg->Write( keyShowSilkScreenLayers, prms.GetFlag( FL_SILKSCREEN ) );
    aCfg->Write( keyShowSolderMaskLayers, prms.GetFlag( FL_SOLDERMASK ) );
    aCfg->Write( keyShowSolderPasteLayers, prms.GetFlag( FL_SOLDERPASTE ) );
    aCfg->Write( keyShowCommentsLayer, prms.GetFlag( FL_COMMENTS ) );
    aCfg->Write( keyShowEcoLayers, prms.GetFlag( FL_ECO ) );
    aCfg->Write( keyShowBoardBody, prms.GetFlag( FL_SHOW_BOARD_BODY ) );
}


void EDA_3D_FRAME::Process_Zoom( wxCommandEvent& event )
{
    int ii;

    switch( event.GetId() )
    {
    case ID_ZOOM_PAGE:

        for( ii = 0; ii < 4; ii++ )
            GetPrm3DVisu().m_Rot[ii] = 0.0;

        GetPrm3DVisu().m_Zoom = 1.0;
        m_canvas->SetOffset( 0.0, 0.0 );
        trackball( GetPrm3DVisu().m_Quat, 0.0, 0.0, 0.0, 0.0 );
        break;

    case ID_ZOOM_IN:
        GetPrm3DVisu().m_Zoom /= 1.2;

        if( GetPrm3DVisu().m_Zoom <= 0.01 )
            GetPrm3DVisu().m_Zoom = 0.01;

        break;

    case ID_ZOOM_OUT:
        GetPrm3DVisu().m_Zoom *= 1.2;
        break;

    case ID_ZOOM_REDRAW:
        break;

    default:
        return;
    }

    m_canvas->Refresh( false );
    m_canvas->DisplayStatus();
}


void EDA_3D_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


void EDA_3D_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
}


double EDA_3D_FRAME::BestZoom()
{
    return 1.0;
}


void EDA_3D_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
}


void EDA_3D_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
#define ROT_ANGLE 10.0
    int     id = event.GetId();
    bool    isChecked = event.IsChecked();

    switch( id )
    {
    case ID_TOOL_SET_VISIBLE_ITEMS:
        Install_3D_ViewOptionDialog( event );
        break;

    case ID_RELOAD3D_BOARD:
        m_reloadRequest = true;
        NewDisplay();
        return;
        break;

    case ID_ROTATE3D_X_POS:
        GetPrm3DVisu().m_ROTX += ROT_ANGLE;
        break;

    case ID_ROTATE3D_X_NEG:
        GetPrm3DVisu().m_ROTX -= ROT_ANGLE;
        break;

    case ID_ROTATE3D_Y_POS:
        GetPrm3DVisu().m_ROTY += ROT_ANGLE;
        break;

    case ID_ROTATE3D_Y_NEG:
        GetPrm3DVisu().m_ROTY -= ROT_ANGLE;
        break;

    case ID_ROTATE3D_Z_POS:
        GetPrm3DVisu().m_ROTZ += ROT_ANGLE;
        break;

    case ID_ROTATE3D_Z_NEG:
        GetPrm3DVisu().m_ROTZ -= ROT_ANGLE;
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
        ToggleOrtho();
        return;

    case ID_TOOL_SCREENCOPY_TOCLIBBOARD:
    case ID_MENU_SCREENCOPY_PNG:
    case ID_MENU_SCREENCOPY_JPEG:
        m_canvas->TakeScreenshot( event );
        break;

    case ID_MENU3D_BGCOLOR_BOTTOM_SELECTION:
        if( Set3DColorFromUser( GetPrm3DVisu().m_BgColor, _( "Background Color, Bottom" ) ) )
            m_canvas->Refresh( true );
        return;

    case ID_MENU3D_BGCOLOR_TOP_SELECTION:
        if( Set3DColorFromUser( GetPrm3DVisu().m_BgColor_Top, _( "Background Color, Top" ) ) )
            m_canvas->Refresh( true );
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

    case ID_MENU3D_REALISTIC_MODE:
        GetPrm3DVisu().SetFlag( FL_USE_REALISTIC_MODE, isChecked );
        GetMenuBar()->FindItem( ID_MENU3D_COMMENTS_ONOFF )->Enable( !isChecked );
        GetMenuBar()->FindItem( ID_MENU3D_ECO_ONOFF )->Enable( !isChecked );
        NewDisplay();
        return;

    case ID_MENU3D_FL_RENDER_SHADOWS:
        GetPrm3DVisu().SetFlag( FL_RENDER_SHADOWS, isChecked );
        NewDisplay();
        return;

    case ID_MENU3D_FL_RENDER_SHOW_HOLES_IN_ZONES:
        GetPrm3DVisu().SetFlag( FL_RENDER_SHOW_HOLES_IN_ZONES, isChecked );
        NewDisplay();
        return;

    case ID_MENU3D_FL_RENDER_TEXTURES:
        GetPrm3DVisu().SetFlag( FL_RENDER_TEXTURES, isChecked );
        NewDisplay(GL_ID_BOARD);
        NewDisplay(GL_ID_TECH_LAYERS);
        return;

    case ID_MENU3D_FL_RENDER_SMOOTH_NORMALS:
        GetPrm3DVisu().SetFlag( FL_RENDER_SMOOTH_NORMALS, isChecked );
        NewDisplay();
        return;

    case ID_MENU3D_FL_RENDER_USE_MODEL_NORMALS:
        GetPrm3DVisu().SetFlag( FL_RENDER_USE_MODEL_NORMALS, isChecked );
        NewDisplay();
        return;

    case ID_MENU3D_FL_RENDER_MATERIAL:
        GetPrm3DVisu().SetFlag( FL_RENDER_MATERIAL, isChecked );
        NewDisplay();
        return;

    case ID_MENU3D_FL_RENDER_SHOW_MODEL_BBOX:
        GetPrm3DVisu().SetFlag( FL_RENDER_SHOW_MODEL_BBOX, isChecked );
        NewDisplay();
        return;

    case ID_MENU3D_SHOW_BOARD_BODY:
        GetPrm3DVisu().SetFlag( FL_SHOW_BOARD_BODY, isChecked );
        NewDisplay();
        return;

    case ID_MENU3D_AXIS_ONOFF:
        GetPrm3DVisu().SetFlag( FL_AXIS, isChecked );
        m_canvas->Refresh();
        return;

    case ID_MENU3D_MODULE_ONOFF:
        GetPrm3DVisu().SetFlag( FL_MODULE, isChecked );
        m_canvas->Refresh();
        return;

    case ID_MENU3D_USE_COPPER_THICKNESS:
        GetPrm3DVisu().SetFlag( FL_USE_COPPER_THICKNESS, isChecked );
        NewDisplay( GL_ID_BOARD );
        NewDisplay( GL_ID_TECH_LAYERS );
        return;

    case ID_MENU3D_ZONE_ONOFF:
        GetPrm3DVisu().SetFlag( FL_ZONE, isChecked );
        NewDisplay( GL_ID_BOARD );
        return;

    case ID_MENU3D_ADHESIVE_ONOFF:
        GetPrm3DVisu().SetFlag( FL_ADHESIVE, isChecked );
        NewDisplay( GL_ID_TECH_LAYERS );
        return;

    case ID_MENU3D_SILKSCREEN_ONOFF:
        GetPrm3DVisu().SetFlag( FL_SILKSCREEN, isChecked );
        NewDisplay( GL_ID_TECH_LAYERS );
        return;

    case ID_MENU3D_SOLDER_MASK_ONOFF:
        GetPrm3DVisu().SetFlag( FL_SOLDERMASK, isChecked );
        NewDisplay( GL_ID_TECH_LAYERS );
        return;

    case ID_MENU3D_SOLDER_PASTE_ONOFF:
        GetPrm3DVisu().SetFlag( FL_SOLDERPASTE, isChecked );
        NewDisplay( GL_ID_TECH_LAYERS );
        return;

    case ID_MENU3D_COMMENTS_ONOFF:
        GetPrm3DVisu().SetFlag( FL_COMMENTS, isChecked );
        NewDisplay( GL_ID_AUX_LAYERS );
        return;

    case ID_MENU3D_ECO_ONOFF:
        GetPrm3DVisu().SetFlag( FL_ECO, isChecked );
        NewDisplay( GL_ID_AUX_LAYERS );
        return;

    default:
        wxLogMessage( wxT( "EDA_3D_FRAME::Process_Special_Functions() error: unknown command" ) );
        return;
    }

    m_canvas->Refresh( true );
    m_canvas->DisplayStatus();
}


void EDA_3D_FRAME::On3DGridSelection( wxCommandEvent& event )
{
    int id = event.GetId();

    for( int ii = ID_MENU3D_GRID_NOGRID; ii < ID_MENU3D_GRID_END; ii++ )
    {
        if( event.GetId() == ii )
            continue;

        GetMenuBar()->Check( ii, false );
    }

    switch( id )
    {
    case ID_MENU3D_GRID_NOGRID:
        GetPrm3DVisu().SetFlag( FL_GRID, false );
        break;

    case ID_MENU3D_GRID_10_MM:
        GetPrm3DVisu().SetFlag( FL_GRID, true );
        GetPrm3DVisu().m_3D_Grid = 10.0;
        break;

    case ID_MENU3D_GRID_5_MM:
        GetPrm3DVisu().SetFlag( FL_GRID, true );
        GetPrm3DVisu().m_3D_Grid = 5.0;
        break;

    case ID_MENU3D_GRID_2P5_MM:
        GetPrm3DVisu().SetFlag( FL_GRID, true );
        GetPrm3DVisu().m_3D_Grid = 2.5;
        break;

    case ID_MENU3D_GRID_1_MM:
        GetPrm3DVisu().SetFlag( FL_GRID, true );
        GetPrm3DVisu().m_3D_Grid = 1.0;
        break;

    default:
        wxLogMessage( wxT( "EDA_3D_FRAME::On3DGridSelection() error: unknown command" ) );
        return;
    }

    NewDisplay( GL_ID_GRID );
}


void EDA_3D_FRAME::NewDisplay( int aGlList )
{
    m_canvas->ClearLists( aGlList );

    // Rebuild the 3D board and refresh the view on reload request:
    if( m_reloadRequest )
        m_canvas->ReportWarnings( true );

    m_canvas->Refresh( true );

    m_canvas->DisplayStatus();
    m_reloadRequest = false;
}


void EDA_3D_FRAME::OnActivate( wxActivateEvent& event )
{
    // Reload data if 3D frame shows a board,
    // because it can be changed since last frame activation
    if( m_reloadRequest )
        NewDisplay();

    event.Skip();    // required under wxMAC
}


/* called to set the background color of the 3D scene
 */
bool EDA_3D_FRAME::Set3DColorFromUser( S3D_COLOR &aColor, const wxString& aTitle,
                                       wxColourData* aPredefinedColors )
{
    wxColour newcolor, oldcolor;

    oldcolor.Set( KiROUND( aColor.m_Red * 255 ),
                  KiROUND( aColor.m_Green * 255 ),
                  KiROUND( aColor.m_Blue * 255 ) );

    wxColourData emptyColorSet;     // Provides a empty predefined set of colors
                                    // if no color set available to avoid use of an
                                    // old color set

    if( aPredefinedColors == NULL )
        aPredefinedColors = &emptyColorSet;

    newcolor = wxGetColourFromUser( this, oldcolor, aTitle, aPredefinedColors );

    if( !newcolor.IsOk() )     // Cancel command
        return false;

    if( newcolor != oldcolor )
    {
        aColor.m_Red      = (double) newcolor.Red() / 255.0;
        aColor.m_Green    = (double) newcolor.Green() / 255.0;
        aColor.m_Blue     = (double) newcolor.Blue() / 255.0;
    }

    return true;
}

/* called to set the silkscreen color. Sets up a number of default colors
 */
bool EDA_3D_FRAME::Set3DSilkScreenColorFromUser()
{
    wxColourData definedColors;

    definedColors.SetCustomColour(0, wxColour( 241, 241, 241 ) );      // White
    definedColors.SetCustomColour(1, wxColour( 180, 180, 180 ) );     // Gray

    bool change = Set3DColorFromUser( GetPrm3DVisu().m_SilkScreenColor,
                                      _( "Silk Screen Color" ),
                                      &definedColors );

    if( change )
        NewDisplay( GL_ID_TECH_LAYERS );

    return change;
}


/* called to set the soldermask color. Sets up a number of default colors
 */
bool EDA_3D_FRAME::Set3DSolderMaskColorFromUser()
{
    wxColourData definedColors;

    definedColors.SetCustomColour(0, wxColour( 20,  51, 36 ) );   // Green
    definedColors.SetCustomColour(1, wxColour( 43,  10, 65 ) );   // Purple
    definedColors.SetCustomColour(2, wxColour( 117, 19, 21 ) );   // Red
    definedColors.SetCustomColour(3, wxColour( 54,  79, 116) );   // Light blue
    definedColors.SetCustomColour(4, wxColour( 11,  11, 11 ) );   // Black
    definedColors.SetCustomColour(5, wxColour( 241, 241,241) );   // White

    bool change = Set3DColorFromUser( GetPrm3DVisu().m_SolderMaskColor,
                                      _( "Solder Mask Color" ),
                                      &definedColors );

    if( change )
        NewDisplay( GL_ID_TECH_LAYERS );

    return change;
}


/* called to set the copper surface color. Sets up a number of default colors
 */
bool EDA_3D_FRAME::Set3DCopperColorFromUser()
{
    wxColourData definedColors;

    definedColors.SetCustomColour( 0, wxColour( 184, 115,  50  ) ); // Copper
    definedColors.SetCustomColour( 1, wxColour( 233, 221,  82 ) );  // Gold
    definedColors.SetCustomColour( 2, wxColour( 213, 213, 213) );   // Silver
    definedColors.SetCustomColour( 3, wxColour( 160, 160, 160) );   // tin


    bool change = Set3DColorFromUser( GetPrm3DVisu().m_CopperColor,
                                      _( "Copper Color" ),
                                      &definedColors );

    if( change )
        NewDisplay( GL_ID_BOARD );

    return change;
}


/* called to set the board body color. Sets up a number of default colors
 */
bool EDA_3D_FRAME::Set3DBoardBodyColorFromUser()
{
    wxColourData definedColors;

    definedColors.SetCustomColour( 0, wxColour(  51,  43, 22 ) );   // FR4 natural, dark
    definedColors.SetCustomColour( 1, wxColour( 109, 116, 75 ) );   // FR4 natural
    definedColors.SetCustomColour( 2, wxColour(  78,  14,  5 ) );   // brown/red
    definedColors.SetCustomColour( 3, wxColour( 146,  99, 47 ) );   // brown 1
    definedColors.SetCustomColour( 4, wxColour( 160, 123, 54 ) );   // brown 2
    definedColors.SetCustomColour( 5, wxColour( 146,  99, 47 ) );   // brown 3
    definedColors.SetCustomColour( 6, wxColour(  63, 126, 71 ) );   // green 1
    definedColors.SetCustomColour( 7, wxColour( 117, 122, 90 ) );   // green 2

    bool change = Set3DColorFromUser( GetPrm3DVisu().m_BoardBodyColor,
                                      _( "Board Body Color" ),
                                      &definedColors );

    if( change )
        NewDisplay( GL_ID_BOARD );

    return change;
}


/* called to set the solder paste layer color. Sets up a number of default colors
 */
bool EDA_3D_FRAME::Set3DSolderPasteColorFromUser()
{
    wxColourData definedColors;

    definedColors.SetCustomColour(0, wxColour( 128, 128, 128 ) );   // grey
    definedColors.SetCustomColour(1, wxColour( 213, 213, 213 ) );   // Silver
    definedColors.SetCustomColour(2, wxColour( 90,  90,  90  ) );   // grey 2

    bool change = Set3DColorFromUser( GetPrm3DVisu().m_SolderPasteColor,
                                      _( "Solder Paste Color" ),
                                      &definedColors );

    if( change )
        NewDisplay( GL_ID_TECH_LAYERS );

    return change;
}


BOARD* EDA_3D_FRAME::GetBoard()
{
    return Parent()->GetBoard();
}


INFO3D_VISU& EDA_3D_FRAME::GetPrm3DVisu() const
{
    // return the INFO3D_VISU which contains the current parameters
    // to draw the 3D view of the board
    return g_Parm_3D_Visu;
}

bool EDA_3D_FRAME::IsEnabled( DISPLAY3D_FLG aItem ) const
{
    // return true if aItem must be displayed
    return GetPrm3DVisu().GetFlag( aItem );
}
