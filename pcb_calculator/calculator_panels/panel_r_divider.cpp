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

#include <algorithm>
#include <cmath>
#include <limits>

#include <calculator_panels/panel_r_divider.h>
#include <bitmaps.h>
#include <eseries.h>
#include <pcb_calculator_settings.h>
#include <pcb_calculator_utils.h>
#include <string_utils.h>
#include <wx/msgdlg.h>

#include <i18n_utility.h> // For _HKI definition in r_divider_help.h

// The statement below ends inside the generated header, where clang-format cannot see it. The
// namespace's closing brace keeps clang-format from indenting the code that follows.
namespace
{
wxString r_divider_help =
#include "r_divider_help.h"
} // namespace


// Relative errors (in percent) below this are reported as exact
static constexpr double EXACT_TOLERANCE = 1e-9;

// Range of the target impedance, in ohms
static constexpr double MIN_TARGET_IMPEDANCE = 0.1;
static constexpr double MAX_TARGET_IMPEDANCE = 100e6;

// Range of the resistor ratio R1 / R2, allowed by this tool. Values outside this range show an error.
static constexpr double MIN_TARGET_RATIO = 1e-6;
static constexpr double MAX_TARGET_RATIO = 1e6;


static wxString formatResistance( double aOhms )
{
    if( aOhms >= 1e12 )
        return wxString::Format( wxT( "%.4g TΩ" ), aOhms / 1e12 );
    else if( aOhms >= 1e9 )
        return wxString::Format( wxT( "%.4g GΩ" ), aOhms / 1e9 );
    else if( aOhms >= 1e6 )
        return wxString::Format( wxT( "%.4g MΩ" ), aOhms / 1e6 );
    else if( aOhms >= 1e3 )
        return wxString::Format( wxT( "%.4g kΩ" ), aOhms / 1e3 );
    else if( aOhms >= 1 )
        return wxString::Format( wxT( "%.4g Ω" ), aOhms );
    else if( aOhms >= 1e-3 )
        return wxString::Format( wxT( "%.4g mΩ" ), aOhms * 1e3 );
    else
        return wxString::Format( wxT( "%.4g µΩ" ), aOhms * 1e6 );
}


/// Two resistors making up one leg, larger one first: "A + B" in series, "A | B" in parallel
static wxString formatLeg( double aFirst, double aSecond, const wxString& aOperator )
{
    return formatResistance( std::max( aFirst, aSecond ) ) + aOperator
           + formatResistance( std::min( aFirst, aSecond ) );
}


static wxString formatError( double aRelativeError )
{
    double error = aRelativeError * 100; // in percent

    if( std::abs( error ) < EXACT_TOLERANCE )
        return _( "Exact" );
    else if( std::abs( error ) < 0.01 )
        return wxString::Format( wxT( "<%.2f %%" ), 0.01 );
    else
        return wxString::Format( wxT( "%+.2f %%" ), error );
}


PANEL_R_DIVIDER::PANEL_R_DIVIDER( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
                                  const wxString& name ) :
        PANEL_R_DIVIDER_BASE( parent, id, pos, size, style, name ),
        m_series( ESERIES::E24 )
{
    m_bitmapDivider->SetBitmap( KiBitmapBundle( BITMAPS::r_divider ) );

    // show markdown formula explanation in lower help panel
    wxString msg;
    ConvertMarkdown2Html( wxGetTranslation( r_divider_help ), msg );
    m_panelHelp->SetPage( msg );

    updateModeUI();

    m_resultsGrid->Bind( wxEVT_SIZE,
                         [this]( wxSizeEvent& aEvent )
                         {
                             stretchColumns();
                             aEvent.Skip();
                         } );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_R_DIVIDER::~PANEL_R_DIVIDER()
{
}


std::vector<std::pair<wxRadioButton*, int>> PANEL_R_DIVIDER::seriesButtons() const
{
    return { { m_e3, ESERIES::E3 },   { m_e6, ESERIES::E6 },   { m_e12, ESERIES::E12 },  { m_e24, ESERIES::E24 },
             { m_e48, ESERIES::E48 }, { m_e96, ESERIES::E96 }, { m_e192, ESERIES::E192 } };
}


void PANEL_R_DIVIDER::ThemeChanged()
{
    // Update the bitmap
    m_bitmapDivider->SetBitmap( KiBitmapBundle( BITMAPS::r_divider ) );

    // Update the HTML window with the help text
    m_panelHelp->ThemeChanged();
}


void PANEL_R_DIVIDER::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    const PCB_CALCULATOR_SETTINGS::R_DIVIDER& cfg = aCfg->m_RDivider;

    // Radio buttons in a group may only be set to true; that clears the rest of the group
    if( cfg.mode == 1 )
        m_rbModeVoltage->SetValue( true );
    else
        m_rbModeRatio->SetValue( true );

    m_ratioValue->SetValue( cfg.ratio );
    m_vinValue->SetValue( cfg.vin );
    m_voutValue->SetValue( cfg.vout );

    if( cfg.impedanceType == 1 )
        m_impedanceParallel->SetValue( true );
    else
        m_impedanceSeries->SetValue( true );

    m_impedanceValue->SetValue( cfg.impedance );

    // Fall back to E24 if the stored series isn't one this panel offers
    m_series = ESERIES::E24;

    wxRadioButton* seriesButton = m_e24;

    for( const auto& [button, series] : seriesButtons() )
    {
        if( series == cfg.series )
        {
            m_series = series;
            seriesButton = button;
        }
    }

    seriesButton->SetValue( true );

    updateModeUI();
}


