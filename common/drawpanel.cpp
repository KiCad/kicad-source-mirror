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

#define CLIP_BOX_PADDING  12


/* Definitions for enabling and disabling debugging features in drawpanel.cpp.
 * Please don't forget to turn these off before making any SvN commits.
 */

#define DEBUG_SHOW_CLIP_RECT       0  // Set to 1 to draw clipping rectangle.
#define DEBUG_DUMP_CLIP_COORDS     0  // Set to 1 to dump clipping rectangle coordinates.
#define DEBUG_DUMP_SCROLL_SETTINGS 0  // Set to 1 to dump scroll settings.


/* Used to inhibit a response to a mouse left button release, after a
 * double click (when releasing the left button at the end of the second
 * click.  Used in eeschema to inhibit a mouse left release command when
 * switching between hierarchical sheets on a double click.
 */
static bool s_IgnoreNextLeftButtonRelease = false;


// Events used by EDA_DRAW_PANEL
BEGIN_EVENT_TABLE( EDA_DRAW_PANEL, wxScrolledWindow )
    EVT_LEAVE_WINDOW( EDA_DRAW_PANEL::OnMouseLeaving )
    EVT_MOUSEWHEEL( EDA_DRAW_PANEL::OnMouseWheel )
    EVT_MOUSE_EVENTS( EDA_DRAW_PANEL::OnMouseEvent )
    EVT_CHAR( EDA_DRAW_PANEL::OnKeyEvent )
    EVT_CHAR_HOOK( EDA_DRAW_PANEL::OnKeyEvent )
    EVT_PAINT( EDA_DRAW_PANEL::OnPaint )
    EVT_SIZE( EDA_DRAW_PANEL::OnSize )
    EVT_SCROLLWIN( EDA_DRAW_PANEL::OnScroll )
    EVT_ACTIVATE( EDA_DRAW_PANEL::OnActivate )
    EVT_MENU_RANGE( ID_PAN_UP, ID_PAN_RIGHT, EDA_DRAW_PANEL::OnPan )
END_EVENT_TABLE()

/***********************************************************************/
/* EDA_DRAW_PANEL base functions (EDA_DRAW_PANEL is the main panel)*/
/***********************************************************************/

