/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
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

/* see
 * http://www.desmith.net/NMdS/Electronics/TraceWidth.html
 * http://www.ultracad.com/articles/pcbtemp.pdf
 * for more info
 */

#include <cassert>
#include <cmath>
#include <kiface_base.h>

#include <calculator_panels/panel_track_width.h>
#include <pcb_calculator_settings.h>
#include <string_utils.h>
#include <units_scales.h>

#include <widgets/unit_selector.h>

#include <i18n_utility.h>   // For _HKI definition
wxString tracks_width_versus_current_formula =
#include "tracks_width_versus_current_formula.h"

extern double DoubleFromString( const wxString& TextValue );

// The IPC2221 formula used to calculate track width is valid only for copper material
const double copper_resistivity = 1.72e-8;


PANEL_TRACK_WIDTH::PANEL_TRACK_WIDTH( wxWindow* parent, wxWindowID id,
                                const wxPoint& pos, const wxSize& size,
                                long style, const wxString& name ) :
        PANEL_TRACK_WIDTH_BASE( parent, id, pos, size, style, name ),
        m_TWMode( TW_MASTER_CURRENT ),
        m_TWNested( false )
{
    m_trackTempUnits->SetLabel( wxT( "°C" ) );
    m_resistivityUnits->SetLabel( wxT( "Ω⋅m" ) );

    m_extTrackResUnits->SetLabel( wxT( "Ω" ) );
    m_intTrackResUnits->SetLabel( wxT( "Ω" ) );

    m_staticText63->SetLabel( _( "Temperature rise" ) + wxT( " (ΔT):" ) );

    // Needed on wxWidgets 3.0 to ensure sizers are correctly set
    GetSizer()->SetSizeHints( this );
}


PANEL_TRACK_WIDTH::~PANEL_TRACK_WIDTH()
{
}


void PANEL_TRACK_WIDTH::ThemeChanged()
{
    m_htmlWinFormulas->ThemeChanged();
}


void PANEL_TRACK_WIDTH::SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    aCfg->m_TrackWidth.current                   = m_TrackCurrentValue->GetValue();
    aCfg->m_TrackWidth.delta_tc                  = m_TrackDeltaTValue->GetValue();
    aCfg->m_TrackWidth.track_len                 = m_TrackLengthValue->GetValue();
    aCfg->m_TrackWidth.track_len_units           = m_TW_CuLength_choiceUnit->GetSelection();
    aCfg->m_TrackWidth.resistivity               = m_TWResistivity->GetValue();
    aCfg->m_TrackWidth.ext_track_width           = m_ExtTrackWidthValue->GetValue();
    aCfg->m_TrackWidth.ext_track_width_units     = m_TW_ExtTrackWidth_choiceUnit->GetSelection();
    aCfg->m_TrackWidth.ext_track_thickness       = m_ExtTrackThicknessValue->GetValue();
    aCfg->m_TrackWidth.ext_track_thickness_units = m_ExtTrackThicknessUnit->GetSelection();
    aCfg->m_TrackWidth.int_track_width           = m_IntTrackWidthValue->GetValue();
    aCfg->m_TrackWidth.int_track_width_units     = m_TW_IntTrackWidth_choiceUnit->GetSelection();
    aCfg->m_TrackWidth.int_track_thickness       = m_IntTrackThicknessValue->GetValue();
    aCfg->m_TrackWidth.int_track_thickness_units = m_IntTrackThicknessUnit->GetSelection();
}


void PANEL_TRACK_WIDTH::OnTWParametersChanged( wxCommandEvent& event )
{
    switch(m_TWMode)
    {
    case TW_MASTER_CURRENT:   OnTWCalculateFromCurrent( event );   break;
    case TW_MASTER_EXT_WIDTH: OnTWCalculateFromExtWidth( event );  break;
    case TW_MASTER_INT_WIDTH: OnTWCalculateFromIntWidth( event );  break;
    }
}


