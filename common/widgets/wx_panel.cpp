/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <widgets/wx_panel.h>
#include <wx/dcclient.h>
#include <wx/settings.h>

WX_PANEL::WX_PANEL( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                    long style, const wxString& name ) :
        wxPanel( parent, id, pos, size, style, name ),
        m_leftBorder( false ),
        m_rightBorder( false ),
        m_topBorder( false ),
        m_bottomBorder( false ),
        m_borderColor( KIGFX::COLOR4D::UNSPECIFIED )
{
    this->Connect( wxEVT_PAINT, wxPaintEventHandler( WX_PANEL::OnPaint ) );
}


WX_PANEL::~WX_PANEL()
{
	this->Disconnect( wxEVT_PAINT, wxPaintEventHandler( WX_PANEL::OnPaint ) );
}


void WX_PANEL::OnPaint( wxPaintEvent& event )
{
    wxRect    rect( wxPoint( 0, 0 ), GetClientSize() );
    wxPaintDC dc( this );

    KIGFX::COLOR4D border = m_borderColor;

    if( border == KIGFX::COLOR4D::UNSPECIFIED )
    {
        KIGFX::COLOR4D bg = wxSystemSettings::GetColour( wxSYS_COLOUR_FRAMEBK );
        KIGFX::COLOR4D fg = wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER );
        border = fg.Mix( bg, 0.18 );
    }

    dc.SetPen( wxPen( border.ToColour(), 1 ) );

    if( m_leftBorder )
        dc.DrawLine( rect.GetLeft(), rect.GetTop(), rect.GetLeft(), rect.GetBottom() );

    if( m_rightBorder )
        dc.DrawLine( rect.GetRight(), rect.GetTop(), rect.GetRight(), rect.GetBottom() );

    if( m_topBorder )
        dc.DrawLine( rect.GetLeft(), rect.GetTop(), rect.GetRight(), rect.GetTop() );

    if( m_bottomBorder )
        dc.DrawLine( rect.GetLeft(), rect.GetBottom(), rect.GetRight(), rect.GetBottom() );
}


