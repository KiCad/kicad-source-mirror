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

#include <bitmaps.h>
#include <wx/textdlg.h>
#include <dialogs/panel_grid_settings.h>
#include <dialogs/dialog_grid_settings.h>
#include <widgets/std_bitmap_button.h>
#include <common.h>
#include <confirm.h>
#include <settings/app_settings.h>
#include <eda_draw_frame.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <tool/grid_menu.h>
#include <tool/common_tools.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

PANEL_GRID_SETTINGS::PANEL_GRID_SETTINGS( wxWindow* aParent, UNITS_PROVIDER* aUnitsProvider,
                                          wxWindow* aEventSource, APP_SETTINGS_BASE* aCfg,
                                          FRAME_T aFrameType ) :
        PANEL_GRID_SETTINGS_BASE( aParent ),
        m_unitsProvider( aUnitsProvider ),
        m_cfg( aCfg ),
        m_frameType( aFrameType ),
        m_eventSource( aEventSource )
{
    m_currentGridCtrl->SetMinSize( FromDIP( m_currentGridCtrl->GetMinSize() ) );

    if( m_frameType == FRAME_FOOTPRINT_EDITOR )
    {
        m_checkGridOverrideConnected->SetLabel( _( "Pads:" ) );
        m_checkGridOverrideWires->Show( false );
        m_gridOverrideWiresChoice->Show( false );
        m_checkGridOverrideVias->Show( false );
        m_gridOverrideViasChoice->Show( false );
    }
    else if( m_frameType == FRAME_PCB_EDITOR )
    {
        m_checkGridOverrideConnected->SetLabel( _( "Footprints/pads:" ) );
        m_checkGridOverrideWires->SetLabel( _( "Tracks:" ) );
    }
    else
    {
        m_gridOverrideViasChoice->SetSelection( 0 );
        m_gridOverrideViasChoice->Show( false );
        m_checkGridOverrideVias->Show( false );

        if( m_frameType != FRAME_SCH
            && m_frameType != FRAME_SCH_SYMBOL_EDITOR
            && m_frameType != FRAME_SCH_VIEWER
            && m_frameType != FRAME_SIMULATOR )
        {
            m_checkGridOverrideConnected->Show( false );
            m_gridOverrideConnectedChoice->Show( false );

            m_checkGridOverrideWires->Show( false );
            m_gridOverrideWiresChoice->Show( false );
        }

        if( m_frameType == FRAME_GERBER )
        {
            m_overridesLabel->Show( false );
            m_staticline3->Show( false );

            m_checkGridOverrideText->Show( false );
            m_gridOverrideTextChoice->Show( false );

            m_checkGridOverrideGraphics->Show( false );
            m_gridOverrideGraphicsChoice->Show( false );
        }
    }

    int hk1 = ACTIONS::gridFast1.GetHotKey();
    int hk2 = ACTIONS::gridFast2.GetHotKey();
    m_grid1HotKey->SetLabel( wxString::Format( wxT( "(%s)" ), KeyNameFromKeyCode( hk1 ) ) );
    m_grid2HotKey->SetLabel( wxString::Format( wxT( "(%s)" ), KeyNameFromKeyCode( hk2 ) ) );

    m_addGridButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_editGridButton->SetBitmap( KiBitmapBundle( BITMAPS::edit ) );
    m_removeGridButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_moveUpButton->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_moveDownButton->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    Layout();
}


void PANEL_GRID_SETTINGS::ResetPanel()
{
    m_grids = m_cfg->DefaultGridSizeList();
    RebuildGridSizes();
}


