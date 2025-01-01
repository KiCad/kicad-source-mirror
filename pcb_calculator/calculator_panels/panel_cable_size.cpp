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
#include <wx/choicdlg.h>

#include <common_data.h>
#include <calculator_panels/panel_cable_size.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>
#include <widgets/unit_selector.h>

extern double DoubleFromString( const wxString& TextValue );

#define VACCUM_PERMEABILITY 1.256637e-6
#define RELATIVE_PERMEABILITY 1

// The default current density in ampere by mm2 (3A/mm2 is common to make transformers)
#define AMP_DENSITY_BY_MM2 3.0

CABLE_SIZE_ENTRY::CABLE_SIZE_ENTRY( const wxString& aName, double aRadius_meter ) :
        m_Name( aName ), m_Radius( aRadius_meter )
{
}


PANEL_CABLE_SIZE::PANEL_CABLE_SIZE( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                    const wxSize& size, long style, const wxString& name ) :
        PANEL_CABLE_SIZE_BASE( parent, id, pos, size, style, name )
{
    buildCableList();

    m_stUnitOhmMeter->SetLabel( wxT( "Ω⋅m" ) );
    m_stUnitDegC->SetLabel( wxT( "°C" ) );
    m_stUnitOhm->SetLabel( wxT( "Ω" ) );
    m_stUnitmmSq->SetLabel( wxT( "mm²" ) );
    m_stUnitAmp_mmSq->SetLabel( wxT( "A/mm²" ) );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );

    // Set internal state flags:
    m_updatingUI = false;
    m_updatingDiameter = false;
    m_updatingArea = false;
    m_updatingLinResistance = false;
    m_updatingFrequency = false;
    m_updatingAmpacity = false;
    m_updatingCurrent = false;
    m_updatingLength = false;
    m_updatingResistanceDc = false;
    m_updatingRVdrop = false;
    m_updatingPower = false;
    m_updatingConductorMaterialResitivity = false;

    m_imperial = false;

    // Initialize variables to a reasonable value (stored in normalized units)
    m_diameter = 0.001;     // i.e. 1 mm2
    m_conductorTemperature = 20;
    m_current = 1.0;
    m_length = 1.0;
    m_conductorMaterialResitivity = 1.72e-8;    //Initialized for copper
    m_conductorMaterialResitivityRef = 1.72e-8; //Initialized for copper at 20 deg C
    m_conductorMaterialThermalCoef = 3.93e-3;
    m_amp_by_mm2 = AMP_DENSITY_BY_MM2;

    updateAll( m_diameter / 2 );
}


PANEL_CABLE_SIZE::~PANEL_CABLE_SIZE()
{
}


void PANEL_CABLE_SIZE::buildCableList()
{
    m_entries.clear();
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG0000" ), 0.005842 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG000" ), 0.00520192 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG00" ), 0.00463296 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG0" ), 0.00412623 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG1" ), 0.00367411 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG2" ), 0.00327152 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG3" ), 0.00291338 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG4" ), 0.00259461 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG5" ), 0.00231013 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG6" ), 0.00205740 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG7" ), 0.00183261 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG8" ), 0.00163195 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG9" ), 0.00145288 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG10" ), 0.00129413 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG11" ), 0.00115189 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG12" ), 0.00102616 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG13" ), 0.0009144 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG14" ), 0.00081407 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG15" ), 0.00072517 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG16" ), 0.00064516 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG17" ), 0.00057531 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG18" ), 0.00051181 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG19" ), 0.00045593 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG20" ), 0.0004046 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG21" ), 0.00036195 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG22" ), 0.00032258 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG23" ), 0.00028702 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG24" ), 0.00025527 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG25" ), 0.00022773 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG26" ), 0.00020193 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG27" ), 0.00018034 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG28" ), 0.00016002 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG29" ), 0.00014351 ) );
    m_entries.emplace_back( CABLE_SIZE_ENTRY( _( "AWG30" ), 0.000127 ) );

    for( CABLE_SIZE_ENTRY entry : m_entries )
    {
        m_sizeChoice->Append( entry.m_Name );
    }
}


void PANEL_CABLE_SIZE::OnUpdateUnit( wxCommandEvent& aEvent )
{
    printAll();
}


