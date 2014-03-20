/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 1992-2011 Kicad Developers, see change_log.txt for contributors.
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
#define KEYWORD_TW_TRACK_COPPER_THICKNESS      wxT( "TW_Track_CopperThickness" )
#define KEYWORD_TW_TRACK_LEN                   wxT( "TW_Track_Len" )
#define KEYWORD_TW_TRACK_COPPER_THICKNESS_UNIT wxT( "TW_Track_CopperThickness_Unit" )
#define KEYWORD_TW_TRACK_LEN_UNIT              wxT( "TW_Track_Len_Unit" )
#define KEYWORD_TW_EXTTRACK_WIDTH_UNIT         wxT( "TW_ExtTrack_Width_Unit" )
#define KEYWORD_TW_INTTRACK_WIDTH_UNIT         wxT( "TW_IntTrack_Width_Unit" )

void PCB_CALCULATOR_FRAME::TW_WriteConfig()
{
    // Save current parameters values in config:
    m_Config->Write( KEYWORD_TW_EXTTRACK_WIDTH_UNIT, m_TW_ExtTrackWidth_choiceUnit->GetSelection() );
    m_Config->Write( KEYWORD_TW_INTTRACK_WIDTH_UNIT, m_TW_IntTrackWidth_choiceUnit->GetSelection() );
    m_Config->Write( KEYWORD_TW_TRACK_COPPER_THICKNESS_UNIT,
                    m_TW_CuThickness_choiceUnit->GetSelection() );
    m_Config->Write( KEYWORD_TW_TRACK_LEN_UNIT, m_TW_CuLength_choiceUnit->GetSelection() );
    m_Config->Write( KEYWORD_TW_CURRENT, m_TrackCurrentValue->GetValue() );
    m_Config->Write( KEYWORD_TW_DELTA_TC, m_TrackDeltaTValue->GetValue() );
    m_Config->Write( KEYWORD_TW_TRACK_COPPER_THICKNESS, m_TrackThicknessValue->GetValue() );
    m_Config->Write( KEYWORD_TW_TRACK_LEN, m_TrackLengthValue->GetValue() );
}


void PCB_CALCULATOR_FRAME::OnTWCalculateButt( wxCommandEvent& event )
{
    // Prepare parameters:
    double current   = std::abs( DoubleFromString( m_TrackCurrentValue->GetValue() ) );
    double thickness = std::abs( DoubleFromString( m_TrackThicknessValue->GetValue() ) );
    double deltaT_C  = std::abs( DoubleFromString( m_TrackDeltaTValue->GetValue() ) );
    double track_len = std::abs( DoubleFromString( m_TrackLengthValue->GetValue() ) );
    double extTrackWidth;
    double intTrackWidth;

    // Normalize units:
    thickness    *= m_TW_CuThickness_choiceUnit->GetUnitScale();
    track_len    *= m_TW_CuLength_choiceUnit->GetUnitScale();
    extTrackWidth = TWCalculate( current, thickness, deltaT_C, false );
    intTrackWidth = TWCalculate( current, thickness, deltaT_C, true );

    // Display values:
    wxString msg;
    msg.Printf( wxT( "%g" ), extTrackWidth / m_TW_ExtTrackWidth_choiceUnit->GetUnitScale() );
    m_ExtTrackWidthValue->SetValue( msg );
    msg.Printf( wxT( "%g" ), intTrackWidth / m_TW_IntTrackWidth_choiceUnit->GetUnitScale() );
    m_IntTrackWidthValue->SetValue( msg );

    // Display areas:
    double   scale = m_TW_ExtTrackWidth_choiceUnit->GetUnitScale();
    double   ext_area  = thickness * extTrackWidth;
    msg.Printf( wxT( "%g" ), ext_area / (scale * scale) );

    m_ExtTrackAreaValue->SetValue( msg );
    wxString strunit = m_TW_ExtTrackWidth_choiceUnit->GetUnitName();
    msg = strunit + wxT( " x " ) + strunit;
    m_ExtTrackAreaUnitLabel->SetLabel( msg );

    scale = m_TW_IntTrackWidth_choiceUnit->GetUnitScale();
    double   int_area  = thickness * intTrackWidth;
    msg.Printf( wxT( "%g" ), int_area / (scale * scale) );

    m_IntTrackAreaValue->SetValue( msg );
    strunit = m_TW_IntTrackWidth_choiceUnit->GetUnitName();
    msg = strunit + wxT( " x " ) + strunit;
    m_IntTrackAreaUnitLabel->SetLabel( msg );

    // Display resistance (assuming using copper ):
    double rho = 1.72e-008;     // In Ohms*meter
    double ext_res = rho / ext_area * track_len;
    msg.Printf( wxT( "%g" ), ext_res );
    m_ExtTrackResistValue->SetValue( msg );

    double int_res = rho / int_area * track_len;
    msg.Printf( wxT( "%g" ), int_res );
    m_IntTrackResistValue->SetValue( msg );

    // Display drop voltage
    double ext_drop_volt = ext_res * current;
    msg.Printf( wxT( "%g" ), ext_drop_volt );
    m_ExtTrackVDropValue->SetValue( msg );

    double int_drop_volt = int_res * current;
    msg.Printf( wxT( "%g" ), int_drop_volt );
    m_IntTrackVDropValue->SetValue( msg );

    // Display power loss
    double loss = ext_drop_volt * current;
    msg.Printf( wxT( "%g" ), loss );
    m_ExtTrackLossValue->SetValue( msg );

    loss = int_drop_volt * current;
    msg.Printf( wxT( "%g" ), loss );
    m_IntTrackLossValue->SetValue( msg );
}


