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

#include <wx/wupdlock.h>
#include "kicad_device_context.h"

#define CURSOR_SIZE 12           // Cursor size in pixels


// Helper class to handle the client Device Context
KicadGraphicContext::KicadGraphicContext( WinEDA_DrawPanel* aDrawPanel ) :
    wxClientDC( aDrawPanel )
{
    GRResetPenAndBrush( this );
    SetBackgroundMode( wxTRANSPARENT );

#ifdef USE_WX_ZOOM
    if( aDrawPanel->GetScreen() != NULL )
    {
        double scale = aDrawPanel->GetScreen()->GetScalingFactor();

        aDrawPanel->SetScale( scale, scale );
        aDrawPanel->DoPrepareDC( *this );
        wxPoint origin = aDrawPanel->GetScreen()->m_DrawOrg;
        SetLogicalOrigin( origin.x, origin.y );
    }
#endif

    aDrawPanel->SetBoundaryBox( this );
}


KicadGraphicContext::~KicadGraphicContext()
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
    m_Parent = parent;
    wxASSERT( m_Parent );

    m_scrollIncrementX = MIN( size.x / 8, 10 );
    m_scrollIncrementY = MIN( size.y / 8, 10 );

    SetBackgroundColour( wxColour( ColorRefs[g_DrawBgColor].m_Red,
                                   ColorRefs[g_DrawBgColor].m_Green,
                                   ColorRefs[g_DrawBgColor].m_Blue ) );
#if defined KICAD_USE_BUFFERED_DC || defined KICAD_USE_BUFFERED_PAINTDC
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );
#endif
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
    m_CursorLevel     = 0;
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
void WinEDA_DrawPanel::DrawCursor( wxDC* aDC, int aColor )
{
    if( m_CursorLevel != 0 || aDC == NULL )
        return;

    wxPoint Cursor = GetScreen()->m_Curseur;

    GRSetDrawMode( aDC, GR_XOR );

    if( m_Parent->m_CursorShape == 1 )    /* Draws a crosshair. */
    {
#ifdef USE_WX_ZOOM
        wxSize clientSize = GetClientSize();
        wxPoint lineStart = wxPoint( Cursor.x, aDC->DeviceToLogicalY( 0 ) );
        wxPoint lineEnd = wxPoint( Cursor.x, aDC->DeviceToLogicalY( clientSize.y ) );
        GRLine( &m_ClipBox, aDC, lineStart, lineEnd, 0, aColor );  // Y azis
        lineStart = wxPoint( aDC->DeviceToLogicalX( 0 ), Cursor.y );
        lineEnd = wxPoint( aDC->DeviceToLogicalX( clientSize.x ), Cursor.y );
        GRLine( &m_ClipBox, aDC, lineStart, lineEnd, 0, aColor );  // X azis
#else
        int dx = GetScreen()->Unscale( m_ClipBox.GetWidth() );
        int dy = GetScreen()->Unscale( m_ClipBox.GetHeight() );
        GRLine( &m_ClipBox, aDC, Cursor.x - dx, Cursor.y,
                Cursor.x + dx, Cursor.y, 0, aColor );            // Y axis
        GRLine( &m_ClipBox, aDC, Cursor.x, Cursor.y - dx,
                Cursor.x, Cursor.y + dy, 0, aColor );            // X axis
#endif
    }
    else
    {
#ifdef USE_WX_ZOOM
        int len = aDC->DeviceToLogicalXRel( CURSOR_SIZE );
#else
        int len = GetScreen()->Unscale( CURSOR_SIZE );
#endif

        GRLine( &m_ClipBox, aDC, Cursor.x - len, Cursor.y,
                Cursor.x + len, Cursor.y, 0, aColor );
        GRLine( &m_ClipBox, aDC, Cursor.x, Cursor.y - len,
                Cursor.x, Cursor.y + len, 0, aColor );
    }
}


/*
 * Remove the grid cursor from the display in preparation for other drawing
 * operations
 */
void WinEDA_DrawPanel::CursorOff( wxDC* DC )
{
    DrawCursor( DC );
    --m_CursorLevel;
}


/*
 *  Display the grid cursor
 */
