/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney at ieee dot org>
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

#include <kiplatform/ui.h>
#include <widgets/bitmap_button.h>
#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/renderer.h>
#include <wx/settings.h>

#define wxCONTROL_SEPARATOR wxCONTROL_SPECIAL


BITMAP_BUTTON::BITMAP_BUTTON( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos,
                              const wxSize& aSize, int aStyles ) :
        wxPanel( aParent, aId, aPos, aSize, aStyles ),
        m_isRadioButton( false ),
        m_showBadge( false ),
        m_badgeColor( wxColor( 210, 0, 0 ) ), // dark red
        m_badgeTextColor( wxColor( wxT( "white" ) ) ),
        m_buttonState( 0 ),
        m_padding( 0 ),
        m_acceptDraggedInClicks( false )
{
    if( aSize == wxDefaultSize )
        SetMinSize( wxButton::GetDefaultSize() + wxSize( m_padding * 2, m_padding * 2) );

    m_badgeFont = GetFont().Smaller().MakeBold();

    setupEvents();
}


BITMAP_BUTTON::BITMAP_BUTTON( wxWindow* aParent, wxWindowID aId, const wxBitmap& aDummyBitmap,
                              const wxPoint& aPos, const wxSize& aSize, int aStyles ) :
        wxPanel( aParent, aId, aPos, aSize, aStyles ),
        m_isRadioButton( false ),
        m_showBadge( false ),
        m_buttonState( 0 ),
        m_padding( 5 ),
        m_acceptDraggedInClicks( false )
{
    if( aSize == wxDefaultSize )
        SetMinSize( wxButton::GetDefaultSize() + wxSize( m_padding * 2, m_padding * 2) );

    setupEvents();
}


void BITMAP_BUTTON::setupEvents()
{
    Bind( wxEVT_PAINT,        &BITMAP_BUTTON::OnPaint,          this );
    Bind( wxEVT_LEFT_UP,      &BITMAP_BUTTON::OnLeftButtonUp,   this );
    Bind( wxEVT_LEFT_DOWN,    &BITMAP_BUTTON::OnLeftButtonDown, this );
    Bind( wxEVT_LEAVE_WINDOW, &BITMAP_BUTTON::OnMouseLeave,     this );
    Bind( wxEVT_ENTER_WINDOW, &BITMAP_BUTTON::OnMouseEnter,     this );
    Bind( wxEVT_KILL_FOCUS,   &BITMAP_BUTTON::OnKillFocus,      this );
    Bind( wxEVT_SET_FOCUS,    &BITMAP_BUTTON::OnSetFocus,       this );
}


BITMAP_BUTTON::~BITMAP_BUTTON()
{
}


void BITMAP_BUTTON::SetPadding( int aPadding )
{
    m_padding = aPadding;
    SetMinSize( m_unadjustedMinSize + wxSize( aPadding * 2, aPadding * 2 ) );
}


void BITMAP_BUTTON::SetBitmap( const wxBitmapBundle& aBmp )
{
    m_normalBitmap = aBmp;
#ifndef __WXMSW__
    m_unadjustedMinSize = m_normalBitmap.GetDefaultSize();
#else
    m_unadjustedMinSize = m_normalBitmap.GetPreferredBitmapSizeFor( this );
#endif

    SetMinSize( wxSize( m_unadjustedMinSize.GetWidth() + ( m_padding * 2 ),
                        m_unadjustedMinSize.GetHeight() + ( m_padding * 2 ) ) );
}


void BITMAP_BUTTON::SetDisabledBitmap( const wxBitmapBundle& aBmp )
{
    m_disabledBitmap = aBmp;
}


void BITMAP_BUTTON::AcceptDragInAsClick( bool aAcceptDragIn )
{
    m_acceptDraggedInClicks = aAcceptDragIn;
}


void BITMAP_BUTTON::OnMouseLeave( wxEvent& aEvent )
{
    if( hasFlag( wxCONTROL_CURRENT | wxCONTROL_PRESSED ) )
    {
        clearFlag( wxCONTROL_CURRENT | wxCONTROL_PRESSED );
        Refresh();
    }

    aEvent.Skip();
}


