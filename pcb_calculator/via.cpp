/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 Kicad Developers, see change_log.txt for contributors.
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

/* All calculations are based on this [1] online calculator:
 *
 * References:
 *
 * [1]: The CircuitCalculator.com Blog - PCB Via Calculator
 * http://circuitcalculator.com/wordpress/2006/03/12/pcb-via-calculator/
 *
 * [2]: Constructing Your Power Supply - Layout Considerations
 * https://www.ti.com/seclit/ml/slup230/slup230.pdf
 *
 * [3]: Current Carrying Capacity of Vias - Some Conceptual Observations
 * https://www.ultracad.com/articles/viacurrents.pdf
 *
 * [4]: IPC-2221A - Generic Standard on Printed Board Design
 * http://www.sphere.bc.ca/class/downloads/ipc_2221a-pcb%20standards.pdf
 *
 * [5]: Copper - online catalogue source - Goodfellow
 * http://www.goodfellow.com/E/Copper.html
 *
 * [6]: Thermal Conductivity of Metals, Metallic Elements and Alloys
 * https://www.engineeringtoolbox.com/thermal-conductivity-metals-d_858.html
 *
 * [7]: Johnson & Graham, High Speed Digital Design: A Handbook of Black Magic
 */

#include <cmath>
#include <wx/wx.h>
#include <wx/config.h>

#include <pcb_calculator_frame_base.h>

#include <pcb_calculator.h>
#include <UnitSelector.h>
#include <units_scales.h>

extern double DoubleFromString( const wxString& TextValue );

// Key words to read/write some parameters in config:
#define KEYWORD_VS_HOLE_DIA           wxT( "VS_Hole_Dia" )
#define KEYWORD_VS_HOLE_DIA_UNIT      wxT( "VS_Hole_Dia_Unit" )
#define KEYWORD_VS_THICKNESS          wxT( "VS_Plating_Thickness" )
#define KEYWORD_VS_THICKNESS_UNIT     wxT( "VS_Plating_Thickness_Unit" )
#define KEYWORD_VS_LENGTH             wxT( "VS_Via_Length" )
#define KEYWORD_VS_LENGTH_UNIT        wxT( "VS_Via_Length_Unit" )
#define KEYWORD_VS_PAD_DIA            wxT( "VS_Pad_Dia" )
#define KEYWORD_VS_PAD_DIA_UNIT       wxT( "VS_Pad_Dia_Unit" )
#define KEYWORD_VS_CLEARANCE_DIA      wxT( "VS_Clearance_Dia" )
#define KEYWORD_VS_CLEARANCE_DIA_UNIT wxT( "VS_Clearance_Dia_Unit" )
#define KEYWORD_VS_PCB_THICKNESS      wxT( "VS_PCB_Thickness" )
#define KEYWORD_VS_PCB_THICKNESS_UNIT wxT( "VS_PCB_Thickness_Unit" )
#define KEYWORD_VS_CH_IMPEDANCE       wxT( "VS_Characteristic_Impedance" )
#define KEYWORD_VS_CH_IMPEDANCE_UNIT  wxT( "VS_Characteristic_Impedance_Unit" )
#define KEYWORD_VS_CURRENT            wxT( "VS_Current" )
#define KEYWORD_VS_RESISTIVITY        wxT( "VS_Resistivity" )
#define KEYWORD_VS_PERMITTIVITY       wxT( "VS_Permittivity" )
#define KEYWORD_VS_TEMP_DIFF          wxT( "VS_Temperature_Differential" )
#define KEYWORD_VS_PULSE_RISE_TIME    wxT( "VS_Pulse_Rise_Time" )

/**
 * Shows a list of current relative dielectric constant(Er)
 * and set the selected value in main dialog frame
 */
void PCB_CALCULATOR_FRAME::OnViaEpsilonR_Button( wxCommandEvent& event )
{
    wxArrayString list;

    // EpsilonR ( relative dielectric constant) list
    list.Add( wxT( "4.5  FR4" ) );
    list.Add( wxT( "9.8  alumina (Al2O3)" ) );
    list.Add( wxT( "3.78  fused quartz" ) );
    list.Add( wxT( "3.38  RO4003" ) );
    list.Add( wxT( "2.2  RT/duroid 5880" ) );
    list.Add( wxT( "10.2  RT/duroid 6010LM" ) );
    list.Add( wxT( "2.1  teflon (PTFE)" ) );
    list.Add( wxT( "4.0  PVC" ) );
    list.Add( wxT( "2.3  PE" ) );
    list.Add( wxT( "6.6  beryllia (BeO)" ) );
    list.Add( wxT( "8.7  aluminum nitride" ) );
    list.Add( wxT( "11.9  silicon" ) );
    list.Add( wxT( "12.9  GaAs" ) );

    wxString value = wxGetSingleChoice( wxEmptyString,
            _("Relative Dielectric Constants"), list).BeforeFirst( ' ' );
    if( ! value.IsEmpty() )
        m_textCtrlPlatingPermittivity->SetValue( value );
}

