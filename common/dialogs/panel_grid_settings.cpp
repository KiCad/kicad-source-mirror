/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <confirm.h>
#include <wx/textdlg.h>
#include <dialogs/panel_grid_settings.h>
#include <dialogs/dialog_grid_settings.h>
#include <widgets/std_bitmap_button.h>
#include <common.h>
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
        m_unitsProvider( aUnitsProvider ), m_cfg( aCfg ), m_frameType( aFrameType ),
        m_eventSource( aEventSource ),
        m_gridOverrideConnected( aUnitsProvider, aEventSource, m_staticTextConnected,
                                 m_GridOverrideConnectedSize, m_staticTextConnectedUnits ),
        m_gridOverrideWires( aUnitsProvider, aEventSource, m_staticTextWires,
                             m_GridOverrideWiresSize, m_staticTextWiresUnits ),
        m_gridOverrideVias( aUnitsProvider, aEventSource, m_staticTextVias, m_GridOverrideViasSize,
                            m_staticTextViasUnits ),
        m_gridOverrideText( aUnitsProvider, aEventSource, m_staticTextText, m_GridOverrideTextSize,
                            m_staticTextTextUnits ),
        m_gridOverrideGraphics( aUnitsProvider, aEventSource, m_staticTextGraphics,
                                m_GridOverrideGraphicsSize, m_staticTextGraphicsUnits )
{
    RebuildGridSizes();

    if( m_frameType == FRAME_PCB_EDITOR || m_frameType == FRAME_FOOTPRINT_EDITOR )
    {
        m_staticTextConnected->SetLabel( wxT( "Footprints/pads:" ) );
        m_staticTextWires->SetLabel( wxT( "Tracks:" ) );
    }
    else
    {
        m_GridOverrideViasSize->SetValue( wxT( "50 mil" ) );
        m_checkGridOverrideVias->Show( false );
        m_gridOverrideVias.Show( false );

        if( m_frameType != FRAME_SCH
            && m_frameType != FRAME_SCH_SYMBOL_EDITOR
            && m_frameType != FRAME_SCH_VIEWER
            && m_frameType != FRAME_SCH_VIEWER_MODAL
            && m_frameType != FRAME_SIMULATOR )
        {
            m_checkGridOverrideConnected->Show( false );
            m_gridOverrideConnected.Show( false );

            m_checkGridOverrideWires->Show( false );
            m_gridOverrideWires.Show( false );
        }
    }

    int hk1 = ACTIONS::gridFast1.GetHotKey();
    int hk2 = ACTIONS::gridFast2.GetHotKey();
    m_grid1HotKey->SetLabel( wxString::Format( wxT( "(%s)" ), KeyNameFromKeyCode( hk1 ) ) );
    m_grid2HotKey->SetLabel( wxString::Format( wxT( "(%s)" ), KeyNameFromKeyCode( hk2 ) ) );

    m_addGridButton->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_removeGridButton->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_moveUpButton->SetBitmap( KiBitmap( BITMAPS::small_up ) );
    m_moveDownButton->SetBitmap( KiBitmap( BITMAPS::small_down ) );

    Layout();
}


void PANEL_GRID_SETTINGS::ResetPanel()
{
    m_cfg->m_Window.grid.grids = m_cfg->DefaultGridSizeList();
    RebuildGridSizes();
    m_cfg->m_Window.grid.last_size_idx = m_currentGridCtrl->GetSelection();
}


void PANEL_GRID_SETTINGS::RebuildGridSizes()
{
    wxString savedCurrentGrid = m_currentGridCtrl->GetStringSelection();
    wxString savedGrid1       = m_grid1Ctrl->GetStringSelection();
    wxString savedGrid2       = m_grid2Ctrl->GetStringSelection();

    wxArrayString grids;
    wxString      msg;
    EDA_IU_SCALE  scale = m_unitsProvider->GetIuScale();
    EDA_UNITS     primaryUnit;
    EDA_UNITS     secondaryUnit;

    m_unitsProvider->GetUnitPair( primaryUnit, secondaryUnit );

    for( const struct GRID& grid : m_cfg->m_Window.grid.grids )
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

    if( !m_currentGridCtrl->SetStringSelection( savedCurrentGrid ) )
        m_currentGridCtrl->SetStringSelection( grids.front() );

    if( !m_grid1Ctrl->SetStringSelection( savedGrid1 ) )
        m_grid1Ctrl->SetStringSelection( grids.front() );

    if( !m_grid2Ctrl->SetStringSelection( savedGrid2 ) )
        m_grid2Ctrl->SetStringSelection( grids.back() );
}


