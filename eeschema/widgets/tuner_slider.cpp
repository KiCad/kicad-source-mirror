/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include <ki_exception.h>
#include <sim/simulator_frame_ui.h>
#include <sim/simulator_frame.h>
#include <sch_symbol.h>
#include <template_fieldnames.h>
#include <widgets/bitmap_button.h>
#include <widgets/std_bitmap_button.h>

#include <algorithm>
#include <cmath>   // log log1p expm1
#include <complex> // norm

// Must be after other includes to avoid conflict with a window header on msys2
#include "tuner_slider.h"
#include "core/kicad_algo.h"

TUNER_SLIDER::TUNER_SLIDER( SIMULATOR_FRAME_UI* aFrame, wxWindow* aParent,
                            const SCH_SHEET_PATH& aSheetPath, SCH_SYMBOL* aSymbol ) :
        TUNER_SLIDER_BASE( aParent ),
        m_symbol( aSymbol->m_Uuid ),
        m_sheetPath( aSheetPath ),
        m_ref( aSymbol->GetRef( &aSheetPath ) ),
        m_min( 0.0 ),
        m_max( 0.0 ),
        m_value( 0.0 ),
        m_runMode( RUN_MODE::SINGLE ),
        m_frame( aFrame )
{
#if  _WIN32
        // BORDER_RAISED/SUNKEN look pretty on every platform but Windows
        long style = GetWindowStyleFlag();
        style &= ~wxBORDER_MASK;
        style |= wxBORDER_SIMPLE;
        SetWindowStyleFlag( style );
#endif //  _WIN32

    const SPICE_ITEM* item = m_frame->GetExporter()->FindItem( m_ref );

    if( !item )
        throw KI_PARAM_ERROR( wxString::Format( _( "%s not found" ), m_ref ) );

    m_name->SetLabel( wxString::Format( _( "Tune %s" ), m_ref ) );
    m_closeBtn->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_e24->SetBitmap( KiBitmapBundle( BITMAPS::e_24 ) );
    m_e24->SetIsCheckButton();
    m_separator->SetIsSeparator();
    m_e48->SetBitmap( KiBitmapBundle( BITMAPS::e_48 ) );
    m_e48->SetIsCheckButton();
    m_e96->SetBitmap( KiBitmapBundle( BITMAPS::e_96 ) );
    m_e96->SetIsCheckButton();
    m_e192->SetBitmap( KiBitmapBundle( BITMAPS::e_192 ) );
    m_e192->SetIsCheckButton();

    m_modeChoice->SetSelection( 0 );
    m_stepCount->SetRange( 2, 100 );
    m_stepCount->SetValue( 5 );
    updateModeControls();

    const SIM_MODEL::PARAM* tunerParam = item->model->GetTunerParam();

    if( !tunerParam )
    {
        throw KI_PARAM_ERROR( wxString::Format( _( "%s has simulation model of type '%s %s'.\n\n"
                                                   "Only passive R, L, C models and voltage and "
                                                   "current sources may be tuned." ),
                                                m_ref,
                                                item->model->GetDeviceInfo().fieldValue,
                                                item->model->GetTypeInfo().fieldValue ) );
    }

    // Special case for potentiometers because we don't have value ranges implemented yet.
    if( item->model->GetType() == SIM_MODEL::TYPE::R_POT )
    {
        std::string valueStr = SIM_VALUE::ToSpice( item->model->GetTunerParam()->value );

        if( valueStr != "" )
            m_value = SPICE_VALUE( valueStr );
        else
            m_value = SPICE_VALUE( "0.5" );

        m_min = SPICE_VALUE( 0 );
        m_max = SPICE_VALUE( 1 );
    }
    else
    {
        m_value = SPICE_VALUE( SIM_VALUE::ToSpice( item->model->GetTunerParam()->value ) );
        m_min = SPICE_VALUE( 0.5 ) * m_value;
        m_max = SPICE_VALUE( 2.0 ) * m_value;
    }

    m_minText->SetValue( m_min.ToOrigString() );
    m_maxText->SetValue( m_max.ToOrigString() );

    updateValueText();
    updateSlider();

    Layout();
}


int TUNER_SLIDER::GetStepCount() const
{
    return std::max( 2, m_stepCount->GetValue() );
}