void PANEL_TRACK_WIDTH::OnTWCalculateFromCurrent( wxCommandEvent& event )
{
    // Setting the calculated values generates further events. Stop them.
    if( m_TWNested )
    {
        event.StopPropagation();
        return;
    }

    m_TWNested = true;

    // Update state.
    if( m_TWMode != TW_MASTER_CURRENT )
    {
        m_TWMode = TW_MASTER_CURRENT;
        TWUpdateModeDisplay();
    }

    // Prepare parameters:
    double current      = std::abs( DoubleFromString( m_TrackCurrentValue->GetValue() ) );
    double extThickness = std::abs( DoubleFromString( m_ExtTrackThicknessValue->GetValue() ) );
    double intThickness = std::abs( DoubleFromString( m_IntTrackThicknessValue->GetValue() ) );
    double deltaT_C     = std::abs( DoubleFromString( m_TrackDeltaTValue->GetValue() ) );

    // Normalize by units.
    extThickness *= m_ExtTrackThicknessUnit->GetUnitScale();
    intThickness *= m_IntTrackThicknessUnit->GetUnitScale();

    // Calculate the widths.
    double extTrackWidth = TWCalculateWidth( current, extThickness, deltaT_C, false );
    double intTrackWidth = TWCalculateWidth( current, intThickness, deltaT_C, true );

    // Update the display.
    TWDisplayValues( current, extTrackWidth, intTrackWidth, extThickness, intThickness );

    // Re-enable the events.
    m_TWNested = false;
}


void PANEL_TRACK_WIDTH::OnTWCalculateFromExtWidth( wxCommandEvent& event )
{
    // Setting the calculated values generates further events. Stop them.
    if( m_TWNested )
    {
        event.StopPropagation();
        return;
    }
    m_TWNested = true;

    // Update state.
    if( m_TWMode != TW_MASTER_EXT_WIDTH )
    {
        m_TWMode = TW_MASTER_EXT_WIDTH;
        TWUpdateModeDisplay();
    }

    // Load parameters.
    double current;
    double extThickness  = std::abs( DoubleFromString( m_ExtTrackThicknessValue->GetValue() ) );
    double intThickness  = std::abs( DoubleFromString( m_IntTrackThicknessValue->GetValue() ) );
    double deltaT_C      = std::abs( DoubleFromString( m_TrackDeltaTValue->GetValue() ) );
    double extTrackWidth = std::abs( DoubleFromString( m_ExtTrackWidthValue->GetValue() ) );
    double intTrackWidth;

    // Normalize units.
    extThickness  *= m_ExtTrackThicknessUnit->GetUnitScale();
    intThickness  *= m_IntTrackThicknessUnit->GetUnitScale();
    extTrackWidth *= m_TW_ExtTrackWidth_choiceUnit->GetUnitScale();

    // Calculate the maximum current.
    current = TWCalculateCurrent( extTrackWidth, extThickness, deltaT_C, false );

    // And now calculate the corresponding internal width.
    intTrackWidth = TWCalculateWidth( current, intThickness, deltaT_C, true );

    // Update the display.
    TWDisplayValues( current, extTrackWidth, intTrackWidth, extThickness, intThickness );

    // Re-enable the events.
    m_TWNested = false;
}


void PANEL_TRACK_WIDTH::OnTWCalculateFromIntWidth( wxCommandEvent& event )
{
    // Setting the calculated values generates further events. Stop them.
    if( m_TWNested )
    {
        event.StopPropagation();
        return;
    }

    m_TWNested = true;

    // Update state.
    if( m_TWMode != TW_MASTER_INT_WIDTH )
    {
        m_TWMode = TW_MASTER_INT_WIDTH;
        TWUpdateModeDisplay();
    }

    // Load parameters.
    double current;
    double extThickness  = std::abs( DoubleFromString( m_ExtTrackThicknessValue->GetValue() ) );
    double intThickness  = std::abs( DoubleFromString( m_IntTrackThicknessValue->GetValue() ) );
    double deltaT_C      = std::abs( DoubleFromString( m_TrackDeltaTValue->GetValue() ) );
    double extTrackWidth;
    double intTrackWidth = std::abs( DoubleFromString( m_IntTrackWidthValue->GetValue() ) );

    // Normalize units.
    extThickness  *= m_ExtTrackThicknessUnit->GetUnitScale();
    intThickness  *= m_IntTrackThicknessUnit->GetUnitScale();
    intTrackWidth *= m_TW_IntTrackWidth_choiceUnit->GetUnitScale();

    // Calculate the maximum current.
    current = TWCalculateCurrent( intTrackWidth, intThickness, deltaT_C, true );

    // And now calculate the corresponding external width.
    extTrackWidth = TWCalculateWidth( current, extThickness, deltaT_C, false );

    // Update the display.
    TWDisplayValues( current, extTrackWidth, intTrackWidth, extThickness, intThickness );

    // Re-enable the events.
    m_TWNested = false;
}


