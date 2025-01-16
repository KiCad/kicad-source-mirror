/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <bitmaps.h>
#include <widgets/wx_collapsible_pane.h>

#include <wx/collpane.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/panel.h>
#include <wx/renderer.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/toplevel.h>
#include <wx/window.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>


wxDEFINE_EVENT( WX_COLLAPSIBLE_PANE_HEADER_CHANGED, wxCommandEvent );
wxDEFINE_EVENT( WX_COLLAPSIBLE_PANE_CHANGED, wxCommandEvent );


bool WX_COLLAPSIBLE_PANE:: Create( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
                                   const wxPoint& aPos, const wxSize& aSize, long aStyle,
                                   const wxValidator& aValidator, const wxString& aName )
{
    if( !wxControl::Create( aParent, aId, aPos, aSize, aStyle, aValidator, aName ) )
        return false;

    m_sizer = new wxBoxSizer( wxVERTICAL );

    m_header = new WX_COLLAPSIBLE_PANE_HEADER( this, wxID_ANY, aLabel, wxPoint( 0, 0 ),
                                               wxDefaultSize );

    m_sizer->Add( m_header, wxSizerFlags().Border( wxBOTTOM, getBorder() ) );

    m_pane = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                          wxTAB_TRAVERSAL | wxNO_BORDER, wxT( "COLLAPSIBLE_PANE_PANE" ) );

    m_pane->Hide();

    Bind( wxEVT_SIZE, &WX_COLLAPSIBLE_PANE::onSize, this );
    Bind( WX_COLLAPSIBLE_PANE_HEADER_CHANGED, &WX_COLLAPSIBLE_PANE::onHeaderClicked, this );

    return true;
}


void WX_COLLAPSIBLE_PANE::init()
{
    m_pane   = nullptr;
    m_sizer  = nullptr;
    m_header = nullptr;
}


WX_COLLAPSIBLE_PANE::~WX_COLLAPSIBLE_PANE()
{
    if( m_sizer )
        m_sizer->SetContainingWindow( nullptr );

    // Not owned by wxWindow
    delete m_sizer;
}


void WX_COLLAPSIBLE_PANE::Collapse( bool aCollapse )
{
    if( IsCollapsed() == aCollapse )
        return;

    InvalidateBestSize();

    m_pane->Show( !aCollapse );
    m_header->SetCollapsed( aCollapse );

    SetSize( GetBestSize() );
}


bool WX_COLLAPSIBLE_PANE::IsCollapsed() const
{
    return !m_pane || !m_pane->IsShown();
}


void WX_COLLAPSIBLE_PANE::SetLabel( const wxString& aLabel )
{
    m_header->SetLabel( aLabel );
    m_header->SetInitialSize();

    Layout();
}


bool WX_COLLAPSIBLE_PANE::SetBackgroundColour( const wxColour& aColor )
{
    m_header->SetBackgroundColour( aColor );
    return wxWindow::SetBackgroundColour( aColor );
}


bool WX_COLLAPSIBLE_PANE::InformFirstDirection( int aDirection, int aSize, int aAvailableOtherDir )
{
    wxWindow* const pane = GetPane();

    if( !pane )
        return false;

    if( !pane->InformFirstDirection( aDirection, aSize, aAvailableOtherDir ) )
        return false;

    InvalidateBestSize();

    return true;
}


wxSize WX_COLLAPSIBLE_PANE::DoGetBestClientSize() const
{
    wxSize size = m_sizer->GetMinSize();

    if( IsExpanded() )
    {
        wxSize paneSize = m_pane->GetBestSize();

        size.SetWidth( std::max( size.GetWidth(), paneSize.x ) );
        size.SetHeight( size.y +  getBorder() + paneSize.y );
    }

    return size;
}


bool WX_COLLAPSIBLE_PANE::Layout()
{
    if( !m_sizer || !m_pane || !m_header )
        return false;

    wxSize size( GetSize() );

    m_sizer->SetDimension( 0, 0, size.x, m_sizer->GetMinSize().y );
    m_sizer->Layout();

    if( IsExpanded() )
    {
        int yoffset = m_sizer->GetSize().y + getBorder();
        m_pane->SetSize( 0, yoffset, size.x, size.y - yoffset );
        m_pane->Layout();
    }

    return true;
}


