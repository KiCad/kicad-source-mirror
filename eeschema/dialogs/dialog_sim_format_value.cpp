/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "sim/spice_value.h"


DIALOG_SIM_FORMAT_VALUE::DIALOG_SIM_FORMAT_VALUE( wxWindow* aParent, int* aPrecision,
                                                  wxString* aRange ) :
        DIALOG_SIM_FORMAT_VALUE_BASE( aParent ),
        m_precision( aPrecision ),
        m_range( aRange )
{
    m_units = aRange->Right( 1 );

    if( m_units == wxS( "V" ) )
    {
        SetTitle( wxString::Format( GetTitle(), _( "Voltage" ) ) );
    }
    else if( m_units == wxS( "A" ) )
    {
        SetTitle( wxString::Format( GetTitle(), _( "Current" ) ) );
    }
    else if( m_units == wxS( "s" ) )
    {
        SetTitle( wxString::Format( GetTitle(), _( "Time" ) ) );
    }
    else if( aRange->Right( 2 ) == wxS( "Hz" ) )
    {
        m_units = aRange->Right( 2 );
        SetTitle( wxString::Format( GetTitle(), _( "Frequency" ) ) );
    }
    else if( aRange->Right( 3 ) == wxS( "dBV" ) )
    {
        m_units = aRange->Right( 3 );
        SetTitle( wxString::Format( GetTitle(), _( "Gain" ) ) );
    }
    else if( m_units == wxS( "°" ) )
    {
        SetTitle( wxString::Format( GetTitle(), _( "Phase" ) ) );
    }
    else
    {
        if( aRange->GetChar( 0 ) == '~' )
            m_units = aRange->Right( aRange->Length() - 1 );
        else if( SPICE_VALUE::ParseSIPrefix( aRange->GetChar( 0 ) ) != SPICE_VALUE::PFX_NONE )
            m_units = aRange->Right( aRange->Length() - 1 );
        else
            m_units = *aRange;

        SetTitle( wxString::Format( GetTitle(), _( "Value" ) ) );
    }

    m_precisionCtrl->SetValue( *aPrecision );

    for( int ii = 1; ii < (int) m_rangeCtrl->GetCount(); ++ii )
        m_rangeCtrl->SetString( ii, m_rangeCtrl->GetString( ii ) + m_units );

    if( aRange->GetChar( 0 ) == '~' )
        m_rangeCtrl->SetSelection( 0 );
    else
        m_rangeCtrl->SetStringSelection( *aRange );
}


bool DIALOG_SIM_FORMAT_VALUE::TransferDataFromWindow()
{
    *m_precision = m_precisionCtrl->GetValue();

    if( m_rangeCtrl->GetSelection() == 0 )
        *m_range = wxS( "~" ) + m_units;
    else
        *m_range = m_rangeCtrl->GetStringSelection();

    return true;
}

