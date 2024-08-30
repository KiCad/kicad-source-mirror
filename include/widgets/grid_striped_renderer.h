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
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#pragma once

#include <wx/colour.h>
#include <wx/grid.h>
#include <wx/generic/gridctrl.h>

class wxGrid;

template<typename T = wxGridCellStringRenderer>
class STRIPED_CELL_RENDERER : public T
{
public:
    STRIPED_CELL_RENDERER() : T() {}

    // Perfect forwarding constructor to handle any base renderer constructor arguments
    template<typename... Args>
    explicit STRIPED_CELL_RENDERER(Args&&... args) : T(std::forward<Args>(args)...) {}

    void Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc,
               const wxRect& rect, int row, int col, bool isSelected ) override
    {
        // First draw the striped background for empty cells
        wxString cellValue = grid.GetCellValue( row, col );

        if( cellValue.IsEmpty() )
        {
            drawStripedBackground( dc, attr, rect, isSelected );
            attr.SetBackgroundColour( wxColour( 0, 0, 0, wxALPHA_TRANSPARENT ) );
        }

        // Then draw the foreground content using the base renderer
        T::Draw( grid, attr, dc, rect, row, col, isSelected );
    }

    wxGridCellRenderer* Clone() const override
    {
        return new STRIPED_CELL_RENDERER<T>(*this);
    }

private:
    void drawStripedBackground(wxDC& dc, wxGridCellAttr& attr, const wxRect& rect, bool isSelected) const
    {
        if( isSelected )
        {
            // For selected cells, use the selection color
            dc.SetBrush( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) ) );
            dc.SetPen( *wxTRANSPARENT_PEN );
            dc.DrawRectangle( rect );
            return;
        }

        // First fill with background color
        wxColour bgColor = attr.GetBackgroundColour();
        dc.SetBrush( wxBrush( bgColor ) );
        dc.SetPen( *wxTRANSPARENT_PEN );
        dc.DrawRectangle( rect );

        // Draw diagonal stripes
        const int stripeSpacing = 20;           // Distance between diagonal lines

        wxColour stripeColor;

        int bgLuminance = bgColor.GetLuminance();

        if( bgLuminance < 128 )
            stripeColor = wxColour( 220, 180, 180 ); // Light red for stripes
        else
            stripeColor = wxColour( 100, 10, 10 ); // Dark red for stripes

        wxPen stripePen( stripeColor, 1, wxPENSTYLE_SOLID );
        dc.SetPen( stripePen );

        // Calculate the diagonal stripes from top-left to bottom-right
        int startX = rect.GetLeft() - rect.GetHeight();
        int endX = rect.GetRight() + rect.GetHeight();

        // Draw diagonal lines spaced evenly
        for( int x = startX; x < endX; x += stripeSpacing )
        {
            int x1 = x;
            int y1 = rect.GetTop();
            int x2 = x + rect.GetHeight();
            int y2 = rect.GetBottom();

            // Clip the line to the rectangle bounds
            if( x1 < rect.GetLeft() )
            {
                int deltaY = rect.GetLeft() - x1;
                x1 = rect.GetLeft();
                y1 = rect.GetTop() + deltaY;
            }

            if( x2 > rect.GetRight() )
            {
                int deltaY = x2 - rect.GetRight();
                x2 = rect.GetRight();
                y2 = rect.GetBottom() - deltaY;
            }

            // Only draw if the line is within the rectangle
            if( x1 <= rect.GetRight() && x2 >= rect.GetLeft() && y1 <= rect.GetBottom() && y2 >= rect.GetTop() )
            {
                dc.DrawLine( x1, y1, x2, y2 );
            }
        }
    }
};


using STRIPED_STRING_RENDERER = STRIPED_CELL_RENDERER<wxGridCellStringRenderer>;
using STRIPED_NUMBER_RENDERER = STRIPED_CELL_RENDERER<wxGridCellNumberRenderer>;
using STRIPED_BOOL_RENDERER = STRIPED_CELL_RENDERER<wxGridCellBoolRenderer>;