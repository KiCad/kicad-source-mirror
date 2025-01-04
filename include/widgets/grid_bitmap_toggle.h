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

#ifndef KICAD_GRID_BITMAP_TOGGLE_H
#define KICAD_GRID_BITMAP_TOGGLE_H

#include <wx/bmpbndl.h>
#include <wx/grid.h>


/**
 * A toggle button renderer for a wxGrid, similar to #BITMAP_TOGGLE.
 */
class GRID_BITMAP_TOGGLE_RENDERER : public wxGridCellRenderer
{
public:
    GRID_BITMAP_TOGGLE_RENDERER( const wxBitmapBundle& aCheckedBitmap,
                                 const wxBitmapBundle& aUncheckedBitmap );

    ~GRID_BITMAP_TOGGLE_RENDERER() {}

    GRID_BITMAP_TOGGLE_RENDERER* Clone() const override;

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc, const wxRect& aRect,
               int aRow, int aCol, bool aIsSelected ) override;

    wxSize GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc,
                        int aRow, int aCol ) override;

private:
    wxBitmapBundle m_bitmapChecked;

    wxBitmapBundle m_bitmapUnchecked;
};

#endif // KICAD_GRID_BITMAP_TOGGLE_H
