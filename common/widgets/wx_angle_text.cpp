/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <wx/wx.h>
#include <widgets/wx_angle_text.h>
#include <eda_rect.h>

WX_ANGLE_TEXT::WX_ANGLE_TEXT( wxWindow* aParent, wxWindowID aId, const wxString& aLabel,
                              const wxPoint& aPos, double aAngle ) :
        wxPanel( aParent, aId, aPos, wxDefaultSize ),
        m_label( aLabel ),
        m_angle( aAngle )
{
    wxFont font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    font.SetPointSize( font.GetPointSize() + 2 );   // rotated text looks visually smaller
    SetFont( font );

    // Measure the size of the text
    wxClientDC dc( this );
    dc.SetFont( font );

    wxSize   size = dc.GetTextExtent( m_label );
    wxPoint  pos  = GetPosition();
    EDA_RECT rect( wxPoint( 0, 0 ), size );
    EDA_RECT bbox = rect.GetBoundingBoxRotated( rect.GetPosition(), m_angle );

    pos.y += bbox.GetTop() + ( rect.GetBottom() - bbox.GetBottom() );
    size.y = bbox.GetHeight();
    size.x = bbox.GetWidth();

    Move( pos );
    SetSize( size );

    Bind( wxEVT_ERASE_BACKGROUND, &WX_ANGLE_TEXT::OnEraseBackground, this );
    Bind( wxEVT_PAINT, &WX_ANGLE_TEXT::OnPaint, this );
}


void WX_ANGLE_TEXT::OnEraseBackground( wxEraseEvent& WXUNUSED( aEvent ) )
{
    // NOP so that we don't erase other labels which intersect
}


void WX_ANGLE_TEXT::OnPaint( wxPaintEvent& WXUNUSED( aEvent ) )
{
    wxPaintDC dc( this );

    dc.SetFont( GetFont() );
    dc.SetTextForeground( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
    dc.SetTextBackground( wxTransparentColor );

    wxSize   size = dc.GetTextExtent( m_label );
    EDA_RECT rect( wxPoint( 0, 0 ), size );
    EDA_RECT bbox = rect.GetBoundingBoxRotated( rect.GetPosition(), m_angle );
    wxPoint  pos( 0, -bbox.GetTop() );

    dc.DrawRotatedText( m_label, pos, m_angle / 10 );
}


EDA_RECT WX_ANGLE_TEXT::GetBoundingBox( wxWindow* aWindow, const wxString& aLabel, double aAngle )
{
    // Create the font used for the text
    wxFont font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    font.SetPointSize( font.GetPointSize() + 2 );   // rotated text looks visually smaller

    // Measure the size of the text
    wxClientDC dc( aWindow );
    dc.SetFont( font );

    wxSize   size = dc.GetTextExtent( aLabel );
    EDA_RECT rect( wxPoint( 0, 0 ), size );

    return rect.GetBoundingBoxRotated( rect.GetPosition(), aAngle );
}