/**
 * Shows a list of current Specific resistance list (rho)
 * and set the selected value in main dialog frame
 */
void PCB_CALCULATOR_FRAME::OnViaRho_Button( wxCommandEvent& event )
{
    wxArrayString list;

    // Specific resistance list in ohms*meters (rho):
    list.Clear();
    list.Add( wxT( "2.4e-8  gold" ) );
    list.Add( wxT( "1.72e-8  copper" ) );
    list.Add( wxT( "1.62e-8  silver" ) );
    list.Add( wxT( "12.4e-8  tin" ) );
    list.Add( wxT( "10.5e-8  platinum" ) );
    list.Add( wxT( "2.62e-8  aluminum" ) );
    list.Add( wxT( "6.9e-8  nickel" ) );
    list.Add( wxT( "3.9e-8  brass (66Cu 34Zn)" ) );
    list.Add( wxT( "9.71e-8  iron" ) );
    list.Add( wxT( "6.0e-8  zinc" ) );

    wxString value = wxGetSingleChoice( wxEmptyString,
            _("Specific Resistance"), list).BeforeFirst( ' ' );
    if( ! value.IsEmpty() )
        m_textCtrlPlatingResistivity->SetValue( value );
}

void PCB_CALCULATOR_FRAME::VS_Init( wxConfigBase* aCfg )
{
    int      tmp;
    wxString msg;

    // Read parameter values
    aCfg->Read( KEYWORD_VS_HOLE_DIA,                &msg, wxT( "18" ) );
    m_textCtrlHoleDia->SetValue( msg );
    aCfg->Read( KEYWORD_VS_HOLE_DIA_UNIT,           &tmp, 0 );
    m_choiceHoleDia->SetSelection( tmp );
    aCfg->Read( KEYWORD_VS_THICKNESS,               &msg, wxT( "1" ) );
    m_textCtrlPlatingThickness->SetValue( msg );
    aCfg->Read( KEYWORD_VS_THICKNESS_UNIT,          &tmp, 0 );
    m_choicePlatingThickness->SetSelection( tmp );
    aCfg->Read( KEYWORD_VS_LENGTH,                  &msg, wxT( "60" ) );
    m_textCtrlViaLength->SetValue( msg );
    aCfg->Read( KEYWORD_VS_LENGTH_UNIT,             &tmp, 0 );
    m_choiceViaLength->SetSelection( tmp );
    aCfg->Read( KEYWORD_VS_PAD_DIA,                 &msg, wxT( "22" ) );
    m_textCtrlViaPadDia->SetValue( msg );
    aCfg->Read( KEYWORD_VS_PAD_DIA_UNIT,            &tmp, 0 );
    m_choiceViaPadDia->SetSelection( tmp );
    aCfg->Read( KEYWORD_VS_CLEARANCE_DIA,           &msg, wxT( "25" ) );
    m_textCtrlClearanceDia->SetValue( msg );
    aCfg->Read( KEYWORD_VS_CLEARANCE_DIA_UNIT,      &tmp, 0 );
    m_choiceClearanceDia->SetSelection( tmp );
    aCfg->Read( KEYWORD_VS_PCB_THICKNESS,           &msg, wxT( "70" ) );
    m_textCtrlBoardThickness->SetValue( msg );
    aCfg->Read( KEYWORD_VS_PCB_THICKNESS_UNIT,      &tmp, 0 );
    m_choiceBoardThickness->SetSelection( tmp );
    aCfg->Read( KEYWORD_VS_CH_IMPEDANCE,            &msg, wxT( "50" ) );
    m_textCtrlImpedance->SetValue( msg );
    aCfg->Read( KEYWORD_VS_CH_IMPEDANCE_UNIT ,      &tmp, 0 );
    m_choiceImpedance->SetSelection( tmp );
    aCfg->Read( KEYWORD_VS_CURRENT,                 &msg, wxT( "1" ) );
    m_textCtrlAppliedCurrent->SetValue( msg );
    aCfg->Read( KEYWORD_VS_RESISTIVITY,             &msg, wxT( "1.72e-8" ) );
    m_textCtrlPlatingResistivity->SetValue( msg );
    aCfg->Read( KEYWORD_VS_PERMITTIVITY,            &msg, wxT( "4.5" ) );
    m_textCtrlPlatingPermittivity->SetValue( msg );
    aCfg->Read( KEYWORD_VS_TEMP_DIFF,               &msg, wxT( "10" ) );
    m_textCtrlTemperatureDiff->SetValue( msg );
    aCfg->Read( KEYWORD_VS_PULSE_RISE_TIME,         &msg, wxT( "1" ) );
    m_textCtrlRiseTime->SetValue( msg );
}