bool PANEL_GRID_SETTINGS::TransferDataFromWindow()
{
    // Validate new settings
    for( UNIT_BINDER* entry : { &m_gridOverrideConnected, &m_gridOverrideWires,
                                &m_gridOverrideVias, &m_gridOverrideText, &m_gridOverrideGraphics } )
    {
        if( !entry->Validate( 0.001, 1000.0, EDA_UNITS::MILLIMETRES ) )
            return false;
    }

    // Apply the new settings
    GRID_SETTINGS& gridCfg = m_cfg->m_Window.grid;

    gridCfg.last_size_idx = m_currentGridCtrl->GetSelection();
    gridCfg.fast_grid_1 = m_grid1Ctrl->GetSelection();
    gridCfg.fast_grid_2 = m_grid2Ctrl->GetSelection();

    gridCfg.override_connected      = m_checkGridOverrideConnected->GetValue();
    gridCfg.override_connected_size = m_unitsProvider->StringFromValue( m_gridOverrideConnected.GetValue(), true );
    gridCfg.override_wires          = m_checkGridOverrideWires->GetValue();
    gridCfg.override_wires_size     = m_unitsProvider->StringFromValue( m_gridOverrideWires.GetValue(), true );
    gridCfg.override_vias           = m_checkGridOverrideVias->GetValue();
    gridCfg.override_vias_size      = m_unitsProvider->StringFromValue( m_gridOverrideVias.GetValue(), true );
    gridCfg.override_text           = m_checkGridOverrideText->GetValue();
    gridCfg.override_text_size      = m_unitsProvider->StringFromValue( m_gridOverrideText.GetValue(), true );
    gridCfg.override_graphics       = m_checkGridOverrideGraphics->GetValue();
    gridCfg.override_graphics_size  = m_unitsProvider->StringFromValue( m_gridOverrideGraphics.GetValue(), true );

    return RESETTABLE_PANEL::TransferDataFromWindow();
}


bool PANEL_GRID_SETTINGS::TransferDataToWindow()
{
    GRID_SETTINGS& gridCfg = m_cfg->m_Window.grid;

    Layout();

    m_currentGridCtrl->SetSelection( gridCfg.last_size_idx );

    m_gridOverrideConnected.SetValue( m_unitsProvider->ValueFromString( gridCfg.override_connected_size ) );
    m_gridOverrideWires.SetValue( m_unitsProvider->ValueFromString( gridCfg.override_wires_size ) );
    m_gridOverrideVias.SetValue( m_unitsProvider->ValueFromString( gridCfg.override_vias_size ) );
    m_gridOverrideText.SetValue( m_unitsProvider->ValueFromString( gridCfg.override_text_size ) );
    m_gridOverrideGraphics.SetValue( m_unitsProvider->ValueFromString( gridCfg.override_graphics_size ) );

    m_checkGridOverrideConnected->SetValue( gridCfg.override_connected );
    m_checkGridOverrideWires->SetValue( gridCfg.override_wires );
    m_checkGridOverrideVias->SetValue( gridCfg.override_vias );
    m_checkGridOverrideText->SetValue( gridCfg.override_text );
    m_checkGridOverrideGraphics->SetValue( gridCfg.override_graphics );

    m_grid1Ctrl->SetSelection( gridCfg.fast_grid_1 );
    m_grid2Ctrl->SetSelection( gridCfg.fast_grid_2 );

    return RESETTABLE_PANEL::TransferDataToWindow();
}


void PANEL_GRID_SETTINGS::OnAddGrid( wxCommandEvent& event )
{
    GRID                 newGrid = GRID{ wxEmptyString, "", "" };
    DIALOG_GRID_SETTINGS dlg( this->GetParent(), m_eventSource, m_unitsProvider, newGrid );

    if( dlg.ShowModal() != wxID_OK )
        return;

    int            row = m_currentGridCtrl->GetSelection();
    GRID_SETTINGS& gridCfg = m_cfg->m_Window.grid;

    for( GRID& g : gridCfg.grids )
    {
        if( newGrid == g )
        {
            DisplayError( this, wxString::Format( _( "Grid size '%s' already exists." ),
                                                  g.UserUnitsMessageText( m_unitsProvider ) ) );
            return;
        }
    }

    gridCfg.grids.insert( gridCfg.grids.begin() + row, newGrid );
    RebuildGridSizes();
    m_currentGridCtrl->SetSelection( row );
}


void PANEL_GRID_SETTINGS::OnRemoveGrid( wxCommandEvent& event )
{
    GRID_SETTINGS& gridCfg = m_cfg->m_Window.grid;
    int            row = m_currentGridCtrl->GetSelection();

    if( gridCfg.grids.size() <= 1 )
    {
        DisplayError( this, wxString::Format( _( "At least one grid size is required." ) ) );
        return;
    }

    gridCfg.grids.erase( gridCfg.grids.begin() + row );
    RebuildGridSizes();

    if( row != 0 )
        m_currentGridCtrl->SetSelection( row - 1 );
}


void PANEL_GRID_SETTINGS::OnMoveGridUp( wxCommandEvent& event )
{
    GRID_SETTINGS& gridCfg = m_cfg->m_Window.grid;
    int            row = m_currentGridCtrl->GetSelection();

    if( gridCfg.grids.size() <= 1 || row == 0 )
        return;

    std::swap( gridCfg.grids[row], gridCfg.grids[row - 1] );
    RebuildGridSizes();

    if( row != 0 )
        m_currentGridCtrl->SetSelection( row - 1 );
}


void PANEL_GRID_SETTINGS::OnMoveGridDown( wxCommandEvent& event )
{
    GRID_SETTINGS& gridCfg = m_cfg->m_Window.grid;
    int            row = m_currentGridCtrl->GetSelection();

    if( gridCfg.grids.size() <= 1 || row == ( (int) gridCfg.grids.size() - 1 ) )
        return;

    std::swap( gridCfg.grids[row], gridCfg.grids[row + 1] );
    RebuildGridSizes();

    if( row != 0 )
        m_currentGridCtrl->SetSelection( row + 1 );
}