void PANEL_CABLE_SIZE::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    aCfg->m_cableSize.diameterUnit = m_diameterUnit->GetSelection();
    aCfg->m_cableSize.linResUnit = m_linResistanceUnit->GetSelection();
    aCfg->m_cableSize.frequencyUnit = m_frequencyUnit->GetSelection();
    aCfg->m_cableSize.lengthUnit = m_lengthUnit->GetSelection();
    aCfg->m_cableSize.conductorMaterialResitivity = wxString( "" )
                                                    << m_conductorMaterialResitivityRef;
    aCfg->m_cableSize.conductorTemperature = m_conductorTempCtrl->GetValue();
    aCfg->m_cableSize.conductorThermalCoef = m_textCtrlConductorThermCoef->GetValue();
    aCfg->m_cableSize.currentDensityChoice = m_slCurrentDensity->GetValue();
}

#include <wx/log.h>
void PANEL_CABLE_SIZE::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_diameterUnit->SetSelection( aCfg->m_cableSize.diameterUnit );
    m_linResistanceUnit->SetSelection( aCfg->m_cableSize.linResUnit );
    m_frequencyUnit->SetSelection( aCfg->m_cableSize.frequencyUnit );
    m_lengthUnit->SetSelection( aCfg->m_cableSize.lengthUnit );
    m_textCtrlConductorResistivity->SetValue( aCfg->m_cableSize.conductorMaterialResitivity );
    m_conductorTempCtrl->SetValue( aCfg->m_cableSize.conductorTemperature );
    m_textCtrlConductorThermCoef->SetValue( aCfg->m_cableSize.conductorThermalCoef );

    int amp_per_mm2_choice = aCfg->m_cableSize.currentDensityChoice;

    // Ensure validity of amp_per_mm2_choice
    if( amp_per_mm2_choice < m_slCurrentDensity->GetMin()
            || amp_per_mm2_choice > m_slCurrentDensity->GetMax() )
    {
        amp_per_mm2_choice = AMP_DENSITY_BY_MM2;
    }

    m_slCurrentDensity->SetValue( amp_per_mm2_choice );
    m_amp_by_mm2 = amp_per_mm2_choice;

    wxString value = wxString( "" ) << m_conductorMaterialResitivity;

    if( m_textCtrlConductorResistivity->IsEmpty() || value == "nan" )
    {
        // Initialize m_textCtrl to fill UI space
        // Working variable initialized earlier
        m_textCtrlConductorResistivity->SetValue( "1.72e-8" );
        m_conductorTempCtrl->SetValue( "20" );
    }

    if( m_textCtrlConductorThermCoef->IsEmpty() )
    {
        // Initialize m_textCtrl to fill UI space
        // Working variable initialized earlier
        m_textCtrlConductorThermCoef->SetValue( "3.93e-3" );
    }

    updateAll( m_diameter / 2 );
}


void PANEL_CABLE_SIZE::onUpdateCurrentDensity( wxScrollEvent& aEvent )
{
    m_amp_by_mm2 = m_slCurrentDensity->GetValue();

    // Update displayed values depending on the current density (mainly Ampacity)
    updateAll( m_diameter / 2 );
}


void PANEL_CABLE_SIZE::OnCableSizeChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        double   value;
        int      index = m_sizeChoice->GetSelection();

        if( ( index >= 0 ) && ( index < m_entries.size() ) )
        {
            value = m_entries.at( index ).m_Radius;
            updateAll( value );
        }
    }
}


void PANEL_CABLE_SIZE::OnConductorResistivityChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        m_updatingConductorMaterialResitivity = true;

        m_conductorMaterialResitivityRef =
                std::abs( DoubleFromString( m_textCtrlConductorResistivity->GetValue() ) );
        updateAll( m_diameter / 2 );
        m_updatingConductorMaterialResitivity = false;
    }
}


void PANEL_CABLE_SIZE::OnConductorResistivity_Button( wxCommandEvent& event )
{
    wxArrayString list = StandardCableConductorList();

    // Shows a list of current Specific resistance list (rho) and select a value
    wxString value = wxGetSingleChoice( wxEmptyString,
                                        _( "Electrical Resistivity in Ohm*m at 20 deg C" ), list )
                             .BeforeFirst( ' ' );

    if( !value.IsEmpty() )
        m_textCtrlConductorResistivity->ChangeValue( value );

    OnConductorResistivityChange( event );
}

void PANEL_CABLE_SIZE::OnConductorThermCoefChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        m_conductorMaterialThermalCoef =
                std::abs( DoubleFromString( m_textCtrlConductorThermCoef->GetValue() ) );
        updateAll( m_diameter / 2 );
    }
}

