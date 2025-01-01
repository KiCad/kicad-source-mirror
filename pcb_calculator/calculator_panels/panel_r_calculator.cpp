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

#include <calculator_panels/panel_r_calculator.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>
#include <wx/msgdlg.h>
#include <eseries.h>
#include "resistor_substitution_utils.h"

#include <i18n_utility.h> // For _HKI definition in r_calculator_help.h
wxString r_calculator_help =
#include "r_calculator_help.h"

/* RES_EQUIV_CALC class considers resistor values from a limited range
 * and only combinations of up to 4 resistors, so target values less than
 * parallel combination of minimum resistors or greater than serial combination
 * of maximum resistors cannot be reasonably looked for
 */
static const double min_target_value = static_cast<double>(RES_EQUIV_CALC_FIRST_VALUE) / 4;
static const double max_target_value = static_cast<double>(RES_EQUIV_CALC_LAST_VALUE) * 4;

extern double DoubleFromString( const wxString& TextValue );

PANEL_R_CALCULATOR::PANEL_R_CALCULATOR( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                        const wxSize& size, long style, const wxString& name ) :
        PANEL_R_CALCULATOR_BASE( parent, id, pos, size, style, name )
{
    m_reqResUnits->SetLabel( wxT( "kΩ" ) );
    m_exclude1Units->SetLabel( wxT( "kΩ" ) );
    m_exclude2Units->SetLabel( wxT( "kΩ" ) );

    wxSize minSize = m_ResRequired->GetMinSize();
    int    minWidth = m_ResRequired->GetTextExtent( wxT( "XXX.XXX" ) ).GetWidth();

    m_ResRequired->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );
    m_ResExclude1->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );
    m_ResExclude2->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );

    minSize = m_ESeriesError2R->GetMinSize();
    minWidth = m_ESeriesError2R->GetTextExtent( wxT( "XX" ) + _( "Exact" ) ).GetWidth();

    m_ESeriesError2R->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );
    m_ESeriesError3R->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );
    m_ESeriesError4R->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );

    // show markdown formula explanation in lower help panel
    wxString msg;
    ConvertMarkdown2Html( wxGetTranslation( r_calculator_help ), msg );
    m_panelESeriesHelp->SetPage( msg );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_R_CALCULATOR::~PANEL_R_CALCULATOR()
{
}


void PANEL_R_CALCULATOR::ThemeChanged()
{
    // Update the HTML window with the help text
    m_panelESeriesHelp->ThemeChanged();
}


void PANEL_R_CALCULATOR::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
}


void PANEL_R_CALCULATOR::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
}


void PANEL_R_CALCULATOR::OnCalculateESeries( wxCommandEvent& event )
{
    double reqr = 1000 * DoubleFromString( m_ResRequired->GetValue() );

    if( std::isnan( reqr ) || reqr < min_target_value || reqr > max_target_value )
    {
        wxMessageBox( wxString::Format( _( "Incorrect required resistance value: %s" ),
                                        m_ResRequired->GetValue() ) );
        return;
    }

    wxBusyCursor busyCursor; // As long as this variable exists, the cursor will be 'busy'

    m_eSeries.NewCalc( reqr ); // assume all values available
    /*
     * Exclude itself. For the case, a value from the available series is found as required value,
     * the calculator assumes this value needs a replacement for the reason of being not available.
     * Two further exclude values can be entered to exclude and are skipped as not being available.
     * All values entered in KiloOhms are converted to Ohm for internal calculation
     */
    m_eSeries.Exclude( reqr );
    m_eSeries.Exclude( 1000 * DoubleFromString( m_ResExclude1->GetValue() ) );
    m_eSeries.Exclude( 1000 * DoubleFromString( m_ResExclude2->GetValue() ) );

    try
    {
        m_eSeries.Calculate();
    }
    catch( const std::exception& exc )
    {
        wxMessageBox( wxString::Format( "Internal error: %s", exc.what() ) );
        return;
    }

    auto showResult = [reqr]( const std::optional<RESISTANCE>& aResult, wxTextCtrl* aFormulaField,
                              wxTextCtrl* aErrorField )
    {
        wxString fs, es; // formula and error string

        if( aResult ) // if value is present
        {
            fs = aResult->name;
            double sol = aResult->value;
            double error = ( sol / reqr - 1 ) * 100; // relative error in percent

            if( std::abs( error ) < epsilon )
                es = _( "Exact" );
            else if( std::abs( error ) < 0.01 )
                es.Printf( "<%.2f", 0.01 );
            else
                es.Printf( "%+.2f", error );
        }
        else
        {
            fs = _( "Not worth using" );
            es = wxEmptyString;
        }

        aFormulaField->SetValue( fs );
        aErrorField->SetValue( es );
    };

    showResult( m_eSeries.GetResults()[RES_EQUIV_CALC::S2R], m_ESeries_Sol2R, m_ESeriesError2R );
    showResult( m_eSeries.GetResults()[RES_EQUIV_CALC::S3R], m_ESeries_Sol3R, m_ESeriesError3R );
    showResult( m_eSeries.GetResults()[RES_EQUIV_CALC::S4R], m_ESeries_Sol4R, m_ESeriesError4R );
}


void PANEL_R_CALCULATOR::OnESeriesSelection( wxCommandEvent& event )
{
    if( event.GetEventObject() == m_e1 )
        m_eSeries.SetSeries( ESERIES::E1 );
    else if( event.GetEventObject() == m_e3 )
        m_eSeries.SetSeries( ESERIES::E3 );
    else if( event.GetEventObject() == m_e12 )
        m_eSeries.SetSeries( ESERIES::E12 );
    else if( event.GetEventObject() == m_e24 )
        m_eSeries.SetSeries( ESERIES::E24 );
    else
        m_eSeries.SetSeries( ESERIES::E6 );
}