void PANEL_TRACK_WIDTH::OnTWResetButtonClick( wxCommandEvent& event )
{
    // Note: a wxString:Format( "%g", xx) is used to use local separator in floats

    // Init main parameters:
    m_TrackDeltaTValue->ChangeValue( wxString::Format( "%g", 10.0 ) );
    m_TrackLengthValue->ChangeValue( wxString::Format( "%g", 20.0 ) );
    m_TW_CuLength_choiceUnit->SetSelection( 0 );
    m_TWResistivity->SetValue( wxString::Format( "%g", copper_resistivity ) );

    m_ExtTrackWidthValue->ChangeValue( wxString::Format( "%g", 0.2 ) );
    m_TW_ExtTrackWidth_choiceUnit->SetSelection( 0 );
    m_ExtTrackThicknessValue->ChangeValue( wxString::Format( "%g", 35.0 ) );
    m_ExtTrackThicknessUnit->SetSelection( 1 );

    m_IntTrackWidthValue->ChangeValue( wxString::Format( "%g", 0.2 ) );
    m_TW_IntTrackWidth_choiceUnit->SetSelection( 0 );
    m_IntTrackThicknessValue->ChangeValue( wxString::Format( "%g", 35.0 ) );
    m_IntTrackThicknessUnit->SetSelection( 1 );

    m_TrackCurrentValue->SetValue( wxString::Format( "%g", 1.0 ) );
}


void PANEL_TRACK_WIDTH::TWDisplayValues( double aCurrent, double aExtWidth, double aIntWidth,
                                         double aExtThickness, double aIntThickness )
{
    wxString msg;

    // Show the current.
    if( m_TWMode != TW_MASTER_CURRENT )
    {
        msg.Printf( wxT( "%g" ), aCurrent );
        m_TrackCurrentValue->SetValue( msg );
    }

    // Load scale factors to convert into output units.
    double extScale = m_TW_ExtTrackWidth_choiceUnit->GetUnitScale();
    double intScale = m_TW_IntTrackWidth_choiceUnit->GetUnitScale();

    // Display the widths.
    if( m_TWMode != TW_MASTER_EXT_WIDTH )
    {
        msg.Printf( wxT( "%g" ), aExtWidth / extScale );
        m_ExtTrackWidthValue->SetValue( msg );
    }

    if( m_TWMode != TW_MASTER_INT_WIDTH )
    {
        msg.Printf( wxT( "%g" ), aIntWidth / intScale );
        m_IntTrackWidthValue->SetValue( msg );
    }

    // Display cross-sectional areas.
    msg.Printf( wxT( "%g" ), (aExtWidth * aExtThickness) / (extScale * extScale) );
    m_ExtTrackAreaValue->SetLabel( msg );
    msg.Printf( wxT( "%g" ), (aIntWidth * aIntThickness) / (intScale * intScale) );
    m_IntTrackAreaValue->SetLabel( msg );

    // Show area units.
    wxString strunit = m_TW_ExtTrackWidth_choiceUnit->GetUnitName();
    msg = strunit + wxT( "²" );
    m_extTrackAreaUnitLabel->SetLabel( msg );
    strunit = m_TW_IntTrackWidth_choiceUnit->GetUnitName();
    msg = strunit + wxT( "²" );
    m_intTrackAreaUnitLabel->SetLabel( msg );

    // Load resistivity and length of traces.
    double rho      = std::abs( DoubleFromString( m_TWResistivity->GetValue() ) );
    double trackLen = std::abs( DoubleFromString( m_TrackLengthValue->GetValue() ) );
    trackLen *= m_TW_CuLength_choiceUnit->GetUnitScale();

    // Calculate resistance.
    double extResistance = ( rho * trackLen ) / ( aExtWidth * aExtThickness );
    double intResistance = ( rho * trackLen ) / ( aIntWidth * aIntThickness );

    // Display resistance.
    msg.Printf( wxT( "%g" ), extResistance );
    m_ExtTrackResistValue->SetLabel( msg );
    msg.Printf( wxT( "%g" ), intResistance );
    m_IntTrackResistValue->SetLabel( msg );

    // Display voltage drop along trace.
    double extV = extResistance * aCurrent;
    msg.Printf( wxT( "%g" ), extV );
    m_ExtTrackVDropValue->SetLabel( msg );
    double intV = intResistance * aCurrent;
    msg.Printf( wxT( "%g" ), intV );
    m_IntTrackVDropValue->SetLabel( msg );

    // And power loss.
    msg.Printf( wxT( "%g" ), extV * aCurrent );
    m_ExtTrackLossValue->SetLabel( msg );
    msg.Printf( wxT( "%g" ), intV * aCurrent );
    m_IntTrackLossValue->SetLabel( msg );
}


