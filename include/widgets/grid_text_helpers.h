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

#ifndef KICAD_GRID_TEXT_HELPERS_H
#define KICAD_GRID_TEXT_HELPERS_H

#include <wx/generic/gridctrl.h>

class wxGrid;

/**
 * A text renderer that can unescape text for display
 * This is useful where it's desired to keep the underlying storage escaped.
 */
class GRID_CELL_ESCAPED_TEXT_RENDERER : public wxGridCellStringRenderer
{
public:
    GRID_CELL_ESCAPED_TEXT_RENDERER();

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDC,
               const wxRect& aRect, int aRow, int aCol, bool isSelected ) override;

    wxSize GetBestSize( wxGrid & grid, wxGridCellAttr & attr, wxDC & dc,
                        int row, int col ) override;
};

#endif // KICAD_GRID_TEXT_HELPERS_H