EDA_DRAW_PANEL::EDA_DRAW_PANEL( EDA_DRAW_FRAME* parent, int id,
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
    m_DisableEraseBG    = false;

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


EDA_DRAW_PANEL::~EDA_DRAW_PANEL()
{
    wxGetApp().m_EDA_Config->Write( wxT( "AutoPAN" ), m_AutoPAN_Enable );
}


BASE_SCREEN* EDA_DRAW_PANEL::GetScreen()
{
    EDA_DRAW_FRAME* parentFrame = m_Parent;

    return parentFrame->GetBaseScreen();
}


/*
 *  Draw the schematic cursor which is usually on grid
 */
void EDA_DRAW_PANEL::DrawCursor( wxDC* aDC, int aColor )
{
    if( m_CursorLevel != 0 || aDC == NULL )
        return;

#ifdef __WXMAC__
    SetCursor(*wxCROSS_CURSOR);
    return;
#endif

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
void EDA_DRAW_PANEL::CursorOff( wxDC* DC )
{
    DrawCursor( DC );
    --m_CursorLevel;
}


/*
 *  Display the grid cursor
 */
void EDA_DRAW_PANEL::CursorOn( wxDC* DC )
{
    ++m_CursorLevel;
    DrawCursor( DC );

    if( m_CursorLevel > 0 )  // Shouldn't happen, but just in case ..
        m_CursorLevel = 0;
}


int EDA_DRAW_PANEL::GetZoom()
{
    return GetScreen()->GetZoom();
}


void EDA_DRAW_PANEL::SetZoom( int zoom )
{
    GetScreen()->SetZoom( zoom );
}


wxRealPoint EDA_DRAW_PANEL::GetGrid()
{
    return GetScreen()->GetGridSize();
}


/**
 * Convert a coordinate position in device (screen) units to logical (drawing) units.
 *
 * @param  aPosition = position in device (screen) units.
 * @return  position in logical (drawing) units.
 */
wxPoint EDA_DRAW_PANEL::CursorRealPosition( const wxPoint& aPosition )
{
    double scalar = GetScreen()->GetScalingFactor();
    wxPoint pos;
    pos.x = wxRound( (double) aPosition.x / scalar );
    pos.y = wxRound( (double) aPosition.y / scalar );
    pos += GetScreen()->m_DrawOrg;
    return pos;
}


/**
 * Function IsPointOnDisplay
 * @param ref_pos is the position to test in pixels, relative to the panel.
 * @return TRUE if ref_pos is a point currently visible on screen
 *         false if ref_pos is out of screen
 */
bool EDA_DRAW_PANEL::IsPointOnDisplay( wxPoint ref_pos )
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

    return display_rect.Contains( ref_pos );
}


void EDA_DRAW_PANEL::PostDirtyRect( EDA_Rect aRect )
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
void EDA_DRAW_PANEL::ConvertPcbUnitsToPixelsUnits( EDA_Rect* aRect )
{
    // Calculate the draw area origin in internal units:
    wxPoint pos = aRect->GetPosition();

    ConvertPcbUnitsToPixelsUnits( &pos );
    aRect->SetOrigin( pos );                // rect origin in pixel units

    double scale = GetScreen()->GetScalingFactor();
    aRect->m_Size.x = wxRound( (double) aRect->m_Size.x * scale );
    aRect->m_Size.y = wxRound( (double) aRect->m_Size.y * scale );
}


void EDA_DRAW_PANEL::ConvertPcbUnitsToPixelsUnits( wxPoint* aPosition )
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

    double x, y;
    double scalar = GetScreen()->GetScalingFactor();
    x = (double) aPosition->x - ( ( (double) drwOrig.x / scalar )
                                  + (double) GetScreen()->m_DrawOrg.x );
    y = (double) aPosition->y - ( ( (double) drwOrig.y / scalar )
                                  + (double) GetScreen()->m_DrawOrg.y );
    aPosition->x = wxRound( x * scalar );
    aPosition->y = wxRound( y * scalar );
}


/**
 * Function CursorScreenPosition
 * @return the cursor current position in pixels in the screen draw area
 */
wxPoint EDA_DRAW_PANEL::CursorScreenPosition()
{
    wxPoint pos = GetScreen()->m_Curseur - GetScreen()->m_DrawOrg;
    double scalar = GetScreen()->GetScalingFactor();

    pos.x = wxRound( (double) pos.x * scalar );
    pos.y = wxRound( (double) pos.y * scalar );

    return pos;
}


/**
 * Function GetScreenCenterRealPosition
 * @return position (in internal units) of the current area center showed
 *         on screen
 */
wxPoint EDA_DRAW_PANEL::GetScreenCenterRealPosition( void )
{
    int x, y, ppuX, ppuY;
    wxPoint pos;
    double  scalar = GetScreen()->GetScalingFactor();

    GetViewStart( &x, &y );
    GetScrollPixelsPerUnit( &ppuX, &ppuY );
    x *= ppuX;
    y *= ppuY;
    pos.x = wxRound( ( (double) GetClientSize().x / 2.0 + (double) x ) / scalar );
    pos.y = wxRound( ( (double) GetClientSize().y / 2.0 + (double) y ) / scalar );
    pos += GetScreen()->m_DrawOrg;

    return pos;
}


/* Move the mouse cursor to the current schematic cursor
 */
void EDA_DRAW_PANEL::MouseToCursorSchema()
{
    wxPoint Mouse = CursorScreenPosition();

    MouseTo( Mouse );
}


/** Move the mouse cursor to the position "Mouse"
 * @param Mouse = mouse cursor position, in pixels units
 */
