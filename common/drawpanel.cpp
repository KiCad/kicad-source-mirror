/*****************/
/* drawpanel.cpp */
/*****************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "macros.h"
#include "id.h"
#include "class_drawpanel.h"
#include "class_base_screen.h"
#include "wxstruct.h"

#include "kicad_device_context.h"

#define CURSOR_SIZE 12           // Cursor size in pixels


// Helper class to handle the client Device Context
KicadGraphicContext::KicadGraphicContext( WinEDA_DrawPanel * aDrawPanel )
    : wxClientDC(aDrawPanel)
{
    GRResetPenAndBrush( this );
    SetBackgroundMode( wxTRANSPARENT );
#ifdef WX_ZOOM
    double scale = aDrawPanel->GetScreen()->GetScalingFactor();
    SetUserScale( scale, scale );
    wxPoint origin = aDrawPanel->GetScreen()->m_DrawOrg;
    SetLogicalOrigin( origin.x, origin.y );
#endif
    aDrawPanel->SetBoundaryBox();
}

KicadGraphicContext::~KicadGraphicContext( )
{
}


/* Used to inhibit a response to a mouse left button release, after a
 * double click (when releasing the left button at the end of the second
 * click.  Used in eeschema to inhibit a mouse left release command when
 * switching between hierarchical sheets on a double click.
 */
static bool s_IgnoreNextLeftButtonRelease = false;


// Events used by WinEDA_DrawPanel
BEGIN_EVENT_TABLE( WinEDA_DrawPanel, wxScrolledWindow )
    EVT_LEAVE_WINDOW( WinEDA_DrawPanel::OnMouseLeaving )
    EVT_MOUSEWHEEL( WinEDA_DrawPanel::OnMouseWheel )
    EVT_MOUSE_EVENTS( WinEDA_DrawPanel::OnMouseEvent )
    EVT_CHAR( WinEDA_DrawPanel::OnKeyEvent )
    EVT_CHAR_HOOK( WinEDA_DrawPanel::OnKeyEvent )
    EVT_PAINT( WinEDA_DrawPanel::OnPaint )
    EVT_SIZE( WinEDA_DrawPanel::OnSize )
    EVT_SCROLLWIN( WinEDA_DrawPanel::OnScroll )
    EVT_ACTIVATE( WinEDA_DrawPanel::OnActivate )
    EVT_MENU_RANGE( ID_PAN_UP, ID_PAN_RIGHT, WinEDA_DrawPanel::OnPan )
END_EVENT_TABLE()

/***********************************************************************/
/* WinEDA_DrawPanel base functions (WinEDA_DrawPanel is the main panel)*/
/***********************************************************************/

WinEDA_DrawPanel::WinEDA_DrawPanel( WinEDA_DrawFrame* parent, int id,
                                    const wxPoint& pos, const wxSize& size ) :
    wxScrolledWindow( parent, id, pos, size,
                      wxBORDER | wxNO_FULL_REPAINT_ON_RESIZE )
{
    m_Parent          = parent;
    wxASSERT( m_Parent );
    m_ScrollButt_unit = 40;

    SetBackgroundColour( wxColour( ColorRefs[g_DrawBgColor].m_Red,
                                   ColorRefs[g_DrawBgColor].m_Green,
                                   ColorRefs[g_DrawBgColor].m_Blue ) );
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );

    EnableScrolling( TRUE, TRUE );
    m_ClipBox.SetSize( size );
    m_ClipBox.SetX( 0 );
    m_ClipBox.SetY( 0 );
    m_CanStartBlock     = -1; // Command block can start if >= 0
    m_AbortEnable       = m_AbortRequest = false;
    m_AutoPAN_Enable    = TRUE;
    m_IgnoreMouseEvents = 0;

    ManageCurseur = NULL;
    ForceCloseManageCurseur = NULL;

    if( wxGetApp().m_EDA_Config )
        wxGetApp().m_EDA_Config->Read( wxT( "AutoPAN" ), &m_AutoPAN_Enable,
                                       true );

    m_AutoPAN_Request    = false;
    m_Block_Enable       = false;
    m_PanelDefaultCursor = m_PanelCursor = wxCURSOR_ARROW;
    m_CursorLevel = 0;
    m_PrintIsMirrored = false;
}


WinEDA_DrawPanel::~WinEDA_DrawPanel()
{
    wxGetApp().m_EDA_Config->Write( wxT( "AutoPAN" ), m_AutoPAN_Enable );
}


BASE_SCREEN* WinEDA_DrawPanel::GetScreen()
{
    WinEDA_DrawFrame* parentFrame = m_Parent;
    return parentFrame->GetBaseScreen();
}


/*
 *  Draw the schematic cursor which is usually on grid
 */
void WinEDA_DrawPanel::Trace_Curseur( wxDC* DC, int color )
{
    if( m_CursorLevel != 0 ||  DC == NULL )
        return;

    wxPoint Cursor = GetScreen()->m_Curseur;

    GRSetDrawMode( DC, GR_XOR );
    if( m_Parent->m_CursorShape == 1 )    /* Draws a crosshair. */
    {
        int dx = GetScreen()->Unscale( m_ClipBox.GetWidth() );
        int dy = GetScreen()->Unscale( m_ClipBox.GetHeight() );

        GRLine( &m_ClipBox, DC, Cursor.x - dx, Cursor.y,
                Cursor.x + dx, Cursor.y, 0, color );            // Y axis
        GRLine( &m_ClipBox, DC, Cursor.x, Cursor.y - dx,
                Cursor.x, Cursor.y + dy, 0, color );            // X axis
    }
    else
    {
        int len = GetScreen()->Unscale( CURSOR_SIZE );

        GRLine( &m_ClipBox, DC, Cursor.x - len, Cursor.y,
                Cursor.x + len, Cursor.y, 0, color );
        GRLine( &m_ClipBox, DC, Cursor.x, Cursor.y - len,
                Cursor.x, Cursor.y + len, 0, color );
    }
}


