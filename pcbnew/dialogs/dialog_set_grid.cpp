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
#include <widgets/unit_binder.h>
#include <pcb_base_edit_frame.h>
#include <tools/pcb_actions.h>
#include <tool/tool_manager.h>
#include <pcbnew_id.h>
#include <tool/common_tools.h>

// Max values for grid size
static const int MAX_GRID_SIZE = KiROUND( 1000.0 * IU_PER_MM );
static const int MIN_GRID_SIZE = KiROUND( 0.001 * IU_PER_MM );


class DIALOG_SET_GRID : public DIALOG_SET_GRID_BASE
{
    PCB_BASE_FRAME* m_parent;
    wxArrayString   m_fast_grid_opts;

public:
    /// This has no dependencies on calling wxFrame derivative, such as PCB_BASE_FRAME.
    DIALOG_SET_GRID( PCB_BASE_FRAME* aParent, const wxArrayString& aGridChoices );

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

private:
    void OnResetGridOrgClick( wxCommandEvent& event ) override;

    UNIT_BINDER m_gridOriginX;
    UNIT_BINDER m_gridOriginY;
    UNIT_BINDER m_userGridX;
    UNIT_BINDER m_userGridY;
};


DIALOG_SET_GRID::DIALOG_SET_GRID( PCB_BASE_FRAME* aParent, const wxArrayString& aGridChoices ):
    DIALOG_SET_GRID_BASE( aParent ),
    m_parent( aParent ),
    m_fast_grid_opts( aGridChoices ),
    m_gridOriginX( aParent, m_staticTextGridPosX, m_GridOriginXCtrl, m_TextPosXUnits ),
    m_gridOriginY( aParent, m_staticTextGridPosY, m_GridOriginYCtrl, m_TextPosYUnits ),
    m_userGridX( aParent, m_staticTextSizeX, m_OptGridSizeX, m_TextSizeXUnits ),
    m_userGridY( aParent, m_staticTextSizeY, m_OptGridSizeY, m_TextSizeYUnits )
{
    m_grid1Ctrl->Append( m_fast_grid_opts );
    m_grid2Ctrl->Append( m_fast_grid_opts );

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

    // Because grid origin is saved in board, show as modified
    m_parent->OnModify();
    m_parent->SetGridOrigin( wxPoint( m_gridOriginX.GetValue(), m_gridOriginY.GetValue() ) );
    m_parent->m_UserGridSize = wxPoint( m_userGridX.GetValue(), m_userGridY.GetValue() );
    m_parent->m_FastGrid1 = m_grid1Ctrl->GetSelection();
    m_parent->m_FastGrid2 = m_grid2Ctrl->GetSelection();

    // User grid
    BASE_SCREEN* screen = m_parent->GetScreen();
    screen->AddGrid( m_parent->m_UserGridSize, EDA_UNITS_T::UNSCALED_UNITS, ID_POPUP_GRID_USER );

    // If the user grid is the current option, recall SetGrid()
    // to force new values put in list as current grid value
    if( screen->GetGridCmdId() == ID_POPUP_GRID_USER )
        screen->SetGrid( ID_POPUP_GRID_USER );

    // Notify GAL
    TOOL_MANAGER* mgr = m_parent->GetToolManager();
    mgr->GetTool<COMMON_TOOLS>()->GridPreset( screen->GetGridCmdId() - ID_POPUP_GRID_LEVEL_1000 );
    mgr->RunAction( ACTIONS::gridSetOrigin, true, new VECTOR2D( m_parent->GetGridOrigin() ) );

    m_parent->UpdateGridSelectBox();

    return wxDialog::TransferDataFromWindow();
}


bool DIALOG_SET_GRID::TransferDataToWindow()
{
    m_userGridX.SetValue( m_parent->m_UserGridSize.x );
    m_userGridY.SetValue( m_parent->m_UserGridSize.y );

    m_gridOriginX.SetValue( m_parent->GetGridOrigin().x );
    m_gridOriginY.SetValue( m_parent->GetGridOrigin().y );

    m_grid1Ctrl->SetSelection( m_parent->m_FastGrid1 );
    m_grid2Ctrl->SetSelection( m_parent->m_FastGrid2 );

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
    DIALOG_SET_GRID dlg( this, m_gridSelectBox->GetStrings() );

    dlg.ShowModal();

    UpdateStatusBar();
    GetCanvas()->Refresh();
}
