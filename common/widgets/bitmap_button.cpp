/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney at ieee dot org>
 * Copyright (C) 2020-2021 Kicad Developers, see AUTHORS.txt for contributors.
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


BITMAP_BUTTON::BITMAP_BUTTON( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos,
                              const wxSize& aSize, int aStyles ) :
        wxPanel( aParent, aId, aPos, aSize, aStyles ),
        m_buttonState( 0 ),
        m_padding( 0 )
{
    if( aSize == wxDefaultSize )
        SetMinSize( wxButton::GetDefaultSize() );

    Bind( wxEVT_PAINT,        &BITMAP_BUTTON::OnPaint,          this );
    Bind( wxEVT_LEFT_UP,      &BITMAP_BUTTON::OnLeftButtonUp,   this );
    Bind( wxEVT_LEFT_DOWN,    &BITMAP_BUTTON::OnLeftButtonDown, this );
    Bind( wxEVT_LEAVE_WINDOW, &BITMAP_BUTTON::OnLeave,          this );
    Bind( wxEVT_ENTER_WINDOW, &BITMAP_BUTTON::OnEnter,          this );
    Bind( wxEVT_KILL_FOCUS,   &BITMAP_BUTTON::OnLeave,          this );
    Bind( wxEVT_SET_FOCUS,    &BITMAP_BUTTON::OnEnter,          this );
}


BITMAP_BUTTON::~BITMAP_BUTTON()
{
}


void BITMAP_BUTTON::SetPadding( int aPadding )
{
    m_padding = aPadding;
    SetMinSize( m_unadjustedMinSize + wxSize( aPadding * 2, aPadding * 2) );
}


void BITMAP_BUTTON::SetBitmap( const wxBitmap& aBmp )
{
    m_normalBitmap = aBmp;
    m_unadjustedMinSize = aBmp.GetSize();

    SetMinSize( wxSize( aBmp.GetWidth() + ( m_padding * 2 ), aBmp.GetHeight() + ( m_padding * 2 ) ) );
}


void BITMAP_BUTTON::SetDisabledBitmap( const wxBitmap& aBmp )
{
    m_disabledBitmap = aBmp;
}


void BITMAP_BUTTON::OnLeave( wxEvent& aEvent )
{
    clearFlag( wxCONTROL_CURRENT );
    Refresh();

    aEvent.Skip();
}


void BITMAP_BUTTON::OnEnter( wxEvent& aEvent )
{
    setFlag( wxCONTROL_CURRENT );
    Refresh();

    aEvent.Skip();
}


void BITMAP_BUTTON::OnLeftButtonUp( wxMouseEvent& aEvent )
{
    // Only create a button event when the control is enabled
    if( !hasFlag( wxCONTROL_DISABLED ) )
    {
        wxEvtHandler* pEventHandler = GetEventHandler();
        wxASSERT( pEventHandler );

        pEventHandler->CallAfter(
                [=]()
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
    setFlag( wxCONTROL_PRESSED );
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

    // This drawing is done so the button looks the same as an AUI toolbar button
    if( !hasFlag( wxCONTROL_DISABLED ) )
    {
        if( hasFlag( wxCONTROL_PRESSED ) )
        {
            dc.SetPen( wxPen( highlightColor ) );
            dc.SetBrush( wxBrush( highlightColor.ChangeLightness( darkMode ? 20 : 150 ) ) );
            dc.DrawRectangle( rect );
        }
        else if( hasFlag( wxCONTROL_CURRENT ) )
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

    const wxBitmap& bmp = hasFlag( wxCONTROL_DISABLED ) ? m_disabledBitmap : m_normalBitmap;

    // Draw the bitmap with the upper-left corner offset by the padding
    if( bmp.IsOk() )
        dc.DrawBitmap( bmp, m_padding, m_padding, true );
}


bool BITMAP_BUTTON::Enable( bool aEnable )
{
    // If the requested state is already the current state, don't do anything
    if( aEnable != hasFlag( wxCONTROL_DISABLED ) )
        return false;

    wxPanel::Enable( aEnable );

    if( aEnable )
        clearFlag( wxCONTROL_DISABLED );
    else
        setFlag( wxCONTROL_DISABLED );

    Refresh();

    return true;
}


void BITMAP_BUTTON::Check( bool aCheck )
{
    if( aCheck )
        setFlag( wxCONTROL_CHECKED );
    else
        clearFlag( wxCONTROL_CHECKED );

    Refresh();
}
