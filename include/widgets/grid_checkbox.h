/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GRID_CELL_CHECKBOX_H
#define GRID_CELL_CHECKBOX_H

#include <wx/grid.h>
#include <wx/generic/gridctrl.h>


class wxGrid;


class GRID_CELL_CHECKBOX_RENDERER : public wxGridCellBoolRenderer
{
public:
    GRID_CELL_CHECKBOX_RENDERER() :
            wxGridCellBoolRenderer()
    {
    }

    GRID_CELL_CHECKBOX_RENDERER( const GRID_CELL_CHECKBOX_RENDERER& aOther ) :
            wxGridCellBoolRenderer( aOther )
    {
    }

    void Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect,
               int row, int col, bool isSelected ) override;
};


#endif  // GRID_CELL_CHECKBOX_H
