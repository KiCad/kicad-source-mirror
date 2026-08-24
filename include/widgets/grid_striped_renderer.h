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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <utility>

#include <wx/colour.h>
#include <wx/grid.h>
#include <wx/generic/gridctrl.h>

class wxGrid;

template<typename T = wxGridCellStringRenderer>
class STRIPED_CELL_RENDERER : public T
{
public:
    template <typename... Args>
    STRIPED_CELL_RENDERER( const wxColour& aStripeOnDarkBackground, const wxColour& aStripeOnLightBackground,
                           Args&&... aArgs ) :
            T( std::forward<Args>( aArgs )... ),
            m_stripeOnDarkBackground( aStripeOnDarkBackground ),
            m_stripeOnLightBackground( aStripeOnLightBackground )
    {
    }

    void Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc,
               const wxRect& rect, int row, int col, bool isSelected ) override
    {
        // Draw the foreground content using the base renderer first
        T::Draw( grid, attr, dc, rect, row, col, isSelected );

        // Overlay striped background for empty cells
        if( grid.GetCellValue( row, col ).IsEmpty() )
            drawStripedBackground( dc, attr, rect, isSelected );
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

        int bgLuminance = bgColor.GetLuminance();
        wxColour stripeColor = bgLuminance < 128 ? m_stripeOnDarkBackground : m_stripeOnLightBackground;

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

    wxColour m_stripeOnDarkBackground;
    wxColour m_stripeOnLightBackground;
};


using STRIPED_STRING_RENDERER = STRIPED_CELL_RENDERER<wxGridCellStringRenderer>;
using STRIPED_NUMBER_RENDERER = STRIPED_CELL_RENDERER<wxGridCellNumberRenderer>;
using STRIPED_BOOL_RENDERER = STRIPED_CELL_RENDERER<wxGridCellBoolRenderer>;