void PANEL_R_DIVIDER::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    PCB_CALCULATOR_SETTINGS::R_DIVIDER& cfg = aCfg->m_RDivider;

    cfg.mode = isVoltageMode() ? 1 : 0;
    cfg.ratio = m_ratioValue->GetValue();
    cfg.vin = m_vinValue->GetValue();
    cfg.vout = m_voutValue->GetValue();
    cfg.impedanceType = isParallelImpedance() ? 1 : 0;
    cfg.impedance = m_impedanceValue->GetValue();
    cfg.series = m_series;
}


void PANEL_R_DIVIDER::OnModeChange( wxCommandEvent& aEvent )
{
    updateModeUI();
}


void PANEL_R_DIVIDER::OnESeriesSelection( wxCommandEvent& aEvent )
{
    for( const auto& [button, series] : seriesButtons() )
    {
        if( aEvent.GetEventObject() == button )
            m_series = series;
    }
}


void PANEL_R_DIVIDER::OnCalculate( wxCommandEvent& aEvent )
{
    double ratio = 0;
    double vinEntered = 0;
    double voutEntered = 0;

    // Comparisons are written as !( x > 0 ) so NaN is rejected too
    if( isVoltageMode() )
    {
        vinEntered = DoubleFromString( m_vinValue->GetValue() );
        voutEntered = DoubleFromString( m_voutValue->GetValue() );

        if( !( vinEntered > 0 ) || !( voutEntered > 0 ) || !( voutEntered < vinEntered ) )
        {
            wxMessageBox( _( "Vin and Vout must be greater than 0 V, and Vout must be less than Vin." ) );
            return;
        }

        ratio = vinEntered / voutEntered - 1;

        if( !( ratio >= MIN_TARGET_RATIO ) || !( ratio <= MAX_TARGET_RATIO ) )
        {
            wxMessageBox( _( "Vin and Vout must give a ratio R1 / R2 between 0.000001 and 1000000." ) );
            return;
        }
    }
    else
    {
        ratio = DoubleFromString( m_ratioValue->GetValue() );

        if( !( ratio >= MIN_TARGET_RATIO ) || !( ratio <= MAX_TARGET_RATIO ) )
        {
            wxMessageBox( _( "The resistance ratio must be between 0.000001 and 1000000." ) );
            return;
        }
    }

    // Entered in kΩ, calculated in Ω
    double impedance = 1000 * DoubleFromString( m_impedanceValue->GetValue() );

    if( !( impedance >= MIN_TARGET_IMPEDANCE ) || !( impedance <= MAX_TARGET_IMPEDANCE ) )
    {
        wxMessageBox( _( "The target impedance must be between 0.1 Ω and 100 MΩ." ) );
        return;
    }

    wxBusyCursor busyCursor; // As long as this variable exists, the cursor will be 'busy'

    // The solver works on a voltage ratio. A resistor ratio k is the same as dividing k + 1 down to 1.
    double vin = isVoltageMode() ? vinEntered : ratio + 1;
    double vout = isVoltageMode() ? voutEntered : 1;

    const std::vector<uint16_t>& series = ESERIES::ESERIES_VALUES( m_series );
    const R_DIVIDER_IMPEDANCE    impedanceType = isParallelImpedance() ? R_DIVIDER_IMPEDANCE::PARALLEL
                                                                       : R_DIVIDER_IMPEDANCE::SERIES;

    const R_DIVIDER_TARGET target = isVoltageMode() ? R_DIVIDER_TARGET::VOLTAGE : R_DIVIDER_TARGET::RESISTOR_RATIO;

    // The two resistor dividers come first, followed by the three resistor ones
    std::vector<R_DIVIDER_RESULT> results = FindResistorDividers( vin, vout, impedance, impedanceType, series,
                                                                  target );

    // The inputs are all within their limits, so the only way to get nothing is a combination
    // of them that calls for resistors the solver does not offer
    if( results.empty() )
    {
        clearResults();
        wxMessageBox( _( "This combination of ratio and impedance needs resistors outside the range of "
                         "1 mΩ to 1 TΩ." ) );
        return;
    }

    fillResults( results, vinEntered );
}


