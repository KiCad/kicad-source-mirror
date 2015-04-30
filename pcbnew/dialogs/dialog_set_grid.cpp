/**
 * @file dialog_set_grid.cpp
 * @brief Manage user grid.
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <macros.h>
#include <common.h>
#include <base_units.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <dialog_set_grid_base.h>
#include <invoke_pcb_dialog.h>

#include <gal/graphics_abstraction_layer.h>
#include <class_draw_panel_gal.h>
#include <tool/tool_manager.h>


class DIALOG_SET_GRID : public DIALOG_SET_GRID_BASE
{
public:

    /// This has no dependencies on calling wxFrame derivative, such as PCB_BASE_FRAME.
    DIALOG_SET_GRID( wxFrame* aCaller, EDA_UNITS_T* aGridUnits, EDA_UNITS_T aBoardUnits,
        wxRealPoint* aUserSize, wxPoint* aOrigin,
        int* aFastGrid1, int* aFastGrid2, const wxArrayString& aGridChoices );

private:
    void            OnResetGridOrgClick( wxCommandEvent& event );
    void            OnCancelClick( wxCommandEvent& event );
    void            OnOkClick( wxCommandEvent& event );

    EDA_UNITS_T&    m_callers_grid_units;
    EDA_UNITS_T     m_callers_board_units;
    wxRealPoint&    m_callers_user_size;
    wxPoint&        m_callers_origin;
    int&            m_callers_fast_grid1;
    int&            m_callers_fast_grid2;

    void            setGridUnits( EDA_UNITS_T units );
    EDA_UNITS_T     getGridUnits();

    void            setGridSize( const wxRealPoint& grid );
    wxRealPoint     getGridSize();

    void            setGridOrigin( const wxPoint& grid );
    wxPoint         getGridOrigin();

    void            setGridForFastSwitching( const wxArrayString& aGrids, int aGrid1, int aGrid2 );
    void            getGridForFastSwitching( int& aGrid1, int& aGrid2 );
};


DIALOG_SET_GRID::DIALOG_SET_GRID( wxFrame* aCaller, EDA_UNITS_T* aGridUnits, EDA_UNITS_T aBoardUnits,
        wxRealPoint* aUserSize, wxPoint* aOrigin, int* aFastGrid1, int* aFastGrid2, const wxArrayString& aGridChoices ):
    DIALOG_SET_GRID_BASE( aCaller ),
    m_callers_grid_units( *aGridUnits ),
    m_callers_board_units( aBoardUnits ),
    m_callers_user_size( *aUserSize ),
    m_callers_origin( *aOrigin ),
    m_callers_fast_grid1( *aFastGrid1 ),
    m_callers_fast_grid2( *aFastGrid2 )
{
    m_TextPosXUnits->SetLabel( GetUnitsLabel( m_callers_board_units ) );
    m_TextPosYUnits->SetLabel( GetUnitsLabel( m_callers_board_units ) );

    m_sdbSizer1OK->SetDefault();      // set OK button as default response to 'Enter' key

    setGridUnits( m_callers_grid_units );
    setGridSize( m_callers_user_size );
    setGridOrigin( m_callers_origin );
    setGridForFastSwitching( aGridChoices, m_callers_fast_grid1, m_callers_fast_grid2 );

    GetSizer()->SetSizeHints( this );
    Fit();
    Centre();
}


void DIALOG_SET_GRID::setGridUnits( EDA_UNITS_T aUnits )
{
    m_UnitGrid->SetSelection( aUnits != INCHES  );
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


wxRealPoint DIALOG_SET_GRID::getGridSize()
{
    wxRealPoint grid;

    // @todo: Some error checking here would be a good thing.
    wxString    x = m_OptGridSizeX->GetValue();
    wxString    y = m_OptGridSizeY->GetValue();

    x.ToDouble( &grid.x );
    y.ToDouble( &grid.y );

    return grid;
}


wxPoint DIALOG_SET_GRID::getGridOrigin()
{
    wxPoint grid;

    // @todo Some error checking here would be a good thing.
    grid.x = ValueFromTextCtrl( *m_GridOriginXCtrl );
    grid.y = ValueFromTextCtrl( *m_GridOriginYCtrl );

    return grid;
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


void DIALOG_SET_GRID::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_SET_GRID::OnOkClick( wxCommandEvent& event )
{
    m_callers_grid_units = getGridUnits();
    m_callers_user_size  = getGridSize();
    m_callers_origin     = getGridOrigin();

    getGridForFastSwitching( m_callers_fast_grid1, m_callers_fast_grid2 );

    EndModal( wxID_OK );
}


#include <class_drawpanel.h>
#include <wxBasePcbFrame.h>

bool PCB_BASE_FRAME::InvokeDialogGrid()
{
    wxPoint grid_origin = GetGridOrigin();

    DIALOG_SET_GRID dlg( this, &m_UserGridUnit, g_UserUnit, &m_UserGridSize,
        &grid_origin, &m_FastGrid1, &m_FastGrid2,
        m_gridSelectBox->GetStrings() );

    int ret = dlg.ShowModal();

    if( ret == wxID_OK )
    {
        if( GetGridOrigin() != grid_origin && IsType( FRAME_PCB ) )
            OnModify();     // because grid origin is saved in board, show as modified

        SetGridOrigin( grid_origin );

        BASE_SCREEN* screen = GetScreen();

        screen->AddGrid( m_UserGridSize, m_UserGridUnit, ID_POPUP_GRID_USER );

        // If the user grid is the current option, recall SetGrid()
        // to force new values put in list as current grid value
        if( screen->GetGridId() == ID_POPUP_GRID_USER )
            screen->SetGrid( ID_POPUP_GRID_USER );

        // Notify GAL
        TOOL_MANAGER* mgr = GetToolManager();

        if( mgr && IsGalCanvasActive() )
            mgr->RunAction( "common.Control.gridPreset", true,
                    ID_POPUP_GRID_USER - ID_POPUP_GRID_LEVEL_1000 );

        m_canvas->Refresh();

        return true;
    }

    return false;
}
