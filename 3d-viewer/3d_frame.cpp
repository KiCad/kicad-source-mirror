/////////////////////////////////////////////////////////////////////////////
// Name:        3d_frame.cpp
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "appl_wxstruct.h"

#if !wxUSE_GLCANVAS
#error Please set wxUSE_GLCANVAS to 1 in setup.h.
#endif

#include "3d_viewer.h"
#include "trackball.h"

#include <wx/colordlg.h>
#include <wxstruct.h>

Info_3D_Visu g_Parm_3D_Visu;
double       g_Draw3d_dx;
double       g_Draw3d_dy;
double       ZBottom;
double       ZTop;
double       DataScale3D; // 3D conversion units.


BEGIN_EVENT_TABLE( EDA_3D_FRAME, wxFrame )
    EVT_ACTIVATE( EDA_3D_FRAME::OnActivate )
    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, EDA_3D_FRAME::Process_Zoom )
    EVT_TOOL_RANGE( ID_START_COMMAND_3D, ID_END_COMMAND_3D,
                    EDA_3D_FRAME::Process_Special_Functions )
    EVT_MENU( wxID_EXIT, EDA_3D_FRAME::Exit3DFrame )
    EVT_MENU( ID_MENU_SCREENCOPY_PNG, EDA_3D_FRAME::Process_Special_Functions )
    EVT_MENU( ID_MENU_SCREENCOPY_JPEG, EDA_3D_FRAME::Process_Special_Functions )
    EVT_CLOSE( EDA_3D_FRAME::OnCloseWindow )
END_EVENT_TABLE()


EDA_3D_FRAME::EDA_3D_FRAME( PCB_BASE_FRAME* parent, const wxString& title, long style ) :
    wxFrame( parent, DISPLAY3D_FRAME, title, wxPoint( -1, -1 ), wxSize( -1, -1 ), style )
{
    m_FrameName     = wxT( "Frame3D" );
    m_Canvas        = NULL;
    m_Parent        = parent;
    m_HToolBar      = NULL;
    m_VToolBar      = NULL;
    m_InternalUnits = 10000;    // Internal units = 1/10000 inch
    m_reloadRequest = false;

    // Give it an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( icon_3d_xpm ) );
    SetIcon( icon );

    GetSettings();
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    // Create the status line
    static const int dims[5] = { -1, 100, 100, 100, 140 };

    CreateStatusBar( 5 );
    SetStatusWidths( 5, dims );

    ReCreateMenuBar();
    ReCreateHToolbar();

    //	ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();

    // Make a Pcb3D_GLCanvas
    int attrs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, 0 };
    m_Canvas = new Pcb3D_GLCanvas( this, attrs );

    m_auimgr.SetManagedWindow( this );

    wxAuiPaneInfo horiz;
    horiz.Gripper( false );
    horiz.DockFixed( true );
    horiz.Movable( false );
    horiz.Floatable( false );
    horiz.CloseButton( false );
    horiz.CaptionVisible( false );

    wxAuiPaneInfo vert( horiz );

    vert.TopDockable( false ).BottomDockable( false );
    horiz.LeftDockable( false ).RightDockable( false );

    m_auimgr.AddPane( m_HToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top() );

    m_auimgr.AddPane( m_Canvas,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    m_auimgr.Update();

    // Fixes bug in Windows (XP and possibly others) where the canvas requires the focus
    // in order to receive mouse events.  Otherwise, the user has to click somewhere on
    // the canvas before it will respond to mouse wheel events.
    m_Canvas->SetFocus();
}


void EDA_3D_FRAME::Exit3DFrame( wxCommandEvent& event )
{
    Close( true );
}


void EDA_3D_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    SaveSettings();

    if( m_Parent )
    {
        m_Parent->m_Draw3DFrame = NULL;
    }

    Destroy();
}