/*
 * Remove the grid cursor from the display in preparation for other drawing
 * operations
 */
void WinEDA_DrawPanel::CursorOff( wxDC* DC )
{
    Trace_Curseur( DC );
    --m_CursorLevel;
}


/*
 *  Display the grid cursor
 */
void WinEDA_DrawPanel::CursorOn( wxDC* DC )
{
    ++m_CursorLevel;
    Trace_Curseur( DC );

    if( m_CursorLevel > 0 )  // Shouldn't happen, but just in case ..
        m_CursorLevel = 0;
}


int WinEDA_DrawPanel::GetZoom()
{
    return GetScreen()->GetZoom();
}


void WinEDA_DrawPanel::SetZoom( int zoom )
{
    GetScreen()->SetZoom( zoom );
}


wxRealPoint WinEDA_DrawPanel::GetGrid()
{
    return GetScreen()->GetGridSize();
}



/** Calculate the cursor position in internal units.
 * @return  position (in internal units)
 * @param  ScreenPos = absolute position in pixels
 */
wxPoint WinEDA_DrawPanel::CursorRealPosition( const wxPoint& ScreenPos )
{
#ifdef WX_ZOOM
    wxCoord x, y;
    INSTALL_DC( DC, this );

    x = DC.DeviceToLogicalX( ScreenPos.x );
    y = DC.DeviceToLogicalY( ScreenPos.y );
    return wxPoint( x, y );
#else
    return GetScreen()->CursorRealPosition( ScreenPos );
#endif
}


/** Function IsPointOnDisplay
 * @param ref_pos is the position to test in pixels, relative to the panel.
 * @return TRUE if ref_pos is a point currently visible on screen
 *         false if ref_pos is out of screen
 */
bool WinEDA_DrawPanel::IsPointOnDisplay( wxPoint ref_pos )
{
    wxPoint  pos;
    EDA_Rect display_rect;

    SetBoundaryBox();
    display_rect = m_ClipBox;

    // Slightly decreased the size of the useful screen area  to avoid drawing
    // limits.
    #define PIXEL_MARGIN 8
    display_rect.Inflate( -PIXEL_MARGIN );

    // Convert physical coordinates.
    pos = CalcUnscrolledPosition( display_rect.GetPosition() );

    GetScreen()->Unscale( pos );
    pos += GetScreen()->m_DrawOrg;
    display_rect.m_Pos = pos;
    GetScreen()->Unscale( display_rect.m_Size );

    return display_rect.Inside( ref_pos );
}


void WinEDA_DrawPanel::PostDirtyRect( EDA_Rect aRect )
{
    // D( printf( "1) PostDirtyRect( x=%d, y=%d, width=%d, height=%d)\n", aRect.m_Pos.x, aRect.m_Pos.y, aRect.m_Size.x, aRect.m_Size.y ); )

    // Convert the rect coordinates and size to pixels (make a draw clip box):
    ConvertPcbUnitsToPixelsUnits( &aRect );

    // Ensure the rectangle is large enough after truncations.
    // The pcb units have finer granularity than the pixels, so it can happen
    // that the rectangle is not large enough for the erase portion.

    aRect.m_Size.x += 4;  // += 1 is not enough!
    aRect.m_Size.y += 4;

    // D( printf( "2) PostDirtyRect( x=%d, y=%d, width=%d, height=%d)\n",  aRect.m_Pos.x, aRect.m_Pos.y, aRect.m_Size.x, aRect.m_Size.y ); )

    // pass wxRect() via EDA_Rect::operator wxRect() overload
    RefreshRect( aRect, TRUE );
}


void WinEDA_DrawPanel::ConvertPcbUnitsToPixelsUnits( EDA_Rect* aRect )
{
    // Calculate the draw area origin in internal units:
    wxPoint pos = aRect->GetPosition();

    ConvertPcbUnitsToPixelsUnits( &pos );
    aRect->SetOrigin( pos );                // rect origin in pixel units
    GetScreen()->Scale( aRect->m_Size );
}


/***************************************************************************/
void WinEDA_DrawPanel::ConvertPcbUnitsToPixelsUnits( wxPoint* aPosition )
/***************************************************************************/
{
    // Calculate the draw area origin in internal units:
    wxPoint drwOrig;
    int     x_axis_scale, y_axis_scale;

    // Origin in scroll units;
    GetViewStart( &drwOrig.x, &drwOrig.y );
    GetScrollPixelsPerUnit( &x_axis_scale, &y_axis_scale );

    // Origin in pixels units
    drwOrig.x *= x_axis_scale;
    drwOrig.y *= y_axis_scale;

    // Origin in internal units
    GetScreen()->Unscale( drwOrig );

    // Real origin, according to the "plot" origin
    drwOrig += GetScreen()->m_DrawOrg;

    // position in internal units, relative to the visible draw area origin
    *aPosition -= drwOrig;

    // position in pixels, relative to the visible draw area origin
    GetScreen()->Scale( *aPosition );
}


/** Function CursorScreenPosition
 * @return the cursor current position in pixels in the screen draw area
 */
wxPoint WinEDA_DrawPanel::CursorScreenPosition()
{
#ifdef WX_ZOOM
    wxCoord x, y;
    INSTALL_DC ( DC, this );

    x = DC.LogicalToDeviceX( GetScreen()->m_Curseur.x );
    y = DC.LogicalToDeviceY( GetScreen()->m_Curseur.y );
    return wxPoint( x, y );
#else
    wxPoint pos = GetScreen()->m_Curseur - GetScreen()->m_DrawOrg;
    GetScreen()->Scale( pos );
    return pos;
#endif
}


