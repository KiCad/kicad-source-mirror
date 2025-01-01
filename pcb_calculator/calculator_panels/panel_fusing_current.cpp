/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
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

// See equation 9b in this paper:
// https://adam-research.de/pdfs/TRM_WhitePaper10_AdiabaticWire.pdf

// See equation 8
//https://scholar.google.com/scholar?hl=en&as_sdt=0%2C5&q=Fusing+of+wires+by+electrical+current&btnG=
#define ABS_ZERO ( -273.15 )

#include <calculator_panels/panel_fusing_current.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>

#include <widgets/unit_selector.h>

#include <i18n_utility.h> // For _HKI definition
wxString fusing_current_help =
#include "fusing_current_help.h"


PANEL_FUSING_CURRENT::PANEL_FUSING_CURRENT( wxWindow * parent, wxWindowID id,
                                            const wxPoint& pos, const wxSize& size,
                                            long style, const wxString& name ) :
PANEL_FUSING_CURRENT_BASE( parent, id, pos, size, style, name )
{
    m_ambientUnit->SetLabel( wxT( "°C" ) );
    m_meltingUnit->SetLabel( wxT( "°C" ) );

    // Set some defaults
    m_ambientValue->SetValue( wxString::Format( wxT( "%i" ), 25 ) );
    m_meltingValue->SetValue( wxString::Format( wxT( "%i" ), 1084 ) ); // Value for copper
    m_meltingValue->SetEditable( false ); // For now, this panel only works for copper.
    m_widthValue->SetValue( wxString::Format( wxT( "%f" ), 0.1 ) );
    m_thicknessValue->SetValue( wxString::Format( wxT( "%f" ), 0.035 ) );
    m_currentValue->SetValue( wxString::Format( wxT( "%f" ), 10.0 ) );
    m_timeValue->SetValue( wxString::Format( wxT( "%f" ), 0.01 ) );

    // show markdown formula explanation in lower help panel
    wxString msg;
    ConvertMarkdown2Html( wxGetTranslation( fusing_current_help ), msg );
    m_htmlHelp->SetPage( msg );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_FUSING_CURRENT::~PANEL_FUSING_CURRENT()
{
}


void PANEL_FUSING_CURRENT::ThemeChanged()
{
    // Update the HTML window with the help text
    m_htmlHelp->ThemeChanged();
}


void PANEL_FUSING_CURRENT::m_onCalculateClick( wxCommandEvent& event )
{
    double Tm, Ta, I, W, T, time;
    bool   valid_Tm, valid_Ta, valid_I, valid_W, valid_T, valid_time;

    valid_Tm   = m_meltingValue->GetValue().ToDouble( &Tm );
    valid_Ta   = m_ambientValue->GetValue().ToDouble( &Ta );
    valid_I    = m_currentValue->GetValue().ToDouble( &I );
    valid_W    = m_widthValue->GetValue().ToDouble( &W );
    valid_T    = m_thicknessValue->GetValue().ToDouble( &T );
    valid_time = m_timeValue->GetValue().ToDouble( &time );

    double scalingT, scalingW;

    scalingT = m_thicknessUnit->GetUnitScale();
    scalingW = m_widthUnit->GetUnitScale();
    T *= scalingT;
    W *= scalingW;

    valid_Tm   &= std::isfinite( Tm );
    valid_Ta   &= std::isfinite( Ta );
    valid_I    &= std::isfinite( I );
    valid_W    &= std::isfinite( W );
    valid_T    &= std::isfinite( T );
    valid_time &= std::isfinite( time );

    if( valid_Tm && valid_Ta )
    {
        valid_Tm &= ( Tm > Ta );
        valid_Ta &= ( Tm > Ta ) && ( Ta > ABS_ZERO );
    }

    valid_I    &= ( I > 0 );
    valid_W    &= ( W > 0 );
    valid_T    &= ( T > 0 );
    valid_time &= ( time > 0 );

    double A     = W * T;

    // The energy required for copper to change phase ( fusion ) is 13.05 kJ / mol.
    // Copper molar mass is 0.06355 kg/mol
    // => The copper energy required for the phase change is 205.35 kJ / kg

    double latentHeat = 205350.0;

    // The change in enthalpy is deltaH = deltaU + delta P * deltaV
    // with U the internal energy, P the pressure and V the volume
    // But for constant pressure, the change in enthalpy is simply the thermal energy

    // Copper specific heat energy is 0.385 kJ / kg / K.
    // The change in heat energy is then 0.385 kJ / kg per degree.

    double cp = 385; // Heat capacity in J / kg / K
    double deltaEnthalpy = ( Tm - Ta ) * cp;
    double density = 8940; // Density of copper to kilogram per cubic meter;
    double volumicEnergy = density * ( deltaEnthalpy + latentHeat );

    // Equation (3) is equivalent to :
    // VolumicEnergy * Volume = R * I^2 * t
    // If we consider the resistivity of copper instead of its resistance:
    // VolumicEnergy * Volume   = resistivity * length / Area  * I^2 * t
    // For a unit length:
    // VolumicEnergy * Area   = resistivity / Area  * I^2 * t
    // We can rewrite it as:
    // VolumicEnergy * ( Area / I )^2 / resistivity  =  t
    // coeff * ( Area / I ) ^2 = t    with coeff = VolumicEnergy / resistivity

    // Copper resistivity at 20C ( 293K ) is 1.72e-8 ohm m
    // Copper temperature coefficient is 0.00393 per degree

    double Ra = ( ( Ta - ABS_ZERO - 293 ) * 0.00393 + 1 ) * 1.72e-8;
    double Rm = ( ( Tm - ABS_ZERO - 293 ) * 0.00393 + 1 ) * 1.72e-8;

    // Let's consider the average resistivity
    double R = ( Rm + Ra ) / 2;
    double coeff = volumicEnergy / R;

    bool valid = valid_I && valid_W && valid_T && valid_Ta && valid_Tm && valid_time;

    if( m_widthRadio->GetValue() )
    {
        if( valid )
        {
            A = I / sqrt( coeff / time );
            W = A / T;
            m_widthValue->SetValue( wxString::Format( wxT( "%f" ), W / scalingW ) );
        }
        else
        {
            m_widthValue->SetValue( _( "Error" ) );
        }
    }
    else if( m_thicknessRadio->GetValue() )
    {
        if( valid )
        {
            A = I / sqrt( coeff / time );
            T = A / W;
            m_thicknessValue->SetValue( wxString::Format( wxT( "%f" ), T / scalingT ) );
        }
        else
        {
            m_thicknessValue->SetValue( _( "Error" ) );
        }
    }
    else if( m_currentRadio->GetValue() )
    {
        if( valid )
        {
            I = A * sqrt( coeff / time );
            m_currentValue->SetValue( wxString::Format( wxT( "%f" ), I ) );
        }
        else
        {
            m_currentValue->SetValue( _( "Error" ) );
        }
    }
    else if( m_timeRadio->GetValue() )
    {
        if( valid )
        {
            time = coeff * A * A / I / I;
            m_timeValue->SetValue( wxString::Format( wxT( "%f" ), time ) );
        }
        else
        {
            m_timeValue->SetValue( _( "Error" ) );
        }
    }
    else
    {
        // What happened ??? an extra radio button ?
    }

    // Now let's check the validity domain using the formula from the paper.
    // https://adam-research.de/pdfs/TRM_WhitePaper10_AdiabaticWire.pdf
    // We approximate the track with a circle having the same area.

    double r = sqrt( A / M_PI );              // radius in m;
    double epsilon = 5.67e-8;                 // Stefan-Boltzmann constant in W / ( m^2 K^4 )
    double sigma   = 0.5;                     // Surface radiative emissivity ( no unit )
    // sigma is according to paper, between polished and oxidized

    double tmKelvin = Tm - ABS_ZERO;
    double frad = 0.5 * ( tmKelvin + 293 ) * ( tmKelvin + 293 ) * ( tmKelvin + 293 );

    double tau = cp * density * r / ( epsilon * sigma * frad * 2 );

    if( 2 * time < tau )
    {
        m_comment->SetLabel( "" );
    }
    else
    {
        m_comment->SetLabel( _( "Current calculation is underestimated due to long fusing time."
                              ) );
    }
}