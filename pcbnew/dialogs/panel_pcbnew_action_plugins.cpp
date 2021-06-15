/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <action_plugin.h>
#include <bitmaps.h>
#include <dialog_footprint_wizard_list.h>
#include <grid_tricks.h>
#include <kiface_i.h>
#include <panel_pcbnew_action_plugins.h>
#include <pcb_edit_frame.h>
#include <python/scripting/pcbnew_scripting.h>
#include <pcbnew_settings.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/paged_dialog.h>
#include <widgets/wx_grid.h>

#include <wx/treebook.h>


#define GRID_CELL_MARGIN 4

PANEL_PCBNEW_ACTION_PLUGINS::PANEL_PCBNEW_ACTION_PLUGINS( PCB_EDIT_FRAME* aFrame,
                                                          PAGED_DIALOG* aWindow ) :
        PANEL_PCBNEW_ACTION_PLUGINS_BASE( aWindow->GetTreebook() ),
        m_frame( aFrame )
{
    m_genericIcon = KiBitmap( BITMAPS::puzzle_piece );
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    m_moveUpButton->SetBitmap( KiBitmap( BITMAPS::small_up ) );
    m_moveDownButton->SetBitmap( KiBitmap( BITMAPS::small_down ) );
    m_openDirectoryButton->SetBitmap( KiBitmap( BITMAPS::small_folder ) );
    m_reloadButton->SetBitmap( KiBitmap( BITMAPS::small_refresh ) );
    m_showErrorsButton->SetBitmap( KiBitmap( BITMAPS::small_warning ) );
}


PANEL_PCBNEW_ACTION_PLUGINS::~PANEL_PCBNEW_ACTION_PLUGINS()
{
    m_grid->PopEventHandler( true );
}


void PANEL_PCBNEW_ACTION_PLUGINS::OnGridCellClick( wxGridEvent& event )
{
    SelectRow( event.GetRow() );
}


void PANEL_PCBNEW_ACTION_PLUGINS::SelectRow( int aRow )
{
    m_grid->ClearSelection();
    m_grid->SelectRow( aRow );
}


void PANEL_PCBNEW_ACTION_PLUGINS::OnMoveUpButtonClick( wxCommandEvent& event )
{
    auto selectedRows = m_grid->GetSelectedRows();

    // If nothing is selected or multiple rows are selected don't do anything.
    if( selectedRows.size() != 1 ) return;

    int selectedRow = selectedRows[0];

    // If first row is selected, then it can't go any further up.
    if( selectedRow == 0 )
    {
        wxBell();
        return;
    }

    SwapRows( selectedRow, selectedRow - 1 );

    SelectRow( selectedRow - 1 );
}


void PANEL_PCBNEW_ACTION_PLUGINS::OnMoveDownButtonClick( wxCommandEvent& event )
{
    auto selectedRows = m_grid->GetSelectedRows();

    // If nothing is selected or multiple rows are selected don't do anything.
    if( selectedRows.size() != 1 ) return;

    int selectedRow = selectedRows[0];

    // If last row is selected, then it can't go any further down.
    if( selectedRow + 1 == m_grid->GetNumberRows() )
    {
        wxBell();
        return;
    }

    SwapRows( selectedRow, selectedRow + 1 );

    SelectRow( selectedRow + 1 );
}


void PANEL_PCBNEW_ACTION_PLUGINS::SwapRows( int aRowA, int aRowB )
{
    m_grid->Freeze();

    // Swap all columns except icon
    wxString tempStr;

    for( int column = 1; column < m_grid->GetNumberCols(); column++ )
    {
        tempStr = m_grid->GetCellValue( aRowA, column );
        m_grid->SetCellValue( aRowA, column, m_grid->GetCellValue( aRowB, column ) );
        m_grid->SetCellValue( aRowB, column, tempStr );
    }

    // Swap icon column renderers
    auto cellRenderer = m_grid->GetCellRenderer( aRowA, COLUMN_ICON );
    m_grid->SetCellRenderer( aRowA, COLUMN_ICON, m_grid->GetCellRenderer( aRowB, COLUMN_ICON ) );
    m_grid->SetCellRenderer( aRowB, COLUMN_ICON, cellRenderer );

    m_grid->Thaw();
}