void EDA_DRAW_PANEL::MouseTo( const wxPoint& Mouse )
{
    int     x, y, xPpu, yPpu;
    wxPoint screenPos, drawingPos;
    wxRect  clientRect( wxPoint( 0, 0 ), GetClientSize() );

    CalcScrolledPosition( Mouse.x, Mouse.y, &screenPos.x, &screenPos.y );

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
void EDA_DRAW_PANEL::OnActivate( wxActivateEvent& event )
{
    m_CanStartBlock = -1;   // Block Command can't start
    event.Skip();
}

void EDA_DRAW_PANEL::OnScroll( wxScrollWinEvent& event )
{
    int id = event.GetEventType();
    int dir;
    int x, y;
    int ppux, ppuy;
    int unitsX, unitsY;
    int maxX, maxY;

    GetViewStart( &x, &y );
    GetScrollPixelsPerUnit( &ppux, &ppuy );
    GetVirtualSize( &unitsX, &unitsY );
    maxX = unitsX;
    maxY = unitsY;

    unitsX /= ppux;
    unitsY /= ppuy;

    dir = event.GetOrientation();   // wxHORIZONTAL or wxVERTICAL

    if( id == wxEVT_SCROLLWIN_LINEUP )
    {
        if( dir == wxHORIZONTAL )
        {
            x -= m_scrollIncrementX;
            if( x < 0 )
                x = 0;
        }
        else
        {
            y -= m_scrollIncrementY;
            if( y < 0 )
                y = 0;
        }
    }
    else if( id == wxEVT_SCROLLWIN_LINEDOWN )
    {
        if( dir == wxHORIZONTAL )
        {
            x += m_scrollIncrementX;
            if( x > maxX )
                x = maxX;
        }
        else
        {
            y += m_scrollIncrementY;
            if( y > maxY )
                y = maxY;
        }
    }
    else if( id == wxEVT_SCROLLWIN_THUMBTRACK )
    {
        if( dir == wxHORIZONTAL )
            x = event.GetPosition();
        else
            y = event.GetPosition();
    }
    else
    {
        event.Skip();
        return;
    }

#if DEBUG_DUMP_SCROLL_SETTINGS
    wxLogDebug( wxT( "Setting scroll bars ppuX=%d, ppuY=%d, unitsX=%d, unitsY=%d," \
                     "posX=%d, posY=%d" ), ppux, ppuy, unitsX, unitsY, x, y );
#endif

    Scroll( x/ppux, y/ppuy );
    event.Skip();
}

void EDA_DRAW_PANEL::OnSize( wxSizeEvent& event )
{
    if( IsShown() )
    {
        INSTALL_DC( dc, this );     // Update boundary box.
    }

    event.Skip();
}


/**
 * Function SetBoundaryBox
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
void EDA_DRAW_PANEL::SetBoundaryBox( wxDC* dc )
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
    double scalar = Screen->GetScalingFactor();
    scrollX = wxRound( Screen->GetGridSize().x * scalar );
    scrollY = wxRound( Screen->GetGridSize().y * scalar );
#else
    scrollX = wxRound( Screen->Scale( Screen->GetGridSize().x ) );
    scrollY = wxRound( Screen->Scale( Screen->GetGridSize().y ) );
#endif

    m_scrollIncrementX = MAX( GetClientSize().x / 8, scrollX );
    m_scrollIncrementY = MAX( GetClientSize().y / 8, scrollY );


#ifdef USE_WX_ZOOM
    /* Using wxDC scaling requires clipping in drawing (logical) units. */
    m_ClipBox.SetOrigin( CalcUnscrolledPosition( wxPoint( 0, 0 ) ) );
    m_ClipBox.Inflate( CLIP_BOX_PADDING );
    m_ClipBox.m_Pos.x = wxRound( (double) m_ClipBox.m_Pos.x / scalar );
    m_ClipBox.m_Pos.y = wxRound( (double) m_ClipBox.m_Pos.y / scalar );
    m_ClipBox.m_Pos += Screen->m_DrawOrg;
    m_ClipBox.m_Size.x = wxRound( (double) m_ClipBox.m_Size.x / scalar );
    m_ClipBox.m_Size.y = wxRound( (double) m_ClipBox.m_Size.y / scalar );
#endif

    Screen->m_ScrollbarPos.x = GetScrollPos( wxHORIZONTAL );
    Screen->m_ScrollbarPos.y = GetScrollPos( wxVERTICAL );
}


