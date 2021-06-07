/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 Kicad Developers, see AUTHORS.txt for contributors.
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
#include <wx/choicdlg.h>

#include <kiface_i.h>

#include "attenuators/attenuator_classes.h"
#include "common_data.h"
#include "class_regulator_data.h"
#include "pcb_calculator_frame.h"
#include "pcb_calculator_settings.h"
#include "units_scales.h"

extern double DoubleFromString( const wxString& TextValue );

/**
 * Shows a list of current relative dielectric constant(Er)
 * and set the selected value in main dialog frame
 */
void PCB_CALCULATOR_FRAME::OnViaEpsilonR_Button( wxCommandEvent& event )
{
    wxArrayString list = StandardRelativeDielectricConstantList();

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
    wxArrayString list = StandardResistivityList();

    wxString value = wxGetSingleChoice( wxEmptyString,
            _("Electrical Resistivity in Ohm*m"), list).BeforeFirst( ' ' );
    if( ! value.IsEmpty() )
        m_textCtrlPlatingResistivity->SetValue( value );
}


void PCB_CALCULATOR_FRAME::onUpdateViaCalcErrorText( wxUpdateUIEvent& event )
{
    // Update the Error message if a via has a external diameter
    // bigger than the clearance area diameter
    // (therefore the via is inside a copper zone and some parameters cannot be calculated)
    double clearanceDia = std::abs( DoubleFromString( m_textCtrlClearanceDia->GetValue() ) );
    clearanceDia *= m_choiceClearanceDia->GetUnitScale();
    double padDia = std::abs( DoubleFromString( m_textCtrlViaPadDia->GetValue() ) );
    padDia *= m_choiceViaPadDia->GetUnitScale();

    m_staticTextWarning->Show( clearanceDia <= padDia );
}


void PCB_CALCULATOR_FRAME::OnViaResetButtonClick( wxCommandEvent& event )
{
    #define DEFAULT_UNIT_SEL_MM 0
    #define DEFAULT_UNIT_SEL_OHM 0

    m_textCtrlHoleDia->SetValue( wxString::Format( "%g", 0.4 ) );
    m_choiceHoleDia->SetSelection( DEFAULT_UNIT_SEL_MM );
    m_textCtrlPlatingThickness->SetValue( wxString::Format( "%g", 0.035 ) );
    m_choicePlatingThickness->SetSelection( DEFAULT_UNIT_SEL_MM );
    m_textCtrlViaLength->SetValue( wxString::Format( "%g", 1.6 ) );
    m_choiceViaLength->SetSelection( DEFAULT_UNIT_SEL_MM );
    m_textCtrlViaPadDia->SetValue( wxString::Format( "%g", 0.6 ) );
    m_choiceViaPadDia->SetSelection( DEFAULT_UNIT_SEL_MM );
    m_textCtrlClearanceDia->SetValue( wxString::Format( "%g", 1.0 ) );
    m_choiceClearanceDia->SetSelection( DEFAULT_UNIT_SEL_MM );
    m_textCtrlImpedance->SetValue( wxString::Format( "%g", 50.0 ) );
    m_choiceImpedance->SetSelection( DEFAULT_UNIT_SEL_OHM );
    m_textCtrlAppliedCurrent->SetValue( wxString::Format( "%g", 1.0 ) );
    m_textCtrlPlatingResistivity->SetValue( wxString::Format( "%g", 1.72e-8 ) );
    m_textCtrlPlatingPermittivity->SetValue( wxString::Format( "%g", 4.5 ) );
    m_textCtrlTemperatureDiff->SetValue( wxString::Format( "%g", 10.0 ) );
    m_textCtrlRiseTime->SetValue( wxString::Format( "%g", 1.0 ) );
}


void PCB_CALCULATOR_FRAME::initViaSizePanel()
{
    auto cfg = static_cast<PCB_CALCULATOR_SETTINGS*>( Kiface().KifaceSettings() );

    m_textCtrlHoleDia->SetValue( cfg->m_ViaSize.hole_diameter );
    m_choiceHoleDia->SetSelection( cfg->m_ViaSize.hole_diameter_units );

    m_textCtrlPlatingThickness->SetValue( cfg->m_ViaSize.thickness );
    m_choicePlatingThickness->SetSelection( cfg->m_ViaSize.thickness_units );

    m_textCtrlViaLength->SetValue( cfg->m_ViaSize.length );
    m_choiceViaLength->SetSelection( cfg->m_ViaSize.length_units );

    m_textCtrlViaPadDia->SetValue( cfg->m_ViaSize.pad_diameter );
    m_choiceViaPadDia->SetSelection( cfg->m_ViaSize.pad_diameter_units );

    m_textCtrlClearanceDia->SetValue( cfg->m_ViaSize.clearance_diameter );
    m_choiceClearanceDia->SetSelection( cfg->m_ViaSize.clearance_diameter_units );

    m_textCtrlImpedance->SetValue( cfg->m_ViaSize.characteristic_impedance );
    m_choiceImpedance->SetSelection( cfg->m_ViaSize.characteristic_impedance_units );

    m_textCtrlAppliedCurrent->SetValue( cfg->m_ViaSize.applied_current );
    m_textCtrlPlatingResistivity->SetValue( cfg->m_ViaSize.plating_resistivity );
    m_textCtrlPlatingPermittivity->SetValue( cfg->m_ViaSize.permittivity );
    m_textCtrlTemperatureDiff->SetValue( cfg->m_ViaSize.temp_rise );
    m_textCtrlRiseTime->SetValue( cfg->m_ViaSize.pulse_rise_time );
}


