/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cmath>

#include <calculator_panels/panel_r_divider.h>
#include <bitmaps.h>
#include <eseries.h>
#include <pcb_calculator_settings.h>
#include <pcb_calculator_utils.h>
#include <r_divider_solver.h>
#include <resistor_substitution_utils.h>
#include <string_utils.h>
#include <widgets/wx_grid.h>
#include <wx/msgdlg.h>

#include <i18n_utility.h> // For _HKI definition in r_divider_help.h
wxString r_divider_help =
#include "r_divider_help.h"


enum COLUMN
{
    COL_R1 = 0,
    COL_R2,
    COL_TARGET,
    COL_TARGET_ERROR,
    COL_IMPEDANCE,
    COL_IMPEDANCE_ERROR
};

// Resistors are offered from 1 mΩ to 10 TΩ
static constexpr int FIRST_DECADE = -3;
static constexpr int LAST_DECADE = 12;


static wxString formatLeg( const R_DIVIDER_LEG& aLeg )
{
    if( aLeg.B == 0 )
        return strValue( aLeg.A );

    return strValue( aLeg.A ) + ( aLeg.Parallel ? " | " : " + " ) + strValue( aLeg.B );
}


static wxString formatError( double aError )
{
    const double percent = aError * 100;

    if( std::abs( percent ) < 1e-9 )
        return _( "Exact" );
    else if( std::abs( percent ) < 0.01 )
        return wxS( "<0.01 %" );
    else
        return wxString::Format( wxS( "%+.2f %%" ), percent );
}


PANEL_R_DIVIDER::PANEL_R_DIVIDER( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
                                  const wxString& name ) :
        PANEL_R_DIVIDER_BASE( parent, id, pos, size, style, name )
{
    m_bitmapDivider->SetBitmap( KiBitmapBundle( BITMAPS::r_divider ) );

    wxString msg;
    ConvertMarkdown2Html( wxGetTranslation( r_divider_help ), msg );
    m_panelHelp->SetPage( msg );

    m_resultsGrid->SetupColumnAutosizer( COL_IMPEDANCE_ERROR );
    updateModeUI();

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


std::vector<std::pair<wxRadioButton*, int>> PANEL_R_DIVIDER::seriesButtons() const
{
    return { { m_e3, ESERIES::E3 },   { m_e6, ESERIES::E6 },   { m_e12, ESERIES::E12 },  { m_e24, ESERIES::E24 },
             { m_e48, ESERIES::E48 }, { m_e96, ESERIES::E96 }, { m_e192, ESERIES::E192 } };
}


int PANEL_R_DIVIDER::selectedSeries() const
{
    for( const auto& [button, series] : seriesButtons() )
    {
        if( button->GetValue() )
            return series;
    }

    return ESERIES::E24;
}


void PANEL_R_DIVIDER::ThemeChanged()
{
    m_bitmapDivider->SetBitmap( KiBitmapBundle( BITMAPS::r_divider ) );
    m_panelHelp->ThemeChanged();
}


void PANEL_R_DIVIDER::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    const PCB_CALCULATOR_SETTINGS::R_DIVIDER& cfg = aCfg->m_RDivider;

    // Radio buttons in a group may only be set to true; that clears the rest of the group
    ( cfg.mode == 1 ? m_rbModeVoltage : m_rbModeRatio )->SetValue( true );
    ( cfg.impedanceType == 1 ? m_impedanceParallel : m_impedanceSeries )->SetValue( true );

    m_ratioValue->SetValue( cfg.ratio );
    m_vinValue->SetValue( cfg.vin );
    m_voutValue->SetValue( cfg.vout );
    m_impedanceValue->SetValue( cfg.impedance );

    m_e24->SetValue( true );

    for( const auto& [button, series] : seriesButtons() )
    {
        if( series == cfg.series )
            button->SetValue( true );
    }

    updateModeUI();
}


void PANEL_R_DIVIDER::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    PCB_CALCULATOR_SETTINGS::R_DIVIDER& cfg = aCfg->m_RDivider;

    cfg.mode = m_rbModeVoltage->GetValue() ? 1 : 0;
    cfg.ratio = m_ratioValue->GetValue();
    cfg.vin = m_vinValue->GetValue();
    cfg.vout = m_voutValue->GetValue();
    cfg.impedanceType = m_impedanceParallel->GetValue() ? 1 : 0;
    cfg.impedance = m_impedanceValue->GetValue();
    cfg.series = selectedSeries();
}