int WX_COLLAPSIBLE_PANE::getBorder() const
{
#if defined( __WXMSW__ )
    wxASSERT( m_header );
    return m_header->ConvertDialogToPixels( wxSize( 2, 0 ) ).x;
#else
    return 3;
#endif
}


void WX_COLLAPSIBLE_PANE::onSize( wxSizeEvent& aEvent )
{
    Layout();
    aEvent.Skip();
}


void WX_COLLAPSIBLE_PANE::onHeaderClicked( wxCommandEvent& aEvent )
{
    if( aEvent.GetEventObject() != m_header )
    {
        aEvent.Skip();
        return;
    }

    Collapse( !IsCollapsed() );

    wxCommandEvent evt( WX_COLLAPSIBLE_PANE_CHANGED, GetId() );
    evt.SetEventObject( this );
    ProcessEvent( evt );
}


// WX_COLLAPSIBLE_PANE_HEADER implementation


void WX_COLLAPSIBLE_PANE_HEADER::init()
{
    m_collapsed = true;
    m_inWindow  = false;
}


bool WX_COLLAPSIBLE_PANE_HEADER::Create( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
                                         const wxPoint& aPos, const wxSize& aSize, long aStyle,
                                         const wxValidator& aValidator, const wxString& aName )
{
    if( !wxControl::Create( aParent, aId, aPos, aSize, aStyle, aValidator, aName ) )
        return false;

    SetLabel( aLabel );

    Bind( wxEVT_PAINT, &WX_COLLAPSIBLE_PANE_HEADER::onPaint, this );
    Bind( wxEVT_SET_FOCUS, &WX_COLLAPSIBLE_PANE_HEADER::onFocus, this );
    Bind( wxEVT_KILL_FOCUS, &WX_COLLAPSIBLE_PANE_HEADER::onFocus, this );
    Bind( wxEVT_ENTER_WINDOW, &WX_COLLAPSIBLE_PANE_HEADER::onEnterWindow, this);
    Bind( wxEVT_LEAVE_WINDOW, &WX_COLLAPSIBLE_PANE_HEADER::onLeaveWindow, this);
    Bind( wxEVT_LEFT_UP, &WX_COLLAPSIBLE_PANE_HEADER::onLeftUp, this );
    Bind( wxEVT_CHAR, &WX_COLLAPSIBLE_PANE_HEADER::onChar, this );

    return true;
}


void WX_COLLAPSIBLE_PANE_HEADER::SetCollapsed( bool aCollapsed )
{
    m_collapsed = aCollapsed;
    Refresh();
}


void WX_COLLAPSIBLE_PANE_HEADER::doSetCollapsed( bool aCollapsed )
{
    SetCollapsed( aCollapsed );

    wxCommandEvent evt( WX_COLLAPSIBLE_PANE_HEADER_CHANGED, GetId() );
    evt.SetEventObject( this );
    ProcessEvent( evt );
}


wxSize WX_COLLAPSIBLE_PANE_HEADER::DoGetBestClientSize() const
{
    WX_COLLAPSIBLE_PANE_HEADER* self = const_cast<WX_COLLAPSIBLE_PANE_HEADER*>( this );

    // The code here parallels that of OnPaint() -- except without drawing.
    wxClientDC dc( self );
    wxString   text;

    wxControl::FindAccelIndex( GetLabel(), &text );

    wxSize size = dc.GetTextExtent( text );

    // Reserve space for arrow (which is a square the height of the text)
    size.x += size.GetHeight();

#ifdef __WXMSW__
    size.IncBy( GetSystemMetrics( SM_CXFOCUSBORDER ),
                GetSystemMetrics( SM_CYFOCUSBORDER ) );
#endif // __WXMSW__

    return size;
}


