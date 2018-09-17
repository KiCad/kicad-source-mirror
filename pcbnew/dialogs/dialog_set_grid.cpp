/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialog_set_grid.cpp
 * @brief Manage user grid.
 */

#include <dialog_set_grid_base.h>

#include <base_units.h>
#include <convert_to_biu.h>
#include <common.h>

#include <pcb_base_frame.h>
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>

#include <gal/graphics_abstraction_layer.h>
#include <tools/pcb_actions.h>
#include <tool/tool_manager.h>

#include <limits.h>

// Max values for grid size
static const double MAX_GRID_SIZE =  1000.0 * IU_PER_MM;
static const double MIN_GRID_SIZE = 0.001 * IU_PER_MM;

// Min/Max value for grid offset
static const double MAX_GRID_OFFSET = INT_MAX / 2.0;

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
    void OnInitDlg( wxInitDialogEvent& event ) override
    {
        // Call the default wxDialog handler of a wxInitDialogEvent
        TransferDataToWindow();

        // Now all widgets have the size fixed, call FinishDialogSettings
        FinishDialogSettings();
    }

    void            setGridSize( const wxPoint& grid );
    bool            getGridSize( wxPoint& aGrisSize );

    void            setGridOrigin( const wxPoint& grid );
    bool            getGridOrigin( wxPoint& aGridOrigin );

    void            setGridForFastSwitching( const wxArrayString& aGrids, int aGrid1, int aGrid2 );
    void            getGridForFastSwitching( int& aGrid1, int& aGrid2 );
};


DIALOG_SET_GRID::DIALOG_SET_GRID( PCB_BASE_FRAME* aParent, const wxArrayString& aGridChoices ):
    DIALOG_SET_GRID_BASE( aParent ),
    m_parent( aParent ),
    m_fast_grid_opts( aGridChoices )
{
    m_sdbSizerOK->SetDefault();         // set OK button as default response to 'Enter' key

    m_TextPosXUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_TextPosYUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );

    m_TextSizeXUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_TextSizeYUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
}


bool DIALOG_SET_GRID::TransferDataFromWindow()
{
    // Validate new settings
    wxPoint gridOrigin;

    if( !getGridOrigin( gridOrigin ) )
    {
        wxMessageBox( wxString::Format( _( "Incorrect grid origin "
                        "(coordinates must be >= %.3f mm and <= %.3f mm)" ),
                        -MAX_GRID_OFFSET/IU_PER_MM, MAX_GRID_OFFSET/IU_PER_MM ) );

        return false;
    }

    wxPoint gridSize;

    if( !getGridSize( gridSize ) )
    {
        wxMessageBox( wxString::Format( _( "Incorrect grid size "
                        "(size must be >= %.3f mm and <= %.3f mm)" ),
                        MIN_GRID_SIZE/IU_PER_MM, MAX_GRID_SIZE/IU_PER_MM ) );

        return false;
    }

    int fastGrid1, fastGrid2;
    getGridForFastSwitching( fastGrid1, fastGrid2 );

    // Apply the new settings

     // Because grid origin is saved in board, show as modified
    m_parent->OnModify();
    m_parent->SetGridOrigin( gridOrigin );
    m_parent->m_UserGridSize = gridSize;
    m_parent->m_FastGrid1 = fastGrid1;
    m_parent->m_FastGrid2 = fastGrid2;

    // User grid
    BASE_SCREEN* screen = m_parent->GetScreen();
    screen->AddGrid( gridSize, EDA_UNITS_T::UNSCALED_UNITS, ID_POPUP_GRID_USER );

    // If the user grid is the current option, recall SetGrid()
    // to force new values put in list as current grid value
    if( screen->GetGridCmdId() == ID_POPUP_GRID_USER )
        screen->SetGrid( ID_POPUP_GRID_USER );

    // Notify GAL
    TOOL_MANAGER* mgr = m_parent->GetToolManager();

    if( mgr && m_parent->IsGalCanvasActive() )
    {
        mgr->RunAction( "common.Control.gridPreset", true,
                screen->GetGridCmdId() - ID_POPUP_GRID_LEVEL_1000 );

        TOOL_EVENT gridOriginUpdate = ACTIONS::gridSetOrigin.MakeEvent();
        gridOriginUpdate.SetParameter( new VECTOR2D( gridOrigin ) );
        mgr->ProcessEvent( gridOriginUpdate );
    }

    return wxDialog::TransferDataFromWindow();
}


