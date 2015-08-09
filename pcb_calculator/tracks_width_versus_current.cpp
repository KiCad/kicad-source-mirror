/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 1992-2015 Kicad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/* see
 * http://www.desmith.net/NMdS/Electronics/TraceWidth.html
 * http://www.ultracad.com/articles/pcbtemp.pdf
 * for more info
 */

#include <cassert>
#include <cmath>
#include <wx/wx.h>
#include <wx/config.h>

#include <pcb_calculator_frame_base.h>

#include <pcb_calculator.h>
#include <UnitSelector.h>
#include <units_scales.h>

extern double DoubleFromString( const wxString& TextValue );

// Key words to read/write some parameters in config:
#define KEYWORD_TW_CURRENT                     wxT( "TW_Track_Current" )
#define KEYWORD_TW_DELTA_TC                    wxT( "TW_Delta_TC" )
#define KEYWORD_TW_TRACK_LEN                   wxT( "TW_Track_Len" )
#define KEYWORD_TW_TRACK_LEN_UNIT              wxT( "TW_Track_Len_Unit" )
#define KEYWORD_TW_RESISTIVITY                 wxT( "TW_Resistivity" )
#define KEYWORD_TW_EXTTRACK_WIDTH              wxT( "TW_ExtTrack_Width" )
#define KEYWORD_TW_EXTTRACK_WIDTH_UNIT         wxT( "TW_ExtTrack_Width_Unit" )
#define KEYWORD_TW_EXTTRACK_THICKNESS          wxT( "TW_ExtTrack_Thickness" )
#define KEYWORD_TW_EXTTRACK_THICKNESS_UNIT     wxT( "TW_ExtTrack_Thickness_Unit" )
#define KEYWORD_TW_INTTRACK_WIDTH              wxT( "TW_IntTrack_Width" )
#define KEYWORD_TW_INTTRACK_WIDTH_UNIT         wxT( "TW_IntTrack_Width_Unit" )
#define KEYWORD_TW_INTTRACK_THICKNESS          wxT( "TW_IntTrack_Thickness" )
#define KEYWORD_TW_INTTRACK_THICKNESS_UNIT     wxT( "TW_IntTrack_Thickness_Unit" )

void PCB_CALCULATOR_FRAME::TW_WriteConfig( wxConfigBase* aCfg )
{
    // Save current parameters values in config.
    aCfg->Write( KEYWORD_TW_CURRENT,                 m_TrackCurrentValue->GetValue() );
    aCfg->Write( KEYWORD_TW_DELTA_TC,                m_TrackDeltaTValue->GetValue() );
    aCfg->Write( KEYWORD_TW_TRACK_LEN,               m_TrackLengthValue->GetValue() );
    aCfg->Write( KEYWORD_TW_TRACK_LEN_UNIT,          m_TW_CuLength_choiceUnit->GetSelection() );
    aCfg->Write( KEYWORD_TW_RESISTIVITY,             m_TWResistivity->GetValue() );
    aCfg->Write( KEYWORD_TW_EXTTRACK_WIDTH,          m_ExtTrackWidthValue->GetValue() );
    aCfg->Write( KEYWORD_TW_EXTTRACK_WIDTH_UNIT,     m_TW_ExtTrackWidth_choiceUnit->GetSelection() );
    aCfg->Write( KEYWORD_TW_EXTTRACK_THICKNESS,      m_ExtTrackThicknessValue->GetValue() );
    aCfg->Write( KEYWORD_TW_EXTTRACK_THICKNESS_UNIT, m_ExtTrackThicknessUnit->GetSelection() );
    aCfg->Write( KEYWORD_TW_INTTRACK_WIDTH,          m_IntTrackWidthValue->GetValue() );
    aCfg->Write( KEYWORD_TW_INTTRACK_WIDTH_UNIT,     m_TW_IntTrackWidth_choiceUnit->GetSelection() );
    aCfg->Write( KEYWORD_TW_INTTRACK_THICKNESS,      m_IntTrackThicknessValue->GetValue() );
    aCfg->Write( KEYWORD_TW_INTTRACK_THICKNESS_UNIT, m_IntTrackThicknessUnit->GetSelection() );
}


