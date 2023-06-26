/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 1992-2023 Kicad Developers, see AUTHORS.txt for contributors.
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
    double   reqr;   // required resistor stored in local copy
    double   error, err3 = 0;
    wxString es, fs; // error and formula strings

    wxBusyCursor dummy;

    reqr = ( 1000 * DoubleFromString( m_ResRequired->GetValue() ) );
    m_eSeries.SetRequiredValue( reqr ); // keep a local copy of required resistor value
    m_eSeries.NewCalc();                // assume all values available
    /*
     * Exclude itself. For the case, a value from the available series is found as required value,
     * the calculator assumes this value needs a replacement for the reason of being not available.
     * Two further exclude values can be entered to exclude and are skipped as not being available.
     * All values entered in KiloOhms are converted to Ohm for internal calculation
     */
    m_eSeries.Exclude( 1000 * DoubleFromString( m_ResRequired->GetValue() ) );
    m_eSeries.Exclude( 1000 * DoubleFromString( m_ResExclude1->GetValue() ) );
    m_eSeries.Exclude( 1000 * DoubleFromString( m_ResExclude2->GetValue() ) );

    try
    {
        m_eSeries.Calculate();
    }
    catch( std::out_of_range const& exc )
    {
        wxString msg;
        msg << "Internal error: " << exc.what();

        wxMessageBox( msg );
        return;
    }

    fs = m_eSeries.GetResults()[RES_EQUIV_CALC::S2R].e_name; // show 2R solution formula string
    m_ESeries_Sol2R->SetValue( fs );
    error = reqr
            + m_eSeries.GetResults()[RES_EQUIV_CALC::S2R].e_value; // absolute value of solution
    error = ( reqr / error - 1 ) * 100;                 // error in percent

    if( error )
    {
        if( std::abs( error ) < 0.01 )
            es.Printf( "<%.2f", 0.01 );
        else
            es.Printf( "%+.2f", error );
    }
    else
    {
        es = _( "Exact" );
    }

    m_ESeriesError2R->SetValue( es );                      // anyway show 2R error string

    if( m_eSeries.GetResults()[RES_EQUIV_CALC::S3R].e_use ) // if 3R solution available
    {
        err3 = reqr + m_eSeries.GetResults()[RES_EQUIV_CALC::S3R].e_value; // calculate the 3R
        err3 = ( reqr / err3 - 1 ) * 100;                  // error in percent

        if( err3 )
        {
            if( std::abs( err3 ) < 0.01 )
                es.Printf( "<%.2f", 0.01 );
            else
                es.Printf( "%+.2f", err3 );
        }
        else
        {
            es = _( "Exact" );
        }

        m_ESeriesError3R->SetValue( es ); // show 3R error string
        fs = m_eSeries.GetResults()[RES_EQUIV_CALC::S3R].e_name;
        m_ESeries_Sol3R->SetValue( fs );  // show 3R formula string
    }
    else                                  // nothing better than 2R found
    {
        fs = _( "Not worth using" );
        m_ESeries_Sol3R->SetValue( fs );
        m_ESeriesError3R->SetValue( wxEmptyString );
    }

    fs = wxEmptyString;

    if( m_eSeries.GetResults()[RES_EQUIV_CALC::S4R].e_use ) // show 4R solution if available
    {
        fs = m_eSeries.GetResults()[RES_EQUIV_CALC::S4R].e_name;

        error = reqr
                + m_eSeries.GetResults()[RES_EQUIV_CALC::S4R].e_value; // absolute value of solution
        error = ( reqr / error - 1 ) * 100;                 // error in percent

        if( error )
            es.Printf( "%+.2f", error );
        else
            es = _( "Exact" );

        m_ESeriesError4R->SetValue( es );
    }
    else // no 4R solution
    {
        fs = _( "Not worth using" );
        es = wxEmptyString;
        m_ESeriesError4R->SetValue( es );
    }

    m_ESeries_Sol4R->SetValue( fs );
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