/** Function GetScreenCenterRealPosition()
 * @return position (in internal units) of the current area center showed
 *         on screen
 */
wxPoint WinEDA_DrawPanel::GetScreenCenterRealPosition( void )
{
    wxSize  size;
    wxPoint realpos;

    size = GetClientSize() / 2;
    realpos = CalcUnscrolledPosition( wxPoint( size.x, size.y ) );

    GetScreen()->Unscale( realpos );
#ifdef WX_ZOOM
//    wxCoord x, y;
//    INSTALL_DC( DC, this );
//    realpos.x = DC.DeviceToLogicalX( realpos.x );
//    realpos.y = DC.DeviceToLogicalY( realpos.y );
#else
    realpos += GetScreen()->m_DrawOrg;
#endif
    return realpos;
}


/* Move the mouse cursor to the current schematic cursor
 */
void WinEDA_DrawPanel::MouseToCursorSchema()
{
    wxPoint Mouse = CursorScreenPosition();

    MouseTo( Mouse );
}


/** Move the mouse cursor to the position "Mouse"
 * @param Mouse = mouse cursor position, in pixels units
 */
void WinEDA_DrawPanel::MouseTo( const wxPoint& Mouse )
{
    int     x, y, xPpu, yPpu;
    wxPoint screenPos, drawingPos;
    wxRect  clientRect( wxPoint( 0, 0 ), GetClientSize() );

#ifdef WX_ZOOM
    CalcScrolledPosition( Mouse.x, Mouse.y, &screenPos.x, &screenPos.y );
#else
    screenPos = Mouse - GetScreen()->m_StartVisu;
#endif

    /* Scroll if the requested mouse position cursor is outside the drawing
     * area. */
    if( !clientRect.Contains( screenPos ) )
    {
        GetViewStart( &x, &y );
        GetScrollPixelsPerUnit( &xPpu, &yPpu );
        CalcUnscrolledPosition( screenPos.x, screenPos.y,
                                &drawingPos.x, &drawingPos.y );

        wxLogDebug( wxT( "MouseTo() initial screen position(%d, %d) " ) \
                    wxT( "rectangle(%d, %d, %d, %d) view(%d, %d)" ),
                    screenPos.x, screenPos.y, clientRect.x, clientRect.y,
                    clientRect.width, clientRect.height, x, y );

        if( screenPos.y < clientRect.GetTop() )
            y -= m_ScrollButt_unit * yPpu;
        else if( screenPos.y > clientRect.GetBottom() )
            y += m_ScrollButt_unit * yPpu;
        else if( clientRect.GetRight() < screenPos.x )
            x += m_ScrollButt_unit * xPpu;
        else
            x -= m_ScrollButt_unit * xPpu;

        Scroll( x, y );
        CalcScrolledPosition( drawingPos.x, drawingPos.y,
                              &screenPos.x, &screenPos.y );

        wxLogDebug( wxT( "MouseTo() scrolled screen position(%d, %d) " ) \
                    wxT( "view(%d, %d)" ), screenPos.x, screenPos.y, x, y );
    }

    WarpPointer( screenPos.x, screenPos.y );
}


/**
 * Called on window activation.
 * init the member m_CanStartBlock to avoid a block start command
 * on activation (because a left mouse button can be pressed and no block
 * command wanted.
 * This happens when enter on a hierarchy sheet on double click
 */
void WinEDA_DrawPanel::OnActivate( wxActivateEvent& event )
{
    m_CanStartBlock = -1;   // Block Command can't start
    event.Skip();
}


void WinEDA_DrawPanel::OnScroll( wxScrollWinEvent& event )
{
    int id = event.GetEventType();
    int dir, value = 0;
    int x, y;

    GetViewStart( &x, &y );
    dir = event.GetOrientation();   // wxHORIZONTAL or wxVERTICAL

    if( id == wxEVT_SCROLLWIN_LINEUP )
        value = -m_ScrollButt_unit;

    else if( id == wxEVT_SCROLLWIN_LINEDOWN )
        value = m_ScrollButt_unit;

    else if( id == wxEVT_SCROLLWIN_THUMBTRACK )
    {
        value = event.GetPosition();
        if( dir == wxHORIZONTAL )
            Scroll( value, -1 );
        else
            Scroll( -1, value );
        return;
    }
    else
    {
        event.Skip();
        return;
    }

    if( dir == wxHORIZONTAL )
    {
        Scroll( x + value, -1 );
    }
    else
    {
        Scroll( -1, y + value );
    }
    event.Skip();
}


void WinEDA_DrawPanel::OnSize( wxSizeEvent& event )
{
    SetBoundaryBox();
    event.Skip();
}


/** Function SetBoundaryBox()
 * set the m_ClipBox member to the current displayed rectangle dimensions
 */
void WinEDA_DrawPanel::SetBoundaryBox()
{
    BASE_SCREEN* Screen = GetScreen();;

    if( !Screen )
        return;
    wxPoint org;
    int     ii, jj;

    GetViewStart( &org.x, &org.y );
    GetScrollPixelsPerUnit( &ii, &jj );
    org.x *= ii;
    org.y *= jj;

    Screen->m_StartVisu = org;

    m_ClipBox.SetOrigin( org );
    m_ClipBox.SetSize( GetClientSize() );

#ifdef WX_ZOOM
    CalcUnscrolledPosition( m_ClipBox.m_Pos.x, m_ClipBox.m_Pos.y,
                            &m_ClipBox.m_Pos.x, &m_ClipBox.m_Pos.y );
#else
    m_ClipBox.m_Pos -= GetScreen()->m_StartVisu;
#endif

    m_ScrollButt_unit = MIN( Screen->m_SizeVisu.x, Screen->m_SizeVisu.y ) / 4;
    if( m_ScrollButt_unit < 2 )
        m_ScrollButt_unit = 2;

    Screen->m_ScrollbarPos.x = GetScrollPos( wxHORIZONTAL );
    Screen->m_ScrollbarPos.y = GetScrollPos( wxVERTICAL );
}