void PANEL_R_DIVIDER::updateModeUI()
{
    bool voltageMode = isVoltageMode();

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

    // Results from the other mode would be shown under the wrong column headings
    clearResults();
    sizeColumns();
}


void PANEL_R_DIVIDER::fillResults( const std::vector<R_DIVIDER_RESULT>& aResults, double aVin )
{
    clearResults();

    // The grid has 9 rows, which is the most that the solvers return: 5 two resistor dividers and
    // 4 three resistor ones
    int rows = std::min<int>( aResults.size(), m_resultsGrid->GetNumberRows() );

    for( int row = 0; row < rows; ++row )
    {
        const R_DIVIDER_SOLUTION& sol = aResults[row].Solution;

        double top = sol.Top();
        double bottom = sol.Bottom();

        wxString target;

        if( isVoltageMode() )
            target = wxString::Format( wxT( "%.4g V" ), aVin * bottom / ( top + bottom ) );
        else
            target = wxString::Format( wxT( "%.4g" ), top / bottom );

        // A leg made of two resistors is written the way the resistor calculator does
        wxString r1 = formatResistance( sol.R1 );
        wxString r2 = formatResistance( sol.R2 );

        switch( sol.Topology )
        {
        case R_DIVIDER_TOPOLOGY::SPLIT_TOP:       r1 = formatLeg( sol.R1, sol.R3, wxT( " + " ) ); break;
        case R_DIVIDER_TOPOLOGY::SPLIT_BOTTOM:    r2 = formatLeg( sol.R2, sol.R3, wxT( " + " ) ); break;
        case R_DIVIDER_TOPOLOGY::PARALLEL_TOP:    r1 = formatLeg( sol.R1, sol.R3, wxT( " | " ) ); break;
        case R_DIVIDER_TOPOLOGY::PARALLEL_BOTTOM: r2 = formatLeg( sol.R2, sol.R3, wxT( " | " ) ); break;
        case R_DIVIDER_TOPOLOGY::TWO_RESISTORS:   break;
        }

        m_resultsGrid->SetCellValue( row, COL_R1, r1 );
        m_resultsGrid->SetCellValue( row, COL_R2, r2 );
        m_resultsGrid->SetCellValue( row, COL_TARGET, target );
        m_resultsGrid->SetCellValue( row, COL_TARGET_ERROR, formatError( aResults[row].TargetError ) );
        m_resultsGrid->SetCellValue( row, COL_IMPEDANCE, formatResistance( aResults[row].Impedance ) );
        m_resultsGrid->SetCellValue( row, COL_IMPEDANCE_ERROR, formatError( aResults[row].ImpedanceError ) );
    }

    // Point out the best of each error in bold, and the resistors of the most accurate two
    // resistor divider. Rows that are equally good up to round-off are all marked.
    if( rows > 0 )
    {
        const wxFont bold = m_resultsGrid->GetDefaultCellFont().Bold();
        const double margin = EXACT_TOLERANCE / 100;

        std::vector<double> targetErrors;
        std::vector<double> impedanceErrors;

        for( int row = 0; row < rows; ++row )
        {
            targetErrors.push_back( std::abs( aResults[row].TargetError ) );
            impedanceErrors.push_back( std::abs( aResults[row].ImpedanceError ) );
        }

        double bestTargetError = *std::min_element( targetErrors.begin(), targetErrors.end() );
        double bestImpedanceError = *std::min_element( impedanceErrors.begin(), impedanceErrors.end() );

        int bestPair = -1;

        for( int row = 0; row < rows; ++row )
        {
            if( targetErrors[row] <= bestTargetError + margin )
                m_resultsGrid->SetCellFont( row, COL_TARGET_ERROR, bold );

            if( impedanceErrors[row] <= bestImpedanceError + margin )
                m_resultsGrid->SetCellFont( row, COL_IMPEDANCE_ERROR, bold );

            // The first of equally accurate pairs is the one closest to the impedance target
            if( aResults[row].Solution.Topology == R_DIVIDER_TOPOLOGY::TWO_RESISTORS
                && ( bestPair < 0 || targetErrors[row] < targetErrors[bestPair] - margin ) )
            {
                bestPair = row;
            }
        }

        if( bestPair >= 0 )
        {
            m_resultsGrid->SetCellFont( bestPair, COL_R1, bold );
            m_resultsGrid->SetCellFont( bestPair, COL_R2, bold );
        }
    }

    sizeColumns();
}


