/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 Anil8735(https://stackoverflow.com/users/3659387/anil8753)
 *                    from https://stackoverflow.com/a/37274011
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/split_button.h>
#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/menu.h>
#include <wx/renderer.h>
#include <wx/settings.h>
#include <wx/version.h>
#include <kiplatform/ui.h>

SPLIT_BUTTON::SPLIT_BUTTON( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
                            const wxPoint& aPos, const wxSize& aSize ) :
        wxPanel( aParent, aId, aPos, aSize, wxBORDER_NONE | wxTAB_TRAVERSAL,
                 wxS( "DropDownButton" ) ),
        m_label( aLabel )
{
    m_arrowButtonWidth = FromDIP( 20 ); // just a fixed eyeballed constant width
    m_widthPadding = FromDIP( 10 );

    if( aSize == wxDefaultSize )
    {
        wxSize defaultSize = wxButton::GetDefaultSize( aParent );
        wxSize textSize = GetTextExtent( m_label );
        SetMinSize( wxSize( std::max( textSize.GetWidth(), defaultSize.GetWidth() + 1 ),
                            defaultSize.GetHeight() + 1 ) );
    }

    Bind( wxEVT_PAINT, &SPLIT_BUTTON::OnPaint, this );
    Bind( wxEVT_LEFT_UP, &SPLIT_BUTTON::OnLeftButtonUp, this );
    Bind( wxEVT_LEFT_DOWN, &SPLIT_BUTTON::OnLeftButtonDown, this );
    Bind( wxEVT_KILL_FOCUS, &SPLIT_BUTTON::OnKillFocus, this );
    Bind( wxEVT_LEAVE_WINDOW, &SPLIT_BUTTON::OnMouseLeave, this );
    Bind( wxEVT_ENTER_WINDOW, &SPLIT_BUTTON::OnMouseEnter, this );

    Bind( wxEVT_SYS_COLOUR_CHANGED, wxSysColourChangedEventHandler( SPLIT_BUTTON::onThemeChanged ),
          this );

    m_pMenu = new wxMenu();
}


SPLIT_BUTTON::~SPLIT_BUTTON()
{
    delete m_pMenu;
    m_pMenu = nullptr;
}


void SPLIT_BUTTON::onThemeChanged( wxSysColourChangedEvent &aEvent )
{
    Refresh();
}


void SPLIT_BUTTON::SetMinSize( const wxSize& aSize )
{
    m_unadjustedMinSize = aSize;
    wxPanel::SetMinSize( wxSize( aSize.GetWidth() + m_arrowButtonWidth + m_widthPadding,
                                 aSize.GetHeight() ) );
}


void SPLIT_BUTTON::SetWidthPadding( int aPadding )
{
    m_widthPadding = aPadding;
    SetMinSize( m_unadjustedMinSize );
}


void SPLIT_BUTTON::SetBitmap( const wxBitmapBundle& aBmp )
{
    m_bitmap = aBmp;

#ifndef __WXMSW__
    SetMinSize( m_bitmap.GetDefaultSize() );
#else
    SetMinSize( m_bitmap.GetPreferredBitmapSizeFor( this ) );
#endif
}


void SPLIT_BUTTON::SetLabel( const wxString& aLabel )
{
    if( m_label != aLabel )
    {
        m_label = aLabel;
        Refresh();
    }
}


wxMenu* SPLIT_BUTTON::GetSplitButtonMenu()
{
    return m_pMenu;
}


void SPLIT_BUTTON::OnKillFocus( wxFocusEvent& aEvent )
{
    if( m_stateButton != 0 || m_stateMenu != 0 )
    {
        m_stateButton = 0;
        m_stateMenu   = 0;
        Refresh();
    }

    aEvent.Skip();
}


void SPLIT_BUTTON::OnMouseLeave( wxMouseEvent& aEvent )
{
    if( m_stateButton != 0 || m_stateMenu != 0 )
    {
        m_stateButton = 0;
        m_stateMenu   = 0;
        Refresh();
    }

    aEvent.Skip();
}


void SPLIT_BUTTON::OnMouseEnter( wxMouseEvent& aEvent )
{
    if( m_stateButton != wxCONTROL_CURRENT || m_stateMenu != wxCONTROL_CURRENT )
    {
        m_stateButton = wxCONTROL_CURRENT;
        m_stateMenu   = wxCONTROL_CURRENT;
        Refresh();
    }

    aEvent.Skip();
}


void SPLIT_BUTTON::OnLeftButtonUp( wxMouseEvent& aEvent )
{
    m_stateButton = 0;
    m_stateMenu   = 0;

    Refresh();

    int x = -1;
    int y = -1;
    aEvent.GetPosition( &x, &y );

    if( x < ( GetSize().GetWidth() - m_arrowButtonWidth ) )
    {
        wxEvtHandler* pEventHandler = GetEventHandler();
        wxASSERT( pEventHandler );

        pEventHandler->CallAfter(
                [this]()
                {
                    wxCommandEvent evt( wxEVT_BUTTON, GetId() );
                    evt.SetEventObject( this );
                    GetEventHandler()->ProcessEvent( evt );
                } );
    }

    m_bLButtonDown = false;

    aEvent.Skip();
}


void SPLIT_BUTTON::OnLeftButtonDown( wxMouseEvent& aEvent )
{
    m_bLButtonDown = true;

    int x = -1;
    int y = -1;
    aEvent.GetPosition( &x, &y );

    if( x >= ( GetSize().GetWidth() - m_arrowButtonWidth ) )
    {
        m_stateButton = 0;
        m_stateMenu   = wxCONTROL_PRESSED;
        Refresh();

        wxSize  size = GetSize();
        wxPoint position;
        position.x = 0;
        position.y = size.GetHeight();
        PopupMenu( m_pMenu, position );

        m_stateMenu = 0;
        Refresh();
    }
    else
    {
        m_stateButton = wxCONTROL_PRESSED;
        m_stateMenu   = wxCONTROL_PRESSED;
        Refresh();
    }

    aEvent.Skip();
}


void SPLIT_BUTTON::OnPaint( wxPaintEvent& WXUNUSED( aEvent ) )
{
    wxPaintDC dc( this );
    wxSize    size  = GetSize();
    const int width = size.GetWidth() - m_arrowButtonWidth;

#ifdef __WXMAC__
    auto drawBackground =
            [&]( wxRect aRect )
            {
                // wxWidgets doesn't have much support for dark mode on OSX; none of the
                // system colours return the right values, nor does wxRendererNative draw
                // the borders correctly.  So we add some empirically chosen hacks here.

                // NOTE: KEEP THESE HACKS IN SYNC WITH STD_BITMAP_BUTTON

                wxColor fg = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT );
                wxColor bg = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );

                if( KIPLATFORM::UI::IsDarkTheme() )
                {
                    bg = bg.ChangeLightness( m_bIsEnable ? 130 : 120 );
                    dc.SetBrush( bg );
                    dc.SetPen( bg );
                }
                else
                {
                    bg = bg.ChangeLightness( m_bIsEnable ? 200 : 160 );
                    dc.SetBrush( bg );
                    fg = fg.ChangeLightness( 180 );
                    dc.SetPen( fg );
                }

                dc.DrawRoundedRectangle( aRect, aRect.height / 4.0 );
            };