void BITMAP_BUTTON::OnMouseEnter( wxEvent& aEvent )
{
    if( !hasFlag( wxCONTROL_CURRENT ) )
    {
        setFlag( wxCONTROL_CURRENT );
        Refresh();
    }

    aEvent.Skip();
}


void BITMAP_BUTTON::OnKillFocus( wxEvent& aEvent )
{
    if( hasFlag( wxCONTROL_FOCUSED | wxCONTROL_CURRENT | wxCONTROL_PRESSED ) )
    {
        clearFlag( wxCONTROL_FOCUSED | wxCONTROL_CURRENT | wxCONTROL_PRESSED );
        Refresh();
    }

    aEvent.Skip();
}


void BITMAP_BUTTON::OnSetFocus( wxEvent& aEvent )
{
    if( !hasFlag( wxCONTROL_CHECKABLE ) )
    {
        if( !hasFlag( wxCONTROL_FOCUSED ) )
        {
            setFlag( wxCONTROL_FOCUSED );
            Refresh();
        }
    }

    aEvent.Skip();
}


void BITMAP_BUTTON::OnLeftButtonUp( wxMouseEvent& aEvent )
{
    // Only create a button event when the control is enabled
    // and only accept clicks that came without prior mouse-down if configured
    if( !hasFlag( wxCONTROL_DISABLED )
            && ( m_acceptDraggedInClicks || hasFlag( wxCONTROL_PRESSED | wxCONTROL_FOCUSED ) ) )
    {
        GetEventHandler()->CallAfter( [=]()
                                      {
                                          wxCommandEvent evt( wxEVT_BUTTON, GetId() );
                                          evt.SetEventObject( this );
                                          GetEventHandler()->ProcessEvent( evt );
                                      } );
    }

    clearFlag( wxCONTROL_PRESSED );
    Refresh();

    aEvent.Skip();
}


void BITMAP_BUTTON::OnLeftButtonDown( wxMouseEvent& aEvent )
{
    if( hasFlag( wxCONTROL_CHECKABLE ) )
    {
        if( hasFlag( wxCONTROL_CHECKED ) && !m_isRadioButton )
        {
            clearFlag( wxCONTROL_CHECKED );

            GetEventHandler()->CallAfter(
                    [=]()
                    {
                        wxCommandEvent evt( wxEVT_BUTTON, GetId() );
                        evt.SetEventObject( this );
                        evt.SetInt( 0 );
                        GetEventHandler()->ProcessEvent( evt );
                    } );
        }
        else
        {
            setFlag( wxCONTROL_CHECKED );

            GetEventHandler()->CallAfter(
                    [=]()
                    {
                        wxCommandEvent evt( wxEVT_BUTTON, GetId() );
                        evt.SetEventObject( this );
                        evt.SetInt( 1 );
                        GetEventHandler()->ProcessEvent( evt );
                    } );
        }
    }
    else
    {
        setFlag( wxCONTROL_PRESSED );
    }

    Refresh();

    aEvent.Skip();
}


