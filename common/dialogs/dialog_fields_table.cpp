/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) Mike Williams
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dialog_fields_table.h"
#include "fields_view_controls_grid_data_model.h"

#include <algorithm>
#include <bitmaps.h>
#include <wildcards_and_files_ext.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/ui_common.h>
#include <wx/filename.h>

#ifdef __WXMAC__
#define COLUMN_MARGIN 4
#else
#define COLUMN_MARGIN 15
#endif


void DIALOG_FIELDS_TABLE::ShowEditTab()
{
    m_nbPages->SetSelection( 0 );
}


void DIALOG_FIELDS_TABLE::ShowExportTab()
{
    m_nbPages->SetSelection( 1 );
}


wxString DIALOG_FIELDS_TABLE::GetDefaultBomFileName( const wxString& aInputFileName )
{
    if( aInputFileName.IsEmpty() )
        return wxEmptyString;

    wxFileName fn( aInputFileName );
    fn.SetExt( FILEEXT::CsvFileExtension );

    return fn.GetFullPath();
}


void DIALOG_FIELDS_TABLE::setSideBarButtonLook( bool aIsLeftPanelCollapsed )
{
    // Set bitmap and tooltip according to left panel visibility

    if( aIsLeftPanelCollapsed )
    {
        m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::right ) );
        m_sidebarButton->SetToolTip( _( "Expand left panel" ) );
    }
    else
    {
        m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::left ) );
        m_sidebarButton->SetToolTip( _( "Collapse left panel" ) );
    }
}


void DIALOG_FIELDS_TABLE::OnTableValueChanged( wxGridEvent& aEvent )
{
    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_TABLE::OnTableColSize( wxGridSizeEvent& aEvent )
{
    aEvent.Skip();

    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_TABLE::OnFilterMouseMoved( wxMouseEvent& aEvent )
{
#if defined( __WXOSX__ ) // Doesn't work properly on other ports
    wxPoint pos = aEvent.GetPosition();
    wxRect  ctrlRect = m_filter->GetScreenRect();
    int     buttonWidth = ctrlRect.GetHeight(); // Presume buttons are square

    // TODO: restore cursor when mouse leaves the filter field (or is it a MSW bug?)
    if( m_filter->IsSearchButtonVisible() && pos.x < buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else if( m_filter->IsCancelButtonVisible() && pos.x > ctrlRect.GetWidth() - buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else
        SetCursor( wxCURSOR_IBEAM );
#endif
}


void DIALOG_FIELDS_TABLE::OnSizeViewControlsGrid( wxSizeEvent& event )
{
    const wxString& showColLabel = m_viewControlsGrid->GetColLabelValue( SHOW_FIELD_COLUMN );
    const wxString& groupByColLabel = m_viewControlsGrid->GetColLabelValue( GROUP_BY_COLUMN );
    int             showColWidth = KIUI::GetTextSize( showColLabel, m_viewControlsGrid ).x + COLUMN_MARGIN;
    int             groupByColWidth = KIUI::GetTextSize( groupByColLabel, m_viewControlsGrid ).x + COLUMN_MARGIN;
    int             remainingWidth = m_viewControlsGrid->GetSize().GetX() - showColWidth - groupByColWidth;

    m_viewControlsGrid->SetColSize( showColWidth, SHOW_FIELD_COLUMN );
    m_viewControlsGrid->SetColSize( groupByColWidth, GROUP_BY_COLUMN );

    if( m_viewControlsGrid->IsColShown( DISPLAY_NAME_COLUMN ) && m_viewControlsGrid->IsColShown( LABEL_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( DISPLAY_NAME_COLUMN, std::max( remainingWidth / 2, 60 ) );
        m_viewControlsGrid->SetColSize( LABEL_COLUMN, std::max( remainingWidth - ( remainingWidth / 2 ), 60 ) );
    }
    else if( m_viewControlsGrid->IsColShown( DISPLAY_NAME_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( DISPLAY_NAME_COLUMN, std::max( remainingWidth, 60 ) );
    }
    else if( m_viewControlsGrid->IsColShown( LABEL_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( LABEL_COLUMN, std::max( remainingWidth, 60 ) );
    }

    event.Skip();
}
