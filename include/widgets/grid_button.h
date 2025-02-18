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

#ifndef KICAD_GRID_BUTTON_H
#define KICAD_GRID_BUTTON_H

#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/grid.h>


class GRID_BUTTON_RENDERER : public wxGridCellRenderer
{
public:
    GRID_BUTTON_RENDERER( const wxString& aLabel );

    virtual ~GRID_BUTTON_RENDERER() = default;

    GRID_BUTTON_RENDERER* Clone() const override;

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc, const wxRect& aRect,
               int aRow, int aCol, bool aIsSelected ) override;

    wxSize GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc,
                        int aRow, int aCol ) override;

private:
    wxButton m_button;
};


class GRID_BITMAP_BUTTON_RENDERER : public wxGridCellRenderer
{
public:
    GRID_BITMAP_BUTTON_RENDERER( const wxBitmapBundle& aBitmap );

    virtual ~GRID_BITMAP_BUTTON_RENDERER() = default;

    GRID_BITMAP_BUTTON_RENDERER* Clone() const override;

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc, const wxRect& aRect,
               int aRow, int aCol, bool aIsSelected ) override;

    wxSize GetBestSize( wxGrid& aGrid, wxGridCellAttr& aAttr, wxDC& aDc,
                        int aRow, int aCol ) override;

private:
    wxBitmapBundle m_bitmap;
};

#endif //KICAD_GRID_BUTTON_H