void EDA_DRAW_PANEL::EraseScreen( wxDC* DC )
{
    GRSetDrawMode( DC, GR_COPY );

    GRSFilledRect( &m_ClipBox, DC, m_ClipBox.GetX(), m_ClipBox.GetY(),
                   m_ClipBox.GetRight(), m_ClipBox.GetBottom(),
                   0, g_DrawBgColor, g_DrawBgColor );

    /* Set to one (1) to draw bounding box validate bounding box calculation. */
#if DEBUG_SHOW_CLIP_RECT
    EDA_Rect bBox = m_ClipBox;
    bBox.Inflate( -DC->DeviceToLogicalXRel( 1 ) );
    GRRect( NULL, DC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif
}


void EDA_DRAW_PANEL::DoPrepareDC(wxDC& dc)
{
#ifdef USE_WX_ZOOM
    if( GetScreen() != NULL )
    {
        double scale = GetScreen()->GetScalingFactor();
        dc.SetUserScale( scale, scale );

        wxPoint pt = CalcUnscrolledPosition( wxPoint( 0, 0 ) );
        dc.SetDeviceOrigin( -pt.x, -pt.y );
        pt = GetScreen()->m_DrawOrg;
        dc.SetLogicalOrigin( pt.x, pt.y );
    }
#endif

    GRResetPenAndBrush( &dc );
    dc.SetBackgroundMode( wxTRANSPARENT );
    SetBoundaryBox( &dc );
}


void EDA_DRAW_PANEL::OnPaint( wxPaintEvent& event )
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

#if DEBUG_DUMP_CLIP_COORDS
    wxLogDebug( wxT( "1) PaintClipBox=(%d, %d, %d, %d), m_ClipBox=(%d, %d, %d, %d)" ),
                PaintClipBox.x, PaintClipBox.y, PaintClipBox.width, PaintClipBox.height,
                m_ClipBox.m_Pos.x, m_ClipBox.m_Pos.y, m_ClipBox.m_Size.x, m_ClipBox.m_Size.y );
#endif

#if defined( USE_WX_ZOOM )
    /* When using wxDC scaling the clipping region coordinates are in drawing
     * (logical) units.
     */
    double scalar = GetScreen()->GetScalingFactor();
    m_ClipBox.m_Pos = CalcUnscrolledPosition( PaintClipBox.GetPosition() );
    m_ClipBox.m_Size = PaintClipBox.GetSize();
    m_ClipBox.Inflate( CLIP_BOX_PADDING );
    m_ClipBox.m_Pos.x = wxRound( (double) m_ClipBox.m_Pos.x / scalar );
    m_ClipBox.m_Pos.y = wxRound( (double) m_ClipBox.m_Pos.y / scalar );
    m_ClipBox.m_Pos += GetScreen()->m_DrawOrg;
    m_ClipBox.m_Size.x = wxRound( (double) m_ClipBox.m_Size.x / scalar );
    m_ClipBox.m_Size.y = wxRound( (double) m_ClipBox.m_Size.y / scalar );
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

#if DEBUG_DUMP_CLIP_COORDS
    wxLogDebug( wxT( "2) PaintClipBox=(%d, %d, %d, %d), m_ClipBox=(%d, %d, %d, %d)" ),
                PaintClipBox.x, PaintClipBox.y, PaintClipBox.width, PaintClipBox.height,
                m_ClipBox.m_Pos.x, m_ClipBox.m_Pos.y, m_ClipBox.m_Size.x, m_ClipBox.m_Size.y );
#endif

    // call ~wxDCClipper() before ~wxPaintDC()
    {
        wxDCClipper dcclip( paintDC, PaintClipBox );
        ReDraw( &paintDC, m_DisableEraseBG ? false : true );
    }

    m_ClipBox = tmp;
    event.Skip();
}


void EDA_DRAW_PANEL::ReDraw( wxDC* DC, bool erasebg )
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


/**
 * Function DrawBackGround
 * @param DC = current Device Context
 * Draws (if allowed) :
 * the grid
 * X and Y axis
 * X and Y auxiliary axis
 */