void TUNER_SLIDER::ShowChangedLanguage()
{
    m_name->SetLabel( wxString::Format( _( "Tune %s" ), m_ref ) );

    wxArrayString modeChoices;
    modeChoices.Add( _( "Single Run" ) );
    modeChoices.Add( _( "Multi Run" ) );

    int selection = m_modeChoice->GetSelection();

    m_modeChoice->Set( modeChoices );

    if( selection < 0 || selection >= static_cast<int>( modeChoices.size() ) )
        selection = 0;

    m_modeChoice->SetSelection( selection );

    m_stepsLabel->SetLabel( _( "Steps" ) );

    updateModeControls();
}


void TUNER_SLIDER::onESeries( wxCommandEvent& event )
{
    for( BITMAP_BUTTON* btn : { m_e24, m_e48, m_e96, m_e192 } )
    {
        if( btn != event.GetEventObject() )
            btn->Check( false );
    }

    wxString oldValue = m_valueText->GetValue();

    updateValueText();

    if( m_valueText->GetValue() != oldValue )
        updateComponentValue();

    event.Skip();
}


void TUNER_SLIDER::onRunModeChanged( wxCommandEvent& event )
{
    int selection = m_modeChoice->GetSelection();

    if( selection == wxNOT_FOUND )
        selection = 0;

    m_runMode = ( selection == 1 ) ? RUN_MODE::MULTI : RUN_MODE::SINGLE;

    updateModeControls();
    updateComponentValue();

    event.Skip();
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
    wxQueueEvent( m_frame, new wxCommandEvent( EVT_SIM_UPDATE ) );
}

void TUNER_SLIDER::updateModeControls()
{
    bool enableSteps = ( m_runMode == RUN_MODE::MULTI );

    m_stepCount->Enable( enableSteps );
    m_stepsLabel->Enable( enableSteps );

    // In Multi Run mode, the middle value text and slider are not directly editable,
    // and save actions are disabled. Re-enable for Single Run.
    bool enableDirectControls = ( m_runMode == RUN_MODE::SINGLE );
    m_valueText->Enable( enableDirectControls );
    m_slider->Enable( enableDirectControls );
    m_saveBtn->Enable( enableDirectControls );
    m_closeBtn->Enable( true );
}


void TUNER_SLIDER::updateSlider()
{
    wxASSERT( m_max >= m_value && m_value >= m_min );
    double value = ( ( m_value - m_min ) / ( m_max - m_min ) ).ToDouble();
    m_slider->SetValue( KiROUND( value * 100.0 ) );
}