void PCB_CALCULATOR_FRAME::OnTWParametersChanged( wxCommandEvent& event )
{
    switch(m_TWMode)
    {
    case TW_MASTER_CURRENT:
        OnTWCalculateFromCurrent( event );
        break;
    case TW_MASTER_EXT_WIDTH:
        OnTWCalculateFromExtWidth( event );
        break;
    case TW_MASTER_INT_WIDTH:
        OnTWCalculateFromIntWidth( event );
        break;
    default:
#ifdef DEBUG
        assert( false );
#endif
        break;
    }
}


void PCB_CALCULATOR_FRAME::OnTWCalculateFromCurrent( wxCommandEvent& event )
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
    double extTrackWidth;
    double intTrackWidth;

    // Normalize units.
    extThickness *= m_ExtTrackThicknessUnit->GetUnitScale();
    intThickness *= m_IntTrackThicknessUnit->GetUnitScale();

    // Calculate the widths.
    extTrackWidth = TWCalculateWidth( current, extThickness, deltaT_C, false );
    intTrackWidth = TWCalculateWidth( current, intThickness, deltaT_C, true );

    // Update the display.
    TWDisplayValues( current, extTrackWidth, intTrackWidth, extThickness, intThickness );

    // Re-enable the events.
    m_TWNested = false;
}


void PCB_CALCULATOR_FRAME::OnTWCalculateFromExtWidth( wxCommandEvent& event )
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


void PCB_CALCULATOR_FRAME::OnTWCalculateFromIntWidth( wxCommandEvent& event )
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


void PCB_CALCULATOR_FRAME::TWDisplayValues( double aCurrent, double aExtWidth,
        double aIntWidth, double aExtThickness, double aIntThickness )
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
    msg = strunit + wxT( " x " ) + strunit;
    m_ExtTrackAreaUnitLabel->SetLabel( msg );
    strunit = m_TW_IntTrackWidth_choiceUnit->GetUnitName();
    msg = strunit + wxT( " x " ) + strunit;
    m_IntTrackAreaUnitLabel->SetLabel( msg );

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


void PCB_CALCULATOR_FRAME::TWUpdateModeDisplay()
{
    wxFont labelfont;
    wxFont controlfont;

    // Set the font weight of the current.
    labelfont = m_staticTextCurrent->GetFont();
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
    labelfont = m_staticTextExtWidth->GetFont();
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
    labelfont = m_staticTextIntWidth->GetFont();
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
}

/* calculate track width for external or internal layers
 *
 * Imax = 0.048 * dT^0.44 * A^0.725 for external layer
 * Imax = 0.024 * dT^0.44 * A^0.725 for internal layer
 * with A = area = aThickness * trackWidth ( in mils )
 * and dT = temperature rise in degree C
 * Of course we want to know trackWidth
 */
double PCB_CALCULATOR_FRAME::TWCalculateWidth( double aCurrent, double aThickness, double aDeltaT_C,
                                          bool aUseInternalLayer )
{
    // Appropriate scale for requested layer.
    double scale = aUseInternalLayer ? 0.024 : 0.048;

    // aThickness is given in normalize units (in meters) and we need mil
    aThickness /= UNIT_MIL;

    /* formula is Imax = scale * dT^0.44 * A^0.725
     * or
     * log(Imax) = log(scale) + 0.44*log(dT)
     *      +(0.725*(log(aThickness) + log(trackWidth))
     * log(trackWidth) * 0.725 = log(Imax) - log(scale) - 0.44*log(dT) - 0.725*log(aThickness)
     */
    double dtmp = log( aCurrent ) - log( scale ) - 0.44 * log( aDeltaT_C ) - 0.725 * log( aThickness );
    dtmp /= 0.725;
    double trackWidth = exp( dtmp );

    trackWidth *= UNIT_MIL;     // We are using normalize units (sizes in meters) and we have mil
    return trackWidth;          // in meters
}


double PCB_CALCULATOR_FRAME::TWCalculateCurrent( double aWidth, double aThickness, double aDeltaT_C,
                                                 bool aUseInternalLayer )
{
    // Appropriate scale for requested layer.
    double scale = aUseInternalLayer ? 0.024 : 0.048;

    // Convert thickness and width to mils.
    aThickness /= UNIT_MIL;
    aWidth     /= UNIT_MIL;

    double area = aThickness * aWidth;
    double current = scale * pow( aDeltaT_C, 0.44 ) * pow( area, 0.725 );
    return current;
}


