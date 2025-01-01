/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <widgets/grid_bitmap_toggle.h>
#include <wx/dc.h>

#include <algorithm>


GRID_BITMAP_TOGGLE_RENDERER::GRID_BITMAP_TOGGLE_RENDERER( const wxBitmapBundle& aCheckedBitmap,
                                                          const wxBitmapBundle& aUncheckedBitmap ) :
        wxGridCellRenderer(),
        m_bitmapChecked( aCheckedBitmap ),
        m_bitmapUnchecked( aUncheckedBitmap )
{
}


GRID_BITMAP_TOGGLE_RENDERER* GRID_BITMAP_TOGGLE_RENDERER::Clone() const
{
    GRID_BITMAP_TOGGLE_RENDERER* clone = new GRID_BITMAP_TOGGLE_RENDERER( m_bitmapChecked,
                                                                          m_bitmapUnchecked );
    return clone;
}


void GRID_BITMAP_TOGGLE_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc,
                                        const wxRect& aRect, int aRow, int aCol, bool aIsSelected )
{
    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDc, aRect, aRow, aCol, aIsSelected );

    bool checked = aGrid.GetCellValue( aRow, aCol ) == "1";
    const wxBitmapBundle& bundle = checked ? m_bitmapChecked : m_bitmapUnchecked;
    wxBitmap bitmap = bundle.GetBitmapFor( &aGrid );

    int x = std::max( 0, ( aRect.GetWidth() - bitmap.GetWidth() ) / 2 );
    int y = std::max( 0, ( aRect.GetHeight() - bitmap.GetHeight() ) / 2 );

    aDc.DrawBitmap( bitmap, aRect.GetTopLeft() + wxPoint( x, y ) );
}


wxSize GRID_BITMAP_TOGGLE_RENDERER::GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc,
                                                 int aRow, int aCol)
{
    return m_bitmapChecked.GetPreferredBitmapSizeFor( &aGrid );
}
