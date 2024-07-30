/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef WX_GRID_AUTOSIZER_H
#define WX_GRID_AUTOSIZER_H

#include <map>

#include <wx/grid.h>

/**
 * Class that manages autosizing of columns in a wxGrid.
 *
 * The class will automatically resize the columns in the grid to fit the content,
 * with one column being flexible and taking up the remaining space.
 */
class WX_GRID_AUTOSIZER
{
public:
    /**
     * Map of column indices to minimum widths.
     *
     * Use 0 to indicate that a column should be autosized to fit content,
     * but without a minimum width.
     */
    using COL_MIN_WIDTHS = std::map<int, int>;

    /**
     * @param aGrid The grid to manage.
     * @param aAutosizedCols A map of columns to autosize: these will sized to fit,
     *                       but not smaller than the width specified.
     * @param aFlexibleCol The column that will take up the remaining space,
     *                     with a minimum width if given in the aAutosizedCols map.
     */
    WX_GRID_AUTOSIZER( wxGrid& aGrid, COL_MIN_WIDTHS aAutosizedCols, unsigned aFlexibleCol );

private:
    void recomputeGridWidths();

    void onSizeEvent( wxSizeEvent& aEvent );

    wxGrid&        m_grid;
    COL_MIN_WIDTHS m_autosizedCols;
    int            m_flexibleCol;

    bool m_gridWidthsDirty = true;
    int  m_gridWidth = 0;
};

#endif // WX_GRID_AUTOSIZER_H