void TUNER_SLIDER::updateValueText()
{
    static std::vector<double> e24 = { 1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0,
                                       3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1 };

    static std::vector<double> e192 = { 1.00, 1.01, 1.02, 1.04, 1.05, 1.06, 1.07, 1.09, 1.10, 1.11,
                                        1.13, 1.14, 1.15, 1.17, 1.18, 1.20, 1.21, 1.23, 1.24, 1.26,
                                        1.27, 1.29, 1.30, 1.32, 1.33, 1.35, 1.37, 1.38, 1.40, 1.42,
                                        1.43, 1.45, 1.47, 1.49, 1.50, 1.52, 1.54, 1.56, 1.58, 1.60,
                                        1.62, 1.64, 1.65, 1.67, 1.69, 1.72, 1.74, 1.76, 1.78, 1.80,
                                        1.82, 1.84, 1.87, 1.89, 1.91, 1.93, 1.96, 1.98, 2.00, 2.03,
                                        2.05, 2.08, 2.10, 2.13, 2.15, 2.18, 2.21, 2.23, 2.26, 2.29,
                                        2.32, 2.34, 2.37, 2.40, 2.43, 2.46, 2.49, 2.52, 2.55, 2.58,
                                        2.61, 2.64, 2.67, 2.71, 2.74, 2.77, 2.80, 2.84, 2.87, 2.91,
                                        2.94, 2.98, 3.01, 3.05, 3.09, 3.12, 3.16, 3.20, 3.24, 3.28,
                                        3.32, 3.36, 3.40, 3.44, 3.48, 3.52, 3.57, 3.61, 3.65, 3.70,
                                        3.74, 3.79, 3.83, 3.88, 3.92, 3.97, 4.02, 4.07, 4.12, 4.17,
                                        4.22, 4.27, 4.32, 4.37, 4.42, 4.48, 4.53, 4.59, 4.64, 4.70,
                                        4.75, 4.81, 4.87, 4.93, 4.99, 5.05, 5.11, 5.17, 5.23, 5.30,
                                        5.36, 5.42, 5.49, 5.56, 5.62, 5.69, 5.76, 5.83, 5.90, 5.97,
                                        6.04, 6.12, 6.19, 6.26, 6.34, 6.42, 6.49, 6.57, 6.65, 6.73,
                                        6.81, 6.90, 6.98, 7.06, 7.15, 7.23, 7.32, 7.41, 7.50, 7.59,
                                        7.68, 7.77, 7.87, 7.96, 8.06, 8.16, 8.25, 8.35, 8.45, 8.56,
                                        8.66, 8.76, 8.87, 8.98, 9.09, 9.20, 9.31, 9.42, 9.53, 9.65,
                                        9.76, 9.88 };

    int      precision = 3;
    wxString prefix;
    double   value = m_value.ToNormalizedDouble( &prefix );

    bool e_24 = m_e24->IsChecked();
    bool e_extended = m_e48->IsChecked() || m_e96->IsChecked() || m_e192->IsChecked();

    if( e_24 || e_extended )
    {
        std::vector<double> table;
        table.reserve( 192 + 24 + 1 /* worst case */ );

        if( e_extended )
        {
            int step = m_e48->IsChecked() ? 4 : m_e96->IsChecked() ? 2 : 1;

            for( size_t ii = 0; ii < e192.size(); ii += step )
                table.push_back( e192[ii] );
        }

        if( e_24 )
            table.insert( table.end(), e24.begin(), e24.end() );

        table.push_back( 10.0 );

        std::sort( table.begin(), table.end() );
        alg::remove_duplicates( table );

        for( double decade : { 1.0, 10.0, 100.0 } )
        {
            for( size_t ii = 0; ii < table.size() - 1; ++ii )
            {
                if( value < ( table[ii] + table[ii+1] ) * decade / 2 )
                {
                    precision = 0;

                    if( decade == 1.0 )
                        precision++;

                    if( e_extended && decade != 100.0 )
                        precision++;

                    m_valueText->SetValue( wxString::Format( wxT( "%.*f%s" ),
                                                             precision,
                                                             table[ii] * decade,
                                                             prefix ) );
                    return;
                }
            }
        }
    }

    wxString valueStr = wxString::Format( wxT( "%.3f" ), value );
    SPICE_VALUE::StripZeros( valueStr );
    m_valueText->SetValue( valueStr + prefix );
}


void TUNER_SLIDER::updateMax()
{
    try
    {
        SPICE_VALUE newMax( m_maxText->GetValue() );
        SetMax( newMax );
        // If in Multi Run mode, changing the range should trigger a re-run
        if( m_runMode == RUN_MODE::MULTI )
            updateComponentValue();
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
        // If in Multi Run mode, changing the range should trigger a re-run
        if( m_runMode == RUN_MODE::MULTI )
            updateComponentValue();
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
    m_frame->UpdateTunerValue( m_sheetPath, m_symbol, GetSymbolRef(), m_valueText->GetValue() );
}


void TUNER_SLIDER::onSliderScroll( wxScrollEvent& event )
{
    m_value = m_min + ( m_max - m_min ) * SPICE_VALUE( m_slider->GetValue() / 100.0 );
    updateValueText();
}


void TUNER_SLIDER::onSliderChanged( wxScrollEvent& event )
{
    m_value = m_min + ( m_max - m_min ) * SPICE_VALUE( m_slider->GetValue() / 100.0 );
    updateValueText();
    updateComponentValue();
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


void TUNER_SLIDER::onStepsChanged( wxSpinEvent& event )
{
    updateComponentValue();
    event.Skip();
}


void TUNER_SLIDER::onStepsTextEnter( wxCommandEvent& event )
{
    long steps;

    if( !event.GetString().ToLong( &steps ) )
        steps = m_stepCount->GetValue();

    if( steps < 2 )
        steps = 2;

    m_stepCount->SetValue( static_cast<int>( steps ) );

    updateComponentValue();
    event.Skip();
}
