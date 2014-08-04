/**
 * @file 3d_frame.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
static const wxChar keyShowRealisticMode[] =    wxT( "ShowRealisticMode" );
static const wxChar keyUseHQinRealisticMode[] = wxT( "UseHQinRealisticMode" );
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
            wxDefaultPosition, wxDefaultSize, style, wxT( "Frame3D" ) )
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
    static const int dims[5] = { -1, 100, 100, 100, 140 };

    CreateStatusBar( 5 );
    SetStatusWidths( 5, dims );

    CreateMenuBar();
    ReCreateMainToolbar();

    // Make a EDA_3D_CANVAS
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

    unsigned int ii;

    // Check if the canvas supports multisampling.
    if( EDA_3D_CANVAS::IsDisplaySupported( attrs ) )
    {
        // Check for possible sample sizes, start form the top.
        int maxSamples = 8; // Any higher doesn't change anything.
        int samplesOffset = 0;

        for( ii = 0; ii < sizeof( attrs ) / sizeof( attrs[0] ) - 1; ii += 2 )
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
        for( ii = 0; ii < sizeof( attrs ) / sizeof( attrs[0] ) - 1; ii += 2 )
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


void EDA_3D_FRAME::Exit3DFrame( wxCommandEvent& event )
{
    Close( true );
}


void EDA_3D_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    if( Parent() )
        Parent()->m_Draw3DFrame = NULL;

    Destroy();
}


void EDA_3D_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    INFO3D_VISU& prms = g_Parm_3D_Visu;

    aCfg->Read( keyBgColor_Red, &g_Parm_3D_Visu.m_BgColor.m_Red, 0.0 );
    aCfg->Read( keyBgColor_Green, &g_Parm_3D_Visu.m_BgColor.m_Green, 0.0 );
    aCfg->Read( keyBgColor_Blue, &g_Parm_3D_Visu.m_BgColor.m_Blue, 0.0 );

    bool tmp;
    aCfg->Read( keyShowRealisticMode, &tmp, false );
    prms.SetFlag( FL_USE_REALISTIC_MODE, tmp );

    aCfg->Read( keyUseHQinRealisticMode, &tmp, false );
    prms.SetFlag( FL_USE_MAXQUALITY_IN_REALISTIC_MODE, tmp );

    aCfg->Read( keyShowAxis, &tmp, true );
    prms.SetFlag( FL_AXIS, tmp );

    aCfg->Read( keyShowGrid, &tmp, true );
    prms.SetFlag( FL_GRID, tmp );

    aCfg->Read( keyShowGridSize, &prms.m_3D_Grid, 10.0 );
    prms.SetFlag( FL_MODULE, tmp );

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

    INFO3D_VISU& prms = g_Parm_3D_Visu;

    aCfg->Write( keyBgColor_Red, g_Parm_3D_Visu.m_BgColor.m_Red );
    aCfg->Write( keyBgColor_Green, g_Parm_3D_Visu.m_BgColor.m_Green );
    aCfg->Write( keyBgColor_Blue, g_Parm_3D_Visu.m_BgColor.m_Blue );
    aCfg->Write( keyShowRealisticMode, prms.GetFlag( FL_USE_REALISTIC_MODE )  );
    aCfg->Write( keyUseHQinRealisticMode, prms.GetFlag( FL_USE_MAXQUALITY_IN_REALISTIC_MODE )  );
    aCfg->Write( keyShowAxis, prms.GetFlag( FL_AXIS )  );
    aCfg->Write( keyShowGrid, prms.GetFlag( FL_GRID )  );
    aCfg->Write( keyShowGridSize, prms.m_3D_Grid  );
    aCfg->Write( keyShowFootprints, prms.GetFlag( FL_MODULE )  );
    aCfg->Write( keyShowCopperThickness, prms.GetFlag( FL_USE_COPPER_THICKNESS )  );
    aCfg->Write( keyShowZones, prms.GetFlag( FL_ZONE )  );
    aCfg->Write( keyShowAdhesiveLayers, prms.GetFlag( FL_ADHESIVE )  );
    aCfg->Write( keyShowSilkScreenLayers, prms.GetFlag( FL_SILKSCREEN )  );
    aCfg->Write( keyShowSolderMaskLayers, prms.GetFlag( FL_SOLDERMASK )  );
    aCfg->Write( keyShowSolderPasteLayers, prms.GetFlag( FL_SOLDERPASTE )  );
    aCfg->Write( keyShowCommentsLayer, prms.GetFlag( FL_COMMENTS )  );
    aCfg->Write( keyShowEcoLayers, prms.GetFlag( FL_ECO )  );
    aCfg->Write( keyShowBoardBody, prms.GetFlag( FL_SHOW_BOARD_BODY )  );
}


void EDA_3D_FRAME::Process_Zoom( wxCommandEvent& event )
{
    int ii;

    switch( event.GetId() )
    {
    case ID_ZOOM_PAGE:

        for( ii = 0; ii < 4; ii++ )
            g_Parm_3D_Visu.m_Rot[ii] = 0.0;

        g_Parm_3D_Visu.m_Zoom = 1.0;
        m_canvas->SetOffset( 0.0, 0.0 );
        trackball( g_Parm_3D_Visu.m_Quat, 0.0, 0.0, 0.0, 0.0 );
        break;

    case ID_ZOOM_IN:
        g_Parm_3D_Visu.m_Zoom /= 1.2;

        if( g_Parm_3D_Visu.m_Zoom <= 0.01 )
            g_Parm_3D_Visu.m_Zoom = 0.01;

        break;

    case ID_ZOOM_OUT:
        g_Parm_3D_Visu.m_Zoom *= 1.2;
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
        NewDisplay();
        return;
        break;

    case ID_ROTATE3D_X_POS:
        g_Parm_3D_Visu.m_ROTX += ROT_ANGLE;
        break;

    case ID_ROTATE3D_X_NEG:
        g_Parm_3D_Visu.m_ROTX -= ROT_ANGLE;
        break;

    case ID_ROTATE3D_Y_POS:
        g_Parm_3D_Visu.m_ROTY += ROT_ANGLE;
        break;

    case ID_ROTATE3D_Y_NEG:
        g_Parm_3D_Visu.m_ROTY -= ROT_ANGLE;
        break;

    case ID_ROTATE3D_Z_POS:
        g_Parm_3D_Visu.m_ROTZ += ROT_ANGLE;
        break;

    case ID_ROTATE3D_Z_NEG:
        g_Parm_3D_Visu.m_ROTZ -= ROT_ANGLE;
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

    case ID_MENU3D_BGCOLOR_SELECTION:
        Set3DBgColor();
        return;

    case ID_MENU3D_REALISTIC_MODE:
        g_Parm_3D_Visu.SetFlag( FL_USE_REALISTIC_MODE, isChecked );
        NewDisplay();
        return;

    case ID_MENU3D_MAX_QUALITY_FOR_REALISTIC_MODE:
        g_Parm_3D_Visu.SetFlag( FL_USE_MAXQUALITY_IN_REALISTIC_MODE, isChecked );
        NewDisplay();
        return;

    case ID_MENU3D_SHOW_BOARD_BODY:
        g_Parm_3D_Visu.SetFlag( FL_SHOW_BOARD_BODY, isChecked );
        NewDisplay();
        return;

    case ID_MENU3D_AXIS_ONOFF:
        g_Parm_3D_Visu.SetFlag( FL_AXIS, isChecked );
        m_canvas->Refresh();
        return;

    case ID_MENU3D_MODULE_ONOFF:
        g_Parm_3D_Visu.SetFlag( FL_MODULE, isChecked );
        m_canvas->Refresh();
        return;

    case ID_MENU3D_USE_COPPER_THICKNESS:
        g_Parm_3D_Visu.SetFlag( FL_USE_COPPER_THICKNESS, isChecked );
        NewDisplay(GL_ID_BOARD);
        return;

    case ID_MENU3D_ZONE_ONOFF:
        g_Parm_3D_Visu.SetFlag( FL_ZONE, isChecked );
        NewDisplay(GL_ID_BOARD);
        return;

    case ID_MENU3D_ADHESIVE_ONOFF:
        g_Parm_3D_Visu.SetFlag( FL_ADHESIVE, isChecked );
        NewDisplay(GL_ID_TECH_LAYERS);
        return;

    case ID_MENU3D_SILKSCREEN_ONOFF:
        g_Parm_3D_Visu.SetFlag( FL_SILKSCREEN, isChecked );
        NewDisplay(GL_ID_TECH_LAYERS);
        return;

    case ID_MENU3D_SOLDER_MASK_ONOFF:
        g_Parm_3D_Visu.SetFlag( FL_SOLDERMASK, isChecked );
        NewDisplay(GL_ID_TECH_LAYERS);
        return;

    case ID_MENU3D_SOLDER_PASTE_ONOFF:
        g_Parm_3D_Visu.SetFlag( FL_SOLDERPASTE, isChecked );
        NewDisplay(GL_ID_TECH_LAYERS);
        return;

    case ID_MENU3D_COMMENTS_ONOFF:
        g_Parm_3D_Visu.SetFlag( FL_COMMENTS, isChecked );
        NewDisplay(GL_ID_AUX_LAYERS);
        return;

    case ID_MENU3D_ECO_ONOFF:
        g_Parm_3D_Visu.SetFlag( FL_ECO, isChecked );
        NewDisplay(GL_ID_AUX_LAYERS);
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

    for( int ii = ID_MENU3D_GRID; ii < ID_MENU3D_GRID_END; ii++ )
    {
        if( event.GetId() == ii )
            continue;

        GetMenuBar()->Check( ii, false );
    }

    switch( id )
    {
    case ID_MENU3D_GRID_NOGRID:
        g_Parm_3D_Visu.SetFlag( FL_GRID, false );
        break;

    case ID_MENU3D_GRID_10_MM:
        g_Parm_3D_Visu.SetFlag( FL_GRID, true );
        g_Parm_3D_Visu.m_3D_Grid = 10.0;
        break;

    case ID_MENU3D_GRID_5_MM:
        g_Parm_3D_Visu.SetFlag( FL_GRID, true );
        g_Parm_3D_Visu.m_3D_Grid = 5.0;
        break;

    case ID_MENU3D_GRID_2P5_MM:
        g_Parm_3D_Visu.SetFlag( FL_GRID, true );
        g_Parm_3D_Visu.m_3D_Grid = 2.5;
        break;

    case ID_MENU3D_GRID_1_MM:
        g_Parm_3D_Visu.SetFlag( FL_GRID, true );
        g_Parm_3D_Visu.m_3D_Grid = 1.0;
        break;

    default:
        wxLogMessage( wxT( "EDA_3D_FRAME::On3DGridSelection() error: unknown command" ) );
        return;
    }

    NewDisplay( GL_ID_GRID );
}


void EDA_3D_FRAME::NewDisplay( int aGlList )
{
    m_reloadRequest = false;

    m_canvas->ClearLists( aGlList );
    m_canvas->CreateDrawGL_List();

    m_canvas->Refresh( true );
    m_canvas->DisplayStatus();
}


void EDA_3D_FRAME::OnActivate( wxActivateEvent& event )
{
    // Reload data if 3D frame shows a footprint,
    // because it can be changed since last frame activation
    if( m_reloadRequest )
        NewDisplay();

    event.Skip();    // required under wxMAC
}


/* called to set the background color of the 3D scene
 */
void EDA_3D_FRAME::Set3DBgColor()
{
    S3D_COLOR   color;
    wxColour    newcolor, oldcolor;

    oldcolor.Set( KiROUND( g_Parm_3D_Visu.m_BgColor.m_Red * 255 ),
                  KiROUND( g_Parm_3D_Visu.m_BgColor.m_Green * 255 ),
                  KiROUND( g_Parm_3D_Visu.m_BgColor.m_Blue * 255 ) );

    newcolor = wxGetColourFromUser( this, oldcolor );

    if( !newcolor.IsOk() )     // Cancel command
        return;

    if( newcolor != oldcolor )
    {
        g_Parm_3D_Visu.m_BgColor.m_Red      = (double) newcolor.Red() / 255.0;
        g_Parm_3D_Visu.m_BgColor.m_Green    = (double) newcolor.Green() / 255.0;
        g_Parm_3D_Visu.m_BgColor.m_Blue     = (double) newcolor.Blue() / 255.0;
        m_canvas->Redraw();
    }
}

BOARD* EDA_3D_FRAME::GetBoard()
{
    return Parent()->GetBoard();
}