void PCB_CALCULATOR_FRAME::VS_WriteConfig( wxConfigBase* aCfg )
{
    // Save current parameters values in config
    aCfg->Write( KEYWORD_VS_HOLE_DIA,               m_textCtrlHoleDia->GetValue() );
    aCfg->Write( KEYWORD_VS_HOLE_DIA_UNIT,          m_choiceHoleDia->GetSelection() );
    aCfg->Write( KEYWORD_VS_THICKNESS,              m_textCtrlPlatingThickness->GetValue() );
    aCfg->Write( KEYWORD_VS_THICKNESS_UNIT,         m_choicePlatingThickness->GetSelection() );
    aCfg->Write( KEYWORD_VS_LENGTH,                 m_textCtrlViaLength->GetValue() );
    aCfg->Write( KEYWORD_VS_LENGTH_UNIT,            m_choiceViaLength->GetSelection() );
    aCfg->Write( KEYWORD_VS_PAD_DIA,                m_textCtrlViaPadDia->GetValue() );
    aCfg->Write( KEYWORD_VS_PAD_DIA_UNIT,           m_choiceViaPadDia->GetSelection() );
    aCfg->Write( KEYWORD_VS_CLEARANCE_DIA,          m_textCtrlClearanceDia->GetValue() );
    aCfg->Write( KEYWORD_VS_CLEARANCE_DIA_UNIT,     m_choiceClearanceDia->GetSelection() );
    aCfg->Write( KEYWORD_VS_PCB_THICKNESS,          m_textCtrlBoardThickness->GetValue() );
    aCfg->Write( KEYWORD_VS_PCB_THICKNESS_UNIT,     m_choiceBoardThickness->GetSelection() );
    aCfg->Write( KEYWORD_VS_CH_IMPEDANCE,           m_textCtrlImpedance->GetValue() );
    aCfg->Write( KEYWORD_VS_CH_IMPEDANCE_UNIT,      m_choiceImpedance->GetSelection() );
    aCfg->Write( KEYWORD_VS_CURRENT,                m_textCtrlAppliedCurrent->GetValue() );
    aCfg->Write( KEYWORD_VS_RESISTIVITY,            m_textCtrlPlatingResistivity->GetValue() );
    aCfg->Write( KEYWORD_VS_PERMITTIVITY,           m_textCtrlPlatingPermittivity->GetValue() );
    aCfg->Write( KEYWORD_VS_TEMP_DIFF,              m_textCtrlTemperatureDiff->GetValue() );
    aCfg->Write( KEYWORD_VS_PULSE_RISE_TIME,        m_textCtrlRiseTime->GetValue() );
}

