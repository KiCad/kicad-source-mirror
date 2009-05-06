/*********************************************************************/
/** setvisu() : initialisations de l'ecran d'affichage du composant **/
/*********************************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "id.h"
#include "confirm.h"
#include "macros.h"

#include "cvpcb.h"
#include "bitmaps.h"
#include "protos.h"
#include "cvstruct.h"

/*
 * NOTE: There is something in 3d_viewer.h that causes a compiler error in
 *       <boost/foreach.hpp> in Linux so move it after cvpcb.h where it is
 *       included to prevent the error from occuring.
 */
#include "3d_viewer.h"



/*****************************************************************/
/* Construction de la table des evenements pour WinEDA_DrawFrame */
/*****************************************************************/

BEGIN_EVENT_TABLE( WinEDA_DisplayFrame, WinEDA_BasePcbFrame )
    EVT_CLOSE( WinEDA_DisplayFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_DrawFrame::OnSize )
    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, WinEDA_DisplayFrame::OnZoom )
    EVT_TOOL( ID_OPTIONS_SETUP, WinEDA_DisplayFrame::InstallOptionsDisplay )
    EVT_TOOL( ID_CVPCB_SHOW3D_FRAME, WinEDA_DisplayFrame::Show3D_Frame )
END_EVENT_TABLE()


/***************************************************************************/
/* WinEDA_DisplayFrame: the frame to display the current focused footprint */
/***************************************************************************/

WinEDA_DisplayFrame::WinEDA_DisplayFrame( WinEDA_CvpcbFrame* father,
                                          const wxString& title,
                                          const wxPoint& pos,
                                          const wxSize& size, long style ) :
    WinEDA_BasePcbFrame( father, CVPCB_DISPLAY_FRAME, title, pos,
                         size, style )
{
    m_FrameName = wxT( "CmpFrame" );

    // Give an icon
    #ifdef __WINDOWS__
    SetIcon( wxICON( a_icon_cvpcb ) );
    #else
    SetIcon( wxICON( icon_cvpcb ) );
    #endif
    SetTitle( title );

    SetBoard( new BOARD( NULL, this ) );
    SetBaseScreen( new PCB_SCREEN() );

    LoadSettings();
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    ReCreateHToolbar();
    ReCreateVToolbar();
    Show( TRUE );
}


/******************************************/
WinEDA_DisplayFrame::~WinEDA_DisplayFrame()
/******************************************/
{
    delete GetBaseScreen();
    SetBaseScreen( 0 );

    delete GetBoard();

    ( (WinEDA_CvpcbFrame*) wxGetApp().GetTopWindow() )->DrawFrame = NULL;
}


/************************************************************/
void WinEDA_DisplayFrame::OnCloseWindow( wxCloseEvent& event )
/************************************************************/

/* Called when the frame is closed
 *  Save current settings (frame position and size
 */
{
    wxPoint pos;
    wxSize  size;

    size = GetSize();
    pos  = GetPosition();

    SaveSettings();
    Destroy();
}


/************************************************/
void WinEDA_DisplayFrame::ReCreateVToolbar()
/************************************************/
{
}


/************************************************/
void WinEDA_DisplayFrame::ReCreateHToolbar()
/************************************************/
{
    if( m_HToolBar != NULL )
        return;

    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );

    SetToolBar( m_HToolBar );

    m_HToolBar->AddTool( ID_OPTIONS_SETUP, wxEmptyString,
                         wxBitmap( display_options_xpm ),
                         _( "Display Options" ) );

    m_HToolBar->AddSeparator();

    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                         wxBitmap( zoom_in_xpm ),
                         _( "zoom + (F1)" ) );

    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                         wxBitmap( zoom_out_xpm ),
                         _( "zoom - (F2)" ) );

    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                         wxBitmap( zoom_redraw_xpm ),
                         _( "redraw (F3)" ) );

    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                         wxBitmap( zoom_auto_xpm ),
                         _( "1:1 zoom" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_SHOW3D_FRAME, wxEmptyString,
                         wxBitmap( show_3d_xpm ),
                         _( "1:1 zoom" ) );

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_HToolBar->Realize();
}


/*******************************************/
void WinEDA_DisplayFrame::SetToolbars()
/*******************************************/
{
}


/*************************************************************************/
void WinEDA_DisplayFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
/*************************************************************************/
{
}


