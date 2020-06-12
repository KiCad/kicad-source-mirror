/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_set_grid_base.h>
#include <base_units.h>
#include <common.h>
#include <settings/app_settings.h>
#include <pcbnew_settings.h>
#include <widgets/unit_binder.h>
#include <pcb_base_edit_frame.h>
#include <tools/pcb_actions.h>
#include <tool/tool_manager.h>
#include <id.h>
#include <tool/common_tools.h>
#include <math/util.h>      // for KiROUND
#include <tool/grid_menu.h>

// Max values for grid size
static const int MAX_GRID_SIZE = KiROUND( 1000.0 * IU_PER_MM );
static const int MIN_GRID_SIZE = KiROUND( 0.001 * IU_PER_MM );


class DIALOG_SET_GRID : public DIALOG_SET_GRID_BASE
{
    PCB_BASE_FRAME* m_parent;

public:
    /// This has no dependencies on calling wxFrame derivative, such as PCB_BASE_FRAME.
    DIALOG_SET_GRID( PCB_BASE_FRAME* aParent );

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

private:
    void OnResetGridOrgClick( wxCommandEvent& event ) override;

    UNIT_BINDER m_gridOriginX;
    UNIT_BINDER m_gridOriginY;
    UNIT_BINDER m_userGridX;
    UNIT_BINDER m_userGridY;
};


DIALOG_SET_GRID::DIALOG_SET_GRID( PCB_BASE_FRAME* aParent ):
    DIALOG_SET_GRID_BASE( aParent ),
    m_parent( aParent ),
    m_gridOriginX( aParent, m_staticTextGridPosX, m_GridOriginXCtrl, m_TextPosXUnits ),
    m_gridOriginY( aParent, m_staticTextGridPosY, m_GridOriginYCtrl, m_TextPosYUnits ),
    m_userGridX( aParent, m_staticTextSizeX, m_OptGridSizeX, m_TextSizeXUnits, true ),
    m_userGridY( aParent, m_staticTextSizeY, m_OptGridSizeY, m_TextSizeYUnits, true )
{
    wxArrayString grids;
    GRID_MENU::BuildChoiceList( &grids, m_parent->config(), GetUserUnits() != EDA_UNITS::INCHES );
    m_grid1Ctrl->Append( grids );
    m_grid2Ctrl->Append( grids );

    m_sdbSizerOK->SetDefault();         // set OK button as default response to 'Enter' key
    SetInitialFocus( m_GridOriginXCtrl );

    Layout();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


bool DIALOG_SET_GRID::TransferDataFromWindow()
{
    // Validate new settings
    if( !m_userGridX.Validate( MIN_GRID_SIZE, MAX_GRID_SIZE ) )
        return false;

    if( !m_userGridY.Validate( MIN_GRID_SIZE, MAX_GRID_SIZE ) )
        return false;

    // Apply the new settings
    GRID_SETTINGS& gridCfg = m_parent->config()->m_Window.grid;

    // Because grid origin is saved in board, show as modified
    m_parent->OnModify();
    m_parent->SetGridOrigin( wxPoint( m_gridOriginX.GetValue(), m_gridOriginY.GetValue() ) );
    gridCfg.user_grid_x = StringFromValue( GetUserUnits(), m_userGridX.GetValue(), true, true );
    gridCfg.user_grid_y = StringFromValue( GetUserUnits(), m_userGridY.GetValue(), true, true );
    m_parent->Settings().m_FastGrid1 = m_grid1Ctrl->GetSelection();
    m_parent->Settings().m_FastGrid2 = m_grid2Ctrl->GetSelection();

    // Notify GAL
    TOOL_MANAGER* mgr = m_parent->GetToolManager();
    mgr->RunAction( ACTIONS::gridPreset, true, gridCfg.last_size_idx );
    mgr->RunAction( ACTIONS::gridSetOrigin, true, new VECTOR2D( m_parent->GetGridOrigin() ) );

    m_parent->UpdateGridSelectBox();

    return wxDialog::TransferDataFromWindow();
}


bool DIALOG_SET_GRID::TransferDataToWindow()
{
    GRID_SETTINGS& settings = m_parent->config()->m_Window.grid;

    m_userGridX.SetValue( ValueFromString( GetUserUnits(), settings.user_grid_x, true ) );
    m_userGridY.SetValue( ValueFromString( GetUserUnits(), settings.user_grid_y, true ) );

    m_gridOriginX.SetValue( m_parent->GetGridOrigin().x );
    m_gridOriginY.SetValue( m_parent->GetGridOrigin().y );

    m_grid1Ctrl->SetSelection( m_parent->Settings().m_FastGrid1 );
    m_grid2Ctrl->SetSelection( m_parent->Settings().m_FastGrid2 );

    int hk1 = ACTIONS::gridFast1.GetHotKey();
    int hk2 = ACTIONS::gridFast2.GetHotKey();
    m_grid1HotKey->SetLabel( wxString::Format( wxT( "(%s)" ), KeyNameFromKeyCode( hk1 ) ) );
    m_grid2HotKey->SetLabel( wxString::Format( wxT( "(%s)" ), KeyNameFromKeyCode( hk2 ) ) );

    return wxDialog::TransferDataToWindow();
}


void DIALOG_SET_GRID::OnResetGridOrgClick( wxCommandEvent& event )
{
    m_gridOriginX.SetValue( 0 );
    m_gridOriginY.SetValue( 0 );
}


void PCB_BASE_EDIT_FRAME::OnGridSettings( wxCommandEvent& event )
{
    DIALOG_SET_GRID dlg( this );

    dlg.ShowModal();

    UpdateStatusBar();
    GetCanvas()->Refresh();
}