void WinEDA_DrawPanel::EraseScreen( wxDC* DC )
{
    GRSetDrawMode( DC, GR_COPY );

#ifndef WX_ZOOM
    GRSFilledRect( &m_ClipBox, DC, m_ClipBox.GetX(), m_ClipBox.GetY(),
                   m_ClipBox.GetRight(), m_ClipBox.GetBottom(),
                   0, g_DrawBgColor, g_DrawBgColor );
#else
    EDA_Rect tmp = m_ClipBox;

    m_ClipBox.m_Pos.x = DC->DeviceToLogicalX( m_ClipBox.m_Pos.x );
    m_ClipBox.m_Pos.y = DC->DeviceToLogicalY( m_ClipBox.m_Pos.y );
    m_ClipBox.m_Size.SetWidth(
        DC->DeviceToLogicalXRel( m_ClipBox.m_Size.GetWidth() ) );
    m_ClipBox.m_Size.SetHeight(
        DC->DeviceToLogicalYRel( m_ClipBox.m_Size.GetHeight() ) );

    GRSFilledRect( &m_ClipBox, DC, m_ClipBox.GetX(), m_ClipBox.GetY(),
                   m_ClipBox.GetRight(), m_ClipBox.GetBottom(),
                   0, g_DrawBgColor, g_DrawBgColor );

    m_ClipBox = tmp;
#endif
}


#if wxUSE_GRAPHICS_CONTEXT
// note: wxUSE_GRAPHICS_CONTEXT must be set to 1 in wxWidgets
// see setup.h in wx Widgets.
// wxWidgets configure can need option  --enable-graphics_ctx
// Currently, **only for tests**
//#define USE_GCDC_IN_KICAD     // uncomment it to use wxGCDC
#endif


void WinEDA_DrawPanel::OnPaint( wxPaintEvent& event )
{
    if( GetScreen() == NULL )
    {
        event.Skip();
        return;
    }

#ifdef USE_GCDC_IN_KICAD
    wxPaintDC pDC( this );
    // Following line should be disabled on MSW and OS X
    wxGCDC paintDC( pDC );
    // Fix for pixel offset bug http://trac.wxwidgets.org/ticket/4187
    paintDC.GetGraphicsContext()->Translate(0.5, 0.5);
#else
    INSTALL_PAINTDC( paintDC, this );
#endif
    EDA_Rect  tmp;
    wxRect    PaintClipBox;
    wxPoint   org;

    tmp = m_ClipBox;

    org = m_ClipBox.GetOrigin();

    wxRegion upd = GetUpdateRegion(); // get the update rect list

    // get the union of all rectangles in the update region, 'upd'
    PaintClipBox = upd.GetBox();

#if 0 && defined (DEBUG)
    printf( "1) PaintClipBox=(%d, %d, %d, %d) org=(%d, %d) m_ClipBox=(%d, %d, %d, %d)\n",
            PaintClipBox.x,
            PaintClipBox.y,
            PaintClipBox.width,
            PaintClipBox.height,
            org.x, org.y,
            m_ClipBox.m_Pos.x, m_ClipBox.m_Pos.y,
            m_ClipBox.m_Size.x, m_ClipBox.m_Size.y
            );
#endif

#ifdef WX_ZOOM
    wxLogDebug( wxT( "1) PaintClipBox=(%d, %d, %d, %d) org=(%d, %d) " \
                     "m_ClipBox=(%d, %d, %d, %d)\n" ), PaintClipBox.x,
                     PaintClipBox.y, PaintClipBox.width, PaintClipBox.height,
                     org.x, org.y,  m_ClipBox.m_Pos.x, m_ClipBox.m_Pos.y,
                     m_ClipBox.m_Size.x, m_ClipBox.m_Size.y );

    wxSize drawing_size = GetScreen()->ReturnPageSize() * 2;
    m_ClipBox.m_Pos.x = 0;
    m_ClipBox.m_Pos.y = 0;
    m_ClipBox.SetWidth( drawing_size.x );
    m_ClipBox.SetHeight( drawing_size.y );

    wxLogDebug( wxT( "2) PaintClipBox=(%d, %d, %d, %d) org=(%d, %d) " \
                     "m_ClipBox=(%d, %d, %d, %d)\n" ), PaintClipBox.x,
                     PaintClipBox.y, PaintClipBox.width, PaintClipBox.height,
                     org.x, org.y,  m_ClipBox.m_Pos.x, m_ClipBox.m_Pos.y,
                     m_ClipBox.m_Size.x, m_ClipBox.m_Size.y );
#else
    PaintClipBox.Offset( org );
    m_ClipBox.SetX( PaintClipBox.GetX() );
    m_ClipBox.SetY( PaintClipBox.GetY() );
    m_ClipBox.SetWidth( PaintClipBox.GetWidth() );
    m_ClipBox.SetHeight( PaintClipBox.GetHeight() );
#endif

    // Be sure the drawpanel clipbox is bigger than the region to repair:
    m_ClipBox.Inflate(1); // Give it one pixel more in each direction

#if 0 && defined (DEBUG)
    printf( "2) PaintClipBox=(%d, %d, %d, %d) org=(%d, %d) m_ClipBox=(%d, %d, %d, %d)\n",
            PaintClipBox.x,
            PaintClipBox.y,
            PaintClipBox.width,
            PaintClipBox.height,
            org.x, org.y,
            m_ClipBox.m_Pos.x, m_ClipBox.m_Pos.y,
            m_ClipBox.m_Size.x, m_ClipBox.m_Size.y
            );
#endif


    PaintClipBox = m_ClipBox;

    // call ~wxDCClipper() before ~wxPaintDC()
    {
#ifndef WX_ZOOM
        wxDCClipper dcclip( paintDC, PaintClipBox );
#endif
        ReDraw( &paintDC, true );

#ifdef WX_ZOOM
        paintDC.SetUserScale( 1.0, 1.0 );
#endif
    }

    m_ClipBox = tmp;
    event.Skip();
}


