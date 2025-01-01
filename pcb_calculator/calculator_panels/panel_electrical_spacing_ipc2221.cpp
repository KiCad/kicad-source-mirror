/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* see
 * http://www.desmith.net/NMdS/Electronics/TraceWidth.html
 * http://www.ultracad.com/articles/pcbtemp.pdf
 * for more info
 */

#include <calculator_panels/panel_electrical_spacing_ipc2221.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>
#include <widgets/unit_selector.h>
#include <units_scales.h>

extern double DoubleFromString( const wxString& TextValue );

// The IPC2221 formula used to calculate track width is valid only for copper material
const double copper_resistivity = 1.72e-8;


#define VALUE_COUNT 7
#define CLASS_COUNT 10

/* These values come from IPC2221
 *  there are 10 voltage classes:
 *  "0 ... 15V" "16 ... 30V" "31 ... 50V" "51 ... 100V"
 *  "101 ... 150V" "151 ... 170V" "171 ... 250V"
 *  "251 ... 300V" "301 ... 500V" " > 500V"
 *  and for each voltage class
 *  there ar e 7 cases:
 *  "B1" "B2" "B3" "B4" "A5" "A6" "A7"
 *  B1 - Internal Conductors
 *  B2 - External Conductors, uncoated, sea level to 3050 m
 *  B3 - External Conductors, uncoated, over 3050 m
 *  B4 - External Conductors, with permanent polymer coating (any elevation)
 *  A5 - External Conductors, with conformal coating over assembly (any elevation)
 *  A6 - External Component lead/termination, uncoated
 *  A7 - External Component lead termination, with conformal coating (any elevation)
 */

/* For voltages greater than 500V, the (per volt) table values
 *  must be added to the 500V values. For example, the elec-
 *  trical spacing for a Type B1 board with 600V is calculated
 *  as:
 *  600V - 500V = 100V
 *  0.25 mm + (100V x 0.0025
 */
static double clist[CLASS_COUNT][VALUE_COUNT] =
{
    { 0.05 * UNIT_MM, 0.1 * UNIT_MM,   0.1 * UNIT_MM,   0.05 * UNIT_MM,  0.13 * UNIT_MM, 0.13 *
      UNIT_MM,
      0.13 * UNIT_MM }, // 0 ... 15V

    { 0.05 * UNIT_MM, 0.1 * UNIT_MM,  0.1 * UNIT_MM,  0.05 * UNIT_MM, 0.13 * UNIT_MM, 0.25 *
      UNIT_MM,
      0.13 * UNIT_MM }, // 16 ... 30V

    { 0.1 * UNIT_MM,  0.6 * UNIT_MM,  0.6 * UNIT_MM,  0.13 * UNIT_MM, 0.13 * UNIT_MM, 0.4 *
      UNIT_MM,
      0.13 * UNIT_MM }, // 31 ... 50V

    { 0.1 * UNIT_MM,  0.6 * UNIT_MM,  1.5 * UNIT_MM,  0.13 * UNIT_MM, 0.13 * UNIT_MM, 0.5 *
      UNIT_MM,
      0.13 * UNIT_MM }, // 51 ... 100V

    { 0.2 * UNIT_MM,  0.6 * UNIT_MM,  3.2 * UNIT_MM,  0.4 * UNIT_MM,  0.4 * UNIT_MM,  0.8 *
      UNIT_MM,
      0.4 * UNIT_MM }, // 101 ... 150V

    { 0.2 * UNIT_MM,  1.25 * UNIT_MM, 3.2 * UNIT_MM,  0.4 * UNIT_MM,  0.4 * UNIT_MM,  0.8 *
      UNIT_MM,
      0.4 * UNIT_MM }, // 151 ... 170V

    { 0.2 * UNIT_MM,  1.25 * UNIT_MM, 6.4 * UNIT_MM,  0.4 * UNIT_MM,  0.4 * UNIT_MM,  0.8 *
      UNIT_MM,
      0.4 * UNIT_MM }, // 171 ... 250V

    { 0.2 * UNIT_MM,  1.25 * UNIT_MM, 12.5 * UNIT_MM, 0.4 * UNIT_MM,  0.4 * UNIT_MM,  0.8 *
      UNIT_MM,
      0.8 * UNIT_MM }, // 251 ... 300V

    { 0.25 * UNIT_MM, 2.5 * UNIT_MM,  12.5 * UNIT_MM, 0.8 * UNIT_MM,  0.8 * UNIT_MM,  1.5 *
      UNIT_MM,
      0.8 * UNIT_MM }, // 301 ... 500V

    // These last values are used to calculate spacing for voltage > 500V
    // there are not the spacing
    { 0.0025 * UNIT_MM, 0.005 * UNIT_MM,  0.025 * UNIT_MM, 0.00305 * UNIT_MM,
      0.00305 * UNIT_MM,  0.00305 * UNIT_MM, 0.00305 * UNIT_MM }, // > 500V
};