void PCB_CALCULATOR_FRAME::OnViaCalculate( wxCommandEvent& event )
{
    // Load parameters
    double finishedHoleDia     = std::abs( DoubleFromString( m_textCtrlHoleDia->GetValue() ) );
    double platingThickness    = std::abs( DoubleFromString( m_textCtrlPlatingThickness->GetValue() ) );
    double viaLength           = std::abs( DoubleFromString( m_textCtrlViaLength->GetValue() ) );
    double padDia              = std::abs( DoubleFromString( m_textCtrlViaPadDia->GetValue() ) );
    double clearanceDia        = std::abs( DoubleFromString( m_textCtrlClearanceDia->GetValue() ) );
    double pcbThickness        = std::abs( DoubleFromString( m_textCtrlBoardThickness->GetValue() ) );
    double charImpedance       = std::abs( DoubleFromString( m_textCtrlImpedance->GetValue() ) );
    double appliedCurrent      = std::abs( DoubleFromString( m_textCtrlAppliedCurrent->GetValue() ) );
    double platingResistivity  = std::abs( DoubleFromString( m_textCtrlPlatingResistivity->GetValue() ) );
    double relativePermitivity = std::abs( DoubleFromString( m_textCtrlPlatingPermittivity->GetValue() ) );
    double temperatureDiff     = std::abs( DoubleFromString( m_textCtrlTemperatureDiff->GetValue() ) );
    double pulseRiseTime       = std::abs( DoubleFromString( m_textCtrlRiseTime->GetValue() ) );

    // Normalize units
    finishedHoleDia    *= m_choiceHoleDia->GetUnitScale();
    platingThickness   *= m_choicePlatingThickness->GetUnitScale();
    viaLength          *= m_choiceViaLength->GetUnitScale();
    padDia             *= m_choiceViaPadDia->GetUnitScale();
    clearanceDia       *= m_choiceClearanceDia->GetUnitScale();
    pcbThickness       *= m_choiceBoardThickness->GetUnitScale();
    charImpedance      *= m_choiceImpedance->GetUnitScale();
    platingResistivity  = platingResistivity / 100; // Ohm-cm to Ohm-m

    // Calculate cross-sectional area of the via's cylindrical structure [3]
    double area = M_PI * (finishedHoleDia + platingThickness) * platingThickness; // m^2

    double viaResistance = platingResistivity * viaLength / area; // Ohms

    // Using thermal resistivity value 2.49e-3 meter-Kelvin/Watt, equivalent to
    // thermal conductivity of 401 Watt/(meter-Kelvin) [5][6]
    const double thermalResistivity = 2.49e-3; // m K/W
    double thermalResistance = thermalResistivity * viaLength / area; // deg C/W

    double voltageDrop = appliedCurrent * viaResistance;

    double powerLoss = appliedCurrent * voltageDrop;

    // Estimate current carrying capacity of the via
    // See comment #17 in [1] for a brief discussion on the formula
    // This formula from IPC-2221 [4] is also used in the Track Width calculator
    area /= pow( UNIT_MIL, 2 ); // m^2 to mil^2
    const double k = 0.048;
    const double b = 0.44;
    const double c = 0.725;
    double estimatedAmpacity = k * pow( temperatureDiff, b ) * pow( area, c );

    // Equation 7.6 in [7]
    double capacitance = 55.51 * relativePermitivity * viaLength * padDia;
    capacitance /= clearanceDia - padDia;

    // Equation 7.8 in [7]
    double timeDegradation = 2.2 * capacitance * charImpedance / 2;

    // Equation 7.9 in [7]
    double inductance = 200 * viaLength;
    inductance *= log( 4 * viaLength / finishedHoleDia ) + 1;

    // Equation 7.11 in [7]
    double reactance = M_PI * inductance / pulseRiseTime;

    // Update the display
    VSDisplayValues( viaResistance, voltageDrop, powerLoss, estimatedAmpacity,
        thermalResistance, capacitance, timeDegradation, inductance, reactance );
}

void PCB_CALCULATOR_FRAME::VSDisplayValues( double aViaResistance, double aVoltageDrop,
        double aPowerLoss, double aEstimatedAmpacity, double aThermalResistance,
        double aCapacitance, double aTimeDegradation, double aInductance, double aReactance )
{
    wxString msg;

    msg.Printf( wxT( "%g" ), aViaResistance );
    m_ViaResistance->SetLabel( msg );

    msg.Printf( wxT( "%g" ), aVoltageDrop );
    m_ViaVoltageDrop->SetLabel( msg );

    msg.Printf( wxT( "%g" ), aPowerLoss );
    m_ViaPowerLoss->SetLabel( msg );

    msg.Printf( wxT( "%g" ), aEstimatedAmpacity );
    m_ViaAmpacity->SetLabel( msg );

    msg.Printf( wxT( "%g" ), aThermalResistance );
    m_ViaThermalResistance->SetLabel( msg );

    msg.Printf( wxT( "%g" ), aCapacitance );
    m_ViaCapacitance->SetLabel( msg );

    msg.Printf( wxT( "%g" ), aTimeDegradation );
    m_RiseTimeOutput->SetLabel( msg );

    msg.Printf( wxT( "%g" ), aInductance );
    m_Inductance->SetLabel( msg );

    msg.Printf( wxT( "%g" ), aReactance );
    m_Reactance->SetLabel( msg );
}