void WinEDA_DrawPanel::ReDraw( wxDC* DC, bool erasebg )
{
    BASE_SCREEN* Screen = GetScreen();

    if( Screen == NULL )
        return;

    if( ( g_DrawBgColor != WHITE ) && ( g_DrawBgColor != BLACK ) )
        g_DrawBgColor = BLACK;

    if( g_DrawBgColor == WHITE )
    {
        g_XorMode    = GR_NXOR;
        g_GhostColor = BLACK;
    }
    else
    {
        g_XorMode    = GR_XOR;
        g_GhostColor = WHITE;
    }

    if( erasebg )
        EraseScreen( DC );

    SetBackgroundColour( wxColour( ColorRefs[g_DrawBgColor].m_Red,
                                   ColorRefs[g_DrawBgColor].m_Green,
                                   ColorRefs[g_DrawBgColor].m_Blue ) );

    GRResetPenAndBrush( DC );

    DC->SetBackground( *wxBLACK_BRUSH );
    DC->SetBackgroundMode( wxTRANSPARENT );
    m_Parent->RedrawActiveWindow( DC, erasebg );
}


/**  Function DrawBackGround
 * @param DC = current Device Context
 * Draws X , Y axis
 * draws the grid
 *  - the grid is drawn only if the zoom level allows a good visibility
 *  - the grid is always centered on the screen center
 */
void WinEDA_DrawPanel::DrawBackGround( wxDC* DC )
{
    int          Color  = BLUE;
    BASE_SCREEN* screen = GetScreen();
    int          ii, jj, xg, yg, color;
    wxRealPoint  screen_grid_size;
    bool         drawgrid = false;
    wxSize       size;
    wxPoint      org;

    color = g_GridColor;

    GRSetDrawMode( DC, GR_COPY );

    /* The grid must be visible. this is possible only is grid value
     * and zoom value are sufficient
     */
    drawgrid = m_Parent->m_Draw_Grid;

    screen_grid_size = screen->GetGridSize();

    wxRealPoint  dgrid = screen_grid_size;
    screen->Scale( dgrid );     // dgrid = grid size in pixels
    // if the grid size is small ( < 5 pixels) do not display all points
    if( dgrid.x  < 5 )
    {
        screen_grid_size.x *= 2;
        dgrid.x *= 2;
    }
    if( dgrid.x < 5 )
        drawgrid = false; // The grid is too small: do not show it

    if( dgrid.y < 5 )
    {
        screen_grid_size.y *= 2;
        dgrid.y *= 2;
    }
    if( dgrid.y < 5 )
        drawgrid = false; // The grid is too small

    GetViewStart( &org.x, &org.y );
    GetScrollPixelsPerUnit( &ii, &jj );
    org.x *= ii;
    org.y *= jj;
    screen->m_StartVisu = org;
    screen->Unscale( org );

    org += screen->m_DrawOrg;

    size = GetClientSize();
    screen->Unscale( size );

#ifdef WX_ZOOM
    screen_grid_size = screen->GetGridSize();

    if( DC->LogicalToDeviceXRel( (int) screen_grid_size.x ) < 5
        || DC->LogicalToDeviceYRel( (int) screen_grid_size.y ) < 5 )
        drawgrid = false;

    org.x = DC->DeviceToLogicalX( org.x );
    org.y = DC->DeviceToLogicalY( org.y );
    size.SetWidth( DC->DeviceToLogicalXRel( size.GetWidth() ) );
    size.SetHeight( DC->DeviceToLogicalYRel( size.GetHeight() ) );
#endif

    if( drawgrid )
    {
        m_Parent->PutOnGrid( &org );

        GRSetColorPen( DC, color );
        for( ii = 0; ; ii++ )
        {
            xg =  wxRound(ii * screen_grid_size.x);
            if( xg > size.x )
                break;
            int xpos = org.x + xg;
            xpos = GRMapX( xpos );
            for( jj = 0; ; jj++ )
            {
                yg = wxRound(jj * screen_grid_size.y);
                if( yg > size.y )
                    break;
                int ypos = org.y + yg;
                DC->DrawPoint( xpos, GRMapY( ypos ) );
            }

         }
    }

    /* Draw axis */
    if( m_Parent->m_Draw_Axis )
    {
        /* Draw the Y axis */
        GRDashedLine( &m_ClipBox, DC, 0, -screen->ReturnPageSize().y,
                      0, screen->ReturnPageSize().y, 0, Color );

        /* Draw the X axis */
        GRDashedLine( &m_ClipBox, DC, -screen->ReturnPageSize().x, 0,
                      screen->ReturnPageSize().x, 0, 0, Color );
    }

    DrawAuxiliaryAxis( DC, GR_COPY );
}


/** m_Draw_Auxiliary_Axis
 * Draw the Auxiliary Axis, used in pcbnew which as origin coordinates
 * for gerber and excellon files
 */
