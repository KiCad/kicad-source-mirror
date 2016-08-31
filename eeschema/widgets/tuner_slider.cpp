/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "tuner_slider.h"

#include <sim/sim_plot_frame.h>
#include <sch_component.h>
#include <template_fieldnames.h>
#include <netlist_exporters/netlist_exporter_pspice.h>

TUNER_SLIDER::TUNER_SLIDER( SIM_PLOT_FRAME* aFrame, wxWindow* aParent, SCH_COMPONENT* aComponent )
    : TUNER_SLIDER_BASE( aParent ), m_component( aComponent ),
    m_min( 0.0 ), m_max( 0.0 ), m_value( 0.0 ), m_frame ( aFrame )
{
    const wxString compName = aComponent->GetField( REFERENCE )->GetText();
    m_name->SetLabel( compName );
    m_value = SPICE_VALUE( aComponent->GetField( VALUE )->GetText() );
    m_changed = false;

    // Generate Spice component name
    char prim = NETLIST_EXPORTER_PSPICE::GetSpiceField( SF_PRIMITIVE, aComponent, 0 )[0];
    m_spiceName = wxString( prim + compName ).Lower();

    // Call Set*() methods to update fields and slider
    m_max = SPICE_VALUE( 2.0 ) * m_value;
    m_min = SPICE_VALUE( 0.5 ) * m_value;

    m_minText->SetValue( m_min.ToOrigString() );
    m_maxText->SetValue( m_max.ToOrigString() );

    updateValueText();
    updateSlider();

    m_simTimer.SetOwner( this );
    Connect( wxEVT_TIMER, wxTimerEventHandler( TUNER_SLIDER::onSimTimer ), NULL, this );
}


bool TUNER_SLIDER::SetValue( const SPICE_VALUE& aVal )
{
    // Get the value into the current range boundaries
    if( aVal > m_max )
        m_value = m_max;
    else if( aVal < m_min )
        m_value = m_min;
    else
        m_value = aVal;

    updateValueText();
    updateSlider();
    updateComponentValue();

    return true;
}


bool TUNER_SLIDER::SetMin( const SPICE_VALUE& aVal )
{
    if( aVal >= m_max )
        return false;

    m_min = aVal;

    if( m_value < aVal )      // Limit the curent value
        SetValue( aVal );

    m_minText->SetValue( aVal.ToOrigString() );
    updateSlider();

    return true;
}


bool TUNER_SLIDER::SetMax( const SPICE_VALUE& aVal )
{
    if( aVal <= m_min )
        return false;

    m_max = aVal;

    if( m_value > aVal )      // Limit the current value
        SetValue( aVal );

    m_maxText->SetValue( aVal.ToOrigString() );
    updateSlider();

    return true;
}


void TUNER_SLIDER::updateComponentValue()
{
    // Start simulation in 100 ms, if the value does not change meanwhile
    m_simTimer.StartOnce( 100 );
}


void TUNER_SLIDER::updateSlider()
{
    assert( m_max >= m_value && m_value >= m_min );

    m_slider->SetValue( ( ( m_value - m_min ) / ( m_max - m_min ) ).ToDouble() * 100.0 );
}


void TUNER_SLIDER::updateValueText()
{
    bool spiceString = m_min.IsSpiceString() || m_max.IsSpiceString();
    m_valueText->SetValue( spiceString ? m_value.ToSpiceString() : m_value.ToString() );
}


void TUNER_SLIDER::onClose( wxCommandEvent& event )
{
    m_frame->RemoveTuner( this );
}


void TUNER_SLIDER::onSave( wxCommandEvent& event )
{
    /// @todo it will crash when component is removed; completely remove m_component
    m_component->GetField( VALUE )->SetText( m_value.ToOrigString() );
}


void TUNER_SLIDER::onSliderChanged( wxScrollEvent& event )
{
    m_value = m_min + ( m_max - m_min ) * SPICE_VALUE( m_slider->GetValue() / 100.0 );
    updateValueText();
    updateComponentValue();
    m_changed = true;
}


void TUNER_SLIDER::onMaxTextEnter( wxCommandEvent& event )
{
    try
    {
        SPICE_VALUE newMax( m_maxText->GetValue() );
        SetMax( newMax );
    }
    catch( std::exception& e )
    {
        // Restore the previous value
        m_maxText->SetValue( m_max.ToOrigString() );
    }
}


void TUNER_SLIDER::onValueTextEnter( wxCommandEvent& event )
{
    try
    {
        SPICE_VALUE newCur( m_valueText->GetValue() );
        SetValue( newCur );
        m_changed = true;
    }
    catch( std::exception& e )
    {
        // Restore the previous value
        m_valueText->SetValue( m_value.ToOrigString() );
    }
}


void TUNER_SLIDER::onMinTextEnter( wxCommandEvent& event )
{
    try
    {
        SPICE_VALUE newMin( m_minText->GetValue() );
        SetMin( newMin );
    }
    catch( std::exception& e )
    {
        // Restore the previous value
        m_minText->SetValue( m_min.ToOrigString() );
    }
}


void TUNER_SLIDER::onSimTimer( wxTimerEvent& event )
{
    if(m_changed)
    {
        wxQueueEvent( m_frame, new wxCommandEvent( EVT_SIM_UPDATE ) );
        m_changed = false;
    }
}
