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

#include <widgets/grid_button.h>
#include <wx/dc.h>
#include <wx/renderer.h>


GRID_BUTTON_RENDERER::GRID_BUTTON_RENDERER( const wxString& aLabel ) :
        wxGridCellRenderer(),
        m_button( nullptr, wxID_ANY, aLabel )
{
}


GRID_BUTTON_RENDERER* GRID_BUTTON_RENDERER::Clone() const
{
    GRID_BUTTON_RENDERER* clone = new GRID_BUTTON_RENDERER( m_button.GetLabel() );
    return clone;
}


void GRID_BUTTON_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc,
                                 const wxRect& aRect, int aRow, int aCol, bool aIsSelected )
{
    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDc, aRect, aRow, aCol, aIsSelected );

    wxRendererNative::Get().DrawPushButton( &aGrid, aDc, aRect );
}


wxSize GRID_BUTTON_RENDERER::GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc,
                                          int aRow, int aCol)
{
    return m_button.GetSize();
}


GRID_BITMAP_BUTTON_RENDERER::GRID_BITMAP_BUTTON_RENDERER( const wxBitmapBundle& aBitmap ) :
        m_bitmap( aBitmap )
{
}

static const wxSize BUTTON_PADDING( 2, 2 );


GRID_BITMAP_BUTTON_RENDERER* GRID_BITMAP_BUTTON_RENDERER::Clone() const
{
    auto clone = new GRID_BITMAP_BUTTON_RENDERER( m_bitmap );
    return clone;
}


void GRID_BITMAP_BUTTON_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc,
                                        const wxRect& aRect, int aRow, int aCol, bool aIsSelected )
{
    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDc, aRect, aRow, aCol, aIsSelected );


    wxBitmap bmp = m_bitmap.GetBitmapFor( &aGrid );
    wxRect bmpRect( wxPoint( 0, 0 ), m_bitmap.GetPreferredLogicalSizeFor( &aGrid ) );
    bmpRect = bmpRect.CenterIn( aRect );

    wxRendererNative::Get().DrawPushButton( &aGrid, aDc, bmpRect.Inflate( BUTTON_PADDING ) );
    aDc.DrawBitmap( bmp, bmpRect.CenterIn( aRect ).GetTopLeft() + BUTTON_PADDING );
}


wxSize GRID_BITMAP_BUTTON_RENDERER::GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc,
                                                 int aRow, int aCol)
{
    return m_bitmap.GetPreferredBitmapSizeFor( &aGrid ) + BUTTON_PADDING;
}
