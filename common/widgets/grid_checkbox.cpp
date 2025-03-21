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

#include "widgets/ui_common.h"
#include <widgets/grid_checkbox.h>
#include <wx/renderer.h>


//-------- Renderer ---------------------------------------------------------------------
//
// Augments wxGridBoolRenderer to support indeterminate state

// ------------ This section copied verbatim from wxwidgets/src/generic/grid.cpp (which
//              is sadly private)

// the space between the cell edge and the checkbox mark
const int GRID_CELL_CHECKBOX_MARGIN = 2;

wxRect
wxGetContentRect(wxSize contentSize,
                 const wxRect& cellRect,
                 int hAlign,
                 int vAlign)
{
    // Keep square aspect ratio for the checkbox, but ensure that it fits into
    // the available space, even if it's smaller than the standard size.
    const wxCoord minSize = wxMin(cellRect.width, cellRect.height);
    if ( contentSize.x >= minSize || contentSize.y >= minSize )
    {
        // It must still have positive size, however.
        const int fittingSize = wxMax(1, minSize - 2*GRID_CELL_CHECKBOX_MARGIN);

        contentSize.x =
        contentSize.y = fittingSize;
    }

    wxRect contentRect(contentSize);

    if ( hAlign & wxALIGN_CENTER_HORIZONTAL )
    {
        contentRect = contentRect.CentreIn(cellRect, wxHORIZONTAL);
    }
    else if ( hAlign & wxALIGN_RIGHT )
    {
        contentRect.SetX(cellRect.x + cellRect.width
                          - contentSize.x - GRID_CELL_CHECKBOX_MARGIN);
    }
    else // ( hAlign == wxALIGN_LEFT ) and invalid alignment value
    {
        contentRect.SetX(cellRect.x + GRID_CELL_CHECKBOX_MARGIN);
    }

    if ( vAlign & wxALIGN_CENTER_VERTICAL )
    {
        contentRect = contentRect.CentreIn(cellRect, wxVERTICAL);
    }
    else if ( vAlign & wxALIGN_BOTTOM )
    {
        contentRect.SetY(cellRect.y + cellRect.height
                          - contentSize.y - GRID_CELL_CHECKBOX_MARGIN);
    }
    else // wxALIGN_TOP
    {
        contentRect.SetY(cellRect.y + GRID_CELL_CHECKBOX_MARGIN);
    }

    return contentRect;
}

// ------------ End copied section

// ------------ This section copied from wxwidgets/src/generic/gridctrl.cpp, with changes
//              noted

void GRID_CELL_CHECKBOX_RENDERER::Draw(wxGrid& grid,
                                       wxGridCellAttr& attr,
                                       wxDC& dc,
                                       const wxRect& rect,
                                       int row, int col,
                                       bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);

    int hAlign = wxALIGN_LEFT;
    int vAlign = wxALIGN_CENTRE_VERTICAL;
    attr.GetNonDefaultAlignment(&hAlign, &vAlign);

    const wxRect checkBoxRect =
        wxGetContentRect(GetBestSize(grid, attr, dc, row, col),
                         rect, hAlign, vAlign);

    bool value;
    if ( grid.GetTable()->CanGetValueAs(row, col, wxGRID_VALUE_BOOL) )
    {
        value = grid.GetTable()->GetValueAsBool(row, col);
    }
    else
    {
        wxString cellval( grid.GetTable()->GetValue(row, col) );
        value = wxGridCellBoolEditor::IsTrueValue(cellval);
    }

    int flags = wxCONTROL_CELL;
#if 1   // KiCad change
    if( grid.GetTable()->GetValue( row, col ) == INDETERMINATE_STATE )
        flags |= wxCONTROL_UNDETERMINED;
    else
#endif
    if (value)
        flags |= wxCONTROL_CHECKED;

    wxRendererNative::Get().DrawCheckBox( &grid, dc, checkBoxRect, flags );
}

// ------------ End copied section



//-------- Editor -----------------------------------------------------------------------
//
// None required; interactivity delegated to GRID_TRICKS.


