/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file draw_panel.cpp
 */

#include <fctsys.h>
#include <wx/timer.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <gr_basic.h>
#include <common.h>
#include <macros.h>
#include <id.h>
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>
#include <base_screen.h>
#include <draw_frame.h>
#include <view/view_controls.h>
#include <gal/gal_display_options.h>
#include <trace_helpers.h>

#include <kicad_device_context.h>

static const int CURSOR_SIZE = 12; ///< Cursor size in pixels

#define CLIP_BOX_PADDING 2

// Definitions for enabling and disabling debugging features in drawpanel.cpp.
// Please don't forget to turn these off before making any commits to Launchpad.
#define DEBUG_SHOW_CLIP_RECT       0  // Set to 1 to draw clipping rectangle.


// Events used by EDA_DRAW_PANEL
BEGIN_EVENT_TABLE( EDA_DRAW_PANEL, wxScrolledWindow )
    EVT_LEAVE_WINDOW( EDA_DRAW_PANEL::OnMouseLeaving )
    EVT_ENTER_WINDOW( EDA_DRAW_PANEL::OnMouseEntering )
    EVT_MOUSEWHEEL( EDA_DRAW_PANEL::OnMouseWheel )
#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
    EVT_MAGNIFY( EDA_DRAW_PANEL::OnMagnify )
#endif
    EVT_PAINT( EDA_DRAW_PANEL::OnPaint )
    EVT_ERASE_BACKGROUND( EDA_DRAW_PANEL::OnEraseBackground )
    EVT_SCROLLWIN( EDA_DRAW_PANEL::OnScroll )
    EVT_ACTIVATE( EDA_DRAW_PANEL::OnActivate )
    EVT_MENU_RANGE( ID_PAN_UP, ID_PAN_RIGHT, EDA_DRAW_PANEL::OnPan )
END_EVENT_TABLE()


/***********************************************************************/
/* EDA_DRAW_PANEL base functions (EDA_DRAW_PANEL is the main panel)*/
/***********************************************************************/

#ifdef __WXMAC__
const int drawPanelStyle = wxHSCROLL | wxVSCROLL | wxALWAYS_SHOW_SB;
#else
const int drawPanelStyle = wxHSCROLL | wxVSCROLL;
#endif

EDA_DRAW_PANEL::EDA_DRAW_PANEL( EDA_DRAW_FRAME* parent, int id,
                                const wxPoint& pos, const wxSize& size ) :
    wxScrolledWindow( parent, id, pos, size, drawPanelStyle )
{
    wxASSERT( parent );

    ShowScrollbars( wxSHOW_SB_ALWAYS, wxSHOW_SB_ALWAYS );
    DisableKeyboardScrolling();

    m_scrollIncrementX = std::min( size.x / 8, 10 );
    m_scrollIncrementY = std::min( size.y / 8, 10 );

    SetLayoutDirection( wxLayout_LeftToRight );

    SetBackgroundColour( parent->GetDrawBgColor().ToColour() );

#if KICAD_USE_BUFFERED_DC || KICAD_USE_BUFFERED_PAINTDC
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );
#endif

    m_ClipBox.SetSize( size );
    m_ClipBox.SetX( 0 );
    m_ClipBox.SetY( 0 );
    m_canStartBlock = -1;       // Command block can start if >= 0
    // Be sure a mouse release button event will be ignored when creating the canvas
    // if the mouse click was not made inside the canvas (can happen sometimes, when
    // launching a editor from a double click made in another frame)
    m_ignoreNextLeftButtonRelease = true;

    m_minDragEventCount = 0;

#ifdef __WXMAC__
    m_defaultCursor = m_currentCursor = wxCURSOR_CROSS;
    m_showCrossHair = false;
#else
    m_defaultCursor = m_currentCursor = wxCURSOR_ARROW;
    m_showCrossHair = true;
#endif

    m_cursorLevel = 0;
    m_PrintIsMirrored = false;

    m_ClickTimer = (wxTimer*) NULL;
    m_doubleClickInterval = 250;
}


EDA_DRAW_PANEL::~EDA_DRAW_PANEL()
{
    wxDELETE( m_ClickTimer );
}


EDA_DRAW_FRAME* EDA_DRAW_PANEL::GetParent() const
{
    wxWindow* mom = wxScrolledWindow::GetParent();
    return (EDA_DRAW_FRAME*) mom;
}