void BITMAP_BUTTON::OnPaint( wxPaintEvent& aEvent )
{
    bool    darkMode       = KIPLATFORM::UI::IsDarkTheme();
    wxColor highlightColor = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );

    // The drawing rectangle
    wxRect    rect( wxPoint( 0, 0 ), GetSize() );
    wxPaintDC dc( this );

    if( hasFlag( wxCONTROL_SEPARATOR ) )
    {
        dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) ) );
        dc.DrawLine( wxPoint( GetSize().x / 2, 0 ), wxPoint( GetSize().x / 2, GetSize().y ) );
        return;
    }

    // This drawing is done so the button looks the same as an AUI toolbar button
    if( !hasFlag( wxCONTROL_DISABLED ) )
    {
        if( hasFlag( wxCONTROL_PRESSED ) )
        {
            dc.SetPen( wxPen( highlightColor ) );
            dc.SetBrush( wxBrush( highlightColor.ChangeLightness( darkMode ? 20 : 150 ) ) );
            dc.DrawRectangle( rect );
        }
        else if( hasFlag( wxCONTROL_CURRENT | wxCONTROL_FOCUSED ) )
        {
            dc.SetPen( wxPen( highlightColor ) );
            dc.SetBrush( wxBrush( highlightColor.ChangeLightness( darkMode ? 40 : 170 ) ) );

            // Checked items need a lighter hover rectangle
            if( hasFlag( wxCONTROL_CHECKED ) )
                dc.SetBrush( wxBrush( highlightColor.ChangeLightness( darkMode ? 50 : 180 ) ) );

            dc.DrawRectangle( rect );
        }
        else if( hasFlag( wxCONTROL_CHECKED ) )
        {
            dc.SetPen( wxPen( highlightColor ) );
            dc.SetBrush( wxBrush( highlightColor.ChangeLightness( darkMode  ? 40 : 170 ) ) );
            dc.DrawRectangle( rect );
        }
    }

    const wxBitmapBundle& bmp = hasFlag( wxCONTROL_DISABLED ) ? m_disabledBitmap : m_normalBitmap;

    // Draw the bitmap with the upper-left corner offset by the padding
    if( bmp.IsOk() )
        dc.DrawBitmap( bmp.GetBitmapFor( this ), m_padding, m_padding, true );

    // Draw the badge
    if( m_showBadge )
    {
        dc.SetFont( m_badgeFont );

        wxSize box_size = dc.GetTextExtent( m_badgeText ) + wxSize( 6, 2 );
        wxSize box_offset = box_size + wxSize( m_padding - 2, m_padding );
        wxSize text_offset = box_offset - wxSize( 3, 1 );

        dc.SetPen( wxPen( m_badgeColor ) );
        dc.SetBrush( wxBrush( m_badgeColor ) );
        dc.DrawRoundedRectangle( rect.GetRightBottom() - box_offset, box_size, -0.25 );

        dc.SetTextForeground( m_badgeTextColor );
        dc.DrawText( m_badgeText, rect.GetRightBottom() - text_offset );
    }
}


bool BITMAP_BUTTON::Enable( bool aEnable )
{
    // If the requested state is already the current state, don't do anything
    if( aEnable != hasFlag( wxCONTROL_DISABLED ) )
        return false;

    wxPanel::Enable( aEnable );

    if( aEnable && hasFlag( wxCONTROL_DISABLED ) )
    {
        clearFlag( wxCONTROL_DISABLED );
        Refresh();
    }

    if( !aEnable && !hasFlag( wxCONTROL_DISABLED ) )
    {
        setFlag( wxCONTROL_DISABLED );
        Refresh();
    }

    return true;
}


void BITMAP_BUTTON::SetIsCheckButton()
{
    setFlag( wxCONTROL_CHECKABLE );
}


void BITMAP_BUTTON::SetIsRadioButton()
{
    setFlag( wxCONTROL_CHECKABLE );
    m_isRadioButton = true;
}


void BITMAP_BUTTON::SetIsSeparator()
{
    setFlag( wxCONTROL_SEPARATOR | wxCONTROL_DISABLED );
    SetMinSize( wxSize( m_padding * 2, wxButton::GetDefaultSize().y ) );
}


void BITMAP_BUTTON::Check( bool aCheck )
{
    wxASSERT_MSG( hasFlag( wxCONTROL_CHECKABLE ), wxS( "Button is not a checkButton." ) );

    if( aCheck && !hasFlag( wxCONTROL_CHECKED ) )
    {
        setFlag( wxCONTROL_CHECKED );
        Refresh();
    }

    if( !aCheck && hasFlag( wxCONTROL_CHECKED ) )
    {
        clearFlag( wxCONTROL_CHECKED );
        Refresh();
    }
}


bool BITMAP_BUTTON::IsChecked() const
{
    wxASSERT_MSG( hasFlag( wxCONTROL_CHECKABLE ), wxS( "Button is not a checkButton." ) );

    return hasFlag( wxCONTROL_CHECKED );
}