void PANEL_R_DIVIDER::sizeColumns()
{
    // Size each column for the widest of its label, a typical value and its current contents,
    // so the grid has its full width before the first calculation and doesn't jump afterwards.
    static const wxString typical[] = { wxT( "999.9 kΩ" ), wxT( "999.9 kΩ" ), wxT( "99.99 V" ),
                                        wxT( "+99.99 %" ), wxT( "999.9 kΩ" ), wxT( "+99.99 %" ) };

    const wxFont labelFont = m_resultsGrid->GetLabelFont();
    const wxFont cellFont = m_resultsGrid->GetDefaultCellFont();
    const int    padding = m_resultsGrid->GetCharWidth();

    auto textWidth =
            [&]( const wxString& aText, const wxFont& aFont )
            {
                int width = 0;
                m_resultsGrid->GetTextExtent( aText, &width, nullptr, nullptr, nullptr, &aFont );
                return width;
            };

    m_minColWidths.resize( m_resultsGrid->GetNumberCols() );

    for( int col = 0; col < m_resultsGrid->GetNumberCols(); ++col )
    {
        int width = std::max( textWidth( m_resultsGrid->GetColLabelValue( col ), labelFont ),
                              textWidth( typical[col], cellFont ) );

        for( int row = 0; row < m_resultsGrid->GetNumberRows(); ++row )
        {
            width = std::max( width, textWidth( m_resultsGrid->GetCellValue( row, col ),
                                                m_resultsGrid->GetCellFont( row, col ) ) );
        }

        m_minColWidths[col] = width + padding;
        m_resultsGrid->SetColSize( col, m_minColWidths[col] );
    }

    // Pin the minimum size to the unstretched columns.  Otherwise the grid's best size would follow
    // the stretched columns and the page could never be made narrower again.
    m_resultsGrid->InvalidateBestSize();
    m_resultsGrid->SetMinSize( m_resultsGrid->GetBestSize() );
    Layout();

    stretchColumns();
}


void PANEL_R_DIVIDER::stretchColumns()
{
    int cols = m_resultsGrid->GetNumberCols();

    if( static_cast<int>( m_minColWidths.size() ) != cols )
        return;

    int extra = m_resultsGrid->GetClientSize().x - m_resultsGrid->GetRowLabelSize();

    for( int width : m_minColWidths )
        extra -= width;

    extra = std::max( extra, 0 );

    // Share the spare width equally; the last column takes the rounding remainder
    for( int col = 0; col < cols; ++col )
    {
        int share = ( col == cols - 1 ) ? extra - ( extra / cols ) * ( cols - 1 ) : extra / cols;

        m_resultsGrid->SetColSize( col, m_minColWidths[col] + share );
    }
}


void PANEL_R_DIVIDER::clearResults()
{
    m_resultsGrid->ClearGrid();

    // Undo the bold of the best results
    for( int row = 0; row < m_resultsGrid->GetNumberRows(); ++row )
    {
        for( int col = 0; col < m_resultsGrid->GetNumberCols(); ++col )
            m_resultsGrid->SetCellFont( row, col, m_resultsGrid->GetDefaultCellFont() );
    }
}
