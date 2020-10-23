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
#include <sim/netlist_exporter_pspice_sim.h>

#include <cmath>   // log log1p expm1
#include <complex> // norm


// characteristic curves (default: 0B Lin)
static const struct { double m_law, m_mid; } CURVES[] =
{
    // same order as choices in m_curve
    { 1.00, 0.10 }, // 10A Log
    { 1.00, 0.15 }, // 15A Log
    { 0.90, 0.13 }, // 15A Log S
    { 0.00, 0.10 }, // 10C Rev Log
    { 0.00, 0.15 }, // 15C Rev Log
    { 0.10, 0.13 }, // 15C Rev Log S
    { 0.50, 0.50 }, // 0B Lin
    { 0.50, 0.15 }, // 4B S-Curve
    { 0.50, 0.10 }, // 5B S-Curve
    { 0.50, 0.00 }  // Switch
};

template <typename T, int S>
static inline int arraysize( const T (&v)[S] ) { return S; }


/**
 * Transform ratio according to linear, logarithmic or reverse logarithmic laws
 * (characteristic curve or taper) of rotary or slide potentiometer (pot or fader).
 *
 * Parameters corresponding to *IEC 60393-1:2008* and *JIS C 5260-1* code letters:
 *
 * | @p aLaw | @p aMid | Code | Resistance Law (Taper)
 * | ------: | ------: | ---: | :---------------------
 * |    1.00 |    0.10 |  10A | CW logarithmic
 * |     ^   |    0.15 |  15A | CW logarithmic (audio)
 * |    0.90 |    0.13 |   ^  | CW logarithmic (high-end audio)
 * |    0.00 |    0.10 |  10C | CCW/reverse logarithmic
 * |     ^   |    0.15 |  15C | CCW/reverse logarithmic (reverse audio)
 * |    0.10 |    0.13 |   ^  | CCW/reverse logarithmic (high-end reverse audio)
 * |    0.50 |    0.50 |   0B | (ideal) linear
 * |     ^   |    0.15 |   4B | symmetric (audio S-curve)
 * |     ^   |    0.10 |   5B | symmetric (S-curve)
 * |     ^   |    0.00 |   —  | switch
 *
 * Standards code letters cross-reference:
 *
 * |  IEC 60393-1:2008 | IEC 60393-1:1989 | MIL-R-94 | Resistance Law
 * | ----------------: | :--------------: | :------: | :-------------
 * |                0B |         A        |     A    | linear
 * |               10A |         B        |     C    | logarithmic
 * |               15A |         ^        |     —    | ^
 * |               10C |         C        |     F    | reverse logarithmic
 * |               15C |         ^        |     —    | ^
 *
 * **Logarithmic Law** is for *levels* (logarithmic units) and is actually an exponential curve.
 * **Reverse** refers to a reverse-mounted resistive element or shaft on a potentiometer
 * (resulting in a reflected curve). An **S-curve** is a curve joined to its (scaled) reflection,
 * and *may* be symmetric or linear. **Inverse** refers to the mathematical inverse of a function.
 *
 * @tparam F is a floating point type.
 * @param  aRatio    is the input (travel) ratio or moving contact (wiper) position, from
 *                   0%/CCW/left (0) through 50%/mid-travel/center (½) to 100%/CW/right (1).
 * @param  aMid      is the logarithmic laws' output ratio at 50%/mid-travel/center (½)
 *                   input ratio.
 * @param  aLaw      is the (resistance) law, interpolating from *reverse logarithmic* (0)
 *                   through *symmetric/linear* (½) to *logarithmic* (1).
 * @param  aInverse  swaps input and output ratios (inverse function, where possible),
 *                   if @c true.
 * @return the output (resistance or voltage) ratio in [0, 1].
 */
