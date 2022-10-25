/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
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
#include <sch_symbol.h>
#include <template_fieldnames.h>
#include <sim/ngspice_helpers.h>

#include <cmath>   // log log1p expm1
#include <complex> // norm


TUNER_SLIDER::TUNER_SLIDER( SIM_PLOT_FRAME* aFrame, wxWindow* aParent, SCH_SYMBOL* aSymbol ) :
    TUNER_SLIDER_BASE( aParent ),
    m_symbol( aSymbol ),
    m_min( 0.0 ),
    m_max( 0.0 ),
    m_value( 0.0 ),
    m_changed( false ),
    m_frame ( aFrame )
{
    wxString ref = aSymbol->GetField( REFERENCE_FIELD )->GetShownText();
    m_item = aFrame->GetExporter()->FindItem( std::string( ref.ToUTF8() ) );

    if( !m_item )
        throw KI_PARAM_ERROR( wxString::Format( _( "Could not find Spice item with reference '%s'" ),
                                                ref ) );

    m_name->SetLabel( ref );


    const SIM_MODEL::PARAM* tunerParam = m_item->model->GetTunerParam();

    if( !tunerParam )
        throw KI_PARAM_ERROR( wxString::Format(
                _( "Symbol '%s' has simulation model of type '%s %s', which cannot be tuned" ),
                ref,
                m_item->model->GetDeviceTypeInfo().fieldValue,
                m_item->model->GetTypeInfo().fieldValue ) );


    // Special case for potentiometers because we don't have value ranges implemented yet.
    if( m_item->model->GetType() == SIM_MODEL::TYPE::R_POT )
    {
        std::string valueStr = m_item->model->GetTunerParam()->value->ToSpiceString();

        if( valueStr != "" )
            m_value = SPICE_VALUE( valueStr );
        else
            m_value = SPICE_VALUE( "0.5" );

        m_min = SPICE_VALUE( 0 );
        m_max = SPICE_VALUE( 1 );
    }
    else
    {
        m_value = SPICE_VALUE( m_item->model->GetTunerParam()->value->ToSpiceString() );
        m_min = SPICE_VALUE( 0.5 ) * m_value;
        m_max = SPICE_VALUE( 2.0 ) * m_value;
    }

    m_minText->SetValue( m_min.ToOrigString() );
    m_maxText->SetValue( m_max.ToOrigString() );

    updateValueText();
    updateSlider();

    m_simTimer.SetOwner( this );
    Connect( wxEVT_TIMER, wxTimerEventHandler( TUNER_SLIDER::onSimTimer ), nullptr, this );
}


std::string TUNER_SLIDER::GetTunerCommand() const
{
    return m_item->model->SpiceGenerator().TunerCommand( *m_item,
                                                         SIM_VALUE_FLOAT( m_value.ToDouble() ) );
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

    if( m_value < aVal )      // Limit the current value
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
    wxASSERT( m_max >= m_value && m_value >= m_min );
    m_slider->SetValue( ( ( m_value - m_min ) / ( m_max - m_min ) ).ToDouble() * 100 );
}


void TUNER_SLIDER::updateValueText()
{
    bool spiceString = m_min.IsSpiceString() || m_max.IsSpiceString();
    m_valueText->SetValue( spiceString ? m_value.ToSpiceString() : m_value.ToString() );
}


void TUNER_SLIDER::updateMax()
{
    try
    {
        SPICE_VALUE newMax( m_maxText->GetValue() );
        SetMax( newMax );
    }
    catch( const KI_PARAM_ERROR& )
    {
        // Restore the previous value
        m_maxText->SetValue( m_max.ToOrigString() );
    }
}


void TUNER_SLIDER::updateValue()
{
    try
    {
        SPICE_VALUE newCur( m_valueText->GetValue() );
        SetValue( newCur );
        m_changed = true;
    }
    catch( const KI_PARAM_ERROR& )
    {
        // Restore the previous value
        m_valueText->SetValue( m_value.ToOrigString() );
    }
}


void TUNER_SLIDER::updateMin()
{
    try
    {
        SPICE_VALUE newMin( m_minText->GetValue() );
        SetMin( newMin );
    }
    catch( const KI_PARAM_ERROR& )
    {
        // Restore the previous value
        m_minText->SetValue( m_min.ToOrigString() );
    }
}


void TUNER_SLIDER::onClose( wxCommandEvent& event )
{
    m_frame->RemoveTuner( this );
}


void TUNER_SLIDER::onSave( wxCommandEvent& event )
{
    m_frame->UpdateTunerValue( m_symbol, m_value.ToOrigString() );
}


void TUNER_SLIDER::onSliderChanged( wxScrollEvent& event )
{
    m_value = m_min + ( m_max - m_min ) * SPICE_VALUE( m_slider->GetValue() / 100.0 );
    updateValueText();
    updateComponentValue();
    m_changed = true;
}


void TUNER_SLIDER::onMaxKillFocus( wxFocusEvent& event )
{
    updateMax();
    event.Skip();       // Mandatory in wxFocusEvent
}


void TUNER_SLIDER::onValueKillFocus( wxFocusEvent& event )
{
    updateValue();
    event.Skip();       // Mandatory in wxFocusEvent
}


void TUNER_SLIDER::onMinKillFocus( wxFocusEvent& event )
{
    updateMin();
    event.Skip();       // Mandatory in wxFocusEvent
}


void TUNER_SLIDER::onMaxTextEnter( wxCommandEvent& event )
{
    updateMax();
    event.Skip();       // Mandatory in wxFocusEvent
}


void TUNER_SLIDER::onValueTextEnter( wxCommandEvent& event )
{
    updateValue();
}


void TUNER_SLIDER::onMinTextEnter( wxCommandEvent& event )
{
    updateMin();
}


void TUNER_SLIDER::onSimTimer( wxTimerEvent& event )
{
    if( m_changed )
    {
        wxQueueEvent( m_frame, new wxCommandEvent( EVT_SIM_UPDATE ) );
        m_changed = false;
    }
}