void EDA_DRAW_PANEL::DrawBackGround( wxDC* DC )
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

    if( m_Parent->m_Draw_Grid_Axis )
        DrawGridAxis( DC, GR_COPY );
}


/**
 * Function DrawGrid
 * @param DC = current Device Context
 * draws the grid
 *  - the grid is drawn only if the zoom level allows a good visibility
 *  - the grid is always centered on the screen center
 */
void EDA_DRAW_PANEL::DrawGrid( wxDC* DC )
{
    #define MIN_GRID_SIZE 10        // min grid size in pixels to allow drawing
    BASE_SCREEN* screen = GetScreen();
    int          ii, jj, xg, yg;
    wxRealPoint  screen_grid_size;
    wxSize       size;
    wxPoint      org;
    wxRealPoint dgrid;

    /* The grid must be visible. this is possible only is grid value
     * and zoom value are sufficient
     */
    screen_grid_size = screen->GetGridSize();
    org = CalcUnscrolledPosition( wxPoint( 0, 0 ) );
    screen->m_StartVisu = org;
    size = GetClientSize();

#ifdef USE_WX_ZOOM
    dgrid.x = DC->LogicalToDeviceXRel( wxRound( screen_grid_size.x ) );
    dgrid.y = DC->LogicalToDeviceXRel( wxRound( screen_grid_size.y ) );

    org = m_ClipBox.m_Pos;
    size = m_ClipBox.m_Size;
#else
    dgrid = screen_grid_size;
    screen->Scale( dgrid );     // dgrid = grid size in pixels
    screen->Unscale( size );
    screen->Unscale( org );
    org += screen->m_DrawOrg;
#endif

    // if the grid size is small ( < MIN_GRID_SIZE pixels ) do not display all points
    bool double_size = false;
    if( dgrid.x < MIN_GRID_SIZE )
    {
        double_size = true;
        dgrid.x *= 2;
    }
    if( dgrid.x < MIN_GRID_SIZE )
        return; // The X grid is too small: do not show it

    if( dgrid.y < MIN_GRID_SIZE )
    {
        double_size = true;
        dgrid.y *= 2;
    }
    if( dgrid.y < MIN_GRID_SIZE )
        return; // The Y grid is too small: do not show it


    m_Parent->PutOnGrid( &org );
    GRSetColorPen( DC, m_Parent->GetGridColor() );
    int xpos, ypos;

    /* When we use an double_size grid, we must align grid ord on double grid
     */
    int increment = double_size ? 2 : 1;
    if( double_size )
    {
        wxRealPoint dblgrid = screen_grid_size + screen_grid_size;
        m_Parent->PutOnGrid( &org, &dblgrid );
    }

    // Draw grid: the best algorithm depend on the platform.
    // under macOSX, the first method is better
    // under window, the second method is better
    // Under linux, to be tested (could be depend on linux versions
    // so perhaps could be necessary to set this option at run time.

    /* The bitmap grid drawing code below cannot be used when wxDC scaling is used
     * as it does not scale the grid bitmap properly.  This needs to be fixed.
     */

#if defined( __WXMAC__ ) && !defined( USE_WX_ZOOM )
    // Use a pixel based draw to display grid
    // When is not used USE_WX_ZOOM
    for( ii = 0; ; ii += increment )
    {
        xg = wxRound( ii * screen_grid_size.x );
        if( xg > size.x )
            break;
        xpos = org.x + xg;
        xpos = GRMapX( xpos );
        for( jj = 0; ; jj += increment )
        {
            yg = wxRound( jj * screen_grid_size.y );
            if( yg > size.y )
                break;
            ypos = org.y + yg;
            DC->DrawPoint( xpos, GRMapY( ypos ) );
        }
    }
#endif

#if defined( USE_WX_ZOOM )
    // Use a pixel based draw to display grid
    // There is a lot of calls, so the cost is hight
    // and grid is slowly drawn on some platforms
    for( ii = 0; ; ii += increment )
    {
        xg = wxRound( ii * screen_grid_size.x );
        if( xg > size.x )
            break;
        xpos = org.x + xg;
        xpos = GRMapX( xpos );
        if( xpos < m_ClipBox.GetOrigin().x ) // column not in active screen area.
            continue;
        if( xpos > m_ClipBox.GetEnd().x )    // end of active area reached.
            break;
        for( jj = 0; ; jj += increment )
        {
            yg = wxRound( jj * screen_grid_size.y );
            if( yg > size.y )
                break;
            ypos = org.y + yg;
            if( ypos < m_ClipBox.GetOrigin().y ) // column not in active screen area.
                continue;
            if( ypos > m_ClipBox.GetEnd().y )    // end of active area reached.
                break;
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
    for( jj = 0; ; jj += increment )   // draw grid points
    {
        yg = wxRound( jj * screen_grid_size.y );
        ypos = screen->Scale( yg );
        if( ypos > screenSize.y )
            break;
        tmpDC.DrawPoint( 0, ypos );
    }

    // Use the layer bitmap itself as a mask when blitting.
    // The bitmap cannot be referenced by a device context
    // when setting the mask.
    tmpDC.SelectObject( wxNullBitmap );
    tmpBM.SetMask( new wxMask( tmpBM, MakeColour( g_DrawBgColor ) ) );
    tmpDC.SelectObject( tmpBM );

    ypos = GRMapY( org.y );
    for( ii = 0; ; ii += increment )
    {
        xg = wxRound( ii * screen_grid_size.x );
        if( xg > size.x )
            break;
        xpos = GRMapX( org.x + xg );
        if( xpos < m_ClipBox.GetOrigin().x) // column not in active screen area.
            continue;
        if( xpos > m_ClipBox.GetEnd().x)    // end of active area reached.
            break;
        DC->Blit( xpos, ypos, 1, screenSize.y, &tmpDC, 0, 0, wxCOPY, true  );
    }

#endif
}


/**
 * Function DrawAuxiliaryAxis
 * Draw the Auxiliary Axis, used in pcbnew which as origin coordinates
 * for gerber and excellon files
 * @param aDC = current Device Context
 * @param aDrawMode = draw mode (GR_COPY, GR_OR ..)
 */
void EDA_DRAW_PANEL::DrawAuxiliaryAxis( wxDC* aDC, int aDrawMode )
{
    if( m_Parent->m_Auxiliary_Axis_Position == wxPoint( 0, 0 ) )
        return;

    int          Color  = DARKRED;
    BASE_SCREEN* screen = GetScreen();

    GRSetDrawMode( aDC, aDrawMode );

    /* Draw the Y axis */
    GRDashedLine( &m_ClipBox, aDC,
                  m_Parent->m_Auxiliary_Axis_Position.x,
                  -screen->ReturnPageSize().y,
                  m_Parent->m_Auxiliary_Axis_Position.x,
                  screen->ReturnPageSize().y,
                  0, Color );

    /* Draw the X axis */
    GRDashedLine( &m_ClipBox, aDC,
                  -screen->ReturnPageSize().x,
                  m_Parent->m_Auxiliary_Axis_Position.y,
                  screen->ReturnPageSize().x,
                  m_Parent->m_Auxiliary_Axis_Position.y,
                  0, Color );
}


void EDA_DRAW_PANEL::DrawGridAxis( wxDC* aDC, int aDrawMode )
{
    BASE_SCREEN* screen = GetScreen();
    if( !m_Parent->m_Draw_Grid_Axis
        || ( screen->m_GridOrigin.x == 0
             && screen->m_GridOrigin.y == 0 ) )
        return;

    int          Color  = m_Parent->GetGridColor();

    GRSetDrawMode( aDC, aDrawMode );

    /* Draw the Y axis */
    GRDashedLine( &m_ClipBox, aDC,
                  screen->m_GridOrigin.x,
                  -screen->ReturnPageSize().y,
                  screen->m_GridOrigin.x,
                  screen->ReturnPageSize().y,
                  0, Color );

    /* Draw the X axis */
    GRDashedLine( &m_ClipBox, aDC,
                  -screen->ReturnPageSize().x,
                  screen->m_GridOrigin.y,
                  screen->ReturnPageSize().x,
                  screen->m_GridOrigin.y,
                  0, Color );
}


/** Build and display a Popup menu on a right mouse button click
 * @return true if a popup menu is shown, or false
 */
bool EDA_DRAW_PANEL::OnRightClick( wxMouseEvent& event )
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
void EDA_DRAW_PANEL::OnMouseLeaving( wxMouseEvent& event )
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
void EDA_DRAW_PANEL::OnMouseWheel( wxMouseEvent& event )
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
    event.Skip();
}


// Called when the canvas receives a mouse event.
void EDA_DRAW_PANEL::OnMouseEvent( wxMouseEvent& event )
{
    int                    localrealbutt = 0, localbutt = 0, localkey = 0;
    BASE_SCREEN*           screen = GetScreen();
    static EDA_DRAW_PANEL* LastPanel;

    if( !screen )
        return;

    /* Adjust value to filter mouse displacement before consider the drag
     * mouse is really a drag command, not just a movement while click
     */
#define MIN_DRAG_COUNT_FOR_START_BLOCK_COMMAND 5

    /* Count the drag events.  Used to filter mouse moves before starting a
     * block command.  A block command can be started only if
     * MinDragEventCount > MIN_DRAG_COUNT_FOR_START_BLOCK_COMMAND
     * and m_CanStartBlock >= 0
     * in order to avoid spurious block commands.
     */
    static int MinDragEventCount;
    if( event.Leaving() )
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

    localrealbutt |= localbutt;     /* compensation default wxGTK */

    /* Compute the cursor position in screen (device) units. */
    screen->m_MousePositionInPixels = CalcUnscrolledPosition( event.GetPosition() );

    /* Compute the cursor position in drawing (logical) units. */
    screen->m_MousePosition =
        CursorRealPosition( CalcUnscrolledPosition( event.GetPosition() ) );

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

    if( event.ButtonUp( wxMOUSE_BTN_MIDDLE )
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
                    if( !m_Parent->HandleBlockBegin( &DC, cmd_type, m_CursorStartPos ) )
                    {
                        // should not occurs: error
                        m_Parent->DisplayToolMsg(
                            wxT( "EDA_DRAW_PANEL::OnMouseEvent() Block Error" ) );
                    }
                    else
                    {
                        m_AutoPAN_Request = TRUE;
                        SetCursor( m_PanelCursor = wxCURSOR_SIZING );
                    }
                }
            }
        }

        if( event.ButtonUp( wxMOUSE_BTN_LEFT ) || event.ButtonUp( wxMOUSE_BTN_MIDDLE ) )
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