void PANEL_GRID_SETTINGS::RebuildGridSizes()
{
    wxString savedCurrentGrid = m_currentGridCtrl->GetStringSelection();

    wxString savedGrid1       = m_grid1Ctrl->GetStringSelection();
    wxString savedGrid2       = m_grid2Ctrl->GetStringSelection();

    wxString savedConnectables = m_gridOverrideConnectedChoice->GetStringSelection();
    wxString savedWires        = m_gridOverrideWiresChoice->GetStringSelection();
    wxString savedVias         = m_gridOverrideViasChoice->GetStringSelection();
    wxString savedText         = m_gridOverrideTextChoice->GetStringSelection();
    wxString savedGraphics     = m_gridOverrideGraphicsChoice->GetStringSelection();

    wxArrayString grids;
    wxString      msg;
    EDA_IU_SCALE  scale = m_unitsProvider->GetIuScale();
    EDA_UNITS     primaryUnit;
    EDA_UNITS     secondaryUnit;

    m_unitsProvider->GetUnitPair( primaryUnit, secondaryUnit );

    for( const struct GRID& grid : m_grids )
    {
        wxString name = grid.name;

        if( !name.IsEmpty() )
            name += wxT( ": " );

        msg.Printf( _( "%s%s (%s)" ), name, grid.MessageText( scale, primaryUnit, true ),
                    grid.MessageText( scale, secondaryUnit, true ) );

        grids.Add( msg );
    }

    m_currentGridCtrl->Set( grids );

    m_grid1Ctrl->Set( grids );
    m_grid2Ctrl->Set( grids );

    m_gridOverrideConnectedChoice->Set( grids );
    m_gridOverrideWiresChoice->Set( grids );
    m_gridOverrideViasChoice->Set( grids );
    m_gridOverrideTextChoice->Set( grids );
    m_gridOverrideGraphicsChoice->Set( grids );

    if( !m_currentGridCtrl->SetStringSelection( savedCurrentGrid ) )
        m_currentGridCtrl->SetStringSelection( grids.front() );

    if( !m_grid1Ctrl->SetStringSelection( savedGrid1 ) )
        m_grid1Ctrl->SetStringSelection( grids.front() );

    if( !m_grid2Ctrl->SetStringSelection( savedGrid2 ) )
        m_grid2Ctrl->SetStringSelection( grids.back() );

    if( !m_gridOverrideConnectedChoice->SetStringSelection( savedConnectables ) )
        m_gridOverrideConnectedChoice->SetStringSelection( grids.front() );

    if( !m_gridOverrideWiresChoice->SetStringSelection( savedWires ) )
        m_gridOverrideWiresChoice->SetStringSelection( grids.front() );

    if( !m_gridOverrideViasChoice->SetStringSelection( savedVias ) )
        m_gridOverrideViasChoice->SetStringSelection( grids.front() );

    if( !m_gridOverrideTextChoice->SetStringSelection( savedText ) )
        m_gridOverrideTextChoice->SetStringSelection( grids.front() );

    if( !m_gridOverrideGraphicsChoice->SetStringSelection( savedGraphics ) )
        m_gridOverrideGraphicsChoice->SetStringSelection( grids.front() );
}


bool PANEL_GRID_SETTINGS::TransferDataFromWindow()
{
    // Apply the new settings
    GRID_SETTINGS& gridCfg = m_cfg->m_Window.grid;

    gridCfg.grids = m_grids;

    gridCfg.last_size_idx = m_currentGridCtrl->GetSelection();

    gridCfg.fast_grid_1 = m_grid1Ctrl->GetSelection();
    gridCfg.fast_grid_2 = m_grid2Ctrl->GetSelection();

    gridCfg.override_connected      = m_checkGridOverrideConnected->GetValue();
    gridCfg.override_connected_idx  = m_gridOverrideConnectedChoice->GetSelection();
    gridCfg.override_wires          = m_checkGridOverrideWires->GetValue();
    gridCfg.override_wires_idx      = m_gridOverrideWiresChoice->GetSelection();
    gridCfg.override_vias           = m_checkGridOverrideVias->GetValue();
    gridCfg.override_vias_idx       = m_gridOverrideViasChoice->GetSelection();
    gridCfg.override_text           = m_checkGridOverrideText->GetValue();
    gridCfg.override_text_idx       = m_gridOverrideTextChoice->GetSelection();
    gridCfg.override_graphics       = m_checkGridOverrideGraphics->GetValue();
    gridCfg.override_graphics_idx   = m_gridOverrideGraphicsChoice->GetSelection();

    return RESETTABLE_PANEL::TransferDataFromWindow();
}


bool PANEL_GRID_SETTINGS::TransferDataToWindow()
{
    GRID_SETTINGS& gridCfg = m_cfg->m_Window.grid;

    // lambda that gives us a safe index into grids regardless of config idx
    auto safeGrid =
            [this]( int idx ) -> int
            {
                if( idx < 0 || idx >= (int) m_grids.size() )
                    return 0;

                return idx;
            };

    Layout();

    m_grids = gridCfg.grids;
    RebuildGridSizes();

    m_currentGridCtrl->SetSelection( safeGrid( gridCfg.last_size_idx ) );

    m_grid1Ctrl->SetSelection( safeGrid( gridCfg.fast_grid_1 ) );
    m_grid2Ctrl->SetSelection( safeGrid( gridCfg.fast_grid_2 ) );

    m_gridOverrideConnectedChoice->SetSelection( safeGrid( gridCfg.override_connected_idx ) );
    m_gridOverrideWiresChoice->SetSelection( safeGrid( gridCfg.override_wires_idx ) );
    m_gridOverrideViasChoice->SetSelection( safeGrid( gridCfg.override_vias_idx ) );
    m_gridOverrideTextChoice->SetSelection( safeGrid( gridCfg.override_text_idx ) );
    m_gridOverrideGraphicsChoice->SetSelection( safeGrid( gridCfg.override_graphics_idx ) );

    m_checkGridOverrideConnected->SetValue( gridCfg.override_connected );
    m_checkGridOverrideWires->SetValue( gridCfg.override_wires );
    m_checkGridOverrideVias->SetValue( gridCfg.override_vias );
    m_checkGridOverrideText->SetValue( gridCfg.override_text );
    m_checkGridOverrideGraphics->SetValue( gridCfg.override_graphics );

    return RESETTABLE_PANEL::TransferDataToWindow();
}