#endif

    // Draw first part of button
    wxRect r1;
    r1.x      = 0;
    r1.y      = 0;
    r1.width  = width;
    r1.height = size.GetHeight();

#ifdef __WXMAC__
    // wxRendereNative doesn't handle dark mode on OSX.
    drawBackground( r1 );
#else

#ifdef _WXMSW_
    r1.width += 2;
#endif

    wxRendererNative::Get().DrawPushButton( this, dc, r1, m_stateButton );
#endif

    SetForegroundColour( m_bIsEnable ? wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT )
                                     : wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

    if( m_bitmap.IsOk() )
    {
#ifndef __WXMSW__
        wxSize     bmpSize = m_bitmap.GetDefaultSize();
#else
        wxSize     bmpSize = m_bitmap.GetPreferredBitmapSizeFor( this );
#endif
        wxBitmap   bmp = m_bitmap.GetBitmapFor( this );
        wxMemoryDC mdc( bmp );

        r1.x = ( width - bmpSize.GetWidth() ) / 2;

        if( r1.x < 0 )
            r1.x = 0;

        r1.y += ( size.GetHeight() - bmpSize.GetHeight() ) / 2;

        dc.Blit( wxPoint( r1.x, r1.y ), bmpSize, &mdc, wxPoint( 0, 0 ), wxCOPY, true );
    }
    else
    {
        r1.y += ( ( size.GetHeight() - GetCharHeight() ) / 2 ) - 1;
        dc.DrawLabel( m_label, r1, wxALIGN_CENTER_HORIZONTAL );
    }

    // Draw second part of button
    wxRect r2;
    r2.x      = width;
    r2.y      = 0;
    r2.width  = m_arrowButtonWidth;
    r2.height = size.GetHeight();

#ifdef __WXMAC__
    // wxRendereNative doesn't handle dark mode on OSX.
    drawBackground( r2 );
#else
    r2.x -= 2;
    wxRendererNative::Get().DrawPushButton( this, dc, r2, m_stateMenu );
#endif

    wxRendererNative::Get().DrawDropArrow( this, dc, r2, m_stateMenu );
}


bool SPLIT_BUTTON::Enable( bool aEnable )
{
    m_bIsEnable = aEnable;
    wxPanel::Enable( m_bIsEnable );

    if( m_bIsEnable
            && ( m_stateButton == wxCONTROL_DISABLED || m_stateMenu == wxCONTROL_DISABLED ) )
    {
        m_stateButton = 0;
        m_stateMenu   = 0;
        Refresh();
    }

    if( !m_bIsEnable
            && ( m_stateButton != wxCONTROL_DISABLED || m_stateMenu != wxCONTROL_DISABLED ) )
    {
        m_stateButton = wxCONTROL_DISABLED;
        m_stateMenu   = wxCONTROL_DISABLED;
        Refresh();
    }

    return aEnable;
}
