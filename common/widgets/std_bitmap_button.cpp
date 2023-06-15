/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 Anil8735(https://stackoverflow.com/users/3659387/anil8753)
 *                    from https://stackoverflow.com/a/37274011
 * Copyright (C) 2020-2023 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/std_bitmap_button.h>
#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/menu.h>
#include <wx/renderer.h>
#include <wx/settings.h>
#include <wx/version.h>
#include <kiplatform/ui.h>


STD_BITMAP_BUTTON::STD_BITMAP_BUTTON( wxWindow* aParent, wxWindowID aId,
                                      const wxBitmap& aDummyBitmap, const wxPoint& aPos,
                                      const wxSize& aSize, int aStyle ) :
        wxPanel( aParent, aId, aPos, aSize, aStyle, wxS( "StdBitmapButton" ) )
{
    if( aSize == wxDefaultSize )
    {
        #if wxCHECK_VERSION( 3, 1, 3 )
            wxSize defaultSize = wxButton::GetDefaultSize( aParent );
        #else
            wxSize defaultSize = wxButton::GetDefaultSize();
        #endif

        SetMinSize( wxSize( defaultSize.GetWidth() + 1, defaultSize.GetHeight() + 1 ) );
    }

    Bind( wxEVT_PAINT, &STD_BITMAP_BUTTON::OnPaint, this );
    Bind( wxEVT_LEFT_UP, &STD_BITMAP_BUTTON::OnLeftButtonUp, this );
    Bind( wxEVT_LEFT_DOWN, &STD_BITMAP_BUTTON::OnLeftButtonDown, this );
    Bind( wxEVT_KILL_FOCUS, &STD_BITMAP_BUTTON::OnKillFocus, this );
    Bind( wxEVT_LEAVE_WINDOW, &STD_BITMAP_BUTTON::OnMouseLeave, this );
    Bind( wxEVT_ENTER_WINDOW, &STD_BITMAP_BUTTON::OnMouseEnter, this );

    Bind( wxEVT_SYS_COLOUR_CHANGED,
          wxSysColourChangedEventHandler( STD_BITMAP_BUTTON::onThemeChanged ),
          this );
}


STD_BITMAP_BUTTON::~STD_BITMAP_BUTTON()
{
}


void STD_BITMAP_BUTTON::onThemeChanged( wxSysColourChangedEvent &aEvent )
{
    Refresh();
}


void STD_BITMAP_BUTTON::SetBitmap( const wxBitmapBundle& aBmp )
{
    m_bitmap = aBmp;

#ifndef __WXMSW__
    wxSize size = m_bitmap.GetDefaultSize();
#else
    wxSize size = m_bitmap.GetPreferredBitmapSizeFor( this );
#endif

    SetMinSize( wxSize( size.GetWidth() + 8, size.GetHeight() + 8 ) );
}


void STD_BITMAP_BUTTON::OnKillFocus( wxFocusEvent& aEvent )
{
    if( m_stateButton != 0 )
    {
        m_stateButton = 0;
        Refresh();
    }

    aEvent.Skip();
}


void STD_BITMAP_BUTTON::OnMouseLeave( wxMouseEvent& aEvent )
{
    if( m_stateButton != 0 )
    {
        m_stateButton = 0;
        Refresh();
    }

    aEvent.Skip();
}


void STD_BITMAP_BUTTON::OnMouseEnter( wxMouseEvent& aEvent )
{
    if( m_stateButton != wxCONTROL_CURRENT )
    {
        m_stateButton = wxCONTROL_CURRENT;
        Refresh();
    }

    aEvent.Skip();
}


void STD_BITMAP_BUTTON::OnLeftButtonUp( wxMouseEvent& aEvent )
{
    m_stateButton = 0;

    Refresh();

    wxEvtHandler* pEventHandler = GetEventHandler();
    wxASSERT( pEventHandler );

    pEventHandler->CallAfter(
            [=]()
            {
                wxCommandEvent evt( wxEVT_BUTTON, GetId() );
                evt.SetEventObject( this );
                GetEventHandler()->ProcessEvent( evt );
            } );

    aEvent.Skip();
}


void STD_BITMAP_BUTTON::OnLeftButtonDown( wxMouseEvent& aEvent )
{
    m_stateButton = wxCONTROL_PRESSED;
    Refresh();

    aEvent.Skip();
}


void STD_BITMAP_BUTTON::OnPaint( wxPaintEvent& WXUNUSED( aEvent ) )
{
    wxPaintDC dc( this );
    wxSize    size  = GetSize();

#ifdef __WXMAC__
    auto drawBackground =
            [&]( wxRect aRect )
            {
                // wxWidgets doesn't have much support for dark mode on OSX; none of the
                // system colours return the right values, nor does wxRendererNative draw
                // the borders correctly.  So we add some empirically chosen hacks here.

                // NOTE: KEEP THESE HACKS IN SYNC WITH SPLIT_BUTTON

                wxColor fg = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT );
                wxColor bg = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );

                aRect.width += 1;
                aRect.height += 1;

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

    wxRect r1;
    r1.x      = 0;
    r1.y      = 0;
    r1.width  = size.GetWidth();
    r1.height = size.GetHeight();

#ifdef __WXMAC__
    // wxRendereNative doesn't handle dark mode on OSX.
    drawBackground( r1 );
#else
    #ifdef __WXMSW__
        r1.width += 1;
    #endif

    wxRendererNative::Get().DrawPushButton( this, dc, r1, m_stateButton );
#endif

    if( m_bitmap.IsOk() )
    {
#ifndef __WXMSW__
        wxSize bmpSize = m_bitmap.GetDefaultSize();
#else
        wxSize bmpSize = m_bitmap.GetPreferredBitmapSizeFor( this );
#endif

        r1.x = ( size.GetWidth() - bmpSize.GetWidth() ) / 2;

        if( r1.x < 0 )
            r1.x = 0;

        r1.y += ( size.GetHeight() - bmpSize.GetHeight() ) / 2;

        wxBitmap bm = m_bitmap.GetBitmapFor( this );

        if( !m_bIsEnable )
            bm.ConvertToDisabled();

        dc.DrawBitmap( bm, r1.x, r1.y, true );
    }
}


bool STD_BITMAP_BUTTON::Enable( bool aEnable )
{
    m_bIsEnable = aEnable;
    wxPanel::Enable( m_bIsEnable );

    if( m_bIsEnable && m_stateButton == wxCONTROL_DISABLED )
    {
        m_stateButton = 0;
        Refresh();
    }

    if( !m_bIsEnable && m_stateButton != wxCONTROL_DISABLED )
    {
        m_stateButton = wxCONTROL_DISABLED;
        Refresh();
    }

    return aEnable;
}