void PANEL_TRACK_WIDTH::TWUpdateModeDisplay()
{
    wxFont labelfont;
    wxFont controlfont;

    // Set the font weight of the current.
    labelfont   = m_staticTextCurrent->GetFont();
    controlfont = m_TrackCurrentValue->GetFont();

    if( m_TWMode == TW_MASTER_CURRENT )
    {
        labelfont.SetWeight( wxFONTWEIGHT_BOLD );
        controlfont.SetWeight( wxFONTWEIGHT_BOLD );
    }
    else
    {
        labelfont.SetWeight( wxFONTWEIGHT_NORMAL );
        controlfont.SetWeight( wxFONTWEIGHT_NORMAL );
    }

    m_staticTextCurrent->SetFont( labelfont );
    m_TrackCurrentValue->SetFont( controlfont );

    // Set the font weight of the external track width.
    labelfont   = m_staticTextExtWidth->GetFont();
    controlfont = m_ExtTrackWidthValue->GetFont();

    if( m_TWMode == TW_MASTER_EXT_WIDTH )
    {
        labelfont.SetWeight( wxFONTWEIGHT_BOLD );
        controlfont.SetWeight( wxFONTWEIGHT_BOLD );
    }
    else
    {
        labelfont.SetWeight( wxFONTWEIGHT_NORMAL );
        controlfont.SetWeight( wxFONTWEIGHT_NORMAL );
    }

    m_staticTextExtWidth->SetFont( labelfont );
    m_ExtTrackWidthValue->SetFont( controlfont );

    // Set the font weight of the internal track width.
    labelfont   = m_staticTextIntWidth->GetFont();
    controlfont = m_IntTrackWidthValue->GetFont();

    if( m_TWMode == TW_MASTER_INT_WIDTH )
    {
        labelfont.SetWeight( wxFONTWEIGHT_BOLD );
        controlfont.SetWeight( wxFONTWEIGHT_BOLD );
    }
    else
    {
        labelfont.SetWeight( wxFONTWEIGHT_NORMAL );
        controlfont.SetWeight( wxFONTWEIGHT_NORMAL );
    }

    m_staticTextIntWidth->SetFont( labelfont );
    m_IntTrackWidthValue->SetFont( controlfont );

    // Text sizes have changed when the font weight was changes
    // So, run the page layout to reflect the changes
    GetSizer()->Layout();
}

/* calculate track width for external or internal layers
 *
 * Imax = 0.048 * dT^0.44 * A^0.725 for external layer
 * Imax = 0.024 * dT^0.44 * A^0.725 for internal layer
 * with A = area = aThickness * trackWidth ( in mils )
 * and dT = temperature rise in degree C
 * Of course we want to know trackWidth
 */