void PCB_CALCULATOR_FRAME::writeViaSizeConfig()
{
    auto cfg = static_cast<PCB_CALCULATOR_SETTINGS*>( Kiface().KifaceSettings() );

    cfg->m_ViaSize.hole_diameter                  = m_textCtrlHoleDia->GetValue();
    cfg->m_ViaSize.hole_diameter_units            = m_choiceHoleDia->GetSelection();
    cfg->m_ViaSize.thickness                      = m_textCtrlPlatingThickness->GetValue();
    cfg->m_ViaSize.thickness_units                = m_choicePlatingThickness->GetSelection();
    cfg->m_ViaSize.length                         = m_textCtrlViaLength->GetValue();
    cfg->m_ViaSize.length_units                   = m_choiceViaLength->GetSelection();
    cfg->m_ViaSize.pad_diameter                   = m_textCtrlViaPadDia->GetValue();
    cfg->m_ViaSize.pad_diameter_units             = m_choiceViaPadDia->GetSelection();
    cfg->m_ViaSize.clearance_diameter             = m_textCtrlClearanceDia->GetValue();
    cfg->m_ViaSize.clearance_diameter_units       = m_choiceClearanceDia->GetSelection();
    cfg->m_ViaSize.characteristic_impedance       = m_textCtrlImpedance->GetValue();
    cfg->m_ViaSize.characteristic_impedance_units = m_choiceImpedance->GetSelection();
    cfg->m_ViaSize.applied_current                = m_textCtrlAppliedCurrent->GetValue();
    cfg->m_ViaSize.plating_resistivity            = m_textCtrlPlatingResistivity->GetValue();
    cfg->m_ViaSize.permittivity                   = m_textCtrlPlatingPermittivity->GetValue();
    cfg->m_ViaSize.temp_rise                      = m_textCtrlTemperatureDiff->GetValue();
    cfg->m_ViaSize.pulse_rise_time                = m_textCtrlRiseTime->GetValue();
}


void PCB_CALCULATOR_FRAME::OnViaCalculate( wxCommandEvent& event )
{
    // Load parameters
    double finishedHoleDia     = std::abs( DoubleFromString( m_textCtrlHoleDia->GetValue() ) );
    double platingThickness    = std::abs( DoubleFromString( m_textCtrlPlatingThickness->GetValue() ) );
    double viaLength           = std::abs( DoubleFromString( m_textCtrlViaLength->GetValue() ) );
    double padDia              = std::abs( DoubleFromString( m_textCtrlViaPadDia->GetValue() ) );
    double clearanceDia        = std::abs( DoubleFromString( m_textCtrlClearanceDia->GetValue() ) );
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
    charImpedance      *= m_choiceImpedance->GetUnitScale();
    // platingResistivity is ok: it is in Ohm*m in tables

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

    msg.Printf( "%g", aViaResistance );
    m_ViaResistance->SetLabel( msg );

    msg.Printf( "%g", aVoltageDrop );
    m_ViaVoltageDrop->SetLabel( msg );

    msg.Printf( "%g", aPowerLoss );
    m_ViaPowerLoss->SetLabel( msg );

    msg.Printf( "%g", aEstimatedAmpacity );
    m_ViaAmpacity->SetLabel( msg );

    msg.Printf( "%g", aThermalResistance );
    m_ViaThermalResistance->SetLabel( msg );

    msg.Printf( "%g", aCapacitance );
    m_ViaCapacitance->SetLabel( msg );

    msg.Printf( "%g", aTimeDegradation );
    m_RiseTimeOutput->SetLabel( msg );

    msg.Printf( "%g", aInductance );
    m_Inductance->SetLabel( msg );

    msg.Printf( "%g", aReactance );
    m_Reactance->SetLabel( msg );
}