void WinEDA_DrawPanel::CursorOn( wxDC* DC )
{
    ++m_CursorLevel;
    DrawCursor( DC );

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
wxPoint WinEDA_DrawPanel::CursorRealPosition( const wxPoint& aPosition )
{
#ifdef USE_WX_ZOOM
    wxCoord x, y;
    INSTALL_DC( DC, this );

    x = DC.DeviceToLogicalX( aPosition.x );
    y = DC.DeviceToLogicalY( aPosition.y );
    return wxPoint( x, y );
#else
    return GetScreen()->CursorRealPosition( aPosition );
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

    INSTALL_DC( dc, this );        // Refresh the boundary box.

    display_rect = m_ClipBox;

    // Slightly decreased the size of the useful screen area  to avoid drawing
    // limits.
    #define PIXEL_MARGIN 8
    display_rect.Inflate( -PIXEL_MARGIN );

#ifndef USE_WX_ZOOM
    // Convert physical coordinates.
    pos = CalcUnscrolledPosition( display_rect.GetPosition() );

    GetScreen()->Unscale( pos );
    pos += GetScreen()->m_DrawOrg;
    display_rect.m_Pos = pos;
    GetScreen()->Unscale( display_rect.m_Size );
#endif

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


/**
 * Scale and offset a rectangle in drawing units to device units.
 *
 * This is the equivalent of wxDC::LogicalToDevice.
 *
 * @param aRect - Rectangle to scale.
 */
void WinEDA_DrawPanel::ConvertPcbUnitsToPixelsUnits( EDA_Rect* aRect )
{
    // Calculate the draw area origin in internal units:
    wxPoint pos = aRect->GetPosition();

    ConvertPcbUnitsToPixelsUnits( &pos );
    aRect->SetOrigin( pos );                // rect origin in pixel units

#if USE_WX_ZOOM
    double scale = GetScreen()->GetScalingFactor();
    aRect->m_Size.x = wxRound( (double) aRect->m_Size.x * scale );
    aRect->m_Size.y = wxRound( (double) aRect->m_Size.y * scale );
#else
    GetScreen()->Scale( aRect->m_Size );
#endif
}


void WinEDA_DrawPanel::ConvertPcbUnitsToPixelsUnits( wxPoint* aPosition )
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

#if USE_WX_ZOOM
    INSTALL_DC( dc, this );

    drwOrig.x = dc.DeviceToLogicalX( drwOrig.x );
    drwOrig.y = dc.DeviceToLogicalY( drwOrig.y );
    *aPosition -= drwOrig;
    aPosition->x = dc.LogicalToDeviceX( aPosition->x );
    aPosition->y = dc.LogicalToDeviceY( aPosition->y );
#else
    // Origin in internal units
    GetScreen()->Unscale( drwOrig );

    // Real origin, according to the "plot" origin
    drwOrig += GetScreen()->m_DrawOrg;

    // position in internal units, relative to the visible draw area origin
    *aPosition -= drwOrig;

    // position in pixels, relative to the visible draw area origin
    GetScreen()->Scale( *aPosition );
#endif
}


/** Function CursorScreenPosition
 * @return the cursor current position in pixels in the screen draw area
 */
wxPoint WinEDA_DrawPanel::CursorScreenPosition()
{
    wxPoint pos = GetScreen()->m_Curseur - GetScreen()->m_DrawOrg;

#ifdef USE_WX_ZOOM
    INSTALL_DC( DC, this );

    pos.x = DC.LogicalToDeviceXRel( pos.x );
    pos.y = DC.LogicalToDeviceYRel( pos.y );
#else
    GetScreen()->Scale( pos );
#endif

    return pos;
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

#ifdef USE_WX_ZOOM
    INSTALL_DC( DC, this );

    realpos.x = DC.DeviceToLogicalX( size.x );
    realpos.y = DC.DeviceToLogicalY( size.y );
#else
    realpos = CalcUnscrolledPosition( wxPoint( size.x, size.y ) );
    GetScreen()->Unscale( realpos );
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

#ifdef USE_WX_ZOOM
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
            y -= m_scrollIncrementY * yPpu;
        else if( screenPos.y > clientRect.GetBottom() )
            y += m_scrollIncrementY * yPpu;
        else if( clientRect.GetRight() < screenPos.x )
            x += m_scrollIncrementX * xPpu;
        else
            x -= m_scrollIncrementX * xPpu;

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
        value = -m_scrollIncrementY;

    else if( id == wxEVT_SCROLLWIN_LINEDOWN )
        value = m_scrollIncrementY;

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
#if !defined( USE_WX_GRAPHICS_CONTEXT )   // Crashes Cairo on initial size event.
    INSTALL_DC( dc, this );     // Update boundary box.
#endif
    event.Skip();
}


/** Function SetBoundaryBox()
 * Set the clip box to the current displayed rectangle dimensions.
 *
 * When using wxDC for scaling, the clip box coordinates are in drawing (logical)
 * units.  In other words, the area of the drawing that will be displayed on the
 * screen.  When using Kicad's scaling, the clip box coordinates are in screen
 * (device) units according to the current scroll position.
 *
 * @param dc - The device context use for drawing with the correct scale and
 *             offsets already configured.  See DoPrepareDC().
 */
void WinEDA_DrawPanel::SetBoundaryBox( wxDC* dc )
{
    wxASSERT( dc != NULL );

    BASE_SCREEN* Screen = GetScreen();;

    if( !Screen )
        return;

    Screen->m_StartVisu = CalcUnscrolledPosition( wxPoint( 0, 0 ) );
    m_ClipBox.SetOrigin( wxPoint( 0, 0 ) );
    m_ClipBox.SetSize( GetClientSize() );

    int scrollX, scrollY;

#ifdef USE_WX_ZOOM
    scrollX = dc->LogicalToDeviceXRel( wxRound( Screen->GetGridSize().x ) );
    scrollY = dc->LogicalToDeviceYRel( wxRound( Screen->GetGridSize().y ) );
#else
    scrollX = wxRound( Screen->Scale( Screen->GetGridSize().x ) );
    scrollY = wxRound( Screen->Scale( Screen->GetGridSize().y ) );
#endif

    m_scrollIncrementX = MAX( GetClientSize().x / 8, scrollX );
    m_scrollIncrementY = MAX( GetClientSize().y / 8, scrollY );


#ifdef USE_WX_ZOOM
    /* Using wxDC scaling requires clipping in drawing (logical) units. */

    m_ClipBox.m_Pos.x = dc->DeviceToLogicalX( 0 );
    m_ClipBox.m_Pos.y = dc->DeviceToLogicalY( 0 );
    m_ClipBox.m_Size.x = dc->DeviceToLogicalXRel( m_ClipBox.m_Size.x );
    m_ClipBox.m_Size.y = dc->DeviceToLogicalYRel( m_ClipBox.m_Size.y );

    /* Set to one (1) to draw bounding box validate bounding box calculation. */
#if 0
    EDA_Rect bBox = m_ClipBox;
    m_ClipBox.Inflate( -dc->DeviceToLogicalXRel( 1 ) );
    GRRect( NULL, dc, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif

    m_ClipBox.Inflate( dc->DeviceToLogicalXRel( 1 ) );

    /* Always set the clipping region to the screen size.  This prevents this bug:
     * <http://trac.wxwidgets.org/ticket/10446> from occurring on WXMSW if you happen
     * to be zoomed way in and your drawing coodinates get too large.
     */
    dc->SetClippingRegion( m_ClipBox );
#endif

    Screen->m_ScrollbarPos.x = GetScrollPos( wxHORIZONTAL );
    Screen->m_ScrollbarPos.y = GetScrollPos( wxVERTICAL );
}


void WinEDA_DrawPanel::EraseScreen( wxDC* DC )
{
    GRSetDrawMode( DC, GR_COPY );

    GRSFilledRect( &m_ClipBox, DC, m_ClipBox.GetX(), m_ClipBox.GetY(),
                   m_ClipBox.GetRight(), m_ClipBox.GetBottom(),
                   0, g_DrawBgColor, g_DrawBgColor );
}


void WinEDA_DrawPanel::DoPrepareDC(wxDC& dc)
{
#ifdef USE_WX_ZOOM
    if( GetScreen() != NULL )
    {
        double scale = GetScreen()->GetScalingFactor();

        SetScale( scale, scale );
        wxScrolledWindow::DoPrepareDC( dc );
        wxPoint origin = GetScreen()->m_DrawOrg;
        dc.SetLogicalOrigin( origin.x, origin.y );
    }
#endif

    GRResetPenAndBrush( &dc );
    dc.SetBackgroundMode( wxTRANSPARENT );
    SetBoundaryBox( &dc );
}


void WinEDA_DrawPanel::OnPaint( wxPaintEvent& event )
{
    if( GetScreen() == NULL )
    {
        event.Skip();
        return;
    }

    INSTALL_PAINTDC( paintDC, this );

    /* wxAutoBufferedPaintDC does not work correctly by setting the user scale and
     * logcial offset.  The bitmap coordinates and scaling are not effected by the
     * code below.  It appears that the wxBufferPaintDC needs to be created with the
     * wxBUFFER_VIRTUAL_AREA set and the wirtual method wxWindow::PrepareDC() needs
     * to be overridden to set up the buffered paint DC properly.  The bitmap grid
     * draw code ( see DrawGrid() below ) will have to be fixed before this can be
     * implemented.
     */

    EDA_Rect tmp = m_ClipBox;

    // Get the union of all rectangles in the update region.
    wxRect PaintClipBox = GetUpdateRegion().GetBox();

#if 0
    wxLogDebug( wxT( "1) PaintClipBox=(%d, %d, %d, %d), m_ClipBox=(%d, %d, %d, %d)" ),
                PaintClipBox.x, PaintClipBox.y, PaintClipBox.width, PaintClipBox.height,
                m_ClipBox.m_Pos.x, m_ClipBox.m_Pos.y, m_ClipBox.m_Size.x, m_ClipBox.m_Size.y );
#endif

#if defined( USE_WX_ZOOM )
    /* When using wxDC scaling the clipping region coordinates are in drawing
     * (logical) units.
     */
    m_ClipBox.m_Pos.x = paintDC.DeviceToLogicalX( PaintClipBox.x );
    m_ClipBox.m_Pos.y = paintDC.DeviceToLogicalY( PaintClipBox.y );
    m_ClipBox.m_Size.x = paintDC.DeviceToLogicalXRel( PaintClipBox.width );
    m_ClipBox.m_Size.y = paintDC.DeviceToLogicalYRel( PaintClipBox.height );

#if 0
    EDA_Rect bBox = m_ClipBox;
    m_ClipBox.Inflate( -paintDC.DeviceToLogicalXRel( 1 ) );
    GRRect( NULL, &paintDC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif

    m_ClipBox.Inflate( paintDC.DeviceToLogicalXRel( 1 ) );
    PaintClipBox = m_ClipBox;
#else
    /* When using Kicad's scaling the clipping region coordinates are in screen
     * (device) units.
     */
    m_ClipBox.SetX( PaintClipBox.GetX() );
    m_ClipBox.SetY( PaintClipBox.GetY() );
    m_ClipBox.SetWidth( PaintClipBox.GetWidth() );
    m_ClipBox.SetHeight( PaintClipBox.GetHeight() );

    // Be sure the drawpanel clipbox is bigger than the region to repair:
    m_ClipBox.Inflate( 1 ); // Give it one pixel more in each direction
#endif

#if 0
    wxLogDebug( wxT( "2) PaintClipBox=(%d, %d, %d, %d), m_ClipBox=(%d, %d, %d, %d)" ),
                PaintClipBox.x, PaintClipBox.y, PaintClipBox.width, PaintClipBox.height,
                m_ClipBox.m_Pos.x, m_ClipBox.m_Pos.y, m_ClipBox.m_Size.x, m_ClipBox.m_Size.y );
#endif

    // call ~wxDCClipper() before ~wxPaintDC()
    {
        wxDCClipper dcclip( paintDC, PaintClipBox );
        ReDraw( &paintDC, true );
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
 * Draws (if allowed) :
 * the grid
 * X and Y axis
 * X and Y auxiliary axis
 */
void WinEDA_DrawPanel::DrawBackGround( wxDC* DC )
{
    int          axis_color = BLUE;
    BASE_SCREEN* screen     = GetScreen();

    GRSetDrawMode( DC, GR_COPY );

    if( m_Parent->IsGridVisible() )
        DrawGrid( DC );

    /* Draw axis */
    if( m_Parent->m_Draw_Axis )
    {
        /* Draw the Y axis */
        GRDashedLine( &m_ClipBox, DC, 0, -screen->ReturnPageSize().y,
                      0, screen->ReturnPageSize().y, 0, axis_color );

        /* Draw the X axis */
        GRDashedLine( &m_ClipBox, DC, -screen->ReturnPageSize().x, 0,
                      screen->ReturnPageSize().x, 0, 0, axis_color );
    }

    if( m_Parent->m_Draw_Auxiliary_Axis )
        DrawAuxiliaryAxis( DC, GR_COPY );
}


/**  Function DrawGrid
 * @param DC = current Device Context
 * draws the grid
 *  - the grid is drawn only if the zoom level allows a good visibility
 *  - the grid is always centered on the screen center
 */
void WinEDA_DrawPanel::DrawGrid( wxDC* DC )
{
    BASE_SCREEN* screen = GetScreen();
    int          ii, jj, xg, yg;
    wxRealPoint  screen_grid_size;
    wxSize       size;
    wxPoint      org;

    /* The grid must be visible. this is possible only is grid value
     * and zoom value are sufficient
     */
    screen_grid_size = screen->GetGridSize();
    org = CalcUnscrolledPosition( wxPoint( 0, 0 ) );
    screen->m_StartVisu = org;
    size = GetClientSize();

#ifdef USE_WX_ZOOM
    if( DC->LogicalToDeviceXRel( wxRound( screen_grid_size.x ) ) < 5
        || DC->LogicalToDeviceXRel( wxRound( screen_grid_size.y ) ) < 5 )
        return;

    org = m_ClipBox.m_Pos;
    size = m_ClipBox.m_Size;
#else
    wxRealPoint dgrid = screen_grid_size;
    screen->Scale( dgrid );     // dgrid = grid size in pixels

    // if the grid size is small ( < 5 pixels) do not display all points
    if( dgrid.x < 5 )
    {
        screen_grid_size.x *= 2;
        dgrid.x *= 2;
    }
    if( dgrid.x < 5 )
        return; // The grid is too small: do not show it

    if( dgrid.y < 5 )
    {
        screen_grid_size.y *= 2;
        dgrid.y *= 2;
    }
    if( dgrid.y < 5 )
        return; // The grid is too small


    screen->Unscale( size );
    screen->Unscale( org );
    org += screen->m_DrawOrg;
#endif

    m_Parent->PutOnGrid( &org );
    GRSetColorPen( DC, m_Parent->GetGridColor() );
    int xpos, ypos;


    // Draw grid: the best algorithm depend on the platform.
    // under macOSX, the first method is better
    // under window, the second method is better
    // Under linux, to be tested (could be depend on linux versions
    // so perhaps could be necessary to set this option at run time.

#if defined( __WXMAC__ )
    wxWindowUpdateLocker( this );   // under macOSX: drawings are faster with this
#endif


    /* The bitmap grid drawing code below cannot be used when wxDC scaling is used
     * as it does not scale the grid bitmap properly.  This needs to be fixed.
     */

#if defined( __WXMAC__ ) || defined( USE_WX_ZOOM )
    // Use a pixel based draw to display grid
    // There is a lot of calls, so the cost is hight
    // and grid is slowly drawn on some platforms
    for( ii = 0; ; ii++ )
    {
        xg = wxRound( ii * screen_grid_size.x );
        if( xg > size.x )
            break;
        xpos = org.x + xg;
        xpos = GRMapX( xpos );
        for( jj = 0; ; jj++ )
        {
            yg = wxRound( jj * screen_grid_size.y );
            if( yg > size.y )
                break;
            ypos = org.y + yg;
            DC->DrawPoint( xpos, GRMapY( ypos ) );
        }
    }

#else

    /* Currently on test: Use a fast way to draw the grid
     * But this is fast only if the Blit function is fast. Not true on all platforms
     * a grid column is drawn; and then copied to others grid columns
     * this is possible because the grid is drawn only after clearing the screen.
     *
     * A first grid column is drawn in a temporary bitmap,
     * and after is duplicated using the Blit function
     * (copy from a screen area to an other screen area)
     */

    wxSize screenSize = GetClientSize();
    wxMemoryDC tmpDC;
    wxBitmap tmpBM( 1, screenSize.y );
    tmpDC.SelectObject( tmpBM );
    GRSetColorPen( &tmpDC, g_DrawBgColor );
    tmpDC.DrawLine( 0, 0, 0, screenSize.y-1 );        // init background
    GRSetColorPen( &tmpDC, m_Parent->GetGridColor() );
    for( jj = 0; ; jj++ )   // draw grid points
    {
        yg = wxRound( jj * screen_grid_size.y );
        ypos = screen->Scale( yg );
        if( ypos > screenSize.y )
            break;
        tmpDC.DrawPoint( 0, ypos );
    }

    ypos = GRMapY( org.y );
    for( ii = 0; ; ii++ )
    {
        xg = wxRound( ii * screen_grid_size.x );
        if( xg > size.x )
            break;
        xpos = GRMapX( org.x + xg );
        if( xpos < m_ClipBox.GetOrigin().x) // column not in active screen area.
        if( xpos > m_ClipBox.GetEnd().x)    // end of active area reached.
            break;
        DC->Blit( xpos, ypos, 1, screenSize.y, &tmpDC, 0, 0  );
    }

#endif
}


/** function DrawAuxiliaryAxis
 * Draw the Auxiliary Axis, used in pcbnew which as origin coordinates
 * for gerber and excellon files
 * @param DC = current Device Context
 */
void WinEDA_DrawPanel::DrawAuxiliaryAxis( wxDC* DC, int drawmode )
{
    if( m_Parent->m_Auxiliary_Axis_Position == wxPoint( 0, 0 ) )
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

    m_Parent->AddMenuZoomAndGrid( &MasterMenu );

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
    if( m_IgnoreMouseEvents )
        return;

    wxRect rect = wxRect( wxPoint( 0, 0 ), GetClientSize() );

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

#ifdef USE_WX_ZOOM
    GetScreen()->m_Curseur = CursorRealPosition( event.GetPosition() );
#else
    GetScreen()->m_Curseur =
        CursorRealPosition( CalcUnscrolledPosition( event.GetPosition() ) );
#endif

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
    else if( event.GetWheelRotation() < 0 )
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

#ifdef USE_WX_ZOOM
    /* Compute the cursor position in screen (device) units. */
    screen->m_MousePositionInPixels = event.GetPosition();

    /* Compute the cursor position in drawing (logical) units. */
    screen->m_MousePosition = CursorRealPosition( event.GetPosition() );
#else
    /* Compute the cursor position in screen (device) units. */
    screen->m_MousePositionInPixels = CalcUnscrolledPosition( event.GetPosition() );

    /* Compute the cursor position in drawing (logical) units. */
    screen->m_MousePosition =
        CursorRealPosition( CalcUnscrolledPosition( event.GetPosition() ) );
#endif

    INSTALL_DC( DC, this );

    int kbstat = 0;

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
    if( localbutt == (int) ( GR_M_LEFT_DOWN | GR_M_DCLICK ) )
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
            {
                //  Start a block command
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
    long key, localkey;
    bool escape = false;
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

    /* Normalize keys code to easily handle keys from Ctrl+A to Ctrl+Z
     * They have an ascii code from 1 to 27 remapped
     * GR_KB_CTRL + 'A' to GR_KB_CTRL + 'Z'
     */
    if( (localkey & (GR_KB_CTRL|GR_KB_ALT|GR_KB_SHIFT)) == GR_KB_CTRL )
        localkey += 'A' - 1;

    INSTALL_DC( DC, this );

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
#ifdef USE_WX_ZOOM
    pos = wxGetMousePosition() - GetScreenPosition();
#else
    pos = CalcUnscrolledPosition( wxGetMousePosition() - GetScreenPosition() );
#endif

    /* Compute cursor position in screen units (pixel) including the
     * current scroll bar position.   Also known as device units to wxDC. */
    Screen->m_MousePositionInPixels = pos;

    /* Compute the cursor position in drawing units.  Also known as logical units
     * to wxDC. */
    Screen->m_MousePosition = CursorRealPosition( pos );

    m_Parent->GeneralControle( &DC, pos );

#if 0
    event.Skip();   // Allow menu shortcut processing
#endif
}


void WinEDA_DrawPanel::OnPan( wxCommandEvent& event )
{
    int x, y;

    GetViewStart( &x, &y );       // x and y are in scroll units, not in pixels

    switch( event.GetId() )
    {
    case ID_PAN_UP:
        y -= m_scrollIncrementY;
        break;

    case ID_PAN_DOWN:
        y += m_scrollIncrementY;
        break;

    case ID_PAN_LEFT:
        x -= m_scrollIncrementX;
        break;

    case ID_PAN_RIGHT:
        x += m_scrollIncrementX;
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
