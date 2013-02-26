/**
 * @file set_grid.cpp
 * @brief Manage user grid.
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <common.h>
#include <class_drawpanel.h>
#include <wxBasePcbFrame.h>
#include <base_units.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <dialog_set_grid_base.h>


class DIALOG_SET_GRID : public DIALOG_SET_GRID_BASE
{
public:
    DIALOG_SET_GRID( wxWindow* parent, const wxPoint& pos );
    ~DIALOG_SET_GRID() { }
    void        SetGridSize( const wxRealPoint& grid );
    wxRealPoint GetGridSize();
    void        SetGridUnits( int units );
    int         GetGridUnits();
    void        SetGridOrigin( const wxPoint& grid );
    wxPoint     GetGridOrigin();
    void        SetGridForFastSwitching( wxArrayString aGrids, int aGrid1, int aGrid2 );
    void        GetGridForFastSwitching( int& aGrid1, int& aGrid2 );

private:
    void        OnResetGridOrgClick( wxCommandEvent& event );
    void        OnCancelClick( wxCommandEvent& event );
    void        OnOkClick( wxCommandEvent& event );
};

void PCB_BASE_FRAME::InstallGridFrame( const wxPoint& pos )
{
    DIALOG_SET_GRID dlg( this, pos );

    dlg.SetGridUnits( m_UserGridUnit );
    dlg.SetGridSize( m_UserGridSize );
    dlg.SetGridOrigin( GetScreen()->m_GridOrigin );

    if( m_gridSelectBox )
        dlg.SetGridForFastSwitching( m_gridSelectBox->GetStrings(), m_FastGrid1, m_FastGrid2 );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_UserGridSize = dlg.GetGridSize();
    m_UserGridUnit = (EDA_UNITS_T) dlg.GetGridUnits();
    GetScreen()->m_GridOrigin = dlg.GetGridOrigin();

    GetScreen()->AddGrid( m_UserGridSize, m_UserGridUnit, ID_POPUP_GRID_USER );

    dlg.GetGridForFastSwitching( m_FastGrid1, m_FastGrid2 );

    // If the user grid is the current option, recall SetGrid()
    // to force new values put in list as current grid value
    if( GetScreen()->GetGridId() == ID_POPUP_GRID_USER )
        GetScreen()->SetGrid( ID_POPUP_GRID_USER  );

    m_canvas->Refresh();
}


DIALOG_SET_GRID::DIALOG_SET_GRID( wxWindow* parent, const wxPoint& pos ) :
    DIALOG_SET_GRID_BASE( parent )
{
    SetFocus();

    m_TextPosXUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_TextPosYUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );

    m_sdbSizer1OK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_SET_GRID::SetGridSize( const wxRealPoint& grid )
{
    wxString msg;

    msg.Printf( wxT( "%.6f" ), grid.x );
    m_OptGridSizeX->SetValue( msg );
    msg.Printf( wxT( "%.6f" ), grid.y );
    m_OptGridSizeY->SetValue( msg );
}


wxRealPoint DIALOG_SET_GRID::GetGridSize()
{
    wxRealPoint grid;


    /* TODO: Some error checking here would be a good thing. */
    m_OptGridSizeX->GetValue().ToDouble( &grid.x );
    m_OptGridSizeY->GetValue().ToDouble( &grid.y );

    return grid;
}


void DIALOG_SET_GRID::SetGridUnits( int units )
{
    if( units != INCHES )
        m_UnitGrid->SetSelection( 1 );
}


int DIALOG_SET_GRID::GetGridUnits()
{
    return m_UnitGrid->GetSelection();
}


wxPoint DIALOG_SET_GRID::GetGridOrigin()
{
    wxPoint grid;

    /* TODO: Some error checking here would be a good thing. */
    grid.x = ReturnValueFromTextCtrl( *m_GridOriginXCtrl );
    grid.y = ReturnValueFromTextCtrl( *m_GridOriginYCtrl );

    return grid;
}


void DIALOG_SET_GRID::SetGridOrigin( const wxPoint& grid )
{
    wxString msg;

    PutValueInLocalUnits( *m_GridOriginXCtrl, grid.x );
    PutValueInLocalUnits( *m_GridOriginYCtrl, grid.y );
}

void DIALOG_SET_GRID::SetGridForFastSwitching( wxArrayString aGrids, int aGrid1, int aGrid2  )
{
    m_comboBoxGrid1->Append( aGrids );
    m_comboBoxGrid2->Append( aGrids );

    m_comboBoxGrid1->SetSelection( aGrid1 );
    m_comboBoxGrid2->SetSelection( aGrid2 );
}

void DIALOG_SET_GRID::GetGridForFastSwitching( int& aGrid1, int& aGrid2 )
{
    aGrid1 = m_comboBoxGrid1->GetSelection();
    aGrid2 = m_comboBoxGrid2->GetSelection();
}


void DIALOG_SET_GRID::OnResetGridOrgClick( wxCommandEvent& event )
{
    SetGridOrigin( wxPoint(0,0) );
}


/*****************************************************************/
void DIALOG_SET_GRID::OnCancelClick( wxCommandEvent& event )
/*****************************************************************/
{
    EndModal( wxID_CANCEL );
}


/*************************************************************************/
void DIALOG_SET_GRID::OnOkClick( wxCommandEvent& event )
/*************************************************************************/
{
    EndModal( wxID_OK );
}