void PANEL_CABLE_SIZE::OnConductorThermCoefChange_Button( wxCommandEvent& event )
{
    wxArrayString list = StandardCableTempCoefList();

    // Shows a list of current Specific resistance list (rho) and select a value
    wxString value = wxGetSingleChoice( wxEmptyString, _( "Temperature coefficient" ), list )
                             .BeforeFirst( ' ' );

    if( !value.IsEmpty() )
        m_textCtrlConductorThermCoef->ChangeValue( value );

    OnConductorThermCoefChange( event );
}

void PANEL_CABLE_SIZE::OnDiameterChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        m_updatingDiameter = true;
        double value;

        if( m_diameterCtrl->GetValue().ToDouble( &value ) )
        {
            updateAll( value / 2 * m_diameterUnit->GetUnitScale() );
            m_sizeChoice->SetSelection( -1 );
        }
        m_updatingDiameter = false;
    }
}


void PANEL_CABLE_SIZE::OnLinResistanceChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        m_updatingLinResistance = true;
        double value;

        if( m_linResistanceCtrl->GetValue().ToDouble( &value ) )
        {
            updateAll( sqrt( m_conductorMaterialResitivity
                             / ( value * m_linResistanceUnit->GetUnitScale() ) / M_PI ) );
            m_sizeChoice->SetSelection( -1 );
        }
        m_updatingLinResistance = false;
    }
}


void PANEL_CABLE_SIZE::OnAreaChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        m_updatingArea = true;
        double value;

        if( m_areaCtrl->GetValue().ToDouble( &value ) )
        {
            updateAll( sqrt( value / M_PI / M2_to_MM2 ) );
            m_sizeChoice->SetSelection( -1 );
        }
        m_updatingArea = false;
    }
}


void PANEL_CABLE_SIZE::OnFrequencyChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        m_updatingFrequency = true;
        double value;

        if( m_frequencyCtrl->GetValue().ToDouble( &value ) )
        {
            updateAll( sqrt( m_conductorMaterialResitivity / value / m_frequencyUnit->GetUnitScale()
                             / M_PI / VACCUM_PERMEABILITY / RELATIVE_PERMEABILITY ) );
            m_sizeChoice->SetSelection( -1 );
        }
        m_updatingFrequency = false;
    }
}


void PANEL_CABLE_SIZE::OnAmpacityChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        m_updatingAmpacity = true;
        double value;

        if( m_AmpacityCtrl->GetValue().ToDouble( &value ) )
        {
            double radius = sqrt( value * m2_by_ampere() / M_PI );
            updateAll( radius );
            m_sizeChoice->SetSelection( -1 );
        }
        m_updatingAmpacity = false;
    }
}

void PANEL_CABLE_SIZE::OnConductorTempChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        double value;

        if( m_conductorTempCtrl->GetValue().ToDouble( &value ) )
        {
            m_conductorTemperature = value;
            updateAll( m_diameter / 2 );
        }
    }
}


void PANEL_CABLE_SIZE::OnCurrentChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        double value;
        m_updatingCurrent = true;

        if( m_currentCtrl->GetValue().ToDouble( &value ) )
        {
            m_current = value;
            updateApplication();
        }
        m_updatingCurrent = false;
    }
}


void PANEL_CABLE_SIZE::OnLengthChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        double value;
        m_updatingLength = true;

        if( m_lengthCtrl->GetValue().ToDouble( &value ) )
        {
            m_length = value * m_lengthUnit->GetUnitScale();
            updateApplication();
        }
        m_updatingLength = false;
    }
}


void PANEL_CABLE_SIZE::OnResistanceDcChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        double value;
        m_updatingResistanceDc = true;

        if( m_resistanceDcCtrl->GetValue().ToDouble( &value ) )
        {
            updateAll( sqrt( m_conductorMaterialResitivity / value * m_length / M_PI ) );
            m_sizeChoice->SetSelection( -1 );
        }
        m_updatingResistanceDc = false;
    }
}


void PANEL_CABLE_SIZE::OnVDropChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        double value;
        m_updatingRVdrop = true;

        if( m_vDropCtrl->GetValue().ToDouble( &value ) )
        {
            updateAll( sqrt( m_conductorMaterialResitivity / value * m_vDropUnit->GetUnitScale()
                             * m_length * m_current / M_PI ) );
            m_sizeChoice->SetSelection( -1 );
        }
        m_updatingRVdrop = false;
    }
}