bool DIALOG_SET_GRID::TransferDataToWindow()
{
    setGridSize( m_parent->m_UserGridSize );
    setGridOrigin( m_parent->GetGridOrigin() );
    setGridForFastSwitching( m_fast_grid_opts, m_parent->m_FastGrid1, m_parent->m_FastGrid2 );

    return wxDialog::TransferDataToWindow();
}


void DIALOG_SET_GRID::setGridSize( const wxPoint& grid )
{
    wxString msg;

    msg.Printf( wxT( "%.10g" ), To_User_Unit( g_UserUnit, grid.x ) );
    m_OptGridSizeX->SetValue( msg );

    msg.Printf( wxT( "%.10g" ), To_User_Unit( g_UserUnit, grid.y ) );
    m_OptGridSizeY->SetValue( msg );
}


bool DIALOG_SET_GRID::getGridSize( wxPoint& aGridSize )
{
    wxString value = m_OptGridSizeX->GetValue();
    double x = DoubleValueFromString( g_UserUnit, value );

    value = m_OptGridSizeY->GetValue();
    double y = DoubleValueFromString( g_UserUnit, value );

    // Some error checking here is a good thing.
    if( y < MIN_GRID_SIZE || y > MAX_GRID_SIZE )
        return false;

    if( x < MIN_GRID_SIZE || x > MAX_GRID_SIZE )
        return false;

    aGridSize.x = KiROUND( x );
    aGridSize.y = KiROUND( y );

    return true;
}


bool DIALOG_SET_GRID::getGridOrigin( wxPoint& aGridOrigin )
{
    wxString value = m_GridOriginXCtrl->GetValue();
    double x = DoubleValueFromString( g_UserUnit, value );

    value = m_GridOriginYCtrl->GetValue();
    double y = DoubleValueFromString( g_UserUnit, value );

    // Some error checking here is a good thing.
    if( x < -MAX_GRID_OFFSET || x > MAX_GRID_OFFSET )
        return false;

    if( y < -MAX_GRID_OFFSET || y > MAX_GRID_OFFSET )
        return false;

    aGridOrigin.x = KiROUND( x );
    aGridOrigin.y = KiROUND( y );

    return true;
}


void DIALOG_SET_GRID::setGridOrigin( const wxPoint& grid )
{
    PutValueInLocalUnits( *m_GridOriginXCtrl, grid.x );
    PutValueInLocalUnits( *m_GridOriginYCtrl, grid.y );
}


void DIALOG_SET_GRID::setGridForFastSwitching( const wxArrayString& aGrids, int aGrid1, int aGrid2 )
{
    m_comboBoxGrid1->Append( aGrids );
    m_comboBoxGrid2->Append( aGrids );

    m_comboBoxGrid1->SetSelection( aGrid1 );
    m_comboBoxGrid2->SetSelection( aGrid2 );
}


void DIALOG_SET_GRID::getGridForFastSwitching( int& aGrid1, int& aGrid2 )
{
    aGrid1 = m_comboBoxGrid1->GetSelection();
    aGrid2 = m_comboBoxGrid2->GetSelection();
}


void DIALOG_SET_GRID::OnResetGridOrgClick( wxCommandEvent& event )
{
    setGridOrigin( wxPoint( 0, 0 ) );
}


bool PCB_BASE_FRAME::InvokeDialogGrid()
{
    DIALOG_SET_GRID dlg( this, m_gridSelectBox->GetStrings() );
    return dlg.ShowModal();
}