void EDA_DRAW_PANEL::OnKeyEvent( wxKeyEvent& event )
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
     * to GR_KB_CTRL + 'A' to GR_KB_CTRL + 'Z'
     */
    if( (localkey > GR_KB_CTRL) && (localkey <= GR_KB_CTRL+26) )
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
    pos = CalcUnscrolledPosition( wxGetMousePosition() - GetScreenPosition() );

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


void EDA_DRAW_PANEL::OnPan( wxCommandEvent& event )
{
    int x, y;
    int ppux, ppuy;
    int unitsX, unitsY;
    int maxX, maxY;

    GetViewStart( &x, &y );
    GetScrollPixelsPerUnit( &ppux, &ppuy );
    GetVirtualSize( &unitsX, &unitsY );
    maxX = unitsX;
    maxY = unitsY;
    unitsX /= ppux;
    unitsY /= ppuy;

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
        wxLogDebug( wxT( "Unknown ID %d in EDA_DRAW_PANEL::OnPan()." ), event.GetId() );
    }

    if( x < 0 )
        x = 0;
    if( y < 0 )
        y = 0;
    if( x > maxX )
        x = maxX;
    if( y > maxY )
        y = maxY;

    Scroll( x/ppux, y/ppuy );
}


void EDA_DRAW_PANEL::UnManageCursor( int id, int cursor, const wxString& title )
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