template <typename F>
static F taper( F aRatio, F aMid = 0.5, F aLaw = 1.0, bool aInverse = false )
{
    // clamp to [0, 1] and short-cut
    if( aRatio <= 0 )
        return 0;
    if( aRatio >= 1 )
        return 1;

    // short-cut for ideal linear or at S-curve inflection point
    if( aMid == 0.5 || aRatio == aLaw )
        return aRatio;

    F t = aRatio;

    // clamp to [0, 1] and short-cut at (non-invertible) limits
    if( aMid <= 0 )
        t = aInverse ? 1 : 0;
    else if( aMid >= 1 )
        t = aInverse ? 0 : 1;
    else
    {
        // clamp, and reflect and/or scale for reverse…symmetric…normal laws
        if( aLaw >= 1 )
            t =     t;
        else if( aLaw <= 0 )
            t =   1-t;
        else if( aRatio <= aLaw )
            t =     t   /     aLaw;
        else
            t = ( 1-t ) / ( 1-aLaw );

        // scaling factors for domain and range in [0, 1]
        F a = std::norm( 1 - 1/aMid );
        F b = std::log( a );
        F c = a - 1;

        // scaling: a = (1 - 1/m)²
        // log law: (aᵗ - 1) / (a - 1)
        // inverse: logₐ(1 + t (a - 1))
        t = aInverse ? std::log1p( t * c ) / b : std::expm1( t * b ) / c;
    }

    // clamp, and scale and/or reflect for reverse…symmetric…normal laws
    if( aLaw >= 1 )
        t =     t;
    else if( aLaw <= 0 )
        t = 1 - t;
    else if( aRatio <= aLaw )
        t =     t *     aLaw;
    else
        t = 1 - t * ( 1-aLaw );

    return t;
}


TUNER_SLIDER::TUNER_SLIDER( SIM_PLOT_FRAME* aFrame, wxWindow* aParent, SCH_SYMBOL* aSymbol ) :
    TUNER_SLIDER_BASE( aParent ),
    m_symbol( aSymbol ),
    m_fieldId( MANDATORY_FIELD_T::VALUE_FIELD ),
    m_min( 0.0 ),
    m_max( 0.0 ),
    m_value( 0.0 ),
    m_frame ( aFrame )
{
    const wxString compName = aSymbol->GetField( REFERENCE_FIELD )->GetText();
    m_name->SetLabel( compName );
    m_spiceTuningCommand = aFrame->GetExporter()->GetSpiceTuningCommand( compName );

    if( m_spiceTuningCommand.second )
    {
        // model parameter, with fixed %-range and unknown initial value
        m_min   =   0;
        m_max   = 100;
        m_value = ( m_max - m_min ) / 2.0; // midpoint
        m_minText->Disable();
        m_maxText->Disable();
        m_saveBtn->Disable(); // not an instance parameter that could be updated (invalid m_fieldId)
    }
    else
    {
        // instance parameter
        if( aSymbol->FindField( NETLIST_EXPORTER_PSPICE::GetSpiceFieldName( SF_MODEL ) ) )
            m_fieldId = aSymbol->FindField( NETLIST_EXPORTER_PSPICE::GetSpiceFieldName( SF_MODEL ) )->GetId();
        else
            m_fieldId = aSymbol->GetField( VALUE_FIELD )->GetId();

        m_value = SPICE_VALUE( aSymbol->GetFieldById( m_fieldId )->GetText() );
        m_min   = SPICE_VALUE( 0.5 ) * m_value;
        m_max   = SPICE_VALUE( 2.0 ) * m_value;
    }

    // Call Set*() methods to update fields and slider
    m_minText->SetValue( m_min.ToOrigString() );
    m_maxText->SetValue( m_max.ToOrigString() );

    updateValueText();
    updateSlider();

    m_simTimer.SetOwner( this );
    Connect( wxEVT_TIMER, wxTimerEventHandler( TUNER_SLIDER::onSimTimer ), nullptr, this );
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
    assert( m_max >= m_value && m_value >= m_min );

    int choice = m_curve->GetSelection();
    wxCHECK( choice >= 0 && choice < arraysize( CURVES ), /*void*/ );

    double ratio  = ( ( m_value - m_min ) / ( m_max - m_min ) ).ToDouble();
    double travel = taper( ratio, CURVES[choice].m_mid, CURVES[choice].m_law, true );
    m_slider->SetValue( KiROUND( travel * 100.0 ) );
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
    m_frame->UpdateTunerValue( m_symbol, m_fieldId, m_value.ToOrigString() );
}


void TUNER_SLIDER::onSliderChanged( wxScrollEvent& event )
{
    int choice = m_curve->GetSelection();
    wxCHECK( choice >= 0 && choice < arraysize( CURVES ), /*void*/ );

    double travel = m_slider->GetValue() / 100.0;
    double ratio  = taper( travel, CURVES[choice].m_mid, CURVES[choice].m_law, false );
    m_value = m_min + ( m_max - m_min ) * SPICE_VALUE( ratio );
    updateValueText();
    updateComponentValue();
    m_changed = true;
}


void TUNER_SLIDER::onCurveChoice( wxCommandEvent& event )
{
    updateValue();
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