void PANEL_PCBNEW_ACTION_PLUGINS::OnReloadButtonClick( wxCommandEvent& event )
{
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::pluginsReload, true );
    TransferDataToWindow();
}


bool PANEL_PCBNEW_ACTION_PLUGINS::TransferDataFromWindow()
{
    PCBNEW_SETTINGS* settings = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( settings );

    if( settings )
    {
        settings->m_VisibleActionPlugins.clear();

        for( int ii = 0; ii < m_grid->GetNumberRows(); ii++ )
        {
            settings->m_VisibleActionPlugins.emplace_back( std::make_pair(
                    m_grid->GetCellValue( ii, COLUMN_PATH ),
                    m_grid->GetCellValue( ii, COLUMN_VISIBLE ) == wxT( "1" ) ) );
        }
    }

    return true;
}


bool PANEL_PCBNEW_ACTION_PLUGINS::TransferDataToWindow()
{
    m_grid->Freeze();

    if( m_grid->GetNumberRows() != 0 )
        m_grid->DeleteRows( 0, m_grid->GetNumberRows() );

    const auto& orderedPlugins = m_frame->GetOrderedActionPlugins();
    m_grid->AppendRows( orderedPlugins.size() );

    for( size_t row = 0; row < orderedPlugins.size(); row++ )
    {
        ACTION_PLUGIN* ap = orderedPlugins[row];

        // Icon
        m_grid->SetCellRenderer( row, COLUMN_ICON, new GRID_CELL_ICON_RENDERER(
                                 ap->iconBitmap.IsOk() ? ap->iconBitmap : m_genericIcon ) );

        // Toolbar button checkbox
        m_grid->SetCellRenderer( row, COLUMN_VISIBLE, new wxGridCellBoolRenderer() );
        m_grid->SetCellAlignment( row, COLUMN_VISIBLE, wxALIGN_CENTER, wxALIGN_CENTER );

        bool showButton = m_frame->GetActionPluginButtonVisible( ap->GetPluginPath(),
                                                                 ap->GetShowToolbarButton() );

        m_grid->SetCellValue( row, COLUMN_VISIBLE, showButton ? wxT( "1" ) : wxEmptyString );

        m_grid->SetCellValue( row, COLUMN_NAME, ap->GetName() );
        m_grid->SetCellValue( row, COLUMN_CATEGORY, ap->GetCategoryName() );
        m_grid->SetCellValue( row, COLUMN_DESCRIPTION, ap->GetDescription() );
        m_grid->SetCellValue( row, COLUMN_PATH, ap->GetPluginPath() );
    }

    for( int col = 0; col < m_grid->GetNumberCols(); col++ )
    {
        const wxString& heading = m_grid->GetColLabelValue( col );
        int             headingWidth = GetTextExtent( heading ).x + 2 * GRID_CELL_MARGIN;

        // Set the minimal width to the column label size.
        m_grid->SetColMinimalWidth( col, headingWidth );
        // Set the width to see the full contents
        m_grid->SetColSize( col, m_grid->GetVisibleWidth( col, true, true, false ) );
    }

    m_grid->AutoSizeRows();

    m_grid->Thaw();

    // Show errors button should be disabled if there are no errors.
    wxString trace;
    pcbnewGetWizardsBackTrace( trace );
    if( trace.empty() )
    {
        m_showErrorsButton->Disable();
        m_showErrorsButton->Hide();
    }
    else
    {
        m_showErrorsButton->Enable();
        m_showErrorsButton->Show();
    }

    return true;
}

void PANEL_PCBNEW_ACTION_PLUGINS::OnOpenDirectoryButtonClick( wxCommandEvent& event )
{
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::pluginsShowFolder, true );
}

void PANEL_PCBNEW_ACTION_PLUGINS::OnShowErrorsButtonClick( wxCommandEvent& event )
{
    wxString trace;
    pcbnewGetWizardsBackTrace( trace );

    // Now display the filtered trace in our dialog
    // (a simple wxMessageBox is really not suitable for long messages)
    DIALOG_FOOTPRINT_WIZARD_LOG logWindow( this );
    logWindow.m_Message->SetValue( trace );
    logWindow.ShowModal();
}