void PANEL_CABLE_SIZE::OnPowerChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        double value;
        m_updatingPower = true;

        if( m_powerCtrl->GetValue().ToDouble( &value ) )
        {
            updateAll( sqrt( m_conductorMaterialResitivity / value * m_powerUnit->GetUnitScale()
                             * m_length * m_current * m_current / M_PI ) );
            m_sizeChoice->SetSelection( -1 );
        }
        m_updatingPower = false;
    }
}


void PANEL_CABLE_SIZE::printAll()
{
    m_updatingUI = true;

    wxString value;
    wxString tooltipString;

    if( !m_updatingDiameter )
    {
        value = wxString( "" ) << m_diameter / m_diameterUnit->GetUnitScale();
        m_diameterCtrl->SetValue( value );
    }

    if( !m_updatingArea )
    {
        value = wxString( "" ) << m_area * M2_to_MM2;
        m_areaCtrl->SetValue( value );
    }

    if( !m_updatingAmpacity )
    {
        value = wxString( "" ) << m_ampacity;
        m_AmpacityCtrl->SetValue( value );
    }

    if( !m_updatingFrequency )
    {
        value = wxString( "" ) << m_maxFrequency / m_frequencyUnit->GetUnitScale();
        m_frequencyCtrl->SetValue( value );
    }

    if( !m_updatingLinResistance )
    {
        value = wxString( "" ) << m_linearResistance / m_linResistanceUnit->GetUnitScale();
        m_linResistanceCtrl->SetValue( value );
    }

    if( !m_updatingLength )
    {
        value = wxString( "" ) << m_length / m_lengthUnit->GetUnitScale();
        m_lengthCtrl->SetValue( value );
    }

    if( !m_updatingConductorMaterialResitivity )
    {
        //This is not really to update m_textCtrlConductorResistivity since we do not override user's input
        //rather than update its tooltip
        //value = wxString( "" ) << m_conductorMaterialResitivity;
        value = wxString( "" ) << m_conductorMaterialResitivityRef;
        m_textCtrlConductorResistivity->SetValue( value );
        tooltipString = wxString( "Resistivity for " )
                        << m_conductorTemperature << wxString( " deg C is" )
                        << m_conductorMaterialResitivity << wxString( " Ohm*m" );
        m_textCtrlConductorResistivity->SetToolTip( tooltipString );
    }

    if( !m_updatingCurrent )
    {
        value = wxString( "" ) << m_current;
        m_currentCtrl->SetValue( value );
    }

    if( !m_updatingResistanceDc )
    {
        value = wxString( "" ) << m_resistanceDc;
        m_resistanceDcCtrl->SetValue( value );
    }

    if( !m_updatingRVdrop )
    {
        value = wxString( "" ) << m_voltageDrop * m_vDropUnit->GetUnitScale();
        m_vDropCtrl->SetValue( value );
    }

    if( !m_updatingPower )
    {
        value = wxString( "" ) << m_dissipatedPower * m_powerUnit->GetUnitScale();
        m_powerCtrl->SetValue( value );
    }

    m_updatingUI = false;
}


void PANEL_CABLE_SIZE::updateAll( double aRadius )
{
    // Update wire properties
    m_diameter = aRadius * 2;
    m_area = M_PI * aRadius * aRadius;
    m_conductorMaterialResitivity =
            m_conductorMaterialResitivityRef
            * ( 1 + m_conductorMaterialThermalCoef * ( m_conductorTemperature - 20 ) );
    m_linearResistance = m_conductorMaterialResitivity / m_area;
    // max frequency is when skin depth = radius
    m_maxFrequency = m_conductorMaterialResitivity
                     / ( M_PI * aRadius * aRadius * VACCUM_PERMEABILITY * RELATIVE_PERMEABILITY );

    m_ampacity = m_area / m2_by_ampere();

    // Update application-specific values
    m_resistanceDc = m_linearResistance * m_length;
    m_voltageDrop = m_resistanceDc * m_current;
    m_dissipatedPower = m_voltageDrop * m_current;

    printAll();
}

void PANEL_CABLE_SIZE::updateApplication()
{
    m_resistanceDc = m_linearResistance * m_length;
    m_voltageDrop = m_resistanceDc * m_current;
    m_dissipatedPower = m_voltageDrop * m_current;

    printAll();
}
