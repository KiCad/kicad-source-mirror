/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <string_utils.h>
#include <widgets/grid_text_helpers.h>


GRID_CELL_ESCAPED_TEXT_RENDERER::GRID_CELL_ESCAPED_TEXT_RENDERER() :
        wxGridCellStringRenderer()
{
}

void GRID_CELL_ESCAPED_TEXT_RENDERER::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
                                            const wxRect& aRect, int aRow, int aCol,
                                            bool isSelected )
{
    wxString unescaped = UnescapeString( aGrid.GetCellValue( aRow, aCol ) );

    wxRect rect = aRect;
    rect.Inflate( -1 );

    // erase background
    wxGridCellRenderer::Draw( aGrid, aAttr, aDC, aRect, aRow, aCol, isSelected );

    SetTextColoursAndFont( aGrid, aAttr, aDC, isSelected );
    aGrid.DrawTextRectangle( aDC, unescaped, rect, wxALIGN_LEFT, wxALIGN_CENTRE );
}


wxSize GRID_CELL_ESCAPED_TEXT_RENDERER::GetBestSize( wxGrid & aGrid, wxGridCellAttr & aAttr,
                                                     wxDC & aDC, int aRow, int aCol )
{
    wxString unescaped = UnescapeString( aGrid.GetCellValue( aRow, aCol ) );
    return wxGridCellStringRenderer::DoGetBestSize( aAttr, aDC, unescaped );
}
