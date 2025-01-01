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

#include <bitmaps.h>
#include <calculator_panels/panel_via_size.h>
#include <common_data.h>
#include <pcb_calculator_settings.h>
#include <units_scales.h>


extern double DoubleFromString( const wxString& TextValue );


PANEL_VIA_SIZE::PANEL_VIA_SIZE( wxWindow* parent, wxWindowID id,
                                const wxPoint& pos, const wxSize& size,
                                long style, const wxString& name ) :
        PANEL_VIA_SIZE_BASE( parent, id, pos, size, style, name )
{
    m_viaResistivityUnits->SetLabel( wxT( "Ω⋅m" ) );
    m_viaTempUnits->SetLabel( wxT( "°C" ) );
    m_viaResUnits->SetLabel( wxT( "Ω" ) );
    m_viaThermalResUnits->SetLabel( wxT( "°C/W" ) );
    m_viaReactanceUnits->SetLabel( wxT( "Ω" ) );

    m_viaBitmap->SetBitmap( KiBitmapBundle( BITMAPS::viacalc ) );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_VIA_SIZE::~PANEL_VIA_SIZE()
{
}


void PANEL_VIA_SIZE::ThemeChanged()
{
    m_viaBitmap->SetBitmap( KiBitmapBundle( BITMAPS::viacalc ) );
}


void PANEL_VIA_SIZE::OnViaEpsilonR_Button( wxCommandEvent& event )
{
    //Shows a list of current relative dielectric constant(Er) and select a value.
    wxArrayString list = StandardRelativeDielectricConstantList();

    wxString value = wxGetSingleChoice( wxEmptyString, _( "Relative Dielectric Constants" ), list )
                             .BeforeFirst( ' ' );

    if( !value.IsEmpty() )
        m_textCtrlPlatingPermittivity->SetValue( value );
}


void PANEL_VIA_SIZE::OnViaRho_Button( wxCommandEvent& event )
{
    wxArrayString list = StandardResistivityList();

    // Shows a list of current Specific resistance list (rho) and select a value
    wxString value = wxGetSingleChoice( wxEmptyString, _( "Electrical Resistivity in Ohm*m" ),
                                        list ).BeforeFirst( ' ' );

    if( !value.IsEmpty() )
        m_textCtrlPlatingResistivity->SetValue( value );
}


void PANEL_VIA_SIZE::onUpdateViaCalcErrorText( wxUpdateUIEvent& event )
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


void PANEL_VIA_SIZE::OnViaResetButtonClick( wxCommandEvent& event )
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


void PANEL_VIA_SIZE::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_textCtrlHoleDia->SetValue( aCfg->m_ViaSize.hole_diameter );
    m_choiceHoleDia->SetSelection( aCfg->m_ViaSize.hole_diameter_units );

    m_textCtrlPlatingThickness->SetValue( aCfg->m_ViaSize.thickness );
    m_choicePlatingThickness->SetSelection( aCfg->m_ViaSize.thickness_units );

    m_textCtrlViaLength->SetValue( aCfg->m_ViaSize.length );
    m_choiceViaLength->SetSelection( aCfg->m_ViaSize.length_units );

    m_textCtrlViaPadDia->SetValue( aCfg->m_ViaSize.pad_diameter );
    m_choiceViaPadDia->SetSelection( aCfg->m_ViaSize.pad_diameter_units );

    m_textCtrlClearanceDia->SetValue( aCfg->m_ViaSize.clearance_diameter );
    m_choiceClearanceDia->SetSelection( aCfg->m_ViaSize.clearance_diameter_units );

    m_textCtrlImpedance->SetValue( aCfg->m_ViaSize.characteristic_impedance );
    m_choiceImpedance->SetSelection( aCfg->m_ViaSize.characteristic_impedance_units );

    m_textCtrlAppliedCurrent->SetValue( aCfg->m_ViaSize.applied_current );
    m_textCtrlPlatingResistivity->SetValue( aCfg->m_ViaSize.plating_resistivity );
    m_textCtrlPlatingPermittivity->SetValue( aCfg->m_ViaSize.permittivity );
    m_textCtrlTemperatureDiff->SetValue( aCfg->m_ViaSize.temp_rise );
    m_textCtrlRiseTime->SetValue( aCfg->m_ViaSize.pulse_rise_time );
}


void PANEL_VIA_SIZE::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    aCfg->m_ViaSize.hole_diameter                  = m_textCtrlHoleDia->GetValue();
    aCfg->m_ViaSize.hole_diameter_units            = m_choiceHoleDia->GetSelection();
    aCfg->m_ViaSize.thickness                      = m_textCtrlPlatingThickness->GetValue();
    aCfg->m_ViaSize.thickness_units                = m_choicePlatingThickness->GetSelection();
    aCfg->m_ViaSize.length                         = m_textCtrlViaLength->GetValue();
    aCfg->m_ViaSize.length_units                   = m_choiceViaLength->GetSelection();
    aCfg->m_ViaSize.pad_diameter                   = m_textCtrlViaPadDia->GetValue();
    aCfg->m_ViaSize.pad_diameter_units             = m_choiceViaPadDia->GetSelection();
    aCfg->m_ViaSize.clearance_diameter             = m_textCtrlClearanceDia->GetValue();
    aCfg->m_ViaSize.clearance_diameter_units       = m_choiceClearanceDia->GetSelection();
    aCfg->m_ViaSize.characteristic_impedance       = m_textCtrlImpedance->GetValue();
    aCfg->m_ViaSize.characteristic_impedance_units = m_choiceImpedance->GetSelection();
    aCfg->m_ViaSize.applied_current                = m_textCtrlAppliedCurrent->GetValue();
    aCfg->m_ViaSize.plating_resistivity            = m_textCtrlPlatingResistivity->GetValue();
    aCfg->m_ViaSize.permittivity                   = m_textCtrlPlatingPermittivity->GetValue();
    aCfg->m_ViaSize.temp_rise                      = m_textCtrlTemperatureDiff->GetValue();
    aCfg->m_ViaSize.pulse_rise_time                = m_textCtrlRiseTime->GetValue();
}


void PANEL_VIA_SIZE::OnViaCalculate( wxCommandEvent& event )
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
    finishedHoleDia  *= m_choiceHoleDia->GetUnitScale();
    platingThickness *= m_choicePlatingThickness->GetUnitScale();
    viaLength        *= m_choiceViaLength->GetUnitScale();
    padDia           *= m_choiceViaPadDia->GetUnitScale();
    clearanceDia     *= m_choiceClearanceDia->GetUnitScale();
    charImpedance    *= m_choiceImpedance->GetUnitScale();
    // platingResistivity is ok: it is in Ohm*m in tables

    // Calculate cross-sectional area of the via's cylindrical structure [3]
    double area = M_PI * ( finishedHoleDia + platingThickness ) * platingThickness; // m^2

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

void PANEL_VIA_SIZE::VSDisplayValues( double aViaResistance, double aVoltageDrop, double aPowerLoss,
                                      double aEstimatedAmpacity, double aThermalResistance,
                                      double aCapacitance, double aTimeDegradation, double aInductance,
                                      double aReactance )
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