PANEL_ELECTRICAL_SPACING_IPC2221::PANEL_ELECTRICAL_SPACING_IPC2221( wxWindow* parent, wxWindowID id,
                                const wxPoint& pos, const wxSize& size,
                                long style, const wxString& name ) :
        PANEL_ELECTRICAL_SPACING_IPC2221_BASE( parent, id, pos, size, style, name )
{
    // Autosize the row label column to be sure label are not truncated
    m_gridElectricalSpacingValues->SetRowLabelSize( wxGRID_AUTOSIZE );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_ELECTRICAL_SPACING_IPC2221::~PANEL_ELECTRICAL_SPACING_IPC2221()
{
}


void PANEL_ELECTRICAL_SPACING_IPC2221::ThemeChanged()
{
    // TODO: Only the grid needs updating, but it isn't done automatically (https://trac.wxwidgets.org/ticket/19279)
}


void PANEL_ELECTRICAL_SPACING_IPC2221::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    aCfg->m_Electrical.spacing_units = m_ElectricalSpacingUnitsSelector->GetSelection();
    aCfg->m_Electrical.spacing_voltage = m_ElectricalSpacingVoltage->GetValue();
}


void PANEL_ELECTRICAL_SPACING_IPC2221::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_ElectricalSpacingUnitsSelector->SetSelection( aCfg->m_Electrical.spacing_units );
    m_ElectricalSpacingVoltage->SetValue( aCfg->m_Electrical.spacing_voltage );

    ElectricalSpacingUpdateData( m_ElectricalSpacingUnitsSelector->GetUnitScale() );
}



void PANEL_ELECTRICAL_SPACING_IPC2221::OnElectricalSpacingUnitsSelection( wxCommandEvent& event )
{
    ElectricalSpacingUpdateData( m_ElectricalSpacingUnitsSelector->GetUnitScale() );
}

void PANEL_ELECTRICAL_SPACING_IPC2221::OnElectricalSpacingRefresh( wxCommandEvent& event )
{
    ElectricalSpacingUpdateData( m_ElectricalSpacingUnitsSelector->GetUnitScale() );
}


void PANEL_ELECTRICAL_SPACING_IPC2221::ElectricalSpacingUpdateData( double aUnitScale )
{
    wxString txt;
    double voltage = 500.0;     // to calculate values at V > 500V
    txt = m_ElectricalSpacingVoltage->GetValue();

    if( ! txt.IsEmpty() )
        voltage = DoubleFromString( txt );

    if( voltage < 500.0 )
        voltage = 500.0;

    txt.Printf( wxT( "%g" ), voltage );
    m_ElectricalSpacingVoltage->SetValue( txt );

    for( int ii = 0; ii < CLASS_COUNT-1; ii++ )
    {
        for( int jj = 0; jj < VALUE_COUNT; jj++ )
        {
            txt.Printf( wxT( "%g" ), clist[ii][jj] / aUnitScale );
            m_gridElectricalSpacingValues->SetCellValue( ii, jj, txt );
        }
    }

    for( int jj = 0; jj < VALUE_COUNT; jj++ )
    {
        double spacing = clist[CLASS_COUNT-2][jj];
        double spacing_extra = clist[CLASS_COUNT-1][jj];
        spacing += spacing_extra * ( voltage - 500.0 );
        txt.Printf( wxT( "%g" ), spacing / aUnitScale );
        m_gridElectricalSpacingValues->SetCellValue( CLASS_COUNT-1, jj, txt );
    }

    m_gridElectricalSpacingValues->SetRowLabelSize( wxGRID_AUTOSIZE );
}