void EDA_3D_FRAME::GetSettings()
{
    wxString  text;
    wxConfig* config = wxGetApp().m_EDA_Config;  // Current config used by application

    if( config )
    {
        text = m_FrameName + wxT( "Pos_x" );
        config->Read( text, &m_FramePos.x );
        text = m_FrameName + wxT( "Pos_y" );
        config->Read( text, &m_FramePos.y );
        text = m_FrameName + wxT( "Size_x" );
        config->Read( text, &m_FrameSize.x, 600 );
        text = m_FrameName + wxT( "Size_y" );
        config->Read( text, &m_FrameSize.y, 400 );
        config->Read( wxT( "BgColor_Red" ), &g_Parm_3D_Visu.m_BgColor.m_Red, 0.0 );
        config->Read( wxT( "BgColor_Green" ), &g_Parm_3D_Visu.m_BgColor.m_Green, 0.0 );
        config->Read( wxT( "BgColor_Blue" ), &g_Parm_3D_Visu.m_BgColor.m_Blue, 0.0 );
    }
#if defined( __WXMAC__ )
    // for macOSX, the window must be below system (macOSX) toolbar
    if( m_FramePos.y < 20 )
        m_FramePos.y = 20;
#endif
}


void EDA_3D_FRAME::SaveSettings()
{
    wxString  text;
    wxConfig* Config = wxGetApp().m_EDA_Config;  //  Current config used by application

    if( !Config )
        return;

    Config->Write( wxT( "BgColor_Red" ), g_Parm_3D_Visu.m_BgColor.m_Red );
    Config->Write( wxT( "BgColor_Green" ), g_Parm_3D_Visu.m_BgColor.m_Green );
    Config->Write( wxT( "BgColor_Blue" ), g_Parm_3D_Visu.m_BgColor.m_Blue );

    if( IsIconized() )
        return;

    m_FrameSize = GetSize();
    m_FramePos  = GetPosition();

    text = m_FrameName + wxT( "Pos_x" );
    Config->Write( text, (long) m_FramePos.x );
    text = m_FrameName + wxT( "Pos_y" );
    Config->Write( text, (long) m_FramePos.y );
    text = m_FrameName + wxT( "Size_x" );
    Config->Write( text, (long) m_FrameSize.x );
    text = m_FrameName + wxT( "Size_y" );
    Config->Write( text, (long) m_FrameSize.y );
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
        g_Draw3d_dx = g_Draw3d_dy = 0;
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

    m_Canvas->Refresh( false );
    m_Canvas->DisplayStatus();
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

    switch( event.GetId() )
    {
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
        m_Canvas->SetView3D( WXK_LEFT );
        return;

    case ID_MOVE3D_RIGHT:
        m_Canvas->SetView3D( WXK_RIGHT );
        return;

    case ID_MOVE3D_UP:
        m_Canvas->SetView3D( WXK_UP );
        return;

    case ID_MOVE3D_DOWN:
        m_Canvas->SetView3D( WXK_DOWN );
        return;

    case ID_ORTHO:
        m_Canvas->ToggleOrtho();
        return;

    case ID_TOOL_SCREENCOPY_TOCLIBBOARD:
    case ID_MENU_SCREENCOPY_PNG:
    case ID_MENU_SCREENCOPY_JPEG:
        m_Canvas->TakeScreenshot( event );
        break;

    case ID_MENU3D_BGCOLOR_SELECTION:
        Set3DBgColor();
        return;

    case ID_MENU3D_AXIS_ONOFF:
        Set3DAxisOnOff();
        return;

    case ID_MENU3D_MODULE_ONOFF:
        Set3DModuleOnOff();
        return;

    case ID_MENU3D_ZONE_ONOFF:
        Set3DZoneOnOff();
        return;

    case ID_MENU3D_COMMENTS_ONOFF:
        Set3DCommentsOnOff();
        return;

    case ID_MENU3D_DRAWINGS_ONOFF:
        Set3DDrawingsOnOff();
        return;

    case ID_MENU3D_ECO1_ONOFF:
        Set3DEco1OnOff();
        return;

    case ID_MENU3D_ECO2_ONOFF:
        Set3DEco2OnOff();
        return;

    default:
        wxMessageBox( wxT( "EDA_3D_FRAME::Process_Special_Functions() error: unknown command" ) );
        return;
    }

    m_Canvas->Refresh( true );
    m_Canvas->DisplayStatus();
}