/*************************************************************************/
void WinEDA_DisplayFrame::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
/*************************************************************************/
{
}


/*********************************************************************************/
bool WinEDA_DisplayFrame::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
/*********************************************************************************/
{
    return true;
}


/****************************************************************/
void WinEDA_DisplayFrame::GeneralControle( wxDC* DC, wxPoint Mouse )
/****************************************************************/
{
    wxRealPoint  delta;
    int     flagcurseur = 0;
    wxPoint curpos, oldpos;
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    curpos = DrawPanel->CursorRealPosition( Mouse );
    oldpos = GetScreen()->m_Curseur;

    delta = GetScreen()->GetGrid();
    GetScreen()->Scale( delta );

    if( delta.x <= 0 )
        delta.x = 1;
    if( delta.y <= 0 )
        delta.y = 1;

    switch( g_KeyPressed )
    {
    case WXK_F1:
        cmd.SetId( ID_POPUP_ZOOM_IN );
        GetEventHandler()->ProcessEvent( cmd );
        flagcurseur = 2;
        curpos = GetScreen()->m_Curseur;
        break;

    case WXK_F2:
        cmd.SetId( ID_POPUP_ZOOM_OUT );
        GetEventHandler()->ProcessEvent( cmd );
        flagcurseur = 2;
        curpos = GetScreen()->m_Curseur;
        break;

    case WXK_F3:
        cmd.SetId( ID_ZOOM_REDRAW );
        GetEventHandler()->ProcessEvent( cmd );
        flagcurseur = 2;
        break;

    case WXK_F4:
        cmd.SetId( ID_POPUP_ZOOM_CENTER );
        GetEventHandler()->ProcessEvent( cmd );
        flagcurseur = 2;
        curpos = GetScreen()->m_Curseur;
        break;

    case ' ':
        GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
        break;

    case WXK_NUMPAD8:       /* cursor moved up */
    case WXK_UP:
        Mouse.y -= wxRound(delta.y);
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD2:       /* cursor moved down */
    case WXK_DOWN:
        Mouse.y += wxRound(delta.y);
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD4:       /*  cursor moved left */
    case WXK_LEFT:
        Mouse.x -= wxRound(delta.x);
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD6:      /*  cursor moved right */
    case WXK_RIGHT:
        Mouse.x += wxRound(delta.x);
        DrawPanel->MouseTo( Mouse );
        break;
    }

    GetScreen()->m_Curseur = curpos;
    /* Put cursor on grid */
    PutOnGrid( &GetScreen()->m_Curseur );

    if( GetScreen()->IsRefreshReq() )
    {
        flagcurseur = 2;
        RedrawActiveWindow( DC, TRUE );
    }

    if( oldpos != GetScreen()->m_Curseur )
    {
        if( flagcurseur != 2 )
        {
            curpos = GetScreen()->m_Curseur;
            GetScreen()->m_Curseur = oldpos;
            DrawPanel->CursorOff( DC );

            GetScreen()->m_Curseur = curpos;
            DrawPanel->CursorOn( DC );
        }

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, DC, 0 );
        }
    }

    UpdateStatusBar();    /* Display new cursor coordinates */
}


/*************************************************************************/
void WinEDA_DisplayFrame::Process_Special_Functions( wxCommandEvent& event )
/*************************************************************************/

/* Called when a tool is selected, or when a popup menu is clicked
 *  Currently : no action exists
 */
{
    int        id = event.GetId();

    wxClientDC dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );

    switch( id )
    {
    default:
        wxMessageBox( wxT( "WinEDA_DisplayFrame::Process_Special_Functions error" ) );
        break;
    }

    SetToolbars();
}

/**
 * Display 3D frame of current footprint selection.
 */
void WinEDA_DisplayFrame::Show3D_Frame( wxCommandEvent& event )
{
    if( m_Draw3DFrame )
    {
        DisplayInfoMessage( this, _( "3D Frame already opened" ) );
        return;
    }

    m_Draw3DFrame = new WinEDA3D_DrawFrame( this, _( "3D Viewer" ),
                                            KICAD_DEFAULT_3D_DRAWFRAME_STYLE |
                                            wxFRAME_FLOAT_ON_PARENT );
    m_Draw3DFrame->Show( TRUE );
}
