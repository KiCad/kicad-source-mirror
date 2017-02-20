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

#include <wxBasePcbFrame.h>
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>

#include <gal/graphics_abstraction_layer.h>
#include <tools/pcb_actions.h>
#include <tool/tool_manager.h>

#include <limits.h>

// Max values for grid size
static const double MAX_GRID_SIZE =  50.0 * IU_PER_MM;
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

    void            setGridUnits( EDA_UNITS_T units );
    EDA_UNITS_T     getGridUnits();

    void            setGridSize( const wxRealPoint& grid );
    bool            getGridSize( wxRealPoint& aGrisSize );

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

    m_TextPosXUnits->SetLabel( GetUnitsLabel( m_parent->m_UserGridUnit ) );
    m_TextPosYUnits->SetLabel( GetUnitsLabel( m_parent->m_UserGridUnit ) );

}


bool DIALOG_SET_GRID::TransferDataFromWindow()
{
    // Validate new settings
    wxRealPoint gridSize;
    if( !getGridSize( gridSize ) )
    {
        wxMessageBox( wxString::Format( _( "Incorrect grid size "
                        "(size must be >= %.3f mm and <= %.3f mm)" ),
                        MIN_GRID_SIZE/IU_PER_MM, MAX_GRID_SIZE/IU_PER_MM ) );

        return false;
    }

    wxPoint gridOrigin;
    if( !getGridOrigin( gridOrigin ) )
    {
        wxMessageBox( wxString::Format( _( "Incorrect grid origin "
                        "(coordinates must be >= %.3f mm and <= %.3f mm)" ),
                        -MAX_GRID_OFFSET/IU_PER_MM, MAX_GRID_OFFSET/IU_PER_MM ) );

        return false;
    }

    int fastGrid1, fastGrid2;
    getGridForFastSwitching( fastGrid1, fastGrid2 );

    EDA_UNITS_T units = getGridUnits();

    // Apply the new settings

     // Because grid origin is saved in board, show as modified
    m_parent->OnModify();
    m_parent->SetGridOrigin( gridOrigin );
    m_parent->m_UserGridUnit = units;
    m_parent->m_UserGridSize = gridSize;
    m_parent->m_FastGrid1 = fastGrid1;
    m_parent->m_FastGrid2 = fastGrid2;

    // User grid
    BASE_SCREEN* screen = m_parent->GetScreen();
    screen->AddGrid( gridSize, units, ID_POPUP_GRID_USER );

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
    setGridUnits( m_parent->m_UserGridUnit );
    setGridSize( m_parent->m_UserGridSize );
    setGridOrigin( m_parent->GetGridOrigin() );
    setGridForFastSwitching( m_fast_grid_opts, m_parent->m_FastGrid1, m_parent->m_FastGrid2 );

    return wxDialog::TransferDataToWindow();
}


void DIALOG_SET_GRID::setGridUnits( EDA_UNITS_T aUnits )
{
    m_UnitGrid->SetSelection( aUnits != INCHES );
}


EDA_UNITS_T DIALOG_SET_GRID::getGridUnits()
{
    return m_UnitGrid->GetSelection() == 0 ? INCHES : MILLIMETRES;
}


void DIALOG_SET_GRID::setGridSize( const wxRealPoint& grid )
{
    wxString msg;

    msg.Printf( wxT( "%.10g" ), grid.x );
    m_OptGridSizeX->SetValue( msg );

    msg.Printf( wxT( "%.10g" ), grid.y );
    m_OptGridSizeY->SetValue( msg );
}


bool DIALOG_SET_GRID::getGridSize( wxRealPoint& aGrisSize )
{
    wxRealPoint grid;
    wxString val = m_OptGridSizeX->GetValue();
    double grid_unit_to_iu = ( getGridUnits() == INCHES ? IU_PER_MILS * 1000 : IU_PER_MM );
    double tmp;

    if( !val.ToDouble( &tmp ) ||
        tmp * grid_unit_to_iu < MIN_GRID_SIZE || tmp * grid_unit_to_iu > MAX_GRID_SIZE )
    {
        return false;
    }
    else
        aGrisSize.x = tmp;

    val = m_OptGridSizeY->GetValue();

    if( !val.ToDouble( &tmp ) ||
        tmp*grid_unit_to_iu < MIN_GRID_SIZE || tmp*grid_unit_to_iu > MAX_GRID_SIZE )
    {
        return false;
    }
    else
        aGrisSize.y = tmp;

    return true;
}


bool DIALOG_SET_GRID::getGridOrigin( wxPoint& aGridOrigin )
{
    double x, y;

    const wxString& x_str = m_GridOriginXCtrl->GetValue();

    if( !x_str.ToDouble( &x ) )
        return false;

    x = DoubleValueFromString( g_UserUnit, x_str );

    // Some error checking here is a good thing.
    if( x < -MAX_GRID_OFFSET || x > MAX_GRID_OFFSET )
        return false;


    const wxString& y_str = m_GridOriginYCtrl->GetValue();

    if( !y_str.ToDouble( &y ) )
        return false;

    y = DoubleValueFromString( g_UserUnit, y_str );

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