void WX_COLLAPSIBLE_PANE_HEADER::onPaint( wxPaintEvent& aEvent )
{
    wxPaintDC dc( this );
    wxRect    rect( wxPoint( 0, 0 ), GetClientSize() );

#ifdef __WXMSW__
    wxBrush brush = dc.GetBrush();
    brush.SetColour( GetParent()->GetBackgroundColour() );
    dc.SetBrush( brush );
    dc.SetPen( *wxTRANSPARENT_PEN );
    dc.DrawRectangle( rect );
#endif

    // Make the background look like a button when the pointer is over it
    if( m_inWindow )
    {
        dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNHIGHLIGHT ) ) );
        dc.SetPen( *wxTRANSPARENT_PEN );
        dc.DrawRectangle( rect );
    }

    wxString text;
    int indexAccel = wxControl::FindAccelIndex( GetLabel(), &text );

    wxSize textSize = dc.GetTextExtent( text );

    // Compute all the sizes
    wxRect arrowRect( 0, 0, textSize.GetHeight(), textSize.GetHeight() );
    wxRect textRect( arrowRect.GetTopRight(), textSize );
    textRect = textRect.CenterIn( rect, wxVERTICAL );

    // Find out if the window we are in is active or not
    bool              isActive = true;
    wxTopLevelWindow* tlw      = dynamic_cast<wxTopLevelWindow*>( wxGetTopLevelParent( this ) );

    if( tlw && !tlw->IsActive() )
        isActive = false;

    // Draw the arrow
    drawArrow( dc, arrowRect, isActive );

    // We are responsible for showing the text as disabled when the window isn't active
    wxColour clr;

    if( isActive )
        clr = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );
    else
        clr = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );

    dc.SetTextForeground( clr );
    dc.DrawLabel( text, textRect, wxALIGN_CENTER_VERTICAL, indexAccel );

#ifdef __WXMSW__
    int flags = 0;

    if( m_inWindow )
        flags |= wxCONTROL_CURRENT;

    int focusSize = GetSystemMetrics( SM_CXFOCUSBORDER );

    if( HasFocus() )
        wxRendererNative::Get().DrawFocusRect( this, dc, textRect.Inflate( focusSize ), flags );
#endif
}


void WX_COLLAPSIBLE_PANE_HEADER::onFocus( wxFocusEvent& aEvent )
{
    Refresh();
    aEvent.Skip();
}


void WX_COLLAPSIBLE_PANE_HEADER::onEnterWindow( wxMouseEvent& aEvent )
{
    m_inWindow = true;
    Refresh();
    aEvent.Skip();
}


void WX_COLLAPSIBLE_PANE_HEADER::onLeaveWindow( wxMouseEvent& aEvent )
{
    m_inWindow = false;
    Refresh();
    aEvent.Skip();
}


void WX_COLLAPSIBLE_PANE_HEADER::onLeftUp( wxMouseEvent& aEvent )
{
    doSetCollapsed( !m_collapsed );
    aEvent.Skip();
}


void WX_COLLAPSIBLE_PANE_HEADER::onChar( wxKeyEvent& aEvent )
{
    switch( aEvent.GetKeyCode() )
    {
    case WXK_SPACE:
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        doSetCollapsed( !m_collapsed );
        break;

    default:
        aEvent.Skip();
        break;
    }
}


void WX_COLLAPSIBLE_PANE_HEADER::drawArrow( wxDC& aDC, wxRect aRect, bool aIsActive )
{
    // The bottom corner of the triangle is located halfway across the area and 3/4 down from
    // the top
    wxPoint btmCorner( aRect.GetWidth() / 2, 3 * aRect.GetHeight() / 4 );

    // The right corner of the triangle is located halfway down from the top and 3/4 across the area
    wxPoint rtCorner( 3 * aRect.GetWidth() / 4, aRect.GetHeight() / 2 );

    // Choose the other corner depending on if the panel is expanded or collapsed
    wxPoint otherCorner( 0, 0 );

    if( m_collapsed )
        otherCorner = wxPoint( aRect.GetWidth() / 2, aRect.GetHeight() / 4 );
    else
        otherCorner = wxPoint( aRect.GetWidth() / 4,  aRect.GetHeight() / 2 );

    // Choose the color to draw the triangle
    wxColour clr;

    // Highlight the arrow when the pointer is inside the header, otherwise use text color
    if( m_inWindow )
        clr = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
    else
        clr = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

    // If the window isn't active, then use the disabled text color
    if( !aIsActive )
        clr = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );

    // Must set both the pen (for the outline) and the brush (for the polygon fill)
    aDC.SetPen( wxPen( clr ) );
    aDC.SetBrush( wxBrush( clr ) );

    // Draw the triangle
    wxPointList points;
    points.Append( &btmCorner );
    points.Append( &rtCorner );
    points.Append( &otherCorner );

    aDC.DrawPolygon( &points );
}
