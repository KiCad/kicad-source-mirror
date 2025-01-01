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

#include <calculator_panels/panel_wavelength.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>
#include <widgets/unit_selector.h>
#include <wx/choicdlg.h>
#include "pcb_calculator_utils.h"
#include "common_data.h"
#include <wx/dcclient.h>

#define SPEED_LIGHT 299792458

PANEL_WAVELENGTH::PANEL_WAVELENGTH( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                    const wxSize& size, long style, const wxString& name ) :
        PANEL_WAVELENGTH_BASE( parent, id, pos, size, style, name )
{
    // Set the min size of wxTextCtrls showing long values
    wxSize minSize = m_speedCtrl->GetSize();
    int    minWidth = m_speedCtrl->GetTextExtent( wxT( "1.234567890E+99" ) ).x;

    m_speedCtrl->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );

    Layout();
}

void PANEL_WAVELENGTH::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    aCfg->m_wavelength.frequency = m_frequency;
    aCfg->m_wavelength.permittivity = m_permittivity;
    aCfg->m_wavelength.permeability = m_permeability;

    aCfg->m_wavelength.frequencyUnit = m_frequencyUnit->GetSelection();
    aCfg->m_wavelength.periodUnit = m_periodUnit->GetSelection();
    aCfg->m_wavelength.wavelengthVacuumUnit = m_wavelengthVacuumUnit->GetSelection();
    aCfg->m_wavelength.wavelengthMediumUnit = m_wavelengthMediumUnit->GetSelection();
    aCfg->m_wavelength.speedUnit = m_speedUnit->GetSelection();
}


void PANEL_WAVELENGTH::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    m_frequencyUnit->SetSelection( aCfg->m_wavelength.frequencyUnit );
    m_periodUnit->SetSelection( aCfg->m_wavelength.periodUnit );
    m_wavelengthVacuumUnit->SetSelection( aCfg->m_wavelength.wavelengthVacuumUnit );
    m_wavelengthMediumUnit->SetSelection( aCfg->m_wavelength.wavelengthMediumUnit );
    m_speedUnit->SetSelection( aCfg->m_wavelength.speedUnit );

    m_permittivity = aCfg->m_wavelength.permittivity;
    m_permeability = aCfg->m_wavelength.permeability;
    m_frequency = aCfg->m_wavelength.frequency;

    wxString value;
    value = wxString( "" ) << m_permittivity;
    m_permittivityCtrl->SetValue( value );
    value = wxString( "" ) << m_permeability;
    m_permeabilityCtrl->SetValue( value );

    update( m_frequency );
}

void PANEL_WAVELENGTH::updateUnits( wxCommandEvent& aEvent )
{
    update( m_frequency );
}


void PANEL_WAVELENGTH::update( double aFrequency )
{
    m_updatingUI = true;
    wxString value;

    if( !m_updatingFrequency )
    {
        value = wxString( "" ) << aFrequency / m_frequencyUnit->GetUnitScale();
        m_frequencyCtrl->SetValue( value );
    }

    if( !m_updatingPeriod )
    {
        value = wxString( "" ) << 1 / aFrequency / m_periodUnit->GetUnitScale();
        m_periodCtrl->SetValue( value );
    }

    if( !m_updatingWavelengthVacuum )
    {
        value = wxString( "" ) << SPEED_LIGHT / aFrequency / m_wavelengthVacuumUnit->GetUnitScale();
        m_wavelengthVacuumCtrl->SetValue( value );
    }

    if( !m_updatingWavelengthMedium )
    {
        value = wxString( "" ) << SPEED_LIGHT / aFrequency / sqrt( m_permittivity * m_permeability )
                                          / m_wavelengthMediumUnit->GetUnitScale();
        m_wavelengthMediumCtrl->SetValue( value );
    }

    if( !m_updatingSpeed )
    {
        value = wxString( "" ) << SPEED_LIGHT / sqrt( m_permittivity * m_permeability )
                                          / m_speedUnit->GetUnitScale();
        m_speedCtrl->SetValue( value );
    }

    m_frequency = aFrequency;

    m_updatingFrequency = false;
    m_updatingPeriod = false;
    m_updatingWavelengthVacuum = false;
    m_updatingWavelengthMedium = false;
    m_updatingSpeed = false;

    m_updatingUI = false;
}

void PANEL_WAVELENGTH::OnFrequencyChange( wxCommandEvent& event )
{
    double value;

    if( m_updatingUI )
    {
        return;
    }

    wxString input = m_frequencyCtrl->GetValue();

    if( input.ToDouble( &value ) )
    {
        if( value > 0 )
        {
            m_updatingFrequency = true;
            update( value * m_frequencyUnit->GetUnitScale() );
        }
    }
}

void PANEL_WAVELENGTH::OnPeriodChange( wxCommandEvent& event )
{
    double value;

    if( m_updatingUI )
    {
        return;
    }

    wxString input = m_periodCtrl->GetValue();

    if( input.ToDouble( &value ) )
    {
        if( value > 0 )
        {
            m_updatingPeriod = true;
            update( 1 / ( value * m_periodUnit->GetUnitScale() ) );
        }
    }
}

void PANEL_WAVELENGTH::OnWavelengthVacuumChange( wxCommandEvent& event )
{
    if( m_updatingUI )
    {
        return;
    }

    double   value;
    wxString input = m_wavelengthVacuumCtrl->GetValue();

    if( input.ToDouble( &value ) )
    {
        if( value > 0 )
        {
            value *= m_wavelengthVacuumUnit->GetUnitScale();
            m_updatingWavelengthVacuum = true;
            update( SPEED_LIGHT / value );
        }
    }
};


void PANEL_WAVELENGTH::OnWavelengthMediumChange( wxCommandEvent& event )
{
    if( m_updatingUI )
    {
        return;
    }

    double   value;
    wxString input = m_wavelengthMediumCtrl->GetValue();

    if( input.ToDouble( &value ) )
    {
        if( value > 0 )
        {
            value *= m_wavelengthMediumUnit->GetUnitScale();
            m_updatingWavelengthMedium = true;
            update( SPEED_LIGHT / value / sqrt( m_permittivity * m_permeability ) );
        }
    }
};

void PANEL_WAVELENGTH::OnPermittivityChange( wxCommandEvent& event )
{
    double   value;
    wxString input = m_permittivityCtrl->GetValue();

    if( input.ToDouble( &value ) )
    {
        if( value >= 1 )
        {
            m_permittivity = value;
            update( m_frequency );
        }
    }
}

void PANEL_WAVELENGTH::OnPermeabilityChange( wxCommandEvent& event )
{
    double   value;
    wxString input = m_permeabilityCtrl->GetValue();

    if( input.ToDouble( &value ) )
    {
        if( value >= 1 )
        {
            m_permeability = value;
            update( m_frequency );
        }
    }
}

void PANEL_WAVELENGTH::OnButtonPermittivity( wxCommandEvent& event )
{
    wxArrayString list = StandardRelativeDielectricConstantList();
    list.Add( "" ); // Add an empty line for no selection

    // Find the previous choice index:
    wxString prevChoiceStr = m_permittivityCtrl->GetValue();
    int      prevChoice = 0;
    findMatch( list, prevChoiceStr, prevChoice );

    int index = wxGetSingleChoiceIndex( wxEmptyString, _( "Relative Dielectric Constants" ), list,
                                        prevChoice );

    if( index >= 0 && !list.Item( index ).IsEmpty() )
    {
        m_permittivityCtrl->SetValue( list.Item( index ).BeforeFirst( ' ' ) );
        // wx automatically calls onPermittivity()
    }
}