void* EDA_DRAW_PANEL::GetDisplayOptions()
{
    return GetParent()->GetDisplayOptions();
}


BASE_SCREEN* EDA_DRAW_PANEL::GetScreen()
{
    EDA_DRAW_FRAME* parentFrame = GetParent();

    return parentFrame->GetScreen();
}


wxPoint EDA_DRAW_PANEL::ToDeviceXY( const wxPoint& pos )
{
    wxPoint ret;
    INSTALL_UNBUFFERED_DC( dc, this );
    ret.x = dc.LogicalToDeviceX( pos.x );
    ret.y = dc.LogicalToDeviceY( pos.y );
    return ret;
}


wxPoint EDA_DRAW_PANEL::ToLogicalXY( const wxPoint& pos )
{
    wxPoint ret;
    INSTALL_UNBUFFERED_DC( dc, this );
    ret.x = dc.DeviceToLogicalX( pos.x );
    ret.y = dc.DeviceToLogicalY( pos.y );
    return ret;
}


void EDA_DRAW_PANEL::DrawCrossHair( wxDC* aDC, COLOR4D aColor )
{
    if( m_cursorLevel != 0 || aDC == NULL || !m_showCrossHair )
        return;

    wxPoint cursor = GetParent()->GetCrossHairPosition();

#ifdef USE_WX_GRAPHICS_CONTEXT
    // Normally cursor color is set to white, so when it is xored with white
    // background, it is painted black effectively. wxGraphicsContext does not have
    // xor operation, so we need to invert the color manually.
    aColor.Invert();
#else
    GRSetDrawMode( aDC, GR_XOR );
#endif

    if( GetParent()->GetGalDisplayOptions().m_fullscreenCursor )
    {
        wxSize  clientSize = GetClientSize();

        // Y axis
        wxPoint lineStart( cursor.x, aDC->DeviceToLogicalY( 0 ) );
        wxPoint lineEnd(   cursor.x, aDC->DeviceToLogicalY( clientSize.y ) );

        GRLine( &m_ClipBox, aDC, lineStart, lineEnd, 0, aColor );

        // X axis
        lineStart = wxPoint( aDC->DeviceToLogicalX( 0 ), cursor.y );
        lineEnd   = wxPoint( aDC->DeviceToLogicalX( clientSize.x ), cursor.y );

        GRLine( &m_ClipBox, aDC, lineStart, lineEnd, 0, aColor );
    }
    else
    {
        int len = aDC->DeviceToLogicalXRel( CURSOR_SIZE );

        GRLine( &m_ClipBox, aDC, cursor.x - len, cursor.y,
                cursor.x + len, cursor.y, 0, aColor );
        GRLine( &m_ClipBox, aDC, cursor.x, cursor.y - len,
                cursor.x, cursor.y + len, 0, aColor );
    }
}


void EDA_DRAW_PANEL::CrossHairOff( wxDC* DC )
{
    DrawCrossHair( DC );
    --m_cursorLevel;
}


void EDA_DRAW_PANEL::CrossHairOn( wxDC* DC )
{
    ++m_cursorLevel;
    DrawCrossHair( DC );

    if( m_cursorLevel > 0 )  // Shouldn't happen, but just in case ..
        m_cursorLevel = 0;
}


wxRealPoint EDA_DRAW_PANEL::GetGrid()
{
    return GetScreen()->GetGridSize();
}


void EDA_DRAW_PANEL::RefreshDrawingRect( const EDA_RECT& aRect, bool aEraseBackground )
{
    INSTALL_UNBUFFERED_DC( dc, this );

    wxRect rect = aRect;

    rect.x = dc.LogicalToDeviceX( rect.x );
    rect.y = dc.LogicalToDeviceY( rect.y );
    rect.width = dc.LogicalToDeviceXRel( rect.width );
    rect.height = dc.LogicalToDeviceYRel( rect.height );

    wxLogTrace( kicadTraceCoords,
                wxT( "Refresh area: drawing (%d, %d, %d, %d), device (%d, %d, %d, %d)" ),
                aRect.GetX(), aRect.GetY(), aRect.GetWidth(), aRect.GetHeight(),
                rect.x, rect.y, rect.width, rect.height );

    RefreshRect( rect, aEraseBackground );
}