void PANEL_R_DIVIDER::OnModeChange( wxCommandEvent& aEvent )
{
    updateModeUI();
}


void PANEL_R_DIVIDER::OnCalculate( wxCommandEvent& aEvent )
{
    const bool   voltageMode = m_rbModeVoltage->GetValue();
    const double vin = DoubleFromString( m_vinValue->GetValue() );
    const double vout = DoubleFromString( m_voutValue->GetValue() );
    const double ratio = voltageMode ? vin / vout - 1 : DoubleFromString( m_ratioValue->GetValue() );
    const double impedance = 1000 * DoubleFromString( m_impedanceValue->GetValue() ); // entered in kΩ

    m_resultsGrid->ClearGrid();

    if( voltageMode && !( vout > 0 && vout < vin ) )
    {
        wxMessageBox( _( "Vout must be greater than 0 V and less than Vin." ) );
        return;
    }

    std::vector<double> resistors;

    for( int decade = FIRST_DECADE; decade <= LAST_DECADE; ++decade )
    {
        ESERIES::ESERIES_IN_DECADE values( selectedSeries(), decade );
        resistors.insert( resistors.end(), values.begin(), values.end() );
    }

    const R_DIVIDER_IMPEDANCE impedanceType =
            m_impedanceParallel->GetValue() ? R_DIVIDER_IMPEDANCE::PARALLEL : R_DIVIDER_IMPEDANCE::SERIES;
    const R_DIVIDER_TARGET target = voltageMode ? R_DIVIDER_TARGET::VOLTAGE : R_DIVIDER_TARGET::RESISTOR_RATIO;

    const std::vector<R_DIVIDER> dividers = FindResistorDividers( ratio, impedance, impedanceType, resistors, target );

    if( dividers.empty() )
    {
        wxMessageBox( _( "The ratio R1 / R2 must be between 0.000001 and 1000000, and the ratio and the impedance "
                         "must call for resistors between 1 mΩ and 10 TΩ." ) );
        return;
    }

    const int rows = static_cast<int>( dividers.size() );

    if( m_resultsGrid->GetNumberRows() < rows )
        m_resultsGrid->AppendRows( rows - m_resultsGrid->GetNumberRows() );

    for( int row = 0; row < rows; ++row )
    {
        const R_DIVIDER& divider = dividers[row];
        const double     top = divider.Top.Value;
        const double     bottom = divider.Bottom.Value;

        const wxString target = voltageMode ? wxString::Format( wxS( "%.4g V" ), vin * bottom / ( top + bottom ) )
                                            : wxString::Format( wxS( "%.4g" ), top / bottom );

        m_resultsGrid->SetCellValue( row, COL_R1, formatLeg( divider.Top ) );
        m_resultsGrid->SetCellValue( row, COL_R2, formatLeg( divider.Bottom ) );
        m_resultsGrid->SetCellValue( row, COL_TARGET, target );
        m_resultsGrid->SetCellValue( row, COL_TARGET_ERROR, formatError( divider.TargetError ) );
        m_resultsGrid->SetCellValue( row, COL_IMPEDANCE, strValue( divider.Impedance ) );
        m_resultsGrid->SetCellValue( row, COL_IMPEDANCE_ERROR, formatError( divider.ImpedanceError ) );
    }

    m_resultsGrid->SetGridWidthsDirty();
}


void PANEL_R_DIVIDER::updateModeUI()
{
    const bool voltageMode = m_rbModeVoltage->GetValue();

    m_ratioLabel->Enable( !voltageMode );
    m_ratioValue->Enable( !voltageMode );
    m_vinLabel->Enable( voltageMode );
    m_vinValue->Enable( voltageMode );
    m_vinUnit->Enable( voltageMode );
    m_voutLabel->Enable( voltageMode );
    m_voutValue->Enable( voltageMode );
    m_voutUnit->Enable( voltageMode );

    m_resultsGrid->SetColLabelValue( COL_TARGET, voltageMode ? _( "Vout" ) : _( "Ratio" ) );
    m_resultsGrid->SetColLabelValue( COL_TARGET_ERROR, voltageMode ? _( "Voltage error" ) : _( "Ratio error" ) );

    // Results of the other mode would sit under the wrong headings
    m_resultsGrid->ClearGrid();
    m_resultsGrid->SetGridWidthsDirty();
}
