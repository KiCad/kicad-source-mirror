/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <dialog_sim_format_value.h>
#include <sim/spice_value.h>
#include <core/kicad_algo.h>


DIALOG_SIM_FORMAT_VALUE::DIALOG_SIM_FORMAT_VALUE( wxWindow* aParent, SPICE_VALUE_FORMAT* aFormat ) :
        DIALOG_SIM_FORMAT_VALUE_BASE( aParent ),
        m_format( aFormat )
{
    OptOut( this );

    if( aFormat->Range.EndsWith( wxS( "V" ) ) )
    {
        m_units = aFormat->Range.Right( 1 );
        SetTitle( wxString::Format( GetTitle(), _( "Voltage" ) ) );
    }
    else if( aFormat->Range.EndsWith( wxS( "A" ) ) )
    {
        m_units = aFormat->Range.Right( 1 );
        SetTitle( wxString::Format( GetTitle(), _( "Current" ) ) );
    }
    else if( aFormat->Range.EndsWith( wxS( "s" ) ) )
    {
        m_units = aFormat->Range.Right( 1 );
        SetTitle( wxString::Format( GetTitle(), _( "Time" ) ) );
    }
    else if( aFormat->Range.EndsWith( wxS( "Hz" ) ) )
    {
        m_units = aFormat->Range.Right( 2 );
        SetTitle( wxString::Format( GetTitle(), _( "Frequency" ) ) );
    }
    else if( aFormat->Range.EndsWith( wxS( "dB" ) ) )
    {
        m_units = aFormat->Range.Right( 3 );
        SetTitle( wxString::Format( GetTitle(), _( "Gain" ) ) );
    }
    else if( aFormat->Range.EndsWith( wxS( "°" ) ) )
    {
        m_units = aFormat->Range.Right( 1 );
        SetTitle( wxString::Format( GetTitle(), _( "Phase" ) ) );
    }
    else if( aFormat->Range.StartsWith( wxS( "~" ), &m_units ) )
    {
        // m_units set as remainder in StartsWith() call....
        SetTitle( wxString::Format( GetTitle(), _( "Value" ) ) );
    }
    else
    {
        if( SPICE_VALUE::ParseSIPrefix( aFormat->Range.GetChar( 0 ) ) != SPICE_VALUE::PFX_NONE )
            m_units = aFormat->Range.Right( aFormat->Range.Length() - 1 );
        else
            m_units = aFormat->Range;

        SetTitle( wxString::Format( GetTitle(), _( "Value" ) ) );
    }

    m_precisionCtrl->SetValue( aFormat->Precision );

    for( int ii = 1; ii < (int) m_rangeCtrl->GetCount(); ++ii )
        m_rangeCtrl->SetString( ii, m_rangeCtrl->GetString( ii ) + m_units );

    if( aFormat->Range.GetChar( 0 ) == '~' )
        m_rangeCtrl->SetSelection( 0 );
    else
        m_rangeCtrl->SetStringSelection( aFormat->Range );
}


bool DIALOG_SIM_FORMAT_VALUE::TransferDataFromWindow()
{
    m_format->Precision = std::clamp( m_precisionCtrl->GetValue(), 1, 9 );

    if( m_rangeCtrl->GetSelection() == 0 )
        m_format->Range = wxS( "~" ) + m_units;
    else
        m_format->Range = m_rangeCtrl->GetStringSelection();

    return true;
}

