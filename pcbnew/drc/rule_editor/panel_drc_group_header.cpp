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

#include <drc/rule_editor/panel_drc_group_header.h>


PANEL_DRC_GROUP_HEADER::PANEL_DRC_GROUP_HEADER( wxWindow* aParent, const std::vector<DRC_RULE_ROW>& aRows ) :
        PANEL_DRC_GROUP_HEADER_BASE( aParent ),
        m_rows( aRows )
{
    int cols = m_dataGrid->GetNumberCols();

    if( cols )
        m_dataGrid->DeleteCols( 0, cols );

    int rows = m_dataGrid->GetNumberRows();

    if( rows )
        m_dataGrid->DeleteRows( 0, rows );

    m_dataGrid->AppendCols( 3 );
    m_dataGrid->SetColLabelValue( 0, _( "Rule type" ) );
    m_dataGrid->SetColLabelValue( 1, _( "Rule name" ) );
    m_dataGrid->SetColLabelValue( 2, _( "Comment" ) );

    m_dataGrid->AppendRows( m_rows.size() );

    for( size_t i = 0; i < m_rows.size(); ++i )
    {
        m_dataGrid->SetCellValue( i, 0, m_rows[i].m_ruleType );
        m_dataGrid->SetCellValue( i, 1, m_rows[i].m_ruleName );
        m_dataGrid->SetCellValue( i, 2, m_rows[i].m_comment );
    }

    // Disable horizontal scroll bar
    m_dataGrid->EnableScrolling( false, true );
}


PANEL_DRC_GROUP_HEADER::~PANEL_DRC_GROUP_HEADER()
{
}


bool PANEL_DRC_GROUP_HEADER::TransferDataToWindow()
{
    return true;
}


bool PANEL_DRC_GROUP_HEADER::TransferDataFromWindow()
{
    return true;
}

void PANEL_DRC_GROUP_HEADER::OnSize( wxSizeEvent& event )
{
    wxSize clientSize = GetClientSize();
    int    availableWidth = clientSize.GetWidth() - wxSystemSettings::GetMetric( wxSYS_VSCROLL_X );
    int    availableHeight = clientSize.GetHeight() - wxSystemSettings::GetMetric( wxSYS_HSCROLL_Y );

    m_dataGrid->SetSize( wxSize( availableWidth, availableHeight ) );
    double total_column_width = m_dataGrid->GetColSize( 0 ) + m_dataGrid->GetColSize( 1 ) + m_dataGrid->GetColSize( 2 );
    double col0_width_ratio = m_dataGrid->GetColSize( 0 ) / total_column_width;
    double col1_width_ratio = m_dataGrid->GetColSize( 1 ) / total_column_width;
    int col0_size = static_cast<int>( availableWidth * col0_width_ratio );
    int col1_size = static_cast<int>( availableWidth * col1_width_ratio );
    int col2_size = availableWidth - col0_size - col1_size;

    m_dataGrid->SetColSize( 0, col0_size );
    m_dataGrid->SetColSize( 1, col1_size );
    m_dataGrid->SetColSize( 2, col2_size );

    // Refresh the grid to apply the new sizes.
    m_dataGrid->ForceRefresh();

    event.Skip(); // Allow further processing of the event.
}

void PANEL_DRC_GROUP_HEADER::OnGridSize( wxGridSizeEvent& event )
{
    wxSizeEvent evt( m_dataGrid->GetSize() );
    OnSize( evt );
}