void PCB_CALCULATOR_FRAME::TW_Init( wxConfigBase* aCfg )
{
    int      tmp;
    wxString msg;

    // Disable calculations while we initialise.
    m_TWNested = true;

    // Read parameter values.
    aCfg->Read( KEYWORD_TW_CURRENT,                 &msg, wxT( "1.0" ) );
    m_TrackCurrentValue->SetValue( msg );
    aCfg->Read( KEYWORD_TW_DELTA_TC,                &msg, wxT( "10.0" ) );
    m_TrackDeltaTValue->SetValue( msg );
    aCfg->Read( KEYWORD_TW_TRACK_LEN,               &msg, wxT( "20" ) );
    m_TrackLengthValue->SetValue( msg );
    aCfg->Read( KEYWORD_TW_TRACK_LEN_UNIT,          &tmp, 0 );
    m_TW_CuLength_choiceUnit->SetSelection( tmp );
    aCfg->Read( KEYWORD_TW_RESISTIVITY,             &msg, wxT( "1.72e-8" ) );
    m_TWResistivity->SetValue( msg );
    aCfg->Read( KEYWORD_TW_EXTTRACK_WIDTH,          &msg, wxT( "0.2" ) );
    m_ExtTrackWidthValue->SetValue( msg );
    aCfg->Read( KEYWORD_TW_EXTTRACK_WIDTH_UNIT,     &tmp, 0 );
    m_TW_ExtTrackWidth_choiceUnit->SetSelection( tmp );
    aCfg->Read( KEYWORD_TW_EXTTRACK_THICKNESS,      &msg, wxT( "0.035" ) );
    m_ExtTrackThicknessValue->SetValue( msg );
    aCfg->Read( KEYWORD_TW_EXTTRACK_THICKNESS_UNIT, &tmp, 0 );
    m_ExtTrackThicknessUnit->SetSelection( tmp );
    aCfg->Read( KEYWORD_TW_INTTRACK_WIDTH,          &msg, wxT( "0.2" ) );
    m_IntTrackWidthValue->SetValue( msg );
    aCfg->Read( KEYWORD_TW_INTTRACK_WIDTH_UNIT,     &tmp, 0 );
    m_TW_IntTrackWidth_choiceUnit->SetSelection( tmp );
    aCfg->Read( KEYWORD_TW_INTTRACK_THICKNESS,      &msg, wxT( "0.035" ) );
    m_IntTrackThicknessValue->SetValue( msg );
    aCfg->Read( KEYWORD_TW_INTTRACK_THICKNESS_UNIT, &tmp, 0 );
    m_IntTrackThicknessUnit->SetSelection( tmp );

    // Init formulas text
    msg = "<br>";
    msg << _( "If you specify the maximum current, then the trace "
              "widths will be calculated to suit." )
        << "<br>" << _( "If you specify one of the trace widths, the maximum "
                        "current it can handle will be calculated. The width "
                        "for the other trace to also handle this current will "
                        "then be calculated." )
        << "<br>" << _( "The controlling value is shown in bold." ) << "<br><br>"
        <<  _( "The calculations are valid for currents up to 35A "
               "(external) or 17.5A (internal), temperature rises "
               "up to 100 deg C, and widths of up to 400mil (10mm)." )<< "<br>"
        << _( "The formula, from IPC 2221, is" )
        << "<center><b>I = K * dT<sup>0.44</sup> * (W*H)<sup>0.725</sup></b></center>"
        << _( "where:" ) << "<br><b>I</b> = "
        << _( "maximum current in amps" )
        << "<br><b>dT</b> = "
        << _( "temperature rise above ambient in deg C" )
        << "<br><b>W,H</b> = "
        << _( "width and thickness in mils" ) << "<br>"
        << "<b>K</b> = "
        << _( "0.024 for internal traces or 0.048 for external traces" );

    m_htmlWinFormulas->AppendToPage( msg );

    // Make sure the correct master mode is displayed.
    TWUpdateModeDisplay();

    // Enable calculations and perform the initial one.
    m_TWNested = false;
    wxCommandEvent dummy;
    OnTWParametersChanged( dummy );
}