void WinEDA_DrawPanel::DrawAuxiliaryAxis( wxDC* DC, int drawmode )
{
    if( !m_Parent->m_Draw_Auxiliary_Axis
        || ( m_Parent->m_Auxiliary_Axis_Position.x == 0
             && m_Parent->m_Auxiliary_Axis_Position.y == 0 ) )
        return;

    int          Color  = DARKRED;
    BASE_SCREEN* screen = GetScreen();

    GRSetDrawMode( DC, drawmode );

    /* Draw the Y axis */
    GRDashedLine( &m_ClipBox, DC,
                  m_Parent->m_Auxiliary_Axis_Position.x,
                  -screen->ReturnPageSize().y,
                  m_Parent->m_Auxiliary_Axis_Position.x,
                  screen->ReturnPageSize().y,
                  0, Color );

    /* Draw the X axis */
    GRDashedLine( &m_ClipBox, DC,
                  -screen->ReturnPageSize().x,
                  m_Parent->m_Auxiliary_Axis_Position.y,
                  screen->ReturnPageSize().x,
                  m_Parent->m_Auxiliary_Axis_Position.y,
                  0, Color );
}


/** Build and display a Popup menu on a right mouse button click
 * @return true if a popup menu is shown, or false
 */
bool WinEDA_DrawPanel::OnRightClick( wxMouseEvent& event )
{
    wxPoint pos;
    wxMenu  MasterMenu;

    pos = event.GetPosition();

    if( !m_Parent->OnRightClick( pos, &MasterMenu ) )
        return false;

    AddMenuZoom( &MasterMenu );

    m_IgnoreMouseEvents = TRUE;
    PopupMenu( &MasterMenu, pos );
    MouseToCursorSchema();
    m_IgnoreMouseEvents = false;

    return true;
}


// Called when the canvas receives a mouse event leaving frame.
void WinEDA_DrawPanel::OnMouseLeaving( wxMouseEvent& event )
{
    if( ManageCurseur == NULL )          // No command in progress.
        m_AutoPAN_Request = false;

    if( !m_AutoPAN_Enable || !m_AutoPAN_Request || m_IgnoreMouseEvents )
        return;

    // Auto pan if mouse is leave working area:
    wxSize size = GetClientSize();

    if( ( size.x < event.GetX() ) || ( size.y < event.GetY() )
        || ( event.GetX() <= 0) || ( event.GetY() <= 0 ) )
    {
        wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED, ID_POPUP_ZOOM_CENTER );
        cmd.SetEventObject( this );
        GetEventHandler()->ProcessEvent( cmd );
    }
}


/*
 * Handle mouse wheel events.
 *
 * The mouse wheel is used to provide support for zooming and panning.  This
 * is accomplished by converting mouse wheel events in pseudo menu command
 * events.
 */
void WinEDA_DrawPanel::OnMouseWheel( wxMouseEvent& event )
{
    wxRect rect = wxRect( wxPoint( 0, 0), GetClientSize() );

    /* Ignore scroll events if the cursor is outside the drawing area. */
    if( event.GetWheelRotation() == 0 || !GetParent()->IsEnabled()
        || !rect.Contains( event.GetPosition() ) )
    {
#if 0
        wxLogDebug( wxT( "OnMouseWheel() position(%d, %d) " ) \
                    wxT( "rectangle(%d, %d, %d, %d)" ),
                    event.GetPosition().x, event.GetPosition().y,
                    rect.x, rect.y, rect.width, rect.height );
#endif
        event.Skip();
        return;
    }

    GetScreen()->m_Curseur =
        CursorRealPosition( CalcUnscrolledPosition( event.GetPosition() ) );

    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    // This is a zoom in or out command
    if( event.GetWheelRotation() > 0 )
    {
        if( event.ShiftDown() && !event.ControlDown() )
            cmd.SetId( ID_PAN_UP );
        else if( event.ControlDown() && !event.ShiftDown() )
            cmd.SetId( ID_PAN_LEFT );
        else
            cmd.SetId( ID_POPUP_ZOOM_IN );
    }
    else if ( event.GetWheelRotation() < 0 )
    {
        if( event.ShiftDown() && !event.ControlDown() )
            cmd.SetId( ID_PAN_DOWN );
        else if( event.ControlDown() && !event.ShiftDown() )
            cmd.SetId( ID_PAN_RIGHT );
        else
            cmd.SetId( ID_POPUP_ZOOM_OUT );
    }

    GetEventHandler()->ProcessEvent( cmd );
}


