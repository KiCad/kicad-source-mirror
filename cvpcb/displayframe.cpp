/*********************************************************************/
/** setvisu() : initialisations de l'ecran d'affichage du composant **/
/*********************************************************************/

#include "fctsys.h"
#include "common.h"

#include "wxstruct.h"
#include "cvpcb.h"
#include "id.h"
#include "bitmaps.h"

#include "protos.h"

#define BITMAP wxBitmap

/*****************************************************************/
/* Construction de la table des evenements pour WinEDA_DrawFrame */
/*****************************************************************/

BEGIN_EVENT_TABLE( WinEDA_DisplayFrame, wxFrame )

COMMON_EVENTS_DRAWFRAME EVT_CLOSE( WinEDA_DisplayFrame::OnCloseWindow )
EVT_SIZE( WinEDA_DrawFrame::OnSize )
EVT_TOOL_RANGE( ID_ZOOM_IN_BUTT, ID_ZOOM_PAGE_BUTT,
                WinEDA_DisplayFrame::Process_Zoom )
EVT_TOOL( ID_OPTIONS_SETUP, WinEDA_DisplayFrame::InstallOptionsDisplay )
EVT_TOOL( ID_CVPCB_SHOW3D_FRAME, WinEDA_BasePcbFrame::Show3D_Frame )
END_EVENT_TABLE()


/***************************************************************************/
/* WinEDA_DisplayFrame: the frame to display the current focused footprint */
/***************************************************************************/

WinEDA_DisplayFrame::WinEDA_DisplayFrame( wxWindow* father, WinEDA_App* parent,
                                          const wxString& title,
                                          const wxPoint& pos, const wxSize& size, long style ) :
    WinEDA_BasePcbFrame( father, parent, CVPCB_DISPLAY_FRAME, title, pos, size, style )
{
    m_FrameName      = wxT( "CmpFrame" );
    m_Draw_Axis      = TRUE;                    // TRUE if we want the axis
    m_Draw_Grid      = TRUE;                    // TRUE if we want the grid
    m_Draw_Sheet_Ref = FALSE;                   // TRUE if we want the sheet references

    // Give an icon
    #ifdef __WINDOWS__
    SetIcon( wxICON( a_icon_cvpcb ) );
    #else
    SetIcon( wxICON( icon_cvpcb ) );
    #endif
    SetTitle( title );

    m_Pcb = new BOARD( NULL, this );

    SetBaseScreen( new PCB_SCREEN( CVPCB_DISPLAY_FRAME ) );

    GetSettings();
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

    delete m_Pcb;

    m_Parent->m_CvpcbFrame->DrawFrame = NULL;
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
                        BITMAP( display_options_xpm ),
                        _( "Display Options" ) );

    m_HToolBar->AddSeparator();

    m_HToolBar->AddTool( ID_ZOOM_IN_BUTT, wxEmptyString, BITMAP( zoom_in_xpm ),
                        _( "zoom + (F1)" ) );

    m_HToolBar->AddTool( ID_ZOOM_OUT_BUTT, wxEmptyString, BITMAP( zoom_out_xpm ),
                        _( "zoom - (F2)" ) );

    m_HToolBar->AddTool( ID_ZOOM_REDRAW_BUTT, wxEmptyString, BITMAP( zoom_redraw_xpm ),
                        _( "redraw (F3)" ) );

    m_HToolBar->AddTool( ID_ZOOM_PAGE_BUTT, wxEmptyString, BITMAP( zoom_auto_xpm ),
                        _( "1:1 zoom" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_SHOW3D_FRAME, wxEmptyString, BITMAP( show_3d_xpm ),
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
    wxSize  delta;
    int     flagcurseur = 0;
    int     zoom = GetScreen()->GetZoom();
    wxPoint curpos, oldpos;

    curpos = DrawPanel->CursorRealPosition( Mouse );
    oldpos = GetScreen()->m_Curseur;

    delta.x = GetScreen()->GetGrid().x / zoom;
    delta.y = GetScreen()->GetGrid().y / zoom;
    if( delta.x <= 0 )
        delta.x = 1;
    if( delta.y <= 0 )
        delta.y = 1;

    if( g_KeyPressed )
    {
        switch( g_KeyPressed )
        {
        case WXK_F1:
            OnZoom( ID_ZOOM_IN_KEY );
            flagcurseur = 2;
            curpos = GetScreen()->m_Curseur;
            break;

        case WXK_F2:
            OnZoom( ID_ZOOM_OUT_KEY );
            flagcurseur = 2;
            curpos = GetScreen()->m_Curseur;
            break;

        case WXK_F3:
            OnZoom( ID_ZOOM_REDRAW_KEY );
            flagcurseur = 2;
            break;

        case WXK_F4:
            OnZoom( ID_ZOOM_CENTER_KEY );
            flagcurseur = 2;
            curpos = GetScreen()->m_Curseur;
            break;

        case ' ':
            GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
            break;

        case WXK_NUMPAD8:       /* cursor moved up */
        case WXK_UP:
            DrawPanel->CalcScrolledPosition( Mouse.x, Mouse.y - delta.y,
                                             &Mouse.x, &Mouse.y );
            GRMouseWarp( DrawPanel, Mouse );
            break;

        case WXK_NUMPAD2:       /* cursor moved down */
        case WXK_DOWN:
            DrawPanel->CalcScrolledPosition( Mouse.x, Mouse.y + delta.y,
                                             &Mouse.x, &Mouse.y );
            GRMouseWarp( DrawPanel, Mouse );
            break;

        case WXK_NUMPAD4:       /*  cursor moved left */
        case WXK_LEFT:
            DrawPanel->CalcScrolledPosition( Mouse.x - delta.x, Mouse.y,
                                             &Mouse.x, &Mouse.y );
            GRMouseWarp( DrawPanel, Mouse );
            break;

        case WXK_NUMPAD6:      /*  cursor moved right */
        case WXK_RIGHT:
            DrawPanel->CalcScrolledPosition( Mouse.x + delta.x, Mouse.y,
                                             &Mouse.x, &Mouse.y );
            GRMouseWarp( DrawPanel, Mouse );
            break;
        }
    }

    GetScreen()->m_Curseur = curpos;
    /* Put cursor on grid */
    PutOnGrid( &GetScreen()->m_Curseur );

    if( GetScreen()->IsRefreshReq() )
    {
        flagcurseur = 2;
        RedrawActiveWindow( DC, TRUE );
    }

    if( (oldpos.x != GetScreen()->m_Curseur.x)
       || (oldpos.y != GetScreen()->m_Curseur.y) )
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

    Affiche_Status_Box();    /* Display new cursor coordinates */
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
