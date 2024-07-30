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

#include "widgets/wx_grid_autosizer.h"

#include <optional>

#include <wx/settings.h>


WX_GRID_AUTOSIZER::WX_GRID_AUTOSIZER( wxGrid& aGrid, COL_MIN_WIDTHS aAutosizedCols,
                                      unsigned aFlexibleCol ) :
        m_grid( aGrid ), m_autosizedCols( std::move( aAutosizedCols ) ),
        m_flexibleCol( aFlexibleCol )
{
    const int colCount = m_grid.GetNumberCols();
    for( const auto& [colIndex, width] : m_autosizedCols )
    {
        wxASSERT_MSG( colIndex < colCount, "Autosized column does not exist in grid" );
    }
    wxASSERT_MSG( m_flexibleCol < colCount, "Flexible column index does not exist in grid" );

    m_grid.Bind( wxEVT_UPDATE_UI,
                 [this]( wxUpdateUIEvent& aEvent )
                 {
                     recomputeGridWidths();
                 } );

    m_grid.Bind( wxEVT_SIZE,
                 [this]( wxSizeEvent& aEvent )
                 {
                     onSizeEvent( aEvent );
                 } );

    // Handles the case when the user changes the cell content to be longer than the
    // current column size
    m_grid.Bind( wxEVT_GRID_CELL_CHANGED,
                 [this]( wxGridEvent& aEvent )
                 {
                     m_gridWidthsDirty = true;
                     aEvent.Skip();
                 } );
}

void WX_GRID_AUTOSIZER::recomputeGridWidths()
{
    if( m_gridWidthsDirty )
    {
        const int width =
                m_grid.GetClientRect().GetWidth() - wxSystemSettings::GetMetric( wxSYS_VSCROLL_X );

        std::optional<int> flexibleMinWidth;
        for( const auto& [colIndex, minWidth] : m_autosizedCols )
        {
            m_grid.AutoSizeColumn( colIndex );
            const int colSize = m_grid.GetColSize( colIndex );
            m_grid.SetColSize( colIndex, std::max( minWidth, colSize ) );

            if( colIndex == m_flexibleCol )
            {
                flexibleMinWidth = minWidth;
            }
        }

        // Gather all the widths except the flexi one
        int nonFlexibleWidth = 0;

        for( int i = 0; i < m_grid.GetNumberCols(); ++i )
        {
            if( i != m_flexibleCol )
            {
                nonFlexibleWidth += m_grid.GetColSize( i );
            }
        }

        m_grid.SetColSize( m_flexibleCol,
                           std::max( flexibleMinWidth.value_or( 0 ), width - nonFlexibleWidth ) );

        // Store the state for next time
        m_gridWidth = m_grid.GetSize().GetX();
        m_gridWidthsDirty = false;
    }
}

void WX_GRID_AUTOSIZER::onSizeEvent( wxSizeEvent& aEvent )
{
    const int width = aEvent.GetSize().GetX();
    if( width != m_gridWidth )
    {
        m_gridWidthsDirty = true;
    }
    aEvent.Skip();
}