// Called when the canvas receives a mouse event.
void WinEDA_DrawPanel::OnMouseEvent( wxMouseEvent& event )
{
    int                      localrealbutt = 0, localbutt = 0, localkey = 0;
    BASE_SCREEN*             screen = GetScreen();
    static WinEDA_DrawPanel* LastPanel;

    if( !screen )
        return;

    /* Adjust value to filter mouse displacement before consider the drag
     * mouse is really a drag command, not just a movement while click
     */
#define MIN_DRAG_COUNT_FOR_START_BLOCK_COMMAND 5

   /* Count the drag events.  Used to filter mouse moves before starting a
    * block command.  A block command can be started only if MinDragEventCount >
    * MIN_DRAG_COUNT_FOR_START_BLOCK_COMMAND in order to avoid spurious block
    * commands. */
    static int MinDragEventCount;
    if( event.Leaving() || event.Entering() )
    {
        m_CanStartBlock = -1;
    }

    if( ManageCurseur == NULL )  // No command in progress
        m_AutoPAN_Request = false;

    if( m_Parent->m_FrameIsActive )
        SetFocus();
    else
        return;

    if( !event.IsButton() && !event.Moving()
        && !event.Dragging() && !localkey )
    {
        return;
    }

    if( event.RightDown() )
    {
        OnRightClick( event );
        return;
    }

    if( m_IgnoreMouseEvents )
        return;

    if( event.LeftIsDown() )
        localrealbutt |= GR_M_LEFT_DOWN;

    if( event.MiddleIsDown() )
        localrealbutt |= GR_M_MIDDLE_DOWN;

    if( event.LeftDown() )
        localbutt = GR_M_LEFT_DOWN;

    if( event.ButtonDClick( 1 ) )
        localbutt = GR_M_LEFT_DOWN | GR_M_DCLICK;

    if( event.MiddleDown() )
        localbutt = GR_M_MIDDLE_DOWN;

    if( event.ButtonDClick( 2 ) )
    {
    }
    ;                               // Unused

    localrealbutt |= localbutt;     /* compensation default wxGTK */

    /* Compute absolute m_MousePosition in pixel units: */
    screen->m_MousePositionInPixels =
        CalcUnscrolledPosition( event.GetPosition() );

    /* Compute absolute m_MousePosition in user units: */
    screen->m_MousePosition =
        CursorRealPosition( screen->m_MousePositionInPixels );

    INSTALL_DC( DC, this );

    int        kbstat = 0;

    DC.SetBackground( *wxBLACK_BRUSH );

    g_KeyPressed = localkey;

    if( event.ShiftDown() )
        kbstat |= GR_KB_SHIFT;
    if( event.ControlDown() )
        kbstat |= GR_KB_CTRL;
    if( event.AltDown() )
        kbstat |= GR_KB_ALT;

    g_MouseOldButtons = localrealbutt;

    // Calling Double Click and Click functions :
    if( localbutt == (int) (GR_M_LEFT_DOWN | GR_M_DCLICK) )
    {
        m_Parent->OnLeftDClick( &DC, screen->m_MousePositionInPixels );

        // inhibit a response to the mouse left button release,
        // because we have a double click, and we do not want a new
        // OnLeftClick command at end of this Double Click
        s_IgnoreNextLeftButtonRelease = true;
    }
    else if( event.LeftUp() )
    {
        // A block command is in progress: a left up is the end of block
        // or this is the end of a double click, already seen
        if( screen->m_BlockLocate.m_State==STATE_NO_BLOCK
            && !s_IgnoreNextLeftButtonRelease )
            m_Parent->OnLeftClick( &DC, screen->m_MousePositionInPixels );

        s_IgnoreNextLeftButtonRelease = false;
    }

    if( !event.LeftIsDown() )
    {
        /* be sure there is a response to a left button release command
         * even when a LeftUp event is not seen.  This happens when a
         * double click opens a dialog box, and the release mouse button
         * is made when the dialog box is open.
         */
        s_IgnoreNextLeftButtonRelease = false;
    }

    if( event.ButtonUp( 2 )
        && (screen->m_BlockLocate.m_State == STATE_NO_BLOCK) )
    {
        // The middle button has been released, with no block command:
        // We use it for a zoom center at cursor position command
        wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED,
                            ID_POPUP_ZOOM_CENTER );
        cmd.SetEventObject( this );
        GetEventHandler()->ProcessEvent( cmd );
    }


    /* Calling the general function on mouse changes (and pseudo key commands) */
    m_Parent->GeneralControle( &DC, screen->m_MousePositionInPixels );


    /*******************************/
    /* Control of block commands : */
    /*******************************/

    // Command block can't start if mouse is dragging a new panel
    if( LastPanel != this )
    {
        MinDragEventCount = 0;
        m_CanStartBlock   = -1;
    }

    /* A new command block can start after a release buttons
     * and if the drag is enough
     * This is to avoid a false start block when a dialog box is dismissed,
     * or when changing panels in hierarchy navigation
     * or when clicking while and moving mouse
     */
    if( !event.LeftIsDown() && !event.MiddleIsDown() )
    {
        MinDragEventCount = 0;
        m_CanStartBlock   = 0;

        /* Remember the last cursor position when a drag mouse starts
         * this is the last position ** before ** clicking a button
         * this is useful to start a block command from the point where the
         * mouse was clicked first
         * (a filter creates a delay for the real block command start, and
         * we must remember this point)
         */
        m_CursorStartPos = screen->m_Curseur;
    }

    if( m_Block_Enable && !(localbutt & GR_M_DCLICK) )
    {
        if( ( screen->m_BlockLocate.m_Command == BLOCK_IDLE )
            || ( screen->m_BlockLocate.m_State == STATE_NO_BLOCK ) )
        {
            screen->m_BlockLocate.SetOrigin( m_CursorStartPos );
        }
        if( event.LeftDown() || event.MiddleDown() )
        {
            if( screen->m_BlockLocate.m_State == STATE_BLOCK_MOVE )
            {
                m_AutoPAN_Request = false;
                m_Parent->HandleBlockPlace( &DC );
                s_IgnoreNextLeftButtonRelease = true;
            }
        }
        else if( ( m_CanStartBlock >= 0 )
                 && ( event.LeftIsDown() || event.MiddleIsDown() )
                 && ManageCurseur == NULL
                 && ForceCloseManageCurseur == NULL )
        {
            // Mouse is dragging: if no block in progress,  start a block
            // command.
            if( screen->m_BlockLocate.m_State == STATE_NO_BLOCK )
            {   //  Start a block command
                int cmd_type = kbstat;

                if( event.MiddleIsDown() )
                    cmd_type |= MOUSE_MIDDLE;

                /* A block command is started if the drag is enough.  A small
                 * drag is ignored (it is certainly a little mouse move when
                 * clicking) not really a drag mouse
                 */
                if( MinDragEventCount < MIN_DRAG_COUNT_FOR_START_BLOCK_COMMAND )
                    MinDragEventCount++;
                else
                {
                    if( !m_Parent->HandleBlockBegin( &DC, cmd_type,
                                                     m_CursorStartPos ) )
                    {
                        // should not occurs: error
                        m_Parent->DisplayToolMsg(
                            wxT( "WinEDA_DrawPanel::OnMouseEvent() Block Error" ) );
                    }
                    else
                    {
                        m_AutoPAN_Request = TRUE;
                        SetCursor( m_PanelCursor = wxCURSOR_SIZING );
                    }
                }
            }
        }

        if( event.ButtonUp( 1 ) || event.ButtonUp( 2 ) )
        {
            /* Release the mouse button: end of block.
             * The command can finish (DELETE) or have a next command (MOVE,
             * COPY).  However the block command is canceled if the block
             * size is small because a block command filtering is already
             * made, this case happens, but only when the on grid cursor has
             * not moved.
             */
            #define BLOCK_MINSIZE_LIMIT 1
            bool BlockIsSmall =
                ( ABS( screen->Scale( screen->m_BlockLocate.GetWidth() ) )
                  < BLOCK_MINSIZE_LIMIT)
                  && ( ABS( screen->Scale( screen->m_BlockLocate.GetHeight() ) )
                       < BLOCK_MINSIZE_LIMIT);

            if( (screen->m_BlockLocate.m_State
                 != STATE_NO_BLOCK) && BlockIsSmall )
            {
                if( ForceCloseManageCurseur )
                {
                    ForceCloseManageCurseur( this, &DC );
                    m_AutoPAN_Request = false;
                }
                SetCursor( m_PanelCursor = m_PanelDefaultCursor );
            }
            else if( screen->m_BlockLocate.m_State == STATE_BLOCK_END )
            {
                m_AutoPAN_Request = false;
                m_Parent->HandleBlockEnd( &DC );
                SetCursor( m_PanelCursor = m_PanelDefaultCursor );
                if( screen->m_BlockLocate.m_State == STATE_BLOCK_MOVE )
                {
                    m_AutoPAN_Request = TRUE;
                    SetCursor( m_PanelCursor = wxCURSOR_HAND );
                }
            }
        }
    }

    // End of block command on a double click
    // To avoid an unwanted block move command if the mouse is moved while
    // double clicking
    if( localbutt == (int) ( GR_M_LEFT_DOWN | GR_M_DCLICK ) )
    {
        if( screen->m_BlockLocate.m_Command != BLOCK_IDLE )
        {
            if( ForceCloseManageCurseur )
            {
                ForceCloseManageCurseur( this, &DC );
                m_AutoPAN_Request = false;
            }
        }
    }


