/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_grid_settings.h>
#include <base_units.h>
#include <common.h>
#include <settings/app_settings.h>
#include <eda_draw_frame.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <tool/grid_menu.h>
#include <tool/common_tools.h>

DIALOG_GRID_SETTINGS::DIALOG_GRID_SETTINGS( EDA_DRAW_FRAME* aParent ):
    DIALOG_GRID_SETTINGS_BASE( aParent ),
    m_parent( aParent ),
    m_gridOriginX( aParent, m_staticTextGridPosX, m_GridOriginXCtrl, m_TextPosXUnits ),
    m_gridOriginY( aParent, m_staticTextGridPosY, m_GridOriginYCtrl, m_TextPosYUnits ),
    m_userGridX( aParent, m_staticTextSizeX, m_OptGridSizeX, m_TextSizeXUnits, true ),
    m_userGridY( aParent, m_staticTextSizeY, m_OptGridSizeY, m_TextSizeYUnits, true )
{
    // Configure display origin transforms
    m_gridOriginX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_gridOriginY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    wxArrayString grids;
    GRID_MENU::BuildChoiceList( &grids, m_parent->config(), m_parent );
    m_currentGridCtrl->Append( grids );
    m_grid1Ctrl->Append( grids );
    m_grid2Ctrl->Append( grids );

    if( m_parent->IsType( FRAME_SCH )
        || m_parent->IsType( FRAME_SCH_SYMBOL_EDITOR )
        || m_parent->IsType( FRAME_SCH_VIEWER )
        || m_parent->IsType( FRAME_SCH_VIEWER_MODAL )
        || m_parent->IsType( FRAME_SIMULATOR ) )
    {
        m_book->SetSelection( 1 );
        m_buttonReset->Hide();              // Eeschema and friends don't use grid origin
    }
    else
    {
        m_book->SetSelection( 0 );
    }

    m_sdbSizerOK->SetDefault();         // set OK button as default response to 'Enter' key
    SetInitialFocus( m_GridOriginXCtrl );

    Layout();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_GRID_SETTINGS::TransferDataFromWindow()
{
    // Validate new settings
    if( !m_userGridX.Validate( 0.001, 1000.0, EDA_UNITS::MILLIMETRES ) )
        return false;

    if( !m_userGridY.Validate( 0.001, 1000.0, EDA_UNITS::MILLIMETRES ) )
        return false;

    // Apply the new settings
    GRID_SETTINGS& gridCfg = m_parent->config()->m_Window.grid;

    gridCfg.last_size_idx = m_currentGridCtrl->GetSelection();
    m_parent->SetGridOrigin( wxPoint( m_gridOriginX.GetValue(), m_gridOriginY.GetValue() ) );
    gridCfg.user_grid_x = StringFromValue( GetUserUnits(), m_userGridX.GetValue(), true );
    gridCfg.user_grid_y = StringFromValue( GetUserUnits(), m_userGridY.GetValue(), true );
    gridCfg.fast_grid_1 = m_grid1Ctrl->GetSelection();
    gridCfg.fast_grid_2 = m_grid2Ctrl->GetSelection();

    // Notify TOOLS
    TOOL_MANAGER* mgr = m_parent->GetToolManager();
    mgr->ResetTools( TOOL_BASE::MODEL_RELOAD );

    // Notify GAL
    mgr->RunAction( ACTIONS::gridPreset, true, gridCfg.last_size_idx );
    mgr->RunAction( ACTIONS::gridSetOrigin, true, new VECTOR2D( m_parent->GetGridOrigin() ) );

    m_parent->UpdateGridSelectBox();

    return wxDialog::TransferDataFromWindow();
}


bool DIALOG_GRID_SETTINGS::TransferDataToWindow()
{
    GRID_SETTINGS& gridCfg = m_parent->config()->m_Window.grid;

    m_currentGridCtrl->SetSelection( m_parent->config()->m_Window.grid.last_size_idx );

    m_userGridX.SetValue( ValueFromString( GetUserUnits(), gridCfg.user_grid_x ) );
    m_userGridY.SetValue( ValueFromString( GetUserUnits(), gridCfg.user_grid_y ) );

    m_gridOriginX.SetValue( m_parent->GetGridOrigin().x );
    m_gridOriginY.SetValue( m_parent->GetGridOrigin().y );

    m_grid1Ctrl->SetSelection( gridCfg.fast_grid_1 );
    m_grid2Ctrl->SetSelection( gridCfg.fast_grid_2 );

    int hk1 = ACTIONS::gridFast1.GetHotKey();
    int hk2 = ACTIONS::gridFast2.GetHotKey();
    m_grid1HotKey->SetLabel( wxString::Format( wxT( "(%s)" ), KeyNameFromKeyCode( hk1 ) ) );
    m_grid2HotKey->SetLabel( wxString::Format( wxT( "(%s)" ), KeyNameFromKeyCode( hk2 ) ) );

    return wxDialog::TransferDataToWindow();
}


void DIALOG_GRID_SETTINGS::OnResetGridOriginClick( wxCommandEvent& event )
{
    m_gridOriginX.SetValue( 0 );
    m_gridOriginY.SetValue( 0 );
}


