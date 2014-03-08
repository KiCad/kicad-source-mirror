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
#include <appl_wxstruct.h>

#include <3d_viewer.h>
#include <3d_canvas.h>
#include <info3d_visu.h>
#include <trackball.h>

#include <wx/colordlg.h>
#include <3d_viewer_id.h>
#include <wxBasePcbFrame.h>

INFO3D_VISU             g_Parm_3D_Visu;

// Key to store 3D Viewer config:
static const wxString   keyBgColor_Red( wxT( "BgColor_Red" ) );
static const wxString   keyBgColor_Green( wxT( "BgColor_Green" ) );
static const wxString   keyBgColor_Blue( wxT( "BgColor_Blue" ) );
static const wxString   keyShowRealisticMode( wxT( "ShowRealisticMode" ) );
static const wxString   keyShowAxis( wxT( "ShowAxis" ) );
static const wxString   keyShowGrid( wxT( "ShowGrid3D" ) );
static const wxString   keyShowGridSize( wxT( "Grid3DSize" ) );
static const wxString   keyShowZones( wxT( "ShowZones" ) );
static const wxString   keyShowFootprints( wxT( "ShowFootprints" ) );
static const wxString   keyShowCopperThickness( wxT( "ShowCopperThickness" ) );
static const wxString   keyShowAdhesiveLayers( wxT( "ShowAdhesiveLayers" ) );
static const wxString   keyShowSilkScreenLayers( wxT( "ShowSilkScreenLayers" ) );
static const wxString   keyShowSolderMaskLayers( wxT( "ShowSolderMasLayers" ) );
static const wxString   keyShowSolderPasteLayers( wxT( "ShowSolderPasteLayers" ) );
static const wxString   keyShowCommentsLayer( wxT( "ShowCommentsLayers" ) );
static const wxString   keyShowBoardBody( wxT( "ShowBoardBody" ) );
static const wxString   keyShowEcoLayers( wxT( "ShowEcoLayers" ) );

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

END_EVENT_TABLE() EDA_3D_FRAME::EDA_3D_FRAME( PCB_BASE_FRAME*   parent,
                                              const wxString&   title,
                                              long              style ) :
    EDA_BASE_FRAME( parent, DISPLAY3D_FRAME_TYPE, title,
                    wxDefaultPosition, wxDefaultSize, style, wxT( "Frame3D" ) )
{
    m_canvas        = NULL;
    m_reloadRequest = false;
    m_ortho         = false;

    // Give it an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_3d_xpm ) );
    SetIcon( icon );

    GetSettings();
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Create the status line
    static const int dims[5] = { -1, 100, 100, 100, 140 };

    CreateStatusBar( 5 );
    SetStatusWidths( 5, dims );

    CreateMenuBar();
    ReCreateMainToolbar();

    // Make a EDA_3D_CANVAS
    int attrs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, 0 };
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