/* calculate track width for external or internal layers
 *
 * Imax = 0.048 * dT^0.44 * A^0.725 for external layer
 * Imax = 0.024 * dT^0.44 * A^0.725 for internal layer
 * with A = area = aThickness * trackWidth ( in mils )
 * and dT = temperature rise in degree C
 * Of course we want to know trackWidth
 */
double PCB_CALCULATOR_FRAME::TWCalculate( double aCurrent, double aThickness, double aDeltaT_C,
                                          bool aUseInternalLayer )
{
    double scale = 0.048;    // We are using normalize units (sizes in meters)

    if( aUseInternalLayer )
        scale *= 0.024 / 0.048;

    // aThickness is given in normalize units (in meters) and we need mil
    aThickness /= UNIT_MIL;

    /* formula is Imax = scale * dT^0.44 * A^0.725
     * or
     * log(Imax) = log(scale) + 0.44*log(dT)
     *      +(0.725*(log(aThickness) + log(trackWidth))
     * log(trackWidth) * 0.725 = ln(Imax) - ln(scale) - 0.44*log(dT) - 0.725*log(aThickness)
     */
    double dtmp = log( aCurrent ) - log( scale ) - 0.44 * log( aDeltaT_C ) - 0.725 * log( aThickness );
    dtmp /= 0.725;
    double trackWidth = exp( dtmp );

    trackWidth *= UNIT_MIL;     // We are using normalize units (sizes in meters) and we have mil
    return trackWidth;          // in meters
}


void PCB_CALCULATOR_FRAME::TW_Init()
{
    int      tmp;
    wxString msg;

    // Read parameters values:
    m_Config->Read( KEYWORD_TW_EXTTRACK_WIDTH_UNIT, &tmp, 0 );
    m_TW_ExtTrackWidth_choiceUnit->SetSelection( tmp );
    m_Config->Read( KEYWORD_TW_INTTRACK_WIDTH_UNIT, &tmp, 0 );
    m_TW_IntTrackWidth_choiceUnit->SetSelection( tmp );
    m_Config->Read( KEYWORD_TW_TRACK_COPPER_THICKNESS_UNIT, &tmp, 0 );
    m_TW_CuThickness_choiceUnit->SetSelection( tmp );
    m_Config->Read( KEYWORD_TW_TRACK_LEN_UNIT, &tmp, 0 );
    m_TW_CuLength_choiceUnit->SetSelection( tmp );
    m_Config->Read( KEYWORD_TW_CURRENT, &msg, wxT( "1.0" ) );
    m_TrackCurrentValue->SetValue( msg );
    m_Config->Read( KEYWORD_TW_DELTA_TC, &msg, wxT( "10.0" ) );
    m_TrackDeltaTValue->SetValue( msg );
    m_Config->Read( KEYWORD_TW_TRACK_COPPER_THICKNESS, &msg, wxT( "0.035" ) );
    m_TrackThicknessValue->SetValue( msg );
    m_Config->Read( KEYWORD_TW_TRACK_LEN, &msg, wxT( "20" ) );
    m_TrackLengthValue->SetValue( msg );

    // Init formulas text
    m_htmlWinFormulas->AppendToPage( _( "<br><em>The formula (from IPC 2221) is:</em></br>" ) );
    m_htmlWinFormulas->AppendToPage( _( "<br><b>I = K * dT<sup>0.44</sup> * (W*H)<sup>0.725</sup></b></br>" ) );
    m_htmlWinFormulas->AppendToPage( _( "<br>Internal traces : <b>K = 0.024</b></br>" ) );
    m_htmlWinFormulas->AppendToPage( _( "<br>External traces: <b>K = 0.048</b></br>" ) );

    m_htmlWinFormulas->AppendToPage( _( "<br>where:</br>" ) );
    m_htmlWinFormulas->AppendToPage( _( "<br><b>I</b> = maximum current in Amps</br>" ) );
    m_htmlWinFormulas->AppendToPage( _( "<br><b>dT</b> = temperature rise above ambient in deg C</br>" ) );
    m_htmlWinFormulas->AppendToPage( _( "<br><b>W,H</b> = Width and Thickness in mils</br>" ) );
}
