/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <calculator_panels/panel_cable_size.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>
#include <widgets/unit_selector.h>

#define M2_to_MM2 1000000.0

#define COPPER_RESISTIVITY 1.72e-8 // ohm meter
#define VACCUM_PERMEABILITY 1.256637e-6
#define RELATIVE_PERMEABILITY 1


CABLE_SIZE_ENTRY::CABLE_SIZE_ENTRY( wxString aName, double aRadius )
{
    m_name = aName;
    m_radius = aRadius;
}


PANEL_CABLE_SIZE::PANEL_CABLE_SIZE( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                    const wxSize& size, long style, const wxString& name ) :
        PANEL_CABLE_SIZE_BASE( parent, id, pos, size, style, name )
{
    m_entries.clear();
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG0000" ), 0.005842 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG000" ), 0.00520192 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG00" ), 0.00463296 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG0" ), 0.00412623 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG1" ), 0.00367411 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG2" ), 0.00327152 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG3" ), 0.00291338 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG4" ), 0.00259461 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG5" ), 0.00231013 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG6" ), 0.00205740 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG7" ), 0.00183261 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG8" ), 0.00163195 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG9" ), 0.00145288 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG10" ), 0.00129413 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG11" ), 0.00115189 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG12" ), 0.00102616 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG13" ), 0.0009144 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG14" ), 0.00081407 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG15" ), 0.00072517 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG16" ), 0.00064516 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG17" ), 0.00057531 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG18" ), 0.00051181 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG19" ), 0.00045593 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG20" ), 0.0004046 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG21" ), 0.00036195 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG22" ), 0.00032258 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG23" ), 0.00028702 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG24" ), 0.00025527 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG25" ), 0.00022773 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG26" ), 0.00020193 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG27" ), 0.00018034 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG28" ), 0.00016002 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG29" ), 0.00014351 ) );
    m_entries.push_back( CABLE_SIZE_ENTRY( _( "AWG30" ), 0.000127 ) );

    for( CABLE_SIZE_ENTRY entry : m_entries )
    {
        m_sizeChoice->Append( entry.m_name );
    }
}


void PANEL_CABLE_SIZE::OnUpdateUnit( wxCommandEvent& aEvent )
{
    printAll();
}


PANEL_CABLE_SIZE::~PANEL_CABLE_SIZE()
{
}


void PANEL_CABLE_SIZE::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    aCfg->m_cableSize.diameterUnit = m_diameterUnit->GetSelection();
    aCfg->m_cableSize.linResUnit = m_linResistanceUnit->GetSelection();
    aCfg->m_cableSize.frequencyUnit = m_frequencyUnit->GetSelection();
    aCfg->m_cableSize.lengthUnit = m_lengthUnit->GetSelection();
}


void PANEL_CABLE_SIZE::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_diameterUnit->SetSelection( aCfg->m_cableSize.diameterUnit );
    m_linResistanceUnit->SetSelection( aCfg->m_cableSize.linResUnit );
    m_frequencyUnit->SetSelection( aCfg->m_cableSize.frequencyUnit );
    m_lengthUnit->SetSelection( aCfg->m_cableSize.lengthUnit );
}

void PANEL_CABLE_SIZE::OnSizeChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        double   value;
        int      index = m_sizeChoice->GetSelection();
        wxString str;

        if( ( index >= 0 ) && ( index < m_entries.size() ) )
        {
            value = m_entries.at( index ).m_radius;
            updateAll( value );
        }
    }
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
            updateAll( sqrt( COPPER_RESISTIVITY / ( value * m_linResistanceUnit->GetUnitScale() )
                             / M_PI ) );
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
            updateAll( sqrt( COPPER_RESISTIVITY / value / m_frequencyUnit->GetUnitScale() / M_PI
                             / VACCUM_PERMEABILITY / RELATIVE_PERMEABILITY ) );
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
            // Based on the 700 circular mils per amp rule of the thumb
            // The long number is the sq m to circular mils conversion
            updateAll( sqrt( value * 700 / 1973525241.77 / M_PI ) );
            m_sizeChoice->SetSelection( -1 );
        }
        m_updatingAmpacity = false;
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


void PANEL_CABLE_SIZE::OnResistanceChange( wxCommandEvent& aEvent )
{
    if( !m_updatingUI )
    {
        double value;
        m_updatingResistance = true;

        if( m_resistanceCtrl->GetValue().ToDouble( &value ) )
        {
            updateAll( sqrt( COPPER_RESISTIVITY / value * m_length / M_PI ) );
            m_sizeChoice->SetSelection( -1 );
        }
        m_updatingResistance = false;
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
            updateAll( sqrt( COPPER_RESISTIVITY / value * m_length * m_current / M_PI ) );
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
            updateAll(
                    sqrt( COPPER_RESISTIVITY / value * m_length * m_current * m_current / M_PI ) );
            m_sizeChoice->SetSelection( -1 );
        }
        m_updatingPower = false;
    }
}


void PANEL_CABLE_SIZE::printAll()
{
    m_updatingUI = true;

    wxString value;

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

    if( !m_updatingCurrent )
    {
        value = wxString( "" ) << m_current;
        m_currentCtrl->SetValue( value );
    }

    if( !m_updatingResistance )
    {
        value = wxString( "" ) << m_resistance;
        m_resistanceCtrl->SetValue( value );
    }

    if( !m_updatingRVdrop )
    {
        value = wxString( "" ) << m_voltageDrop;
        m_vDropCtrl->SetValue( value );
    }

    if( !m_updatingPower )
    {
        value = wxString( "" ) << m_dissipatedPower;
        m_powerCtrl->SetValue( value );
    }

    m_updatingUI = false;
}


void PANEL_CABLE_SIZE::updateAll( double aRadius )
{
    // Update wire properties
    m_diameter = aRadius * 2;
    m_area = M_PI * aRadius * aRadius;
    m_linearResistance = COPPER_RESISTIVITY / m_area;
    // max frequency is when skin depth = radius
    m_maxFrequency = COPPER_RESISTIVITY
                     / ( M_PI * aRadius * aRadius * VACCUM_PERMEABILITY * RELATIVE_PERMEABILITY );

    // Based on the 700 circular mils per amp rule of the thumb
    // The long number is the sq m to circular mils conversion
    m_ampacity = ( m_area * 1973525241.77 ) / 700;
    // Update application-specific values
    m_resistance = m_linearResistance * m_length;
    m_voltageDrop = m_resistance * m_current;
    m_dissipatedPower = m_voltageDrop * m_current;
    printAll();
}

void PANEL_CABLE_SIZE::updateApplication()
{
    m_resistance = m_linearResistance * m_length;
    m_voltageDrop = m_resistance * m_current;
    m_dissipatedPower = m_voltageDrop * m_current;
    printAll();
}