void EDA_DRAW_PANEL::Refresh( bool eraseBackground, const wxRect* rect )
{
    GetParent()->GetGalCanvas()->Refresh();
}


wxPoint EDA_DRAW_PANEL::GetScreenCenterLogicalPosition()
{
    wxSize size = GetClientSize() / 2;
    INSTALL_UNBUFFERED_DC( dc, this );

    return wxPoint( dc.DeviceToLogicalX( size.x ), dc.DeviceToLogicalY( size.y ) );
}


void EDA_DRAW_PANEL::OnActivate( wxActivateEvent& event )
{
    m_canStartBlock = -1;   // Block Command can't start
    event.Skip();
}


void EDA_DRAW_PANEL::OnScroll( wxScrollWinEvent& event )
{
    int id = event.GetEventType();
    int x, y;
    int ppux, ppuy;
    int csizeX, csizeY;
    int unitsX, unitsY;

    GetViewStart( &x, &y );
    GetScrollPixelsPerUnit( &ppux, &ppuy );
    GetClientSize( &csizeX, &csizeY );
    GetVirtualSize( &unitsX, &unitsY );

    int tmpX = x;
    int tmpY = y;

    csizeX /= ppux;
    csizeY /= ppuy;

    unitsX /= ppux;
    unitsY /= ppuy;

    int dir = event.GetOrientation();   // wxHORIZONTAL or wxVERTICAL

    // On windows and on wxWidgets >= 2.9.5 and < 3.1,
    // there is a bug in mousewheel event which always generates 2 scroll events
    // (should be the case only for the default mousewheel event)
    // with id = wxEVT_SCROLLWIN_LINEUP or wxEVT_SCROLLWIN_LINEDOWN
    // so we skip these events.
    // Note they are here just in case, because they are not actually used
    // in Kicad
#if wxCHECK_VERSION( 3, 1, 0 ) || !wxCHECK_VERSION( 2, 9, 5 ) || ( !defined (__WINDOWS__) && !defined (__WXMAC__) )
    int maxX = unitsX - csizeX;
    int maxY = unitsY - csizeY;

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
    else
#endif
    if( id == wxEVT_SCROLLWIN_THUMBTRACK )
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

    wxLogTrace( kicadTraceCoords,
                wxT( "Setting scroll bars ppuX=%d, ppuY=%d, unitsX=%d, unitsY=%d, posX=%d, posY=%d" ),
                ppux, ppuy, unitsX, unitsY, x, y );

    double scale = GetParent()->GetScreen()->GetScalingFactor();

    wxPoint center = GetParent()->GetScrollCenterPosition();
    center.x += KiROUND( (double) ( x - tmpX ) / scale );
    center.y += KiROUND( (double) ( y - tmpY ) / scale );
    GetParent()->SetScrollCenterPosition( center );

    Scroll( x, y );
    event.Skip();
}


void EDA_DRAW_PANEL::SetClipBox( wxDC& aDC, const wxRect* aRect )
{
    wxRect clipBox;

    // Use the entire visible device area if no clip area was defined.
    if( aRect == NULL )
    {
        BASE_SCREEN* Screen = GetScreen();

        if( !Screen )
            return;

        Screen->m_StartVisu = CalcUnscrolledPosition( wxPoint( 0, 0 ) );
        clipBox.SetSize( GetClientSize() );

        int scrollX, scrollY;

        double scalar = Screen->GetScalingFactor();
        scrollX = KiROUND( Screen->GetGridSize().x * scalar );
        scrollY = KiROUND( Screen->GetGridSize().y * scalar );

        m_scrollIncrementX = std::max( GetClientSize().x / 8, scrollX );
        m_scrollIncrementY = std::max( GetClientSize().y / 8, scrollY );
        Screen->m_ScrollbarPos.x = GetScrollPos( wxHORIZONTAL );
        Screen->m_ScrollbarPos.y = GetScrollPos( wxVERTICAL );
    }
    else
    {
        clipBox = *aRect;
    }

    // Pad clip box in device units.
    clipBox.Inflate( CLIP_BOX_PADDING );

    // Convert from device units to drawing units.
    m_ClipBox.SetOrigin( wxPoint( aDC.DeviceToLogicalX( clipBox.x ),
                                  aDC.DeviceToLogicalY( clipBox.y ) ) );
    m_ClipBox.SetSize( wxSize( aDC.DeviceToLogicalXRel( clipBox.width ),
                               aDC.DeviceToLogicalYRel( clipBox.height ) ) );

    wxLogTrace( kicadTraceCoords,
                wxT( "Device clip box=(%d, %d, %d, %d), Logical clip box=(%d, %d, %d, %d)" ),
                clipBox.x, clipBox.y, clipBox.width, clipBox.height,
                m_ClipBox.GetX(), m_ClipBox.GetY(), m_ClipBox.GetWidth(), m_ClipBox.GetHeight() );
}