void EDA_3D_FRAME::NewDisplay()
{
    m_reloadRequest = false;

    m_Canvas->ClearLists();
    m_Canvas->CreateDrawGL_List();

//    m_Canvas->InitGL();
    m_Canvas->Refresh( true );
    m_Canvas->DisplayStatus();
}


void EDA_3D_FRAME::OnActivate( wxActivateEvent& event )
{
    // Reload data if 3D frame shows a footprint,
    // because it can be changed since last frame activation
    if( m_reloadRequest )
        NewDisplay();

    event.Skip();   // required under wxMAC
}


/* called to set the background color of the 3D scene
 */
void EDA_3D_FRAME::Set3DBgColor()
{
    S3D_Color color;
    wxColour  newcolor, oldcolor;

    oldcolor.Set( wxRound( g_Parm_3D_Visu.m_BgColor.m_Red * 255 ),
                  wxRound( g_Parm_3D_Visu.m_BgColor.m_Green * 255 ),
                  wxRound( g_Parm_3D_Visu.m_BgColor.m_Blue * 255 ) );

    newcolor = wxGetColourFromUser( this, oldcolor );

    if( newcolor != oldcolor )
    {
        g_Parm_3D_Visu.m_BgColor.m_Red   = (double) newcolor.Red() / 255.0;
        g_Parm_3D_Visu.m_BgColor.m_Green = (double) newcolor.Green() / 255.0;
        g_Parm_3D_Visu.m_BgColor.m_Blue  = (double) newcolor.Blue() / 255.0;
        NewDisplay();
    }
}


void EDA_3D_FRAME::Set3DAxisOnOff()
{
    if( g_Parm_3D_Visu.m_Draw3DAxis )
        g_Parm_3D_Visu.m_Draw3DAxis = false;
    else
        g_Parm_3D_Visu.m_Draw3DAxis = true;

    NewDisplay();
}


void EDA_3D_FRAME::Set3DModuleOnOff()
{
    if( g_Parm_3D_Visu.m_Draw3DModule )
        g_Parm_3D_Visu.m_Draw3DModule = false;
    else
        g_Parm_3D_Visu.m_Draw3DModule = true;

    NewDisplay();
}


void EDA_3D_FRAME::Set3DZoneOnOff()
{
    if( g_Parm_3D_Visu.m_Draw3DZone )
        g_Parm_3D_Visu.m_Draw3DZone = false;
    else
        g_Parm_3D_Visu.m_Draw3DZone = true;
    NewDisplay();
}


void EDA_3D_FRAME::Set3DCommentsOnOff()
{
    if( g_Parm_3D_Visu.m_Draw3DComments )
        g_Parm_3D_Visu.m_Draw3DComments = false;
    else
        g_Parm_3D_Visu.m_Draw3DComments = true;

    NewDisplay();
}


void EDA_3D_FRAME::Set3DDrawingsOnOff()
{
    if( g_Parm_3D_Visu.m_Draw3DDrawings )
        g_Parm_3D_Visu.m_Draw3DDrawings = false;
    else
        g_Parm_3D_Visu.m_Draw3DDrawings = true;

    NewDisplay();
}


void EDA_3D_FRAME::Set3DEco1OnOff()
{
    if( g_Parm_3D_Visu.m_Draw3DEco1 )
        g_Parm_3D_Visu.m_Draw3DEco1 = false;
    else
        g_Parm_3D_Visu.m_Draw3DEco1 = true;

    NewDisplay();
}


void EDA_3D_FRAME::Set3DEco2OnOff()
{
    if( g_Parm_3D_Visu.m_Draw3DEco2 )
        g_Parm_3D_Visu.m_Draw3DEco2 = false;
    else
        g_Parm_3D_Visu.m_Draw3DEco2 = true;

    NewDisplay();
}