void EDA_3D_FRAME::GetSettings()
{
    wxConfig*   config = wxGetApp().GetSettings(); // Current config used by application
    class INFO3D_VISU& prms = g_Parm_3D_Visu;

    if( config )
    {
        EDA_BASE_FRAME::LoadSettings();

        config->Read( keyBgColor_Red, &prms.m_BgColor.m_Red, 0.0 );
        config->Read( keyBgColor_Green, &prms.m_BgColor.m_Green, 0.0 );
        config->Read( keyBgColor_Blue, &prms.m_BgColor.m_Blue, 0.0 );

        bool tmp;
        config->Read( keyShowRealisticMode, &tmp, false );
        prms.SetFlag( FL_USE_REALISTIC_MODE, tmp );

        config->Read( keyShowAxis, &tmp, true );
        prms.SetFlag( FL_AXIS, tmp );

        config->Read( keyShowGrid, &tmp, true );
        prms.SetFlag( FL_GRID, tmp );

        config->Read( keyShowGridSize, &prms.m_3D_Grid, 10.0 );
        prms.SetFlag( FL_MODULE, tmp );

        config->Read( keyShowFootprints, &tmp, true );
        prms.SetFlag( FL_MODULE, tmp );

        config->Read( keyShowCopperThickness, &tmp, false );
        prms.SetFlag( FL_USE_COPPER_THICKNESS, tmp );

        config->Read( keyShowZones, &tmp, true );
        prms.SetFlag( FL_ZONE, tmp );

        config->Read( keyShowAdhesiveLayers, &tmp, true );
        prms.SetFlag( FL_ADHESIVE, tmp );

        config->Read( keyShowSilkScreenLayers, &tmp, true );
        prms.SetFlag( FL_SILKSCREEN, tmp );

        config->Read( keyShowSolderMaskLayers, &tmp, true );
        prms.SetFlag( FL_SOLDERMASK, tmp );

        config->Read( keyShowSolderPasteLayers, &tmp, true );
        prms.SetFlag( FL_SOLDERPASTE, tmp );

        config->Read( keyShowCommentsLayer, &tmp, true );
        prms.SetFlag( FL_COMMENTS, tmp );

        config->Read( keyShowEcoLayers, &tmp, true );
        prms.SetFlag( FL_ECO, tmp );

        config->Read( keyShowBoardBody, &tmp, true );
        prms.SetFlag( FL_SHOW_BOARD_BODY, tmp );
    }
}


void EDA_3D_FRAME::SaveSettings()
{
    wxConfig* config = wxGetApp().GetSettings(); // Current config used by application

    if( !config )
        return;

    EDA_BASE_FRAME::SaveSettings();

    class INFO3D_VISU& prms = g_Parm_3D_Visu;
    config->Write( keyBgColor_Red, prms.m_BgColor.m_Red );
    config->Write( keyBgColor_Green, prms.m_BgColor.m_Green );
    config->Write( keyBgColor_Blue, prms.m_BgColor.m_Blue );
    config->Write( keyShowRealisticMode, prms.GetFlag( FL_USE_REALISTIC_MODE )  );
    config->Write( keyShowAxis, prms.GetFlag( FL_AXIS )  );
    config->Write( keyShowGrid, prms.GetFlag( FL_GRID )  );
    config->Write( keyShowGridSize, prms.m_3D_Grid  );
    config->Write( keyShowFootprints, prms.GetFlag( FL_MODULE )  );
    config->Write( keyShowCopperThickness, prms.GetFlag( FL_USE_COPPER_THICKNESS )  );
    config->Write( keyShowZones, prms.GetFlag( FL_ZONE )  );
    config->Write( keyShowAdhesiveLayers, prms.GetFlag( FL_ADHESIVE )  );
    config->Write( keyShowSilkScreenLayers, prms.GetFlag( FL_SILKSCREEN )  );
    config->Write( keyShowSolderMaskLayers, prms.GetFlag( FL_SOLDERMASK )  );
    config->Write( keyShowSolderPasteLayers, prms.GetFlag( FL_SOLDERPASTE )  );
    config->Write( keyShowCommentsLayer, prms.GetFlag( FL_COMMENTS )  );
    config->Write( keyShowEcoLayers, prms.GetFlag( FL_ECO )  );
    config->Write( keyShowBoardBody, prms.GetFlag( FL_SHOW_BOARD_BODY )  );
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


void EDA_3D_FRAME::NewDisplay( GLuint aGlList )
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

    if( ! newcolor.IsOk() )     // Happens on cancel dialog
        return;

    if( newcolor != oldcolor )
    {
        g_Parm_3D_Visu.m_BgColor.m_Red = (double) newcolor.Red() / 255.0;
        g_Parm_3D_Visu.m_BgColor.m_Green    = (double) newcolor.Green() / 255.0;
        g_Parm_3D_Visu.m_BgColor.m_Blue     = (double) newcolor.Blue() / 255.0;
        NewDisplay();
    }
}

BOARD* EDA_3D_FRAME::GetBoard()
{
    return Parent()->GetBoard();
}