void EDA_DRAW_PANEL::DoPrepareDC( wxDC& dc )
{
    wxScrolledWindow::DoPrepareDC( dc );

    if( GetScreen() != NULL )
    {
        double scale = GetScreen()->GetScalingFactor();
        dc.SetUserScale( scale, scale );

        wxPoint pt = GetScreen()->m_DrawOrg;
        dc.SetLogicalOrigin( pt.x, pt.y );
    }

    SetClipBox( dc );                         // Reset the clip box to the entire screen.
    GRResetPenAndBrush( &dc );
    dc.SetBackgroundMode( wxTRANSPARENT );
}


void EDA_DRAW_PANEL::OnPaint( wxPaintEvent& event )
{
    // OBSOLETE
}


// Set to 1 to draw auxirilary axis as lines, 0 to draw as target (circle with cross)
#define DRAW_AXIS_AS_LINES 0
// Size in pixels of the target shape
#define AXIS_SIZE_IN_PIXELS 15

void EDA_DRAW_PANEL::OnMouseEntering( wxMouseEvent& aEvent )
{
    // OBSOLETE
    aEvent.Skip();
}


void EDA_DRAW_PANEL::OnMouseLeaving( wxMouseEvent& event )
{
    // OBSOLETE
    event.Skip();
}


void EDA_DRAW_PANEL::OnMouseWheel( wxMouseEvent& event )
{
    // OBSOLETE
    event.Skip();
}


#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
void EDA_DRAW_PANEL::OnMagnify( wxMouseEvent& event )
{
    // OBSOLETE
    event.Skip();
}
#endif


void EDA_DRAW_PANEL::OnCharHook( wxKeyEvent& event )
{
    wxLogTrace( kicadTraceKeyEvent, "EDA_DRAW_PANEL::OnCharHook %s", dump( event ) );
    event.Skip();
}


void EDA_DRAW_PANEL::OnPan( wxCommandEvent& event )
{
    int x, y;
    int ppux, ppuy;
    int unitsX, unitsY;
    int maxX, maxY;
    int tmpX, tmpY;

    GetViewStart( &x, &y );
    GetScrollPixelsPerUnit( &ppux, &ppuy );
    GetVirtualSize( &unitsX, &unitsY );
    tmpX = x;
    tmpY = y;
    maxX = unitsX;
    maxY = unitsY;
    unitsX /= ppux;
    unitsY /= ppuy;

    wxLogTrace( kicadTraceCoords,
                wxT( "Scroll center position before pan: (%d, %d)" ), tmpX, tmpY );

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

    bool updateCenterScrollPos = true;

    if( x < 0 )
    {
        x = 0;
        updateCenterScrollPos = false;
    }

    if( y < 0 )
    {
        y = 0;
        updateCenterScrollPos = false;
    }

    if( x > maxX )
    {
        x = maxX;
        updateCenterScrollPos = false;
    }

    if( y > maxY )
    {
        y = maxY;
        updateCenterScrollPos = false;
    }

    // Don't update the scroll position beyond the scroll limits.
    if( updateCenterScrollPos )
    {
        double scale = GetParent()->GetScreen()->GetScalingFactor();

        wxPoint center = GetParent()->GetScrollCenterPosition();
        center.x += KiROUND( (double) ( x - tmpX ) / scale );
        center.y += KiROUND( (double) ( y - tmpY ) / scale );
        GetParent()->SetScrollCenterPosition( center );

        wxLogTrace( kicadTraceCoords,
                    wxT( "Scroll center position after pan: (%d, %d)" ), center.x, center.y );
    }

    Scroll( x/ppux, y/ppuy );
}