void PANEL_GRID_SETTINGS::OnAddGrid( wxCommandEvent& event )
{
    GRID                 newGrid = GRID{ wxEmptyString, "", "" };
    DIALOG_GRID_SETTINGS dlg( wxGetTopLevelParent( this ), m_eventSource, m_unitsProvider,
                              newGrid );

    if( dlg.ShowModal() != wxID_OK )
        return;

    int row = m_currentGridCtrl->GetSelection();

    for( GRID& g : m_grids )
    {
        if( newGrid == g )
        {
            wxWindow* topLevelParent = wxGetTopLevelParent( this );

            DisplayError( topLevelParent,
                          wxString::Format( _( "Grid size '%s' already exists." ),
                                            g.UserUnitsMessageText( m_unitsProvider ) ) );
            return;
        }
    }

    m_grids.insert( m_grids.begin() + row, newGrid );
    RebuildGridSizes();
    m_currentGridCtrl->SetSelection( row );
}


void PANEL_GRID_SETTINGS::OnEditGrid( wxCommandEvent& event )
{
    onEditGrid();
}


void PANEL_GRID_SETTINGS::onEditGrid()
{
    int row = m_currentGridCtrl->GetSelection();

    if( row < 0 )
        return;

    GRID                 editGrid = m_grids[row];
    DIALOG_GRID_SETTINGS dlg( wxGetTopLevelParent( this ), m_eventSource, m_unitsProvider,
                              editGrid );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // If the user just clicked OK without changing anything,
    // then return or we'll trigger the same grid check
    if( editGrid == m_grids[row] )
        return;

    for( GRID& g : m_grids )
    {
        if( editGrid == g )
        {
            wxWindow* topLevelParent = wxGetTopLevelParent( this );

            DisplayError( topLevelParent,
                          wxString::Format( _( "Grid size '%s' already exists." ),
                                            g.UserUnitsMessageText( m_unitsProvider ) ) );
            return;
        }
    }

    m_grids[row] = editGrid;

    RebuildGridSizes();
    m_currentGridCtrl->SetSelection( row );
}


void PANEL_GRID_SETTINGS::OnRemoveGrid( wxCommandEvent& event )
{
    int row = m_currentGridCtrl->GetSelection();

    if( m_grids.size() <= 1 )
        return;

    m_grids.erase( m_grids.begin() + row );
    RebuildGridSizes();

    if( row != 0 )
        m_currentGridCtrl->SetSelection( row - 1 );
}


void PANEL_GRID_SETTINGS::OnMoveGridUp( wxCommandEvent& event )
{
    int row = m_currentGridCtrl->GetSelection();

    if( m_grids.size() <= 1 || row == 0 )
        return;

    std::swap( m_grids[row], m_grids[row - 1] );
    RebuildGridSizes();

    if( row != 0 )
        m_currentGridCtrl->SetSelection( row - 1 );
}


void PANEL_GRID_SETTINGS::OnMoveGridDown( wxCommandEvent& event )
{
    int row = m_currentGridCtrl->GetSelection();

    if( m_grids.size() <= 1 || row == ( (int) m_grids.size() - 1 ) )
        return;

    std::swap( m_grids[row], m_grids[row + 1] );
    RebuildGridSizes();

    if( row != 0 )
        m_currentGridCtrl->SetSelection( row + 1 );
}


void PANEL_GRID_SETTINGS::OnUpdateEditGrid( wxUpdateUIEvent& event )
{
    // Enable edit when there is a valid selection
    event.Enable( m_currentGridCtrl->GetSelection() >= 0 );
}


void PANEL_GRID_SETTINGS::OnUpdateMoveUp( wxUpdateUIEvent& event )
{
    int curRow  = m_currentGridCtrl->GetSelection();
    int numRows = (int) m_grids.size();

    // Enable move up when there are multiple grids and it is not the first row
    event.Enable( ( numRows > 1 ) && ( curRow > 0 ) );
}


void PANEL_GRID_SETTINGS::OnUpdateMoveDown( wxUpdateUIEvent& event )
{
    int curRow  = m_currentGridCtrl->GetSelection();
    int numRows = (int) m_grids.size();

    // Enable move down when there are multiple grids and it is not the last row
    event.Enable( ( numRows > 1 ) && ( curRow < ( numRows - 1 ) ) );
}


void PANEL_GRID_SETTINGS::OnUpdateRemove( wxUpdateUIEvent& event )
{
    // Enable remove if there is more than 1 grid
    event.Enable( m_grids.size() > 1 );
}