#if 0
    wxString msg_debug;
    msg_debug.Printf( " block state %d, cmd %d",
                      screen->m_BlockLocate.m_State,
                      screen->m_BlockLocate.m_Command );
    m_Parent->PrintMsg( msg_debug );
#endif

    LastPanel = this;
}


void WinEDA_DrawPanel::OnKeyEvent( wxKeyEvent& event )
{
    long    key, localkey;
    bool    escape = false;
    wxPoint pos;

    key = localkey = event.GetKeyCode();

    switch( localkey )
    {
    case WXK_CONTROL:
    case WXK_CAPITAL:
    case WXK_SHIFT:
    case WXK_NUMLOCK:
    case WXK_LBUTTON:
    case WXK_RBUTTON:
    case WXK_ALT:
        return;

    case WXK_ESCAPE:
        escape = m_AbortRequest = TRUE;
        break;
    }

    if( event.ControlDown() )
        localkey |= GR_KB_CTRL;
    if( event.AltDown() )
        localkey |= GR_KB_ALT;
    if( event.ShiftDown() && (key > 256) )
        localkey |= GR_KB_SHIFT;

    INSTALL_DC(DC, this );

    BASE_SCREEN* Screen = GetScreen();


    g_KeyPressed = localkey;

    if( escape )
    {
        if( ManageCurseur && ForceCloseManageCurseur )
        {
            SetCursor( m_PanelCursor = m_PanelDefaultCursor );
            ForceCloseManageCurseur( this, &DC );
            SetCursor( m_PanelCursor = m_PanelDefaultCursor );
        }
        else
        {
            m_PanelCursor = m_PanelDefaultCursor = wxCURSOR_ARROW;
            m_Parent->SetToolID( 0, m_PanelCursor, wxEmptyString );
        }
    }

    /* Some key commands use the current mouse position: refresh it */
#ifdef WX_ZOOM
    pos = CalcUnscrolledPosition( wxGetMousePosition() );
#else
    pos = CalcUnscrolledPosition( wxGetMousePosition() - GetScreenPosition() );
#endif

    /* Compute absolute mouse position in pixel units (i.e. considering the
       current scroll) : */
    Screen->m_MousePositionInPixels = pos;

    /* Compute absolute mouse position in user units: */
    Screen->m_MousePosition = CursorRealPosition( pos );

    m_Parent->GeneralControle( &DC, pos );

#if 0
    event.Skip();   // Allow menu shortcut processing
#endif
}


void WinEDA_DrawPanel::OnPan( wxCommandEvent& event )
{
    int        x, y;

    GetViewStart( &x, &y );       // x and y are in scroll units, not in pixels

    switch( event.GetId() )
    {
    case ID_PAN_UP:
        y -= m_ScrollButt_unit;
        break;

    case ID_PAN_DOWN:
        y += m_ScrollButt_unit;
        break;

    case ID_PAN_LEFT:
        x -= m_ScrollButt_unit;
        break;

    case ID_PAN_RIGHT:
        x += m_ScrollButt_unit;
        break;

    default:
        wxLogDebug( wxT( "Unknown ID %d in WinEDA_DrawPanel::OnPan()." ),
                    event.GetId() );
    }

    Scroll( x, y );
    MouseToCursorSchema();
}


void WinEDA_DrawPanel::UnManageCursor( int id, int cursor,
                                       const wxString& title )
{
    if( ManageCurseur && ForceCloseManageCurseur )
    {
        INSTALL_DC( dc, this );
        ForceCloseManageCurseur( this, &dc );
        m_AutoPAN_Request = false;
    }
    if( id != -1 && cursor != -1 )
    {
        wxASSERT( cursor > wxCURSOR_NONE && cursor < wxCURSOR_MAX );
        m_Parent->SetToolID( id, cursor, title );
    }
}