double PANEL_TRACK_WIDTH::TWCalculateWidth( double aCurrent, double aThickness, double aDeltaT_C,
                                            bool aUseInternalLayer )
{
    // Appropriate scale for requested layer.
    double scale = aUseInternalLayer ? 0.024 : 0.048;

    // aThickness is given in normalize units (in meters) and we need mil
    aThickness /= UNIT_MIL;

    /* formula is Imax = scale * dT^0.44 * A^0.725
     * or
     * log(Imax) = log(scale) + 0.44*log(dT) +(0.725*(log(aThickness) + log(trackWidth))
     * log(trackWidth) * 0.725 = log(Imax) - log(scale) - 0.44*log(dT) - 0.725*log(aThickness)
     */
    double dtmp = log( aCurrent ) - log( scale ) - 0.44 * log( aDeltaT_C ) - 0.725 * log( aThickness );
    dtmp /= 0.725;
    double trackWidth = exp( dtmp );

    trackWidth *= UNIT_MIL;     // We are using normalize units (sizes in meters) and we have mil
    return trackWidth;          // in meters
}


double PANEL_TRACK_WIDTH::TWCalculateCurrent( double aWidth, double aThickness, double aDeltaT_C,
                                              bool aUseInternalLayer )
{
    // Appropriate scale for requested layer.
    double scale = aUseInternalLayer ? 0.024 : 0.048;

    // Convert thickness and width to mils.
    aThickness /= UNIT_MIL;
    aWidth     /= UNIT_MIL;

    double area    = aThickness * aWidth;
    double current = scale * pow( aDeltaT_C, 0.44 ) * pow( area, 0.725 );

    return current;
}


void PANEL_TRACK_WIDTH::LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg )
{
    // Disable calculations while we initialise.
    m_TWNested = true;

    // Read parameter values.
    m_TrackCurrentValue->SetValue( aCfg->m_TrackWidth.current );
    m_TrackDeltaTValue->SetValue( aCfg->m_TrackWidth.delta_tc );
    m_TrackLengthValue->SetValue( aCfg->m_TrackWidth.track_len );
    m_TW_CuLength_choiceUnit->SetSelection( aCfg->m_TrackWidth.track_len_units );
#if 0   // the IPC formula is valid for copper traces, so we do not currently adjust the resistivity
    m_TWResistivity->SetValue( aCfg->m_TrackWidth.resistivity );
#else
    m_TWResistivity->SetValue( wxString::Format( "%g", copper_resistivity ) );
#endif
    m_ExtTrackWidthValue->SetValue( aCfg->m_TrackWidth.ext_track_width );
    m_TW_ExtTrackWidth_choiceUnit->SetSelection( aCfg->m_TrackWidth.ext_track_width_units );
    m_ExtTrackThicknessValue->SetValue( aCfg->m_TrackWidth.ext_track_thickness );
    m_ExtTrackThicknessUnit->SetSelection( aCfg->m_TrackWidth.ext_track_thickness_units );
    m_IntTrackWidthValue->SetValue( aCfg->m_TrackWidth.int_track_width );
    m_TW_IntTrackWidth_choiceUnit->SetSelection( aCfg->m_TrackWidth.int_track_width_units );
    m_IntTrackThicknessValue->SetValue( aCfg->m_TrackWidth.int_track_thickness );
    m_IntTrackThicknessUnit->SetSelection( aCfg->m_TrackWidth.int_track_thickness_units );

    if( tracks_width_versus_current_formula.StartsWith( "<!" ) )
        m_htmlWinFormulas->SetPage( tracks_width_versus_current_formula );
    else
    {
        wxString html_txt;
        ConvertMarkdown2Html( wxGetTranslation( tracks_width_versus_current_formula ), html_txt );
        m_htmlWinFormulas->SetPage( html_txt );
    }

    // Make sure the correct master mode is displayed.
    TWUpdateModeDisplay();

    // Enable calculations and perform the initial one.
    m_TWNested = false;

    wxCommandEvent dummy;
    OnTWParametersChanged( dummy );
}
